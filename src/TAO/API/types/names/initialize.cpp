/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/names.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

/* Global TAO namespace. */
namespace TAO
{
    /* API Layer namespace. */
    namespace API
    {
        /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not
         *  map directly to a function in the target API.  Insted this method can be overriden to
         *  parse the incoming URL and route to a different/generic method handler, adding parameter
         *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
         *  added to the jsonParams
         *  The return json contains the modifed method URL to be called.
         */
        std::string Names::RewriteURL(const std::string& strMethod, json::json& jsonParams)
        {
            /* The rewritten method name to return.  Default to the method name passed in */
            std::string strMethodRewritten = strMethod;

            /* support passing the  name after the method e.g. get/name/myname */
            std::size_t nPos = strMethod.find("/name/");
            if(nPos != std::string::npos)
            {
                std::string strNameOrAddress;

                /* Edge case for /names/list/name/history/somename */
                if(strMethod == "list/name/history")
                    return strMethod;
                else if(strMethod.find("list/name/history/") != std::string::npos)
                {
                    strMethodRewritten = "list/name/history";

                    /* Get the name or address that comes after the list/name/history part */
                    strNameOrAddress = strMethod.substr(18); 
                }
                else
                {
                    /* get the method name from the incoming string */
                    strMethodRewritten = strMethod.substr(0, nPos+5);

                    /* Get the name or address that comes after the /name/ part */
                    strNameOrAddress = strMethod.substr(nPos +6);
                }

                
                /* Edge case for claim/name/txid */
                if(strMethodRewritten == "claim/name")
                    jsonParams["txid"] = strNameOrAddress;
                /* Determine whether the name/address is a valid register address and set the name or address parameter accordingly */
                else if(IsRegisterAddress(strNameOrAddress) && strMethodRewritten == "get/name")
                    jsonParams["register_address"] = strNameOrAddress;
                else if(IsRegisterAddress(strNameOrAddress))
                    jsonParams["address"] = strNameOrAddress;
                else
                    jsonParams["name"] = strNameOrAddress;
                    
            }

            return strMethodRewritten;
        }


        /* Standard initialization function. */
        void Names::Initialize()
        {
            mapFunctions["create/name"]             = Function(std::bind(&Names::Create,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["update/name"]             = Function(std::bind(&Names::UpdateName,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get/name"]                = Function(std::bind(&Names::Get,       this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["transfer/name"]           = Function(std::bind(&Names::TransferName,  this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["claim/name"]              = Function(std::bind(&Names::ClaimName,  this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/name/history"]       = Function(std::bind(&Names::NameHistory,   this, std::placeholders::_1, std::placeholders::_2));
        }
    }
}
