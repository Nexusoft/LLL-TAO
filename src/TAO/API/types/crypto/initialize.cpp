/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/crypto.h>
#include <TAO/API/include/utils.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        
        

        /* Standard initialization function. */
        void Crypto::Initialize()
        {
            mapFunctions["list/keys"]           = Function(std::bind(&Crypto::List, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["create/key"]          = Function(std::bind(&Crypto::Create,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get/key"]             = Function(std::bind(&Crypto::Get,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get/privatekey"]      = Function(std::bind(&Crypto::GetPrivate, this, std::placeholders::_1, std::placeholders::_2));
            // mapFunctions["change/scheme"]       = Function(std::bind(&Crypto::ChangeScheme,this, std::placeholders::_1, std::placeholders::_2));
            // mapFunctions["encrypt/data"]        = Function(std::bind(&Crypto::Encrypt,this, std::placeholders::_1, std::placeholders::_2));
            // mapFunctions["decrypt/data"]        = Function(std::bind(&Crypto::Decrypt,this, std::placeholders::_1, std::placeholders::_2));
            // mapFunctions["create/signature"]    = Function(std::bind(&Crypto::Sign,this, std::placeholders::_1, std::placeholders::_2));
            // mapFunctions["verify/signature"]    = Function(std::bind(&Crypto::Verify,this, std::placeholders::_1, std::placeholders::_2));
            // mapFunctions["get/hash"]            = Function(std::bind(&Crypto::Hash,this, std::placeholders::_1, std::placeholders::_2));
        }

        /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not
        *  map directly to a function in the target API.  Insted this method can be overriden to
        *  parse the incoming URL and route to a different/generic method handler, adding parameter
        *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
        *  added to the jsonParams
        *  The return json contains the modifed method URL to be called.
        */
        std::string Crypto::RewriteURL(const std::string& strMethod, json::json& jsonParams)
        {
            std::string strMethodRewritten = strMethod;

            /* support passing the key name after the method e.g. get/key/auth */
            std::size_t nPos = strMethod.find("/key/");
            if(nPos != std::string::npos)
            {
                std::string strName;

                strMethodRewritten = strMethod.substr(0, nPos+4);

                /* Get the name or address that comes after the /key/ part */
                strName = strMethod.substr(nPos +5);
            
                /* Check to see whether there is a fieldname after the key name, i.e.  get/key/auth/scheme */
                nPos = strName.find("/");

                if(nPos != std::string::npos)
                {
                    /* Passing in the fieldname is only supported for the /get/  so if the user has
                        requested a different method then just return the requested URL, which will in turn error */
                    if(strMethodRewritten != "get/key")
                        return strMethod;

                    std::string strFieldName = strName.substr(nPos +1);
                    strName = strName.substr(0, nPos);
                    jsonParams["fieldname"] = strFieldName;
                }

                jsonParams["name"] = strName;

            }

            return strMethodRewritten;
        }
    }
}
