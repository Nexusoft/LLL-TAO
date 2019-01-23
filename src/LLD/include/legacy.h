/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_INCLUDE_LEGACY_H
#define NEXUS_LLD_INCLUDE_LEGACY_H

#include <LLC/types/uint1024.h>

#include <LLD/include/version.h>
#include <LLD/templates/sector.h>

#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>

#include <Legacy/types/transaction.h>

namespace LLD
{

    /** @class LegacyDB
     *
     *  Database class for storing legacy transactions.
     *
     **/
    class LegacyDB : public SectorDatabase<BinaryHashMap, BinaryLRU>
    {
    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LegacyDB(uint8_t nFlags = FLAGS::CREATE | FLAGS::WRITE)
        : SectorDatabase("legacy", nFlags) {}


        /** Write Tx.
         *
         *  Writes a transaction to the legacy DB.
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *  @param[in] tx The transaction object to write.
         *
         *  @return True if the transaction was successfully written.
         *
         **/
        bool WriteTx(const uint512_t& hashTransaction, const Legacy::Transaction& tx)
        {
            return Write(std::make_pair(std::string("tx"), hashTransaction), tx);
        }


        /** Read Tx.
         *
         *  Reads a transaction from the legacy DB.
         *
         *  @param[in] hashTransaction The txid of transaction to read.
         *  @param[in] tx The transaction object to read.
         *
         *  @return True if the transaction was successfully read.
         *
         **/
        bool ReadTx(const uint512_t& hashTransaction, Legacy::Transaction& tx)
        {
            return Read(std::make_pair(std::string("tx"), hashTransaction), tx);
        }


        /** Erase Tx.
         *
         *  Erases a transaction from the ledger DB.
         *
         *  @param[in] hashTransaction The txid of transaction to erase.
         *
         *  @return True if the transaction was successfully erased.
         *
         **/
        bool EraseTx(const uint512_t& hashTransaction)
        {
            return Erase(std::make_pair(std::string("tx"), hashTransaction));
        }


        /** Has Tx.
         *
         *  Checks if a transaction exists.
         *
         *  @param[in] hashTransaction The txid of transaction to check.
         *
         *  @return True if the transaction exists.
         *
         **/
        bool HasTx(const uint512_t& hashTransaction)
        {
            return Exists(std::make_pair(std::string("tx"), hashTransaction));
        }


        /** Write Spend
         *
         *  Writes an output as spent
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *  @param[in] nOutput The output that was spent
         *
         *  @return True if the written.
         *
         **/
        bool WriteSpend(const uint512_t& hashTransaction, uint32_t nOutput)
        {
            return Write(std::make_pair(hashTransaction, nOutput));
        }


        /** Erase Spend
         *
         *  Removes a spend flag on an output
         *
         *  @param[in] hashTransaction The txid of transaction to write.
         *  @param[in] nOutput The output that is unspent
         *
         *  @return True if the erased.
         *
         **/
        bool EraseSpend(const uint512_t& hashTransaction, uint32_t nOutput)
        {
            return Erase(std::make_pair(hashTransaction, nOutput));
        }


        /** Is Spent
         *
         *  Checks if an output was spent
         *
         *  @param[in] hashTransaction The txid of transaction to check.
         *  @param[in] nOutput The output to check
         *
         *  @return True if the output is spent.
         *
         **/
        bool IsSpent(const uint512_t& hashTransaction, uint32_t nOutput)
        {
            return Exists(std::make_pair(hashTransaction, nOutput));
        }
    };
}

#endif
