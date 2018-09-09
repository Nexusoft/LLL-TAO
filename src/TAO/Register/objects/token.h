/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_TOKEN_H
#define NEXUS_TAO_REGISTER_INCLUDE_TOKEN_H

namespace TAO
{

    namespace Register
    {

        class Token
        {
        public:

            /** The identifier of the account token. **/
            uint256 hashIdentifier;


            /** The maximum supply of said token. **/
            uint64_t nMaxSupply;


            /** The significant figures of said token. **/
            uint8_t  nCoinDigits;


            /** Serialization methods. **/
            SERIALIZE_HEADER


            Token() : hashIdentifier(0), nBalance(0)
            {

            }

            void print() const;
        };
    }
}

#endif
