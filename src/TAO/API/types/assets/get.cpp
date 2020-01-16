/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/assets.h>
#include <TAO/API/types/names.h>
#include <TAO/API/include/json.h>
#include <TAO/API/include/object_utils.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get the data from a digital asset */
        json::json Assets::Get(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the Register ID. */
            TAO::Register::Address hashRegister ;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetBase58(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-33, "Missing name / address");

            /* Check to see if the caller has requested to validate against a template */
            TAO::Register::Address hashTemplate;
            TAO::Register::Object filterTemplate;
            if(params.find("template_name") != params.end())
            /* If template name is provided then use this to deduce the register address */
                hashTemplate = Names::ResolveAddress(params, params["template_name"].get<std::string>());
            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("template_address") != params.end())
                hashTemplate.SetBase58(params["template_address"].get<std::string>());

            /* Check to see if the caller has requested to cast the response to the template type */
            bool fCast = false;
            if(params.find("cast") != params.end() && 
                (params["cast"].get<std::string>() == "true" || params["cast"].get<std::string>() == "1"))
                fCast = true;

            if(hashTemplate.IsObject())
            {
                /* Read the template object from the database */
                if( !LLD::Register->ReadState(hashTemplate, filterTemplate, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-224, "Template not found");
                
                /* Parse the schema so that we can access the fields */
                if(!filterTemplate.Parse())
                    throw APIException(-36, "Failed to parse object register");
            }


            /* Check to see whether the caller has requested a specific data field to return */
            std::string strDataField = "";


            /* Get the asset from the register DB.  We can read it as an Object.
               If this fails then we try to read it as a base State type and assume it was
               created as a raw format asset */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-34, "Asset not found");

            /* Only include raw and non-standard object types (assets)*/
            if(object.nType != TAO::Register::REGISTER::APPEND
            && object.nType != TAO::Register::REGISTER::RAW
            && object.nType != TAO::Register::REGISTER::OBJECT)
            {
                throw APIException(-35, "Specified name/address is not an asset.");
            }

            if(object.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* parse object so that the data fields can be accessed */
                if(!object.Parse())
                    throw APIException(-36, "Failed to parse object register");

                /* Only include non standard object registers (assets) */
                if(object.Standard() != TAO::Register::OBJECTS::NONSTANDARD)
                    throw APIException(-35, "Specified name/address is not an asset.");

                /* Check if the caller has requested to validate against a template */
                if(hashTemplate.IsObject() && !filterTemplate.IsNull())
                {
                    /* Check that it matches the template*/
                    if(!Matches(object, filterTemplate))
                        throw APIException(-225, "Asset does not match template");

                    /* If the caller has requested to cast the asset to the requested template then cast it */
                    if(fCast)
                        object = Cast(object, filterTemplate);
                }
            }

            /* Populate the response JSON */
            ret["owner"]    = TAO::Register::Address(object.hashOwner).ToString();
            ret["created"]  = object.nCreated;
            ret["modified"] = object.nModified;

            json::json data  =TAO::API::ObjectToJSON(params, object, hashRegister);

            /* Copy the asset data in to the response after the type/checksum */
            ret.insert(data.begin(), data.end());


            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            if(params.find("fieldname") != params.end())
            {
                /* First get the fieldname from the response */
                std::string strFieldname =  params["fieldname"].get<std::string>();

                /* Iterate through the response keys */
                for(auto it = ret.begin(); it != ret.end(); ++it)
                    /* If this key is not the one that was requested then erase it */
                    if(it.key() != strFieldname)
                        ret.erase(it);
            }

            return ret;
        }
    }
}
