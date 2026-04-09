/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2026

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"Simulac haesitas, sequitur timor." - As soon as you hesitate, fear follows.

____________________________________________________________________________________________*/

#include <Legacy/include/evaluate.h>

#include <LLD/include/global.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/indexing.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/types/mempool.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Index a transaction related to a specific session. */
    void Indexing::IndexSession(const uint512_t& hashTx)
    {
        /* Check if handling legacy or tritium. */
        if(hashTx.GetType() == TAO::Ledger::TRITIUM)
        {
            /* Index our sigchain first. */
            IndexSigchain(hashTx);

            /* Make sure the transaction is on disk. */
            TAO::Ledger::Transaction tx;
            if(LLD::Ledger->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                IndexDependant(hashTx, tx);

            return; //code style
        }

        /* Check for legacy transaction type. */
        if(hashTx.GetType() == TAO::Ledger::LEGACY)
        {
            /* Make sure the transaction is on disk. */
            Legacy::Transaction tx;
            if(LLD::Legacy->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
                IndexDependant(hashTx, tx);

            return; //code style
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
            if(LLD::Ledger->ReadTx(hashTx, tx, TAO::Ledger::FLAGS::MEMPOOL))
            {
                /* Check if we need to index the main sigchain. */
                if(LLD::Sessions->Active(tx.hashGenesis)) //we want to catch all calls to this without SESSION_TIMEOUT
                {
                    /* Build an API transaction. */
                    TAO::API::Transaction tIndex =
                        TAO::API::Transaction(tx);

                    /* Index the transaction to the database. */
                    if(!tIndex.Index(hashTx))
                        return;
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
        if(!LLD::Sessions->ReadLast(hashGenesis, hash))
            return;

        /* Loop until we reach confirmed transaction. */
        while(!config::fShutdown.load())
        {
            /* Read the transaction from the ledger database. */
            TAO::API::Transaction tx;
            if(!LLD::Sessions->ReadTx(hash, tx))
            {
                debug::warning(FUNCTION, "read for ", hashGenesis.SubString(), " failed at tx ", hash.SubString());
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
            if(!LLD::Sessions->ReadTx(*hash, tx))
            {
                debug::warning(FUNCTION, "read for ", hashGenesis.SubString(), " failed at tx ", hash->SubString());
                break;
            }

            /* Broadcast our transaction if it is in the mempool already. */
            if(TAO::Ledger::mempool.Has(*hash))
                tx.Broadcast();

            /* Otherwise accept and execute this transaction. */
            else if(!TAO::Ledger::mempool.Accept(tx))
            {
                debug::warning(FUNCTION, "accept for ", hash->SubString(), " failed");
                continue;
            }
        }
    }


    /* Index transaction level events for logged in sessions. */
    void Indexing::IndexDependant(const uint512_t& hashTx, const Legacy::Transaction& tx)
    {
        /* Loop thgrough the available outputs. */
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
                if(LLD::Sessions->Active(state.hashOwner))
                {
                    /* Debug output to show event has fired. */
                    debug::log(1, FUNCTION, "LEGACY: ",
                        "for genesis ", state.hashOwner.SubString(), " | ", hashTx.SubString());

                    /* Write our events to database. */
                    if(LLD::Sessions->PushEvent(state.hashOwner, hashTx, nContract))
                        LLD::Sessions->IncrementLegacySequence(state.hashOwner);
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

            /* Switch based on our primitive type that could be a dependant. */
            switch(nOP)
            {
                /* Handle for typical Debits and Transfers. */
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
                        /* Skip over partials as this is handled seperate. */
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
                    if(LLD::Sessions->Active(hashRecipient))
                    {
                        /* Debug output to show event has fired. */
                        debug::log(1, FUNCTION, (nOP == TAO::Operation::OP::TRANSFER ? "TRANSFER: " : "DEBIT: "),
                            "for genesis ", hashRecipient.SubString(), " | ", hashTx.SubString());

                        /* Push to unclaimed indexes if processing incoming transfer. */
                        if(nOP == TAO::Operation::OP::TRANSFER)
                            LLD::Sessions->PushUnclaimed(hashRecipient, hashAddress);

                        /* Write our events to database. */
                        if(LLD::Sessions->PushEvent(hashRecipient, hashTx, nContract))
                            LLD::Sessions->IncrementTritiumSequence(hashRecipient);
                    }

                    break;
                }

                /* Handle for a coinbase contract. */
                case TAO::Operation::OP::COINBASE:
                {
                    /* Get the genesis. */
                    uint256_t hashRecipient;
                    rContract >> hashRecipient;

                    /* Check if we need to build index for this contract. */
                    if(LLD::Sessions->Active(hashRecipient))
                    {
                        /* Debug output to show event has fired. */
                        debug::log(1, FUNCTION, "COINBASE: for genesis ", hashRecipient.SubString(), " | ", hashTx.SubString());

                        /* Write our events to database. */
                        if(LLD::Sessions->PushEvent(hashRecipient, hashTx, nContract))
                        {
                            /* We don't increment our events index for miner coinbase contract. */
                            if(hashRecipient == tx.hashGenesis)
                                continue;

                            /* Increment our events sequence if not our sigchain. */
                            LLD::Sessions->IncrementTritiumSequence(hashRecipient);
                        }
                    }

                    break;
                }
            }
        }
    }
}
