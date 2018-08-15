/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

            (c) Copyright The Nexus Developers 2014 - 2017

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_TYPES_INCLUDE_DATA_H
#define NEXUS_TAO_LEDGER_TYPES_INCLUDE_DATA_H

#include <vector>

#include "data.h"

#include "../../../../Util/templates/serialize.h"

namespace TAO
{

    namespace Ledger
    {

        /** Tritium Transaction Object.

            Simple data type class that holds the transaction version, nextHash, and ledger data.

        **/
        class CTritiumTransaction
        {

            int nVersion;

            uint256 hashNext;

            CData vchLedgerData;


            //memory only
            std::vector<unsigned char> vchPubKey;

            std::vector<unsigned char> vchSig;

            bool IsValid()
            {

            }
        };

}
