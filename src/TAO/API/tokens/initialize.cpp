/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/tokens.h>
#include <TAO/API/include/utils.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        Tokens tokens;

        /* Standard initialization function. */
        void Tokens::Initialize()
        {
            mapFunctions["create"]        = Function(std::bind(&Tokens::Create,         this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["credit"]        = Function(std::bind(&Tokens::Credit,         this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["debit"]         = Function(std::bind(&Tokens::Debit,          this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get"]           = Function(std::bind(&Tokens::Get,            this, std::placeholders::_1, std::placeholders::_2));
            
        }

        /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not
        *  map directly to a function in the target API.  Insted this method can be overriden to
        *  parse the incoming URL and route to a different/generic method handler, adding parameter
        *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
        *  added to the jsonParams
        *  The return json contains the modifed method URL to be called.
        */
        std::string Tokens::RewriteURL(const std::string& strMethod, json::json& jsonParams)
        {
            std::string strMethodRewritten = strMethod;
            /* route the /token and /account endpoints to the generic ones e.g. create/token to create?type=token */


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

                    /* Determine whether this is a valid register address and set the name or address parameter accordingly */
                    if(IsRegisterAddress(strNameOrAddress))
                        jsonParams["address"] = strNameOrAddress;
                    else
                        jsonParams["name"] = strNameOrAddress;
                    
                }

                /* Set the type parameter to token */
                jsonParams["type"] = "token";

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

                    /* Determine whether this is a valid register address and set the name or address parameter accordingly */
                    if(IsRegisterAddress(strNameOrAddress))
                        jsonParams["address"] = strNameOrAddress;
                    else
                        jsonParams["name"] = strNameOrAddress;
                    
                }

                /* Set the type parameter to account */
                jsonParams["type"] = "account";

                return strMethodRewritten;
            }

            return strMethodRewritten;
        }
    }
}
