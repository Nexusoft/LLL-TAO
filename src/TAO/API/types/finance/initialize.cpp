/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/finance.h>
#include <TAO/API/include/utils.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Standard initialization function. */
        void Finance::Initialize()
        {
            mapFunctions["create/account"]  = Function(std::bind(&Finance::Create, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["credit/account"]  = Function(std::bind(&Finance::Credit, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["debit/account"]   = Function(std::bind(&Finance::Debit, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get/account"]     = Function(std::bind(&Finance::Get, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get/balances"]   = Function(std::bind(&Finance::GetBalances, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get/stakeinfo"]   = Function(std::bind(&Finance::Info, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/accounts"]   = Function(std::bind(&Finance::List, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/account/transactions"]  = Function(std::bind(&Finance::ListTransactions, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["migrate/stake"]       = Function(std::bind(&Finance::MigrateStake, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["migrate/accounts"]    = Function(std::bind(&Finance::MigrateAccounts, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["set/stake"]       = Function(std::bind(&Finance::Stake, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/trustaccounts"] = Function(std::bind(&Finance::TrustAccounts, this, std::placeholders::_1, std::placeholders::_2));
        }

        /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not
        *  map directly to a function in the target API.  Insted this method can be overriden to
        *  parse the incoming URL and route to a different/generic method handler, adding parameter
        *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
        *  added to the jsonParams
        *  The return json contains the modifed method URL to be called.
        */
        std::string Finance::RewriteURL(const std::string& strMethod, json::json& jsonParams)
        {
            std::string strMethodRewritten = strMethod;

            /* support passing the account name or address after the method e.g. get/account/myaccount */
            std::size_t nPos = strMethod.find("/account/");
            if(nPos != std::string::npos)
            {
                std::string strNameOrAddress;

                /* edge case for list/account/transactions */
                std::size_t nListPos = strMethod.find("/account/transactions/");

                if(nListPos != std::string::npos)
                {
                    /* Extract the method name */
                    strMethodRewritten = strMethod.substr(0, nListPos+21);

                    /* Get the name or address that comes after the /account/ part */
                    strNameOrAddress = strMethod.substr(nListPos +22);
                }
                else
                {
                    /* Extract the method name */
                    strMethodRewritten = strMethod.substr(0, nPos+8);

                    /* Get the name or address that comes after the /account/ part */
                    strNameOrAddress = strMethod.substr(nPos +9);
                }


                /* Check to see whether there is a fieldname after the token name, i.e.  get/account/myaccount/somefield */
                nPos = strNameOrAddress.find("/");

                if(nPos != std::string::npos)
                {
                    /* Passing in the fieldname is only supported for the /get/  so if the user has
                        requested a different method then just return the requested URL, which will in turn error */
                    if(strMethodRewritten != "get/account")
                        return strMethod;

                    std::string strFieldName = strNameOrAddress.substr(nPos +1);
                    strNameOrAddress = strNameOrAddress.substr(0, nPos);
                    jsonParams["fieldname"] = strFieldName;
                }


                /* Determine whether the name/address is a valid register address and set the name or address parameter accordingly */
                if(IsRegisterAddress(strNameOrAddress))
                    jsonParams["address"] = strNameOrAddress;
                else
                    jsonParams["name"] = strNameOrAddress;

            }

            return strMethodRewritten;
        }
    }
}
