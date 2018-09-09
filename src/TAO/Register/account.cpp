/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/objects/account.h>

namespace TAO
{
    namespace Register
    {
        class CAccount
        {
        public:

            /** The identifier of the account token. **/
            uint256 hashIdentifier;

            /** The balance of total tokens in account. **/
            uint64_t nBalance;

            /** Serialization methods. **/
            SERIALIZE_HEADER

            CAccount() : hashIdentifier(0), nBalance(0)
            {

            }

            void print();
        };
    }
}
