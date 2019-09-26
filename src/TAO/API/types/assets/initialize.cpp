/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/assets.h>
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
        std::string Assets::RewriteURL(const std::string& strMethod, json::json& jsonParams)
        {
            std::string strMethodRewritten = strMethod;

            /* support passing the asset name after the method e.g. get/asset/myasset */
            std::size_t nPos = strMethod.find("/asset/");
            if(nPos != std::string::npos)
            {
                std::string strNameOrAddress;

                /* Edge case for /assets/list/asset/history/assetname */
                if(strMethod == "list/asset/history")
                    return strMethod;
                else if(strMethod.find("list/asset/history/") != std::string::npos)
                {
                    strMethodRewritten = "list/asset/history";

                    /* Get the name or address that comes after the list/asset/history part */
                    strNameOrAddress = strMethod.substr(19); 
                }
                else
                {
                    /* get the method name from the incoming string */
                    strMethodRewritten = strMethod.substr(0, nPos+6);

                    /* Get the name or address that comes after the /asset/ part */
                    strNameOrAddress = strMethod.substr(nPos +7);
                }

                /* Check to see whether there is a fieldname after the token name, i.e. asset/get/asset/myasset/somefield */
                nPos = strNameOrAddress.find("/");

                if(nPos != std::string::npos)
                {
                    /* Passing in the fieldname is only supported for the /get/  so if the user has 
                        requested a different method then just return the requested URL, which will in turn error */
                    if(strMethodRewritten != "get/asset")
                        return strMethod;

                    std::string strFieldName = strNameOrAddress.substr(nPos +1);
                    strNameOrAddress = strNameOrAddress.substr(0, nPos);
                    jsonParams["fieldname"] = strFieldName;
                }

                
                /* Edge case for claim/asset/txid */
                if(strMethodRewritten == "claim/asset")
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
        void Assets::Initialize()
        {
            mapFunctions["create/asset"]             = Function(std::bind(&Assets::Create,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["update/asset"]             = Function(std::bind(&Assets::Update,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get/asset"]                = Function(std::bind(&Assets::Get,       this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["transfer/asset"]           = Function(std::bind(&Assets::Transfer,  this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["claim/asset"]              = Function(std::bind(&Assets::Claim,  this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/asset/history"]       = Function(std::bind(&Assets::History,   this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["tokenize/asset"]           = Function(std::bind(&Assets::Tokenize,  this, std::placeholders::_1, std::placeholders::_2));
        }
    }
}
