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

#include "../../../../Util/templates/serialize.h"

namespace TAO
{

    namespace Ledger
    {

        /** CData Class
        *
        *  This class is responsible for serializing and deserializing a parameter from the Operations layer to interpret new data or call upon registers.
        *
        **/
        class CData
        {
            /** The length of the parameter data. **/
            uint16_t nLength;

            /** The byte code of the parameter data. **/
            std::vector<uint8_t> vchData;

            /** Default Paramter Consturctor. **/
            CData() : nLength(0) {}

            /** Data Constrcutor to set from byte vector. **/
            CData(std::vector<uint8_t> vchDataIn) : nLength(vchDataIn.size()), vchData(vchDataIn) { }

            IMPLEMENT_SERIALIZE
            (
                READWRITE(nLength);
                READWRITE(FLATDATA(vchData));
            )

            /** Is NULL Check. **/
            bool IsNull()
            {
                return (nLength == 0 && vchParameter.empty());
            }

            /** Set NULL Flag. **/
            void SetNull()
            {
                nLength = 0;
                vchParameter.clear();
            }

            /** Return the bytes in this data state. */
            std::vector<uint8_t> bytes() { return vchData; }


            /** Return a 256 bit uint32_t of the data's hash. **/
            uint256_t GetHash() const
            {
                return LLC::SK256(vchData.begin(), vchData.end());
            }
        };

    }

}
