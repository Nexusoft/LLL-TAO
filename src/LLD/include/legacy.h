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
#include <LLD/cache/binary_lfu.h>
#include <LLD/keychain/hashmap.h>

#include <Legacy/types/transaction.h>


namespace LLD
{

    /** LegacyDB
     *
     *  Database class for storing legacy transactions.
     *
     **/
    class LegacyDB : public SectorDatabase<BinaryHashMap, BinaryLFU>
    {
    public:


        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LegacyDB(uint8_t nFlags = FLAGS::CREATE | FLAGS::WRITE);


        /** Default Destructor **/
        virtual ~LegacyDB();


        /** WriteTx
         *
         *  Writes a transaction to the legacy DB.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *  @param[in] tx The transaction object to write.
         *
         *  @return True if the transaction was successfully written, false otherwise.
         *
         **/
        bool WriteTx(const uint512_t& hashTransaction, const Legacy::Transaction& tx);


        /** ReadTx
         *
         *  Reads a transaction from the legacy DB.
         *
         *  @param[in] hashTransaction The txid of transaction to read.
         *  @param[in] tx The transaction object to read.
         *
         *  @return True if the transaction was successfully read, false otherwise.
         *
         **/
        bool ReadTx(const uint512_t& hashTransaction, Legacy::Transaction& tx);


        /** EraseTx
         *
         *  Erases a transaction from the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to erase.
         *
         *  @return True if the transaction was successfully erased, false otherwise.
         *
         **/
        bool EraseTx(const uint512_t& hashTransaction);


        /** HasTx
         *
         *  Checks if a transaction exists.
         *
         *  @param[in] hashTransaction The txid of transaction to check.
         *
         *  @return True if the transaction exists, false otherwise.
         *
         **/
        bool HasTx(const uint512_t& hashTransaction);


        /** WriteSpend
         *
         *  Writes an output as spent.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *  @param[in] nOutput The output that was spent.
         *
         *  @return True if the spend is written, false otherwise.
         *
         **/
        bool WriteSpend(const uint512_t& hashTransaction, uint32_t nOutput);


        /** EraseSpend
         *
         *  Removes a spend flag on an output.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *  @param[in] nOutput The output that is unspent.
         *
         *  @return True if the spend is erased, false otherwise.
         *
         **/
        bool EraseSpend(const uint512_t& hashTransaction, uint32_t nOutput);


        /** IsSpent
         *
         *  Checks if an output was spent.
         *
         *  @param[in] hashTransaction The txid of transaction to check.
         *  @param[in] nOutput The output to check.
         *
         *  @return True if the output is spent, false otherwise.
         *
         **/
        bool IsSpent(const uint512_t& hashTransaction, uint32_t nOutput);

    };
}

#endif
