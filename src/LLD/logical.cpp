/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
    LogicalDB::LogicalDB(const uint8_t nFlagsIn, const uint32_t nBucketsIn, const uint32_t nCacheIn)
    : SectorDatabase(std::string("_API")
    , nFlagsIn
    , nBucketsIn
    , nCacheIn)
    {
    }


    /** Default Destructor **/
    LogicalDB::~LogicalDB()
    {
    }


    /* Writes a session's access time to the database. */
    bool LogicalDB::WriteSession(const uint256_t& hashGenesis, const uint64_t nActive)
    {
        return Write(std::make_pair(std::string("access"), hashGenesis), nActive);
    }


    /* Reads a session's access time to the database. */
    bool LogicalDB::ReadSession(const uint256_t& hashGenesis, uint64_t &nActive)
    {
        return Read(std::make_pair(std::string("access"), hashGenesis), nActive);
    }


    /* Reads the last txid that was indexed. */
    bool LogicalDB::ReadLastIndex(uint512_t &hashTx)
    {
        return Read(std::string("indexing"), hashTx);
    }


    /* Writes the last txid that was indexed. */
    bool LogicalDB::WriteLastIndex(const uint512_t& hashTx)
    {
        return Write(std::string("indexing"), hashTx);
    }


    /* Reads the last txid that was indexed. */
    bool LogicalDB::ReadLast(const uint256_t& hashGenesis, uint512_t &hashTx)
    {
        return Read(std::make_pair(std::string("indexing.last"), hashGenesis), hashTx);
    }


    /* Writes the last txid that was indexed. */
    bool LogicalDB::WriteLast(const uint256_t& hashGenesis, const uint512_t& hashTx)
    {
        return Write(std::make_pair(std::string("indexing.last"), hashGenesis), hashTx);
    }

    /* Writes the first transaction-id to disk. */
    bool LogicalDB::WriteFirst(const uint256_t& hashGenesis, const uint512_t& hashTx)
    {
        return Write(std::make_pair(std::string("indexing.first"), hashGenesis), hashTx);
    }


    /* Checks if a given genesis-id has been indexed. */
    bool LogicalDB::HasFirst(const uint256_t& hashGenesis)
    {
        return Exists(std::make_pair(std::string("indexing.first"), hashGenesis));
    }


    /* Reads the first transaction-id from disk. */
    bool LogicalDB::ReadFirst(const uint256_t& hashGenesis, uint512_t& hashTx)
    {
        return Read(std::make_pair(std::string("indexing.first"), hashGenesis), hashTx);
    }


    /* Writes the last txid that was indexed. */
    bool LogicalDB::EraseLast(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("indexing.last"), hashGenesis));
    }


    /* Writes the first transaction-id to disk. */
    bool LogicalDB::EraseFirst(const uint256_t& hashGenesis)
    {
        return Erase(std::make_pair(std::string("indexing.first"), hashGenesis));
    }


    /* Writes a transaction to the Logical DB. */
    bool LogicalDB::WriteTx(const uint512_t& hashTx, const TAO::API::Transaction& tx)
    {
        return Write(std::make_pair(std::string("tx"), hashTx), tx);
    }


    /* Erase a transaction from the Logical DB. */
    bool LogicalDB::EraseTx(const uint512_t& hashTx)
    {
        return Erase(std::make_pair(std::string("tx"), hashTx));
    }


    /* Reads a transaction from the Logical DB. */
    bool LogicalDB::ReadTx(const uint512_t& hashTx, TAO::API::Transaction &tx)
    {
        return Read(std::make_pair(std::string("tx"), hashTx), tx);
    }


    /* Push an register transaction to process for given genesis-id. */
    bool LogicalDB::PushTransaction(const uint256_t& hashRegister, const uint512_t& hashTx)
    {
        /* Start an ACID transaction for this set of records. */
        TxnBegin();

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

        return TxnCommit();
    }


    /* Erase an register transaction for given genesis-id. */
    bool LogicalDB::EraseTransaction(const uint256_t& hashRegister)
    {
        /* Start an ACID transaction for this set of records. */
        TxnBegin();

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

        return TxnCommit();
    }


    /* List the txide's that modified a register state for given genesis-id. */
    bool LogicalDB::ListTransactions(const uint256_t& hashRegister, std::vector<uint512_t> &vTransactions)
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


    /* Push an register transaction to process for given register address. */
    bool LogicalDB::PushRegisterTx(const uint256_t& hashRegister, const uint512_t& hashTx)
    {
        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("register.tx.sequence"), hashRegister), nOwnerSequence);

        /* Start an ACID transaction for this set of records. */
        TxnBegin();

        /* Add our indexing entry by owner sequence number. */
        if(!Write(std::make_tuple(std::string("register.tx.index"), (nOwnerSequence % 5), hashRegister), hashTx))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("register.tx.sequence"), hashRegister), ++nOwnerSequence))
            return false;

        return TxnCommit();
    }


    /* Erase an register transaction for given register address. */
    bool LogicalDB::EraseRegisterTx(const uint256_t& hashRegister)
    {
        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        if(!Read(std::make_pair(std::string("register.tx.sequence"), hashRegister), nOwnerSequence))
            return false;

        /* Start an ACID transaction for this set of records. */
        TxnBegin();

        /* Add our indexing entry by owner sequence number. */
        if(!Erase(std::make_tuple(std::string("register.tx.index"), (--nOwnerSequence % 5), hashRegister)))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("register.tx.sequence"), hashRegister), nOwnerSequence))
            return false;

        return TxnCommit();
    }


    /* Get an register transaction for given register address. */
    bool LogicalDB::LastRegisterTx(const uint256_t& hashRegister, uint512_t &hashTx)
    {
        /* Get our current sequence number. */
        uint32_t nOwnerSequence = 0;

        /* Read our sequences from disk. */
        if(!Read(std::make_pair(std::string("register.tx.sequence"), hashRegister), nOwnerSequence))
            return false;

        /* Add our indexing entry by owner sequence number. */
        if(!Read(std::make_tuple(std::string("register.tx.index"), (--nOwnerSequence % 5), hashRegister), hashTx))
            return false;

        return true;
    }


    /* Build indexes for transactions over a rolling modulus. For -indexregister flag. */
    void LogicalDB::IndexRegisters()
    {
        /* Not allowed in -client mode. */
        if(config::fClient.load())
            return;

        /* Check for address indexing flag. */
        if(Exists(std::string("register.indexed")))
        {
            /* Check there is no argument supplied. */
            if(!config::HasArg("-indexregister"))
            {
                /* Warn that -indexheight is persistent. */
                debug::notice(FUNCTION, "-indexregister enabled from valid indexes");

                /* Set indexing argument now. */
                RECURSIVE(config::ARGS_MUTEX);
                config::mapArgs["-indexregister"] = "1";
            }

            return;
        }

        /* Check there is no argument supplied. */
        if(!config::GetBoolArg("-indexregister"))
            return;

        /* Start a timer to track. */
        runtime::timer timer;
        timer.Start();

        /* Get our starting hash. */
        const uint1024_t hashBegin =
            TAO::Ledger::ChainState::Genesis();

        /* Read the first tritium block. */
        TAO::Ledger::BlockState state;
        if(!LLD::Ledger->ReadBlock(hashBegin, state))
        {
            debug::warning(FUNCTION, "No tritium blocks available ", hashBegin.SubString());
            return;
        }

        /* Keep track of our total count. */
        uint32_t nScannedCount = 0;

        /* Start our scan. */
        debug::notice(FUNCTION, "Building register indexes from block ", state.GetHash().SubString());
        while(!config::fShutdown.load())
        {
            /* Loop through found transactions. */
            for(uint32_t nIndex = 0; nIndex < state.vtx.size(); ++nIndex)
            {
                /* We only care about tritium transactions. */
                if(state.vtx[nIndex].first != TAO::Ledger::TRANSACTION::TRITIUM)
                    continue;

                /* Get a reference of our txid. */
                const uint512_t& hashTx =
                    state.vtx[nIndex].second;

                /* Read the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashTx, tx))
                    continue;

                /* Iterate the transaction contracts. */
                std::set<uint256_t> setAddresses;
                for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                {
                    /* Grab contract reference. */
                    const TAO::Operation::Contract& rContract = tx[nContract];

                    /* Unpack the address we will be working on. */
                    uint256_t hashAddress;
                    if(!TAO::Register::Unpack(rContract, hashAddress))
                        continue;

                    /* Check for duplicate entries. */
                    if(setAddresses.count(hashAddress))
                        continue;

                    /* Check fo register in database. */
                    PushRegisterTx(hashAddress, hashTx);

                    /* Push the address now. */
                    setAddresses.insert(hashAddress);
                }

                /* Update the scanned count for meters. */
                ++nScannedCount;

                /* Meter for output. */
                if(nScannedCount % 100000 == 0)
                {
                    /* Get the time it took to rescan. */
                    uint32_t nElapsedSeconds = timer.Elapsed();
                    debug::log(0, FUNCTION, "Built ", nScannedCount, " register indexes in ", nElapsedSeconds, " seconds (",
                        std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");
                }
            }

            /* Iterate to the next block in the queue. */
            state = state.Next();
            if(!state)
                break;
        }

        /* Write our last index now. */
        Write(std::string("register.indexed"));

        debug::notice(FUNCTION, "Complated scanning ", nScannedCount, " in ", timer.Elapsed(), " seconds");
    }


    /* Push an register to process for given genesis-id. */
    bool LogicalDB::PushRegister(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        /* Start an ACID transaction for this set of records. */
        TxnBegin();

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

        return TxnCommit();
    }


    /* Erase an register for given genesis-id. */
    bool LogicalDB::EraseRegister(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return WriteDeindex(hashGenesis, hashRegister);
    }


    /* List the current active registers for given genesis-id. */
    bool LogicalDB::ListRegisters(const uint256_t& hashGenesis, std::vector<TAO::Register::Address> &vRegisters)
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
            if(HasTransfer(hashGenesis, hashRegister))
                continue; //NOTE: we skip over transfer keys

            /* Check for de-indexed keys. */
            if(HasDeindex(hashGenesis, hashRegister))
                continue; //NOTE: we skip over deindexed keys

            /* Check for already executed contracts to omit. */
            vRegisters.push_back(hashRegister);

            /* Increment our sequence number. */
            //++nSequence;
        }

        return !vRegisters.empty();
    }


    /* Push a tokenized register to process for given genesis-id. */
    bool LogicalDB::PushTokenized(const uint256_t& hashGenesis, const std::pair<uint256_t, uint256_t>& pairTokenized)
    {
        /* Start an ACID transaction for this set of records. */
        TxnBegin();

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

        return TxnCommit();
    }


    /* Erase a tokenized register for given genesis-id. */
    bool LogicalDB::EraseTokenized(const uint256_t& hashGenesis, const std::pair<uint256_t, uint256_t>& pairTokenized)
    {
        return WriteDeindex(hashGenesis, pairTokenized.first);
    }


    /* List current active tokenized registers for given genesis-id. */
    bool LogicalDB::ListTokenized(const uint256_t& hashGenesis, std::vector<std::pair<uint256_t, uint256_t>> &vRegisters)
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
    bool LogicalDB::PushUnclaimed(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        /* Start an ACID transaction for this set of records. */
        TxnBegin();

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

        return TxnCommit();
    }


    /* List the current unclaimed registers for given genesis-id. */
    bool LogicalDB::ListUnclaimed(const uint256_t& hashGenesis, std::vector<TAO::Register::Address> &vRegisters)
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
            vRegisters.push_back(hashRegister);

            /* Increment our sequence number. */
            //++nSequence;
        }

        return !vRegisters.empty();
    }


    /* Checks if a register has been indexed in the database already. */
    bool LogicalDB::HasRegister(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Exists(std::make_tuple(std::string("registers.proof"), hashGenesis, hashRegister));
    }


    /* Writes a key that indicates a register was transferred from sigchain. */
    bool LogicalDB::WriteDeindex(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Write(std::make_tuple(std::string("registers.deindex"), hashGenesis, hashRegister));
    }


    /* Checks a key that indicates a register was transferred from sigchain. */
    bool LogicalDB::HasDeindex(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Exists(std::make_tuple(std::string("registers.deindex"), hashGenesis, hashRegister));
    }


    /* Erases a key that indicates a register was transferred from sigchain. */
    bool LogicalDB::EraseDeindex(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Erase(std::make_tuple(std::string("registers.deindex"), hashGenesis, hashRegister));
    }


    /* Writes a key that indicates a register was transferred from sigchain. */
    bool LogicalDB::WriteTransfer(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Write(std::make_tuple(std::string("registers.transfer"), hashGenesis, hashRegister));
    }


    /* Checks a key that indicates a register was transferred from sigchain. */
    bool LogicalDB::HasTransfer(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Exists(std::make_tuple(std::string("registers.transfer"), hashGenesis, hashRegister));
    }


    /* Erases a key that indicates a register was transferred from sigchain. */
    bool LogicalDB::EraseTransfer(const uint256_t& hashGenesis, const uint256_t& hashRegister)
    {
        return Erase(std::make_tuple(std::string("registers.transfer"), hashGenesis, hashRegister));
    }


    /* Push an event to process for given genesis-id. */
    bool LogicalDB::PushEvent(const uint256_t& hashGenesis, const uint512_t& hashTx, const uint32_t nContract)
    {
        /* Check for already existing order. */
        if(HasEvent(hashTx, nContract))
            return false;

        /* Get our current sequence number. */
        uint32_t nSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("events.sequence"), hashGenesis), nSequence);

        /* Start an ACID transaction for this set of records. */
        TxnBegin();

        /* Add our indexing entry by owner sequence number. */
        if(!Write(std::make_tuple(std::string("events.index"), nSequence, hashGenesis), std::make_pair(hashTx, nContract)))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("events.sequence"), hashGenesis), ++nSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_tuple(std::string("events.proof"), hashTx, nContract)))
            return false;

        return TxnCommit();
    }


    /* List the current active events for given genesis-id. */
    bool LogicalDB::ListEvents(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vEvents)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairEvent;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_tuple(std::string("events.index"), nSequence, hashGenesis), pairEvent))
                break;

            /* Check for already executed contracts to omit. */
            vEvents.push_back(pairEvent);

            /* Set our new sequence. */
            ++nSequence;
            fSuccess = true;
        }

        return fSuccess;
    }


    /* Read the last event that was processed for given sigchain. */
    bool LogicalDB::ReadTritiumSequence(const uint256_t& hashGenesis, uint32_t &nSequence)
    {
        return Read(std::make_pair(std::string("indexing.sequence"), hashGenesis), nSequence);
    }


    /* Write the last event that was processed for given sigchain. */
    bool LogicalDB::IncrementTritiumSequence(const uint256_t& hashGenesis)
    {
        /* Read our current sequence. */
        uint32_t nSequence = 0;
        ReadTritiumSequence(hashGenesis, nSequence);

        return Write(std::make_pair(std::string("indexing.sequence"), hashGenesis), ++nSequence);
    }


    /* Read the last event that was processed for given sigchain. */
    bool LogicalDB::ReadLegacySequence(const uint256_t& hashGenesis, uint32_t &nSequence)
    {
        return Read(std::make_pair(std::string("indexing.sequence.legacy"), hashGenesis), nSequence);
    }


    /* Write the last event that was processed for given sigchain. */
    bool LogicalDB::IncrementLegacySequence(const uint256_t& hashGenesis)
    {
        /* Read our current sequence. */
        uint32_t nSequence = 0;
        ReadLegacySequence(hashGenesis, nSequence);

        return Write(std::make_pair(std::string("indexing.sequence.legacy"), hashGenesis), ++nSequence);
    }


    /* Checks if an event has been indexed in the database already. */
    bool LogicalDB::HasEvent(const uint512_t& hashTx, const uint32_t nContract)
    {
        return Exists(std::make_tuple(std::string("events.proof"), hashTx, nContract));
    }


    /* Push an contract to process for given genesis-id. */
    bool LogicalDB::PushContract(const uint256_t& hashGenesis, const uint512_t& hashTx, const uint32_t nContract)
    {
        /* Check for already existing order. */
        if(HasContract(hashTx, nContract))
            return false;

        /* Get our current sequence number. */
        uint32_t nSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("contracts.sequence"), hashGenesis), nSequence);

        /* Start an ACID transaction for this set of records. */
        TxnBegin();

        /* Add our indexing entry by owner sequence number. */
        if(!Write(std::make_tuple(std::string("contracts.index"), nSequence, hashGenesis), std::make_pair(hashTx, nContract)))
            return false;

        /* Write our new events sequence to disk. */
        if(!Write(std::make_pair(std::string("contracts.sequence"), hashGenesis), ++nSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_tuple(std::string("contracts.proof"), hashTx, nContract)))
            return false;

        return TxnCommit();
    }


    /* List the current active contracts for given genesis-id. */
    bool LogicalDB::ListContracts(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vContracts)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairContract;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_tuple(std::string("contracts.index"), nSequence, hashGenesis), pairContract))
                break;

            /* Check for already executed contracts to omit. */
            vContracts.push_back(pairContract);

            /* Set our new sequence. */
            ++nSequence;
            fSuccess = true;
        }

        return fSuccess;
    }


    /* Checks if an contract has been indexed in the database already. */
    bool LogicalDB::HasContract(const uint512_t& hashTx, const uint32_t nContract)
    {
        return Exists(std::make_tuple(std::string("contracts.proof"), hashTx, nContract));
    }


    /* Pushes an order to the orderbook stack. */
    bool LogicalDB::PushOrder(const std::pair<uint256_t, uint256_t>& pairMarket,
                              const TAO::Operation::Contract& rContract, const uint32_t nContract)
    {
        /* Grab a refernece of our txid. */
        const uint512_t& hashTx =
            rContract.Hash();

        /* Grab a reference of our caller. */
        const uint256_t& hashOwner =
            rContract.Caller();

        /* Check for already existing order. */
        if(HasOrder(hashTx, nContract))
            return false;

        /* Get our current sequence number. */
        uint32_t nMarketSequence = 0, nOwnerSequence = 0;

        /* Read our sequences from disk. */
        Read(std::make_pair(std::string("market.sequence"), pairMarket), nMarketSequence);
        Read(std::make_pair(std::string("owner.sequence"),  hashOwner),   nOwnerSequence);

        /* Start an ACID transaction for this set of records. */
        TxnBegin();

        /* Write our order by sequence number. */
        if(!Write(std::make_pair(nMarketSequence, pairMarket), std::make_pair(hashTx, nContract)))
            return false;

        /* Add an additional indexing entry for owner level orders. */
        if(!Index(std::make_pair(nOwnerSequence, hashOwner), std::make_pair(nMarketSequence, pairMarket)))
            return false;

        /* Write our new market sequence to disk. */
        if(!Write(std::make_pair(std::string("market.sequence"), pairMarket), ++nMarketSequence))
            return false;

        /* Write our new owner sequence to disk. */
        if(!Write(std::make_pair(std::string("owner.sequence"), hashOwner), ++nOwnerSequence))
            return false;

        /* Write our order proof. */
        if(!Write(std::make_pair(hashTx, nContract)))
            return false;

        return TxnCommit();
    }


    /* Pulls a list of orders from the orderbook stack. */
    bool LogicalDB::ListOrders(const std::pair<uint256_t, uint256_t>& pairMarket, std::vector<std::pair<uint512_t, uint32_t>> &vOrders)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, pairMarket), pairOrder))
                break;

            /* Check for already executed contracts to omit. */
            if(!LLD::Contract->HasContract(pairOrder, TAO::Ledger::FLAGS::MEMPOOL))
            {
                vOrders.push_back(pairOrder);
                fSuccess = true;
            }

            /* Set our new sequence. */
            ++nSequence;
        }

        return fSuccess;
    }


    /* List the current active orders for given user's sigchain. */
    bool LogicalDB::ListOrders(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vOrders)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, hashGenesis), pairOrder))
                break;

            /* Check for already executed contracts to omit. */
            if(!LLD::Contract->HasContract(pairOrder, TAO::Ledger::FLAGS::MEMPOOL))
            {
                vOrders.push_back(pairOrder);
                fSuccess = true;
            }

            /* Increment our sequence number. */
            ++nSequence;
        }

        return fSuccess;
    }

    /* List the current active orders for given market pair. */
    bool LogicalDB::ListAllOrders(const std::pair<uint256_t, uint256_t>& pairMarket, std::vector<std::pair<uint512_t, uint32_t>> &vOrders)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, pairMarket), pairOrder))
                break;

            /* Push order ot our list. */
            vOrders.push_back(pairOrder);

            /* Set our new sequence. */
            ++nSequence;
            fSuccess = true;
        }

        return fSuccess;
    }


    /* List the current active orders for given user's sigchain. */
    bool LogicalDB::ListAllOrders(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vOrders)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, hashGenesis), pairOrder))
                break;

            /* Push order ot our list. */
            vOrders.push_back(pairOrder);

            /* Set our new sequence. */
            ++nSequence;
            fSuccess = true;
        }

        return fSuccess;
    }


    /* List the current completed orders for given user's sigchain. */
    bool LogicalDB::ListExecuted(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vExecuted)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, hashGenesis), pairOrder))
                break;

            /* Check for already executed contracts to omit. */
            if(LLD::Contract->HasContract(pairOrder, TAO::Ledger::FLAGS::MEMPOOL))
            {
                vExecuted.push_back(pairOrder);
                fSuccess = true;
            }

            /* Increment our sequence number. */
            ++nSequence;
        }

        return fSuccess;
    }


    /*  List the current completed orders for given market pair. */
    bool LogicalDB::ListExecuted(const std::pair<uint256_t, uint256_t>& pairMarket, std::vector<std::pair<uint512_t, uint32_t>> &vExecuted)
    {
        /* Track our return success. */
        bool fSuccess = false;

        /* Cache our txid and contract as a pair. */
        std::pair<uint512_t, uint32_t> pairOrder;

        /* Loop until we have failed. */
        uint32_t nSequence = 0;
        while(!config::fShutdown.load()) //we want to early terminate on shutdown
        {
            /* Read our current record. */
            if(!Read(std::make_pair(nSequence, pairMarket), pairOrder))
                break;

            /* Check for already executed contracts to omit. */
            if(LLD::Contract->HasContract(pairOrder, TAO::Ledger::FLAGS::MEMPOOL))
            {
                vExecuted.push_back(pairOrder);
                fSuccess = true;
            }

            /* Increment our sequence number. */
            ++nSequence;
        }

        return fSuccess;
    }


    /* Checks if an order has been indexed in the database already. */
    bool LogicalDB::HasOrder(const uint512_t& hashTx, const uint32_t nContract)
    {
        return Exists(std::make_pair(hashTx, nContract));
    }


    /* Writes a register address PTR mapping from address to name address */
    bool LogicalDB::WritePTR(const uint256_t& hashAddress, const uint256_t& hashName)
    {
        return Write(std::make_pair(std::string("ptr"), hashAddress), hashName);
    }


    /* Reads a register address PTR mapping from address to name address */
    bool LogicalDB::ReadPTR(const uint256_t& hashAddress, uint256_t &hashName)
    {
        return Read(std::make_pair(std::string("ptr"), hashAddress), hashName);
    }


    /* Erases a register address PTR mapping */
    bool LogicalDB::ErasePTR(const uint256_t& hashAddress)
    {
        return Erase(std::make_pair(std::string("ptr"), hashAddress));
    }

    /* Checks if a register address has a PTR mapping */
    bool LogicalDB::HasPTR(const uint256_t& hashAddress)
    {
        return Exists(std::make_pair(std::string("ptr"), hashAddress));
    }
}
