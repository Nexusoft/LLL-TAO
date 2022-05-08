/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
    class LogicalDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
    public:

        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LogicalDB(const uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE,
            const uint32_t nBucketsIn = 77773, const uint32_t nCacheIn = 1024 * 1024);


        /** Default Destructor **/
        virtual ~LogicalDB();


        /** WriteSession
         *
         *  Writes a session's access time to the database.
         *
         *  @param[in] hashGenesis The genesis of session.
         *  @param[in] nActive The timestamp this session was last active
         *
         *  @return true if written successfully
         *
         **/
        bool WriteSession(const uint256_t& hashGenesis, const uint64_t nActive);


        /** ReadSession
         *
         *  Reads a session's access time to the database.
         *
         *  @param[in] hashGenesis The genesis of session.
         *  @param[out] nActive The timestamp this session was last active
         *
         *  @return true if read successfully
         *
         **/
        bool ReadSession(const uint256_t& hashGenesis, uint64_t &nActive);


        /** ReadLastIndex
         *
         *  Reads the last txid that was indexed.
         *
         *  @param[out] hashTx The txid of the last indexed transaction.
         *
         *  @return true if read successfully
         *
         **/
        bool ReadLastIndex(uint512_t &hashTx);


        /** WriteLastIndex
         *
         *  Writes the last txid that was indexed.
         *
         *  @param[out] hashTx The txid of the last indexed transaction.
         *
         *  @return true if read successfully
         *
         **/
        bool WriteLastIndex(const uint512_t& hashTx);


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
         *  @param[in] vRegisters The list of events extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListRegisters(const uint256_t& hashGenesis, std::vector<TAO::Register::Address> &vRegisters);


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


        /** ListUnclaimed
         *
         *  List the current unclaimed registers for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to list registers for.
         *  @param[in] vRegisters The list of events extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListUnclaimed(const uint256_t& hashGenesis, std::vector<TAO::Register::Address> &vRegisters);


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


        /** ListEvents
         *
         *  List the current active events for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to list events for.
         *  @param[in] vEvents The list of events extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListEvents(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vEvents);


        /** ReadLastEvent
         *
         *  Read the last event that was processed for given sigchain.
         *
         *  @param[in] hashGenesis The genesis-id to check event for.
         *  @param[out] nSequence The last sequence that was written.
         *
         *  @return if the record was read successfully.
         *
         **/
        bool ReadLastEvent(const uint256_t& hashGenesis, uint32_t &nSequence);


        /** IncrementLastEvent
         *
         *  Write the last event that was processed for given sigchain.
         *
         *  @param[in] hashGenesis The genesis-id to check event for.
         *
         *  @return if the record was written successfully.
         *
         **/
        bool IncrementLastEvent(const uint256_t& hashGenesis);


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


        /** PushEvent
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


        /** ListEvents
         *
         *  List the current active contracts for given genesis-id.
         *
         *  @param[in] hashGenesis The genesis-id to list events for.
         *  @param[in] vEvents The list of events extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListContracts(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vContracts);


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


        /** PushOrder
         *
         *  Pushes an order to the orderbook stack.
         *
         *  @param[in] pairMarket The market-pair of token-id's
         *  @param[in] rContract The contract that contains the existing order.
         *  @param[in] nContract The contract-id that contains the order
         *
         *  @return true if written successfully
         *
         **/
        bool PushOrder(const std::pair<uint256_t, uint256_t>& pairMarket,
                       const TAO::Operation::Contract& rContract, const uint32_t nContract);


        /** ListOrders
         *
         *  List the current active orders for given market pair.
         *
         *  @param[in] pairMarket The market-pair of token-id's
         *  @param[in] vOrders The list of orders extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListOrders(const std::pair<uint256_t, uint256_t>& pairMarket, std::vector<std::pair<uint512_t, uint32_t>> &vOrders);


        /** ListOrders
         *
         *  List the current active orders for given user's sigchain.
         *
         *  @param[in] hashGenesis The current genesis-id to grab orders by.
         *  @param[in] vOrders The list of orders extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListOrders(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vOrders);


        /** ListAllOrders
         *
         *  List the current active orders for given market pair.
         *
         *  @param[in] pairMarket The market-pair of token-id's
         *  @param[in] vOrders The list of orders extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListAllOrders(const std::pair<uint256_t, uint256_t>& pairMarket, std::vector<std::pair<uint512_t, uint32_t>> &vOrders);


        /** ListAllOrders
         *
         *  List the current active orders for given user's sigchain.
         *
         *  @param[in] hashGenesis The current genesis-id to grab orders by.
         *  @param[in] vOrders The list of orders extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListAllOrders(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vOrders);


        /** ListExecuted
         *
         *  List the current completed orders for given user's sigchain.
         *
         *  @param[in] hashGenesis The current genesis-id to grab orders by.
         *  @param[in] vOrders The list of orders extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool ListExecuted(const uint256_t& hashGenesis, std::vector<std::pair<uint512_t, uint32_t>> &vExecuted);


        /** ListExecuted
         *
         *  List the current completed orders for given market pair.
         *
         *  @param[in] pairMarket The market-pair of token-id's
         *  @param[in] vExecuted The list of orders executed.
         *
         *  @return true if written successfully
         *
         **/
        bool ListExecuted(const std::pair<uint256_t, uint256_t>& pairMarket, std::vector<std::pair<uint512_t, uint32_t>> &vExecuted);


        /** HasOrder
         *
         *  Checks if an order has been indexed in the database already.
         *
         *  @param[in] hashAddress The address we are mapping.
         *
         *  @return true if db contains ptr record.
         *
         **/
        bool HasOrder(const uint512_t& hashTx, const uint32_t nContract);


        /** WritePTR
         *
         *  Writes a register address PTR mapping from address to name address
         *
         *  @param[in] hashAddress The address we are mapping.
         *  @param[in] hashName The name record we are mapping to.
         *
         *  @return true if written successfully
         *
         **/
        bool WritePTR(const uint256_t& hashAddress, const uint256_t& hashName);


        /** ReadPTR
         *
         *  Reads a register address PTR mapping from address to name address
         *
         *  @param[in] hashAddress The address we are mapping.
         *  @param[in] hashName The name record we are mapping to.
         *
         *  @return true if read successfully
         *
         **/
        bool ReadPTR(const uint256_t& hashAddress, uint256_t &hashName);


        /** ErasePTR
         *
         *  Erases a register address PTR mapping
         *
         *  @param[in] hashAddress The address we are mapping.
         *
         *  @return true if item erased
         *
         **/
        bool ErasePTR(const uint256_t& hashAddress);


        /** HasPTR
         *
         *  Checks if a register address has a PTR mapping
         *
         *  @param[in] hashAddress The address we are mapping.
         *
         *  @return true if db contains ptr record.
         *
         **/
        bool HasPTR(const uint256_t& hashAddress);

    };
}
