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
        LocalDB(uint8_t nFlags = FLAGS::CREATE | FLAGS::WRITE)
        : SectorDatabase("local", nFlags) {}

        bool WriteGenesis(uint256_t hashGenesis, TAO::Ledger::Transaction tx)
        {
            return Write(std::make_pair(std::string("genesis"), hashGenesis), tx);
        }

        bool ReadGenesis(uint256_t hashGenesis, TAO::Ledger::Transaction& tx)
        {
            return Read(std::make_pair(std::string("genesis"), hashGenesis), tx);
        }

        bool HasGenesis(uint256_t hashGenesis)
        {
            return Exists(std::make_pair(std::string("genesis"), hashGenesis));
        }

        bool WriteLast(uint256_t hashGenesis, uint512_t hashLast)
        {
            return Write(std::make_pair(std::string("last"), hashGenesis), hashLast);
        }

        bool ReadLast(uint256_t hashGenesis, uint512_t& hashLast)
        {
            return Read(std::make_pair(std::string("last"), hashGenesis), hashLast);
        }
    };
}

#endif
