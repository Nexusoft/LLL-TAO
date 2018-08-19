/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

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


            /** The nextHash which can claim the signature chain. */
            uint256_t hashNext;

            /** The data to be recorded in the ledger. **/
            CData vchLedgerData;

            //memory only, to be disposed once fully locked into the chain behind a checkpoints
            //this is for the segregated keys from transaction data.
            std::vector<uint8_t> vchPubKey;
            std::vector<uint8_t> vchSig;

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
