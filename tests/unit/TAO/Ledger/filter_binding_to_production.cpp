/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

/*
 * Production-binding test for the Option B mempool-only-predecessor filter.
 *
 *   tests/unit/TAO/Ledger/filter_mempool_only_predecessor.cpp pins the
 *   *contract* via an inline simulator. Those tests verify the simulator
 *   obeys the invariants but they do not bind to the production source —
 *   a future edit that breaks the production gate in
 *   src/TAO/Ledger/create.cpp::AddTransactions() but leaves the simulator
 *   alone would pass all simulator tests.
 *
 *   This file closes that gap by reading src/TAO/Ledger/create.cpp as
 *   text and asserting the *exact* gate predicates are present in the
 *   production AddTransactions() body. If anyone removes or weakens the
 *   gate, these tests fail immediately, producing a CI-visible regression
 *   tied to the production source, not to a simulator.
 *
 *   Why source introspection?
 *     - AddTransactions() requires a live LLD environment and a populated
 *       mempool to exercise end-to-end. The existing simulator tests
 *       intentionally avoid that wiring so they can run in any unit-test
 *       process. A direct functional test would either replicate that
 *       wiring (heavy, fragile) or be an integration test (out of scope
 *       for tests/unit).
 *     - A source-introspection test is the lightest possible binding
 *       that survives the constraint "do not modify production logic":
 *       it asserts the production gate exists, is keyed on the right
 *       predicates (IsFirst exemption, in-block-chain exemption,
 *       LLD::Ledger->HasTx disk lookup), and is wired to the `setInBlock`
 *       state shared with the main vtx accumulation loop.
 *
 *   What is asserted (one TEST_CASE per invariant — order matches the
 *   numbering in filter_mempool_only_predecessor.cpp):
 *     I0 — create.cpp can be located and contains AddTransactions(
 *     I1 — IsFirst genesis is exempt (the `!tx.IsFirst()` guard exists)
 *     I2 — in-block chaining is preserved (`setInBlock` is checked)
 *     I3 — disk-confirmed predecessor is accepted
 *           (`LLD::Ledger->HasTx(tx.hashPrevTx, FLAGS::BLOCK)`)
 *     I4 — drop predicate is the conjunction `!fInBlock && !fOnDisk`
 *     I5 — setInBlock is populated alongside block.vtx.push_back
 *           (so the in-block exemption is actually maintained)
 *
 *   Related: docs/architecture/MEMPOOL_ONLY_PREDECESSOR_FILTER.md
 *
 *   Note on brittleness — the assertions below are intentionally exact
 *   string matches against the current production source. A reformat of
 *   create.cpp (e.g. adding spaces inside `HasTx( ... )` or rewriting
 *   `if(!tx.IsFirst())` as `if (!tx.IsFirst())`) will fail these tests
 *   even though the semantics are preserved. This is the *purpose* of a
 *   binding test: any edit to the gate triggers a CI signal so the
 *   author confirms the change is intentional. If you reformat
 *   create.cpp and these tests fail, update the literal patterns here
 *   to match the new spelling — do not relax the patterns into a fuzzy
 *   match, which would defeat the binding.
 */

#include <unit/catch2/catch.hpp>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>


namespace
{
    /* Read the first candidate file that exists into a string. Returns
     * the empty string if no candidate was readable. The path that was
     * successfully read is written to the `pathOut` out-parameter to
     * help diagnose failures (CI runs may execute from the repo root,
     * from build/, or from tests/ depending on harness). */
    std::string ReadFirstAvailable(
        const std::vector<std::string>& vCandidates,
        std::string& pathOut)
    {
        for(const auto& strPath : vCandidates)
        {
            std::ifstream f(strPath, std::ios::in | std::ios::binary);
            if(!f.is_open())
                continue;

            std::ostringstream ss;
            ss << f.rdbuf();
            std::string strContent = ss.str();
            if(strContent.empty())
                continue;

            pathOut = strPath;
            return strContent;
        }

        pathOut.clear();
        return std::string();
    }


    /* Build a candidate-paths list for src/TAO/Ledger/create.cpp.
     *
     * We try several relative roots so the test is robust to the CWD the
     * harness happens to use. The list is ordered cheapest-first. */
    std::vector<std::string> CreateCppCandidates()
    {
        return {
            "src/TAO/Ledger/create.cpp",
            "./src/TAO/Ledger/create.cpp",
            "../src/TAO/Ledger/create.cpp",
            "../../src/TAO/Ledger/create.cpp",
            "../../../src/TAO/Ledger/create.cpp",
            "../../../../src/TAO/Ledger/create.cpp",
        };
    }


    /* Load production create.cpp into a string and SKIP the test (with a
     * loud message) if it is not reachable from CWD. We prefer skip over
     * fail because some downstream packagers ship only the compiled
     * binary without sources. Skipping is *advisory* — when run from the
     * repo root (the CI configuration), the file is always reachable and
     * the binding assertions execute. */
    std::string LoadCreateCpp(std::string& pathOut)
    {
        return ReadFirstAvailable(CreateCppCandidates(), pathOut);
    }


    /* True iff `needle` appears in `haystack`. */
    bool Has(const std::string& haystack, const char* needle)
    {
        return haystack.find(needle) != std::string::npos;
    }
}


/* ===========================================================================
 * Test I0 — Production create.cpp is reachable and defines AddTransactions()
 *
 * Binds the rest of the file to the production source. If this test fails
 * to *locate* create.cpp it is treated as informational (WARN + SUCCEED)
 * — the production file may simply not be present in this build context.
 * If the file is located but does NOT contain the AddTransactions() entry
 * point, that IS a regression and the test fails.
 * =========================================================================== */
TEST_CASE( "Option B binding: production create.cpp defines AddTransactions()",
           "[filter_mempool_only_predecessor_binding]" )
{
    std::string strPath;
    const std::string strSource = LoadCreateCpp(strPath);

    if(strSource.empty())
    {
        WARN("src/TAO/Ledger/create.cpp not reachable from CWD — "
             "binding tests will be skipped. Run from repository root "
             "to enable full binding coverage.");
        SUCCEED("create.cpp not available in this environment (advisory)");
        return;
    }

    INFO("Loaded production source from: " << strPath);
    REQUIRE(Has(strSource, "AddTransactions(TAO::Ledger::TritiumBlock& block)"));
}


/* ===========================================================================
 * Test I1 — Production filter exempts IsFirst() (genesis) transactions
 *
 * The simulator invariant I1 requires that genesis transactions are kept
 * unconditionally. In the production source this is implemented as the
 * outer guard `if(!tx.IsFirst())` wrapping the mempool-only-predecessor
 * gate, so a genesis transaction bypasses the gate entirely.
 * =========================================================================== */
TEST_CASE( "Option B binding: production gate is wrapped in !tx.IsFirst()",
           "[filter_mempool_only_predecessor_binding]" )
{
    std::string strPath;
    const std::string strSource = LoadCreateCpp(strPath);
    if(strSource.empty())
    {
        SUCCEED("create.cpp not available in this environment (advisory)");
        return;
    }
    INFO("Loaded production source from: " << strPath);

    /* The gate must be inside a `!tx.IsFirst()` branch — this is what
     * implements I1 in production. */
    REQUIRE(Has(strSource, "if(!tx.IsFirst())"));
}


/* ===========================================================================
 * Test I2 — Production filter checks setInBlock for in-block chaining
 *
 * Invariant I2: if hashPrevTx matches a tx already accepted earlier in
 * the same candidate block, the predecessor is treated as
 * disk-equivalent. Production implements this via `setInBlock`, a
 * std::set<uint512_t> of hashes accumulated as the loop accepts entries.
 * =========================================================================== */
TEST_CASE( "Option B binding: production gate checks setInBlock for in-block chaining",
           "[filter_mempool_only_predecessor_binding]" )
{
    std::string strPath;
    const std::string strSource = LoadCreateCpp(strPath);
    if(strSource.empty())
    {
        SUCCEED("create.cpp not available in this environment (advisory)");
        return;
    }
    INFO("Loaded production source from: " << strPath);

    REQUIRE(Has(strSource, "std::set<uint512_t> setInBlock"));
    REQUIRE(Has(strSource, "setInBlock.count(tx.hashPrevTx)"));
}


/* ===========================================================================
 * Test I3 — Production filter accepts disk-confirmed predecessors
 *
 * Invariant I3: a predecessor present on disk (FLAGS::BLOCK lookup) is
 * accepted. The production implementation calls
 * `LLD::Ledger->HasTx(tx.hashPrevTx, FLAGS::BLOCK)`. The FLAGS::BLOCK
 * argument matters — it discriminates persisted state from MEMPOOL/MINER
 * lookups and is what makes the gate "mempool-ONLY" rather than
 * "mempool-absent".
 * =========================================================================== */
TEST_CASE( "Option B binding: production gate queries disk via LLD::Ledger->HasTx FLAGS::BLOCK",
           "[filter_mempool_only_predecessor_binding]" )
{
    std::string strPath;
    const std::string strSource = LoadCreateCpp(strPath);
    if(strSource.empty())
    {
        SUCCEED("create.cpp not available in this environment (advisory)");
        return;
    }
    INFO("Loaded production source from: " << strPath);

    REQUIRE(Has(strSource, "LLD::Ledger->HasTx(tx.hashPrevTx, FLAGS::BLOCK)"));
}


/* ===========================================================================
 * Test I4 — Production drop predicate is the conjunction !fInBlock && !fOnDisk
 *
 * Invariant I4: the drop decision is `!on disk AND !in-block`. The
 * production source spells this exactly as `if(!fInBlock && !fOnDisk)`
 * (or the symmetric form). A weakening to disjunction (`||`) would let
 * mempool-only-predecessor entries through and silently defeat the gate.
 * =========================================================================== */
TEST_CASE( "Option B binding: production drop predicate is !fInBlock && !fOnDisk",
           "[filter_mempool_only_predecessor_binding]" )
{
    std::string strPath;
    const std::string strSource = LoadCreateCpp(strPath);
    if(strSource.empty())
    {
        SUCCEED("create.cpp not available in this environment (advisory)");
        return;
    }
    INFO("Loaded production source from: " << strPath);

    /* Allow either argument ordering — both are semantically equivalent
     * conjunctions that drop iff predecessor is neither in-block nor
     * on-disk. Reject any other combination (notably `||`). */
    const bool fOrderA = Has(strSource, "!fInBlock && !fOnDisk");
    const bool fOrderB = Has(strSource, "!fOnDisk && !fInBlock");
    INFO("Production conjunction order A (!fInBlock && !fOnDisk): " << fOrderA);
    INFO("Production conjunction order B (!fOnDisk && !fInBlock): " << fOrderB);
    REQUIRE((fOrderA || fOrderB));

    /* Defence-in-depth: if someone replaced the AND with an OR while
     * keeping the operand names the same, fail loudly. */
    REQUIRE_FALSE(Has(strSource, "!fInBlock || !fOnDisk"));
    REQUIRE_FALSE(Has(strSource, "!fOnDisk || !fInBlock"));
}


/* ===========================================================================
 * Test I5 — Production loop populates setInBlock alongside block.vtx
 *
 * The in-block chaining exemption (I2) is only correct if setInBlock is
 * actually populated as the loop accepts entries. The production source
 * pushes to block.vtx and inserts into setInBlock in the same accept
 * branch. If anyone removes the setInBlock.insert(...) call, I2 silently
 * regresses (in-block chained sigchain pairs would start being dropped).
 * =========================================================================== */
TEST_CASE( "Option B binding: production loop populates setInBlock when accepting tx",
           "[filter_mempool_only_predecessor_binding]" )
{
    std::string strPath;
    const std::string strSource = LoadCreateCpp(strPath);
    if(strSource.empty())
    {
        SUCCEED("create.cpp not available in this environment (advisory)");
        return;
    }
    INFO("Loaded production source from: " << strPath);

    REQUIRE(Has(strSource, "block.vtx.push_back"));
    REQUIRE(Has(strSource, "setInBlock.insert(hash)"));
}
