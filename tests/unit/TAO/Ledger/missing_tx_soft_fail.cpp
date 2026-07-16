/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>

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


    /** MissingBlock
     *
     *  A minimal Block subclass whose Check() passes but populates vMissing,
     *  simulating a valid block that is temporarily missing a transaction. This
     *  is the soft-fail "incomplete" condition handled by Process().
     */
    class MissingBlock final : public TAO::Ledger::Block
    {
    public:
        MissingBlock()
        : TAO::Ledger::Block()
        {
        }

        MissingBlock* Clone() const override
        {
            return new MissingBlock(*this);
        }

        bool Check(bool /*fForceProof*/ = false) const override
        {
            /* Simulate a block whose single vtx is not yet in LLD. Check()
             * itself succeeds; the missing transaction is reported via vMissing
             * and handled as a soft incomplete condition. */
            vMissing.clear();
            vMissing.push_back(std::make_pair(
                TAO::Ledger::TRANSACTION::TRITIUM, uint512_t(0xdeadbeef)));

            return true;
        }

        bool Accept() const override
        {
            return false;
        }
    };


    /** PassBlock
     *
     *  A Block subclass whose Check() and Accept() both succeed with no missing
     *  transactions, so that Process() reaches the ACCEPTED path.
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
            vMissing.clear();
            return true;
        }

        bool Accept() const override
        {
            return true;
        }
    };
}


TEST_CASE("Missing transactions yield a soft INCOMPLETE, never a REJECT", "[ledger][process]")
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

    /* Create the missing-tx block referencing the known prev hash. */
    MissingBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 2;
    block.nBits         = 1;
    block.nNonce        = 42;

    const uint1024_t hashBlock = block.GetHash();

    /* Ensure the block hash itself is NOT already in LLD (not a duplicate). */
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashBlock));

    /* Clear any stale retry state from previous tests. */
    TAO::Ledger::mapLastMissing.clear();

    /* Drive the block through Process() up to the retry limit. Each iteration
     * should return INCOMPLETE (never REJECTED) and increment the retry
     * counter keyed by the block hash. */
    for(uint64_t i = 1; i <= LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES; ++i)
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        INFO("iteration " << i);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock));
        REQUIRE(TAO::Ledger::mapLastMissing[hashBlock] == i);
    }

    /* The next call exceeds the retry limit: still INCOMPLETE (not REJECTED),
     * but the block's missing-tx requests are cleared so we stop re-requesting
     * a permanently unresolvable transaction. */
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
        REQUIRE(block.vMissing.empty());
        REQUIRE(block.hashMissing == 0);
    }

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    LLD::Ledger->EraseBlock(hashBlock);
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Missing-transaction retry counter clears on successful ACCEPT", "[ledger][process]")
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

    /* Create a block that is temporarily missing a transaction. */
    MissingBlock missingBlock;
    missingBlock.nVersion      = 4;
    missingBlock.hashPrevBlock = hashPrev;
    missingBlock.nChannel      = 2;
    missingBlock.nHeight       = 11;
    missingBlock.nBits         = 1;
    missingBlock.nNonce        = 99;

    const uint1024_t hashMissing = missingBlock.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashMissing));

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapOrphans.clear();

    /* Drive a few INCOMPLETE results to build up the counter. */
    for(uint32_t i = 0; i < 3; ++i)
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(missingBlock, nStatus);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    }
    REQUIRE(TAO::Ledger::mapLastMissing[hashMissing] == 3);

    /* Now process a block that passes Check()/Accept() with no missing tx and
     * confirm its retry counter is cleared on accept. */
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
    TAO::Ledger::mapLastMissing[hashGood] = 5;

    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(goodBlock, nStatus);

        REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED) != 0);
        /* The counter for this block should have been cleared on accept. */
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashGood) == 0);
    }

    /* The missing block's counter should still be there (untouched by the good
     * accept). */
    REQUIRE(TAO::Ledger::mapLastMissing[hashMissing] == 3);

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("mapLastMissing is bounded at MAX_MISSING_MAP_ENTRIES", "[ledger][process]")
{
    LedgerGuard env;

    /* Pre-fill the map to the cap by direct insertion so the test runs in
     * microseconds rather than calling Process() MAX_MISSING_MAP_ENTRIES times. */
    TAO::Ledger::mapLastMissing.clear();
    const uint64_t CAP = TAO::Ledger::MAX_MISSING_MAP_ENTRIES;
    for(uint64_t i = 1; i <= CAP; ++i)
        TAO::Ledger::mapLastMissing[uint1024_t(i)] = 1;

    REQUIRE(TAO::Ledger::mapLastMissing.size() == CAP);

    /* Write a prev block so the new missing block doesn't take the orphan path. */
    const uint1024_t hashPrevCap(0xC0DE12345ULL);

    TAO::Ledger::BlockState statePrevCap;
    statePrevCap.nVersion      = 4;
    statePrevCap.hashPrevBlock = uint1024_t(0);
    statePrevCap.nChannel      = 2;
    statePrevCap.nHeight       = 30;
    statePrevCap.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrevCap, statePrevCap));

    /* A missing block whose hash is NOT one of the 10 000 pre-filled keys. */
    MissingBlock capBlock;
    capBlock.nVersion      = 4;
    capBlock.hashPrevBlock = hashPrevCap;
    capBlock.nChannel      = 2;
    capBlock.nHeight       = 31;
    capBlock.nBits         = 1;
    capBlock.nNonce        = 7777;

    const uint1024_t hashCap = capBlock.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashCap));

    /* Process the new missing block: should trigger a clear then insert 1 entry. */
    uint8_t nStatus = 0;
    TAO::Ledger::Process(capBlock, nStatus);

    REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);

    /* After the cap-clear the map must contain only the single new entry. */
    REQUIRE(TAO::Ledger::mapLastMissing.size() == 1);
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashCap) == 1);
    REQUIRE(TAO::Ledger::mapLastMissing[hashCap] == 1);

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    LLD::Ledger->EraseBlock(hashPrevCap);
}


TEST_CASE("Orphan-drain missing tx sets hashMissing to orphan's own hash", "[ledger][process]")
{
    LedgerGuard env;

    TAO::Ledger::mapOrphans.clear();
    TAO::Ledger::mapLastMissing.clear();

    /* Write a prev block so the accepted parent block doesn't take the orphan
     * path when Process() is called with it. */
    const uint1024_t hashPrevOrphan(0xABCD5600ULL);

    TAO::Ledger::BlockState statePrevOrphan;
    statePrevOrphan.nVersion      = 4;
    statePrevOrphan.hashPrevBlock = uint1024_t(0);
    statePrevOrphan.nChannel      = 2;
    statePrevOrphan.nHeight       = 40;
    statePrevOrphan.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrevOrphan, statePrevOrphan));

    /* Build the accepted parent PassBlock so we can determine its hash. */
    PassBlock parentBlock;
    parentBlock.nVersion      = 4;
    parentBlock.hashPrevBlock = hashPrevOrphan;
    parentBlock.nChannel      = 2;
    parentBlock.nHeight       = 41;
    parentBlock.nBits         = 1;
    parentBlock.nNonce        = 2001;

    const uint1024_t hashParent = parentBlock.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashParent));

    /* Build the orphan MissingBlock whose parent is the just-accepted block. */
    MissingBlock orphanBlock;
    orphanBlock.nVersion      = 4;
    orphanBlock.hashPrevBlock = hashParent;   /* parent = accepted block */
    orphanBlock.nChannel      = 2;
    orphanBlock.nHeight       = 42;
    orphanBlock.nBits         = 1;
    orphanBlock.nNonce        = 3001;

    const uint1024_t hashOrphan = orphanBlock.GetHash();

    /* Seed mapOrphans: keyed by the orphan's parent hash (= hashParent). */
    TAO::Ledger::mapOrphans.insert(
        std::make_pair(hashParent,
        std::unique_ptr<TAO::Ledger::Block>(orphanBlock.Clone())));

    /* Process the parent block: it is accepted, then the orphan-drain kicks in
     * and finds the orphan has missing transactions. */
    uint8_t nStatus = 0;
    TAO::Ledger::Process(parentBlock, nStatus);

    /* Parent was accepted, orphan is incomplete. */
    REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);

    /* hashMissing must be the ORPHAN's own hash, not the parent/map key. */
    REQUIRE(parentBlock.hashMissing == hashOrphan);
    REQUIRE(parentBlock.hashMissing != hashParent);

    /* Retry counter should have been started for the orphan's hash. */
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashOrphan) == 1);
    REQUIRE(TAO::Ledger::mapLastMissing[hashOrphan] == 1);

    /* Clean up. */
    TAO::Ledger::mapOrphans.clear();
    TAO::Ledger::mapLastMissing.clear();
    LLD::Ledger->EraseBlock(hashPrevOrphan);
}


TEST_CASE("Persisted orphan is removed while draining", "[ledger][process]")
{
    LedgerGuard env;

    TAO::Ledger::mapOrphans.clear();

    const uint1024_t hashPrev(0xABCD5700ULL);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 50;
    statePrev.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    PassBlock parentBlock;
    parentBlock.nVersion      = 4;
    parentBlock.hashPrevBlock = hashPrev;
    parentBlock.nChannel      = 2;
    parentBlock.nHeight       = 51;
    parentBlock.nBits         = 1;
    parentBlock.nNonce        = 4001;

    const uint1024_t hashParent = parentBlock.GetHash();

    PassBlock orphanBlock;
    orphanBlock.nVersion      = 4;
    orphanBlock.hashPrevBlock = hashParent;
    orphanBlock.nChannel      = 2;
    orphanBlock.nHeight       = 52;
    orphanBlock.nBits         = 1;
    orphanBlock.nNonce        = 4002;

    const uint1024_t hashOrphan = orphanBlock.GetHash();
    TAO::Ledger::BlockState stateOrphan;
    stateOrphan.nVersion      = orphanBlock.nVersion;
    stateOrphan.hashPrevBlock = orphanBlock.hashPrevBlock;
    stateOrphan.nChannel      = orphanBlock.nChannel;
    stateOrphan.nHeight       = orphanBlock.nHeight;
    stateOrphan.nBits         = orphanBlock.nBits;
    REQUIRE(LLD::Ledger->WriteBlock(hashOrphan, stateOrphan));

    TAO::Ledger::mapOrphans.insert(
        std::make_pair(hashParent,
        std::unique_ptr<TAO::Ledger::Block>(orphanBlock.Clone())));

    uint8_t nStatus = 0;
    TAO::Ledger::Process(parentBlock, nStatus);

    REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED) != 0);
    REQUIRE(TAO::Ledger::mapOrphans.empty());

    LLD::Ledger->EraseBlock(hashOrphan);
    LLD::Ledger->EraseBlock(hashPrev);
}
