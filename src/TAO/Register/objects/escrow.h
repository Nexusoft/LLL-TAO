/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_ACCOUNT_H
#define NEXUS_TAO_REGISTER_INCLUDE_ACCOUNT_H

namespace TAO
{

    namespace Register
    {

        class Escrow
        {
        public:

            /** The total signers. **/
            uint32_t nSignatories;


            /** The required signers. **/
            uint32_t nRequired;


            /** The hashed keys that can sign. **/
            std::vector<uint256_t> hashPubKeys;


            /** Serialization methods. **/
            SERIALIZE_HEADER


            Escrow() : hashIdentifier(0), nBalance(0)
            {

            }

            void print() const;
        };
    }
}

#endif
