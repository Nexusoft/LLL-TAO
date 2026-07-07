/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

/* [Option E] Consensus-boundary lock tests for the coinbase/coinstake maturity
 * rules, plus [Option B] mempool-grace semantics.  These tests exist to prevent
 * reversions: the numeric constants and the version-dependent behavior of
 * MaturityCoinBase()/MaturityCoinStake() were verified byte-identical to
 * upstream Nexusoft/LLL-TAO (branch `merging`); any change here is a consensus
 * divergence and must be deliberate. */

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/args.h>

#include <unit/catch2/catch.hpp>

namespace
{
    /* RAII guard to force mainnet/testnet mode for a scope and restore after. */
    struct NetModeGuard
    {
        bool fPrev;

        explicit NetModeGuard(bool fTestNet)
        : fPrev(config::fTestNet.load())
        {
            config::fTestNet.store(fTestNet);
        }

        ~NetModeGuard()
        {
            config::fTestNet.store(fPrev);
        }
    };

    /* Build a minimal BlockState with a given version. */
    TAO::Ledger::BlockState MakeState(const uint32_t nVersion, const uint32_t nHeight = 1000000)
    {
        TAO::Ledger::BlockState state;
        state.nVersion = nVersion;
        state.nHeight  = nHeight;

        return state;
    }
}


TEST_CASE("Maturity constants match upstream consensus values", "[maturity][consensus]")
{
    /* [Option E] These values were audited byte-identical against upstream
     * Nexusoft/LLL-TAO (`merging`). Changing any of them forks consensus. */
    REQUIRE(TAO::Ledger::TESTNET_MATURITY_BLOCKS == 10);
    REQUIRE(TAO::Ledger::NEXUS_MATURITY_LEGACY == 100);
    REQUIRE(TAO::Ledger::NEXUS_MATURITY_COINBASE == 500);
    REQUIRE(TAO::Ledger::NEXUS_MATURITY_COINSTAKE == 250);
}


TEST_CASE("MaturityCoinBase honors network and version rules", "[maturity][consensus]")
{
    SECTION("testnet always uses TESTNET_MATURITY_BLOCKS")
    {
        NetModeGuard guard(true);

        REQUIRE(TAO::Ledger::MaturityCoinBase(MakeState(8)) == TAO::Ledger::TESTNET_MATURITY_BLOCKS);
        REQUIRE(TAO::Ledger::MaturityCoinStake(MakeState(8)) == TAO::Ledger::TESTNET_MATURITY_BLOCKS);
    }

    SECTION("mainnet pre-v7 blocks use the legacy interval")
    {
        NetModeGuard guard(false);

        REQUIRE(TAO::Ledger::MaturityCoinBase(MakeState(6)) == TAO::Ledger::NEXUS_MATURITY_LEGACY);
        REQUIRE(TAO::Ledger::MaturityCoinStake(MakeState(6)) == TAO::Ledger::NEXUS_MATURITY_LEGACY);
    }

    SECTION("mainnet v7+ blocks use the tritium maturity values")
    {
        NetModeGuard guard(false);

        REQUIRE(TAO::Ledger::MaturityCoinBase(MakeState(7)) == TAO::Ledger::NEXUS_MATURITY_COINBASE);
        REQUIRE(TAO::Ledger::MaturityCoinBase(MakeState(8)) == TAO::Ledger::NEXUS_MATURITY_COINBASE);
        REQUIRE(TAO::Ledger::MaturityCoinStake(MakeState(7)) == TAO::Ledger::NEXUS_MATURITY_COINSTAKE);
    }
}


TEST_CASE("Mempool maturity grace applies only a bounded, policy-level relaxation", "[maturity][mempool]")
{
    SECTION("grace window is small and non-zero")
    {
        /* The whole point of the grace is to cover a tip lagging the block
         * containing the maturity-boundary spend by 1-2 blocks. It must never
         * grow to a size that meaningfully weakens mempool policy. */
        REQUIRE(TAO::Ledger::MEMPOOL_MATURITY_GRACE >= 1);
        REQUIRE(TAO::Ledger::MEMPOOL_MATURITY_GRACE <= 5);
    }

    SECTION("mainnet mempool maturity is consensus maturity minus grace")
    {
        NetModeGuard guard(false);

        REQUIRE(TAO::Ledger::MaturityCoinBaseMempool(MakeState(8))
            == TAO::Ledger::NEXUS_MATURITY_COINBASE - TAO::Ledger::MEMPOOL_MATURITY_GRACE);

        /* [Option E] The relaxed policy threshold must remain strictly below the
         * consensus threshold — i.e. the grace only ever ADMITS more, it can
         * never cause the mempool to be stricter than consensus. */
        REQUIRE(TAO::Ledger::MaturityCoinBaseMempool(MakeState(8))
            < TAO::Ledger::MaturityCoinBase(MakeState(8)));
    }

    SECTION("testnet mempool maturity also gets the grace")
    {
        NetModeGuard guard(true);

        REQUIRE(TAO::Ledger::MaturityCoinBaseMempool(MakeState(8))
            == TAO::Ledger::TESTNET_MATURITY_BLOCKS - TAO::Ledger::MEMPOOL_MATURITY_GRACE);
    }

    SECTION("grace is floored so a 0-confirmation coinbase spend is never admitted")
    {
        /* Guard the floor logic directly: for any consensus maturity at or below
         * grace+1 the mempool requirement clamps to 1. Constructing that
         * scenario live requires TESTNET_MATURITY_BLOCKS <= grace+1, which is
         * not the case today, so lock the arithmetic invariant instead: the
         * returned requirement is always >= 1. */
        NetModeGuard guardTest(true);
        REQUIRE(TAO::Ledger::MaturityCoinBaseMempool(MakeState(8)) >= 1);

        NetModeGuard guardMain(false);
        REQUIRE(TAO::Ledger::MaturityCoinBaseMempool(MakeState(8)) >= 1);
    }
}
