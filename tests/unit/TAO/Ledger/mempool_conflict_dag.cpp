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
 *   R8  Capacity clear refuses to park a detached dependent (no parent).
 *   R9  Replacing a child slot drops the displaced child's entire tail.
 *   R10 Failed root re-admission drops the parked tail (does not process it).
 *   R11 Failed dependent re-admission drops the remaining tail.
 *   R12 Removing an intermediate dependent drops its complete tail.
 *
 * Implemented as an inline simulator so the contract is testable without
 * LLD/Accept signature wiring. Production wiring lives in
 * src/TAO/Ledger/mempool.cpp (AddConflictRoot / ParkConflictDependent /
 * DropConflictTree / DropConflictDependents / ProcessConflictDependents).
 * The simulator mirrors those helpers 1:1 so regressions in the rules surface
 * here even when full mempool Accept fixtures are unavailable.
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
        static constexpr std::size_t MAX_ROOTS = 10000;
        static constexpr std::size_t MAX_DEPS  = 10000;

        /* Test hook: override caps to exercise Bound without 10k inserts. */
        std::size_t nMaxRoots = MAX_ROOTS;
        std::size_t nMaxDeps  = MAX_DEPS;

        std::map<uint64_t, SimTx> mapRoots;            /* hash -> tx */
        std::map<uint64_t, uint64_t> mapRootByGenesis;  /* genesis -> earliest root hash */
        std::map<uint64_t, SimTx> mapDeps;              /* hashPrev -> child */
        std::map<uint64_t, SimTx> mapDepsByIndex;       /* hash -> child */
        std::vector<uint64_t> vAdmitted;                /* resolve walk log */
        std::set<uint64_t> setFailAccept;               /* hashes whose Accept fails */


        void Bound()
        {
            if(mapRoots.size() >= nMaxRoots || mapDepsByIndex.size() >= nMaxDeps)
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


        /* Drop children/tail hanging off hashParent (not hashParent itself). */
        void DropDependents(uint64_t hashParent)
        {
            uint64_t cur = hashParent;
            while(mapDeps.count(cur))
            {
                const SimTx child = mapDeps[cur];
                mapDeps.erase(cur);
                mapDepsByIndex.erase(child.hash);
                cur = child.hash;
            }
        }


        void DropTree(uint64_t hashRoot)
        {
            EraseRoot(hashRoot);
            DropDependents(hashRoot);
        }


        bool IsConflictNode(uint64_t hash) const
        {
            return mapRoots.count(hash) || mapDepsByIndex.count(hash);
        }


        bool ParkDependent(const SimTx& tx)
        {
            if(mapDepsByIndex.count(tx.hash))
                return true;

            Bound();

            /* R8: refuse detached park after capacity clear wiped the parent. */
            if(!IsConflictNode(tx.hashPrev))
                return false;

            const auto it = mapDeps.find(tx.hashPrev);
            if(it != mapDeps.end())
            {
                if(it->second.hash == tx.hash)
                    return true;
                if(tx.nSequence >= it->second.nSequence)
                    return false;

                /* R9: drop displaced child's entire tail before replacing. */
                const uint64_t displaced = it->second.hash;
                DropDependents(displaced);
                mapDepsByIndex.erase(displaced);
            }

            mapDeps[tx.hashPrev] = tx;
            mapDepsByIndex[tx.hash] = tx;
            return true;
        }


        /* Simulate Accept: succeed unless hash is in setFailAccept. */
        bool AcceptSim(uint64_t hash)
        {
            if(setFailAccept.count(hash))
                return false;
            vAdmitted.push_back(hash);
            return true;
        }


        /* ProcessDependents: stop + drop tail on Accept failure (R11). */
        void ProcessDependents(uint64_t hashParent)
        {
            uint64_t cur = hashParent;
            while(mapDeps.count(cur))
            {
                const SimTx child = mapDeps[cur];
                mapDeps.erase(cur);
                mapDepsByIndex.erase(child.hash);

                if(!AcceptSim(child.hash))
                {
                    DropDependents(child.hash);
                    return;
                }

                cur = child.hash;
            }
        }


        /* Resolve a root: erase, try admit root, drain only on success (R10). */
        void ResolveRoot(uint64_t hashRoot)
        {
            EraseRoot(hashRoot);

            if(AcceptSim(hashRoot))
                ProcessDependents(hashRoot);
            else
                DropDependents(hashRoot);
        }


        /* Remove one node (root or dependent) and preserve DAG invariant (R12). */
        void Remove(uint64_t hash)
        {
            if(mapRoots.count(hash))
            {
                DropTree(hash);
                return;
            }

            if(mapDepsByIndex.count(hash))
            {
                const SimTx dep = mapDepsByIndex[hash];
                const auto it = mapDeps.find(dep.hashPrev);
                if(it != mapDeps.end() && it->second.hash == hash)
                    mapDeps.erase(it);

                mapDepsByIndex.erase(hash);
                DropDependents(hash);
            }
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


TEST_CASE("Option C DAG: capacity clear refuses detached dependent park",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;
    /* Keep root cap high so the first park is not wiped by a full root set;
     * only the dependent cap triggers Bound. */
    dag.nMaxRoots = 100;
    dag.nMaxDeps  = 1;

    dag.AddRoot(SimTx{1, 0, 1, 1});
    REQUIRE(dag.ParkDependent(SimTx{2, 1, 1, 2}));
    REQUIRE(dag.Conflicts() == 1);
    REQUIRE(dag.Dependents() == 1);

    /* Next park hits dep cap → Bound clears entire DAG, then parent is gone
     * so the new child must NOT be parked detached (R8). */
    REQUIRE_FALSE(dag.ParkDependent(SimTx{3, 1, 1, 3}));
    REQUIRE(dag.Conflicts() == 0);
    REQUIRE(dag.Dependents() == 0);
    REQUIRE_FALSE(dag.IsConflictNode(3));
}


TEST_CASE("Option C DAG: replacing child drops displaced tail",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;

    dag.AddRoot(SimTx{1, 0, 1, 1});

    /* late child with its own grandchild tail */
    REQUIRE(dag.ParkDependent(SimTx{20, 1, 1, 5}));
    REQUIRE(dag.ParkDependent(SimTx{30, 20, 1, 6}));
    REQUIRE(dag.ParkDependent(SimTx{40, 30, 1, 7}));
    REQUIRE(dag.Dependents() == 3);

    /* earlier child replaces 20 — must drop 30 and 40 (R9). */
    REQUIRE(dag.ParkDependent(SimTx{21, 1, 1, 3}));
    REQUIRE(dag.Dependents() == 1);
    REQUIRE(dag.mapDepsByIndex.count(21) == 1);
    REQUIRE(dag.mapDepsByIndex.count(20) == 0);
    REQUIRE(dag.mapDepsByIndex.count(30) == 0);
    REQUIRE(dag.mapDepsByIndex.count(40) == 0);
    REQUIRE(dag.mapDeps.count(20) == 0);
    REQUIRE(dag.mapDeps.count(30) == 0);
}


TEST_CASE("Option C DAG: failed root re-admission drops parked tail",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;

    dag.AddRoot(SimTx{1, 100, 7, 5});
    dag.ParkDependent(SimTx{2, 1, 7, 6});
    dag.ParkDependent(SimTx{3, 2, 7, 7});

    dag.setFailAccept.insert(1); /* root Accept fails */

    /* R10 */
    dag.ResolveRoot(1);

    REQUIRE(dag.vAdmitted.empty());
    REQUIRE(dag.Conflicts() == 0);
    REQUIRE(dag.Dependents() == 0);
    REQUIRE(dag.mapDeps.count(1) == 0);
    REQUIRE(dag.mapDeps.count(2) == 0);
}


TEST_CASE("Option C DAG: failed dependent re-admission drops remaining tail",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;

    dag.AddRoot(SimTx{1, 100, 7, 5});
    dag.ParkDependent(SimTx{2, 1, 7, 6});
    dag.ParkDependent(SimTx{3, 2, 7, 7});
    dag.ParkDependent(SimTx{4, 3, 7, 8});

    dag.setFailAccept.insert(3); /* middle dependent fails */

    /* R11 */
    dag.ResolveRoot(1);

    REQUIRE(dag.vAdmitted.size() == 2);
    REQUIRE(dag.vAdmitted[0] == 1);
    REQUIRE(dag.vAdmitted[1] == 2);
    REQUIRE(dag.Conflicts() == 0);
    REQUIRE(dag.Dependents() == 0);
    REQUIRE(dag.mapDepsByIndex.count(3) == 0);
    REQUIRE(dag.mapDepsByIndex.count(4) == 0);
}


TEST_CASE("Option C DAG: remove intermediate dependent drops complete tail",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;

    dag.AddRoot(SimTx{1, 0, 1, 1});
    dag.ParkDependent(SimTx{2, 1, 1, 2});
    dag.ParkDependent(SimTx{3, 2, 1, 3});
    dag.ParkDependent(SimTx{4, 3, 1, 4});
    REQUIRE(dag.Dependents() == 3);

    /* R12: remove middle node 2 → drop 3 and 4; root remains. */
    dag.Remove(2);

    REQUIRE(dag.Conflicts() == 1);
    REQUIRE(dag.Dependents() == 0);
    REQUIRE(dag.mapRoots.count(1) == 1);
    REQUIRE(dag.mapDepsByIndex.count(2) == 0);
    REQUIRE(dag.mapDepsByIndex.count(3) == 0);
    REQUIRE(dag.mapDepsByIndex.count(4) == 0);
    REQUIRE(dag.mapDeps.count(1) == 0);
}


TEST_CASE("Option C DAG: remove root drops entire tree",
          "[mempool_conflict_dag]")
{
    ConflictDAG dag;

    dag.AddRoot(SimTx{1, 0, 1, 1});
    dag.ParkDependent(SimTx{2, 1, 1, 2});
    dag.ParkDependent(SimTx{3, 2, 1, 3});

    dag.Remove(1);

    REQUIRE(dag.Conflicts() == 0);
    REQUIRE(dag.Dependents() == 0);
    REQUIRE(dag.mapRootByGenesis.count(1) == 0);
}
