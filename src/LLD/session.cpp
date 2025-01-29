/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>

#include <LLD/include/global.h>

#include <TAO/API/types/transaction.h>

#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/unpack.h>

#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>

namespace LLD
{
    /** The Database Constructor. To determine file location and the Bytes per Record. **/
    SessionDB::SessionDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_SESSION")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)
    {
    }


    /** Default Destructor **/
    SessionDB::~SessionDB()
    {
    }


    /* Writes a session's access time to the database. */
    bool SessionDB::WriteAccess(const uint256_t& hashGenesis, const uint64_t nActive)
    {
        /* Only push to our access list if this is our first entry. */
        if(!Exists(std::make_pair(std::string("access"), hashGenesis)))
        {
            /* Start our list sequence. */
            uint32_t nSequence = 0;
            Read(std::string("sessions.sequence"), nSequence);

            /* Push this item to our list now. */
            Write(std::make_pair(std::string("sessions.list"), ++nSequence), hashGenesis);
        }

        /* Otherwise update our timestamp. */
        return Write(std::make_pair(std::string("access"), hashGenesis), nActive);
    }


    /* Reads a session's access time to the database. */
    bool SessionDB::ReadAccess(const uint256_t& hashGenesis, uint64_t &nActive)
    {
        return Read(std::make_pair(std::string("access"), hashGenesis), nActive);
    }


    /* Lists all the sessions that have been accessed on this node. */
    bool SessionDB::ListAccesses(std::map<uint256_t, uint64_t> &mapSessions, const uint64_t nTimeframe)
    {
        /* Make sure we clear our list first. */
        mapSessions.clear(); //just in case

        /* Track our current genesis we have read. */
        uint256_t hashGenesis = 0;

        /* Iterate until we run out of items. */
        uint32_t nSequence = 0;
        while(Read(std::make_pair(std::string("sessions.list"), nSequence++), hashGenesis))
        {
            /* Read the most recent access time. */
            uint64_t nAccess = 0;
            if(Read(std::make_pair(std::string("access"), hashGenesis), nAccess))
            {
                /* Check this against our timeframe. */
                if(nTimeframe == 0 || nAccess + nTimeframe < runtime::unifiedtimestamp())
                    mapSessions[hashGenesis] = nAccess;
            }
            else
                return false;
        }

        return true;
    }


    /* Writes a username - genesis hash pair to the local database. */
    bool SessionDB::WriteFirst(const SecureString& strUsername, const uint256_t& hashGenesis)
    {
        std::vector<uint8_t> vKey(strUsername.begin(), strUsername.end());
        return Write(std::make_pair(std::string("genesis"), vKey), hashGenesis);
    }


    /* Reads a genesis hash from the local database for a given username */
    bool SessionDB::ReadFirst(const SecureString& strUsername, uint256_t &hashGenesis)
    {
        std::vector<uint8_t> vKey(strUsername.begin(), strUsername.end());
        return Read(std::make_pair(std::string("genesis"), vKey), hashGenesis);
    }


    /* Writes session data to the local database. */
    bool SessionDB::WriteSession(const uint256_t& hashGenesis, const std::vector<uint8_t>& vchData)
    {
        return Write(std::make_pair(std::string("session"), hashGenesis), vchData);
    }


    /* Reads session data from the local database */
    bool SessionDB::ReadSession(const uint256_t& hashGenesis, std::vector<uint8_t>& vchData)
    {
        return Read(std::make_pair(std::string("session"), hashGenesis), vchData);
    }


    /* Deletes session data from the local database fort he given session ID. */
    bool SessionDB::EraseSession(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("session"), hashGenesis));
    }


    /* Determines whether the local DB contains session data for the given session ID */
    bool SessionDB::HasSession(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("session"), hashGenesis));
    }


    /* Reads the last txid that was indexed. */
    bool SessionDB::ReadLast(const uint256_t& hashGenesis, uint512_t &hashTx)
    {
        return Read(std::make_pair(std::string("indexing.last"), hashGenesis), hashTx);
    }


    /* Reads the last txid that was confirmed. */
    bool SessionDB::ReadLastConfirmed(const uint256_t& hashGenesis, uint512_t &hashTx)
    {
        /* Read the last transaction entry. */
        if(!Read(std::make_pair(std::string("indexing.last"), hashGenesis), hashTx))
            return false;

        /* Now iterate backwards and check for indexing entries. */
        while(!config::fShutdown.load())
        {
            /* Check if we have a valid indexing entry. */
            if(LLD::Ledger->HasIndex(hashTx))
                break;

            /* Read the transaction from the ledger database. */
            TAO::API::Transaction tx;
            if(!ReadTx(hashTx, tx))
                break;

            /* Check for first. */
            if(tx.IsFirst())
            {
                hashTx = 0; //we mark unindexed first transaction as 0 to get it from Tritium LLP
                break;
            }

            /* Set hash to previous hash. */
            hashTx = tx.hashPrevTx;
        }

        return true;
    }


    /* Writes the last txid that was indexed. */
    bool SessionDB::WriteLast(const uint256_t& hashGenesis, const uint512_t& hashTx)
    {
        return Write(std::make_pair(std::string("indexing.last"), hashGenesis), hashTx);
    }

    /* Writes the first transaction-id to disk. */
    bool SessionDB::WriteFirst(const uint256_t& hashGenesis, const uint512_t& hashTx)
    {
        return Write(std::make_pair(std::string("indexing.first"), hashGenesis), hashTx);
    }


    /* Checks if a given genesis-id has been indexed. */
    bool SessionDB::HasFirst(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("indexing.first"), hashGenesis));
    }


    /* Reads the first transaction-id from disk. */
    bool SessionDB::ReadFirst(const uint256_t& hashGenesis, uint512_t& hashTx)
    {
        return Read(std::make_pair(std::string("indexing.first"), hashGenesis), hashTx);
    }


    /* Reads the first transaction-id from disk. */
    bool SessionDB::ReadFirst(const uint256_t& hashGenesis, TAO::API::Transaction &tx)
    {
        /* Get our txid of first event. */
        uint512_t hashFirst = 0;
        if(!Read(std::make_pair(std::string("indexing.first"), hashGenesis), hashFirst))
            return false;

        return Read(std::make_pair(std::string("tx"), hashFirst), tx);
    }


    /* Writes the last txid that was indexed. */
    bool SessionDB::EraseLast(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("indexing.last"), hashGenesis));
    }


    /* Writes the first transaction-id to disk. */
    bool SessionDB::EraseFirst(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("indexing.first"), hashGenesis));
    }


    /* Writes a transaction to the Logical DB. */
    bool SessionDB::WriteTx(const uint512_t& hashTx, const TAO::API::Transaction& tx)
    {
        return Write(std::make_pair(std::string("tx"), hashTx), tx);
    }


    /* Erase a transaction from the Logical DB. */
    bool SessionDB::EraseTx(const uint512_t& hashTx)
    {
        return Erase(std::make_pair(std::string("tx"), hashTx));
    }

    /* Checks if a transaction exists. */
    bool SessionDB::HasTx(const uint512_t& hashTx)
    {
        return Exists(std::make_pair(std::string("tx"), hashTx));
    }


    /* Reads a transaction from the Logical DB. */
    bool SessionDB::ReadTx(const uint512_t& hashTx, TAO::API::Transaction &tx)
    {
        return Read(std::make_pair(std::string("tx"), hashTx), tx);
    }


    /* Push an register transaction to process for given genesis-id. */
    bool SessionDB::PushTransaction(const uint256_t& hashRegister, const uint512_t& hashTx)
    {
        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("transactions.sequence"), hashRegister), nOwnerSequence);

        /* Add our indexing entry by owner sequence number. */
        if(!Write(std::make_tuple(std::string("transactions.index"), nOwnerSequence, hashRegister), hashTx))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("transactions.sequence"), hashRegister), ++nOwnerSequence))
            return false;

        return true;
    }


    /* Erase an register transaction for given genesis-id. */
    bool SessionDB::EraseTransaction(const uint256_t& hashRegister)
    {
        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        if(!Read(std::make_pair(std::string("transactions.sequence"), hashRegister), nOwnerSequence))
            return false;

        /* Add our indexing entry by owner sequence number. */
        if(!Erase(std::make_tuple(std::string("transactions.index"), --nOwnerSequence, hashRegister)))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("transactions.sequence"), hashRegister), nOwnerSequence))
            return false;

        return true;
    }


    /* List the txide's that modified a register state for given genesis-id. */
    bool SessionDB::ListTransactions(const uint256_t& hashRegister, std::vector<uint512_t> &vTransactions)
    {
        /* Cache our txid and contract as a pair. */
        uint512_t hashTx;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_tuple(std::string("transactions.index"), nSequence, hashRegister), hashTx))
                break;

            /* Check for already executed contracts to omit. */
            vTransactions.push_back(hashTx);

            /* Increment our sequence number. */
            ++nSequence;
        }

        return !vTransactions.empty();
    }


    /* Push an register to process for given genesis-id. */
    bool SessionDB::PushRegister(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        /* Check for an active de-index. */
        if(HasDeindex(hashGenesis, hashRegister))
            return EraseDeindex(hashGenesis, hashRegister);

        /* Check for already existing order. */
        if(HasRegister(hashGenesis, hashRegister))
            return false;

        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("registers.sequence"), hashGenesis), nOwnerSequence);

        /* Add our indexing entry by owner sequence number. */
        if(!Write(std::make_tuple(std::string("registers.index"), nOwnerSequence, hashGenesis), hashRegister))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("registers.sequence"), hashGenesis), ++nOwnerSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_tuple(std::string("registers.proof"), hashGenesis, hashRegister)))
            return false;

        return true;
    }


    /* Erase an register for given genesis-id. */
    bool SessionDB::EraseRegister(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return WriteDeindex(hashGenesis, hashRegister);
    }


    /* List the current active registers for given genesis-id. */
    bool SessionDB::ListRegisters(const uint256_t& hashGenesis, std::set<TAO::Register::Address> &setAddresses, bool fTransferred)
    {
        /* Cache our txid and contract as a pair. */
        uint256_t hashRegister;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_tuple(std::string("registers.index"), nSequence++, hashGenesis), hashRegister))
                break;

            /* Check for transfer keys. */
            if(!fTransferred && HasTransfer(hashGenesis, hashRegister))
                continue; //NOTE: we skip over transfer keys

            /* Check for de-indexed keys. */
            if(HasDeindex(hashGenesis, hashRegister))
                continue; //NOTE: we skip over deindexed keys

            /* Check for already executed contracts to omit. */
            setAddresses.insert(hashRegister);
        }

        return !setAddresses.empty();
    }


    /* Push a tokenized register to process for given genesis-id. */
    bool SessionDB::PushTokenized(const uint256_t& hashGenesis, const std::pair<uint256_t, uint256_t>& pairTokenized)
    {
        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("tokenized.sequence"), hashGenesis), nOwnerSequence);

        /* Add our indexing entry by owner sequence number. */
        if(!Write(std::make_tuple(std::string("tokenized.index"), nOwnerSequence, hashGenesis), pairTokenized))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("tokenized.sequence"), hashGenesis), ++nOwnerSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_tuple(std::string("tokenized.proof"), hashGenesis, pairTokenized)))
            return false;

        return true;
    }


    /* Erase a tokenized register for given genesis-id. */
    bool SessionDB::EraseTokenized(const uint256_t& hashGenesis, const std::pair<uint256_t, uint256_t>& pairTokenized)
    {
        return WriteDeindex(hashGenesis, pairTokenized.first);
    }


    /* List current active tokenized registers for given genesis-id. */
    bool SessionDB::ListTokenized(const uint256_t& hashGenesis, std::vector<std::pair<uint256_t, uint256_t>> &vRegisters)
    {
        /* Cache our token and asset addresses as pair. */
        std::pair<uint256_t, uint256_t> pairTokenized;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_tuple(std::string("tokenized.index"), nSequence++, hashGenesis), pairTokenized))
                break;

            /* Check for transfer keys. */
            if(HasTransfer(hashGenesis, pairTokenized.first))
                continue; //NOTE: we skip over transfer keys

            /* Check for de-indexed keys. */
            if(HasDeindex(hashGenesis, pairTokenized.first))
                continue; //NOTE: we skip over deindexed keys

            /* Check for already executed contracts to omit. */
            vRegisters.push_back(pairTokenized);
        }

        return !vRegisters.empty();
    }


    /* Push an unclaimed address event to process for given genesis-id. */
    bool SessionDB::PushUnclaimed(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        /* Check for an active de-index. */
        if(HasDeindex(hashGenesis, hashRegister))
            return EraseDeindex(hashGenesis, hashRegister);

        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("unclaimed.sequence"), hashGenesis), nOwnerSequence);

        /* Add our indexing entry by owner sequence number. */
        if(!Write(std::make_tuple(std::string("unclaimed.index"), nOwnerSequence, hashGenesis), hashRegister))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("unclaimed.sequence"), hashGenesis), ++nOwnerSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_tuple(std::string("unclaimed.proof"), hashGenesis, hashRegister)))
            return false;

        return true;
    }


    /* Erase an unclaimed address event to process for given genesis-id. */
    bool SessionDB::EraseUnclaimed(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("unclaimed.sequence"), hashGenesis), nOwnerSequence);

        /* Add our indexing entry by owner sequence number. */
        uint256_t hashCheck;
        if(!Read(std::make_tuple(std::string("unclaimed.index"), nOwnerSequence, hashGenesis), hashCheck))
            return false;

        /* Check that the registers match. */
        if(hashCheck != hashRegister)
            return debug::error(FUNCTION, "erase must be called in reverse consecutive order");

        /* Erase our unclaimed by sequence. */
        if(!Erase(std::make_tuple(std::string("unclaimed.index"), nOwnerSequence, hashGenesis)))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("unclaimed.sequence"), hashGenesis), --nOwnerSequence))
            return false;

        /* Erase our order proof. */
        if(!Erase(std::make_tuple(std::string("unclaimed.proof"), hashGenesis, hashRegister)))
            return false;

        return true;
    }


    /* List the current unclaimed registers for given genesis-id. */
    bool SessionDB::ListUnclaimed(const uint256_t& hashGenesis, std::set<TAO::Register::Address> &setAddresses)
    {
        /* Cache our txid and contract as a pair. */
        uint256_t hashRegister;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_tuple(std::string("unclaimed.index"), nSequence++, hashGenesis), hashRegister))
                break;

            /* Check for already existing order. */
            if(HasRegister(hashGenesis, hashRegister))
                continue;

            /* Check for de-indexed keys. */
            if(HasDeindex(hashGenesis, hashRegister))
                continue; //NOTE: we skip over deindexed keys

            /* Check for already executed contracts to omit. */
            setAddresses.insert(hashRegister);

            /* Increment our sequence number. */
            //++nSequence;
        }

        return !setAddresses.empty();
    }


    /* Checks if a register has been indexed in the database already. */
    bool SessionDB::HasRegister(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Exists(std::make_tuple(std::string("registers.proof"), hashGenesis, hashRegister));
    }


    /* Writes a key that indicates a register was transferred from sigchain. */
    bool SessionDB::WriteDeindex(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Write(std::make_tuple(std::string("registers.deindex"), hashGenesis, hashRegister));
    }


    /* Checks a key that indicates a register was transferred from sigchain. */
    bool SessionDB::HasDeindex(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Exists(std::make_tuple(std::string("registers.deindex"), hashGenesis, hashRegister));
    }


    /* Erases a key that indicates a register was transferred from sigchain. */
    bool SessionDB::EraseDeindex(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Erase(std::make_tuple(std::string("registers.deindex"), hashGenesis, hashRegister));
    }


    /* Writes a key that indicates a register was transferred from sigchain. */
    bool SessionDB::WriteTransfer(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Write(std::make_tuple(std::string("registers.transfer"), hashGenesis, hashRegister));
    }


    /* Checks a key that indicates a register was transferred from sigchain. */
    bool SessionDB::HasTransfer(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Exists(std::make_tuple(std::string("registers.transfer"), hashGenesis, hashRegister));
    }


    /* Erases a key that indicates a register was transferred from sigchain. */
    bool SessionDB::EraseTransfer(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Erase(std::make_tuple(std::string("registers.transfer"), hashGenesis, hashRegister));
    }


    /* Push an event to process for given genesis-id. */
    bool SessionDB::PushEvent(const uint256_t& hashGenesis, const uint512_t& hashTx, const uint32_t nContract)
    {
        /* Check for already existing order. */
        if(HasEvent(hashTx, nContract))
            return false;

        /* Get our current sequence number. */
        uint32_t nSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("events.sequence"), hashGenesis), nSequence);

        /* Add our indexing entry by owner sequence number. */
        if(!Write(std::make_tuple(std::string("events.index"), nSequence, hashGenesis), std::make_pair(hashTx, nContract)))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("events.sequence"), hashGenesis), ++nSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_tuple(std::string("events.proof"), hashTx, nContract)))
            return false;

        return true;
    }


    /* Erase a contract for given genesis-id. */
    bool SessionDB::EraseEvent(const uint256_t& hashGenesis, const uint512_t& hashTx, const uint32_t nContract)
    {
        /* Check for already existing order. */
        if(!HasEvent(hashTx, nContract))
            return false;

        /* Get our current sequence number. */
        uint32_t nSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("events.sequence"), hashGenesis), nSequence);

        /* Add our indexing entry by owner sequence number. */
        std::pair<uint512_t, uint32_t> pairContract;
        if(!Read(std::make_tuple(std::string("events.index"), nSequence, hashGenesis), pairContract))
            return false;

        /* Check that this is correct contract (we need to erase in reverse order like a stack). */
        if(pairContract != std::make_pair(hashTx, nContract))
            return debug::error(FUNCTION, "cannot erase contract that is not at top of stack");

        /* Erase the contract from the stack now. */
        if(!Erase(std::make_tuple(std::string("events.index"), nSequence, hashGenesis)))
            return false;

        /* Write our order proof. */
        if(!Erase(std::make_tuple(std::string("events.proof"), hashTx, nContract)))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("events.sequence"), hashGenesis), --nSequence))
            return false;

        return true;
    }


    /* List the current active events for given genesis-id. */
    bool SessionDB::ListEvents(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vEvents, const int32_t nLimit)
    {
        /* Let's just keep this as a local static. */
        const static bool fForced =
            config::GetBoolArg("-forcesequence", false);

        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairEvent;

        /* Get our current events sequence. */
        uint32_t nSequence = 0;

        /* In case we want to force full check here. */
        if(!fForced)
            Read(std::make_pair(std::string("events.list.sequence"), hashGenesis), nSequence);

        /* Loop until we have failed. */
        uint32_t nTotal = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Check our limits. */
            if(nTotal >= nLimit && nLimit != -1 && !fForced)
            {
                /* Check for our verbose setting. */
                if(config::nVerbose >= 4)
                    debug::log(4, FUNCTION, "Listing ", VARIABLE(nTotal), " event contracts from ", VARIABLE(nSequence - nTotal));

                return fSuccess;
            }

            /* Read our current record. */
            if(!Read(std::make_tuple(std::string("events.index"), nSequence, hashGenesis), pairEvent))
                break;

            /* Check for already executed contracts to omit. */
            vEvents.push_back(pairEvent);

            /* Set our new sequence. */
            ++nSequence;
            ++nTotal;

            /* Set our success flag. */
            fSuccess = true;
        }

        /* Check for our verbose setting. */
        if(config::nVerbose >= 4)
            debug::log(4, FUNCTION, "Listing ", VARIABLE(nTotal), " event contracts from ", VARIABLE(nSequence - nTotal));

        return fSuccess;
    }


    /* Read the last event that was processed for given sigchain. */
    bool SessionDB::ReadTritiumSequence(const uint256_t& hashGenesis, uint32_t &nSequence)
    {
        return Read(std::make_pair(std::string("indexing.sequence"), hashGenesis), nSequence);
    }


    /* Write the last event that was processed for given sigchain. */
    bool SessionDB::IncrementTritiumSequence(const uint256_t& hashGenesis)
    {
        /* Read our current sequence. */
        uint32_t nSequence = 0;
        ReadTritiumSequence(hashGenesis, nSequence);

        return Write(std::make_pair(std::string("indexing.sequence"), hashGenesis), ++nSequence);
    }


    /* Write the last event that was processed for given sigchain. */
    bool SessionDB::DecrementTritiumSequence(const uint256_t& hashGenesis)
    {
        /* Read our current sequence. */
        uint32_t nSequence = 0;
        ReadTritiumSequence(hashGenesis, nSequence);

        return Write(std::make_pair(std::string("indexing.sequence"), hashGenesis), --nSequence);
    }


    /* Read the last event that was processed for given sigchain. */
    bool SessionDB::ReadLegacySequence(const uint256_t& hashGenesis, uint32_t &nSequence)
    {
        return Read(std::make_pair(std::string("indexing.sequence.legacy"), hashGenesis), nSequence);
    }


    /* Write the last event that was processed for given sigchain. */
    bool SessionDB::IncrementLegacySequence(const uint256_t& hashGenesis)
    {
        /* Read our current sequence. */
        uint32_t nSequence = 0;
        ReadLegacySequence(hashGenesis, nSequence);

        return Write(std::make_pair(std::string("indexing.sequence.legacy"), hashGenesis), ++nSequence);
    }


    /* Erase the last event that was processed for given sigchain. */
    bool SessionDB::DecrementLegacySequence(const uint256_t& hashGenesis)
    {
        /* Read our current sequence. */
        uint32_t nSequence = 0;
        ReadLegacySequence(hashGenesis, nSequence);

        return Write(std::make_pair(std::string("indexing.sequence.legacy"), hashGenesis), --nSequence);
    }


    /* Checks if an event has been indexed in the database already. */
    bool SessionDB::HasEvent(const uint512_t& hashTx, const uint32_t nContract)
    {
        return Exists(std::make_tuple(std::string("events.proof"), hashTx, nContract));
    }


    /* Push an contract to process for given genesis-id. */
    bool SessionDB::PushContract(const uint256_t& hashGenesis, const uint512_t& hashTx, const uint32_t nContract)
    {
        /* Check for already existing order. */
        if(HasContract(hashTx, nContract))
            return false;

        /* Get our current sequence number. */
        uint32_t nSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("contracts.sequence"), hashGenesis), nSequence);

        /* Add our indexing entry by owner sequence number. */
        if(!Write(std::make_tuple(std::string("contracts.index"), nSequence, hashGenesis), std::make_pair(hashTx, nContract)))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("contracts.sequence"), hashGenesis), ++nSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_tuple(std::string("contracts.proof"), hashTx, nContract)))
            return false;

        return true;
    }


    /* Erase a contract for given genesis-id. */
    bool SessionDB::EraseContract(const uint256_t& hashGenesis, const uint512_t& hashTx, const uint32_t nContract)
    {
        /* Check for already existing order. */
        if(!HasContract(hashTx, nContract))
            return false;

        /* Get our current sequence number. */
        uint32_t nSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("contracts.sequence"), hashGenesis), nSequence);

        /* Add our indexing entry by owner sequence number. */
        std::pair<uint512_t, uint32_t> pairContract;
        if(!Read(std::make_tuple(std::string("contracts.index"), nSequence, hashGenesis), pairContract))
            return false;

        /* Check that this is correct contract (we need to erase in reverse order like a stack). */
        if(pairContract != std::make_pair(hashTx, nContract))
            return debug::error(FUNCTION, "cannot erase contract that is not at top of stack");

        /* Erase the contract from the stack now. */
        if(!Erase(std::make_tuple(std::string("contracts.index"), nSequence, hashGenesis)))
            return false;

        /* Write our order proof. */
        if(!Erase(std::make_tuple(std::string("contracts.proof"), hashTx, nContract)))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("contracts.sequence"), hashGenesis), --nSequence))
            return false;

        return true;
    }


    /* Increment the last contract that was fully processed. */
    bool SessionDB::IncrementEventSequence(const uint256_t& hashGenesis)
    {
        /* Let's just keep this as a local static. */
        const static bool fForced =
            config::GetBoolArg("-forcesequence", false);

        /* We don't need to increment events when in forced mode. */
        if(fForced)
            return true;

        /* Read our current sequence. */
        uint32_t nSequence = 0;
        Read(std::make_pair(std::string("events.list.sequence"), hashGenesis), nSequence);

        return Write(std::make_pair(std::string("events.list.sequence"), hashGenesis), ++nSequence);
    }


    /* Increment the last contract that was fully processed. */
    bool SessionDB::IncrementContractSequence(const uint256_t& hashGenesis)
    {
        /* Let's just keep this as a local static. */
        const static bool fForced =
            config::GetBoolArg("-forcesequence", false);

        /* We don't need to increment events when in forced mode. */
        if(fForced)
            return true;

        /* Read our current sequence. */
        uint32_t nSequence = 0;
        Read(std::make_pair(std::string("contracts.list.sequence"), hashGenesis), nSequence);

        return Write(std::make_pair(std::string("contracts.list.sequence"), hashGenesis), ++nSequence);
    }


    /* List the current active contracts for given genesis-id. */
    bool SessionDB::ListContracts(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vContracts, const int32_t nLimit)
    {
        /* Let's just keep this as a local static. */
        const static bool fForced =
            config::GetBoolArg("-forcesequence", false);

        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairContract;

        /* Get our current events sequence. */
        uint32_t nSequence = 0;

        /* In case we want to force full check here. */
        if(!fForced)
            Read(std::make_pair(std::string("contracts.list.sequence"), hashGenesis), nSequence);

        /* Loop until we have failed. */
        uint32_t nTotal = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Check our limits. */
            if(nTotal >= nLimit && nLimit != -1 && !fForced)
            {
                /* Check for our verbose setting. */
                if(config::nVerbose >= 4)
                    debug::log(4, FUNCTION, "Listing ", VARIABLE(nTotal), " expiring contracts from ", VARIABLE(nSequence - nTotal));

                return fSuccess;
            }

            /* Read our current record. */
            if(!Read(std::make_tuple(std::string("contracts.index"), nSequence, hashGenesis), pairContract))
                break;

            /* Check for already executed contracts to omit. */
            vContracts.push_back(pairContract);

            /* Set our new sequence. */
            ++nSequence;
            ++nTotal;

            /* Set our success flag. */
            fSuccess = true;
        }

        /* Check for our verbose setting. */
        if(config::nVerbose >= 4)
            debug::log(4, FUNCTION, "Listing ", VARIABLE(nTotal), " expiring contracts from ", VARIABLE(nSequence - nTotal));

        return fSuccess;
    }


    /* Checks if an contract has been indexed in the database already. */
    bool SessionDB::HasContract(const uint512_t& hashTx, const uint32_t nContract)
    {
        return Exists(std::make_tuple(std::string("contracts.proof"), hashTx, nContract));
    }
}
