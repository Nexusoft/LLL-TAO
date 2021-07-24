/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/include/build.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/commands/assets.h>

#include <TAO/API/include/extract.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/templates/datastream.h>

#include <Util/include/convert.h>
#include <Util/include/base64.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Create an asset or digital item. */
    encoding::json Assets::Create(const encoding::json& jParams, const bool fHelp)
    {
        /* Generate a random hash for this objects register address */
        TAO::Register::Address hashRegister;

        /* Check for format parameter. */
        const std::string strFormat = ExtractFormat(jParams, "raw, basic, json");

        /* Handle for the raw specifier. */
        std::vector<TAO::Operation::Contract> vContracts(1);
        if(strFormat == "raw")
        {
            /* Set the proper asset type. */
            hashRegister = TAO::Register::Address(TAO::Register::Address::RAW);

            /* If format = raw then use a raw state register rather than an object */
            if(jParams.find("data") == jParams.end())
                throw Exception(-18, "Missing data");

            /* Check data is in a string field */
            if(!jParams["data"].is_string())
                throw Exception(-19, "Data must be a string with this asset format.");

            /* Serialise the incoming data into a state register*/
            DataStream ssData(SER_REGISTER, 1);

            /* Then the raw data */
            ssData << jParams["data"].get<std::string>();

            /* Submit the payload object. */
            vContracts[0] << uint8_t(TAO::Operation::OP::CREATE) << hashRegister;
            vContracts[0] << uint8_t(TAO::Register::REGISTER::RAW) << ssData.Bytes();
        }
        else if(strFormat == "basic")
        {
            /* Set the proper asset type. */
            hashRegister = TAO::Register::Address(TAO::Register::Address::OBJECT);

            /* declare the object register to hold the asset data*/
            TAO::Register::Object asset = TAO::Register::CreateAsset();

            /* Track the number of fields so that we can check there is at least one */
            uint32_t nFieldCount = 0;

            /* Iterate through the paramers and infer the type for each value */
            for(auto it = jParams.begin(); it != jParams.end(); ++it)
            {
                /* Skip any incoming parameters that are keywords used by this API method*/
                if(it.key() == "pin"
                || it.key() == "PIN"
                || it.key() == "session"
                || it.key() == "name"
                || it.key() == "format"
                || it.key() == "token_name"
                || it.key() == "token_value")
                {
                    continue;
                }

                /* Handle switch for string types in BASIC encoding */
                if(it->is_string())
                {
                    ++nFieldCount;

                    std::string strValue = it->get<std::string>();
                    asset << it.key() << uint8_t(TAO::Register::TYPES::STRING) << strValue;
                }
                else
                    throw Exception(-19, "Data must be a string with this asset format.");
            }

            if(nFieldCount == 0)
                throw Exception(-20, "Missing asset value fields.");

            /* Submit the payload object. */
            vContracts[0] << uint8_t(TAO::Operation::OP::CREATE) << hashRegister;
            vContracts[0] << uint8_t(TAO::Register::REGISTER::OBJECT) << asset.GetState();
        }
        else if(strFormat == "JSON")
        {
            /* Set the proper asset type. */
            hashRegister = TAO::Register::Address(TAO::Register::Address::OBJECT);

            /* If format = JSON then grab the asset definition from the json field */
            if(jParams.find("json") == jParams.end())
                throw Exception(-21, "Missing json parameter.");

            if(!jParams["json"].is_array())
                throw Exception(-22, "json field must be an array.");

            /* declare the object register to hold the asset data*/
            TAO::Register::Object asset = TAO::Register::CreateAsset();

            encoding::json jsonAssetDefinition = jParams["json"];

            /* Track the number of fields so that we can check there is at least one */
            int nFieldCount = 0;

            /* Iterate through each field definition */
            for(auto it = jsonAssetDefinition.begin(); it != jsonAssetDefinition.end(); ++it)
            {
                /* Check that the required fields have been provided*/
                if(it->find("name") == it->end())
                    throw Exception(-22, "Missing name field in json definition.");

                if(it->find("type") == it->end())
                    throw Exception(-23, "Missing type field in json definition.");

                if(it->find("value") == it->end())
                    throw Exception(-24, "Missing value field in json definition.");

                if(it->find("mutable") == it->end())
                    throw Exception(-25, "Missing mutable field in json definition.");

                /* Parse the values out of the definition json*/
                std::string strName =  (*it)["name"].get<std::string>();
                std::string strType =  (*it)["type"].get<std::string>();
                std::string strValue = (*it)["value"].get<std::string>();
                bool fMutable = (*it)["mutable"].get<std::string>() == "true";
                bool fBytesInvalid = false;
                std::vector<uint8_t> vchBytes;

                /* Convert the value to bytes if the type is bytes */
                if(strType == "bytes")
                {
                    try
                    {
                        vchBytes = encoding::DecodeBase64(strValue.c_str(), &fBytesInvalid);
                    }
                    catch(const std::exception& e)
                    {
                        fBytesInvalid = true;
                    }


                }
                /* Declare the max length variable */
                size_t nMaxLength = 0;

                if(strType == "string" || strType == "bytes")
                {
                    /* Determine the length of the data passed in */
                    std::size_t nDataLength = strType == "string" ? strValue.length() : vchBytes.size();

                    /* If this is a mutable string or byte fields then set the length.
                    This can either be set by the caller in a  maxlength field or we will default it
                    based on the field data type.` */
                    if(fMutable)
                    {
                        /* Determine the length of the data passed in */
                        std::size_t nDataLength = strType == "string" ? strValue.length() : vchBytes.size();

                        /* If the caller specifies a maxlength then use this to set the size of the string or bytes array */
                        if(it->find("maxlength") != it->end())
                        {
                            nMaxLength = std::stoul((*it)["maxlength"].get<std::string>());

                            /* If they specify a value less than the data length then error */
                            if(nMaxLength < nDataLength)
                                throw Exception(-26, "maxlength value is less than the specified data length.");
                        }
                        else
                        {
                            /* If the caller hasn't specified a maxlength then set a suitable default
                            by rounding up the current length to the nearest 64 bytes. */
                            nMaxLength = (((uint8_t)(nDataLength / 64)) +1) * 64;
                        }
                    }
                    else
                    {
                        /* If the field is not mutable then the max length is simply the data length */
                        nMaxLength = nDataLength;
                    }
                }

                /* Serialize the data field name */
                asset << strName;

                /* Add the mutable flag if defined */
                if(fMutable)
                    asset << uint8_t(TAO::Register::TYPES::MUTABLE);

                /* lastly add the data type and initial value*/
                if(strType == "uint8")
                    asset << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(stoul(strValue));
                else if(strType == "uint16")
                    asset << uint8_t(TAO::Register::TYPES::UINT16_T) << uint16_t(stoul(strValue));
                else if(strType == "uint32")
                    asset << uint8_t(TAO::Register::TYPES::UINT32_T) << uint32_t(stoul(strValue));
                else if(strType == "uint64")
                    asset << uint8_t(TAO::Register::TYPES::UINT64_T) << uint64_t(stoul(strValue));
                else if(strType == "uint256")
                    asset << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(strValue);
                else if(strType == "uint512")
                    asset << uint8_t(TAO::Register::TYPES::UINT512_T) << uint512_t(strValue);
                else if(strType == "uint1024")
                    asset << uint8_t(TAO::Register::TYPES::UINT1024_T) << uint1024_t(strValue);
                else if(strType == "string")
                {
                    /* Ensure that the serialized value is padded out to the max length */
                    strValue.resize(nMaxLength);

                    asset << uint8_t(TAO::Register::TYPES::STRING) << strValue;
                }
                else if(strType == "bytes")
                {
                    if(fBytesInvalid)
                        throw Exception(-27, "Malformed base64 encoding");

                    /* Ensure that the serialized value is padded out to the max length */
                    vchBytes.resize(nMaxLength);

                    asset << uint8_t(TAO::Register::TYPES::BYTES) << vchBytes;
                }
                else
                {
                    throw Exception(-154, "Invalid field type " + strType);
                }


                /* Increment total fields. */
                ++nFieldCount;
            }

            if(nFieldCount == 0)
                throw Exception(-28, "Missing asset field definitions");

            /* Submit the payload object. */
            vContracts[0] << uint8_t(TAO::Operation::OP::CREATE) << hashRegister;
            vContracts[0] << uint8_t(TAO::Register::REGISTER::OBJECT) << asset.GetState();
        }

        /* Add optional name if specified. */
        BuildName(jParams, hashRegister, vContracts);

        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
