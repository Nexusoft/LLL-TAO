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
            unsigned short nLength;

            /** The byte code of the parameter data. **/
            std::vector<unsigned char> vchData;

            /** Default Paramter Consturctor. **/
            CData() : nLength(0) {}

            /** Data Constrcutor to set from byte vector. **/
            CData(std::vector<unsigned char> vchDataIn) : nLength(vchDataIn.size()), vchData(vchDataIn) { }

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
            std::vector<unsigned char> bytes() { return vchData; }


            /** Return a 256 bit unsigned int of the data's hash. **/
            LLC::uint256 GetHash() const
            {
                return LLC::SK256(vchData.begin(), vchData.end());
            }
        };

    }

}
