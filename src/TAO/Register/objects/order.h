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

        class Order
        {
        public:

            /** The identifier of the order (product). **/
            uint256 hashIdentifier;


            /** The quantity ordered **/
            uint32_t  nQuantity;


            /** Serialization methods. **/
            SERIALIZE_HEADER


            Order() : hashIdentifier(0), nBalance(0)
            {

            }

            void print() const;
        };
    }
}

#endif
