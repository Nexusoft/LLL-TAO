/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/signals.h>
#include <Util/include/convert.h>
#include <Util/include/runtime.h>
#include <Util/include/filesystem.h>

#include <TAO/API/include/cmd.h>
#include <LLP/types/corenode.h>
#include <LLP/types/rpcnode.h>
#include <TAO/API/include/rpc.h>

#include <LLP/include/global.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/chainstate.h>

#include <TAO/API/include/supply.h>
#include <TAO/API/include/accounts.h>

#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>

#include <TAO/Operation/include/execute.h>

#include <LLP/types/miner.h>

#include <iostream>
#include <sstream>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/constants.h>



/* Declare the Global LLD Instances. */
namespace LLD
{
    RegisterDB* regDB;
    LedgerDB*   legDB;
    LocalDB*    locDB;
}


/* Declare the Global LLP Instances. */
namespace LLP
{
    Server<TritiumNode>* TRITIUM_SERVER;
    Server<LegacyNode> * LEGACY_SERVER;
}

void Thread()
{

}

int main(int argc, char** argv)
{
    LLP::Server<LLP::CoreNode>* CORE_SERVER = nullptr;
    LLP::Server<LLP::RPCNode>* RPC_SERVER = nullptr;

    /* Setup the timer timer. */
    runtime::timer timer;
    timer.Start();

    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Parse out the parameters */
    config::ParseParameters(argc, argv);


    /* Read the configuration file. */
    config::ReadConfigFile(config::mapArgs, config::mapMultiArgs);


    /* Handle Commandline switch */
    for (int i = 1; i < argc; ++i)
    {
        if (!IsSwitchChar(argv[i][0]))
        {
            if(config::GetBoolArg("-api"))
                return TAO::API::CommandLineAPI(argc, argv, i);

            return TAO::API::CommandLineRPC(argc, argv, i);
        }
    }


    /* Create directories if they don't exist yet. */
    if(!filesystem::exists(config::GetDataDir(false)) &&
        filesystem::create_directory(config::GetDataDir(false)))
    {
        debug::log(0, FUNCTION, "Generated Path ", config::GetDataDir(false));
    }



    /* Create the database instances. */
    LLD::regDB = new LLD::RegisterDB(LLD::FLAGS::CREATE | LLD::FLAGS::APPEND | LLD::FLAGS::FORCE);
    LLD::locDB = new LLD::LocalDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);
    LLD::legDB = new LLD::LedgerDB(LLD::FLAGS::CREATE | LLD::FLAGS::WRITE);


    /** Load the Wallet Database. **/
    bool fFirstRun;
    if (!Legacy::CWallet::InitializeWallet(config::GetArg("-wallet", Legacy::CWalletDB::DEFAULT_WALLET_DB)))
        return debug::error("failed initializing wallet");

    uint32_t nLoadWalletRet = Legacy::CWallet::GetInstance().LoadWallet(fFirstRun);
    if (nLoadWalletRet != Legacy::DB_LOAD_OK)
    {
        if (nLoadWalletRet == Legacy::DB_CORRUPT)
            return debug::error("failed loading wallet.dat: Wallet corrupted");
        else if (nLoadWalletRet == Legacy::DB_TOO_NEW)
            return debug::error("failed loading wallet.dat: Wallet requires newer version of Nexus");
        else if (nLoadWalletRet == Legacy::DB_NEED_REWRITE)
            return debug::error("wallet needed to be rewritten: restart Nexus to complete");
        else
            return debug::error("failed loading wallet.dat");
    }


    /** Initialize ChainState. */
    TAO::Ledger::ChainState::Initialize();


    /* Initialize the Tritium Server. */
    LLP::TRITIUM_SERVER = new LLP::Server<LLP::TritiumNode>(
        config::GetArg("-port", config::fTestNet ? 8888 : 9888),
        10,
        30,
        false,
        0,
        0,
        60,
        config::GetBoolArg("-listen", true),
        config::GetBoolArg("-meters", false),
        true);


    //-addnode means add to address manager for this specific Server
    //-connect means follow the logic below this and try to establish a connection
    /* Add node to Tritium server */
    if(config::mapMultiArgs["-addnode"].size() > 0)
    {
        for(auto node : config::mapMultiArgs["-addnode"])
        {
            LLP::TRITIUM_SERVER->AddConnection(
                node,
                config::GetArg("-port", config::fTestNet ? 8888 : 9888));
        }
    }


    //try to addnode 127.0.0.1 or some unreachable address
    //get both of these to try and race on connection so they break getaddrinfo
    /* Initialize the Legacy Server. */
    LLP::LEGACY_SERVER = new LLP::Server<LLP::LegacyNode>(
        config::GetArg("-port", config::fTestNet ? 8323 : 9323),
        10,
        30,
        false,
        0,
        0,
        60,
        config::GetBoolArg("-listen", true),
        config::GetBoolArg("-meters", false),
        true);

    if(config::mapMultiArgs["-addnode"].size() > 0)
    {
        for(auto node : config::mapMultiArgs["-addnode"])
        {
            LLP::LEGACY_SERVER->AddConnection(
                node,
                config::GetArg("-port", config::fTestNet ? 8323 : 9323));
        }
    }

    /* Create the Core API Server. */
    CORE_SERVER = new LLP::Server<LLP::CoreNode>(
        config::GetArg("-apiport", 8080),
        10,
        30,
        false,
        0,
        0,
        60,
        config::GetBoolArg("-listen", true),
        false,
        false);



    /* Set up RPC server */
    TAO::API::RPCCommands = new TAO::API::RPC();
    RPC_SERVER = new LLP::Server<LLP::RPCNode>(
        config::GetArg("-rpcport", config::fTestNet? 8336 : 9336),
        1,
        30,
        false,
        0,
        0,
        60,
        config::GetBoolArg("-listen", true),
        false,
        false);



    /* Elapsed Milliseconds from timer. */
    uint32_t nElapsed = timer.ElapsedMilliseconds();
    timer.Stop();


    /* Sleep before output. */
    runtime::sleep(100);


    /* Startup performance metric. */
    debug::log(0, FUNCTION, "Started up in ", nElapsed, "ms");


    /* Get the account. */
    TAO::Ledger::SignatureChain* user = new TAO::Ledger::SignatureChain("colin", "pass");

    for(uint32_t n = 0; n < config::GetArg("-test", 0); n++)
    {
        /* Create the transaction. */
        TAO::Ledger::Transaction tx;
        if(!TAO::Ledger::CreateTransaction(user, "1234", tx))
            debug::error(0, FUNCTION, "failed to create");

        /* Submit the transaction payload. */
        uint256_t hashRegister = LLC::GetRand256();

        /* Test the payload feature. */
        DataStream ssData(SER_REGISTER, 1);
        ssData << std::string("this is test data");

        /* Submit the payload object. */
        tx << (uint8_t)TAO::Operation::OP::REGISTER << hashRegister << (uint8_t)TAO::Register::OBJECT::APPEND << static_cast<std::vector<uint8_t>>(ssData);

        /* Execute the operations layer. */
        if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
            debug::error(0, FUNCTION, "Operations failed to execute");

        /* Sign the transaction. */
        if(!tx.Sign(user->Generate(tx.nSequence, "1234")))
            debug::error(0, FUNCTION, "Failed to sign");

        tx.print();

        /* Execute the operations layer. */
        if(!TAO::Ledger::mempool.Accept(tx))
            debug::error(0, FUNCTION, "Failed to accept");

        LLD::locDB->WriteLast(tx.hashGenesis, tx.GetHash());
    }

    /* Initialize generator thread. */
    std::thread thread;
    if(config::GetBoolArg("-private"))
        thread = std::thread(TAO::Ledger::ThreadGenerator);

    /* Wait for shutdown. */
    std::mutex SHUTDOWN_MUTEX;
    std::unique_lock<std::mutex> SHUTDOWN_LOCK(SHUTDOWN_MUTEX);
    SHUTDOWN.wait(SHUTDOWN_LOCK, []{ return config::fShutdown; });


    /* Shutdown metrics. */
    timer.Reset();


    /* Cleanup the ledger database. */
    if(LLD::legDB)
    {
        debug::log(0, FUNCTION, "Shutting down ledgerDB");

        delete LLD::legDB;
    }


    /* Cleanup the register database. */
    if(LLD::regDB)
    {
        debug::log(0, FUNCTION, "Shutting down registerDB");

        delete LLD::regDB;
    }


    /* Cleanup the local database. */
    if(LLD::locDB)
    {
        debug::log(0, FUNCTION, "Shutting down localDB");

        delete LLD::locDB;
    }


    /* Shutdown the tritium server and its subsystems */
    if(LLP::TRITIUM_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down Tritium Server");

        LLP::TRITIUM_SERVER->Shutdown();
        delete LLP::TRITIUM_SERVER;
    }


    /* Shutdown the legacy server and its subsystems */
    if(LLP::LEGACY_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down Legacy Server");

        LLP::LEGACY_SERVER->Shutdown();
        delete LLP::LEGACY_SERVER;
    }


    //checkout these guys for memory leaks
    /* Shutdown the core API server and its subsystems */
    if(CORE_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down API Server");

        CORE_SERVER->Shutdown();
        delete CORE_SERVER;
    }


    /* Shutdown the RPC server and its subsystems */
    if(RPC_SERVER)
    {
        debug::log(0, FUNCTION, "Shutting down RPC Server");

        RPC_SERVER->Shutdown();
        delete RPC_SERVER;
    }



    /* Elapsed Milliseconds from timer. */
    nElapsed = timer.ElapsedMilliseconds();


    /* Startup performance metric. */
    debug::log(0, FUNCTION, "Closed in ", nElapsed, "ms");


    return 0;
}
