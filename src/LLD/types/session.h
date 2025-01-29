/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <LLD/templates/sector.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

namespace TAO::API       { class Transaction; }
namespace TAO::Operation { class Contract;    }

namespace LLD
{

   /** LocalDB
    *
    *  Database class for storing local wallet transactions.
    *
    **/
    class SessionDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
    public:

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        SessionDB(const uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE,
            const uint32_t nBucketsIn = 77773, const uint32_t nCacheIn = 1024 * 1024);


        /** Default Destructor **/
        virtual ~SessionDB();


        /** WriteAccess
         *
         *  Writes a session's access time to the database.
         *
         *  @param[in] hashGenesis The genesis of session.
         *  @param[in] nActive The timestamp this session was last active
         *
         *  @return true if written successfully
         *
         **/
        bool WriteAccess(const uint256_t& hashGenesis, const uint64_t nActive);


        /** ReadAccess
         *
         *  Reads a session's access time to the database.
         *
         *  @param[in] hashGenesis The genesis of session.
         *  @param[out] nActive The timestamp this session was last active
         *
         *  @return true if read successfully
         *
         **/
        bool ReadAccess(const uint256_t& hashGenesis, uint64_t &nActive);



        /** ListAccesses
         *
         *  Lists all the sessions that have been accessed on this node.
         *
         *  @param[out] mapSessions The list of active sessions for this node.
         *  @param[in] nTimeframe The maximum number of seconds since last access.
         *
         *  @return true if read successfully
         *
         **/
        bool ListAccesses(std::map<uint256_t, uint64_t> &mapSessions, const uint64_t nTimeframe = 0);


        /** WriteFirst
         *
         *  Writes a username - genesis hash pair to the local database.
         *
         *  @param[in] strUsername The username.
         *  @param[in] hashGenesis The genesis hash corresponding to this username.
         *
         *  @return True if the last was successfully written, false otherwise.
         *
         **/
        bool WriteFirst(const SecureString& strUsername, const uint256_t& hashGenesis);


        /** ReadFirst
         *
         *  Reads a genesis hash from the local database for a given username.
         *
         *  @param[in] strUsername The username to read the genesis hash for.
         *  @param[out] hashGenesis The genesis hash corresponding to this username.
         *
         *  @return True if the last was successfully read, false otherwise.
         *
         **/
        bool ReadFirst(const SecureString& strUsername, uint256_t &hashGenesis);


        /** WriteSession
         *
         *  Writes session data to the local database.
         *
         *  @param[in] hashGenesis The genesis hash of the user to save the session for.
         *  @param[in] vchData The session data to be saved.
         *
         *  @return True if the session was successfully written, false otherwise.
         *
         **/
        bool WriteSession(const uint256_t& hashGenesis, const std::vector<uint8_t>& vchData);


        /** ReadSession
         *
         *  Reads session data from the local database .
         *
         *  @param[in] nSession The genesis hash of the user to load the session for
         *  @param[out] vchData The session data to be loaded.
         *
         *  @return True if the session was successfully read, false otherwise.
         *
         **/
        bool ReadSession(const uint256_t& hashGenesis, std::vector<uint8_t>& vchData);


        /** EraseSession
         *
         *  Deletes session data from the local database fort he given session ID.
         *
         *  @param[in] hashGenesis The genesis hash of the user to erase the session for
         *
         *  @return True if the data was successfully deleted, false otherwise.
         *
         **/
        bool EraseSession(const uint256_t& hashGenesis);


        /** HasSession
         *
         *  Determines whether the local DB contains session data for the given session ID
         *
         *  @param[in] hashGenesis The genesis hash of the user to check
         *
         *  @return True if the session data exists, false otherwise.
         *
         **/
        bool HasSession(const uint256_t& hashGenesis);


        /** ReadLast
         *
         *  Reads the last txid that was indexed.
         *
         *  @param[in] hashGenesis The genesis-id we are reading last index for.
         *  @param[out] hashTx The txid of the last indexed transaction.
         *
         *  @return true if read successfully
         *
         **/
        bool ReadLast(const uint256_t& hashGenesis, uint512_t &hashTx);


        /** ReadLastConfirmed
         *
         *  Reads the last txid that was confirmed.
         *
         *  @param[in] hashGenesis The genesis-id we are reading last index for.
         *  @param[out] hashTx The txid of the last indexed transaction.
         *
         *  @return true if read successfully
         *
         **/
        bool ReadLastConfirmed(const uint256_t& hashGenesis, uint512_t &hashTx);


        /** WriteLast
         *
         *  Writes the last txid that was indexed.
         *
         *  @param[in] hashGenesis The genesis-id we are writing last index for.
         *  @param[out] hashTx The txid of the last indexed transaction.
         *
         *  @return true if read successfully
         *
         **/
        bool WriteLast(const uint256_t& hashGenesis, const uint512_t& hashTx);


        /** WriteFirst
         *
         *  Writes the first transaction-id to disk.
         *
         *  @param[in] hashGenesis The genesis ID to write for.
         *  @param[in] hashTx The transaction-id to write for.
         *
         *  @return True if the genesis is written, false otherwise.
         *
         **/
        bool WriteFirst(const uint256_t& hashGenesis, const uint512_t& hashTx);


        /** HasFirst
         *
         *  Checks if a given genesis-id has been indexed.
         *
         *  @param[in] hashGenesis The genesis ID to read for.
         *
         *  @return True if the genesis was read, false otherwise.
         *
         **/
        bool HasFirst(const uint256_t& hashGenesis);


        /** ReadFirst
         *
         *  Reads the first transaction-id from disk.
         *
         *  @param[in] hashGenesis The genesis ID to read for.
         *  @param[out] hashTx The transaction-id to read for.
         *
         *  @return True if the genesis was read, false otherwise.
         *
         **/
        bool ReadFirst(const uint256_t& hashGenesis, uint512_t& hashTx);


        /** ReadFirst
         *
         *  Reads the first transaction-id from disk.
         *
         *  @param[in] hashGenesis The genesis ID to read for.
         *  @param[out] tx The transaction object to read.
         *
         *  @return True if the genesis was read, false otherwise.
         *
         **/
        bool ReadFirst(const uint256_t& hashGenesis, TAO::API::Transaction &tx);


        /** EraseLast
         *
         *  Erases the last txid that was indexed.
         *
         *  @param[in] hashGenesis The genesis-id we are writing last index for.
         *
         *  @return true if read successfully
         *
         **/
        bool EraseLast(const uint256_t& hashGenesis);


        /** EraseFirst
         *
         *  Erases the first transaction-id to disk.
         *
         *  @param[in] hashGenesis The genesis ID to write for.
         *
         *  @return True if the genesis is written, false otherwise.
         *
         **/
        bool EraseFirst(const uint256_t& hashGenesis);


        /** WriteTx
         *
         *  Writes a transaction to the Logical DB.
         *
         *  @param[in] hashTx The txid of transaction to write.
         *  @param[in] tx The transaction object to write.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool WriteTx(const uint512_t& hashTx, const TAO::API::Transaction& tx);


        /** EraseTx
         *
         *  Erase a transaction from the Logical DB.
         *
         *  @param[in] hashTx The txid of transaction to erase.
         *
         *  @return True if the transaction was successfully erased, false otherwise.
         *
         **/
        bool EraseTx(const uint512_t& hashTx);


        /** HasTx
         *
         *  Checks if a transaction exists.
         *
         *  @param[in] hashTx The txid of transaction to check.
         *
         *  @return True if the transaction exists, false otherwise.
         *
         **/
        bool HasTx(const uint512_t& hashTx);


        /** ReadTx
         *
         *  Reads a transaction from the Logical DB.
         *
         *  @param[in] hashTx The txid of transaction to read.
         *  @param[out] tx The transaction object to read.
         *  @param[in] nFlags The flags to determine memory pool or disk
         *
         *  @return True if the transaction was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint512_t& hashTx, TAO::API::Transaction &tx);


        /** PushTransaction
         *
         *  Push an register transaction to process for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push register for.
         *  @param[in] hashRegister The address of register to push
         *  @param[in] hashTx The txid of the transaction that modified register.
         *
         *  @return true if event was pushed successfully.
         *
         **/
        bool PushTransaction(const uint256_t& hashRegister, const uint512_t& hashTx);


        /** EraseTransaction
         *
         *  Erase an register transaction for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push register for.
         *  @param[in] hashRegister The address of register to erase
         *
         *  @return true if event was pushed successfully.
         *
         **/
        bool EraseTransaction(const uint256_t& hashRegister);


        /** ListTransactions
         *
         *  List the txide's that modified a register state.
         *
         *  @param[in] hashRegister The address of register to list for
         *  @param[in] vTransactions The list of events extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListTransactions(const uint256_t& hashRegister, std::vector<uint512_t> &vTransactions);


        /** PushRegister
         *
         *  Push an register to process for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push register for.
         *  @param[in] hashRegister The address of register to push
         *
         *  @return true if event was pushed successfully.
         *
         **/
        bool PushRegister(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** EraseRegister
         *
         *  Erase an register for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push register for.
         *  @param[in] hashRegister The address of register to erase
         *
         *  @return true if event was pushed successfully.
         *
         **/
        bool EraseRegister(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** ListRegisters
         *
         *  List the current active registers for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to list registers for.
         *  @param[in] setAddresses The list of events extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListRegisters(const uint256_t& hashGenesis, std::set<TAO::Register::Address> &setAddresses, bool fTransferred = false);


        /** PushTokenized
         *
         *  Push a tokenized register to process for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push register for.
         *  @param[in] hashRegister The address of register to push
         *
         *  @return true if event was pushed successfully.
         *
         **/
        bool PushTokenized(const uint256_t& hashGenesis, const std::pair<uint256_t, uint256_t>& pairTokenized);


        /** EraseTokenized
         *
         *  Erase a tokenized register for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push register for.
         *  @param[in] hashRegister The address of register to erase
         *
         *  @return true if event was pushed successfully.
         *
         **/
        bool EraseTokenized(const uint256_t& hashGenesis, const std::pair<uint256_t, uint256_t>& pairTokenized);


        /** ListTokenized
         *
         *  List current active tokenized registers for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to list registers for.
         *  @param[in] vRegisters The list of events extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListTokenized(const uint256_t& hashGenesis, std::vector<std::pair<uint256_t, uint256_t>> &vRegisters);


        /** PushUnclaimed
         *
         *  Push an unclaimed address event to process for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push event for.
         *  @param[in] hashRegister The register address that is unclaimed
         *
         *  @return true if event was pushed successfully.
         *
         **/
        bool PushUnclaimed(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** EraseUnclaimed
         *
         *  Erase an unclaimed address event to process for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push event for.
         *  @param[in] hashRegister The register address that is unclaimed
         *
         *  @return true if event was pushed successfully.
         *
         **/
        bool EraseUnclaimed(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** ListUnclaimed
         *
         *  List the current unclaimed registers for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to list registers for.
         *  @param[in] setAddresses The list of events extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListUnclaimed(const uint256_t& hashGenesis, std::set<TAO::Register::Address> &setAddresses);


        /** HasRegister
         *
         *  Checks if a register has been indexed in the database already.
         *
         *  @param[in] hashTx The txid we are checking for.
         *  @param[in] nContract The contract-id we are checking for
         *
         *  @return true if db contains the valid event
         *
         **/
        bool HasRegister(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** WriteDeindex
         *
         *  Writes a key that indicates a register was deindexed by reorganize
         *
         *  @param[in] hashGenesis The genesis-id to list registers for.
         *  @param[in] hashRegister The register address that is being transferred.
         *
         *  @return true if db wrote new transfer correctly.
         *
         **/
        bool WriteDeindex(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** HasDeindex
         *
         *  Checks a key that indicates a register was deindexed by reorganize
         *
         *  @param[in] hashGenesis The genesis-id to list registers for.
         *  @param[in] hashRegister The register address that is being transferred.
         *
         *  @return true if db wrote new transfer correctly.
         *
         **/
        bool HasDeindex(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** EraseTransfer
         *
         *  Erases a key that indicates a register was deindexed by reorganize
         *
         *  @param[in] hashGenesis The genesis-id to list registers for.
         *  @param[in] hashRegister The register address that is being transferred.
         *
         *  @return true if db wrote new transfer correctly.
         *
         **/
        bool EraseDeindex(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** WriteTransfer
         *
         *  Writes a key that indicates a register was transferred from sigchain.
         *
         *  @param[in] hashGenesis The genesis-id to list registers for.
         *  @param[in] hashRegister The register address that is being transferred.
         *
         *  @return true if db wrote new transfer correctly.
         *
         **/
        bool WriteTransfer(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** HasTransfer
         *
         *  Checks a key that indicates a register was transferred from sigchain.
         *
         *  @param[in] hashGenesis The genesis-id to list registers for.
         *  @param[in] hashRegister The register address that is being transferred.
         *
         *  @return true if db wrote new transfer correctly.
         *
         **/
        bool HasTransfer(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** EraseTransfer
         *
         *  Erases a key that indicates a register was transferred from sigchain.
         *
         *  @param[in] hashGenesis The genesis-id to list registers for.
         *  @param[in] hashRegister The register address that is being transferred.
         *
         *  @return true if db wrote new transfer correctly.
         *
         **/
        bool EraseTransfer(const uint256_t& hashGenesis, const uint256_t& hashRegister);


        /** PushEvent
         *
         *  Push an event to process for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push event for.
         *  @param[in] hashTx The txid we are adding event for.
         *  @param[in] nContract The contract-id that contains the event
         *
         *  @return true if event was pushed successfully.
         *
         **/
        bool PushEvent(const uint256_t& hashGenesis, const uint512_t& hashTx, const uint32_t nContract);


        /** EraseEvent
         *
         *  Erase an event for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push event for.
         *  @param[in] hashTx The txid we are adding event for.
         *  @param[in] nContract The contract-id that contains the event
         *
         *  @return true if event was erased successfully.
         *
         **/
        bool EraseEvent(const uint256_t& hashGenesis, const uint512_t& hashTx, const uint32_t nContract);


        /** ListEvents
         *
         *  List the current active events for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to list events for.
         *  @param[in] vEvents The list of events extracted.
         *  @param[in] nLimit The maximum number of events to get.
         *
         *  @return true if written successfully
         *
         **/
        bool ListEvents(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vEvents, const int32_t nLimit = -1);


        /** ReadTritiumSequence
         *
         *  Read the last event that was processed for given sigchain.
         *
         *  @param[in] hashGenesis The genesis-id to check event for.
         *  @param[out] nSequence The last sequence that was written.
         *
         *  @return if the record was read successfully.
         *
         **/
        bool ReadTritiumSequence(const uint256_t& hashGenesis, uint32_t &nSequence);


        /** IncrementTritiumSequence
         *
         *  Write the last event that was processed for given sigchain.
         *
         *  @param[in] hashGenesis The genesis-id to check event for.
         *
         *  @return if the record was written successfully.
         *
         **/
        bool IncrementTritiumSequence(const uint256_t& hashGenesis);


        /** DecrementTritiumSequence
         *
         *  Remove the last event that was processed for given sigchain.
         *
         *  @param[in] hashGenesis The genesis-id to check event for.
         *
         *  @return if the record was written successfully.
         *
         **/
        bool DecrementTritiumSequence(const uint256_t& hashGenesis);


        /** ReadLegacySequence
         *
         *  Read the last event that was processed for given sigchain.
         *
         *  @param[in] hashGenesis The genesis-id to check event for.
         *  @param[out] nSequence The last sequence that was written.
         *
         *  @return if the record was read successfully.
         *
         **/
        bool ReadLegacySequence(const uint256_t& hashGenesis, uint32_t &nSequence);


        /** IncrementLegacySequence
         *
         *  Write the last event that was processed for given sigchain.
         *
         *  @param[in] hashGenesis The genesis-id to check event for.
         *
         *  @return if the record was written successfully.
         *
         **/
        bool IncrementLegacySequence(const uint256_t& hashGenesis);


        /** DecrementLegacySequence
         *
         *  Erase the last event that was processed for given sigchain.
         *
         *  @param[in] hashGenesis The genesis-id to check event for.
         *
         *  @return if the record was written successfully.
         *
         **/
        bool DecrementLegacySequence(const uint256_t& hashGenesis);


        /** HasEvent
         *
         *  Checks if an event has been indexed in the database already.
         *
         *  @param[in] hashTx The txid we are checking for.
         *  @param[in] nContract The contract-id we are checking for
         *
         *  @return true if db contains the valid event
         *
         **/
        bool HasEvent(const uint512_t& hashTx, const uint32_t nContract);


        /** PushContract
         *
         *  Push an contract to process for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push event for.
         *  @param[in] hashTx The txid we are adding event for.
         *  @param[in] nContract The contract-id that contains the event
         *
         *  @return true if event was pushed successfully.
         *
         **/
        bool PushContract(const uint256_t& hashGenesis, const uint512_t& hashTx, const uint32_t nContract);


        /** EraseContract
         *
         *  Erase a contract for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to push event for.
         *  @param[in] hashTx The txid we are adding event for.
         *  @param[in] nContract The contract-id that contains the event
         *
         *  @return true if event was erased successfully.
         *
         **/
        bool EraseContract(const uint256_t& hashGenesis, const uint512_t& hashTx, const uint32_t nContract);


        /** IncrementEventSequence
         *
         *  Increment the last event that was fully processed.
         *
         *  @param[in] hashGenesis The genesis-id to check event for.
         *
         *  @return if the record was written successfully.
         *
         **/
        bool IncrementEventSequence(const uint256_t& hashGenesis);


        /** IncrementContractSequence
         *
         *  Increment the last contract that was fully processed.
         *
         *  @param[in] hashGenesis The genesis-id to check event for.
         *
         *  @return if the record was written successfully.
         *
         **/
        bool IncrementContractSequence(const uint256_t& hashGenesis);



        /** ListContracts
         *
         *  List the current active contracts for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to list events for.
         *  @param[in] vEvents The list of events extracted.
         *  @param[in] nLimit The maximum number of contracts to get.
         *
         *  @return true if written successfully
         *
         **/
        bool ListContracts(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vContracts, const int32_t nLimit = -1);


        /** HasEvent
         *
         *  Checks if an contract has been indexed in the database already.
         *
         *  @param[in] hashTx The txid we are checking for.
         *  @param[in] nContract The contract-id we are checking for
         *
         *  @return true if db contains the valid event
         *
         **/
        bool HasContract(const uint512_t& hashTx, const uint32_t nContract);

    };
}
