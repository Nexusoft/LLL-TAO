/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#define CATCH_CONFIG_NO_POSIX_SIGNALS
/* Provide our own main() so suite-wide globals are initialized even when a
 * tag filter skips the [args] TEST_CASE (e.g. ./nexus "[ledger]").  Without
 * this, any ledger/mempool test that touches LLD/ChainState globals segfaults
 * because setup used to live only inside TEST_CASE("Arguments Tests","[args]"). */
#define CATCH_CONFIG_RUNNER
#include <unit/catch2/catch.hpp>

#include <Legacy/wallet/wallet.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>

#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/include/chainstate.h>

#include <LLP/include/global.h>

#include <Util/include/filesystem.h>
#include <Util/include/args.h>

#include <iostream>
#include <mutex>


namespace
{

/** EnsureUnitTestEnvironment
 *
 *  One-shot suite bootstrap shared by main() and the [args] assertions.
 *  Safe to call repeatedly; subsequent calls are no-ops once ready.
 *
 **/
    bool EnsureUnitTestEnvironment()
    {
        static std::mutex MUTEX;
        static bool fReady = false;

        std::lock_guard<std::mutex> lock(MUTEX);
        if(fReady)
            return true;

        config::fTestNet = true;
        config::mapArgs["-private"] = "1";
        config::mapArgs["-testnet"] = "92349234";
        config::mapArgs["-flushwallet"] = "false";
        config::mapArgs["-apiauth"]     = "0";
        config::mapArgs["-generate"]    = "password";

        /* To simplify the API testing we will always use multiuser mode */
        config::fMultiuser = true;
        config::mapArgs["-verbose"] = "3";
        config::fHybrid    = true;

        //get the data directory
        std::string strPath = config::GetDataDir();

        //test the filesystem remove and also clear from previous unit tests
        if(filesystem::exists(strPath))
        {
            if(!filesystem::remove_directories(strPath))
                return false;
            if(filesystem::exists(strPath))
                return false;
        }

        //create LLD instances
        // NOTE: Logical + Sessions are required once TAO::API::Initialize() starts
        // Indexing::RefreshEvents / session services; omitting them null-derefs
        // LLD::Logical->ReadLastIndex() on the refresh thread (SIGSEGV).
        LLD::Contract = new LLD::ContractDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
        LLD::Register = new LLD::RegisterDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
        LLD::Local    = new LLD::LocalDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
        LLD::Ledger   = new LLD::LedgerDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
        LLD::Trust    = new LLD::TrustDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
        LLD::Legacy   = new LLD::LegacyDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
        LLD::Logical  = new LLD::LogicalDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
        LLD::Sessions = new LLD::SessionDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);


        //load wallet
        bool fFirstRun = false;
        if(!Legacy::Wallet::Initialize(Legacy::WalletDB::DEFAULT_WALLET_DB))
            return false;
        if(Legacy::Wallet::LoadWallet(fFirstRun) != Legacy::DB_LOAD_OK)
            return false;


        //initialize chain state
        if(!TAO::Ledger::ChainState::Initialize())
            return false;

        //create best chain.
        TAO::Ledger::BlockState state;
        state.nHeight = 200;
        state.nBits   = 555;

        //write best to disk
        const uint1024_t hashBest = state.GetHash();
        LLD::Ledger->WriteBlock(hashBest, state);

        //set best block — keep hashBestChain in lockstep with tStateBest so
        // mining-template hashPrevBlock guards (and any tip-staleness checks)
        // see a consistent tip rather than genesis vs height-200 mismatch.
        TAO::Ledger::ChainState::tStateBest.store(state);
        TAO::Ledger::ChainState::hashBestChain.store(hashBest);
        TAO::Ledger::ChainState::nBestHeight.store(200);

        //check best
        if(TAO::Ledger::ChainState::tStateBest.load().IsNull())
            return false;
        if(TAO::Ledger::ChainState::hashBestChain.load() != hashBest)
            return false;


        /** Initialize network resources. (Need before RPC/API for WSAStartup call in Windows)
         *
         *  LLP::Initialize() already:
         *    - calls TAO::API::Initialize() once (indexing threads, command registry)
         *    - constructs API_SERVER when -apiauth=0 (set above)
         *  Do NOT call TAO::API::Initialize() or new API_SERVER again here:
         *  a second Indexing::Initialize() assigns over still-joinable
         *  std::thread members and std::terminates the process.
         **/
        if(!LLP::Initialize())
            return false;

        fReady = true;
        return true;
    }

} /* anonymous namespace */


TEST_CASE("Arguments Tests", "[args]")
{
    REQUIRE(EnsureUnitTestEnvironment());

    REQUIRE(config::fTestNet.load() == true);
    REQUIRE(config::GetArg("-testnet", 0) == 92349234);
    REQUIRE(config::fMultiuser.load() == true);
    REQUIRE(config::fHybrid.load() == true);

    REQUIRE(LLD::Ledger   != nullptr);
    REQUIRE(LLD::Register != nullptr);
    REQUIRE(LLD::Contract != nullptr);
    REQUIRE(LLD::Local    != nullptr);
    REQUIRE(LLD::Trust    != nullptr);
    REQUIRE(LLD::Legacy   != nullptr);
    REQUIRE(LLD::Logical  != nullptr);
    REQUIRE(LLD::Sessions != nullptr);

    REQUIRE_FALSE(TAO::Ledger::ChainState::tStateBest.load().IsNull());
    REQUIRE(TAO::Ledger::ChainState::nBestHeight.load() == 200);
    REQUIRE(TAO::Ledger::ChainState::hashBestChain.load()
         == TAO::Ledger::ChainState::tStateBest.load().GetHash());
}


int main(int argc, char* argv[])
{
    /* Bootstrap globals before Catch runs any TEST_CASE.  Tag filters such as
     * "[ledger]" skip the [args] case, so setup must not live only there. */
    if(!EnsureUnitTestEnvironment())
    {
        std::cerr << "FATAL: unit-test environment bootstrap failed\n";
        return 1;
    }

    const int nResult = Catch::Session().run(argc, argv);

    /* Clean shutdown so joinable LLP/API worker threads do not
     * std::terminate during static destruction after the suite ends. */
    config::fShutdown.store(true);
    try { TAO::API::Shutdown(); } catch(...) {}
    try { LLP::Shutdown(); } catch(...) {}

    return nResult;
}
