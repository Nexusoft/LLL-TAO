/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/tokens/types/tokens.h>
#include <TAO/API/finance/types/finance.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/global.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Tokens::Initialize()
    {
        //XXX: we should format this better, we can't go over 132 characters in a line based on formatting guidelines.
        mapFunctions["create/token"]              = Function(std::bind(&Tokens::Create,            this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["credit/token"]              = Function(std::bind(&Finance::Credit,           TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["debit/token"]               = Function(std::bind(&Tokens::Debit,             this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/token"]                 = Function(std::bind(&Tokens::Get,               this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/token/transactions"]   = Function(std::bind(&Tokens::ListTransactions,  this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["burn/token"]                = Function(std::bind(&Tokens::Burn,              this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/token/accounts"]       = Function(std::bind(&Tokens::ListTokenAccounts, this, std::placeholders::_1, std::placeholders::_2));

        /* Temporary reroute of the account methods to the finance API equivalents XXX: this is really hacky */
        mapFunctions["create/account"]            = Function(std::bind(&Finance::Create,           TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["credit/account"]            = Function(std::bind(&Finance::Credit,           TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["debit/account"]             = Function(std::bind(&Finance::Debit,            TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/account"]               = Function(std::bind(&Finance::Get,              TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/accounts"]             = Function(std::bind(&Finance::List,             TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/account/transactions"] = Function(std::bind(&Finance::ListTransactions, TAO::API::finance, std::placeholders::_1, std::placeholders::_2));
    }

    /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not
    *  map directly to a function in the target API.  Insted this method can be overriden to
    *  parse the incoming URL and route to a different/generic method handler, adding parameter
    *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
    *  added to the jParams
    *  The return json contains the modifed method URL to be called.
    */
    std::string Tokens::RewriteURL(const std::string& strMethod, json::json& jParams)
    {
        //XXX: this is a lot of copy/paste code that could very easily be put into a helper function
        std::string strMethodRewritten = strMethod;

        /* Edge case for list/token/accounts  */
        if(strMethod.find("list/token/accounts") != std::string::npos)
        {
            /* set the method name */
            strMethodRewritten = "list/token/accounts";

            /* Check to see whether there is a name after the /accounts/ name, i.e. list/token/accounts/mytoken */
            std::size_t nPos = strMethod.find("/accounts/");

            if(nPos != std::string::npos)
            {
                /* Get the name or address that comes after the /accounts/ part */
                std::string strNameOrAddress = strMethod.substr(nPos +10);

                /* Determine whether the name/address is a valid register address and set the name or address parameter accordingly */
                if(CheckAddress(strNameOrAddress))
                    jParams["address"] = strNameOrAddress;
                else
                    jParams["name"] = strNameOrAddress;

            }

            return strMethodRewritten;
        }

        std::size_t nPos = strMethod.find("/token");
        if(nPos != std::string::npos)
        {
            /* get the method name from before the /token */
            strMethodRewritten = strMethod.substr(0, nPos);

            /* Check to see whether there is a token name after the /token/ name, i.e. tokens/get/token/mytoken */
            nPos = strMethod.find("/token/");

            if(nPos != std::string::npos)
            {
                /* Get the name or address that comes after the /token/ part */
                std::string strNameOrAddress = strMethod.substr(nPos +7);

                /* Check to see whether there is a fieldname after the token name, i.e. tokens/get/token/mytoken/somefield */
                nPos = strNameOrAddress.find("/");

                if(nPos != std::string::npos)
                {
                    /* Passing in the fieldname is only supported for the /get/ method so if the user has
                       requested a different method then just return the requested URL, which will in turn error */
                    if(strMethodRewritten != "get")
                        return strMethod;

                    std::string strFieldName = strNameOrAddress.substr(nPos +1);
                    strNameOrAddress = strNameOrAddress.substr(0, nPos);
                    jParams["fieldname"] = strFieldName;
                }

                /* Determine whether the name/address is a valid register address and set the name or address parameter accordingly */
                if(CheckAddress(strNameOrAddress))
                    jParams["address"] = strNameOrAddress;
                else
                    jParams["name"] = strNameOrAddress;

            }

            /* Set the type parameter to token */
            jParams["type"] = "token";

            return strMethodRewritten;
        }

        nPos = strMethod.find("/account");
        if(nPos != std::string::npos)
        {
            /* get the method name from before the /account */
            strMethodRewritten = strMethod.substr(0, nPos);

            /* Check to see whether there is an account name after the /account/ name, i.e. tokens/get/account/myaccount */
            nPos = strMethod.find("/account/");

            if(nPos != std::string::npos)
            {
                /* Get the name or address that comes after the /account/ part */
                std::string strNameOrAddress = strMethod.substr(nPos +9);

                /* Check to see whether there is a fieldname after the account name, i.e. tokens/get/account/myaccount/somefield */
                nPos = strNameOrAddress.find("/");

                if(nPos != std::string::npos)
                {
                    /* Passing in the fieldname is only supported for the /get/ method so if the user has
                       requested a different method then just return the requested URL, which will in turn error */
                    if(strMethodRewritten != "get")
                        return strMethod;

                    std::string strFieldName = strNameOrAddress.substr(nPos +1);
                    strNameOrAddress = strNameOrAddress.substr(0, nPos);
                    jParams["fieldname"] = strFieldName;
                }

                /* Determine whether the name/address is a valid register address and set the name or address parameter accordingly */
                if(CheckAddress(strNameOrAddress))
                    jParams["address"] = strNameOrAddress;
                else
                    jParams["name"] = strNameOrAddress;

            }

            /* Set the type parameter to account */
            jParams["type"] = "account";

            return strMethodRewritten;
        }

        return strMethodRewritten;
    }
}
