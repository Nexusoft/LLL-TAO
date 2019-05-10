/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/assets.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        Assets assets;

        /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not
        *  map directly to a function in the target API.  Insted this method can be overriden to
        *  parse the incoming URL and route to a different/generic method handler, adding parameter
        *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset 
        *  added to the jsonParams
        *  The return json contains the modifed method URL to be called.
        */
        std::string Assets::RewriteURL( const std::string& strMethod, json::json& jsonParams ) 
        { 
            std::string strMethodRewritten = strMethod;

            
            /* route get/myasset to get/asset?name=myasset */
            /* check to see if this method is a get/myasset format. i.e. it starts with get/ */
            if(strMethod.find("get/") == 0)
            {
                /* get the asset name from after the get/ */
                std::string strAssetName = strMethod.substr(4);
                
                /* Check to see whether there is a datafield after the asset name, i.e. get/myasset/somefield */
                std::string::size_type pos = strAssetName.find("/");
                
                if(pos != std::string::npos)
                {
                    std::string strFieldName = strAssetName.substr(pos +1);
                    strAssetName = strAssetName.substr(0, pos);
                    jsonParams["datafield"] = strFieldName;
                }

                strMethodRewritten = "get/asset";
                jsonParams["name"] = strAssetName;
            }
            
            /* route create/myasset to create/asset?name=myasset */
            /* check to see if this method is a create/myasset format. i.e. it starts with tokenize/ */
            else if(strMethod.find("create/") == 0)
            {
                /* get the asset name from after the create/ */
                std::string strAssetName = strMethod.substr(7);

                strMethodRewritten = "create/asset";
                jsonParams["name"] = strAssetName;
            }

            /* route update/myasset to update/asset?name=myasset */
            /* check to see if this method is a update/myasset format. i.e. it starts with tokenize/ */
            else if(strMethod.find("update/") == 0)
            {
                /* get the asset name from after the update/ */
                std::string strAssetName = strMethod.substr(7);

                strMethodRewritten = "update/asset";
                jsonParams["name"] = strAssetName;
            }

            /* route transfer/myasset to transfer/asset?name=myasset */
            /* check to see if this method is a transfer/myasset format. i.e. it starts with tokenize/ */
            else if(strMethod.find("transfer/") == 0)
            {
                /* get the asset name from after the transfer/ */
                std::string strAssetName = strMethod.substr(9);

                strMethodRewritten = "transfer/asset";
                jsonParams["name"] = strAssetName;
            }

            
            /* route tokenize/myasset to tokenize/asset?name=myasset */
            /* check to see if this method is a tokenize/myasset format. i.e. it starts with tokenize/ */
            else if(strMethod.find("tokenize/") == 0)
            {
                /* get the asset name from after the tokenize/ */
                std::string strAssetName = strMethod.substr(9);
                
                strMethodRewritten = "tokenize/asset";
                jsonParams["name"] = strAssetName;
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
            mapFunctions["list/history"]            = Function(std::bind(&Assets::History,   this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["tokenize/asset"]           = Function(std::bind(&Assets::Tokenize,  this, std::placeholders::_1, std::placeholders::_2));
        }
    }
}
