/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/supply.h>
#include <TAO/API/include/utils.h>

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
        std::string Supply::RewriteURL(const std::string& strMethod, json::json& jsonParams)
        {
            std::string strMethodRewritten = strMethod;

            /* support passing the item name after the method e.g. get/item/myitem */
            std::size_t nPos = strMethod.find("/item/");
            if(nPos != std::string::npos)
            {
                std::string strNameOrAddress;

                /* Edge case for /supply/list/item/history/itemname */
                if(strMethod == "list/item/history")
                    return strMethod;
                else if(strMethod.find("list/item/history/") != std::string::npos)
                {
                    strMethodRewritten = "list/item/history";

                    /* Get the name or address that comes after the list/item/history part */
                    strNameOrAddress = strMethod.substr(18); 
                }
                else
                {
                    /* get the method name from the incoming string */
                    strMethodRewritten = strMethod.substr(0, nPos+5);

                    /* Get the name or address that comes after the /item/ part */
                    strNameOrAddress = strMethod.substr(nPos +6);
                }

                /* Check to see whether there is a fieldname after the token name, i.e. supply/get/item/myitem/somefield */
                nPos = strNameOrAddress.find("/");

                if(nPos != std::string::npos)
                {
                    /* Passing in the fieldname is only supported for the /get/  so if the user has 
                        requested a different method then just return the requested URL, which will in turn error */
                    if(strMethodRewritten != "get/item")
                        return strMethod;

                    std::string strFieldName = strNameOrAddress.substr(nPos +1);
                    strNameOrAddress = strNameOrAddress.substr(0, nPos);
                    jsonParams["fieldname"] = strFieldName;
                }

                
                /* Edge case for claim/item/txid */
                if(strMethodRewritten == "claim/item")
                    jsonParams["txid"] = strNameOrAddress;
                /* Determine whether the name/address is a valid register address and set the name or address parameter accordingly */
                else if(IsRegisterAddress(strNameOrAddress))
                    jsonParams["address"] = strNameOrAddress;
                else
                    jsonParams["name"] = strNameOrAddress;
                    
            }

            return strMethodRewritten;
        }

        /* Standard initialization function. */
        void Supply::Initialize()
        {
            mapFunctions["get/item"]             = Function(std::bind(&Supply::GetItem,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["transfer/item"]        = Function(std::bind(&Supply::Transfer,   this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["claim/item"]           = Function(std::bind(&Supply::Claim,      this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["create/item"]          = Function(std::bind(&Supply::CreateItem, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["update/item"]          = Function(std::bind(&Supply::UpdateItem, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/item/history"]    = Function(std::bind(&Supply::History,    this, std::placeholders::_1, std::placeholders::_2));
        }
    }
}
