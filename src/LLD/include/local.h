/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_INCLUDE_LOCAL_H
#define NEXUS_LLD_INCLUDE_LOCAL_H

#include <LLC/types/uint1024.h>

#include <LLD/include/version.h>
#include <LLD/templates/sector.h>

#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/filemap.h>

#include <TAO/Register/include/state.h>
#include <TAO/Ledger/types/transaction.h>

namespace LLD
{

    class LocalDB : public SectorDatabase<BinaryFileMap, BinaryLRU>
    {
    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LocalDB(const char* pszMode="r+") : SectorDatabase("local", pszMode) {}

        bool WriteGenesis(TAO::Ledger::Transaction tx)
        {
            return Write(std::string("genesis"), tx);
        }

        bool ReadGenesis(TAO::Ledger::Transaction& tx)
        {
            return Read(std::string("genesis"), tx);
        }

        bool WriteTx(TAO::Ledger::Transaction tx)
        {
            return Write(std::make_pair(std::string("tx"), tx.GetHash()), tx);
        }

        bool ReadTx(uint512_t hash, TAO::Ledger::Transaction& tx)
        {
            return Read(std::make_pair(std::string("tx"), hash), tx);
        }

        bool WriteLast(uint512_t hashLast)
        {
            return Write(std::string("last"), hashLast);
        }

        bool ReadLast(uint512_t& hashLast)
        {
            return Read(std::string("last"), hashLast);
        }
    };
}

#endif
