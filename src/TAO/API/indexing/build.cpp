/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2026

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"Labor omnia vincit" - 	Work conquers all

____________________________________________________________________________________________*/

#include <LLD/include/global.h>
#include <LLP/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Ledger/types/mempool.h>

#include <mutex>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Build a user's indexing entries. */
    void Indexing::BuildIndexes(const uint256_t& hashGenesis)
    {
        /* Track our last event processed so we don't double up our work. */
        uint512_t hashLast = 0;

        /* Read our last sequence. */
        uint32_t nTritiumSequence = 0;
        LLD::Sessions->ReadTritiumSequence(hashGenesis, nTritiumSequence);

        /* Debug output so w4e can track our events indexes. */
        debug::log(2, FUNCTION, "Building tritium event indexes from ", VARIABLE(nTritiumSequence), " for genesis=", hashGenesis.SubString());

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

        /* Read our last sequence. */
        uint32_t nLegacySequence = 0;
        LLD::Sessions->ReadLegacySequence(hashGenesis, nLegacySequence);

        /* Debug output so w4e can track our events indexes. */
        debug::log(2, FUNCTION, "Building legacy event indexes from ", VARIABLE(nLegacySequence), " for genesis=", hashGenesis.SubString());

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

        /* Hold the pool lock before the session index lock: the rebuild reads
         * mempool-backed transactions, including inside IndexSession().
         * Hold the session index lock before the first indexing mutation and
         * through both rebuild loops. IndexSession()/IndexSigchain() and the
         * later Index() loop both write indexing.last. Ledger-loaded API
         * transactions have hashNextTx == 0, so an interleaved Index() can
         * otherwise rewrite indexing.last to an older transaction. Each Index()
         * takes the lock again. Release both before BroadcastUnconfirmed(). */
        std::unique_lock<std::recursive_mutex> POOL_LOCK(TAO::Ledger::mempool.MUTEX);
        std::unique_lock<std::recursive_mutex> INDEX_LOCK(Transaction::IndexLock(hashGenesis));

        /* Check that our ledger indexes are up-to-date with our logical indexes. */
        uint512_t hashLedger = 0;
        if(LLD::Ledger->ReadLast(hashGenesis, hashLedger, TAO::Ledger::FLAGS::MEMPOOL))
        {
            /* Build list of transaction hashes. */
            std::vector<uint512_t> vBuild;

            /* Read all transactions from our last index. A failed read must
             * not fall through to a default-zero predecessor: that spins until
             * shutdown while this thread holds mempool.MUTEX. */
            hashLast = hashLedger;
            bool fLedgerWalkComplete = false;
            while(!config::fShutdown.load())
            {
                /* Read the transaction from the ledger database. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                {
                    debug::warning(FUNCTION, "incomplete index rebuild for genesis=",
                        hashGenesis.SubString(), ": failed to read ", hashLast.SubString());
                    break;
                }

                /* Check for valid logical indexes. */
                if(!LLD::Sessions->HasTx(hashLast))
                    vBuild.push_back(hashLast);
                else
                {
                    fLedgerWalkComplete = true;
                    break;
                }

                /* Break on first after we have checked indexes. */
                if(tx.IsFirst())
                {
                    fLedgerWalkComplete = true;
                    break;
                }

                /* A repeated or zero predecessor must not spin under the pool lock. */
                const uint512_t hashPrev = tx.hashPrevTx;
                if(hashPrev == hashLast)
                {
                    debug::warning(FUNCTION, "incomplete index rebuild for genesis=",
                        hashGenesis.SubString(), ": predecessor did not advance at ",
                        hashLast.SubString());
                    break;
                }
                hashLast = hashPrev;
            }

            /* Do not apply a partial predecessor chain. The next rebuild retries. */
            if(!fLedgerWalkComplete)
                vBuild.clear();

            /* Only output our data when we have indexes to build. */
            if(fLedgerWalkComplete && !vBuild.empty())
            {
                debug::log(1, FUNCTION, "Building ", vBuild.size(), " indexes for genesis=", hashGenesis.SubString());

                /* Reverse iterate our list of entries and index. */
                for(auto hashTx = vBuild.rbegin(); hashTx != vBuild.rend(); ++hashTx)
                    IndexSession(*hashTx);
            }
        }

        /* Check that our last indexing entries match. */
        uint512_t hashLogical = 0;
        if(LLD::Sessions->ReadLast(hashGenesis, hashLogical))
        {
            /* Build list of transaction hashes. */
            std::vector<uint512_t> vIndex;

            /* Read all transactions from our last index. Same failure rule as
             * the ledger walk above: stop, report, and do not use a partial chain. */
            uint512_t hashTx = hashLogical;
            bool fLogicalWalkComplete = false;
            while(!config::fShutdown.load())
            {
                /* Read the transaction from the ledger database. */
                TAO::API::Transaction tx;
                if(!LLD::Sessions->ReadTx(hashTx, tx))
                {
                    debug::warning(FUNCTION, "incomplete index rebuild for genesis=",
                        hashGenesis.SubString(), ": failed to read ", hashTx.SubString());
                    break;
                }

                /* Break on our first confirmed tx. */
                if(tx.Confirmed())
                {
                    fLogicalWalkComplete = true;
                    break;
                }

                /* Check that we have an index here. */
                if(LLD::Ledger->HasIndex(hashTx) && !tx.Confirmed())
                    vIndex.push_back(hashTx); //this will warm up the LLD cache if available, or remain low footprint if not

                /* Break on first after we have checked indexes. */
                if(tx.IsFirst())
                {
                    fLogicalWalkComplete = true;
                    break;
                }

                /* A repeated or zero predecessor must not spin under the pool lock. */
                const uint512_t hashPrev = tx.hashPrevTx;
                if(hashPrev == hashTx)
                {
                    debug::warning(FUNCTION, "incomplete index rebuild for genesis=",
                        hashGenesis.SubString(), ": predecessor did not advance at ",
                        hashTx.SubString());
                    break;
                }
                hashTx = hashPrev;
            }

            /* Do not apply a partial predecessor chain. The next rebuild retries. */
            if(!fLogicalWalkComplete)
                vIndex.clear();

            /* Only output our data when we have indexes to build. */
            if(fLogicalWalkComplete && !vIndex.empty())
                debug::log(1, FUNCTION, "Updating ", vIndex.size(), " indexes for genesis=", hashGenesis.SubString());

            /* Reverse iterate our list of entries and index. */
            for(auto hashTx = vIndex.rbegin(); hashTx != vIndex.rend(); ++hashTx)
            {
                /* Read the transaction from the ledger database. */
                TAO::API::Transaction tx;
                if(LLD::Sessions->ReadTx(*hashTx, tx))
                {
                    /* Index the transaction to the database. */
                    if(!tx.Index(*hashTx))
                        debug::warning(FUNCTION, "failed to update index ", hashTx->SubString());

                    /* Log that tx was rebroadcast. */
                    debug::log(1, FUNCTION, "Updated Indexes for ", hashTx->SubString(), " to logical db");
                }
            }

            INDEX_LOCK.unlock();
            POOL_LOCK.unlock();

            /* Check if we need to re-broadcast anything. */
            BroadcastUnconfirmed(hashGenesis);
        }
    }
}
