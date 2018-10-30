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

        Account::print() const;
        {
            printf("Account(id=%s, balance=%u)\n", hashIdentifier.ToString().substr(0, 20).c_str(), nBalance)
        }
    }
}
