/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <Legacy/include/evaluate.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/mutex.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/string.h>

#include <functional>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Queue to handle dispatch requests. */
    util::atomic::lock_unique_ptr<std::queue<uint512_t>> Indexing::DISPATCH;


    /* Thread for running dispatch. */
    std::thread Indexing::EVENTS_THREAD;


    /* Condition variable to wake up the indexing thread. */
    std::condition_variable Indexing::CONDITION;


    /* Queue to handle dispatch requests. */
    util::atomic::lock_unique_ptr<std::queue<uint256_t>> Indexing::INITIALIZE;


    /* Thread for running dispatch. */
    std::thread Indexing::INITIALIZE_THREAD;


    /* Condition variable to wake up the indexing thread. */
    std::condition_variable Indexing::INITIALIZE_CONDITION;


    /* Set to track active indexing entries. */
    std::set<std::string> Indexing::REGISTERED;


    /* Mutex around registration. */
    std::mutex Indexing::REGISTERED_MUTEX;


    /* Initializes the current indexing systems. */
    void Indexing::Initialize()
    {
        /* Read our list of active login sessions. */

        /* Initialize our thread objects now. */
        Indexing::DISPATCH      = util::atomic::lock_unique_ptr<std::queue<uint512_t>>(new std::queue<uint512_t>());
        Indexing::EVENTS_THREAD = std::thread(&Indexing::Manager);


        /* Initialize our indexing thread now. */
        Indexing::INITIALIZE    = util::atomic::lock_unique_ptr<std::queue<uint256_t>>(new std::queue<uint256_t>());
        Indexing::INITIALIZE_THREAD = std::thread(&Indexing::InitializeThread);
    }


    /* Initialize a user's indexing entries. */
    void Indexing::Initialize(const uint256_t& hashSession)
    {
        /* Add our genesis to initialize ordering. */
        INITIALIZE->push(hashSession);
        INITIALIZE_CONDITION.notify_all();
    }


    /* Checks current events against transaction history to ensure we are up to date. */
    void Indexing::RefreshEvents()
    {
        /* Check to disable for -client mode. */
        if(config::fClient.load())
            return;

        /* Our list of transactions to read. */
        std::vector<TAO::Ledger::Transaction> vtx;

        /* Start a timer to track. */
        runtime::timer timer;
        timer.Start();

        /* Check if we have a last entry. */
        uint512_t hashLast;

        /* See if we need to bootstrap indexes. */
        const bool fIndexed =
            LLD::Logical->ReadLastIndex(hashLast);

        /* Handle first key if needed. */
        if(!fIndexed)
        {
            /* Grab our starting txid by quick read. */
            if(LLD::Ledger->BatchRead("tx", vtx, 1))
                hashLast = vtx[0].GetHash();

            debug::log(0, FUNCTION, "Initializing indexing tx ", hashLast.SubString());
        }

        /* Keep track of our total count. */
        uint32_t nScannedCount = 0;

        /* Start our scan. */
        debug::log(0, FUNCTION, "Scanning from tx ", hashLast.SubString());
        while(!config::fShutdown.load())
        {
            /* Read the next batch of inventory. */
            std::vector<TAO::Ledger::Transaction> vtx;
            if(!LLD::Ledger->BatchRead(hashLast, "tx", vtx, 1000, fIndexed))
                break;

            /* Loop through found transactions. */
            for(const auto& tx : vtx)
            {
                /* Check that our transactions are in the main chain. */
                if(!tx.IsConfirmed())
                    continue;

                /* Iterate the transaction contracts. */
                for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                {
                    /* Grab contract reference. */
                    const TAO::Operation::Contract& rContract = tx[nContract];

                    {
                        LOCK(REGISTERED_MUTEX);

                        /* Loop through registered commands. */
                        for(const auto& strCommands : REGISTERED)
                            Commands::Instance(strCommands)->Index(rContract, nContract);
                    }
                }

                /* Update the scanned count for meters. */
                ++nScannedCount;

                /* Meter for output. */
                if(nScannedCount % 100000 == 0)
                {
                    /* Get the time it took to rescan. */
                    uint32_t nElapsedSeconds = timer.Elapsed();
                    debug::log(0, FUNCTION, "Processed ", nScannedCount, " in ", nElapsedSeconds, " seconds (",
                        std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");
                }
            }

            /* Set hash Last. */
            hashLast = vtx.back().GetHash();

            /* Check for end. */
            if(vtx.size() != 1000)
                break;
        }

        /* Write our last index now. */
        LLD::Logical->WriteLastIndex(hashLast);

        debug::log(0, FUNCTION, "Completed scanning ", nScannedCount, " tx in ", timer.Elapsed(), " seconds");
    }


    /*  Index a new block hash to relay thread.*/
    void Indexing::PushTransaction(const uint512_t& hashTx)
    {
        DISPATCH->push(hashTx);
        CONDITION.notify_one();
    }


    /* Handle relays of all events for LLP when processing block. */
    void Indexing::Manager()
    {
        /* Refresh our events. */
        RefreshEvents();

        /* Main loop controlled by condition variable. */
        std::mutex CONDITION_MUTEX;
        while(!config::fShutdown.load())
        {
            /* Wait for entries in the queue. */
            std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
            CONDITION.wait(CONDITION_LOCK,
            []
            {
                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return true;

                /* Check for suspended state. */
                if(config::fSuspended.load())
                    return false;

                return Indexing::DISPATCH->size() != 0;
            });

            /* Check for shutdown. */
            if(config::fShutdown.load())
                return;

            /* Start a stopwatch. */
            runtime::stopwatch swTimer;
            swTimer.start();

            /* Grab the next entry in the queue. */
            const uint512_t hashTx = DISPATCH->front();
            DISPATCH->pop();

            /* Fire off indexing now. */
            IndexSigchain(hashTx);

            /* Write our last index now. */
            LLD::Logical->WriteLastIndex(hashTx);
        }
    }


    /* Index tritium transaction level events for logged in sessions. */
    void Indexing::IndexSigchain(const uint512_t& hashTx)
    {
        /* Check if handling legacy or tritium. */
        if(hashTx.GetType() == TAO::Ledger::TRITIUM)
        {
            /* Make sure the transaction is on disk. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashTx, tx))
                return;

            /* Build our local sigchain events indexes. */
            index_transaction(hashTx, tx);

            /* Iterate the transaction contracts. */
            for(uint32_t nContract = 0; nContract < tx.Size(); nContract++)
            {
                /* Grab contract reference. */
                const TAO::Operation::Contract& rContract = tx[nContract];

                {
                    LOCK(REGISTERED_MUTEX);

                    /* Loop through registered commands. */
                    for(const auto& strCommands : REGISTERED)
                        Commands::Instance(strCommands)->Index(rContract, nContract);
                }
            }
        }

        /* Check for legacy transaction type. */
        if(hashTx.GetType() == TAO::Ledger::LEGACY)
        {
            /* Make sure the transaction is on disk. */
            Legacy::Transaction tx;
            if(!LLD::Legacy->ReadTx(hashTx, tx))
                return;

            /* Loop through the available outputs. */
            for(uint32_t nContract = 0; nContract < tx.vout.size(); nContract++)
            {
                /* Grab a reference of our output. */
                const Legacy::TxOut& txout = tx.vout[nContract];

                /* Extract our register address. */
                uint256_t hashTo;
                if(Legacy::ExtractRegister(txout.scriptPubKey, hashTo))
                {
                    /* Read the owner of register. (check this for MEMPOOL, too) */
                    TAO::Register::State state;
                    if(!LLD::Register->ReadState(hashTo, state, TAO::Ledger::FLAGS::LOOKUP))
                        continue;

                    /* Check if owner is authenticated. */
                    if(Authentication::Active(state.hashOwner))
                    {
                        /* Write our events to database. */
                        if(!LLD::Logical->PushEvent(state.hashOwner, hashTx, nContract))
                            continue;

                        /* Increment our sequence. */
                        if(!LLD::Logical->IncrementLegacySequence(state.hashOwner))
                            continue;
                    }
                }
            }
        }
    }


    /* Broadcast our unconfirmed transactions if there are any. */
    void Indexing::BroadcastUnconfirmed(const uint256_t& hashGenesis)
    {
        /* Build list of transaction hashes. */
        std::vector<uint512_t> vHashes;

        /* Read all transactions from our last index. */
        uint512_t hash;
        if(!LLD::Logical->ReadLast(hashGenesis, hash))
            return;

        /* Loop until we reach confirmed transaction. */
        while(!config::fShutdown.load())
        {
            /* Read the transaction from the ledger database. */
            TAO::API::Transaction tx;
            if(!LLD::Logical->ReadTx(hash, tx))
            {
                debug::warning(FUNCTION, "check for ", hashGenesis.SubString(), " failed at ", VARIABLE(hash.SubString()));
                break;
            }

            /* Check we have index to break. */
            if(LLD::Ledger->HasIndex(hash))
                break;

            /* Push transaction to list. */
            vHashes.push_back(hash); //this will warm up the LLD cache if available, or remain low footprint if not

            /* Check for first. */
            if(tx.IsFirst())
                break;

            /* Set hash to previous hash. */
            hash = tx.hashPrevTx;
        }

        /* Reverse iterate our list of entries. */
        for(auto hash = vHashes.rbegin(); hash != vHashes.rend(); ++hash)
        {
            /* Read the transaction from the ledger database. */
            TAO::API::Transaction tx;
            if(!LLD::Logical->ReadTx(*hash, tx))
            {
                debug::warning(FUNCTION, "check for ", hashGenesis.SubString(), " failed at ", VARIABLE(hash->SubString()));
                continue;
            }

            /* Execute the operations layer. */
            if(TAO::Ledger::mempool.Has(*hash))
            {
                /* Broadcast our transaction now. */
                tx.Broadcast();
            }

            /* Otherwise accept and execute this transaction. */
            else if(!TAO::Ledger::mempool.Accept(tx))
            {
                debug::warning(FUNCTION, "accept for ", hash->SubString(), " failed");
                continue;
            }
        }
    }


    /*  Refresh our events and transactions for a given sigchain. */
    void Indexing::DownloadSigchain(const uint256_t& hashGenesis)
    {
        /* Check for client mode. */
        if(!config::fClient.load())
            return;

        /* Check for genesis. */
        if(LLP::TRITIUM_SERVER)
        {
            /* Find an active connection to sync from. */
            std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
            if(pNode != nullptr)
            {
                debug::log(0, FUNCTION, "CLIENT MODE: Synchronizing Sigchain");

                /* Get the last txid in sigchain. */
                uint512_t hashLast;
                LLD::Logical->ReadLastConfirmed(hashGenesis, hashLast);

                do
                {
                    /* Request the sig chain. */
                    debug::log(0, FUNCTION, "CLIENT MODE: Requesting LIST::SIGCHAIN for ", hashGenesis.SubString());
                    LLP::TritiumNode::BlockingMessage
                    (
                        30000,
                        pNode.get(), LLP::TritiumNode::ACTION::LIST,
                        uint8_t(LLP::TritiumNode::TYPES::SIGCHAIN), hashGenesis, hashLast
                    );
                    debug::log(0, FUNCTION, "CLIENT MODE: LIST::SIGCHAIN received for ", hashGenesis.SubString());

                    /* Check for shutdown. */
                    if(config::fShutdown.load())
                        break;

                    uint512_t hashCurrent;
                    LLD::Logical->ReadLastConfirmed(hashGenesis, hashCurrent);

                    if(hashCurrent == hashLast)
                    {
                        debug::log(0, FUNCTION, "CLIENT MODE: LIST::SIGCHAIN completed for ", hashGenesis.SubString());
                        break;
                    }
                }
                while(LLD::Logical->ReadLastConfirmed(hashGenesis, hashLast));
            }
            else
                debug::error(FUNCTION, "no connections available...");
        }
    }


    /* Refresh our notifications for a given sigchain. */
    void Indexing::DownloadNotifications(const uint256_t& hashGenesis)
    {
        /* Check for client mode. */
        if(!config::fClient.load())
            return;

        /* Check for genesis. */
        if(LLP::TRITIUM_SERVER)
        {
            /* Find an active connection to sync from. */
            std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
            if(pNode != nullptr)
            {
                debug::log(0, FUNCTION, "CLIENT MODE: Synchronizing Notifications");

                /* Get our current tritium events sequence now. */
                uint32_t nTritiumSequence = 0;
                LLD::Logical->ReadTritiumSequence(hashGenesis, nTritiumSequence);

                /* Loop until we have received all of our events. */
                do
                {
                    /* Request the sig chain. */
                    debug::log(0, FUNCTION, "CLIENT MODE: Requesting LIST::NOTIFICATION from ", nTritiumSequence, " for ", hashGenesis.SubString());
                    LLP::TritiumNode::BlockingMessage
                    (
                        30000,
                        pNode.get(), LLP::TritiumNode::ACTION::LIST,
                        uint8_t(LLP::TritiumNode::TYPES::NOTIFICATION), hashGenesis, nTritiumSequence
                    );
                    debug::log(0, FUNCTION, "CLIENT MODE: LIST::NOTIFICATION received for ", hashGenesis.SubString());

                    /* Check for shutdown. */
                    if(config::fShutdown.load())
                        break;

                    /* Cache our current sequence to see if we got any new events while waiting. */
                    uint32_t nCurrentSequence = 0;
                    LLD::Logical->ReadTritiumSequence(hashGenesis, nCurrentSequence);

                    /* Check that are starting and current sequences match. */
                    if(nCurrentSequence == nTritiumSequence)
                    {
                        debug::log(0, FUNCTION, "CLIENT MODE: LIST::NOTIFICATION completed for ", hashGenesis.SubString());
                        break;
                    }
                }
                while(LLD::Logical->ReadTritiumSequence(hashGenesis, nTritiumSequence));

                /* Get our current legacy events sequence now. */
                uint32_t nLegacySequence = 0;
                LLD::Logical->ReadLegacySequence(hashGenesis, nLegacySequence);

                /* Loop until we have received all of our events. */
                do
                {
                    /* Request the sig chain. */
                    debug::log(0, FUNCTION, "CLIENT MODE: Requesting LIST::LEGACY::NOTIFICATION from ", nLegacySequence, " for ", hashGenesis.SubString());
                    LLP::TritiumNode::BlockingMessage
                    (
                        30000,
                        pNode.get(), LLP::TritiumNode::ACTION::LIST,
                        uint8_t(LLP::TritiumNode::SPECIFIER::LEGACY), uint8_t(LLP::TritiumNode::TYPES::NOTIFICATION),
                        hashGenesis, nLegacySequence
                    );
                    debug::log(0, FUNCTION, "CLIENT MODE: LIST::LEGACY::NOTIFICATION received for ", hashGenesis.SubString());

                    /* Check for shutdown. */
                    if(config::fShutdown.load())
                        break;

                    /* Cache our current sequence to see if we got any new events while waiting. */
                    uint32_t nCurrentSequence = 0;
                    LLD::Logical->ReadLegacySequence(hashGenesis, nCurrentSequence);

                    /* Check that are starting and current sequences match. */
                    if(nCurrentSequence == nLegacySequence)
                    {
                        debug::log(0, FUNCTION, "CLIENT MODE: LIST::LEGACY::NOTIFICATION completed for ", hashGenesis.SubString());
                        break;
                    }
                }
                while(LLD::Logical->ReadLegacySequence(hashGenesis, nLegacySequence));
            }
            else
                debug::error(FUNCTION, "no connections available...");
        }
    }


    /* Default destructor. */
    void Indexing::InitializeThread()
    {
        /* Track our current genesis that we are initializing. */
        uint256_t hashSession = TAO::API::Authentication::SESSION::INVALID;

        /* Main loop controlled by condition variable. */
        std::mutex CONDITION_MUTEX;
        while(!config::fShutdown.load())
        {
            try
            {
                /* Cleanup our previous indexing session by setting our status. */
                if(hashSession != TAO::API::Authentication::SESSION::INVALID)
                {
                    /* Get our current genesis-id to start initialization. */
                    const uint256_t hashGenesis =
                        Authentication::Caller(hashSession);

                    /* Track our ledger database sequence. */
                    uint32_t nLegacySequence = 0;
                    LLD::Logical->ReadLegacySequence(hashGenesis, nLegacySequence);

                    /* Track our logical database sequence. */
                    uint32_t nLogicalSequence = 0;
                    LLD::Logical->ReadTritiumSequence(hashGenesis, nLogicalSequence);

                    //TODO: check why we are getting an extra transaction on FLAGS::MEMPOOL
                    uint512_t hashLedgerLast = 0;
                    LLD::Ledger->ReadLast(hashGenesis, hashLedgerLast, TAO::Ledger::FLAGS::MEMPOOL);

                    TAO::Ledger::Transaction txLedgerLast;
                    LLD::Ledger->ReadTx(hashLedgerLast, txLedgerLast, TAO::Ledger::FLAGS::MEMPOOL);

                    uint512_t hashLogicalLast = 0;
                    LLD::Logical->ReadLast(hashGenesis, hashLogicalLast);

                    TAO::API::Transaction txLogicalLast;
                    LLD::Logical->ReadTx(hashLogicalLast, txLogicalLast);

                    uint32_t nLogicalHeight = txLogicalLast.nSequence;
                    uint32_t nLedgerHeight  = txLedgerLast.nSequence;

                    /* Set our indexing status to ready now. */
                    Authentication::SetReady(hashSession);

                    /* Debug output to track our sequences. */
                    debug::log(0, FUNCTION, "Completed building indexes at ", VARIABLE(nLegacySequence), " | ", VARIABLE(nLogicalSequence), " | ", VARIABLE(nLedgerHeight), " | ", VARIABLE(nLogicalHeight), " for genesis=", hashGenesis.SubString());

                    /* Reset the genesis-id now. */
                    hashSession = TAO::API::Authentication::SESSION::INVALID;

                    continue;
                }

                /* Wait for entries in the queue. */
                std::unique_lock<std::mutex> CONDITION_LOCK(CONDITION_MUTEX);
                INITIALIZE_CONDITION.wait(CONDITION_LOCK,
                [&]
                {
                    /* Check for shutdown. */
                    if(config::fShutdown.load())
                        return true;

                    /* Check for suspended state. */
                    if(config::fSuspended.load())
                        return false;

                    /* Check for a session that needs to be wiped. */
                    if(hashSession != TAO::API::Authentication::SESSION::INVALID)
                        return true;

                    return Indexing::INITIALIZE->size() != 0;
                });

                /* Check for shutdown. */
                if(config::fShutdown.load())
                    return;

                /* Check that we have items in the queue. */
                if(Indexing::INITIALIZE->empty())
                    continue;

                /* Get the current genesis-id to initialize for. */
                hashSession = INITIALIZE->front();
                INITIALIZE->pop();

                /* Get our current genesis-id to start initialization. */
                const uint256_t hashGenesis =
                    Authentication::Caller(hashSession);

                /* Sync the sigchain if an active client before building our indexes. */
                if(config::fClient.load())
                {
                    /* Broadcast our unconfirmed transactions first. */
                    BroadcastUnconfirmed(hashGenesis);

                    /* Process our sigchain events now. */
                    DownloadNotifications(hashGenesis);
                    DownloadSigchain(hashGenesis);

                    /* Exit out of this thread if we are shutting down. */
                    if(config::fShutdown.load())
                        return;
                }

                /* EVENTS INDEXING DISABLED for -client mode since we build them when downloading. */
                else
                {
                    /* Read our last sequence. */
                    uint32_t nTritiumSequence = 0;
                    LLD::Logical->ReadTritiumSequence(hashGenesis, nTritiumSequence);

                    /* Read our last sequence. */
                    uint32_t nLegacySequence = 0;
                    LLD::Logical->ReadLegacySequence(hashGenesis, nLegacySequence);

                    /* Debug output so w4e can track our events indexes. */
                    debug::log(2, FUNCTION, "Building events indexes from ", VARIABLE(nTritiumSequence), " | ", VARIABLE(nLegacySequence), " for genesis=", hashGenesis.SubString());

                    /* Loop through our ledger level events. */
                    TAO::Ledger::Transaction tTritium;
                    while(LLD::Ledger->ReadEvent(hashGenesis, nTritiumSequence++, tTritium))
                    {
                        /* Check for shutdown. */
                        if(config::fShutdown.load())
                            return;

                        /* Cache our current event's txid. */
                        const uint512_t hashEvent =
                            tTritium.GetHash(true); //true to override cache

                        /* Index our dependant transaction. */
                        IndexDependant(hashEvent, tTritium);
                    }

                    /* Loop through our ledger level events. */
                    Legacy::Transaction tLegacy;
                    while(LLD::Legacy->ReadEvent(hashGenesis, nLegacySequence++, tLegacy))
                    {
                        /* Check for shutdown. */
                        if(config::fShutdown.load())
                            return;

                        /* Cache our current event's txid. */
                        const uint512_t hashEvent =
                            tLegacy.GetHash();

                        /* Index our dependant transaction. */
                        IndexDependant(hashEvent, tLegacy);
                    }

                    /* Check our current last hash from ledger layer. */
                    uint512_t hashLedger;
                    if(LLD::Ledger->ReadLast(hashGenesis, hashLedger, TAO::Ledger::FLAGS::MEMPOOL))
                    {
                        /* Check that our last indexing entries match. */
                        uint512_t hashLogical = 0;
                        if(!LLD::Logical->ReadLast(hashGenesis, hashLogical) || hashLedger != hashLogical)
                        {
                            /* Check if our logical hash is indexed so we know logical database is behind. */
                            if(LLD::Ledger->HasIndex(hashLogical) || hashLogical == 0)
                            {
                                debug::log(1, FUNCTION, "Building indexes for genesis=", hashGenesis.SubString(), " from=", hashLedger.SubString());

                                /* Build list of transaction hashes. */
                                std::vector<uint512_t> vHashes;

                                /* Read all transactions from our last index. */
                                uint512_t hashTx = hashLedger;
                                while(!config::fShutdown.load())
                                {
                                    /* Read the transaction from the ledger database. */
                                    TAO::Ledger::Transaction tx;
                                    if(!LLD::Ledger->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                                    {
                                        debug::warning(FUNCTION, "read failed at ", hashTx.SubString());
                                        break;
                                    }

                                    /* Push transaction to list. */
                                    vHashes.push_back(hashTx); //this will warm up the LLD cache if available, or remain low footprint if not

                                    /* Check for first. */
                                    if(tx.IsFirst() || hashTx == hashLogical)
                                        break;

                                    /* Set hash to previous hash. */
                                    hashTx = tx.hashPrevTx;
                                }

                                /* Reverse iterate our list of entries and index. */
                                for(auto hashTx = vHashes.rbegin(); hashTx != vHashes.rend(); ++hashTx)
                                {
                                    /* Index our sigchain events now. */
                                    IndexSigchain(*hashTx);

                                    /* Log that tx was rebroadcast. */
                                    debug::log(1, FUNCTION, "Indexed ", hashTx->SubString(), " to logical db");
                                }
                            }

                            /* Otherwise logical database is ahead and we need to re-broadcast. */
                            else
                                BroadcastUnconfirmed(hashGenesis);
                        }
                    }
                }
            }
            catch(const Exception& e)
            {
                debug::warning(e.what());
            }
        }
    }


    /* Index transaction level events for logged in sessions. */
    void Indexing::IndexDependant(const uint512_t& hashTx, const Legacy::Transaction& tx)
    {
        /* Loop through the available outputs. */
        for(uint32_t nContract = 0; nContract < tx.vout.size(); nContract++)
        {
            /* Grab a reference of our output. */
            const Legacy::TxOut& txout = tx.vout[nContract];

            /* Extract our register address. */
            uint256_t hashTo;
            if(Legacy::ExtractRegister(txout.scriptPubKey, hashTo))
            {
                /* Read the owner of register. (check this for MEMPOOL, too) */
                TAO::Register::State state;
                if(!LLD::Register->ReadState(hashTo, state, TAO::Ledger::FLAGS::LOOKUP))
                    continue;

                /* Check if owner is authenticated. */
                if(Authentication::Active(state.hashOwner))
                {
                    /* Write our events to database. */
                    if(!LLD::Logical->PushEvent(state.hashOwner, hashTx, nContract))
                        continue;

                    /* Increment our sequence. */
                    if(!LLD::Logical->IncrementLegacySequence(state.hashOwner))
                        continue;

                    debug::log(2, FUNCTION, "LEGACY: ",
                        "for genesis ", state.hashOwner.SubString(), " | ", VARIABLE(hashTx.SubString()), ", ", VARIABLE(nContract));
                }
            }
        }
    }


    /* Index transaction level events for logged in sessions. */
    void Indexing::IndexDependant(const uint512_t& hashTx, const TAO::Ledger::Transaction& tx)
    {
        /* Check all the tx contracts. */
        for(uint32_t nContract = 0; nContract < tx.Size(); nContract++)
        {
            /* Grab reference of our contract. */
            const TAO::Operation::Contract& rContract = tx[nContract];

            /* Skip to our primitive. */
            rContract.SeekToPrimitive();

            /* Check the contract's primitive. */
            uint8_t nOP = 0;
            rContract >> nOP;
            switch(nOP)
            {
                case TAO::Operation::OP::TRANSFER:
                case TAO::Operation::OP::DEBIT:
                {
                    /* Get the register address. */
                    TAO::Register::Address hashAddress;
                    rContract >> hashAddress;

                    /* Deserialize recipient from contract. */
                    TAO::Register::Address hashRecipient;
                    rContract >> hashRecipient;

                    /* Special check when handling a DEBIT. */
                    if(nOP == TAO::Operation::OP::DEBIT)
                    {
                        /* Skip over partials as this is handled separate. */
                        if(hashRecipient.IsObject())
                            continue;

                        /* Read the owner of register. (check this for MEMPOOL, too) */
                        TAO::Register::State oRegister;
                        if(!LLD::Register->ReadState(hashRecipient, oRegister, TAO::Ledger::FLAGS::LOOKUP))
                            continue;

                        /* Set our hash to based on owner. */
                        hashRecipient = oRegister.hashOwner;
                    }

                    /* Check if we need to build index for this contract. */
                    if(Authentication::Active(hashRecipient))
                    {
                        /* Push to unclaimed indexes if processing incoming transfer. */
                        if(nOP == TAO::Operation::OP::TRANSFER && !LLD::Logical->PushUnclaimed(hashRecipient, hashAddress))
                            continue;

                        /* Write our events to database. */
                        if(!LLD::Logical->PushEvent(hashRecipient, hashTx, nContract))
                        {
                            debug::warning(FUNCTION, "failed to push event for ", hashTx.SubString(), " contract ", nContract);
                            continue;
                        }

                        /* Increment our sequence. */
                        if(!LLD::Logical->IncrementTritiumSequence(hashRecipient))
                        {
                            debug::warning(FUNCTION, "failed to increment sequence for ", hashTx.SubString(), " contract ", nContract);
                            continue;
                        }
                    }

                    debug::log(2, FUNCTION, (nOP == TAO::Operation::OP::TRANSFER ? "TRANSFER: " : "DEBIT: "),
                        "for genesis ", hashRecipient.SubString(), " | ", VARIABLE(hashTx.SubString()), ", ", VARIABLE(nContract));

                    break;
                }

                case TAO::Operation::OP::COINBASE:
                {
                    /* Get the genesis. */
                    uint256_t hashRecipient;
                    rContract >> hashRecipient;

                    /* Check if we need to build index for this contract. */
                    if(Authentication::Active(hashRecipient))
                    {
                        /* Write our events to database. */
                        if(!LLD::Logical->PushEvent(hashRecipient, hashTx, nContract))
                            continue;

                        /* We don't increment our events index for miner coinbase contract. */
                        if(hashRecipient == tx.hashGenesis)
                            continue;

                        /* Increment our sequence. */
                        if(!LLD::Logical->IncrementTritiumSequence(hashRecipient))
                            continue;

                        debug::log(2, FUNCTION, "COINBASE: for genesis ", hashRecipient.SubString(), " | ", VARIABLE(hashTx.SubString()), ", ", VARIABLE(nContract));
                    }

                    break;
                }
            }
        }
    }


    /* Default destructor. */
    void Indexing::Shutdown()
    {
        /* Cleanup our initialization thread. */
        INITIALIZE_CONDITION.notify_all();
        if(INITIALIZE_THREAD.joinable())
            INITIALIZE_THREAD.join();

        /* Cleanup our dispatch thread. */
        CONDITION.notify_all();
        if(EVENTS_THREAD.joinable())
            EVENTS_THREAD.join();

        /* Clear open registrations. */
        {
            LOCK(REGISTERED_MUTEX);
            REGISTERED.clear();
        }
    }


    /* Index list of user level indexing entries. */
    void Indexing::index_transaction(const uint512_t& hash, const TAO::Ledger::Transaction& tx)
    {
        /* Check if we need to index the main sigchain. */
        if(Authentication::Active(tx.hashGenesis))
        {
            /* Build an API transaction. */
            TAO::API::Transaction tIndex =
                TAO::API::Transaction(tx);

            /* Index the transaction to the database. */
            if(!tIndex.Index(hash))
                return;
        }

        /* Check all the tx contracts. */
        for(uint32_t nContract = 0; nContract < tx.Size(); nContract++)
        {
            /* Grab reference of our contract. */
            const TAO::Operation::Contract& rContract = tx[nContract];

            /* Skip to our primitive. */
            rContract.SeekToPrimitive();

            /* Check the contract's primitive. */
            uint8_t nOP = 0;
            rContract >> nOP;
            switch(nOP)
            {
                case TAO::Operation::OP::TRANSFER:
                case TAO::Operation::OP::DEBIT:
                {
                    /* Get the register address. */
                    TAO::Register::Address hashAddress;
                    rContract >> hashAddress;

                    /* Deserialize recipient from contract. */
                    TAO::Register::Address hashRecipient;
                    rContract >> hashRecipient;

                    /* Special check when handling a DEBIT. */
                    if(nOP == TAO::Operation::OP::DEBIT)
                    {
                        /* Skip over partials as this is handled separate. */
                        if(hashRecipient.IsObject())
                            continue;

                        /* Read the owner of register. (check this for MEMPOOL, too) */
                        TAO::Register::State oRegister;
                        if(!LLD::Register->ReadState(hashRecipient, oRegister, TAO::Ledger::FLAGS::LOOKUP))
                            continue;

                        /* Set our hash to based on owner. */
                        hashRecipient = oRegister.hashOwner;

                        /* Check for active debit from with contract. */
                        if(Authentication::Active(tx.hashGenesis))
                        {
                            /* Write our events to database. */
                            if(!LLD::Logical->PushContract(tx.hashGenesis, hash, nContract))
                                continue;
                        }
                    }

                    /* Check if we need to build index for this contract. */
                    if(Authentication::Active(hashRecipient))
                    {
                        /* Push to unclaimed indexes if processing incoming transfer. */
                        if(nOP == TAO::Operation::OP::TRANSFER && !LLD::Logical->PushUnclaimed(hashRecipient, hashAddress))
                            continue;

                        /* Write our events to database. */
                        if(!LLD::Logical->PushEvent(hashRecipient, hash, nContract))
                            continue;

                        /* Increment our sequence. */
                        if(!LLD::Logical->IncrementTritiumSequence(hashRecipient))
                            continue;
                    }

                    debug::log(2, FUNCTION, (nOP == TAO::Operation::OP::TRANSFER ? "TRANSFER: " : "DEBIT: "),
                        "for genesis ", hashRecipient.SubString(), " | ", VARIABLE(hash.SubString()), ", ", VARIABLE(nContract));

                    break;
                }

                case TAO::Operation::OP::COINBASE:
                {
                    /* Get the genesis. */
                    uint256_t hashRecipient;
                    rContract >> hashRecipient;

                    /* Check if we need to build index for this contract. */
                    if(Authentication::Active(hashRecipient))
                    {
                        /* Write our events to database. */
                        if(!LLD::Logical->PushEvent(hashRecipient, hash, nContract))
                            continue;

                        /* We don't increment our events index for miner coinbase contract. */
                        if(hashRecipient == tx.hashGenesis)
                            continue;

                        /* Increment our sequence. */
                        if(!LLD::Logical->IncrementTritiumSequence(hashRecipient))
                            continue;

                        debug::log(2, FUNCTION, "COINBASE: for genesis ", hashRecipient.SubString(), " | ", VARIABLE(hash.SubString()), ", ", VARIABLE(nContract));
                    }

                    break;
                }
            }
        }
    }
}
