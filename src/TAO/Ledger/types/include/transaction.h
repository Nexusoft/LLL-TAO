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

            /** The transaction version. **/
            int nVersion;


            /** The ID of the global signature chain. May not include this in the transaciton payload.**/
            uint256 hashID;

            /** The nextHash which can claim the signature chain. */
            uint256 hashNext;

            /** The data to be recorded in the ledger. **/
            CData vchLedgerData;

            //memory only, to be disposed once fully locked into the chain behind a checkpoints
            //this is for the segregated keys from transaction data.
            std::vector<unsigned char> vchPubKey;
            std::vector<unsigned char> vchSig;

            IMPLEMENT_SERIALIZE
            (
                READWRITE(nVersion);
                READWRITE(hashID);
                READWRITE(hashNext);
                REACWRITE(vchLedgerData);
            )

            bool IsValid()
            {

            }
        };

}
