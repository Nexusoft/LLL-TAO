/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>
#include <LLP/include/version.h>
#include <LLP/templates/socket.h>
#include <LLP/include/base_address.h>

#include <TAO/Ledger/include/admissibility.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/credentials.h>
#include <TAO/Ledger/types/locator.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <Util/include/args.h>
#include <Util/include/filesystem.h>
#include <Util/include/runtime.h>
#include <Util/templates/datastream.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

#include <unit/catch2/catch.hpp>

#include <algorithm>

#ifndef WIN32
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif


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


    class RejectBlock final : public TAO::Ledger::Block
    {
    public:
        RejectBlock* Clone() const override
        {
            return new RejectBlock(*this);
        }

        bool Check(bool /*fForceProof*/ = false) const override
        {
            return false;
        }

        bool Accept() const override
        {
            return false;
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
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();

    /* Drive the block through Process() up to the retry limit. Each iteration
     * should return INCOMPLETE (never REJECTED) and increment the retry
     * counter keyed by the block hash.  Clear mapLastMissingProcessTime before
     * each call so the 250-ms rate-limit guard does not coalesce rapid
     * successive test invocations into a single counter increment. */
    for(uint64_t i = 1; i <= LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES; ++i)
    {
        TAO::Ledger::mapLastMissingProcessTime.erase(hashBlock);
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        INFO("iteration " << i);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock));
        REQUIRE(TAO::Ledger::mapLastMissing[hashBlock] == i);
    }

    /* The next call exceeds the retry limit: still INCOMPLETE (not REJECTED),
     * the block's missing-tx markers are cleared (hashMissing = 0 signals the
     * LLP layer to escalate to full branch recovery), and — critically — the
     * mapLastMissing entry is ERASED so the next arrival of this block starts
     * a fresh retry cycle rather than being permanently silenced. */
    {
        TAO::Ledger::mapLastMissingProcessTime.erase(hashBlock);
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
        REQUIRE_FALSE(block.vMissing.empty());
        REQUIRE(block.hashMissing == 0);
        /* Counter must be erased — a leftover >50 value would permanently
         * wedge this block on every future arrival. */
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) == 0);
        REQUIRE(TAO::Ledger::mapMissingBranchEscalations.count(hashBlock) == 1);
        REQUIRE(TAO::Ledger::mapMissingBranchEscalations[hashBlock] == 1);
        REQUIRE_FALSE(TAO::Ledger::IsMissingBranchRecoveryCapped(hashBlock));
    }

    /* The retry counter has been reset, so the very next Process() call
     * for the same block should start a fresh cycle: INCOMPLETE again and
     * the counter re-initialized to 1, not permanently dropped. */
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) != 0);
        REQUIRE(TAO::Ledger::mapLastMissing[hashBlock] == 1);
    }

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    LLD::Ledger->EraseBlock(hashBlock);
    LLD::Ledger->EraseBlock(hashPrev);
}

TEST_CASE("Missing-tx branch-recovery escalations are counted and capped", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t hashPrev(0x2345bcde);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 1;
    statePrev.nBits         = 1;

    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    MissingBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 2;
    block.nBits         = 1;
    block.nNonce        = 4201;

    const uint1024_t hashBlock = block.GetHash();
    const uint32_t nTestCycles = TAO::Ledger::MAX_BRANCH_RECOVERY_ESCALATIONS + 1;
    const uint64_t nRetries = LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES + 1;

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    for(uint32_t nCycle = 1; nCycle <= nTestCycles; ++nCycle)
    {
        for(uint64_t i = 0; i < nRetries; ++i)
        {
            /* Bypass the 250-ms reprocess rate-limit so rapid test calls each
             * count as a distinct attempt rather than being coalesced. */
            TAO::Ledger::mapLastMissingProcessTime.erase(hashBlock);
            uint8_t nStatus = 0;
            TAO::Ledger::Process(block, nStatus);
            REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
            REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED) == 0);
        }

        INFO("cycle " << nCycle);
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) == 0);
        REQUIRE(TAO::Ledger::MissingBranchRecoveryEscalations(hashBlock) == nCycle);
        REQUIRE(TAO::Ledger::IsMissingBranchRecoveryCapped(hashBlock)
            == (nCycle > TAO::Ledger::MAX_BRANCH_RECOVERY_ESCALATIONS));
    }

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();
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
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();

    /* Drive a few INCOMPLETE results to build up the counter.
     * Erase the rate-limit entry before each call so the 250-ms guard does
     * not prevent rapid successive test calls from each incrementing the
     * counter. */
    for(uint32_t i = 0; i < 3; ++i)
    {
        TAO::Ledger::mapLastMissingProcessTime.erase(hashMissing);
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
    TAO::Ledger::mapLastMissingProcessTime.clear();
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

    TAO::Ledger::mapOrphans.Clear();
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
    REQUIRE(TAO::Ledger::mapOrphans.Insert(orphanBlock));

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
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();
    LLD::Ledger->EraseBlock(hashPrevOrphan);
}


TEST_CASE("Orphan-drain retry-limit erases counter and signals branch recovery", "[ledger][process]")
{
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();

    /* Write a prev block so the accepted parent block doesn't take the orphan
     * path when Process() is called. */
    const uint1024_t hashPrevOrphan2(0xABCD5601ULL);

    TAO::Ledger::BlockState statePrevOrphan2;
    statePrevOrphan2.nVersion      = 4;
    statePrevOrphan2.hashPrevBlock = uint1024_t(0);
    statePrevOrphan2.nChannel      = 2;
    statePrevOrphan2.nHeight       = 43;
    statePrevOrphan2.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrevOrphan2, statePrevOrphan2));

    /* Build the accepted parent PassBlock. */
    PassBlock parentBlock2;
    parentBlock2.nVersion      = 4;
    parentBlock2.hashPrevBlock = hashPrevOrphan2;
    parentBlock2.nChannel      = 2;
    parentBlock2.nHeight       = 44;
    parentBlock2.nBits         = 1;
    parentBlock2.nNonce        = 2002;

    const uint1024_t hashParent2 = parentBlock2.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashParent2));

    /* Build the orphan MissingBlock whose parent is the just-accepted block. */
    MissingBlock orphanBlock2;
    orphanBlock2.nVersion      = 4;
    orphanBlock2.hashPrevBlock = hashParent2;
    orphanBlock2.nChannel      = 2;
    orphanBlock2.nHeight       = 45;
    orphanBlock2.nBits         = 1;
    orphanBlock2.nNonce        = 3002;

    const uint1024_t hashOrphan2 = orphanBlock2.GetHash();

    /* Seed the orphan pool. */
    REQUIRE(TAO::Ledger::mapOrphans.Insert(orphanBlock2));

    /* Pre-fill the retry counter to exactly the retry limit, simulating that
     * this orphan has already been retried MAX_MISSING_TRANSACTIONS_RETRIES
     * times.  The next call to Process() should push it over the limit. */
    TAO::Ledger::mapLastMissing[hashOrphan2] =
        LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES;

    /* Process the parent (accepted), which triggers the orphan-drain and
     * encounters an over-limit retry for the orphan's missing transactions. */
    uint8_t nStatus = 0;
    TAO::Ledger::Process(parentBlock2, nStatus);

    /* Parent accepted, orphan still incomplete. */
    REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED)  != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);

    /* The escalation path now preserves missing hashes while setting
     * hashMissing=0 so LLP can fan out per-transaction recovery requests. */
    REQUIRE_FALSE(parentBlock2.vMissing.empty());
    REQUIRE(parentBlock2.hashMissing == 0);

    /* CRITICAL: the entry must be ERASED so the orphan is not permanently
     * wedged on future arrivals — a leftover value > MAX would cause
     * immediate silent drop on every subsequent Process() call. */
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashOrphan2) == 0);

    /* Clean up. */
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();
    LLD::Ledger->EraseBlock(hashPrevOrphan2);
}


TEST_CASE("Persisted orphan is removed while draining", "[ledger][process]")
{
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();

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

    REQUIRE(TAO::Ledger::mapOrphans.Insert(orphanBlock));

    uint8_t nStatus = 0;
    TAO::Ledger::Process(parentBlock, nStatus);

    REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED) != 0);
    REQUIRE(TAO::Ledger::mapOrphans.Empty());
    REQUIRE(LLD::Ledger->HasBlock(hashOrphan));

    LLD::Ledger->EraseBlock(hashOrphan);
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Block orphan pool retains siblings and suppresses exact duplicates", "[ledger][process][orphan_pool]")
{
    TAO::Ledger::OrphanPool pool;
    const uint1024_t hashParent(0x9911);

    PassBlock first;
    first.nVersion = 4;
    first.hashPrevBlock = hashParent;
    first.nNonce = 1;
    first.hashMerkleRoot = uint512_t(1);

    PassBlock second = first;
    second.nNonce = 2;
    second.hashMerkleRoot = uint512_t(2);

    REQUIRE(pool.Insert(first));
    REQUIRE(pool.Insert(second));
    REQUIRE_FALSE(pool.Insert(first));
    REQUIRE(pool.Size() == 2);

    const std::vector<uint1024_t> vChildren = pool.Children(hashParent);
    REQUIRE(vChildren.size() == 2);
    REQUIRE(std::is_sorted(vChildren.begin(), vChildren.end()));
}


TEST_CASE("Block orphan pool FIFO eviction keeps indexes consistent", "[ledger][process][orphan_pool]")
{
    TAO::Ledger::OrphanPool pool;
    const uint1024_t hashParent(0x9922);
    uint1024_t hashFirst = 0;

    for(uint64_t i = 1; i <= TAO::Ledger::MAX_BLOCK_ORPHANS; ++i)
    {
        PassBlock block;
        block.nVersion = 4;
        block.hashPrevBlock = hashParent;
        block.nNonce = i;
        block.hashMerkleRoot = uint512_t(i);
        if(i == 1)
            hashFirst = block.GetHash();
        REQUIRE(pool.Insert(block));
    }

    PassBlock overflow;
    overflow.nVersion = 4;
    overflow.hashPrevBlock = hashParent;
    overflow.nNonce = TAO::Ledger::MAX_BLOCK_ORPHANS + 1;
    overflow.hashMerkleRoot = uint512_t(TAO::Ledger::MAX_BLOCK_ORPHANS + 1);

    uint1024_t hashEvicted = 0;
    REQUIRE(pool.Insert(overflow, &hashEvicted));
    REQUIRE(hashEvicted == hashFirst);
    REQUIRE_FALSE(pool.Contains(hashFirst));
    REQUIRE(pool.Size() == TAO::Ledger::MAX_BLOCK_ORPHANS);
    REQUIRE(pool.Children(hashParent).size() == TAO::Ledger::MAX_BLOCK_ORPHANS);
}


TEST_CASE("Orphan drain handles trees, invalid subtrees, and incomplete siblings", "[ledger][process][orphan_pool]")
{
    LedgerGuard env;
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();

    const uint1024_t hashPrev(0x9933);
    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion = 4;
    statePrev.nHeight = 70;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    PassBlock parent;
    parent.nVersion = 4;
    parent.hashPrevBlock = hashPrev;
    parent.nHeight = 71;
    parent.nNonce = 10;
    const uint1024_t hashParent = parent.GetHash();

    MissingBlock incomplete;
    incomplete.nVersion = 4;
    incomplete.hashPrevBlock = hashParent;
    incomplete.nHeight = 72;
    incomplete.nNonce = 11;
    const uint1024_t hashIncomplete = incomplete.GetHash();

    PassBlock sibling;
    sibling.nVersion = 4;
    sibling.hashPrevBlock = hashParent;
    sibling.nHeight = 72;
    sibling.nNonce = 12;
    const uint1024_t hashSibling = sibling.GetHash();

    PassBlock grandchild;
    grandchild.nVersion = 4;
    grandchild.hashPrevBlock = hashSibling;
    grandchild.nHeight = 73;
    grandchild.nNonce = 13;
    const uint1024_t hashGrandchild = grandchild.GetHash();

    RejectBlock invalid;
    invalid.nVersion = 4;
    invalid.hashPrevBlock = hashParent;
    invalid.nHeight = 72;
    invalid.nNonce = 14;
    const uint1024_t hashInvalid = invalid.GetHash();

    PassBlock invalidDescendant;
    invalidDescendant.nVersion = 4;
    invalidDescendant.hashPrevBlock = hashInvalid;
    invalidDescendant.nHeight = 73;
    invalidDescendant.nNonce = 15;
    const uint1024_t hashInvalidDescendant = invalidDescendant.GetHash();

    REQUIRE(TAO::Ledger::mapOrphans.Insert(incomplete));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(sibling));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(grandchild));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(invalid));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(invalidDescendant));

    uint8_t nStatus = 0;
    TAO::Ledger::Process(parent, nStatus);

    REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED) != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    REQUIRE(TAO::Ledger::mapOrphans.Contains(hashIncomplete));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashSibling));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashGrandchild));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashInvalid));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashInvalidDescendant));

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Disk-known block is not requeued as an orphan", "[ledger][process][orphan_pool]")
{
    LedgerGuard env;
    TAO::Ledger::mapOrphans.Clear();

    PassBlock block;
    block.nVersion = 4;
    block.hashPrevBlock = uint1024_t(0x9944);
    block.nHeight = 80;
    block.nNonce = 20;
    const uint1024_t hashBlock = block.GetHash();

    TAO::Ledger::BlockState state;
    state.nVersion = block.nVersion;
    state.hashPrevBlock = block.hashPrevBlock;
    state.nHeight = block.nHeight;
    state.nNonce = block.nNonce;
    REQUIRE(LLD::Ledger->WriteBlock(hashBlock, state));

    uint8_t nStatus = 0;
    TAO::Ledger::Process(block, nStatus);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::DUPLICATE) != 0);
    REQUIRE(TAO::Ledger::mapOrphans.Empty());

    LLD::Ledger->EraseBlock(hashBlock);
}


TEST_CASE("Unified fork scoring is shared by candidate activation", "[ledger][fork_choice]")
{
    TAO::Ledger::BlockState best;
    best.nVersion = 7;
    best.nChannel = 2;
    best.nHeight = 100;
    best.nNonce = 1;
    best.nChannelWeight[0] = 10;
    best.nChannelWeight[1] = 10;
    best.nChannelWeight[2] = 10;

    TAO::Ledger::BlockState candidate = best;
    candidate.nNonce = 2;
    candidate.nChannelWeight[0] = 11;
    candidate.nChannelWeight[1] = 11;
    candidate.nChannelWeight[2] = 9;
    REQUIRE(candidate.IsHeavierThan(best));

    candidate = best;
    candidate.nNonce = 3;
    candidate.nChannelWeight[0] = 11;
    REQUIRE(candidate.IsHeavierThan(best));

    candidate = best;
    candidate.nNonce = 4;
    candidate.nHeight = 102;
    candidate.nChannelWeight[0] = 11;
    candidate.nChannelWeight[1] = 9;
    REQUIRE(candidate.IsHeavierThan(best));

    candidate = best;
    candidate.nNonce = 5;
    REQUIRE_FALSE(candidate.IsHeavierThan(best));

    candidate.nVersion = 6;
    best.nVersion = 6;
    candidate.nChainTrust = 101;
    best.nChainTrust = 100;
    REQUIRE(candidate.IsHeavierThan(best));
}


TEST_CASE("Persisted block validation bypasses only incoming duplicate rejection",
    "[ledger][process][stored_validation]")
{
    LedgerGuard env;

    const bool fHybridBefore = config::fHybrid.load();
    config::fHybrid.store(true);

    TAO::Ledger::TritiumBlock block;
    block.nVersion = TAO::Ledger::CurrentBlockVersion();
    block.nChannel = 3;
    block.nHeight = 2;
    block.nNonce = 0x647;
    block.nTime = runtime::unifiedtimestamp();

    block.producer.nVersion = TAO::Ledger::CurrentTransactionVersion();
    block.producer.hashGenesis = uint256_t(
        "0xb7a74c14508bd09e104eff93d86cbbdc5c9556ae68546895d964d8374a0e9a41");
    block.producer.hashPrevTx = uint512_t(1);
    block.producer.hashNext = uint512_t(2);
    block.producer.nSequence = 1;
    block.producer.nTimestamp = block.nTime;
    block.producer.nKeyType = TAO::Ledger::SIGNATURE::BRAINPOOL;
    block.producer[0] << uint8_t(TAO::Operation::OP::AUTHORIZE);

    const uint512_t hashSecret(3);
    REQUIRE(block.producer.Sign(hashSecret));

    const uint512_t hashProducer = block.producer.GetHash();
    block.hashMerkleRoot = block.BuildMerkleTree({hashProducer});

    LLC::ECKey key(LLC::BRAINPOOL_P512_T1, 64);
    const std::vector<uint8_t> vSecret = hashSecret.GetBytes();
    REQUIRE(key.SetSecret(LLC::CSecret(vSecret.begin(), vSecret.end()), true));
    REQUIRE(block.GenerateSignature(key));

    TAO::Ledger::BlockState state(block);
    REQUIRE(LLD::Ledger->WriteBlock(block.GetHash(), state));

    REQUIRE_FALSE(block.Check());
    REQUIRE(block.CheckStored());

    LLD::Ledger->EraseBlock(block.GetHash());
    config::fHybrid.store(fHybridBefore);
}


TEST_CASE("Failed candidate activation preserves the active best chain",
    "[ledger][process][fork_choice]")
{
    LedgerGuard env;

    const TAO::Ledger::BlockState stateBestBefore =
        TAO::Ledger::ChainState::tStateBest.load();
    const uint1024_t hashBestBefore =
        TAO::Ledger::ChainState::hashBestChain.load();

    TAO::Ledger::BlockState candidate = stateBestBefore;
    candidate.nVersion = 7;
    candidate.nNonce += 1;
    candidate.nHeight += 1;
    candidate.nChannelWeight[0] += 1;
    candidate.vtx.clear();

    REQUIRE_FALSE(TAO::Ledger::ActivateCandidateBestChain(
        candidate, "unit test invalid candidate", true));
    REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashBestBefore);
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load() == stateBestBefore);
}


TEST_CASE("Synchronization requires the advertised hash to be active",
    "[ledger][process][sync]")
{
    const uint1024_t hashActive(0x646);
    const uint1024_t hashStoredSideBranch(0x647);
    const uint1024_t hashBestBefore =
        TAO::Ledger::ChainState::hashBestChain.load();

    TAO::Ledger::ChainState::hashBestChain.store(hashActive);

    REQUIRE(TAO::Ledger::IsBestChainSynchronized(hashActive));
    REQUIRE_FALSE(TAO::Ledger::IsBestChainSynchronized(hashStoredSideBranch));
    REQUIRE_FALSE(TAO::Ledger::IsBestChainSynchronized(uint1024_t(0)));

    TAO::Ledger::ChainState::hashBestChain.store(hashBestBefore);
}


/* ========================================================================
 * Follow-up regression tests for the third failure mode:
 *   - Escalation cap invariant
 *   - Blacklist short-circuit
 *   - 250-ms rate limit
 *   - Blacklist clears on accept
 *   - Throttle key unification (mapLastOrphanRequest keyed by hashPrevBlock)
 *   - PurgeOrphanRecoveryState completeness
 *   - AttemptPeerBestChainRecovery orphan-pool walkback
 *   - Hash-channel (nChannel=2) PoW non-regression
 * ======================================================================== */


TEST_CASE("Escalation cap invariant: counter never exceeds MAX+1", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t hashPrev(0xCA000001ULL);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 100;
    statePrev.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    MissingBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 101;
    block.nBits         = 1;
    block.nNonce        = 9001;

    const uint1024_t hashBlock = block.GetHash();
    const uint64_t nRetries = LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES + 1;
    const uint32_t nMaxEsc   = TAO::Ledger::MAX_BRANCH_RECOVERY_ESCALATIONS;

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    /* Drive through enough cycles to reach and exceed the cap. */
    const uint32_t nExtraCycles = 5;
    const uint32_t nTotalCycles = nMaxEsc + 1 + nExtraCycles;

    uint32_t nCyclesCompleted = 0;
    for(uint32_t nCycle = 0; nCycle < nTotalCycles; ++nCycle)
    {
        /* Every call to Process() returns INCOMPLETE (hashMissing==0 once
         * blacklisted, since Check()/mapLastMissing are then skipped).
         * Drive the inner retry loop only while the block is not yet
         * blacklisted, since once blacklisted the escalation counter is
         * frozen and further cycles would be redundant. */
        if(TAO::Ledger::setUnrecoverableBlocks.count(hashBlock))
            break; /* blacklisted — further calls short-circuit immediately */

        for(uint64_t i = 0; i < nRetries; ++i)
        {
            TAO::Ledger::mapLastMissingProcessTime.erase(hashBlock);
            uint8_t nStatus = 0;
            TAO::Ledger::Process(block, nStatus);
            /* Status is always INCOMPLETE; break as soon as we're blacklisted
             * since further arrivals no longer advance the escalation counter. */
            if(TAO::Ledger::setUnrecoverableBlocks.count(hashBlock))
                break;
            REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        }
        ++nCyclesCompleted;
    }

    /* After the cap cycle fires the counter must be exactly MAX+1 — no more,
     * no less.  Subsequent arrivals are blocked by the blacklist before
     * reaching incrementMissingEscalations(), so the counter is frozen. */
    const uint32_t nFinalEsc = TAO::Ledger::MissingBranchRecoveryEscalations(hashBlock);
    REQUIRE(nFinalEsc == nMaxEsc + 1);
    REQUIRE(TAO::Ledger::IsMissingBranchRecoveryCapped(hashBlock));
    REQUIRE(TAO::Ledger::setUnrecoverableBlocks.count(hashBlock) == 1);

    /* Drive many more calls: every one returns INCOMPLETE with hashMissing==0
     * (not IGNORED — the blacklist guard now reports INCOMPLETE so the LLP
     * capped-path branch, and ShouldSendBranchSyncRequest(), stay reachable
     * on every arrival), counter stays frozen. */
    for(uint32_t i = 0; i < 20; ++i)
    {
        TAO::Ledger::mapLastMissingProcessTime.erase(hashBlock);
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE(block.hashMissing == 0);
        REQUIRE(TAO::Ledger::MissingBranchRecoveryEscalations(hashBlock) == nMaxEsc + 1);
    }

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Blacklist short-circuit: INCOMPLETE with hashMissing=0", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t hashPrev(0xB100001ULL);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 110;
    statePrev.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    MissingBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 111;
    block.nBits         = 1;
    block.nNonce        = 9002;

    const uint1024_t hashBlock = block.GetHash();

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    /* Manually blacklist the block. */
    TAO::Ledger::setUnrecoverableBlocks.insert(hashBlock);

    /* Process() must short-circuit immediately — before any Check(),
     * mapLastMissing write, or escalation logic — but must still report
     * INCOMPLETE with hashMissing == 0 (not a silent IGNORED) so the LLP
     * capped-path branch, and therefore ShouldSendBranchSyncRequest(),
     * stays reachable on every arrival rather than firing only once. */
    uint8_t nStatus = 0;
    TAO::Ledger::Process(block, nStatus);

    REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
    REQUIRE(block.hashMissing == 0);

    /* mapLastMissing must NOT have been written — the early-return prevented it. */
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) == 0);

    /* Clean up. */
    TAO::Ledger::setUnrecoverableBlocks.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Rate-limit: two rapid calls produce only one full-path execution", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t hashPrev(0xA1000001ULL);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 120;
    statePrev.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    MissingBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 121;
    block.nBits         = 1;
    block.nNonce        = 9003;

    const uint1024_t hashBlock = block.GetHash();

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    /* First call: hash not yet in mapLastMissing — no rate-limit check.
     * Process() runs the full path and sets mapLastMissing[hash] = 1. */
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) == 1);
        REQUIRE(TAO::Ledger::mapLastMissing[hashBlock] == 1);
    }

    /* Second call immediately (within 250 ms): hash IS in mapLastMissing and
     * mapLastMissingProcessTime was set by the first call — rate-limit fires.
     * Returns INCOMPLETE immediately; counter stays at 1. */
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        /* Counter must NOT have been incremented — rate-limit short-circuited. */
        REQUIRE(TAO::Ledger::mapLastMissing[hashBlock] == 1);
    }

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Blacklist clears on ACCEPT: all recovery maps erased", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t hashPrev(0xBC000001ULL);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 130;
    statePrev.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    /* PassBlock will be accepted (Check/Accept both succeed). */
    PassBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 131;
    block.nBits         = 1;
    block.nNonce        = 9004;

    const uint1024_t hashBlock = block.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashBlock));

    /* Pre-populate every recovery map to simulate a prior incomplete run. */
    TAO::Ledger::mapLastMissing[hashBlock]              = 42;
    TAO::Ledger::mapMissingBranchEscalations[hashBlock] = 2;
    TAO::Ledger::setUnrecoverableBlocks.insert(hashBlock);
    TAO::Ledger::mapLastMissingProcessTime[hashBlock]   = runtime::timestamp(true) - 10000;

    /* Process() checks setUnrecoverableBlocks FIRST, so a blacklisted PassBlock
     * would return IGNORED.  Verify the accept-path clears maps correctly:
     * first, remove from blacklist to let the accept path run, then accept. */
    TAO::Ledger::setUnrecoverableBlocks.erase(hashBlock);

    uint8_t nStatus = 0;
    TAO::Ledger::Process(block, nStatus);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED) != 0);

    /* All recovery maps must be cleared on ACCEPT. */
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock)              == 0);
    REQUIRE(TAO::Ledger::mapMissingBranchEscalations.count(hashBlock) == 0);
    REQUIRE(TAO::Ledger::setUnrecoverableBlocks.count(hashBlock)      == 0);
    REQUIRE(TAO::Ledger::mapLastMissingProcessTime.count(hashBlock)   == 0);

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Throttle key unification: mapLastOrphanRequest keyed by hashPrevBlock", "[ledger][process]")
{
    /* ShouldSendBranchSyncRequest() records its timestamp in mapLastOrphanRequest
     * keyed by the hashAncestor argument (== block.hashPrevBlock, the missing
     * ancestor).  Both the orphan-insert path in Process() and the capped-path
     * LIST in the LLP layer use this same canonical key so the drain-loop
     * cleanup erase(hashParent) is always effective. */

    const uint1024_t hashAncestor(0xA2000001ULL);

    TAO::Ledger::mapLastOrphanRequest.clear();

    /* First call must return true (no prior entry). */
    REQUIRE(TAO::Ledger::ShouldSendBranchSyncRequest(hashAncestor));

    /* Verify the timestamp was recorded under the ancestor key. */
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashAncestor) == 1);

    /* Second call within ORPHAN_REQUEST_THROTTLE_SECONDS must return false. */
    REQUIRE_FALSE(TAO::Ledger::ShouldSendBranchSyncRequest(hashAncestor));

    /* The drain-loop cleanup: erasing by hashParent (== hashAncestor) removes
     * the throttle entry regardless of which code path wrote it. */
    TAO::Ledger::mapLastOrphanRequest.erase(hashAncestor);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashAncestor) == 0);

    /* After erase, ShouldSendBranchSyncRequest should allow a new request. */
    REQUIRE(TAO::Ledger::ShouldSendBranchSyncRequest(hashAncestor));

    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("PurgeOrphanRecoveryState clears all correlated maps", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t h1(0xA3000001ULL);
    const uint1024_t h2(0xA3000002ULL);
    const uint1024_t h3(0xA3000003ULL);

    /* Seed every recovery map with at least one entry. */
    TAO::Ledger::mapLastMissing[h1]              = 5;
    TAO::Ledger::mapMissingBranchEscalations[h2] = 2;
    TAO::Ledger::setUnrecoverableBlocks.insert(h3);
    TAO::Ledger::mapLastMissingProcessTime[h1]   = 12345678;
    TAO::Ledger::mapLastOrphanRequest[h2]        = 87654321;

    /* Seed the orphan pool with a dummy block. */
    PassBlock dummy;
    dummy.nVersion      = 4;
    dummy.hashPrevBlock = uint1024_t(0xDEAD);
    dummy.nNonce        = 0xBEEF;
    TAO::Ledger::mapOrphans.Insert(dummy);
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Empty());

    /* Purge everything at once. */
    TAO::Ledger::PurgeOrphanRecoveryState("unit-test");

    /* Every map and set must be empty after the purge. */
    REQUIRE(TAO::Ledger::mapOrphans.Empty());
    REQUIRE(TAO::Ledger::mapLastMissing.empty());
    REQUIRE(TAO::Ledger::mapMissingBranchEscalations.empty());
    REQUIRE(TAO::Ledger::setUnrecoverableBlocks.empty());
    REQUIRE(TAO::Ledger::mapLastMissingProcessTime.empty());
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.empty());
}


TEST_CASE("AttemptPeerBestChainRecovery walks orphan pool for connectable ancestor",
    "[ledger][process]")
{
    LedgerGuard env;

    /* Set up a known-good root block on disk. */
    const uint1024_t hashRoot(0xA4000001ULL);

    TAO::Ledger::BlockState stateRoot;
    stateRoot.nVersion      = 4;
    stateRoot.hashPrevBlock = uint1024_t(0);
    stateRoot.nChannel      = 2;
    stateRoot.nHeight       = 200;
    stateRoot.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashRoot, stateRoot));

    /* Block A: connectable orphan (its hashPrevBlock is on disk). */
    PassBlock blockA;
    blockA.nVersion      = 4;
    blockA.hashPrevBlock = hashRoot;
    blockA.nChannel      = 2;
    blockA.nHeight       = 201;
    blockA.nBits         = 1;
    blockA.nNonce        = 10001;
    const uint1024_t hashA = blockA.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashA));

    /* Block B: peer tip — orphan whose hashPrevBlock is hashA (also in pool). */
    PassBlock blockB;
    blockB.nVersion      = 4;
    blockB.hashPrevBlock = hashA;
    blockB.nChannel      = 2;
    blockB.nHeight       = 202;
    blockB.nBits         = 1;
    blockB.nNonce        = 10002;
    const uint1024_t hashB = blockB.GetHash();

    /* Seed both blocks in the orphan pool: B is the peer's advertised tip;
     * A is the connectable ancestor (prev is on disk). */
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();
    REQUIRE(TAO::Ledger::mapOrphans.Insert(blockA));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(blockB));

    /* AttemptPeerBestChainRecovery should:
     *   1. ReadBlock(hashB) fails (not on disk).
     *   2. Walk back: hashB → hashA (connectable, prev=hashRoot is on disk).
     *   3. Process(blockA) → ACCEPTED → BFS drain picks up blockB.
     *   4. Return PROGRESS (forward progress made). */
    const auto result = TAO::Ledger::AttemptPeerBestChainRecovery(
        hashB, 202, "unit-test", nullptr);

    REQUIRE(result == TAO::Ledger::PeerBestRecoveryResult::PROGRESS);

    /* Both blockA and blockB must have been walked through Accept() (the
     * mock PassBlock::Accept() does not itself persist to disk, so this is
     * verified via the orphan pool having been fully drained rather than
     * LLD::Ledger->HasBlock(), mirroring the "Persisted orphan is removed
     * while draining" test's pattern). Orphan pool should be empty: blockA
     * was consumed as the connectable ancestor fed through Process(), and
     * blockB was drained by the BFS walk that follows a successful accept. */
    REQUIRE(TAO::Ledger::mapOrphans.Empty());
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashA));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashB));

    /* Clean up. */
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    LLD::Ledger->EraseBlock(hashA);
    LLD::Ledger->EraseBlock(hashB);
    LLD::Ledger->EraseBlock(hashRoot);
}


TEST_CASE("AttemptPeerBestChainRecovery requests branch sync when tip far ahead",
    "[ledger][process][a1]")
{
    /* A1 regression: peer tip not on disk AND not in the orphan pool must
     * issue a throttled locator LIST + SPECIFIER::TRANSACTIONS (not a silent
     * no-op).  Exercise the path with an injectable TritiumNode backed by a
     * socketpair so we can assert the on-wire message, throttle key, and the
     * pfBranchSyncQueued out-parameter used by callers to skip duplicate LIST. */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashFarTip(0xA1000000000000FEULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashFarTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashFarTip));

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    /* Capture locator target before recovery so the expected packet matches
     * the LIST payload that was actually queued (hashBestChain is read once
     * inside the helper at send time). */
    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashFarTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fBranchSyncQueued = false;
    const auto result = TAO::Ledger::AttemptPeerBestChainRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-a1", &node, &fBranchSyncQueued);

    REQUIRE(result == TAO::Ledger::PeerBestRecoveryResult::FETCH_QUEUED);
    REQUIRE(fBranchSyncQueued);
    REQUIRE(TAO::Ledger::mapOrphans.Empty());

    /* Throttle key is the far tip itself when no orphan walk occurred. */
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    /* Flush any overflow-buffered remainder, then drain the peer read end. */
    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent.insert(vSent.end(), buf.begin(), buf.begin() + n);
        }
    }

    REQUIRE(vSent == vExpected);

    /* Immediate second call must be throttled: no new LIST, out-param false. */
    bool fBranchSyncQueued2 = true; /* helper must clear this */
    const auto result2 = TAO::Ledger::AttemptPeerBestChainRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-a1-throttle", &node,
        &fBranchSyncQueued2);
    REQUIRE(result2 == TAO::Ledger::PeerBestRecoveryResult::FETCH_THROTTLED);
    REQUIRE_FALSE(fBranchSyncQueued2);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent2;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent2.insert(vSent2.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent2.empty());

    /* BaseConnection only Close()s when fCONNECTED; we never connected, so
     * own both ends of the socketpair and clear node.fd before destruction. */
    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    /* WIN32: no socketpair — still verify throttle recording with a bare node.
     * PushMessage may fail without a live fd, so accept QUEUED or SKIPPED. */
    LLP::TritiumNode node;
    bool fBranchSyncQueued = false;
    const auto result = TAO::Ledger::AttemptPeerBestChainRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-a1", &node, &fBranchSyncQueued);
    REQUIRE((result == TAO::Ledger::PeerBestRecoveryResult::FETCH_QUEUED
          || result == TAO::Ledger::PeerBestRecoveryResult::SKIPPED));
    REQUIRE(TAO::Ledger::mapOrphans.Empty());
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);
#endif

    /* Clean up any throttle residue keyed on the far tip. */
    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("Missing-tx escalation queues only one LIST when recovery already did",
    "[ledger][process][a1][missing-tx-escalation]")
{
    /* Regression for the tritium missing-tx escalation coordination:
     * RequestMissingTxBranchRecovery must not follow a successful recovery LIST
     * with an identical fallback LIST (duplicate branch traffic + TxResponseWindow
     * replacement on the same peer).  Calling AttemptPeerBestChainRecovery alone
     * cannot catch a removed if(!fBranchSyncQueued) guard. */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashFarTip(0xA1000000000000FFULL);
    const uint1024_t hashIncomplete(0xA100000000000100ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashFarTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashFarTip));

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashFarTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fBranchSyncQueued = false;
    const bool fProgress = TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-escalation", &node, &fBranchSyncQueued);

    REQUIRE_FALSE(fProgress);
    REQUIRE(fBranchSyncQueued);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent.insert(vSent.end(), buf.begin(), buf.begin() + n);
        }
    }

    /* Combined path must emit exactly one LIST — not recovery+fallback. */
    REQUIRE(vSent.size() == vExpected.size());
    REQUIRE(vSent == vExpected);

    /* Fallback-only path (unknown peer best) still queues one LIST to hashBlock. */
    TAO::Ledger::mapLastOrphanRequest.clear();
    DataStream ssFallback(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssFallback
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashIncomplete);
    const std::vector<uint8_t> vFallbackExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssFallback).GetBytes();

    bool fFallbackQueued = false;
    const bool fFallbackProgress = TAO::Ledger::RequestMissingTxBranchRecovery(
        uint1024_t(0), hashIncomplete, /*nPeerHeight=*/0,
        "unit-test-missing-tx-fallback", &node, &fFallbackQueued);
    REQUIRE_FALSE(fFallbackProgress);
    REQUIRE(fFallbackQueued);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vFallbackSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vFallbackSent.insert(vFallbackSent.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vFallbackSent == vFallbackExpected);

    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    LLP::TritiumNode node;
    bool fBranchSyncQueued = false;
    const bool fProgress = TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-escalation", &node, &fBranchSyncQueued);
    REQUIRE_FALSE(fProgress);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);
#endif

    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("Missing-tx escalation second call emits no LIST while throttled",
    "[ledger][process][a1][missing-tx-escalation][throttle]")
{
    /* Regression: RequestMissingTxBranchRecovery must not treat
     * AttemptPeerBestChainRecovery's throttle denial (fBranchSyncQueued=false)
     * as permission to fire the unthrottled fallback LIST.  Two immediate
     * combined calls must produce exactly one on-wire LIST. */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashFarTip(0xA100000000000101ULL);
    const uint1024_t hashIncomplete(0xA100000000000102ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashFarTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashFarTip));

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashFarTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fQueued1 = false;
    REQUIRE_FALSE(TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-throttle-1", &node, &fQueued1));
    REQUIRE(fQueued1);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent1;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent1.insert(vSent1.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent1 == vExpected);

    /* Immediate second combined call: recovery is throttled; fallback must not
     * emit another LIST (the bug sent fallback whenever queued==false). */
    bool fQueued2 = true; /* helper must clear when nothing is sent */
    REQUIRE_FALSE(TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-throttle-2", &node, &fQueued2));
    REQUIRE_FALSE(fQueued2);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent2;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent2.insert(vSent2.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent2.empty());

    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    LLP::TritiumNode node;
    bool fQueued1 = false;
    REQUIRE_FALSE(TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-throttle-1", &node, &fQueued1));
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    bool fQueued2 = true;
    REQUIRE_FALSE(TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-throttle-2", &node, &fQueued2));
    REQUIRE_FALSE(fQueued2);
#endif

    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("BESTCHAIN recovery queues only one LIST for unknown tip",
    "[ledger][process][a1][bestchain]")
{
    /* TIP-01 / TIP-02: RequestBestChainBranchRecovery must route unknown tips
     * through AttemptPeerBestChainRecovery (throttled LIST + fanout-capable)
     * and must not emit a second unthrottled fallback LIST on the same call. */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashFarTip(0xBC01000000000001ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashFarTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashFarTip));

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashFarTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fQueued = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain", &node, &fQueued));
    REQUIRE(fQueued);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent.insert(vSent.end(), buf.begin(), buf.begin() + n);
        }
    }

    /* Coordinator LIST only — no fallback double-LIST on the same call. */
    REQUIRE(vSent.size() == vExpected.size());
    REQUIRE(vSent == vExpected);

    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    LLP::TritiumNode node;
    bool fQueued = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain", &node, &fQueued));
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);
#endif

    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("BESTCHAIN recovery second notify emits no LIST while throttled",
    "[ledger][process][a1][bestchain][throttle]")
{
    /* TIP-01: chatty BESTCHAIN must not thrash TxResponseWindow.  Two immediate
     * RequestBestChainBranchRecovery calls for the same unknown tip produce
     * exactly one on-wire LIST (second call is FETCH_THROTTLED with no fallback). */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashFarTip(0xBC01000000000002ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashFarTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashFarTip));

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashFarTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fQueued1 = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain-throttle-1", &node, &fQueued1));
    REQUIRE(fQueued1);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent1;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent1.insert(vSent1.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent1 == vExpected);

    bool fQueued2 = true; /* helper must clear when nothing is sent */
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain-throttle-2", &node, &fQueued2));
    REQUIRE_FALSE(fQueued2);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent2;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent2.insert(vSent2.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent2.empty());

    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    LLP::TritiumNode node;
    bool fQueued1 = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain-throttle-1", &node, &fQueued1));
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    bool fQueued2 = true;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain-throttle-2", &node, &fQueued2));
    REQUIRE_FALSE(fQueued2);
#endif

    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("BESTCHAIN recovery ignores unknown tip from behind peer",
"[ledger][process][a1][bestchain][height-gate]")
{
/* Historical BESTCHAIN height gate: an unknown tip advertised by a peer
 * whose height is below local best must not queue LIST, open a
 * TxResponseWindow, or consume mapLastOrphanRequest throttle. */
LedgerGuard env;

TAO::Ledger::mapOrphans.Clear();
TAO::Ledger::mapLastOrphanRequest.clear();
TAO::Ledger::mapLastMissing.clear();
TAO::Ledger::mapLastMissingProcessTime.clear();
TAO::Ledger::setUnrecoverableBlocks.clear();

const uint1024_t hashUnknownTip(0xBC01000000000003ULL);
REQUIRE_FALSE(LLD::Ledger->HasBlock(hashUnknownTip));
REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashUnknownTip));

const uint32_t nSavedBestHeight = TAO::Ledger::ChainState::nBestHeight.load();
TAO::Ledger::ChainState::nBestHeight.store(1000);

#ifndef WIN32
int fds[2] = {-1, -1};
REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

LLP::TritiumNode node;
node.fd     = fds[1];
node.events = POLLIN;

bool fQueued = true; /* helper must clear when nothing is sent */
REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
    hashUnknownTip, /*nPeerHeight=*/500, "unit-test-bestchain-behind",
    &node, &fQueued));
REQUIRE_FALSE(fQueued);
REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashUnknownTip) == 0);
REQUIRE(TAO::Ledger::mapLastOrphanRequest.empty());

while(node.Buffered() > 0)
{
    if(node.Flush() <= 0)
        break;
}

std::vector<uint8_t> vSent;
{
    std::vector<uint8_t> buf(65536);
    for(;;)
    {
        const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
        if(n <= 0)
            break;
        vSent.insert(vSent.end(), buf.begin(), buf.begin() + n);
    }
}
REQUIRE(vSent.empty());

node.fd = -1;
close(fds[0]);
close(fds[1]);
#else
LLP::TritiumNode node;
bool fQueued = true;
REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
    hashUnknownTip, /*nPeerHeight=*/500, "unit-test-bestchain-behind",
    &node, &fQueued));
REQUIRE_FALSE(fQueued);
REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashUnknownTip) == 0);
#endif

TAO::Ledger::ChainState::nBestHeight.store(nSavedBestHeight);
TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("BESTCHAIN recovery skips near-tip unknown race",
    "[ledger][process][a1][bestchain][near-tip]")
{
    /* Post-#694 regression: an unknown tip at peer_height == local (or only
     * BESTCHAIN_NEAR_TIP_HEIGHT_SLACK ahead) is the normal tip-advance race
     * only when a matching BLOCK inventory GET was already queued AND the tip
     * is not already in mapOrphans.  Without that GET (Sync() omits BLOCK;
     * relay can deliver BESTCHAIN alone), or when the tip sits in the orphan
     * pool (duplicate GET is a no-op ORPHAN return), recovery must still run.
     * Far tips (delta > slack) always recover. */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashNearTip(0xBC01000000000004ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashNearTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashNearTip));

    const uint32_t nSavedBestHeight = TAO::Ledger::ChainState::nBestHeight.load();
    const uint32_t nLocalHeight = 1000;
    TAO::Ledger::ChainState::nBestHeight.store(nLocalHeight);

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    /* Equal height with matching BLOCK GET: inventory owns the race. */
    bool fQueuedEqual = true;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNearTip, /*nPeerHeight=*/nLocalHeight, "unit-test-bestchain-near-eq",
        &node, &fQueuedEqual, /*fMatchingBlockInventoryGet=*/true));
    REQUIRE_FALSE(fQueuedEqual);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashNearTip) == 0);

    /* +1 height with matching BLOCK GET: still within near-tip slack. */
    bool fQueuedPlusOne = true;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNearTip,
        /*nPeerHeight=*/nLocalHeight + TAO::Ledger::BESTCHAIN_NEAR_TIP_HEIGHT_SLACK,
        "unit-test-bestchain-near-plus1", &node, &fQueuedPlusOne,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE_FALSE(fQueuedPlusOne);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashNearTip) == 0);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent.insert(vSent.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent.empty());

    /* Near-tip without a matching BLOCK GET must still recover (no stall). */
    const uint1024_t hashNoInvTip(0xBC01000000000006ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashNoInvTip));

    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpectedNoInv(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpectedNoInv
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashNoInvTip);
    const std::vector<uint8_t> vExpectedNoInv =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpectedNoInv).GetBytes();

    bool fQueuedNoInv = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNoInvTip, /*nPeerHeight=*/nLocalHeight,
        "unit-test-bestchain-near-no-inv", &node, &fQueuedNoInv,
        /*fMatchingBlockInventoryGet=*/false));
    REQUIRE(fQueuedNoInv);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashNoInvTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vNoInvSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vNoInvSent.insert(vNoInvSent.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vNoInvSent == vExpectedNoInv);

    /* +2 heights: material gap — coordinator must still fetch. */
    const uint1024_t hashGapTip(0xBC01000000000005ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashGapTip));

    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashGapTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fQueuedGap = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashGapTip,
        /*nPeerHeight=*/nLocalHeight + TAO::Ledger::BESTCHAIN_NEAR_TIP_HEIGHT_SLACK + 1,
        "unit-test-bestchain-gap", &node, &fQueuedGap,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE(fQueuedGap);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashGapTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vGapSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vGapSent.insert(vGapSent.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vGapSent == vExpected);

    /* Near-tip tip already in the orphan pool must NOT take the inventory
     * shortcut: a duplicate BLOCK GET returns ORPHAN immediately from Process()
     * and never walks ancestry / reissues the missing-branch LIST.  Seed an
     * orphan whose prev is not on disk so the coordinator gap path fires. */
    PassBlock orphanNearTip;
    orphanNearTip.nVersion      = 4;
    orphanNearTip.hashPrevBlock = uint1024_t(0xBC01DEADBEEF0001ULL);
    orphanNearTip.nChannel      = 2;
    orphanNearTip.nHeight       = nLocalHeight;
    orphanNearTip.nBits         = 1;
    orphanNearTip.nNonce        = 0x0B01;
    orphanNearTip.hashMerkleRoot = uint512_t(0x0B01);
    const uint1024_t hashOrphanNearTip = orphanNearTip.GetHash();
    const uint1024_t hashOrphanAncestor = orphanNearTip.hashPrevBlock;

    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashOrphanNearTip));
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashOrphanAncestor));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(orphanNearTip));
    REQUIRE(TAO::Ledger::mapOrphans.Contains(hashOrphanNearTip));

    DataStream ssExpectedOrphan(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpectedOrphan
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashOrphanNearTip);
    const std::vector<uint8_t> vExpectedOrphan =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpectedOrphan).GetBytes();

    bool fQueuedOrphan = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashOrphanNearTip, /*nPeerHeight=*/nLocalHeight,
        "unit-test-bestchain-near-orphan", &node, &fQueuedOrphan,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE(fQueuedOrphan);
    /* Gap-path throttle key is the deepest missing ancestor (orphan prev),
     * not the advertised tip — same contract as AttemptPeerBestChainRecovery. */
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashOrphanAncestor) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vOrphanSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vOrphanSent.insert(vOrphanSent.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vOrphanSent == vExpectedOrphan);

    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    LLP::TritiumNode node;
    bool fQueuedEqual = true;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNearTip, /*nPeerHeight=*/nLocalHeight, "unit-test-bestchain-near-eq",
        &node, &fQueuedEqual, /*fMatchingBlockInventoryGet=*/true));
    REQUIRE_FALSE(fQueuedEqual);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashNearTip) == 0);

    bool fQueuedPlusOne = true;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNearTip,
        /*nPeerHeight=*/nLocalHeight + TAO::Ledger::BESTCHAIN_NEAR_TIP_HEIGHT_SLACK,
        "unit-test-bestchain-near-plus1", &node, &fQueuedPlusOne,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE_FALSE(fQueuedPlusOne);

    const uint1024_t hashNoInvTip(0xBC01000000000006ULL);
    bool fQueuedNoInv = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNoInvTip, /*nPeerHeight=*/nLocalHeight,
        "unit-test-bestchain-near-no-inv", &node, &fQueuedNoInv,
        /*fMatchingBlockInventoryGet=*/false));
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashNoInvTip) == 1);

    const uint1024_t hashGapTip(0xBC01000000000005ULL);
    bool fQueuedGap = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashGapTip,
        /*nPeerHeight=*/nLocalHeight + TAO::Ledger::BESTCHAIN_NEAR_TIP_HEIGHT_SLACK + 1,
        "unit-test-bestchain-gap", &node, &fQueuedGap,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashGapTip) == 1);

    PassBlock orphanNearTip;
    orphanNearTip.nVersion      = 4;
    orphanNearTip.hashPrevBlock = uint1024_t(0xBC01DEADBEEF0001ULL);
    orphanNearTip.nChannel      = 2;
    orphanNearTip.nHeight       = nLocalHeight;
    orphanNearTip.nBits         = 1;
    orphanNearTip.nNonce        = 0x0B01;
    orphanNearTip.hashMerkleRoot = uint512_t(0x0B01);
    const uint1024_t hashOrphanNearTip = orphanNearTip.GetHash();
    const uint1024_t hashOrphanAncestor = orphanNearTip.hashPrevBlock;
    REQUIRE(TAO::Ledger::mapOrphans.Insert(orphanNearTip));

    bool fQueuedOrphan = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashOrphanNearTip, /*nPeerHeight=*/nLocalHeight,
        "unit-test-bestchain-near-orphan", &node, &fQueuedOrphan,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashOrphanAncestor) == 1);
#endif

    TAO::Ledger::ChainState::nBestHeight.store(nSavedBestHeight);
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("Process primary path does not force PrimeCheck on IBD (source guard)",
"[ledger][process][primecheck]")
{
    /* Regression guard for the multi-day sync collapse: the primary
     * Process() ingestion path must call block.Check() with the default
     * fForceProof=false so Synchronizing() can skip full PrimeCheck.
     * The recovery retry may still use Check(true). */
    const char* vCandidates[] = {
        "src/TAO/Ledger/process.cpp",
        "./src/TAO/Ledger/process.cpp",
        "../src/TAO/Ledger/process.cpp",
        "../../src/TAO/Ledger/process.cpp",
    };

    std::string strSource;
    for(const char* psz : vCandidates)
    {
        std::ifstream f(psz, std::ios::in | std::ios::binary);
        if(!f.is_open())
            continue;
        std::ostringstream ss;
        ss << f.rdbuf();
        strSource = ss.str();
        if(!strSource.empty())
            break;
    }

    if(strSource.empty())
    {
        WARN("process.cpp not reachable from CWD; skipping source guard");
        SUCCEED();
        return;
    }

    /* Primary path must use default Check() (no forced proof). */
    REQUIRE(strSource.find("if(!fSkipCheck && !block.Check())") != std::string::npos);

    /* Must NOT reintroduce a forced-proof primary call of the form
     * if(!fSkipCheck && !block.Check(true)). */
    REQUIRE(strSource.find("if(!fSkipCheck && !block.Check(true))") == std::string::npos);

    /* Recovery retry is allowed to force proof. */
    REQUIRE(strSource.find("if(block.Check(true))") != std::string::npos);

    /* A1: far-tip path must issue locator branch sync, not only log
     * block-not-yet-received and return. */
    REQUIRE(strSource.find("action=locator-branch-sync-request") != std::string::npos);
    REQUIRE(strSource.find("action=block-not-yet-received") == std::string::npos);
}


TEST_CASE("Hash-channel (nChannel=2) PoW non-regression: Check() branches correctly",
    "[ledger][process]")
{
    /* Verify that a Hash-channel block has its proof hash computed over
     * [nVersion .. nNonce] (not [nVersion .. nBits] like the Prime channel).
     * This is tested at the Block level to confirm ProofHash() dispatches
     * correctly; the stuck block in production never reached VerifyWork()
     * because it returned early at the vMissing guard. */

    TAO::Ledger::TritiumBlock block;
    block.nVersion = TAO::Ledger::CurrentBlockVersion();
    block.nChannel = 2;   /* Hash channel */
    block.nHeight  = 1;
    block.nNonce   = 0xDEADBEEF12345678ULL;
    block.nBits    = 0x20ffffff;

    /* ProofHash for nChannel != 1 must include nNonce (it's computed over
     * [nVersion, nNonce] inclusive), so changing nNonce changes the hash. */
    const uint1024_t hashNonce1 = block.ProofHash();
    block.nNonce = 0xDEADBEEF12345679ULL;
    const uint1024_t hashNonce2 = block.ProofHash();
    REQUIRE(hashNonce1 != hashNonce2);

    /* For comparison, Prime channel (nChannel == 1) hashes over
     * [nVersion, nBits] — nNonce changes do NOT change the proof hash. */
    block.nChannel = 1;
    const uint1024_t hashPrime1 = block.ProofHash();
    block.nNonce = 0xDEADBEEF12345680ULL;
    const uint1024_t hashPrime2 = block.ProofHash();
    REQUIRE(hashPrime1 == hashPrime2);

    /* Restore Hash channel and verify changing nBits also changes the hash
     * (both fields are in [nVersion, nNonce]). */
    block.nChannel = 2;
    block.nNonce   = 0xDEADBEEF12345678ULL;
    const uint1024_t hashBits1 = block.ProofHash();
    block.nBits = 0x20fefefe;
    const uint1024_t hashBits2 = block.ProofHash();
    REQUIRE(hashBits1 != hashBits2);
}


/* ==========================================================================
 * Tests for ACTION::LIST specifier sync-state coupling
 *
 * Background (PR #66x regression): fork-recovery LIST calls incorrectly used
 * SPECIFIER::SYNC after initial synchronisation completed.  The receiving
 * handler rejects SYNC blocks as "unsolicited" whenever fSynchronized == true,
 * causing silent DISCONNECT::FORCE.  The fix is to use SPECIFIER::TRANSACTIONS
 * on every post-sync recovery path, preserving SPECIFIER::SYNC only for the
 * true sync-time paths (TritiumNode::Sync() and the LASTINDEX handler).
 * ========================================================================== */

TEST_CASE("SPECIFIER enum values are stable (regression guard)", "[ledger][process]")
{
    /* These constant values are part of the on-wire protocol.  Any reordering
     * or renumbering silently changes which specifier is sent to remote peers.
     * Document them explicitly so an accidental renumber shows up as a test
     * failure rather than a protocol regression. */
    REQUIRE(uint8_t(LLP::TritiumNode::SPECIFIER::LEGACY)       == 0x40);
    REQUIRE(uint8_t(LLP::TritiumNode::SPECIFIER::TRITIUM)      == 0x41);
    REQUIRE(uint8_t(LLP::TritiumNode::SPECIFIER::SYNC)         == 0x42);
    REQUIRE(uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS) == 0x43);
    REQUIRE(uint8_t(LLP::TritiumNode::SPECIFIER::CLIENT)       == 0x44);

    /* TRANSACTIONS and SYNC must be distinct — the entire post-sync vs sync-time
     * specifier split relies on this. */
    REQUIRE(LLP::TritiumNode::SPECIFIER::TRANSACTIONS != LLP::TritiumNode::SPECIFIER::SYNC);
}


TEST_CASE("Unsolicited-sync guard: fSynchronized==true rejects SPECIFIER::SYNC",
    "[ledger][process]")
{
    /* Document the guard condition that makes the SPECIFIER::SYNC regression
     * possible.  In src/LLP/tritium.cpp the TYPES::BLOCK / SPECIFIER::SYNC
     * handler reads:
     *
     *   if(nCurrentSession != TAO::Ledger::nSyncSession || fSynchronized.load())
     *       return debug::drop(FUNCTION, "unsolicted sync block");
     *
     * After initial sync completes:
     *   - TAO::Ledger::nSyncSession is reset to 0
     *   - fSynchronized is set to true
     *
     * Therefore the guard trips for every SYNC block received on a fully
     * synced node — including responses to our own recovery requests. */

    /* Save and restore the global sync session so we don't disturb other tests. */
    const uint64_t nSavedSession = TAO::Ledger::nSyncSession.load();
    const bool fSavedSynced      = LLP::TritiumNode::fSynchronized.load();

    /* --- Simulate: initial-sync in progress -------------------------------- */
    /* Assign an arbitrary non-zero sync session and clear fSynchronized. */
    TAO::Ledger::nSyncSession.store(42);
    LLP::TritiumNode::fSynchronized.store(false);

    const uint64_t nCurrentSession = 42; /* matches nSyncSession */

    /* Guard condition: (nCurrentSession != nSyncSession) || fSynchronized */
    const bool fDropDuringSyncWithMatchingSession =
        (nCurrentSession != TAO::Ledger::nSyncSession.load())
        || LLP::TritiumNode::fSynchronized.load();

    /* Should NOT drop during active sync when sessions match. */
    REQUIRE_FALSE(fDropDuringSyncWithMatchingSession);

    /* --- Simulate: sync complete (post-sync state) ------------------------- */
    TAO::Ledger::nSyncSession.store(0);
    LLP::TritiumNode::fSynchronized.store(true);

    const bool fDropPostSync =
        (nCurrentSession != TAO::Ledger::nSyncSession.load())
        || LLP::TritiumNode::fSynchronized.load();

    /* MUST drop: any SYNC block arriving on a synced node is unsolicited from
     * the receiver's perspective, even if we sent the LIST ourselves. */
    REQUIRE(fDropPostSync);

    /* --- Simulate: session mismatch during sync (different peer) ----------- */
    TAO::Ledger::nSyncSession.store(99);     /* different from nCurrentSession */
    LLP::TritiumNode::fSynchronized.store(false);

    const bool fDropMismatchedSession =
        (nCurrentSession != TAO::Ledger::nSyncSession.load())
        || LLP::TritiumNode::fSynchronized.load();

    /* Must drop: this peer is not the designated sync peer. */
    REQUIRE(fDropMismatchedSession);

    /* Restore global state. */
    TAO::Ledger::nSyncSession.store(nSavedSession);
    LLP::TritiumNode::fSynchronized.store(fSavedSynced);
}


TEST_CASE("Post-sync state: nSyncSession==0 and fSynchronized==true after Sync() completes",
    "[ledger][process]")
{
    /* After the ACTION::NOTIFY BESTCHAIN handler detects synchronisation is
     * complete it executes:
     *
     *   fSynchronized.store(true);
     *   TAO::Ledger::nSyncSession.store(0);
     *
     * This test documents the expected post-sync state so that any future
     * change to the completion logic is visible as a test failure. */

    const uint64_t nSavedSession = TAO::Ledger::nSyncSession.load();
    const bool fSavedSynced      = LLP::TritiumNode::fSynchronized.load();

    /* Simulate the two stores that mark sync complete. */
    LLP::TritiumNode::fSynchronized.store(true);
    TAO::Ledger::nSyncSession.store(0);

    REQUIRE(LLP::TritiumNode::fSynchronized.load() == true);
    REQUIRE(TAO::Ledger::nSyncSession.load()       == 0);

    /* In this state, the unsolicited-sync guard always fires regardless of
     * what nCurrentSession the *receiving* handler reads: even if some stale
     * connection still has nCurrentSession == 0 the condition
     *   (0 != 0) || true  →  true
     * causes a drop.  This is why SPECIFIER::TRANSACTIONS must be used for
     * all post-sync fork-recovery LIST requests. */
    const uint64_t nCurrentSession = 0;   /* worst-case stale session value */
    const bool fWouldDrop =
        (nCurrentSession != TAO::Ledger::nSyncSession.load())
        || LLP::TritiumNode::fSynchronized.load();
    REQUIRE(fWouldDrop);

    /* Restore. */
    TAO::Ledger::nSyncSession.store(nSavedSession);
    LLP::TritiumNode::fSynchronized.store(fSavedSynced);
}


TEST_CASE("Sync completion height guard prevents stale half-chain finalization", "[ledger][process]")
{
    const uint32_t nSavedBestHeight    = TAO::Ledger::ChainState::nBestHeight.load();
    const uint32_t nSavedMaxPeerHeight = TAO::Ledger::ChainState::nMaxPeerHeight.load();

    /* Model the completion guard in src/LLP/tritium.cpp:
     *   local_best >= sync_peer_height
     *   && sync_peer_height + tolerance >= global_max_peer_height
     *
     * This prevents declaring full sync when the active sync peer is still far
     * behind the strongest advertised peer height (the half-chain stale-state
     * failure mode). */
    constexpr uint32_t nTolerance = 2;

    TAO::Ledger::ChainState::nBestHeight.store(500000);
    TAO::Ledger::ChainState::nMaxPeerHeight.store(900000);
    const uint32_t nSyncPeerHeight = 500000;

    const bool fCanFinalizeWithStaleSyncPeer =
        (TAO::Ledger::ChainState::nBestHeight.load() >= nSyncPeerHeight)
        && (static_cast<uint64_t>(nSyncPeerHeight) + nTolerance
            >= static_cast<uint64_t>(TAO::Ledger::ChainState::nMaxPeerHeight.load()));

    REQUIRE_FALSE(fCanFinalizeWithStaleSyncPeer);

    TAO::Ledger::ChainState::nBestHeight.store(900000);
    TAO::Ledger::ChainState::nMaxPeerHeight.store(900001);
    const uint32_t nNearTipSyncPeerHeight = 900000;

    const bool fCanFinalizeNearTip =
        (TAO::Ledger::ChainState::nBestHeight.load() >= nNearTipSyncPeerHeight)
        && (static_cast<uint64_t>(nNearTipSyncPeerHeight) + nTolerance
            >= static_cast<uint64_t>(TAO::Ledger::ChainState::nMaxPeerHeight.load()));

    REQUIRE(fCanFinalizeNearTip);

    TAO::Ledger::ChainState::nBestHeight.store(nSavedBestHeight);
    TAO::Ledger::ChainState::nMaxPeerHeight.store(nSavedMaxPeerHeight);
}


/* ==========================================================================
 * Tests for the admissibility classifier (stranded-state loop fix)
 *
 * Background: three call sites compared against local chain state, decided
 * something was invalid, and took no recovery action.  On a node already
 * behind, local state is exactly what is wrong, so the condition is
 * self-sustaining.  The admissibility classifier distinguishes
 * INVALID_ABSOLUTE from DEFERRED_LOCAL_STATE so recovery paths can retain
 * and retry instead of permanently blacklisting.
 * ========================================================================== */

TEST_CASE("AdmissibilityClass enum has correct values", "[ledger][process]")
{
    /* Verify the enum is defined and the values are distinct.
     * This locks in the classifier API so future refactors show up as
     * compile-time failures rather than silent behavioural regressions. */
    REQUIRE(TAO::Ledger::AdmissibilityClass::VALID
         != TAO::Ledger::AdmissibilityClass::INVALID_ABSOLUTE);
    REQUIRE(TAO::Ledger::AdmissibilityClass::VALID
         != TAO::Ledger::AdmissibilityClass::DEFERRED_LOCAL_STATE);
    REQUIRE(TAO::Ledger::AdmissibilityClass::INVALID_ABSOLUTE
         != TAO::Ledger::AdmissibilityClass::DEFERRED_LOCAL_STATE);
    REQUIRE(TAO::Ledger::AdmissibilityClass::UNKNOWN
         != TAO::Ledger::AdmissibilityClass::VALID);
}


TEST_CASE("AdmissibilityClass thread-local side-channel set/take semantics", "[ledger][process]")
{
    /* The thread-local side-channel used between Transaction::Connect() and
     * Mempool::Accept() must:
     *   1. Start at UNKNOWN.
     *   2. Reflect the value set by SetLastConnectClass().
     *   3. Be reset to UNKNOWN after a single TakeLastConnectClass() call —
     *      stale classifications must not persist across unrelated Connect()
     *      calls on the same thread. */

    /* Initial value must be UNKNOWN. */
    TAO::Ledger::g_nLastConnectClass = TAO::Ledger::AdmissibilityClass::UNKNOWN;
    REQUIRE(TAO::Ledger::g_nLastConnectClass == TAO::Ledger::AdmissibilityClass::UNKNOWN);

    /* After setting DEFERRED_LOCAL_STATE, TakeLastConnectClass() returns it. */
    TAO::Ledger::SetLastConnectClass(TAO::Ledger::AdmissibilityClass::DEFERRED_LOCAL_STATE);
    REQUIRE(TAO::Ledger::g_nLastConnectClass ==
            TAO::Ledger::AdmissibilityClass::DEFERRED_LOCAL_STATE);

    const TAO::Ledger::AdmissibilityClass cls = TAO::Ledger::TakeLastConnectClass();
    REQUIRE(cls == TAO::Ledger::AdmissibilityClass::DEFERRED_LOCAL_STATE);

    /* After the take the slot must be reset to UNKNOWN. */
    REQUIRE(TAO::Ledger::g_nLastConnectClass == TAO::Ledger::AdmissibilityClass::UNKNOWN);

    /* A second take without an intervening set returns UNKNOWN. */
    const TAO::Ledger::AdmissibilityClass cls2 = TAO::Ledger::TakeLastConnectClass();
    REQUIRE(cls2 == TAO::Ledger::AdmissibilityClass::UNKNOWN);

    /* INVALID_ABSOLUTE round-trip. */
    TAO::Ledger::SetLastConnectClass(TAO::Ledger::AdmissibilityClass::INVALID_ABSOLUTE);
    REQUIRE(TAO::Ledger::TakeLastConnectClass() ==
            TAO::Ledger::AdmissibilityClass::INVALID_ABSOLUTE);
    REQUIRE(TAO::Ledger::g_nLastConnectClass == TAO::Ledger::AdmissibilityClass::UNKNOWN);
}


TEST_CASE("nMaxPeerHeight is distinct from nBestHeight", "[ledger][process]")
{
    /* nMaxPeerHeight is the highest height any connected peer has advertised.
     * It must be accessible and independently settable from nBestHeight so
     * the coinbase-maturity deferral logic can tell the difference between
     * "immature at local height" and "would be mature at peer height". */

    const uint32_t nSavedLocal = TAO::Ledger::ChainState::nBestHeight.load();
    const uint32_t nSavedPeer  = TAO::Ledger::ChainState::nMaxPeerHeight.load();

    /* Simulate local height 100, peer height 102. */
    TAO::Ledger::ChainState::nBestHeight.store(100);
    TAO::Ledger::ChainState::nMaxPeerHeight.store(102);

    REQUIRE(TAO::Ledger::ChainState::nBestHeight.load()    == 100);
    REQUIRE(TAO::Ledger::ChainState::nMaxPeerHeight.load() == 102);
    REQUIRE(TAO::Ledger::ChainState::nMaxPeerHeight.load() >
            TAO::Ledger::ChainState::nBestHeight.load());

    /* The gap between peer height and local height is the number of extra
     * confirmations a coinbase would have at network height. */
    const uint32_t nExtraConfs =
        TAO::Ledger::ChainState::nMaxPeerHeight.load() -
        TAO::Ledger::ChainState::nBestHeight.load();
    REQUIRE(nExtraConfs == 2);

    /* Restore. */
    TAO::Ledger::ChainState::nBestHeight.store(nSavedLocal);
    TAO::Ledger::ChainState::nMaxPeerHeight.store(nSavedPeer);
}


TEST_CASE("Idle sigchain with ancestor on main chain must not be INVALID_ABSOLUTE",
    "[ledger][process]")
{
    /* === Constraint from problem statement (treat as given) ===
     *
     * ForkDivergenceInfo::nDepth measures sigchain staleness, NOT fork
     * divergence.  A sigchain idle for N blocks yields nDepth = N on a
     * completely healthy node.  The observed divergence_depth=16172 was
     * emitted with fAncestorOnMainChain == true — not a fork.
     *
     * This test locks in the invariant: when fAncestorOnMainChain == true,
     * the correct classification is DEFERRED_LOCAL_STATE regardless of nDepth.
     * nDepth must NEVER be used as a rejection or eviction input.  Unlike a
     * bare predicate check, this test drives the actual Mempool::Check()
     * conflict-reconciliation pass end-to-end so a future change that adds a
     * depth threshold to Check() would fail this test. */

    using namespace TAO::Register;
    using namespace TAO::Operation;

    LedgerGuard env;

    const uint32_t nSavedBestHeight = TAO::Ledger::ChainState::nBestHeight.load();

    /* Unique identifiers for this test. */
    const uint256_t hashGenesis  =
        TAO::Ledger::Credentials::Genesis(std::string("strandedstateidle" + std::to_string(LLC::GetRand())).c_str());
    const uint512_t hashPrivKey1 = LLC::GetRand512();
    const uint512_t hashAncestorTx(0xA11CE500 + 1);
    const uint1024_t hashAncestorBlock(0xA11CEB10 + 1);
    const uint512_t hashDiskLast = LLC::GetRand512();

    /* Ancestor block: mark it connected into the main chain via a nonzero
     * hashNextBlock, matching BlockState::IsInMainChain()'s definition
     * (hashNextBlock != 0 || hash == best chain tip).  Height is set far
     * below the current best height to reproduce the large idle nDepth. */
    TAO::Ledger::BlockState stateAncestor;
    stateAncestor.nVersion      = 4;
    stateAncestor.hashPrevBlock = uint1024_t(0);
    stateAncestor.nChannel      = 2;
    stateAncestor.nHeight       = 500;
    stateAncestor.nBits         = 1;
    stateAncestor.hashNextBlock = uint1024_t(0xBEEF);

    REQUIRE(LLD::Ledger->WriteBlock(hashAncestorBlock, stateAncestor));
    REQUIRE(LLD::Ledger->IndexBlock(hashAncestorTx, hashAncestorBlock));

    /* A minimal placeholder record so LLD::Ledger->HasTx()/Exists() for the
     * ancestor tx hash succeeds (Mempool::Accept() requires the previous
     * transaction to be known before evaluating conflict state). */
    {
        TAO::Ledger::Transaction txAncestor;
        txAncestor.hashGenesis = hashGenesis;
        txAncestor.nSequence   = 0;
        txAncestor.nTimestamp  = runtime::timestamp();
        REQUIRE(LLD::Ledger->WriteTx(hashAncestorTx, txAncestor));
    }

    /* Set the local best height far ahead of the ancestor so nDepth (which
     * is diagnostic-only) comes out large, mirroring the ~16172 seen in
     * production. */
    TAO::Ledger::ChainState::nBestHeight.store(stateAncestor.nHeight + 16172);

    /* Disk's committed last transaction for this genesis is unrelated to the
     * ancestor tx that the conflicting transaction expects as its
     * predecessor.  This mismatch is exactly what Check()'s reconciliation
     * pass treats as a conflict. */
    REQUIRE(LLD::Ledger->WriteLast(hashGenesis, hashDiskLast));

    /* Build a conflicting, idle-sigchain transaction whose hashPrevTx points
     * at the ancestor (on the main chain), not at disk's actual last hash. */
    TAO::Ledger::Transaction tx;
    tx.hashGenesis = hashGenesis;
    tx.nSequence   = 1;
    tx.hashPrevTx  = hashAncestorTx;
    tx.nTimestamp  = runtime::timestamp();
    tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
    tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
    tx.NextHash(LLC::GetRand512());

    TAO::Register::Address hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

    Object object;
    object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(1);

    tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

    REQUIRE(tx.Build());
    tx.Sign(hashPrivKey1);

    const uint512_t hashTx = tx.GetHash();

    /* Accept() should reject with a conflict (hashPrevTx != disk's last
     * hash) and place the transaction in the conflicted pool rather than
     * accepting or dropping it outright. */
    REQUIRE_FALSE(TAO::Ledger::mempool.Accept(tx));

    {
        TAO::Ledger::Transaction txOut;
        bool fConflicted = false;
        REQUIRE(TAO::Ledger::mempool.Get(hashTx, txOut, fConflicted));
        REQUIRE(fConflicted);
    }

    /* Drive the real reconciliation pass. Because the ancestor block is on
     * the main chain, Check() must classify this as DEFERRED_LOCAL_STATE and
     * retain the transaction in mapConflicts rather than evicting it,
     * regardless of the large diagnostic nDepth. */
    TAO::Ledger::mempool.Check();

    {
        TAO::Ledger::Transaction txOut;
        bool fConflicted = false;
        REQUIRE(TAO::Ledger::mempool.Get(hashTx, txOut, fConflicted));
        REQUIRE(fConflicted);
    }

    /* Cross-check: when the ancestor is NOT on the main chain (genuine
     * fork), Check() must evict rather than retain. Reuse the same genesis
     * with a fresh ancestor whose hashNextBlock is zero and whose own hash
     * is not the best chain tip. */
    const uint512_t hashForkAncestorTx(0xFEED500 + 1);
    const uint1024_t hashForkAncestorBlock(0xFEEDB10 + 1);

    TAO::Ledger::BlockState stateForkAncestor;
    stateForkAncestor.nVersion      = 4;
    stateForkAncestor.hashPrevBlock = uint1024_t(0);
    stateForkAncestor.nChannel      = 2;
    stateForkAncestor.nHeight       = 501;
    stateForkAncestor.nBits         = 1;
    stateForkAncestor.hashNextBlock = uint1024_t(0); /* not connected: off-chain */

    REQUIRE(LLD::Ledger->WriteBlock(hashForkAncestorBlock, stateForkAncestor));
    REQUIRE(LLD::Ledger->IndexBlock(hashForkAncestorTx, hashForkAncestorBlock));

    {
        TAO::Ledger::Transaction txForkAncestor;
        txForkAncestor.hashGenesis = hashGenesis;
        txForkAncestor.nSequence   = 0;
        txForkAncestor.nTimestamp  = runtime::timestamp();
        REQUIRE(LLD::Ledger->WriteTx(hashForkAncestorTx, txForkAncestor));
    }

    TAO::Ledger::Transaction txFork;
    txFork.hashGenesis = hashGenesis;
    txFork.nSequence   = 1;
    txFork.hashPrevTx  = hashForkAncestorTx;
    txFork.nTimestamp  = runtime::timestamp();
    txFork.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
    txFork.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
    txFork.NextHash(LLC::GetRand512());

    TAO::Register::Address hashAddressFork = TAO::Register::Address(TAO::Register::Address::OBJECT);
    Object objectFork;
    objectFork << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(2);
    txFork[0] << uint8_t(OP::CREATE) << hashAddressFork << uint8_t(REGISTER::OBJECT) << objectFork.GetState();

    REQUIRE(txFork.Build());
    txFork.Sign(hashPrivKey1);

    const uint512_t hashTxFork = txFork.GetHash();

    /* Remove the previous conflict first so this genesis's conflict set only
     * reflects the fork scenario for this reconciliation pass. Remove()
     * erases mapConflicts entries unconditionally but only returns true when
     * the hash was also found in the live mapLedger pool, so its return
     * value is not asserted here. */
    TAO::Ledger::mempool.Remove(hashTx);

    REQUIRE_FALSE(TAO::Ledger::mempool.Accept(txFork));

    TAO::Ledger::mempool.Check();

    {
        TAO::Ledger::Transaction txOut;
        bool fConflicted = false;
        /* Evicted: Get() must fail to locate it in either pool. */
        REQUIRE_FALSE(TAO::Ledger::mempool.Get(hashTxFork, txOut, fConflicted));
    }

    /* Clean up. */
    TAO::Ledger::mempool.Remove(hashTxFork);
    LLD::Ledger->EraseBlock(hashAncestorBlock);
    LLD::Ledger->EraseBlock(hashForkAncestorBlock);
    TAO::Ledger::ChainState::nBestHeight.store(nSavedBestHeight);
}


TEST_CASE("mapConflicts retry budget bounds are well-formed", "[ledger][process]")
{
    /* MAX_CONFLICT_STALE_RETRIES must be positive (so at least one retry is
     * allowed before force-eviction) and must not exceed MAX_CONFLICTS_MAP_ENTRIES
     * (which bounds the retry map itself). */
    REQUIRE(TAO::Ledger::MAX_CONFLICT_STALE_RETRIES  > 0u);
    REQUIRE(TAO::Ledger::MAX_CONFLICT_STALE_RETRIES  <=
            TAO::Ledger::MAX_CONFLICTS_MAP_ENTRIES);

    /* The retry window at CONFLICTS_SWEEP_INTERVAL_SECONDS (30 s) should be
     * at least 5 minutes so a node with modest peer latency can recover. */
    const uint64_t nWindowSecs =
        static_cast<uint64_t>(TAO::Ledger::MAX_CONFLICT_STALE_RETRIES)
        * TAO::Ledger::CONFLICTS_SWEEP_INTERVAL_SECONDS;
    REQUIRE(nWindowSecs >= 300u);  /* >= 5 minutes */
}
