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


        bool WriteTx(uint512_t hashTransaction, Legacy::Transaction tx)
        {
            return Write(std::make_pair(std::string("tx"), hashTransaction), tx);
        }


        bool ReadTx(uint512_t hashTransaction, Legacy::Transaction& tx)
        {
            return Read(std::make_pair(std::string("tx"), hashTransaction), tx);
        }


        bool EraseTx(uint512_t hashTransaction)
        {
            return Erase(std::make_pair(std::string("tx"), hashTransaction));
        }


        bool HasTx(uint512_t hashTransaction)
        {
            return Exists(std::make_pair(std::string("tx"), hashTransaction));
        }


        bool WriteSpend(uint512_t hashTransaction, uint32_t nOutput)
        {
            return Write(std::make_pair(hashTransaction, nOutput));
        }


        bool EraseSpend(uint512_t hashTransaction, uint32_t nOutput)
        {
            return Erase(std::make_pair(hashTransaction, nOutput));
        }


        bool IsSpent(uint512_t hashTransaction, uint32_t nOutput)
        {
            return Exists(std::make_pair(hashTransaction, nOutput));
        }
    };
}

#endif
