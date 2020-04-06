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
#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>

#include <Util/include/base64.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get the schema for an asset */
        json::json Assets::GetSchema(const json::json& params, bool fHelp)
        {
            /* The response JSON array */
            json::json ret = json::json::array();

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


            /* Get the asset from the register DB.  We can read it as an Object.
               If this fails then we try to read it as a base State type and assume it was
               created as a raw format asset */
            TAO::Register::Object object;
            if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-34, "Asset not found");

            if(config::fClient.load() && object.hashOwner != users->GetCallersGenesis(params))
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");


            /* Only include non-standard object types */
            if(object.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* parse object so that the data fields can be accessed */
                if(!object.Parse())
                    throw APIException(-36, "Failed to parse object register");

                /* Only include non standard object registers (assets) */
                if(object.Standard() != TAO::Register::OBJECTS::NONSTANDARD)
                    throw APIException(-35, "Specified name/address is not an asset.");
            }
            else
                throw APIException(-35, "Specified name/address is not an asset.");

            /* Populate the response JSON */
            /* Get List of field names in this asset object */
            std::vector<std::string> vFieldNames = object.GetFieldNames();

            /* Declare type and data variables for unpacking the Object fields */
            for(const auto& strName : vFieldNames)
            {
                /* JSON for this field */
                json::json field;

                /* max field size (only applicable to mutable fields) */
                std::size_t nMaxSize = 0;

                /* Add the name */
                field["name"] = strName;

                /* Get the type */
                uint8_t nType = 0;
                object.Type(strName, nType);

                /* Switch based on type. */
                switch(nType)
                {
                    /* Check for uint8_t type. */
                    case TAO::Register::TYPES::UINT8_T:
                    {
                        /* Add the type */
                        field["type"] = "uint8";

                        /* Set the return value from object register data. */
                        field["value"] = object.get<uint8_t>(strName);

                        break;
                    }

                    /* Check for uint16_t type. */
                    case TAO::Register::TYPES::UINT16_T:
                    {
                        /* Add the type */
                        field["type"] = "uint16";

                        /* Set the return value from object register data. */
                        field["value"] = object.get<uint16_t>(strName);

                        break;
                    }

                    /* Check for uint32_t type. */
                    case TAO::Register::TYPES::UINT32_T:
                    {
                        /* Add the type */
                        field["type"] = "uint32";

                        /* Set the return value from object register data. */
                        field["value"] = object.get<uint32_t>(strName);

                        break;
                    }

                    /* Check for uint64_t type. */
                    case TAO::Register::TYPES::UINT64_T:
                    {
                        /* Add the type */
                        field["type"] = "uint64";

                        /* Set the return value from object register data. */
                        field["value"] = object.get<uint64_t>(strName);

                        break;
                    }

                    /* Check for uint256_t type. */
                    case TAO::Register::TYPES::UINT256_T:
                    {
                        /* Add the type */
                        field["type"] = "uint256";

                        /* Set the return value from object register data. */
                        field["value"] = object.get<uint256_t>(strName).GetHex();

                        break;
                    }

                    /* Check for uint512_t type. */
                    case TAO::Register::TYPES::UINT512_T:
                    {
                        /* Add the type */
                        field["type"] = "uint512";

                        /* Set the return value from object register data. */
                        field["value"] = object.get<uint512_t>(strName).GetHex();

                        break;
                    }

                    /* Check for uint1024_t type. */
                    case TAO::Register::TYPES::UINT1024_T:
                    {
                        /* Add the type */
                        field["type"] = "uint1024";

                        /* Set the return value from object register data. */
                        field["value"] = object.get<uint1024_t>(strName).GetHex();

                        break;
                    }

                    /* Check for string type. */
                    case TAO::Register::TYPES::STRING:
                    {
                        /* Add the type */
                        field["type"] = "string";

                        /* Set the return value from object register data. */
                        std::string strRet = object.get<std::string>(strName);

                        /* get the size */
                        nMaxSize = strRet.length();

                        /* Remove trailing nulls from the data, which are padding to maxlength on mutable fields */
                        field["value"] = strRet.substr(0, strRet.find_last_not_of('\0') + 1);


                        break;
                    }

                    /* Check for bytes type. */
                    case TAO::Register::TYPES::BYTES:
                    {
                        /* Add the type */
                        field["type"] = "bytes";

                        /* Set the return value from object register data. */
                        std::vector<uint8_t> vRet = object.get<std::vector<uint8_t>>(strName);

                        /* get the size */
                        nMaxSize = vRet.size();

                        /* Remove trailing nulls from the data, which are padding to maxlength on mutable fields */
                        vRet.erase(std::find(vRet.begin(), vRet.end(), '\0'), vRet.end());

                        /* Add to return value in base64 encoding. */
                        field["value"] = encoding::EncodeBase64(&vRet[0], vRet.size()) ;

                        break;
                    }
                }

                /* Add mutable flag */
                field["mutable"] = object.mapData[strName].second;

                /* If mutable, add the max size */
                if(object.mapData[strName].second && nMaxSize > 0)
                    field["maxlength"] = nMaxSize;

                /* Add the field to the response array */
                ret.push_back(field);
            }

            return ret;
        }
    }
}
