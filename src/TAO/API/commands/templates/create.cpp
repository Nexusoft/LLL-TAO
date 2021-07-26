/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/templates.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/execute.h>
#include <TAO/API/include/extract.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/create.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/types/address.h>

#include <Util/templates/datastream.h>

#include <Util/include/string.h>
#include <Util/include/base64.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Create an tPayload or digital item. */
    encoding::json Templates::Create(const encoding::json& jParams, const bool fHelp,
                                     const std::string& strAllowed, const uint16_t nType)
    {
        /* Generate our address based on formatting type. */
        uint256_t hashRegister;

        /* Check for format parameter. */
        const std::string strFormat =
            ExtractFormat(jParams, "", strAllowed);

        /* Handle for the raw specifier. */
        std::vector<TAO::Operation::Contract> vContracts(1);

        /* Handle for readonly raw format. */
        if(strFormat == "readonly")
        {
            /* Set the proper tPayload type. */
            hashRegister = TAO::Register::Address(TAO::Register::Address::READONLY);

            /* Check for our data parameter. */
            if(!CheckParameter(jParams, "data", "string"))
                throw Exception(-28, "Missing parameter [data] for command");

            /* Serialise the incoming data into a state register */
            DataStream ssData(SER_REGISTER, 1);
            ssData << uint16_t(nType) << jParams["data"].get<std::string>();

            /* Submit the payload object. */
            vContracts[0] << uint8_t(TAO::Operation::OP::CREATE)   << hashRegister;
            vContracts[0] << uint8_t(TAO::Register::REGISTER::READONLY) << ssData.Bytes();
        }

        /* Handle for raw formats. */
        if(strFormat == "raw")
        {
            /* Set the proper payload type. */
            hashRegister = TAO::Register::Address(TAO::Register::Address::RAW);

            /* Check for our data parameter. */
            if(!CheckParameter(jParams, "data", "string"))
                throw Exception(-28, "Missing parameter [data] for command");

            /* Get our payload now. */
            std::string strPayload =
                jParams["data"].get<std::string>();

            /* If the caller specifies a maxlength then use this to set the size of the string */
            const uint64_t nMaxLength =
                ExtractInteger<uint64_t>(jParams, "maxlength", ((strPayload.size() / 128) + 1) * 128, 1000); //128 bytes default padding

            /* Check for minimum ranges. */
            if(nMaxLength < strPayload.size())
                throw Exception(-60, "[data] out of range [", strPayload.size(), "]");

            /* Adjust our serialization length. */
            strPayload.resize(nMaxLength);

            /* Serialise the incoming data into a state register */
            DataStream ssData(SER_REGISTER, 1);
            ssData << uint16_t(nType) << strPayload;

            /* Submit the payload object. */
            vContracts[0] << uint8_t(TAO::Operation::OP::CREATE)   << hashRegister;
            vContracts[0] << uint8_t(TAO::Register::REGISTER::RAW) << ssData.Bytes();
        }

        /* Handle for basic formats. */
        if(strFormat == "basic")
        {
            /* Declare the object register to hold the payload data*/
            TAO::Register::Object tPayload =
                BuildObject(jParams, hashRegister);

            /* Track the number of fields */
            uint32_t nFieldCount = 0;

            /* Iterate through the paramers and infer the type for each value */
            for(auto it = jParams.begin(); it != jParams.end(); ++it)
            {
                /* Get our keyname. */
                const std::string strField = it.key();

                /* Skip any incoming parameters that are keywords used by this API method*/
                if(ToLower(strField) == "pin"
                || ToLower(strField) == "session"
                || ToLower(strField) == "name"
                || ToLower(strField) == "format"
                || ToLower(strField) == "maxlength"
                || ToLower(strField) == "request")
                {
                    continue;
                }

                /* Make sure the name is not reserved. */
                if(TAO::Register::Reserved(strField))
                    throw Exception(-22, "Field [", strField, "] is a reserved field name");

                /* Grab our value name. */
                std::string strPayload;

                /* Handle if string. */
                if(it->is_string())
                    strPayload = it->get<std::string>();

                /* Handle if number. */
                if(it->is_number())
                {
                    /* Handle for unsigned. */
                    if(it->is_number_unsigned())
                        strPayload = debug::safe_printstr(it->get<uint64_t>());

                    /* Handle for signed. */
                    else if(it->is_number_integer())
                        strPayload = debug::safe_printstr(it->get<int64_t>());

                    /* Handle for float. */
                    else if(it->is_number_float())
                        strPayload = debug::safe_printstr(it->get<double>());
                }

                /* Check that parameter was converted correctly. */
                if(strPayload.empty())
                    throw Exception(-19, "Invalid type [", strField, "=", it->type_name(), "] for command");

                /* If the caller specifies a maxlength then use this to set the size of the string */
                const uint64_t nMaxLength =
                    ExtractInteger<uint64_t>(jParams, "maxlength", ((strPayload.size() / 32) + 1) * 32, 1000); //32 bytes default padding

                /* Check for minimum ranges. */
                if(nMaxLength < strPayload.size())
                    throw Exception(-60, "[", strField, "] out of range [", strPayload.size(), "]");

                /* Adjust our serialization length. */
                strPayload.resize(nMaxLength);

                /* Add our payload to the tPayload. */
                tPayload << strField << uint8_t(TAO::Register::TYPES::MUTABLE);
                tPayload << uint8_t(TAO::Register::TYPES::STRING) << strPayload;

                /* Keep track of our added fields. */
                ++nFieldCount;
            }

            /* Check for missing parameters. */
            if(nFieldCount == 0)
                throw Exception(-28, "Missing parameter [field=string] for command");

            /* Submit the payload object. */
            vContracts[0] << uint8_t(TAO::Operation::OP::CREATE)      << hashRegister;
            vContracts[0] << uint8_t(TAO::Register::REGISTER::OBJECT) << tPayload.GetState();
        }

        /* Handle for the json formats. */
        if(strFormat == "json")
        {
            /* Check for our data parameter. */
            if(!CheckParameter(jParams, "json", "array"))
                throw Exception(-28, "Missing parameter [json] for command");

            /* Declare the object register to hold the payload data*/
            TAO::Register::Object tPayload =
                BuildObject(jParams, hashRegister);

            /* Grab our schema from input parameters. */
            encoding::json jSchema = jParams["json"];

            /* Check our data-set size. */
            if(jSchema.empty())
                throw Exception(-28, "Missing parameter [json] for command");

            /* Iterate through each field definition */
            for(auto it = jSchema.begin(); it != jSchema.end(); ++it)
            {
                /* Check for our name parameter. */
                if(!CheckParameter((*it), "name", "string"))
                    throw Exception(-28, "Missing parameter [name] for command");

                /* Check for our type parameter. */
                if(!CheckParameter((*it), "type", "string"))
                    throw Exception(-28, "Missing parameter [type] for command");

                /* Check for our value parameter. */
                if(!CheckParameter((*it), "value", "string, number"))
                    throw Exception(-28, "Missing parameter [value] for command");

                /* Check for our mutable parameter. */
                if(!CheckParameter((*it), "mutable", "string"))
                    throw Exception(-28, "Missing parameter [mutable] for command");

                /* Grab our name string from parameter */
                const std::string strName =
                    (*it)["name"].get<std::string>();

                /* Make sure the name is not reserved. */
                if(TAO::Register::Reserved(strName))
                    throw Exception(-22, "Field [", strName, "] is a reserved field name");

                /* Add to the payload. */
                tPayload << strName;

                /* Check for mutable. */
                if(((*it)["mutable"].get<std::string>() == "true"))
                    tPayload << uint8_t(TAO::Register::TYPES::MUTABLE);

                /* Grab our type string from parameter */
                const std::string strType =
                    (*it)["type"].get<std::string>();

                /* Handle for 8-bit unsigned int. */
                if(strType == "uint8")
                    tPayload << uint8_t(TAO::Register::TYPES::UINT8_T) << ExtractInteger<uint8_t>((*it), "value");

                /* Handle for 16-bit unsigned int. */
                if(strType == "uint16")
                    tPayload << uint8_t(TAO::Register::TYPES::UINT16_T) << ExtractInteger<uint16_t>((*it), "value");

                /* Handle for 32-bit unsigned int. */
                if(strType == "uint32")
                    tPayload << uint8_t(TAO::Register::TYPES::UINT32_T) << ExtractInteger<uint32_t>((*it), "value");

                /* Handle for 64-bit unsigned int. */
                if(strType == "uint64")
                    tPayload << uint8_t(TAO::Register::TYPES::UINT64_T) << ExtractInteger<uint64_t>((*it), "value");

                /* Handle for 256-bit unsigned int. */
                if(strType == "uint256")
                    tPayload << uint8_t(TAO::Register::TYPES::UINT256_T) << ExtractHash<uint256_t>((*it), "value");

                /* Handle for 512-bit unsigned int. */
                if(strType == "uint512")
                    tPayload << uint8_t(TAO::Register::TYPES::UINT512_T) << ExtractHash<uint512_t>((*it), "value");

                /* Handle for 1024-bit unsigned int. */
                if(strType == "uint1024")
                    tPayload << uint8_t(TAO::Register::TYPES::UINT512_T) << ExtractHash<uint1024_t>((*it), "value");

                /* Handle for string data. */
                if(strType == "string")
                {
                    /* Grab our payload. */
                    std::string strPayload =
                        (*it)["value"].get<std::string>();

                    /* If the caller specifies a maxlength then use this to set the size of the string */
                    const uint64_t nMaxLength =
                        ExtractInteger<uint64_t>((*it), "maxlength", ((strPayload.size() / 64) + 1) * 64, 1000);

                    /* Check for minimum ranges. */
                    if(nMaxLength < strPayload.size())
                        throw Exception(-60, "[", strName, "] out of range [", strPayload.size(), "]");

                    /* Adjust our serialization length. */
                    strPayload.resize(nMaxLength);

                    /* Add to the payload now. */
                    tPayload << uint8_t(TAO::Register::TYPES::STRING) << strPayload;
                }

                /* Handle for standard binary data. */
                if(strType == "bytes")
                {
                    /* Track if we have invalid encoding. */
                    bool fInvalid = false;

                    /* Grab our payload. */
                    const std::vector<uint8_t> vPayload =
                        encoding::DecodeBase64((*it)["value"].get<std::string>().c_str(), &fInvalid);

                    /* Check for invalid payload. */
                    if(fInvalid)
                        throw Exception(-35, "Invalid parameter [json.value], expecting [base64]");

                    /* Add to the payload now. */
                    tPayload << uint8_t(TAO::Register::TYPES::BYTES) << vPayload;
                }
            }

            /* Submit the payload object. */
            vContracts[0] << uint8_t(TAO::Operation::OP::CREATE)      << hashRegister;
            vContracts[0] << uint8_t(TAO::Register::REGISTER::OBJECT) << tPayload.GetState();
        }

        /* Build our object first to check against our standards. */
        TAO::Register::Object tObject =
            ExecuteContract(vContracts[0]);

        /* Check that our formatting matches our standard. */
        if(!CheckStandard(jParams, tObject))
            throw Exception(-18, "Invalid format for standard [", jParams["request"]["type"].get<std::string>(), "]");

        /* Add optional name if specified. */
        BuildName(jParams, hashRegister, vContracts);

        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
