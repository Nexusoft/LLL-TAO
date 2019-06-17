/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/users.h>
#include <TAO/API/include/utils.h>

namespace TAO
{
    namespace API
    {
        /* Standard initialization function. */
        void Users::Initialize()
        {
            mapFunctions["create/user"]              = Function(std::bind(&Users::Create,        this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["login/user"]               = Function(std::bind(&Users::Login,         this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["logout/user"]              = Function(std::bind(&Users::Logout,        this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["lock/user"]                = Function(std::bind(&Users::Lock,          this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["unlock/user"]              = Function(std::bind(&Users::Unlock,        this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/transactions"]        = Function(std::bind(&Users::Transactions,  this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/notifications"]       = Function(std::bind(&Users::Notifications, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/assets"]              = Function(std::bind(&Users::Assets,        this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/tokens"]              = Function(std::bind(&Users::Tokens,        this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/accounts"]            = Function(std::bind(&Users::Accounts,      this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/names"]               = Function(std::bind(&Users::Names,         this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/namespaces"]          = Function(std::bind(&Users::Namespaces,    this, std::placeholders::_1, std::placeholders::_2));
        }

        /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not
        *  map directly to a function in the target API.  Insted this method can be overriden to
        *  parse the incoming URL and route to a different/generic method handler, adding parameter
        *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
        *  added to the jsonParams
        *  The return json contains the modifed method URL to be called.
        */
        std::string Users::RewriteURL(const std::string& strMethod, json::json& jsonParams)
        {
            std::string strMethodRewritten = strMethod;
            std::string strNameOrAddress;


            if(strMethod.find("user/")          != std::string::npos
            || strMethod.find("transactions/")  != std::string::npos
            || strMethod.find("notifications/") != std::string::npos
            || strMethod.find("assets/")        != std::string::npos
            || strMethod.find("accounts/")      != std::string::npos
            || strMethod.find("tokens/")        != std::string::npos
            || strMethod.find("names/")         != std::string::npos
            || strMethod.find("namespaces/")    != std::string::npos)
            {
                /* support passing the username after a list method e.g. list/assets/myusername */
                size_t nPos = strMethod.find_last_of("/");

                if(nPos != std::string::npos)
                {
                    /* get the method name from the incoming string */
                    strMethodRewritten = strMethod.substr(0, nPos);

                    /* Get the name or address that comes after the /item/ part */
                    strNameOrAddress = strMethod.substr(nPos + 1);

                    /* Determine whether the name/address is a valid register address and set the name or address parameter accordingly */
                    if(IsRegisterAddress(strNameOrAddress))
                        jsonParams["genesis"] = strNameOrAddress;
                    else
                        jsonParams["username"] = strNameOrAddress;
                }
            }

            return strMethodRewritten;
        }

    }
}
