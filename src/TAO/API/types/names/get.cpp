/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/names.h>
#include <TAO/API/types/users.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Register/include/names.h>

/* Global TAO namespace. */
namespace TAO
{
    /* API Layer namespace. */
    namespace API
    {
        /* Get the data from a name */
        json::json Names::Get(const json::json& params, bool fHelp)
        {
            /* Return JSON object */
            json::json jsonRet;

            /* Declare the Name object to look for*/
            TAO::Register::Object name;

            /* Register address of Name object */
            uint256_t hashNameRegister = 0;

            /* Get the session to be used for this API call. */
            uint64_t nSession = users->GetSession(params, true);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);

            /* If the caller has provided a name parameter then retrieve it by name */
            if(params.find("name") != params.end())
                name = Names::GetName(params, params["name"].get<std::string>(), hashNameRegister);

            /* Otherwise try to find the name record based on the register address. */
            else if(params.find("register_address") != params.end())
            {
                /* Check that the caller has passed a valid register address */
                if(!IsRegisterAddress(params["register_address"].get<std::string>()))
                    throw APIException(-25, "Invalid register address");

                /* The register address that the name points to */
                uint256_t hashRegister;
                hashRegister.SetHex(params["register_address"].get<std::string>());

                /* Get the name object based on the register address it points to*/
                name = Names::GetName( user->Genesis(), hashRegister, hashNameRegister);
            }
            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing name / register address");

            /* Ensure we found a name object */
            if(name.IsNull())
                throw APIException(-23, "Name not found.");

            /* Populate the json response */
            jsonRet["namespace"] = name.get<std::string>("namespace"); 
            jsonRet["name"] = name.get<std::string>("name");
            jsonRet["register_address"] = name.get<uint256_t>("address").ToString();
            jsonRet["address"] = hashNameRegister.GetHex();
            jsonRet["owner"] = name.hashOwner.GetHex();

            return jsonRet;
        }


        /* Get the data from a namespace */
        json::json Names::GetNamespace(const json::json& params, bool fHelp)
        {
            /* Return JSON object */
            json::json jsonRet;

            /* Register address of Namespace object */
            uint256_t hashRegister = 0;

            /* If the caller has provided a name parameter then retrieve it by name */
            if(params.find("name") == params.end())
                throw APIException(-23, "Missing namespace name");

            /* Get the namespace name */
            std::string strNamespace = params["name"].get<std::string>();

            /* Namespace register address is a SK256 hash of the namespace name */
            hashRegister = LLC::SK256(strNamespace);

            /* Retrieve the namespace object */
            TAO::Register::Object namespaceObject;
            if(!TAO::Register::GetNamespaceRegister(strNamespace, namespaceObject))
                throw APIException(-23, "Invalid namespace");

            /* Populate the json response */
            jsonRet["name"] = namespaceObject.get<std::string>("namespace"); 
            jsonRet["address"] = hashRegister.GetHex();
            jsonRet["owner"] = namespaceObject.hashOwner.GetHex();

            return jsonRet;
        }
    }
}
