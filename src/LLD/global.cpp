/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/include/enum.h> //for internal flags

namespace LLD
{
    /* The LLD global instance pointers. */
    ContractDB*   Contract;
    RegisterDB*   Register;
    LedgerDB*     Ledger;
    LocalDB*      Local;
    ClientDB*     Client;

    /* For Legacy LLD objects. */
    TrustDB*      Trust;
    LegacyDB*     Legacy;

    /* For global ACID transaction. */
    std::recursive_mutex ACID_MUTEX;


    /*  Initialize the global LLD instances. */
    void Initialize()
    {
        debug::log(0, FUNCTION, "Initializing LLD");

        /* _CONTRACT database instance. */
        {
            /* Create the ContractDB configuration object. */
            Config::Hashmap Config =
                Config::Hashmap("_CONTRACT", FLAGS::CREATE | FLAGS::FORCE);

            /* Set the ContractDB database internal settings. */
            Config.HASHMAP_TOTAL_BUCKETS    = 77773;
            Config.MAX_HASHMAP_FILES        = 256;
            Config.MAX_SECTOR_FILE_STREAMS  = 16;
            Config.MAX_HASHMAP_FILE_STREAMS = 64;
            Config.MAX_SECTOR_BUFFER_SIZE   = 0; //0 bytes, since we are in force mode this won't be used at all
            Config.MAX_SECTOR_CACHE_SIZE    = 1024 * 1024; //1 MB of cache available

            /* Create the ContractDB database instance. */
            Contract = new ContractDB(Config);
        }



        /* _REGISTER database instance. */
        {
            /* Create the RegisterDB configuration object. */
            Config::Hashmap Config =
                Config::Hashmap("_REGISTER", FLAGS::CREATE | FLAGS::FORCE);

            /* Set the RegisterDB database internal settings. */
            Config.HASHMAP_TOTAL_BUCKETS    = 77773;
            Config.MAX_HASHMAP_FILES        = 256;
            Config.MAX_SECTOR_FILE_STREAMS  = 16;
            Config.MAX_HASHMAP_FILE_STREAMS = 64;
            Config.MIN_LINEAR_PROBES        = 1;
            Config.MAX_SECTOR_BUFFER_SIZE   = 0; //0 bytes, since we are in force mode this won't be used at all
            Config.MAX_SECTOR_CACHE_SIZE    = config::GetArg("-registercache", 2) * 1024 * 1024; //2 MB of cache by default

            /* Create the RegisterDB database instance. */
            Register = new RegisterDB(Config);
        }


        /* _LEDGER database instance. */
        {
            /* Create the LedgerDB configuration object. */
            Config::Hashmap Config =
                Config::Hashmap("_LEDGER", FLAGS::CREATE | FLAGS::FORCE);

            /* Set the LedgerDB database internal settings. */
            Config.HASHMAP_TOTAL_BUCKETS    = config::fClient.load() ? 77773 : (256 * 256 * 64);
            Config.MAX_HASHMAP_FILES        = 512;
            Config.MAX_SECTOR_FILE_STREAMS  = 16;
            Config.MAX_HASHMAP_FILE_STREAMS = 128;
            Config.MIN_LINEAR_PROBES        = 1;
            Config.MAX_SECTOR_BUFFER_SIZE   = 0; //0 bytes, since we are in force mode this won't be used at all
            Config.MAX_SECTOR_CACHE_SIZE    = config::GetArg("-ledgercache", 2) * 1024 * 1024; //2 MB of cache by default

            /* Create the LedgerDB database instance. */
            Ledger = new LedgerDB(Config);
        }


        /* _LEGACY database instance. */
        {
            /* Create the LegacyDB configuration object. */
            Config::Hashmap Config =
                Config::Hashmap("_LEGACY", FLAGS::CREATE | FLAGS::FORCE);

            /* Set the LegacyDB database internal settings. */
            Config.HASHMAP_TOTAL_BUCKETS    = config::fClient.load() ? 77773 : (256 * 256 * 64);
            Config.MAX_HASHMAP_FILES        = 128;
            Config.MAX_SECTOR_FILE_STREAMS  = 16;
            Config.MAX_HASHMAP_FILE_STREAMS = 128;
            Config.MIN_LINEAR_PROBES        = 1;
            Config.MAX_SECTOR_BUFFER_SIZE   = 0; //0 bytes, since we are in force mode this won't be used at all
            Config.MAX_SECTOR_CACHE_SIZE    = config::GetArg("-legacycache", 1) * 1024 * 1024; //1 MB of cache by default

            /* Create the LegacyDB database instance. */
            Legacy = new LegacyDB(Config);
        }


        /* _TRUST database instance. */
        {
            /* Create the TrustDB configuration object. */
            Config::Hashmap Config =
                Config::Hashmap("_TRUST", FLAGS::CREATE | FLAGS::FORCE);

            /* Set the TrustDB database internal settings. */
            Config.HASHMAP_TOTAL_BUCKETS    = 77773;
            Config.MAX_HASHMAP_FILES        = 4; //TODO: make sure this doesn't break anything :D
            Config.MAX_SECTOR_FILE_STREAMS  = 4;
            Config.MAX_HASHMAP_FILE_STREAMS = 4;
            Config.MIN_LINEAR_PROBES        = 2;
            Config.MAX_SECTOR_BUFFER_SIZE   = 0; //0 bytes, since we are in force mode this won't be used at all
            Config.MAX_SECTOR_CACHE_SIZE    = 1024 * 1024; //1 MB of cache by default

            /* Create the TrustDB database instance. */
            Trust = new TrustDB(Config);
        }


        /* _LOCAL database instance. */
        {
            /* Create the LocalDB configuration object. */
            Config::Hashmap Config =
                Config::Hashmap("_LOCAL", FLAGS::CREATE | FLAGS::FORCE);

            /* Set the LocalDB database internal settings. */
            Config.HASHMAP_TOTAL_BUCKETS    = 77773;
            Config.MAX_HASHMAP_FILES        = 4; //TODO: make sure this doesn't break anything :D
            Config.MAX_SECTOR_FILE_STREAMS  = 4;
            Config.MAX_HASHMAP_FILE_STREAMS = 4;
            Config.MIN_LINEAR_PROBES        = 2;
            Config.MAX_SECTOR_BUFFER_SIZE   = 0; //0 bytes, since we are in force mode this won't be used at all
            Config.MAX_SECTOR_CACHE_SIZE    = 1024 * 1024; //1 MB of cache by default

            /* Create the LocalDB database instance. */
            Local = new LocalDB(Config);
        }


        /* _CLIENT database instance. */
        if(config::fClient.load())
        {
            /* Create the ClientDB configuration object. */
            Config::Hashmap Config =
                Config::Hashmap("_CLIENT", FLAGS::CREATE | FLAGS::FORCE);

            /* Set the ClientDB database internal settings. */
            Config.HASHMAP_TOTAL_BUCKETS    = 77773;
            Config.MAX_HASHMAP_FILES        = 8;
            Config.MAX_SECTOR_FILE_STREAMS  = 4;
            Config.MAX_HASHMAP_FILE_STREAMS = 4;
            Config.MIN_LINEAR_PROBES        = 2;
            Config.MAX_SECTOR_BUFFER_SIZE   = 0; //0 bytes, since we are in force mode this won't be used at all
            Config.MAX_SECTOR_CACHE_SIZE    = 1024 * 1024; //1 MB of cache by default

            /* Create new client database if enabled. */
            Client    = new ClientDB(Config);
        }

        /* Handle database recovery mode. */
        TxnRecovery();
    }


    /*  Shutdown and cleanup the global LLD instances. */
    void Shutdown()
    {
        debug::log(0, FUNCTION, "Shutting down LLD");

        /* Cleanup the contract database. */
        if(Contract)
        {
            debug::log(2, FUNCTION, "Shutting down ContractDB");
            delete Contract;
        }

        /* Cleanup the ledger database. */
        if(Ledger)
        {
            debug::log(2, FUNCTION, "Shutting down LedgerDB");
            delete Ledger;
        }


        /* Cleanup the register database. */
        if(Register)
        {
            debug::log(2, FUNCTION, "Shutting down RegisterDB");
            delete Register;
        }


        /* Cleanup the local database. */
        if(Local)
        {
            debug::log(2, FUNCTION, "Shutting down LocalDB");
            delete Local;
        }


        /* Cleanup the client database. */
        if(Client)
        {
            debug::log(2, FUNCTION, "Shutting down ClientDB");
            delete Client;
        }


        /* Cleanup the legacy database. */
        if(Legacy)
        {
            debug::log(2, FUNCTION, "Shutting down LegacyDB");
            delete Legacy;
        }


        /* Cleanup the trust database. */
        if(Trust)
        {
            debug::log(2, FUNCTION, "Shutting down TrustDB");
            delete Trust;
        }
    }


    /* Check the transactions for recovery. */
    void TxnRecovery()
    {
        /* Flag to determine if there are any failures. */
        bool fRecovery = true;

        /* Check the contract DB journal. */
        if(Contract && !Contract->TxnRecovery())
            fRecovery = false;

        /* Check the register DB journal. */
        if(Register && !Register->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(Ledger && !Ledger->TxnRecovery())
            fRecovery = false;

        /* Check the local DB journal. */
        if(Local && !Local->TxnRecovery())
            fRecovery = false;

        /* Check the client DB journal. */
        if(Client && !Client->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(Trust && !Trust->TxnRecovery())
            fRecovery = false;

        /* Check the ledger DB journal. */
        if(Legacy && !Legacy->TxnRecovery())
            fRecovery = false;

        /* Commit the transactions if journals are recovered. */
        if(fRecovery)
        {
            debug::log(0, FUNCTION, "all transactions are complete, recovering...");

            /* Commit contract DB transaction. */
            if(Contract)
                Contract->TxnCommit();

            /* Commit register DB transaction. */
            if(Register)
                Register->TxnCommit();

            /* Commit ledger DB transaction. */
            if(Ledger)
                Ledger->TxnCommit();

            /* Commit the local DB transaction. */
            if(Local)
                Local->TxnCommit();

            /* Commit the client DB transaction. */
            if(Client)
                Client->TxnCommit();

            /* Commit the trust DB transaction. */
            if(Trust)
                Trust->TxnCommit();

            /* Commit the legacy DB transaction. */
            if(Legacy)
                Legacy->TxnCommit();
        }

        /* Abort all the transactions. */
        TxnAbort();
    }


    /* Global handler for all LLD instances. */
    void TxnBegin(const uint8_t nFlags)
    {
        /* Handle memory commits if in memory m ode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Start the contract DB transaction. */
            if(Contract)
                Contract->MemoryBegin(nFlags);

            /* Start the register DB transacdtion. */
            if(Register)
                Register->MemoryBegin(nFlags);

            /* Start the ledger DB transaction. */
            if(Ledger)
                Ledger->MemoryBegin(nFlags);

            return;
        }


        /* Start the contract DB transaction. */
        if(Contract)
            Contract->TxnBegin();

        /* Start the register DB transacdtion. */
        if(Register)
            Register->TxnBegin();

        /* Start the ledger DB transaction. */
        if(Ledger)
            Ledger->TxnBegin();

        /* Start the local DB transaction. */
        if(Local)
            Local->TxnBegin();

        /* Start the client DB transaction. */
        if(Client)
            Client->TxnBegin();

        /* Start the trust DB transaction. */
        if(Trust)
            Trust->TxnBegin();

        /* Start the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnBegin();
    }


    /* Global handler for all LLD instances. */
    void TxnAbort(const uint8_t nFlags)
    {
        /* Handle memory commits if in memory m ode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER)
        {
            /* Abort the contract DB transaction. */
            if(Contract)
                Contract->MemoryRelease(nFlags);

            /* Abort the register DB transacdtion. */
            if(Register)
                Register->MemoryRelease(nFlags);

            /* Abort the ledger DB transaction. */
            if(Ledger)
                Ledger->MemoryRelease(nFlags);

            return;
        }

        /* Abort the contract DB transaction. */
        if(Contract)
            Contract->TxnRelease();

        /* Abort the register DB transaction. */
        if(Register)
            Register->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(Ledger)
            Ledger->TxnRelease();

        /* Abort the local DB transaction. */
        if(Local)
            Local->TxnRelease();

        /* Abort the client DB transaction. */
        if(Client)
            Client->TxnRelease();

        /* Abort the trust DB transaction. */
        if(Trust)
            Trust->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnRelease();
    }


    /* Global handler for all LLD instances. */
    void TxnCommit(const uint8_t nFlags)
    {
        /* Handle memory commits if in memory mode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
        {
            RLOCK(ACID_MUTEX);

            /* Commit the contract DB transaction. */
            if(Contract)
                Contract->MemoryCommit();

            /* Commit the register DB transacdtion. */
            if(Register)
                Register->MemoryCommit();

            /* Commit the ledger DB transaction. */
            if(Ledger)
                Ledger->MemoryCommit();

            return;
        }

        /* Set a checkpoint for contract DB. */
        if(Contract)
            Contract->TxnCheckpoint();

        /* Set a checkpoint for register DB. */
        if(Register)
            Register->TxnCheckpoint();

        /* Set a checkpoint for ledger DB. */
        if(Ledger)
            Ledger->TxnCheckpoint();

        /* Set a checkpoint for local DB. */
        if(Local)
            Local->TxnCheckpoint();

        /* Set a checkpoint for client DB. */
        if(Client)
            Client->TxnCheckpoint();

        /* Set a checkpoint for trust DB. */
        if(Trust)
            Trust->TxnCheckpoint();

        /* Set a checkpoint for legacy DB. */
        if(Legacy)
            Legacy->TxnCheckpoint();


        /* Commit contract DB transaction. */
        if(Contract)
            Contract->TxnCommit();

        /* Commit register DB transaction. */
        if(Register)
            Register->TxnCommit();

        /* Commit legacy DB transaction. */
        if(Ledger)
            Ledger->TxnCommit();

        /* Commit the local DB transaction. */
        if(Local)
            Local->TxnCommit();

        /* Commit the client DB transaction. */
        if(Client)
            Client->TxnCommit();

        /* Commit the trust DB transaction. */
        if(Trust)
            Trust->TxnCommit();

        /* Commit the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnCommit();


        /* Abort the contract DB transaction. */
        if(Contract)
            Contract->TxnRelease();

        /* Abort the register DB transaction. */
        if(Register)
            Register->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(Ledger)
            Ledger->TxnRelease();

        /* Abort the local DB transaction. */
        if(Local)
            Local->TxnRelease();

        /* Abort the client DB transaction. */
        if(Client)
            Client->TxnRelease();

        /* Abort the trust DB transaction. */
        if(Trust)
            Trust->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(Legacy)
            Legacy->TxnRelease();
    }
}
