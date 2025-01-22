/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2023

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

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
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


    /* Thread for refreshing events. */
    std::thread Indexing::REFRESH_THREAD;


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
        Indexing::DISPATCH       = util::atomic::lock_unique_ptr<std::queue<uint512_t>>(new std::queue<uint512_t>());
        Indexing::EVENTS_THREAD  = std::thread(&Indexing::Manager);
        Indexing::REFRESH_THREAD = std::thread(&Indexing::RefreshEvents);


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
                    LLD::Sessions->ReadLegacySequence(hashGenesis, nLegacySequence);

                    /* Track our logical database sequence. */
                    uint32_t nLogicalSequence = 0;
                    LLD::Sessions->ReadTritiumSequence(hashGenesis, nLogicalSequence);

                    //TODO: check why we are getting an extra transaction on FLAGS::MEMPOOL
                    uint512_t hashLedgerLast = 0;
                    LLD::Ledger->ReadLast(hashGenesis, hashLedgerLast, TAO::Ledger::FLAGS::MEMPOOL);

                    TAO::Ledger::Transaction txLedgerLast;
                    LLD::Ledger->ReadTx(hashLedgerLast, txLedgerLast, TAO::Ledger::FLAGS::MEMPOOL);

                    uint512_t hashLogicalLast = 0;
                    LLD::Sessions->ReadLast(hashGenesis, hashLogicalLast);

                    TAO::API::Transaction txLogicalLast;
                    LLD::Sessions->ReadTx(hashLogicalLast, txLogicalLast);

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
                    LLD::Sessions->ReadTritiumSequence(hashGenesis, nTritiumSequence);

                    /* Read our last sequence. */
                    uint32_t nLegacySequence = 0;
                    LLD::Sessions->ReadLegacySequence(hashGenesis, nLegacySequence);

                    /* Debug output so w4e can track our events indexes. */
                    debug::log(2, FUNCTION, "Building events indexes from ", VARIABLE(nTritiumSequence), " | ", VARIABLE(nLegacySequence), " for genesis=", hashGenesis.SubString());

                    /* Track our last event processed so we don't double up our work. */
                    uint512_t hashLast;

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

                        /* Check if we have already processed this event. */
                        if(hashEvent == hashLast)
                            continue;

                        /* Index our dependant transaction. */
                        IndexDependant(hashEvent, tTritium);

                        /* Set our new dependant hash. */
                        hashLast = hashEvent;
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

                        /* Check if we have already processed this event. */
                        if(hashEvent == hashLast)
                            continue;

                        /* Index our dependant transaction. */
                        IndexDependant(hashEvent, tLegacy);

                        /* Set our new dependant hash. */
                        hashLast = hashEvent;
                    }

                    /* Check that our ledger indexes are up-to-date with our logical indexes. */
                    uint512_t hashLedger = 0;
                    if(LLD::Ledger->ReadLast(hashGenesis, hashLedger, TAO::Ledger::FLAGS::MEMPOOL))
                    {
                        /* Build list of transaction hashes. */
                        std::vector<uint512_t> vBuild;

                        /* Read all transactions from our last index. */
                        uint512_t hashTx = hashLedger;
                        while(!config::fShutdown.load())
                        {
                            /* Read the transaction from the ledger database. */
                            TAO::Ledger::Transaction tx;
                            if(!LLD::Ledger->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                            {
                                debug::warning(FUNCTION, "pre-build read failed at ", hashTx.SubString());
                                break;
                            }

                            /* Check for valid logical indexes. */
                            if(!LLD::Sessions->HasTx(hashTx))
                                vBuild.push_back(hashTx);
                            else
                                break;

                            /* Break on first after we have checked indexes. */
                            if(tx.IsFirst())
                                break;

                            /* Set hash to previous hash. */
                            hashTx = tx.hashPrevTx;
                        }

                        /* Only output our data when we have indexes to build. */
                        if(!vBuild.empty())
                            debug::log(1, FUNCTION, "Building ", vBuild.size(), " indexes for genesis=", hashGenesis.SubString());

                        /* Reverse iterate our list of entries and index. */
                        for(auto hashTx = vBuild.rbegin(); hashTx != vBuild.rend(); ++hashTx)
                        {
                            /* Read the transaction from the ledger database. */
                            TAO::Ledger::Transaction tx;
                            if(!LLD::Ledger->ReadTx(*hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                            {
                                debug::warning(FUNCTION, "build read failed at ", hashTx->SubString());
                                break;
                            }

                            /* Index the transaction on main sigchain. */
                            index_transaction(*hashTx, tx);

                            /* Log that tx was rebroadcast. */
                            debug::log(1, FUNCTION, "Built Indexes for ", hashTx->SubString(), " to logical db");
                        }
                    }


                    /* Check that our last indexing entries match. */
                    uint512_t hashLogical = 0;
                    if(LLD::Sessions->ReadLast(hashGenesis, hashLogical))
                    {
                        /* Build list of transaction hashes. */
                        std::vector<uint512_t> vIndex;

                        /* Read all transactions from our last index. */
                        uint512_t hashTx = hashLogical;
                        while(!config::fShutdown.load())
                        {
                            /* Read the transaction from the ledger database. */
                            TAO::API::Transaction tx;
                            if(!LLD::Sessions->ReadTx(hashTx, tx))
                            {
                                debug::warning(FUNCTION, "pre-update read failed at ", hashTx.SubString());
                                break;
                            }

                            /* Break on our first confirmed tx. */
                            if(tx.Confirmed())
                                break;

                            /* Check that we have an index here. */
                            if(LLD::Ledger->HasIndex(hashTx) && !tx.Confirmed())
                                vIndex.push_back(hashTx); //this will warm up the LLD cache if available, or remain low footprint if not

                            /* Break on first after we have checked indexes. */
                            if(tx.IsFirst())
                                break;

                            /* Set hash to previous hash. */
                            hashTx = tx.hashPrevTx;
                        }

                        /* Only output our data when we have indexes to build. */
                        if(!vIndex.empty())
                            debug::log(1, FUNCTION, "Updating ", vIndex.size(), " indexes for genesis=", hashGenesis.SubString());

                        /* Reverse iterate our list of entries and index. */
                        for(auto hashTx = vIndex.rbegin(); hashTx != vIndex.rend(); ++hashTx)
                        {
                            /* Read the transaction from the ledger database. */
                            TAO::API::Transaction tx;
                            if(!LLD::Sessions->ReadTx(*hashTx, tx))
                            {
                                debug::warning(FUNCTION, "update read failed at ", hashTx->SubString());
                                break;
                            }

                            /* Index the transaction to the database. */
                            if(!tx.Index(*hashTx))
                                debug::warning(FUNCTION, "failed to update index ", hashTx->SubString());

                            /* Log that tx was rebroadcast. */
                            debug::log(1, FUNCTION, "Updated Indexes for ", hashTx->SubString(), " to logical db");
                        }

                        /* Check if we need to re-broadcast anything. */
                        BroadcastUnconfirmed(hashGenesis);
                    }
                }
            }
            catch(const Exception& e)
            {
                debug::warning(e.what());
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

        /* Cleanup our dispatch thread. */
        if(REFRESH_THREAD.joinable())
            REFRESH_THREAD.join();

        /* Clear open registrations. */
        {
            LOCK(REGISTERED_MUTEX);
            REGISTERED.clear();
        }
    }
}
