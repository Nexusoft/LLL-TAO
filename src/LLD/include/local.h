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
#include <TAO/Ledger/types/transaction.h>

namespace LLD
{

    class LocalDB : public SectorDatabase<BinaryFileMap>
    {
    public:
        /** The Database Constructor. To determine file location and the Bytes per Record. **/
        LocalDB(const char* pszMode="r+") : SectorDatabase("local", pszMode) {}

        bool WriteGenesis(TAO::Ledger::Transaction tx)
        {
            return Write(std::make_pair(std::string("genesis")), tx);
        }

        bool WriteTx(TAO::Ledger::Transaction tx)
        {

        }
    };
}

#endif
