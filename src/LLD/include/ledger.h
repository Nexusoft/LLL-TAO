/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_INCLUDE_LEDGER_H
#define NEXUS_LLD_INCLUDE_LEDGER_H

#include <LLC/types/uint1024.h>

#include <LLD/include/version.h>
#include <LLD/templates/sector.h>
#include <LLD/templates/filemap.h>

#include <TAO/Register/include/state.h>

namespace LLD
{

    class LedgerDB : public SectorDatabase<BinaryFileMap>
    {
    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LedgerDB(const char* pszMode="r+") : SectorDatabase("ledger", pszMode) {}

        bool WriteTx(uint512_t hashTransaction, TAO::Ledger::Transaction tx)
        {
            return Write(std::make_pair(std::string("tx"), hashTransaction), tx);
        }


        bool ReadTx(uint512_t hashTransaction, TAO::Ledger::Transaction& tx)
        {
            return Read(std::make_pair(std::string("tx"), hashTransaction), tx);
        }


        bool WriteProof(uint256_t hashProof, uint512_t hashTransaction)
        {
            uint64_t nDummy = 0; //this is being used as a boolean express. TODO: make LLD handle bool key writes
            return Write(std::make_pair(hashProof, hashTransaction), nDummy);
        }


        bool HasProof(uint256_t hashProof, uint512_t hashTransaction)
        {
            return Exists(std::make_pair(hashProof, hashTransaction));
        }
    };
}

#endif
