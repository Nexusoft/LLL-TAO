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
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/tritium.h>

#include <TAO/Operation/include/enum.h>

#include <Util/include/args.h>
#include <Util/include/filesystem.h>
#include <Util/include/runtime.h>

#include <unit/catch2/catch.hpp>

#include <algorithm>


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
     * the block's missing-tx markers are cleared (hashMissing = 0 signals the
     * LLP layer to escalate to full branch recovery), and — critically — the
     * mapLastMissing entry is ERASED so the next arrival of this block starts
     * a fresh retry cycle rather than being permanently silenced. */
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
        REQUIRE(block.vMissing.empty());
        REQUIRE(block.hashMissing == 0);
        /* Counter must be erased — a leftover >50 value would permanently
         * wedge this block on every future arrival. */
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) == 0);
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

    /* The escalation path must clear the missing markers. */
    REQUIRE(parentBlock2.vMissing.empty());
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
