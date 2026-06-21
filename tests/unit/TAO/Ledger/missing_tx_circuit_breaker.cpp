/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/args.h>
#include <Util/include/filesystem.h>

#include <unit/catch2/catch.hpp>


namespace
{
    /*  Lightweight guard that creates a temporary LedgerDB when the global
     *  test suite hasn't initialised one (e.g. when running only [process]
     *  tagged tests in isolation).  On destruction it deletes only what it
     *  created, leaving the global state untouched if it was already set up. */
    struct LedgerGuard
    {
        bool ownedLedger{false};

        LedgerGuard()
        {
            config::fTestNet.store(true);
            config::mapArgs["-testnet"] = "1";

            if(!LLD::Ledger)
            {
                LLD::Ledger = new LLD::LedgerDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
                ownedLedger = true;
            }
        }

        ~LedgerGuard()
        {
            if(ownedLedger)
            {
                delete LLD::Ledger;
                LLD::Ledger = nullptr;
            }
        }
    };


    /** StubBlock
     *
     *  A minimal Block subclass whose Check() always populates vMissing and
     *  returns false, simulating a block that references a permanently
     *  unresolvable missing transaction.
     */
    class StubBlock final : public TAO::Ledger::Block
    {
    public:
        StubBlock()
        : TAO::Ledger::Block()
        {
        }

        StubBlock* Clone() const override
        {
            return new StubBlock(*this);
        }

        bool Check(bool /*fForceProof*/ = false) const override
        {
            /* Simulate a block whose single vtx can never be found in LLD. */
            vMissing.clear();
            vMissing.push_back(std::make_pair(
                TAO::Ledger::TRANSACTION::TRITIUM, uint512_t(0xdeadbeef)));

            return false;
        }

        bool Accept() const override
        {
            return false;
        }
    };


    /** PassBlock
     *
     *  A Block subclass whose Check() and Accept() both return true, so that
     *  Process() can reach the ACCEPTED path and clear the circuit-breaker
     *  counter.
     */
    class PassBlock final : public TAO::Ledger::Block
    {
    public:
        PassBlock()
        : TAO::Ledger::Block()
        {
        }

        PassBlock* Clone() const override
        {
            return new PassBlock(*this);
        }

        bool Check(bool /*fForceProof*/ = false) const override
        {
            return true;
        }

        bool Accept() const override
        {
            return true;
        }
    };
}


TEST_CASE("Circuit-breaker trips after MAX_MISSING_RETRIES", "[ledger][process]")
{
    LedgerGuard env;

    /* Use a unique prev-block hash for this test so we don't collide with
     * other tests.  Write a minimal BlockState to LLD so Process() does not
     * take the orphan path. */
    const uint1024_t hashPrev(0x1234abcd);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 1;
    statePrev.nBits         = 1;

    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    /* Create the stub block referencing the known prev hash. */
    StubBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 2;
    block.nBits         = 1;
    block.nNonce        = 42;

    const uint1024_t hashBlock = block.GetHash();

    /* Ensure the block hash itself is NOT already in LLD (not a duplicate). */
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashBlock));

    /* Clear any stale circuit-breaker state from previous tests. */
    auto& mapAttempts = TAO::Ledger::GetMissingAttempts();
    mapAttempts.clear();

    /* Drive the block through Process() MAX_MISSING_RETRIES - 1 times.
     * Each iteration should return INCOMPLETE (the tx can't be found). */
    for(uint32_t i = 1; i < TAO::Ledger::MAX_MISSING_RETRIES; ++i)
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        INFO("iteration " << i);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
        REQUIRE(mapAttempts.count(hashBlock));
        REQUIRE(mapAttempts[hashBlock] == i);
    }

    /* The next call should trip the circuit-breaker: REJECTED, not INCOMPLETE. */
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   != 0);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) == 0);

        /* The attempt counter for this block should have been cleaned up. */
        REQUIRE(mapAttempts.count(hashBlock) == 0);
    }

    /* Clean up: erase the fake block state from LLD. */
    LLD::Ledger->EraseBlock(hashBlock);
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Circuit-breaker counter clears on successful ACCEPT", "[ledger][process]")
{
    LedgerGuard env;

    /* Set up a prev block in LLD. */
    const uint1024_t hashPrev(0x5678ef01);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 10;
    statePrev.nBits         = 1;

    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    /* Create a stub block that always fails with missing tx. */
    StubBlock badBlock;
    badBlock.nVersion      = 4;
    badBlock.hashPrevBlock = hashPrev;
    badBlock.nChannel      = 2;
    badBlock.nHeight       = 11;
    badBlock.nBits         = 1;
    badBlock.nNonce        = 99;

    const uint1024_t hashBad = badBlock.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashBad));

    auto& mapAttempts = TAO::Ledger::GetMissingAttempts();
    mapAttempts.clear();
    TAO::Ledger::mapOrphans.clear();

    /* Drive a few INCOMPLETE results to build up the counter. */
    for(uint32_t i = 0; i < 3; ++i)
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(badBlock, nStatus);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    }
    REQUIRE(mapAttempts[hashBad] == 3);

    /* Now process a block that passes Check() and Accept(), confirming
     * the counter for the accepted block is cleared. */
    PassBlock goodBlock;
    goodBlock.nVersion      = 4;
    goodBlock.hashPrevBlock = hashPrev;
    goodBlock.nChannel      = 2;
    goodBlock.nHeight       = 11;
    goodBlock.nBits         = 1;
    goodBlock.nNonce        = 100;

    const uint1024_t hashGood = goodBlock.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashGood));

    /* Artificially add a counter for the good block's hash. */
    mapAttempts[hashGood] = 5;

    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(goodBlock, nStatus);

        REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED) != 0);
        /* The counter for this block should have been cleared on accept. */
        REQUIRE(mapAttempts.count(hashGood) == 0);
    }

    /* The bad block's counter should still be there (untouched by the good accept). */
    REQUIRE(mapAttempts[hashBad] == 3);

    /* Clean up. */
    mapAttempts.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Circuit-breaker evicts orphan on trip", "[ledger][process]")
{
    LedgerGuard env;

    /* Set up a prev block in LLD. */
    const uint1024_t hashPrev(0xaabb0011);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 20;
    statePrev.nBits         = 1;

    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    /* Create the failing block. */
    StubBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 21;
    block.nBits         = 1;
    block.nNonce        = 77;

    const uint1024_t hashBlock = block.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashBlock));

    auto& mapAttempts = TAO::Ledger::GetMissingAttempts();
    mapAttempts.clear();

    /* Plant the block in the orphan map under both its prev hash and its own
     * hash to verify eviction covers both keys. */
    TAO::Ledger::mapOrphans[hashBlock] =
        std::unique_ptr<TAO::Ledger::Block>(block.Clone());
    TAO::Ledger::mapOrphans[block.hashPrevBlock] =
        std::unique_ptr<TAO::Ledger::Block>(block.Clone());

    REQUIRE(TAO::Ledger::mapOrphans.count(hashBlock));
    REQUIRE(TAO::Ledger::mapOrphans.count(block.hashPrevBlock));

    /* Pre-set the counter to one below the threshold so the next call trips. */
    mapAttempts[hashBlock] = TAO::Ledger::MAX_MISSING_RETRIES - 1;

    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED) != 0);
    }

    /* Both orphan entries should have been evicted. */
    REQUIRE(TAO::Ledger::mapOrphans.count(hashBlock) == 0);
    REQUIRE(TAO::Ledger::mapOrphans.count(block.hashPrevBlock) == 0);

    /* The attempt counter should have been cleaned up. */
    REQUIRE(mapAttempts.count(hashBlock) == 0);

    /* Clean up. */
    LLD::Ledger->EraseBlock(hashPrev);
}
