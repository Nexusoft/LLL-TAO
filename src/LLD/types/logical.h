/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

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


        /** PushRegisterTx
         *
         *   Push an register transaction to process for given register address.
         *
         *  @param[in] hashRegister The address of register to list for
         *  @param[in] hashTx The txid we are pushing register for.
         *
         *  @return true if written successfully
         *
         **/
        bool PushRegisterTx(const uint256_t& hashRegister, const uint512_t& hashTx);


        /** EraseRegisterTx
         *
         *  Erase the last txide that modified a register state.
         *
         *  @param[in] hashRegister The address of register to list for
         *
         *  @return true if written successfully
         *
         **/
        bool EraseRegisterTx(const uint256_t& hashRegister);


        /** LastRegisterTx
         *
         *  Get an register transaction for given register address.
         *
         *  @param[in] hashRegister The address of register to list for
         *  @param[in] hashTx The list of events extracted.
         *
         *  @return true if written successfully
         *
         **/
        bool LastRegisterTx(const uint256_t& hashRegister, uint512_t &hashTx);


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


       /** PushOrder
        *
        *  Pushes an order to the orderbook stack for a given asset.
        *
        *  @param[in] hashRegister The asset's register address.
        *  @param[in] rContract The contract that contains the existing order.
        *  @param[in] nContract The contract-id that contains the order
        *
        *  @return true if written successfully
        *
        **/
       bool PushOrder(const uint256_t& hashRegister, const TAO::Operation::Contract& rContract, const uint32_t nContract);


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


        /** Build indexes for transactions over a rolling modulus. For -indexregister flag. **/
        void IndexRegisters();

    };
}
