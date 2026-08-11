/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

/*
 * Focused regression tests for ValidateVtxSigchainConsistency()'s current
 * Connect-aligned predecessor model:
 *   (1) reject malformed self-reference
 *   (2) use in-block mapLast first
 *   (3) otherwise use disk-only ReadLast()
 *   (4) if no disk anchor exists, defer to Connect()
 *
 * Construction-time mempool-only predecessor filtering is covered separately by
 * tests/unit/TAO/Ledger/filter_mempool_only_predecessor.cpp.
 */

#include <LLC/include/random.h>

#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/transaction.h>

#include <unit/catch2/catch.hpp>

#include <map>
#include <vector>

namespace
{
    TAO::Ledger::Transaction MakeTx(const uint256_t& hashGenesis, const uint32_t nSeq,
                                    const uint512_t hashPrevTx = 0, const uint32_t nTimeOffset = 0)
    {
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = hashGenesis;
        tx.nSequence   = nSeq;
        tx.hashPrevTx  = hashPrevTx;
        tx.nTimestamp  = 1700000000u + nSeq + nTimeOffset;
        tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
        return tx;
    }

    bool SimulateConsistencyCheck(
        const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>>& vtxPairs,
        const std::map<uint256_t, uint512_t>& mapDiskLast)
    {
        std::map<uint256_t, uint512_t> mapLast;

        for(const auto& entry : vtxPairs)
        {
            const uint512_t& txHash = entry.first;
            const TAO::Ledger::Transaction& tx = entry.second;

            if(tx.IsFirst())
            {
                mapLast[tx.hashGenesis] = txHash;
                continue;
            }

            if(tx.hashPrevTx == txHash)
                return false;

            uint512_t hashLast = 0;
            bool fAnchorFound = false;

            if(mapLast.count(tx.hashGenesis))
            {
                hashLast = mapLast.at(tx.hashGenesis);
                fAnchorFound = true;
            }
            else
            {
                const auto itDisk = mapDiskLast.find(tx.hashGenesis);
                if(itDisk != mapDiskLast.end())
                {
                    hashLast = itDisk->second;
                    fAnchorFound = true;
                }
                else
                {
                    mapLast[tx.hashGenesis] = txHash;
                    continue;
                }
            }

            if(fAnchorFound && tx.hashPrevTx != hashLast)
                return false;

            mapLast[tx.hashGenesis] = txHash;
        }

        return true;
    }
}

TEST_CASE("ValidateVtxSigchainConsistency: mempool-only predecessor is rejected by disk/in-block submit checks",
    "[validate_vtx_consistency][ledger]")
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    /* txSeq2 references mempool-only predecessor hashSeq1, but this tx is not
     * in-block chained from txSeq1 and disk anchor still points to hashSeq0. */
    TAO::Ledger::Transaction txSeq2 = MakeTx(genesis, 2, hashSeq1);
    const uint512_t hashSeq2 = txSeq2.GetHash();

    const std::map<uint256_t, uint512_t> diskLast = {{genesis, hashSeq0}};
    const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {{hashSeq2, txSeq2}};

    REQUIRE(SimulateConsistencyCheck(vtx, diskLast) == false);
}

TEST_CASE("ValidateVtxSigchainConsistency: disk-confirmed predecessor is accepted",
    "[validate_vtx_consistency][ledger]")
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    const std::map<uint256_t, uint512_t> diskLast = {{genesis, hashSeq0}};
    const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {{hashSeq1, txSeq1}};

    REQUIRE(SimulateConsistencyCheck(vtx, diskLast) == true);
}

TEST_CASE("ValidateVtxSigchainConsistency: stale predecessor versus disk anchor is rejected",
    "[validate_vtx_consistency][ledger]")
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    TAO::Ledger::Transaction txSeq2Stale = MakeTx(genesis, 2, hashSeq0);
    const uint512_t hashSeq2Stale = txSeq2Stale.GetHash();

    const std::map<uint256_t, uint512_t> diskLast = {{genesis, hashSeq1}};
    const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {{hashSeq2Stale, txSeq2Stale}};

    REQUIRE(SimulateConsistencyCheck(vtx, diskLast) == false);
}

TEST_CASE("ValidateVtxSigchainConsistency: in-block same-genesis chain is accepted",
    "[validate_vtx_consistency][ledger]")
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    TAO::Ledger::Transaction txSeq2 = MakeTx(genesis, 2, hashSeq1);
    const uint512_t hashSeq2 = txSeq2.GetHash();

    const std::map<uint256_t, uint512_t> diskLast = {{genesis, hashSeq0}};
    const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq1, txSeq1},
        {hashSeq2, txSeq2}
    };

    REQUIRE(SimulateConsistencyCheck(vtx, diskLast) == true);
}

TEST_CASE("ValidateVtxSigchainConsistency: stale second in-block tx is rejected",
    "[validate_vtx_consistency][ledger]")
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq0 = MakeTx(genesis, 0);
    const uint512_t hashSeq0 = txSeq0.GetHash();

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, hashSeq0);
    const uint512_t hashSeq1 = txSeq1.GetHash();

    TAO::Ledger::Transaction txSeq2Stale = MakeTx(genesis, 2, hashSeq0);
    const uint512_t hashSeq2Stale = txSeq2Stale.GetHash();

    const std::map<uint256_t, uint512_t> diskLast = {{genesis, hashSeq0}};
    const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {
        {hashSeq1, txSeq1},
        {hashSeq2Stale, txSeq2Stale}
    };

    REQUIRE(SimulateConsistencyCheck(vtx, diskLast) == false);
}

TEST_CASE("ValidateVtxSigchainConsistency: IsFirst skips predecessor check",
    "[validate_vtx_consistency][ledger]")
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txGenesis = MakeTx(genesis, 0);
    const uint512_t hashGenesisTx = txGenesis.GetHash();

    REQUIRE(txGenesis.IsFirst());

    const std::map<uint256_t, uint512_t> diskLast;
    const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {{hashGenesisTx, txGenesis}};

    REQUIRE(SimulateConsistencyCheck(vtx, diskLast) == true);
}

TEST_CASE("ValidateVtxSigchainConsistency: missing disk anchor defers to Connect",
    "[validate_vtx_consistency][ledger]")
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txSeq1 = MakeTx(genesis, 1, uint512_t(LLC::GetRand512()));
    const uint512_t hashSeq1 = txSeq1.GetHash();

    const std::map<uint256_t, uint512_t> diskLast;
    const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {{hashSeq1, txSeq1}};

    REQUIRE(SimulateConsistencyCheck(vtx, diskLast) == true);
}

TEST_CASE("ValidateVtxSigchainConsistency: malformed self-reference is rejected",
    "[validate_vtx_consistency][ledger]")
{
    const uint256_t genesis = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction tx = MakeTx(genesis, 1, uint512_t(0));
    const uint512_t txHash = tx.GetHash();
    tx.hashPrevTx = txHash;

    const std::map<uint256_t, uint512_t> diskLast;
    const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtx = {{txHash, tx}};

    REQUIRE(SimulateConsistencyCheck(vtx, diskLast) == false);
}

TEST_CASE("ValidateVtxSigchainConsistency: cross-genesis anchors are independent",
    "[validate_vtx_consistency][ledger]")
{
    const uint256_t genesisA = TAO::Ledger::Genesis(LLC::GetRand256(), true);
    const uint256_t genesisB = TAO::Ledger::Genesis(LLC::GetRand256(), true);

    TAO::Ledger::Transaction txA0 = MakeTx(genesisA, 0);
    const uint512_t hashA0 = txA0.GetHash();
    TAO::Ledger::Transaction txA1 = MakeTx(genesisA, 1, hashA0);
    const uint512_t hashA1 = txA1.GetHash();

    TAO::Ledger::Transaction txB0 = MakeTx(genesisB, 0);
    const uint512_t hashB0 = txB0.GetHash();
    TAO::Ledger::Transaction txB0Alt = MakeTx(genesisB, 0, 0, 999);
    const uint512_t hashB0Alt = txB0Alt.GetHash();
    TAO::Ledger::Transaction txB1Stale = MakeTx(genesisB, 1, hashB0Alt);
    const uint512_t hashB1Stale = txB1Stale.GetHash();

    const std::map<uint256_t, uint512_t> diskLast = {
        {genesisA, hashA0},
        {genesisB, hashB0}
    };

    const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxMixed = {
        {hashA1, txA1},
        {hashB1Stale, txB1Stale}
    };
    REQUIRE(SimulateConsistencyCheck(vtxMixed, diskLast) == false);

    const std::vector<std::pair<uint512_t, TAO::Ledger::Transaction>> vtxAOnly = {
        {hashA1, txA1}
    };
    REQUIRE(SimulateConsistencyCheck(vtxAOnly, diskLast) == true);
}
