/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <TAO/Operation/types/contract.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/tritium.h>

#include <LLP/include/coinbase_validation.h>

#include <LLC/types/uint1024.h>

#include <cstdint>
#include <vector>


/** Test Suite: Coinbase Contract Stream Size Invariant
 *
 *  These tests validate the 49-byte coinbase contract layout invariant:
 *
 *    A well-formed OP::COINBASE contract in CreateProducer writes exactly:
 *      - 1 byte  : OP::COINBASE opcode (0x12)
 *      - 32 bytes: hashGenesis / hashRewardRecipient (uint256_t)
 *      - 8 bytes : nCredit (uint64_t)
 *      - 8 bytes : nExtraNonce (uint64_t)
 *      = 49 bytes total
 *
 *  The burst-block TOCTOU race (documented in docs/BURST_BLOCK_COINBASE_PRIMITIVE_OVERFLOW.md)
 *  can produce a coinbase contract stream that is NOT exactly 49 bytes.  The pre-commit guard
 *  in handle_submit_block_stateless (stateless_miner_connection.cpp) and handle_submit_block_stateless in miner.cpp catches this
 *  before AcceptMinedBlock() and returns BLOCK_REJECTED with reason MALFORMED_PRODUCER (11)
 *  instead of letting it fail deep inside TAO::Register::Verify.
 *
 *  These tests verify:
 *    1. A correctly constructed OP::COINBASE contract stream is exactly 49 bytes.
 *    2. A contract with extra trailing bytes is NOT 49 bytes (over-written stream).
 *    3. A contract that is under-written is NOT 49 bytes (partially written stream).
 *    4. The Empty() guard short-circuits correctly before Primitive() is called.
 *    5. The ambassador/developer payout layout (nExtraNonce=0) is also exactly 49 bytes.
 *
 *  These tests do NOT call CreateProducer (which requires a live chain and sigchain).
 *  They construct the contract byte-stream directly, matching what CreateProducer writes.
 **/


/* ─── constants ─────────────────────────────────────────────────────────────── */

/** Expected size of every well-formed OP::COINBASE contract stream in CreateProducer. */
static constexpr uint64_t COINBASE_STREAM_SIZE = 49;

/** OP::COINBASE opcode value. */
static constexpr uint8_t OP_COINBASE = static_cast<uint8_t>(TAO::Operation::OP::COINBASE);


/* ─── helpers ─────────────────────────────────────────────────────────────── */

/** Build a canonical well-formed OP::COINBASE contract:
 *    opcode (1) + hashGenesis (32) + nCredit (8) + nExtraNonce (8) = 49 bytes.
 *
 *  @param[in] hashGenesis    The 32-byte recipient sigchain genesis.
 *  @param[in] nCredit        The block reward to credit (uint64_t).
 *  @param[in] nExtraNonce    The extra nonce (uint64_t, 0 for ambassador/developer slots).
 *
 *  @return A fully populated Contract whose Operations() vector is exactly 49 bytes.
 **/
static TAO::Operation::Contract MakeCoinbaseContract(
    const uint256_t& hashGenesis,
    const uint64_t   nCredit,
    const uint64_t   nExtraNonce)
{
    TAO::Operation::Contract contract;
    contract << OP_COINBASE;       // 1 byte
    contract << hashGenesis;       // 32 bytes
    contract << nCredit;           // 8 bytes
    contract << nExtraNonce;       // 8 bytes
    return contract;               // total: 49 bytes
}

/** Build a contract whose stream has extra trailing bytes (simulates double-write race). */
static TAO::Operation::Contract MakeOversizedCoinbaseContract(
    const uint256_t& hashGenesis,
    const uint64_t   nCredit,
    const uint64_t   nExtraNonce)
{
    TAO::Operation::Contract contract;
    contract << OP_COINBASE;
    contract << hashGenesis;
    contract << nCredit;
    contract << nExtraNonce;
    /* Simulate a partial second-write appending extra bytes (the race scenario). */
    contract << OP_COINBASE;       // +1 extra byte
    return contract;
}

/** Build a contract whose stream is missing the nExtraNonce (partial write). */
static TAO::Operation::Contract MakeUndersizedCoinbaseContract(
    const uint256_t& hashGenesis,
    const uint64_t   nCredit)
{
    TAO::Operation::Contract contract;
    contract << OP_COINBASE;
    contract << hashGenesis;
    contract << nCredit;
    /* nExtraNonce intentionally omitted — 41 bytes only. */
    return contract;
}


/* ─── Test Cases ─────────────────────────────────────────────────────────── */

TEST_CASE("Coinbase contract: well-formed stream is exactly 49 bytes", "[coinbase][burst_block]")
{
    const uint256_t hashGenesis = uint256_t("a174011c93ca1c80bca5deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdead");
    const uint64_t  nCredit     = 1000000000ULL;   // 1 NXS in satoshis
    const uint64_t  nExtraNonce = 0x0000000100000000ULL;

    TAO::Operation::Contract contract = MakeCoinbaseContract(hashGenesis, nCredit, nExtraNonce);

    REQUIRE(contract.Operations().size() == COINBASE_STREAM_SIZE);
    REQUIRE(!contract.Empty());
    REQUIRE(contract.Primitive() == OP_COINBASE);
}


TEST_CASE("Coinbase contract: ambassador/developer slot (nExtraNonce=0) is exactly 49 bytes", "[coinbase][burst_block]")
{
    /* Ambassador and developer payout slots write uint64_t(0) for the extra nonce field.
     * This must also produce exactly 49 bytes. */
    const uint256_t hashAmbassador = uint256_t("0000000000000000000000000000000000000000000000000000000000000001");
    const uint64_t  nCredit        = 500000000ULL;
    const uint64_t  nExtraNonce    = 0;  // ambassador/developer use uint64_t(0)

    TAO::Operation::Contract contract = MakeCoinbaseContract(hashAmbassador, nCredit, nExtraNonce);

    REQUIRE(contract.Operations().size() == COINBASE_STREAM_SIZE);
    REQUIRE(!contract.Empty());
    REQUIRE(contract.Primitive() == OP_COINBASE);
}


TEST_CASE("Coinbase contract: zero hashGenesis still produces 49 bytes", "[coinbase][burst_block]")
{
    /* Edge case: a zero genesis hash writes 32 zero bytes — stream must still be 49. */
    const uint256_t hashGenesis = 0;
    const uint64_t  nCredit     = 100ULL;
    const uint64_t  nExtraNonce = 1ULL;

    TAO::Operation::Contract contract = MakeCoinbaseContract(hashGenesis, nCredit, nExtraNonce);

    REQUIRE(contract.Operations().size() == COINBASE_STREAM_SIZE);
}


TEST_CASE("Coinbase contract: oversized stream (double-write race) is NOT 49 bytes", "[coinbase][burst_block]")
{
    /* Simulates the TOCTOU burst-block race: a second partial write appends extra
     * bytes after the well-formed 49-byte stream.  The guard should catch this. */
    const uint256_t hashGenesis = uint256_t("deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef");
    const uint64_t  nCredit     = 1000000000ULL;
    const uint64_t  nExtraNonce = 0x0000000200000000ULL;

    TAO::Operation::Contract contract = MakeOversizedCoinbaseContract(hashGenesis, nCredit, nExtraNonce);

    /* The stream has 50 bytes (49 + 1 extra opcode byte from the simulated race). */
    REQUIRE(contract.Operations().size() == COINBASE_STREAM_SIZE + 1);

    /* The guard condition: operations size != 49 triggers rejection. */
    REQUIRE(contract.Operations().size() != COINBASE_STREAM_SIZE);
}


TEST_CASE("Coinbase contract: undersized stream (partial write) is NOT 49 bytes", "[coinbase][burst_block]")
{
    /* Simulates a partially written coinbase: nExtraNonce was never appended.
     * 41 bytes instead of 49. */
    const uint256_t hashGenesis = uint256_t("cafecafecafecafecafecafecafecafecafecafecafecafecafecafecafecafe");
    const uint64_t  nCredit     = 2000000000ULL;

    TAO::Operation::Contract contract = MakeUndersizedCoinbaseContract(hashGenesis, nCredit);

    /* 1 (opcode) + 32 (genesis) + 8 (credit) = 41 bytes — missing nExtraNonce. */
    REQUIRE(contract.Operations().size() == 41);
    REQUIRE(contract.Operations().size() != COINBASE_STREAM_SIZE);
}


TEST_CASE("Coinbase contract: Empty() contract does not call Primitive()", "[coinbase][burst_block]")
{
    /* The guard short-circuits with Empty() before calling Primitive() to avoid
     * the throw from Primitive() on a zero-length stream. */
    TAO::Operation::Contract contract;

    REQUIRE(contract.Empty());

    /* Verify Empty() is the correct early-exit path. */
    /* (Calling contract.Primitive() on an empty contract would throw — do NOT call it here.) */
}


TEST_CASE("Coinbase contract: non-coinbase contract is not checked by guard", "[coinbase][burst_block]")
{
    /* A contract with OP::AUTHORIZE (staking) should not be rejected by the guard.
     * The guard is OP::COINBASE-specific. */
    const uint256_t hashPrevTx = uint256_t("1111111111111111111111111111111111111111111111111111111111111111");
    const uint256_t hashGenesis = uint256_t("2222222222222222222222222222222222222222222222222222222222222222");

    TAO::Operation::Contract contract;
    contract << static_cast<uint8_t>(TAO::Operation::OP::AUTHORIZE);
    contract << hashPrevTx;   // 32 bytes
    contract << hashGenesis;  // 32 bytes

    /* 1 (opcode) + 32 + 32 = 65 bytes — NOT 49, but NOT a coinbase contract. */
    REQUIRE(contract.Operations().size() == 65);
    REQUIRE(!contract.Empty());
    REQUIRE(contract.Primitive() == static_cast<uint8_t>(TAO::Operation::OP::AUTHORIZE));

    /* Guard logic: only fires for OP::COINBASE primitive. */
    const bool fShouldReject =
        (contract.Primitive() == OP_COINBASE) &&
        (contract.Operations().size() != COINBASE_STREAM_SIZE);

    REQUIRE(fShouldReject == false);
}


TEST_CASE("Coinbase contract: multiple independent slots all produce 49 bytes", "[coinbase][burst_block]")
{
    /* Verify that multiple contract slots (miner + coinbase recipients) all
     * independently produce 49-byte streams, matching the invariant that
     * every OP::COINBASE slot in a producer must be exactly 49 bytes. */
    const uint256_t hashMiner      = uint256_t("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    const uint256_t hashRecipient1 = uint256_t("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    const uint256_t hashRecipient2 = uint256_t("cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");

    const TAO::Operation::Contract slot0 = MakeCoinbaseContract(hashMiner,      1000000000ULL, 0x100000000ULL);
    const TAO::Operation::Contract slot1 = MakeCoinbaseContract(hashRecipient1,  500000000ULL, 0x100000000ULL);
    const TAO::Operation::Contract slot2 = MakeCoinbaseContract(hashRecipient2,  500000000ULL, 0x100000000ULL);

    REQUIRE(slot0.Operations().size() == COINBASE_STREAM_SIZE);
    REQUIRE(slot1.Operations().size() == COINBASE_STREAM_SIZE);
    REQUIRE(slot2.Operations().size() == COINBASE_STREAM_SIZE);
}


/* ─── Shared helper tests (LLP::CoinbaseValidation) ──────────────────────── */

/** Build a synthetic TritiumBlock whose producer holds the supplied contracts. */
static TAO::Ledger::TritiumBlock MakeBlockWithProducerContracts(
    const std::vector<TAO::Operation::Contract>& vContracts)
{
    TAO::Ledger::TritiumBlock block;
    block.nHeight  = 1234567;
    block.nChannel = 2;       // Hash channel
    block.nNonce   = 0xfeedface;
    for(uint32_t i = 0; i < vContracts.size(); ++i)
        block.producer[i] = vContracts[i];
    return block;
}


TEST_CASE("CoinbaseValidation: well-formed producer is not flagged", "[coinbase][burst_block][shared_helper]")
{
    const uint256_t hashGenesis = uint256_t("a174011c93ca1c80bca5deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdead");
    std::vector<TAO::Operation::Contract> vContracts;
    vContracts.push_back(MakeCoinbaseContract(hashGenesis, 1000000000ULL, 0x100000000ULL));

    TAO::Ledger::TritiumBlock block = MakeBlockWithProducerContracts(vContracts);

    const auto result = LLP::CoinbaseValidation::DetectMalformedCoinbase(block);
    REQUIRE(result.malformed == false);
}


TEST_CASE("CoinbaseValidation: oversized coinbase (98 bytes) is flagged at first slot", "[coinbase][burst_block][shared_helper]")
{
    /* This reproduces the exact production failure mode: CreateProducer was called
     * with a producer that already had a 49-byte coinbase, so the second write
     * appended a duplicate 49-byte stream giving 98 bytes total.  After the
     * deterministic fix in CreateProducer (rProducer = Transaction()), this can
     * no longer happen — but the shared guard remains defense-in-depth. */
    const uint256_t hashGenesis = uint256_t("deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef");

    TAO::Operation::Contract doubled;
    doubled << OP_COINBASE; doubled << hashGenesis; doubled << uint64_t(1000000000ULL); doubled << uint64_t(0x100000000ULL);
    doubled << OP_COINBASE; doubled << hashGenesis; doubled << uint64_t(1000000000ULL); doubled << uint64_t(0x100000000ULL);

    REQUIRE(doubled.Operations().size() == 2 * COINBASE_STREAM_SIZE);

    std::vector<TAO::Operation::Contract> vContracts;
    vContracts.push_back(doubled);
    TAO::Ledger::TritiumBlock block = MakeBlockWithProducerContracts(vContracts);

    const auto result = LLP::CoinbaseValidation::DetectMalformedCoinbase(block);
    REQUIRE(result.malformed == true);
    REQUIRE(result.contract_index == 0);
    REQUIRE(result.actual_size == 2 * COINBASE_STREAM_SIZE);
}


TEST_CASE("CoinbaseValidation: malformed slot beyond miner reward is flagged with correct index", "[coinbase][burst_block][shared_helper]")
{
    const uint256_t hashMiner       = uint256_t("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    const uint256_t hashCorrupt     = uint256_t("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");

    std::vector<TAO::Operation::Contract> vContracts;
    vContracts.push_back(MakeCoinbaseContract(hashMiner, 1000000000ULL, 0x100000000ULL));
    vContracts.push_back(MakeUndersizedCoinbaseContract(hashCorrupt, 500000000ULL));   // slot 1 short

    TAO::Ledger::TritiumBlock block = MakeBlockWithProducerContracts(vContracts);

    const auto result = LLP::CoinbaseValidation::DetectMalformedCoinbase(block);
    REQUIRE(result.malformed == true);
    REQUIRE(result.contract_index == 1);
    REQUIRE(result.actual_size == 41);
}


TEST_CASE("CoinbaseValidation: ambassador/developer slot (extra_nonce=0) is not flagged", "[coinbase][burst_block][shared_helper]")
{
    const uint256_t hashMiner      = uint256_t("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    const uint256_t hashAmbassador = uint256_t("0000000000000000000000000000000000000000000000000000000000000001");
    const uint256_t hashDeveloper  = uint256_t("0000000000000000000000000000000000000000000000000000000000000002");

    std::vector<TAO::Operation::Contract> vContracts;
    vContracts.push_back(MakeCoinbaseContract(hashMiner,      1000000000ULL, 0x100000000ULL));
    vContracts.push_back(MakeCoinbaseContract(hashAmbassador,  500000000ULL, 0));
    vContracts.push_back(MakeCoinbaseContract(hashDeveloper,    50000000ULL, 0));

    TAO::Ledger::TritiumBlock block = MakeBlockWithProducerContracts(vContracts);

    const auto result = LLP::CoinbaseValidation::DetectMalformedCoinbase(block);
    REQUIRE(result.malformed == false);
}


TEST_CASE("CoinbaseValidation: empty producer is not flagged", "[coinbase][burst_block][shared_helper]")
{
    /* A producer with zero contracts (e.g. PoS channel before stake-minter writes)
     * must not be flagged as malformed. */
    TAO::Ledger::TritiumBlock block;
    block.nHeight  = 1;
    block.nChannel = 0;

    REQUIRE(block.producer.Size() == 0);

    const auto result = LLP::CoinbaseValidation::DetectMalformedCoinbase(block);
    REQUIRE(result.malformed == false);
}


/* ─── CreateProducer fix precondition test ───────────────────────────────── */

TEST_CASE("Transaction reset: assigning default-constructed Transaction clears vContracts",
          "[coinbase][burst_block][create_producer_fix]")
{
    /* Regression test for the deterministic CreateProducer fix.
     *
     * The fix at the top of TAO::Ledger::CreateProducer is:
     *
     *     rProducer = TAO::Ledger::Transaction();
     *
     * which relies on the assignment operator clearing vContracts (so that the
     * subsequent `rProducer[0] << OP::COINBASE; ...` writes start from byte 0
     * rather than appending to a stale 49-byte stream copied from the cached
     * block).  This test pins that contract: if anyone changes the assignment
     * operator to retain old contract state, this test will fail and the
     * mainnet "can not verify PRIMITIVE per contract" rejection will return.
     */
    const uint256_t hashGenesis = uint256_t("a174011c93ca1c80bca5deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdead");

    TAO::Ledger::Transaction tx;
    tx[0] = MakeCoinbaseContract(hashGenesis, 1000000000ULL, 0x100000000ULL);

    REQUIRE(tx.Size() == 1);
    REQUIRE(tx[0].Operations().size() == COINBASE_STREAM_SIZE);

    /* Apply the fix's reset. */
    tx = TAO::Ledger::Transaction();

    /* After reset the producer must be empty so subsequent coinbase writes start
     * from byte 0 and produce exactly 49 bytes — not 98. */
    REQUIRE(tx.Size() == 0);
}


TEST_CASE("Transaction reset: re-writing coinbase after reset yields 49 bytes (not 98)",
          "[coinbase][burst_block][create_producer_fix]")
{
    /* End-to-end simulation of the bug → fix path:
     *
     *   1. Producer arrives carrying a 49-byte coinbase from a cached template.
     *   2. CreateProducer (post-fix) resets the producer, then writes the coinbase.
     *   3. Final coinbase contract must be exactly 49 bytes — not 98.
     *
     * Pre-fix, step 2 was just a CreateTransaction call that did NOT reset the
     * contracts, so step 3 produced a 98-byte stream and the block was rejected
     * deep in TAO::Register::Verify.
     */
    const uint256_t hashGenesisCached = uint256_t("a174011c93ca1c80bca5deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdead");
    const uint256_t hashGenesisNew    = uint256_t("b285022d04db2d91cdb6cafefacecafefacecafefacecafefacecafefacecafe");

    TAO::Ledger::Transaction txProducer;
    /* Step 1 — simulate the cached producer copy carrying its old coinbase. */
    txProducer[0] = MakeCoinbaseContract(hashGenesisCached, 1000000000ULL, 953ULL);
    REQUIRE(txProducer[0].Operations().size() == COINBASE_STREAM_SIZE);

    /* Step 2 — apply the deterministic fix. */
    txProducer = TAO::Ledger::Transaction();

    /* Step 3 — write a fresh coinbase for the new miner reward + extra nonce
     * (this is exactly what CreateProducer does post-CreateTransaction). */
    txProducer[0] << OP_COINBASE;
    txProducer[0] << hashGenesisNew;
    txProducer[0] << uint64_t(1000000000ULL);
    txProducer[0] << uint64_t(954ULL);

    REQUIRE(txProducer[0].Operations().size() == COINBASE_STREAM_SIZE);

    /* Defense-in-depth: the shared guard would not flag this. */
    TAO::Ledger::TritiumBlock block;
    block.nHeight  = 6703531;
    block.nChannel = 2;
    block.producer = txProducer;

    const auto result = LLP::CoinbaseValidation::DetectMalformedCoinbase(block);
    REQUIRE(result.malformed == false);
}
