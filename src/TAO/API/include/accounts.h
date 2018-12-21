/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_INCLUDE_ACCOUNTS_H
#define NEXUS_TAO_API_INCLUDE_ACCOUNTS_H

#include <TAO/API/types/base.h>

namespace TAO::API
{

    /** Accounts API Class
     *
     *  Manages the function pointers for all Accounts commands.
     *
     **/
    class Accounts : public Base
    {
    public:
        Accounts() {}

        void Initialize();

        std::string GetName() const
        {
            return "Accounts";
        }

        /** Create Account
         *
         *  Create's a user account.
         *
         **/
        json::json CreateAccount(const json::json& params, bool fHelp);


        /** Get Transactions
         *
         *  Get transactions for an account
         *
         **/
        json::json GetTransactions(const json::json& params, bool fHelp);
    };

    extern Accounts accounts;
}

#endif
