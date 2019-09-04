/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLD_INCLUDE_LEGACY_H
#define NEXUS_LLD_INCLUDE_LEGACY_H

#include <LLC/types/uint1024.h>

#include <LLD/templates/sector.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <Legacy/types/transaction.h>

#include <TAO/Ledger/include/enum.h>

namespace LLD
{

    /** LegacyDB
     *
     *  Database class for storing legacy transactions.
     *
     **/
    class LegacyDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LegacyDB(const uint8_t nFlagsIn = FLAGS::CREATE | FLAGS::WRITE,
            const uint32_t nBucketsIn = 77773, const uint32_t nCacheIn = 1024 * 1024);


        /** Default Destructor **/
        virtual ~LegacyDB();


        /** WriteTx
         *
         *  Writes a transaction to the legacy DB.
         *
         *  @param[in] hashTx The txid of transaction to write.
         *  @param[in] tx The transaction object to write.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool WriteTx(const uint512_t& hashTx, const Legacy::Transaction& tx);


        /** ReadTx
         *
         *  Reads a transaction from the legacy DB.
         *
         *  @param[in] hashTx The txid of transaction to read.
         *  @param[in] tx The transaction object to read.
         *  @param[in] nFlags The flags to check from
         *
         *  @return True if the transaction was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint512_t& hashTx, Legacy::Transaction& tx, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** EraseTx
         *
         *  Erases a transaction from the ledger DB.
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
         *  @param[in] nFlags The flags to check from
         *
         *  @return True if the transaction exists, false otherwise.
         *
         **/
        bool HasTx(const uint512_t& hashTx, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** WriteSpend
         *
         *  Writes an output as spent.
         *
         *  @param[in] hashTx The txid of transaction to write.
         *  @param[in] nOutput The output that was spent.
         *
         *  @return True if the spend is written, false otherwise.
         *
         **/
        bool WriteSpend(const uint512_t& hashTx, uint32_t nOutput);


        /** EraseSpend
         *
         *  Removes a spend flag on an output.
         *
         *  @param[in] hashTx The txid of transaction to write.
         *  @param[in] nOutput The output that is unspent.
         *
         *  @return True if the spend is erased, false otherwise.
         *
         **/
        bool EraseSpend(const uint512_t& hashTx, uint32_t nOutput);


        /** IsSpent
         *
         *  Checks if an output was spent.
         *
         *  @param[in] hashTx The txid of transaction to check.
         *  @param[in] nOutput The output to check.
         *
         *  @return True if the output is spent, false otherwise.
         *
         **/
        bool IsSpent(const uint512_t& hashTx, uint32_t nOutput);


        /** WriteSequence
         *
         *  Writes a new sequence event to the legacy database.
         *
         *  @param[in] hashAddress The event address to write.
         *  @param[in] nSequence The sequence to write.
         *
         *  @return True if the write was successful.
         *
         **/
        bool WriteSequence(const uint256_t& hashAddress, const uint32_t nSequence);


        /** ReadSequence
         *
         *  Reads a new sequence from the legacy database
         *
         *  @param[in] hashAddress The event address to read.
         *  @param[out] nSequence The sequence to read.
         *
         *  @return True if the write was successful.
         *
         **/
        bool ReadSequence(const uint256_t& hashAddress, uint32_t &nSequence);


        /** WriteEvent
         *
         *  Write a new event to the legacy database of current txid.
         *
         *  @param[in] hashAddress The event address to write.
         *  @param[in] hashTx The transaction event is triggering.
         *
         *  @return True if the write was successful.
         *
         **/
        bool WriteEvent(const uint256_t& hashAddress, const uint512_t& hashTx);


        /** EraseEvent
         *
         *  Erase an event from the legacy database
         *
         *  @param[in] hashAddress The event address to write.
         *
         *  @return True if the write was successful.
         *
         **/
        bool EraseEvent(const uint256_t& hashAddress);


        /** ReadEvent
         *
         *  Reads a new event to the ledger database of foreign index.
         *  This is responsible for knowing legacy transactions that correlate to your sig chain.
         *
         *  @param[in] hashAddress The event address to write.
         *  @param[in] nSequence The sequence number to read from event.
         *  @param[out] tx The legacy transaction to read event from.
         *
         *  @return True if the write was successful.
         *
         **/
        bool ReadEvent(const uint256_t& hashAddress, const uint32_t nSequence, Legacy::Transaction &tx);


        /** WriteTrustConversion
         *
         *  Writes the hash of a trust key to record that it has been converted from Legacy to Tritium.
         *
         *  @param[in] hashTrust The trust key hash.
         *
         *  @return True if the trust hash is written, false otherwise.
         *
         **/
        bool WriteTrustConversion(const uint512_t& hashTrust);


        /** HasTrustConversion
         *
         *  Checks if a Legacy trust key has already been converted to Tritium.
         *
         *  @param[in] hashTrust The trust key hash.
         *
         *  @return True if the trust key converted, false otherwise.
         *
         **/
        bool HasTrustConversion(const uint512_t& hashTrust);


        /** EraseTrustConversion
         *
         *  Erase a Legacy trust key conversion.
         *
         *  @param[in] hashTrust The trust key hash.
         *
         *  @return True if the trust key erased, false otherwise.
         *
         **/
        bool EraseTrustConversion(const uint512_t& hashTrust);

    };
}

#endif
