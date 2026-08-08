/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

/*
 * Option C — Per-genesis conflict DAG contract tests.
 *
 * These pin the pure data-structure rules that Mempool now enforces:
 *
 *   R1  Roots only in the root set (direct tip disagreement).
 *   R2  Descendants park as dependents under their parent; never become roots
 *       solely because their parent is conflicted.
 *   R3  One dependent slot per parent; earliest nSequence wins.
 *   R4  DropConflictTree removes the root and the whole parked tail.
 *   R5  Per-genesis earliest-root index tracks the lowest nSequence root.
 *   R6  Conflicts() counts roots only; dependents are a separate counter.
 *   R7  Resolve walk re-admits parent then drains dependents in chain order.
 *
 * Implemented as an inline simulator so the contract is testable without
 * LLD/Accept signature wiring. Production wiring lives in
 * src/TAO/Ledger/mempool.cpp (AddConflictRoot / ParkConflictDependent /
 * DropConflictTree / ProcessConflictDependents).
 */

#include <unit/catch2/catch.hpp>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <limits>


namespace
{
    struct SimTx
    {
        uint64_t hash        = 0;
        uint64_t hashPrev    = 0;
        uint64_t hashGenesis = 0;
        uint32_t nSequence   = 0;
    };


    struct ConflictDAG
    {
        std::map<uint64_t, SimTx> mapRoots;            /* hash -> tx */
        std::map<uint64_t, uint64_t> mapRootByGenesis;  /* genesis -> earliest root hash */
        std::map<uint64_t, SimTx> mapDeps;              /* hashPrev -> child */
        std::map<uint64_t, SimTx> mapDepsByIndex;       /* hash -> child */
        std::vector<uint64_t> vAdmitted;                /* resolve walk log */


        void Bound()
        {
            if(mapRoots.size() >= 10000 || mapDepsByIndex.size() >= 10000)
            {
                mapRoots.clear();
                mapRootByGenesis.clear();
                mapDeps.clear();
                mapDepsByIndex.clear();
            }
        }


        void AddRoot(const SimTx& tx)
        {
            if(mapRoots.count(tx.hash))
                return;

            Bound();
            mapRoots[tx.hash] = tx;

            const auto it = mapRootByGenesis.find(tx.hashGenesis);
            if(it == mapRootByGenesis.end())
            {
                mapRootByGenesis[tx.hashGenesis] = tx.hash;
            }
            else
            {
                const auto itRoot = mapRoots.find(it->second);
                if(itRoot == mapRoots.end() || tx.nSequence < itRoot->second.nSequence)
                    it->second = tx.hash;
            }
        }


        void EraseRoot(uint64_t hash)
        {
            const auto it = mapRoots.find(hash);
            if(it == mapRoots.end())
                return;

            const uint64_t genesis = it->second.hashGenesis;
            mapRoots.erase(it);

            const auto itG = mapRootByGenesis.find(genesis);
            if(itG == mapRootByGenesis.end())
                return;

            if(itG->second != hash && mapRoots.count(itG->second))
                return;

            uint64_t best = 0;
            uint32_t bestSeq = std::numeric_limits<uint32_t>::max();
            bool found = false;
            for(const auto& e : mapRoots)
            {
                if(e.second.hashGenesis != genesis)
                    continue;
                if(!found || e.second.nSequence < bestSeq)
                {
                    found = true;
                    bestSeq = e.second.nSequence;
                    best = e.first;
                }
            }

            if(found)
                itG->second = best;
            else
                mapRootByGenesis.erase(itG);
        }


        void DropTree(uint64_t hashRoot)
        {
            EraseRoot(hashRoot);

            uint64_t parent = hashRoot;
            while(mapDeps.count(parent))
            {
                const SimTx child = mapDeps[parent];
                mapDeps.erase(parent);
                mapDepsByIndex.erase(child.hash);
                parent = child.hash;
            }
        }


        bool ParkDependent(const SimTx& tx)
        {
            if(mapDepsByIndex.count(tx.hash))
                return true;

            Bound();

            const auto it = mapDeps.find(tx.hashPrev);
            if(it != mapDeps.end())
            {
                if(it->second.hash == tx.hash)
                    return true;
                if(tx.nSequence >= it->second.nSequence)
                    return false;

                mapDepsByIndex.erase(it->second.hash);
            }

            mapDeps[tx.hashPrev] = tx;
            mapDepsByIndex[tx.hash] = tx;
            return true;
        }


        bool IsConflictNode(uint64_t hash) const
        {
            return mapRoots.count(hash) || mapDepsByIndex.count(hash);
        }


        /* Simulate Accept succeeding for every parked dependent. */
        void ProcessDependents(uint64_t hashParent)
        {
            uint64_t cur = hashParent;
            while(mapDeps.count(cur))
            {
                const SimTx child = mapDeps[cur];
                mapDeps.erase(cur);
                mapDepsByIndex.erase(child.hash);
                vAdmitted.push_back(child.hash);
                cur = child.hash;
            }
        }


        /* Resolve a root: erase, admit root, drain dependents. */
        void ResolveRoot(uint64_t hashRoot)
        {
            EraseRoot(hashRoot);
            vAdmitted.push_back(hashRoot);
            ProcessDependents(hashRoot);
        }


        std::size_t Conflicts() const { return mapRoots.size(); }
        std::size_t Dependents() const { return mapDepsByIndex.size(); }
    };
}


TEST_CASE("Option C DAG: root-only insert does not cascade descendants",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;

    SimTx root{1, 100, 7, 5};
    SimTx child{2, 1, 7, 6};
    SimTx grand{3, 2, 7, 7};

    dag.AddRoot(root);
    REQUIRE(dag.IsConflictNode(child.hashPrev));
    REQUIRE(dag.ParkDependent(child));
    REQUIRE(dag.IsConflictNode(grand.hashPrev));
    REQUIRE(dag.ParkDependent(grand));

    /* R1/R2: only the root is in the root set. */
    REQUIRE(dag.Conflicts() == 1);
    REQUIRE(dag.Dependents() == 2);
    REQUIRE(dag.mapRoots.count(1) == 1);
    REQUIRE(dag.mapRoots.count(2) == 0);
    REQUIRE(dag.mapRoots.count(3) == 0);
}


TEST_CASE("Option C DAG: earliest-sequence dependent wins parent slot",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;

    SimTx root{10, 0, 1, 1};
    dag.AddRoot(root);

    SimTx late{20, 10, 1, 5};
    SimTx early{21, 10, 1, 3};

    REQUIRE(dag.ParkDependent(late));
    REQUIRE(dag.Dependents() == 1);

    /* R3: earlier sequence replaces later occupant. */
    REQUIRE(dag.ParkDependent(early));
    REQUIRE(dag.Dependents() == 1);
    REQUIRE(dag.mapDepsByIndex.count(21) == 1);
    REQUIRE(dag.mapDepsByIndex.count(20) == 0);

    /* Later contender is dropped. */
    SimTx later{22, 10, 1, 9};
    REQUIRE_FALSE(dag.ParkDependent(later));
    REQUIRE(dag.Dependents() == 1);
}


TEST_CASE("Option C DAG: DropConflictTree removes root and whole tail",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;

    SimTx root{1, 50, 9, 2};
    SimTx c1{2, 1, 9, 3};
    SimTx c2{3, 2, 9, 4};

    dag.AddRoot(root);
    dag.ParkDependent(c1);
    dag.ParkDependent(c2);

    REQUIRE(dag.Conflicts() == 1);
    REQUIRE(dag.Dependents() == 2);

    /* R4 */
    dag.DropTree(1);
    REQUIRE(dag.Conflicts() == 0);
    REQUIRE(dag.Dependents() == 0);
    REQUIRE(dag.mapRootByGenesis.count(9) == 0);
}


TEST_CASE("Option C DAG: per-genesis index tracks earliest root",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;

    SimTx rLate{11, 1, 42, 8};
    SimTx rEarly{12, 1, 42, 3};
    SimTx rOther{13, 2, 99, 1};

    dag.AddRoot(rLate);
    REQUIRE(dag.mapRootByGenesis[42] == 11);

    /* R5: earlier sequence becomes the index tip. */
    dag.AddRoot(rEarly);
    REQUIRE(dag.mapRootByGenesis[42] == 12);

    dag.AddRoot(rOther);
    REQUIRE(dag.mapRootByGenesis[99] == 13);

    /* Erasing earliest promotes the remaining root. */
    dag.EraseRoot(12);
    REQUIRE(dag.mapRootByGenesis[42] == 11);

    dag.EraseRoot(11);
    REQUIRE(dag.mapRootByGenesis.count(42) == 0);
}


TEST_CASE("Option C DAG: Conflicts counts roots only",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;

    dag.AddRoot(SimTx{1, 0, 1, 1});
    dag.AddRoot(SimTx{2, 0, 2, 1});
    dag.ParkDependent(SimTx{3, 1, 1, 2});
    dag.ParkDependent(SimTx{4, 3, 1, 3});
    dag.ParkDependent(SimTx{5, 2, 2, 2});

    /* R6 */
    REQUIRE(dag.Conflicts() == 2);
    REQUIRE(dag.Dependents() == 3);
}


TEST_CASE("Option C DAG: resolve walk admits root then dependents in order",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;

    dag.AddRoot(SimTx{1, 100, 7, 5});
    dag.ParkDependent(SimTx{2, 1, 7, 6});
    dag.ParkDependent(SimTx{3, 2, 7, 7});

    /* R7 */
    dag.ResolveRoot(1);

    REQUIRE(dag.vAdmitted.size() == 3);
    REQUIRE(dag.vAdmitted[0] == 1);
    REQUIRE(dag.vAdmitted[1] == 2);
    REQUIRE(dag.vAdmitted[2] == 3);
    REQUIRE(dag.Conflicts() == 0);
    REQUIRE(dag.Dependents() == 0);
}
