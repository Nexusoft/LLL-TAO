/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>

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


    /* Set to track active login sessions */
    util::atomic::lock_unique_ptr<std::set<uint256_t>> Indexing::SESSIONS;


    /* Thread for running dispatch. */
    std::thread Indexing::EVENTS_THREAD;


    /* Condition variable to wake up the indexing thread. */
    std::condition_variable Indexing::CONDITION;


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

        /* Initialize our sessions object now. */
        Indexing::SESSIONS      = util::atomic::lock_unique_ptr<std::set<uint256_t>>(new std::set<uint256_t>());
    }


    /* Checks current events against transaction history to ensure we are up to date. */
    void Indexing::RefreshEvents()
    {
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
                /* Iterate the transaction contracts. */
                for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                {
                    /* Grab contract reference. */
                    const TAO::Operation::Contract& rContract = tx[nContract];

                    {
                        LOCK(REGISTERED_MUTEX);

                        /* Loop through registered commands. */
                        for(const auto& strCommands : REGISTERED)
                        {
                            debug::log(0, FUNCTION, "Dispatching for ", VARIABLE(strCommands), " | ", VARIABLE(nContract));
                            Commands::Instance(strCommands)->Index(rContract, nContract);
                        }
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

        debug::log(0, FUNCTION, "Complated scanning ", nScannedCount, " tx in ", timer.Elapsed(), " seconds");
    }


    /*  Index a new block hash to relay thread.*/
    void Indexing::PushIndex(const uint512_t& hashTx)
    {
        DISPATCH->push(hashTx);
        CONDITION.notify_one();
    }


    /* Push a new session to monitor for indexes. */
    void Indexing::PushSession(const uint256_t& hashGenesis)
    {
        /* Check that session isn't already registered. */
        if(SESSIONS->count(hashGenesis))
            return;

        /* Initialize our genesis sigchain. */
        initialize_sigchain(hashGenesis);

        /* Insert new session into our set to monitor. */
        SESSIONS->insert(hashGenesis);
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

            //TODO: check for legacy transaction events for sigchain events.

            /* Make sure the transaction is on disk. */
            TAO::Ledger::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashTx, tx))
                continue;

            /* Build our local sigchain events indexes. */
            index_transaction(hashTx, tx);

            /* Iterate the transaction contracts. */
            for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
            {
                /* Grab contract reference. */
                const TAO::Operation::Contract& rContract = tx[nContract];

                {
                    LOCK(REGISTERED_MUTEX);

                    /* Loop through registered commands. */
                    for(const auto& strCommands : REGISTERED)
                    {
                        debug::log(0, FUNCTION, "Dispatching for ", VARIABLE(strCommands), " | ", VARIABLE(nContract));
                        Commands::Instance(strCommands)->Index(rContract, nContract);
                    }
                }
            }

            /* Write our last index now. */
            if(!LLD::Logical->WriteLastIndex(hashTx))
                continue;
        }
    }


    /* Default destructor. */
    void Indexing::Shutdown()
    {
        /* Cleanup our dispatch thread. */
        CONDITION.notify_all();
        if(EVENTS_THREAD.joinable())
            EVENTS_THREAD.join();

        /* Clear open sessions. */
        SESSIONS->clear();

        /* Clear open registrations. */
        {
            LOCK(REGISTERED_MUTEX);
            REGISTERED.clear();
        }
    }


    /* Initialize a user's indexing entries. */
    void Indexing::initialize_sigchain(const uint256_t& hashGenesis)
    {
        /* Check our current last hash from ledger layer. */
        uint512_t hashLedger;
        if(!LLD::Ledger->ReadLast(hashGenesis, hashLedger, TAO::Ledger::FLAGS::MEMPOOL))
        {
            debug::log(0, FUNCTION, "No indexes for genesis=", hashGenesis.SubString());
            return;
        }

        /* Check that our last indexing entries match. */
        uint512_t hashLogical;
        if(!LLD::Logical->ReadLast(hashGenesis, hashLogical) || hashLedger != hashLogical)
        {
            debug::log(0, FUNCTION, "Buiding indexes for genesis=", hashGenesis.SubString());

            /* Build list of transaction hashes. */
            std::vector<uint512_t> vHashes;

            /* Read all transactions from our last index. */
            uint512_t hash = hashLedger;
            while(hash != hashLogical && !config::fShutdown.load())
            {
                /* Read the transaction from the ledger database. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hash, tx, TAO::Ledger::FLAGS::MEMPOOL))
                {
                    debug::warning(FUNCTION, "check for ", hashGenesis.SubString(), " failed at ", VARIABLE(hash.SubString()));
                    return;
                }

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
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(*hash, tx))
                {
                    debug::warning(FUNCTION, "index for ", hashGenesis.SubString(), " failed at ", VARIABLE(hash->SubString()));
                    return;
                }

                /* Build an API transaction. */
                TAO::API::Transaction tIndex =
                    TAO::API::Transaction(tx);

                /* Index the transaction to the database. */
                if(!tIndex.Index(*hash))
                    debug::warning(FUNCTION, "failed to index ", VARIABLE(hash->SubString()));
            }

            return;
        }

        //handle foreign sigchain events on sigchain sync by syncing up with recent events
    }


    /* Index list of user level indexing entries. */
    void Indexing::index_transaction(const uint512_t& hash, const TAO::Ledger::Transaction& tx)
    {
        /* Check if we need to index the main sigchain. */
        if(SESSIONS->count(tx.hashGenesis))
        {
            /* Build an API transaction. */
            TAO::API::Transaction tIndex =
                TAO::API::Transaction(tx);

            /* Index the transaction to the database. */
            if(!tIndex.Index(hash))
                debug::warning(FUNCTION, "failed to index ", VARIABLE(hash.SubString()));
        }

        /* Check all the tx contracts. */
        for(uint32_t n = 0; n < tx.Size(); ++n)
        {
            /* Grab reference of our contract. */
            const TAO::Operation::Contract& rContract = tx[n];

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
                    /* Seek to recipient. */
                    rContract.Seek(32,  TAO::Operation::Contract::OPERATIONS);

                    /* Deserialize recipient from contract. */
                    uint256_t hashRecipient;
                    rContract >> hashRecipient;

                    if(nOP == TAO::Operation::OP::DEBIT)
                    {
                        /* Read the owner of register. (check this for MEMPOOL, too) */
                        TAO::Register::State oRegister;
                        if(!LLD::Register->ReadState(hashRecipient, oRegister))
                            continue;

                        /* Set our hash to based on owner. */
                        hashRecipient = oRegister.hashOwner;
                    }

                    /* Check if we need to build index for this contract. */
                    if(SESSIONS->count(hashRecipient))
                    {
                        /* Check for local sessions. */
                        //if(SESSIONS->at(hashRecipient).Type() != Authentication::Session::LOCAL)
                        //    continue;

                        /* Write our events to database. */
                        if(!LLD::Logical->PushEvent(hashRecipient, rContract, n))
                        {
                            debug::error(FUNCTION, "Failed to write event (", VARIABLE(hashRecipient.SubString()), " | ", VARIABLE(n), ") to logical database");

                            continue;
                        }
                    }

                    debug::log(2, FUNCTION, (nOP == TAO::Operation::OP::TRANSFER ? "TRANSFER: " : "DEBIT: "),
                        "for genesis ", hashRecipient.SubString());

                    break;
                }

                case TAO::Operation::OP::COINBASE:
                {
                    /* Get the genesis. */
                    uint256_t hashRecipient;
                    rContract >> hashRecipient;

                    /* Check if we need to build index for this contract. */
                    if(SESSIONS->count(hashRecipient))
                    {
                        /* Check for local sessions. */
                        //if(SESSIONS->at(hashRecipient).Type() != Authentication::Session::LOCAL)
                        //    continue;

                        /* Write our events to database. */
                        if(!LLD::Logical->PushEvent(hashRecipient, rContract, n))
                        {
                            debug::error(FUNCTION, "Failed to write event (", VARIABLE(hashRecipient.SubString()), " | ", VARIABLE(n), ") to logical database");

                            continue;
                        }

                        debug::log(2, FUNCTION, "COINBASE: for genesis ", hashRecipient.SubString());
                    }

                    break;
                }

                //we want to tell clients or indexes that something was claimed for clients
                case TAO::Operation::OP::CLAIM:
                case TAO::Operation::OP::CREDIT:
                {

                    //check the txid to an active session
                    //if(LLD::Logical->ReadTx(hashDependant, rTx))
                    //{

                    //}

                    break;
                }
            }
        }
    }
}
