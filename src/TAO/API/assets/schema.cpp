/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/assets/types/assets.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/json.h>

#include <Util/include/base64.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the schema for an asset */
    encoding::json Assets::GetSchema(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the Register ID. */
        const uint256_t hashRegister = ExtractAddress(jParams);

        /* Get the asset from the register DB. */
        TAO::Register::Object tObject;
        if(!LLD::Register->ReadObject(hashRegister, tObject, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-13, "Object not found");

        /* Now lets check our expected types match. */
        if(!CheckStandard(jParams, tObject))
            throw Exception(-49, "Unsupported type for name/address");

        /* Get List of field names in this asset object */
        const std::vector<std::string> vFieldNames = tObject.ListFields();

        /* Declare type and data variables for unpacking the Object fields */
        encoding::json jRet = encoding::json::array();
        for(const auto& strName : vFieldNames)
        {
            /* max field size (only applicable to mutable fields) */
            std::size_t nMaxSize = 0;

            /* Add the name */
            encoding::json jField =
                {{ "name", strName }};

            /* Get the type */
            uint8_t nType = 0;
            tObject.Type(strName, nType);

            /* Switch based on type. */
            switch(nType)
            {
                /* Check for uint8_t type. */
                case TAO::Register::TYPES::UINT8_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint8";
                    jField["value"] = tObject.get<uint8_t>(strName);

                    break;
                }

                /* Check for uint16_t type. */
                case TAO::Register::TYPES::UINT16_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint16";
                    jField["value"] = tObject.get<uint16_t>(strName);

                    break;
                }

                /* Check for uint32_t type. */
                case TAO::Register::TYPES::UINT32_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint32";
                    jField["value"] = tObject.get<uint32_t>(strName);

                    break;
                }

                /* Check for uint64_t type. */
                case TAO::Register::TYPES::UINT64_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint64";
                    jField["value"] = tObject.get<uint64_t>(strName);

                    break;
                }

                /* Check for uint256_t type. */
                case TAO::Register::TYPES::UINT256_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint256";
                    jField["value"] = tObject.get<uint256_t>(strName).GetHex();

                    break;
                }

                /* Check for uint512_t type. */
                case TAO::Register::TYPES::UINT512_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint512";
                    jField["value"] = tObject.get<uint512_t>(strName).GetHex();

                    break;
                }

                /* Check for uint1024_t type. */
                case TAO::Register::TYPES::UINT1024_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint1024";
                    jField["value"] = tObject.get<uint1024_t>(strName).GetHex();

                    break;
                }

                /* Check for string type. */
                case TAO::Register::TYPES::STRING:
                {
                    /* Add the type */
                    jField["type"] = "string";

                    /* Set the return value from object register data. */
                    const std::string strRet =
                        tObject.get<std::string>(strName);

                    /* get the size */
                    nMaxSize = strRet.length();

                    /* Remove trailing nulls from the data, which are padding to maxlength on mutable fields */
                    jField["value"] = strRet.substr(0, strRet.find_last_not_of('\0') + 1);

                    break;
                }

                /* Check for bytes type. */
                case TAO::Register::TYPES::BYTES:
                {
                    /* Add the type */
                    jField["type"] = "bytes";

                    /* Set the return value from object register data. */
                    std::vector<uint8_t> vRet =
                        tObject.get<std::vector<uint8_t>>(strName);

                    /* get the size */
                    nMaxSize = vRet.size();

                    /* Remove trailing nulls from the data, which are padding to maxlength on mutable fields */
                    vRet.erase(std::find(vRet.begin(), vRet.end(), '\0'), vRet.end());

                    /* Add to return value in base64 encoding. */
                    jField["value"] = encoding::EncodeBase64(&vRet[0], vRet.size()) ;

                    break;
                }
            }

            /* Add mutable flag */
            jField["mutable"] = tObject.mapData[strName].second;

            /* If mutable, add the max size */
            if(tObject.mapData[strName].second && nMaxSize > 0)
                jField["maxlength"] = nMaxSize;

            /* Add the field to the response array */
            jRet.push_back(jField);
        }

        return jRet;
    }
}
