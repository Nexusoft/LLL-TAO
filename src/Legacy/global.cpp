/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/include/ambassador.h>
#include <Legacy/include/global.h>
#include <Legacy/wallet/wallet.h>

#include <TAO/Ledger/include/chainstate.h>

namespace Legacy
{
    #ifndef NO_WALLET
    Legacy::RPC* Commands;
    #endif

    /*  Instantiate global instances of the API. */
    bool Initialize()
    {
        /* Client mode doesn't need to initialize the wallet. */
        if(config::fClient.load())
            return true;

        #ifndef NO_WALLET

        debug::log(0, FUNCTION, "Initializing Legacy");

        /* Load the Wallet Database. NOTE this needs to be done before ChainState::Initialize as that can disconnect blocks causing
           the wallet to be accessed if they contain any legacy stake transactions */
        bool fFirstRun;
        if (!Legacy::Wallet::InitializeWallet(config::GetArg(std::string("-wallet"), Legacy::WalletDB::DEFAULT_WALLET_DB)))
            return debug::error("Failed initializing wallet");

        /* Initialize the scripts for legacy mode. */
        Legacy::InitializeScripts();

        /* Check the wallet loading for errors. */
        uint32_t nLoadWalletRet = Legacy::Wallet::GetInstance().LoadWallet(fFirstRun);
        if (nLoadWalletRet != Legacy::DB_LOAD_OK)
        {
            if (nLoadWalletRet == Legacy::DB_CORRUPT)
                return debug::error("Failed loading wallet.dat: Wallet corrupted");
            else if (nLoadWalletRet == Legacy::DB_TOO_NEW)
                return debug::error("Failed loading wallet.dat: Wallet requires newer version of Nexus");
            else if (nLoadWalletRet == Legacy::DB_NEEDS_RESCAN)
            {
                debug::log(0, FUNCTION, "Wallet.dat was cleaned or repaired, rescanning now");

                Legacy::Wallet::GetInstance().ScanForWalletTransactions(TAO::Ledger::ChainState::stateGenesis, true);
            }
            else
                return debug::error("Failed loading wallet.dat");
        }

        /* Handle Rescanning. */
        if(config::GetBoolArg(std::string("-rescan")))
            Legacy::Wallet::GetInstance().ScanForWalletTransactions(TAO::Ledger::ChainState::stateGenesis, true);

        /* Relay transactions. */
        Legacy::Wallet::GetInstance().ResendWalletTransactions();

        /* Create RPC server. */
        Commands = new Legacy::RPC();

        #else

        /* Initialize the scripts for legacy mode. */
        Legacy::InitializeScripts();

        #endif

        return true;
    }


    /*  Delete global instances of the API. */
    void Shutdown()
    {
        /* Client mode doesn't need to initialize the wallet. */
        if(config::fClient.load())
            return;

        /* Shutdown our RPC server if enabled. */
        #ifndef NO_WALLET

        debug::log(0, FUNCTION, "Shutting down Legacy");

        /* Shut down wallet database environment. */
        if(config::GetBoolArg(std::string("-flushwallet"), true))
            Legacy::WalletDB::ShutdownFlushThread();

        /* Shutdown wallet environment. */
        Legacy::BerkeleyDB::GetInstance().Shutdown();

        /* Delete RPC server. */
        if(Commands)
            delete Commands;

        #endif
    }
}
