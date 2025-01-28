/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2023

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"Labor omnia vincit" - 	Work conquers all

____________________________________________________________________________________________*/

#include <LLD/include/global.h>
#include <LLP/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Build a user's indexing entries. */
    void Indexing::BuildIndexes(const uint256_t& hashSession)
    {
        /* Get our current genesis-id to start initialization. */
        const uint256_t hashGenesis =
            Authentication::Caller(hashSession);

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

        /* Check that our ledger indexes are up-to-date with our logical indexes. */
        uint512_t hashLedger = 0;
        if(LLD::Ledger->ReadLast(hashGenesis, hashLedger, TAO::Ledger::FLAGS::MEMPOOL))
        {
            /* Build list of transaction hashes. */
            std::vector<uint512_t> vBuild;

            /* Read all transactions from our last index. */
            hashLast = hashLedger;
            while(!config::fShutdown.load())
            {
                /* Read the transaction from the ledger database. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                {
                    debug::warning(FUNCTION, "pre-build read failed at ", hashLast.SubString());
                    break;
                }

                /* Check for valid logical indexes. */
                if(!LLD::Sessions->HasTx(hashLast))
                    vBuild.push_back(hashLast);
                else
                    break;

                /* Break on first after we have checked indexes. */
                if(tx.IsFirst())
                    break;

                /* Set hash to previous hash. */
                hashLast = tx.hashPrevTx;
            }

            /* Only output our data when we have indexes to build. */
            if(!vBuild.empty())
            {
                debug::log(1, FUNCTION, "Building ", vBuild.size(), " indexes for genesis=", hashGenesis.SubString());

                /* Reverse iterate our list of entries and index. */
                for(auto hashTx = vBuild.rbegin(); hashTx != vBuild.rend(); ++hashTx)
                {
                    /* Fire off indexing now. */
                    IndexSigchain(*hashTx);

                    /* Log that tx was rebroadcast. */
                    debug::log(1, FUNCTION, "Built Indexes for ", hashTx->SubString(), " to logical db");
                }
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
