/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/include/enum.h> //for internal flags

namespace LLD
{
    /* The LLD global instance pointers. */
    LogicalDB*    Logical;
    ContractDB*   Contract;
    RegisterDB*   Register;
    LedgerDB*     Ledger;
    LocalDB*      Local;
    ClientDB*     Client;

    /* For Legacy LLD objects. */
    TrustDB*      Trust;
    LegacyDB*     Legacy;


    /*  Initialize the global LLD instances. */
    void Initialize()
    {
        debug::log(0, FUNCTION, "Initializing LLD");

        /* _CONTRACT database instance. */
        {
            /* Create the ContractDB configuration object. */
            Config::Base BASE =
                Config::Base("_CONTRACT", FLAGS::CREATE | FLAGS::FORCE);

            /* Create the ContractDB sector configuration object. */
            Config::Static SECTOR             = Config::Static(BASE);
            SECTOR.MAX_SECTOR_FILE_STREAMS    = 16;
            SECTOR.MAX_SECTOR_BUFFER_SIZE     = 0; //0 bytes, since we are in force mode this won't be used at all
            SECTOR.MAX_SECTOR_CACHE_SIZE      = 1024 * 1024; //1 MB of cache available

            /* Create the ContractDB keychain configuration object. */
            Config::Hashmap KEYCHAIN          = Config::Hashmap(BASE);
            KEYCHAIN.HASHMAP_TOTAL_BUCKETS    = 77773;
            KEYCHAIN.MAX_HASHMAPS             = 256;
            KEYCHAIN.MAX_HASHMAP_FILE_STREAMS = 64;

            /* Create the ContractDB database instance. */
            Contract = new ContractDB(SECTOR, KEYCHAIN);
        }



        /* _REGISTER database instance. */
        {
            /* Create the RegisterDB configuration object. */
            Config::Base BASE =
                Config::Base("_REGISTER", FLAGS::CREATE | FLAGS::FORCE);

            /* Create the RegisterDB sector configuration object. */
            Config::Static SECTOR             = Config::Static(BASE);
            SECTOR.MAX_SECTOR_FILE_STREAMS    = 16;
            SECTOR.MAX_SECTOR_BUFFER_SIZE     = 0; //0 bytes, since we are in force mode this won't be used at all
            SECTOR.MAX_SECTOR_CACHE_SIZE      = config::GetArg("-registercache", 2) * 1024 * 1024; //2 MB of cache by default

            /* Create the RegisterDB keychain configuration object. */
            Config::Hashmap KEYCHAIN          = Config::Hashmap(BASE);
            KEYCHAIN.HASHMAP_TOTAL_BUCKETS    = 77773;
            KEYCHAIN.MAX_HASHMAPS             = 256;
            KEYCHAIN.MAX_HASHMAP_FILE_STREAMS = 64;
            KEYCHAIN.MIN_LINEAR_PROBES        = 1;

            /* Create the RegisterDB database instance. */
            Register = new RegisterDB(SECTOR, KEYCHAIN);
        }


        /* _LEDGER database instance. */
        {
            /* Create the LedgerDB configuration object. */
            Config::Base BASE =
                Config::Base("_LEDGER", FLAGS::CREATE | FLAGS::FORCE);

            /* Create the LedgerDB sector configuration object. */
            Config::Static SECTOR             = Config::Static(BASE);
            SECTOR.MAX_SECTOR_FILE_STREAMS    = 16;
            SECTOR.MAX_SECTOR_BUFFER_SIZE     = 0; //0 bytes, since we are in force mode this won't be used at all
            SECTOR.MAX_SECTOR_CACHE_SIZE      = config::GetArg("-ledgercache", 2) * 1024 * 1024; //2 MB of cache by default

            /* Create the LedgerDB keychain configuration object. */
            Config::Hashmap KEYCHAIN          = Config::Hashmap(BASE);
            KEYCHAIN.HASHMAP_TOTAL_BUCKETS    = config::fClient.load() ? 77773 : (256 * 256 * 64);
            KEYCHAIN.MAX_HASHMAPS             = 512;
            KEYCHAIN.MAX_HASHMAP_FILE_STREAMS = 128;
            KEYCHAIN.MIN_LINEAR_PROBES        = 1;

            /* Create the LedgerDB database instance. */
            Ledger = new LedgerDB(SECTOR, KEYCHAIN);
        }


        /* _LEDGER database instance. */
        {
            /* Create the LedgerDB configuration object. */
            Config::Base BASE =
                Config::Base("_LOGICAL", FLAGS::CREATE | FLAGS::FORCE);

            /* Create the LedgerDB sector configuration object. */
            Config::Static SECTOR             = Config::Static(BASE);
            SECTOR.MAX_SECTOR_FILE_STREAMS    = 16;
            SECTOR.MAX_SECTOR_BUFFER_SIZE     = 0; //0 bytes, since we are in force mode this won't be used at all
            SECTOR.MAX_SECTOR_CACHE_SIZE      = config::GetArg("-logicalcache", 2) * 1024 * 1024; //2 MB of cache by default

            /* Create the LedgerDB keychain configuration object. */
            Config::Hashmap KEYCHAIN          = Config::Hashmap(BASE);
            KEYCHAIN.HASHMAP_TOTAL_BUCKETS    = config::fClient.load() ? 77773 : (256 * 256 * 64);
            KEYCHAIN.MAX_HASHMAPS             = 128;
            KEYCHAIN.MAX_HASHMAP_FILE_STREAMS = 128;
            KEYCHAIN.MIN_LINEAR_PROBES        = 1;

            /* Create the LedgerDB database instance. */
            Logical = new LogicalDB(SECTOR, KEYCHAIN);
        }


        /* _LEGACY database instance. */
        {
            /* Create the LegacyDB configuration object. */
            Config::Base BASE =
                Config::Base("_LEGACY", FLAGS::CREATE | FLAGS::FORCE);

            /* Create the LegacyDB sector configuration object. */
            Config::Static SECTOR             = Config::Static(BASE);
            SECTOR.MAX_SECTOR_FILE_STREAMS    = 16;
            SECTOR.MAX_SECTOR_BUFFER_SIZE     = 0; //0 bytes, since we are in force mode this won't be used at all
            SECTOR.MAX_SECTOR_CACHE_SIZE      = config::GetArg("-legacycache", 1) * 1024 * 1024; //1 MB of cache by default

            /* Create the LegacyDB keychain configuration object. */
            Config::Hashmap KEYCHAIN          = Config::Hashmap(BASE);
            KEYCHAIN.HASHMAP_TOTAL_BUCKETS    = config::fClient.load() ? 77773 : (256 * 256 * 64);
            KEYCHAIN.MAX_HASHMAPS             = 128;
            KEYCHAIN.MAX_HASHMAP_FILE_STREAMS = 128;
            KEYCHAIN.MIN_LINEAR_PROBES        = 1;

            /* Create the LegacyDB database instance. */
            Legacy = new LegacyDB(SECTOR, KEYCHAIN);
        }


        /* _TRUST database instance. */
        {
            /* Create the TrustDB configuration object. */
            Config::Base BASE =
                Config::Base("_TRUST", FLAGS::CREATE | FLAGS::FORCE);

            /* Create the TrustDB sector configuration object. */
            Config::Static SECTOR             = Config::Static(BASE);
            SECTOR.MAX_SECTOR_FILE_STREAMS    = 4;
            SECTOR.MAX_SECTOR_BUFFER_SIZE     = 0; //0 bytes, since we are in force mode this won't be used at all
            SECTOR.MAX_SECTOR_CACHE_SIZE      = 1024 * 1024; //1 MB of cache by default

            /* Create the TrustDB keychain configuration object. */
            Config::Hashmap KEYCHAIN          = Config::Hashmap(BASE);
            KEYCHAIN.HASHMAP_TOTAL_BUCKETS    = 77773;
            KEYCHAIN.MAX_HASHMAPS             = 4; //TODO: make sure this doesn't break anything :D
            KEYCHAIN.MAX_HASHMAP_FILE_STREAMS = 4;
            KEYCHAIN.MIN_LINEAR_PROBES        = 1;

            /* Create the TrustDB database instance. */
            Trust = new TrustDB(SECTOR, KEYCHAIN);
        }


        /* _LOCAL database instance. */
        {
            /* Create the LocalDB configuration object. */
            Config::Base BASE =
                Config::Base("_LOCAL", FLAGS::CREATE | FLAGS::FORCE);

            /* Create the LocalDB sector configuration object. */
            Config::Static SECTOR             = Config::Static(BASE);
            SECTOR.MAX_SECTOR_FILE_STREAMS    = 4;
            SECTOR.MAX_SECTOR_BUFFER_SIZE     = 0; //0 bytes, since we are in force mode this won't be used at all
            SECTOR.MAX_SECTOR_CACHE_SIZE      = 1024 * 1024; //1 MB of cache by default

            /* Create the LocalDB keychain configuration object. */
            Config::Hashmap KEYCHAIN          = Config::Hashmap(BASE);
            KEYCHAIN.HASHMAP_TOTAL_BUCKETS    = 77773;
            KEYCHAIN.MAX_HASHMAPS             = 4; //TODO: make sure this doesn't break anything :D
            KEYCHAIN.MAX_HASHMAP_FILE_STREAMS = 4;
            KEYCHAIN.MIN_LINEAR_PROBES        = 1;

            /* Create the LocalDB database instance. */
            Local = new LocalDB(SECTOR, KEYCHAIN);
        }


        /* _CLIENT database instance. */
        if(config::fClient.load())
        {
            /* Create the ClientDB configuration object. */
            Config::Base BASE =
                Config::Base("_CLIENT", FLAGS::CREATE | FLAGS::FORCE);

            /* Create the ClientDB sector configuration object. */
            Config::Static SECTOR             = Config::Static(BASE);
            SECTOR.MAX_SECTOR_FILE_STREAMS    = 4;
            SECTOR.MAX_SECTOR_BUFFER_SIZE     = 0; //0 bytes, since we are in force mode this won't be used at all
            SECTOR.MAX_SECTOR_CACHE_SIZE      = 1024 * 1024; //1 MB of cache by default

            /* Set the ClientDB database internal settings. */
            Config::Hashmap KEYCHAIN          = Config::Hashmap(BASE);
            KEYCHAIN.HASHMAP_TOTAL_BUCKETS    = 77773;
            KEYCHAIN.MAX_HASHMAPS             = 256;
            KEYCHAIN.MAX_HASHMAP_FILE_STREAMS = 4;
            KEYCHAIN.MIN_LINEAR_PROBES        = 1;

            /* Create new client database if enabled. */
            Client    = new ClientDB(SECTOR, KEYCHAIN);
        }


        /* Handle database recovery mode. */
        TxnRecovery();
    }


    /* Run our indexing entries and routines. */
    void Indexing() //TODO: combine all of these into one single indexing routine (include -indexheight)
    {
        debug::log(0, FUNCTION, "Indexing LLD");


        /* Check for reindexing entries. */
        Logical->IndexRegisters();


        /* Check for reindexing entries. */
        Register->IndexAddress();


        /* Check for reindexing entries. */
        Ledger->IndexProofs();
    }


    /*  Shutdown and cleanup the global LLD instances. */
    void Shutdown()
    {
        debug::log(0, FUNCTION, "Shutting down LLD");

        /* Cleanup the contract database. */
        if(Logical)
            delete Logical;

        /* Cleanup the contract database. */
        if(Contract)
            delete Contract;

        /* Cleanup the ledger database. */
        if(Ledger)
            delete Ledger;

        /* Cleanup the register database. */
        if(Register)
            delete Register;

        /* Cleanup the local database. */
        if(Local)
            delete Local;

        /* Cleanup the client database. */
        if(Client)
            delete Client;

        /* Cleanup the legacy database. */
        if(Legacy)
            delete Legacy;

        /* Cleanup the trust database. */
        if(Trust)
            delete Trust;
    }


    /* Check the transactions for recovery. */
    void TxnRecovery()
    {
        /* Flag to determine if there are any failures. */
        bool fRecovery = true;

        /* Special handle for -client mode. */
        if(config::fClient.load())
        {
            /* Check the contract DB journal. */
            if(Contract && !Contract->TxnRecovery())
                fRecovery = false;

            /* Check the register DB journal. */
            if(Register && !Register->TxnRecovery())
                fRecovery = false;

            /* Check the ledger DB journal. */
            if(Client && !Client->TxnRecovery())
                fRecovery = false;

            /* Check the ledger DB journal. */
            if(Logical && !Logical->TxnRecovery())
                fRecovery = false;

            /* Commit the transactions if journals are recovered. */
            if(fRecovery)
            {
                debug::log(0, FUNCTION, "all transactions are complete, recovering...");

                /* Commit Contract DB transaction. */
                if(Contract)
                    Contract->TxnCommit();

                /* Commit Register DB transaction. */
                if(Register)
                    Register->TxnCommit();

                /* Commit the Client DB transaction. */
                if(Client)
                    Client->TxnCommit();

                /* Commit the Logical DB transaction. */
                if(Logical)
                    Logical->TxnCommit();
            }

            /* Abort all the transactions. */
            TxnAbort(TAO::Ledger::FLAGS::BLOCK, INSTANCES::MERKLE);
        }

        /* Regular mainnet mode recovery. */
        else
        {
            /* Check the contract DB journal. */
            if(Contract && !Contract->TxnRecovery())
                fRecovery = false;

            /* Check the register DB journal. */
            if(Register && !Register->TxnRecovery())
                fRecovery = false;

            /* Check the ledger DB journal. */
            if(Ledger && !Ledger->TxnRecovery())
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

                /* Commit the trust DB transaction. */
                if(Trust)
                    Trust->TxnCommit();

                /* Commit the legacy DB transaction. */
                if(Legacy)
                    Legacy->TxnCommit();
            }

            /* Abort all the transactions. */
            TxnAbort(TAO::Ledger::FLAGS::BLOCK, INSTANCES::CONSENSUS);
        }


    }


    /* Global handler for all LLD instances. */
    void TxnBegin(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Start the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->MemoryBegin(nFlags);

        /* Start the register DB transacdtion. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->MemoryBegin(nFlags);

        /* Start the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->MemoryBegin(nFlags);

        /* Handle memory commits if in memory m ode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER || nFlags == TAO::Ledger::FLAGS::SANITIZE)
            return;

        /* Start the Logical DB transaction. */
        if(Logical && (nInstances & INSTANCES::LOGICAL))
            Logical->TxnBegin();

        /* Start the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->TxnBegin();

        /* Start the register DB transacdtion. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->TxnBegin();

        /* Start the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->TxnBegin();

        /* Start the client DB transaction. */
        if(Client && (nInstances & INSTANCES::CLIENT))
            Client->TxnBegin();

        /* Start the trust DB transaction. */
        if(Trust && (nInstances & INSTANCES::TRUST))
            Trust->TxnBegin();

        /* Start the legacy DB transaction. */
        if(Legacy && (nInstances & INSTANCES::LEGACY))
            Legacy->TxnBegin();
    }


    /* Global handler for all LLD instances. */
    void TxnAbort(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Abort the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->MemoryRelease(nFlags);

        /* Abort the register DB transacdtion. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->MemoryRelease(nFlags);

        /* Abort the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->MemoryRelease(nFlags);

        /* Handle memory commits if in memory m ode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL || nFlags == TAO::Ledger::FLAGS::MINER || nFlags == TAO::Ledger::FLAGS::SANITIZE)
            return;

        /* Abort the Logical DB transaction. */
        if(Logical && (nInstances & INSTANCES::LOGICAL))
            Logical->TxnRelease();

        /* Abort the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->TxnRelease();

        /* Abort the register DB transaction. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->TxnRelease();

        /* Abort the client DB transaction. */
        if(Client && (nInstances & INSTANCES::CLIENT))
            Client->TxnRelease();

        /* Abort the trust DB transaction. */
        if(Trust && (nInstances & INSTANCES::TRUST))
            Trust->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(Legacy && (nInstances & INSTANCES::LEGACY))
            Legacy->TxnRelease();
    }


    /* Global handler for all LLD instances. */
    void TxnCommit(const uint8_t nFlags, const uint16_t nInstances)
    {
        /* Special check if using MINER or SANITIZE flags. */
        if(nFlags == TAO::Ledger::FLAGS::MINER || nFlags == TAO::Ledger::FLAGS::SANITIZE)
            return; //we want to abort in case this is called accidentally. We don't want to commit these states to internal memory

        /* Commit the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->MemoryCommit();

        /* Commit the register DB transacdtion. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->MemoryCommit();

        /* Commit the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->MemoryCommit();

        /* Handle memory commits if in memory mode. */
        if(nFlags == TAO::Ledger::FLAGS::MEMPOOL)
            return;

        /* Set a checkpoint for Logical DB. */
        if(Logical && (nInstances & INSTANCES::LOGICAL))
            Logical->TxnCheckpoint();

        /* Set a checkpoint for contract DB. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->TxnCheckpoint();

        /* Set a checkpoint for register DB. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->TxnCheckpoint();

        /* Set a checkpoint for ledger DB. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->TxnCheckpoint();

        /* Set a checkpoint for client DB. */
        if(Client && (nInstances & INSTANCES::CLIENT))
            Client->TxnCheckpoint();

        /* Set a checkpoint for trust DB. */
        if(Trust && (nInstances & INSTANCES::TRUST))
            Trust->TxnCheckpoint();

        /* Set a checkpoint for legacy DB. */
        if(Legacy && (nInstances & INSTANCES::LEGACY))
            Legacy->TxnCheckpoint();


        /* Commit Logical DB transaction. */
        if(Logical && (nInstances & INSTANCES::LOGICAL))
            Logical->TxnCommit();

        /* Commit contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->TxnCommit();

        /* Commit register DB transaction. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->TxnCommit();

        /* Commit legacy DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->TxnCommit();

        /* Commit the client DB transaction. */
        if(Client && (nInstances & INSTANCES::CLIENT))
            Client->TxnCommit();

        /* Commit the trust DB transaction. */
        if(Trust && (nInstances & INSTANCES::TRUST))
            Trust->TxnCommit();

        /* Commit the legacy DB transaction. */
        if(Legacy && (nInstances & INSTANCES::LEGACY))
            Legacy->TxnCommit();


        /* Abort the Logical DB transaction. */
        if(Logical && (nInstances & INSTANCES::LOGICAL))
            Logical->TxnRelease();

        /* Abort the contract DB transaction. */
        if(Contract && (nInstances & INSTANCES::CONTRACT))
            Contract->TxnRelease();

        /* Abort the register DB transaction. */
        if(Register && (nInstances & INSTANCES::REGISTER))
            Register->TxnRelease();

        /* Abort the ledger DB transaction. */
        if(Ledger && (nInstances & INSTANCES::LEDGER))
            Ledger->TxnRelease();

        /* Abort the client DB transaction. */
        if(Client && (nInstances & INSTANCES::CLIENT))
            Client->TxnRelease();

        /* Abort the trust DB transaction. */
        if(Trust && (nInstances & INSTANCES::TRUST))
            Trust->TxnRelease();

        /* Abort the legacy DB transaction. */
        if(Legacy && (nInstances & INSTANCES::LEGACY))
            Legacy->TxnRelease();
    }
}
