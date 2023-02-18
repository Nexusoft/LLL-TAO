/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/assets.h>

#include <Util/include/base64.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns the JSON representation of this invoice */
    encoding::json Assets::SchemaToJSON(const TAO::Register::Object& rObject, const uint256_t& hashRegister)
    {
        /* Get List of field names in this asset object */
        const std::vector<std::string> vMembers = rObject.Members();

        /* Declare type and data variables for unpacking the Object fields */
        encoding::json jRet = encoding::json::array();
        for(const auto& strMember : vMembers)
        {
            /* max field size (only applicable to mutable fields) */
            std::size_t nMaxSize = 0;

            /* Add the name */
            encoding::json jField =
                {{ "name", strMember }};

            /* Get the type */
            uint8_t nType = 0;
            rObject.Type(strMember, nType);

            /* Switch based on type. */
            switch(nType)
            {
                /* Check for uint8_t type. */
                case TAO::Register::TYPES::UINT8_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint8";
                    jField["value"] = rObject.get<uint8_t>(strMember);

                    break;
                }

                /* Check for uint16_t type. */
                case TAO::Register::TYPES::UINT16_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint16";
                    jField["value"] = rObject.get<uint16_t>(strMember);

                    break;
                }

                /* Check for uint32_t type. */
                case TAO::Register::TYPES::UINT32_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint32";
                    jField["value"] = rObject.get<uint32_t>(strMember);

                    break;
                }

                /* Check for uint64_t type. */
                case TAO::Register::TYPES::UINT64_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint64";
                    jField["value"] = rObject.get<uint64_t>(strMember);

                    break;
                }

                /* Check for uint256_t type. */
                case TAO::Register::TYPES::UINT256_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint256";
                    jField["value"] = rObject.get<uint256_t>(strMember).GetHex();

                    break;
                }

                /* Check for uint512_t type. */
                case TAO::Register::TYPES::UINT512_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint512";
                    jField["value"] = rObject.get<uint512_t>(strMember).GetHex();

                    break;
                }

                /* Check for uint1024_t type. */
                case TAO::Register::TYPES::UINT1024_T:
                {
                    /* Set the return value from object register data. */
                    jField["type"]  = "uint1024";
                    jField["value"] = rObject.get<uint1024_t>(strMember).GetHex();

                    break;
                }

                /* Check for string type. */
                case TAO::Register::TYPES::STRING:
                {
                    /* Add the type */
                    jField["type"] = "string";

                    /* Set the return value from object register data. */
                    const std::string strRet =
                        rObject.get<std::string>(strMember);

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
                        rObject.get<std::vector<uint8_t>>(strMember);

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
            jField["mutable"] = rObject.Mutable(strMember);

            /* If mutable, add the max size */
            if(rObject.Mutable(strMember) && nMaxSize > 0)
                jField["maxlength"] = nMaxSize;

            /* Add the field to the response array */
            jRet.push_back(jField);
        }

        return jRet;
    }
}
