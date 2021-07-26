/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/templates.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/types/object.h>

#include <Util/templates/datastream.h>

#include <Util/include/string.h>
#include <Util/include/base64.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Update the data in an asset */
    encoding::json Templates::Update(const encoding::json& jParams, const bool fHelp,
                                     const std::string& strAllowed, const uint16_t nUserType)
    {
        /* Get the Register address. */
        const uint256_t hashRegister =
            ExtractAddress(jParams);

        /* Check for format parameter. */
        const std::string strFormat =
            ExtractFormat(jParams, "", strAllowed);

        /* Get the token / account object. */
        TAO::Register::Object tObject;
        if(!LLD::Register->ReadObject(hashRegister, tObject, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-13, "Object not found");

        /* Now lets check our expected types match. */
        if(!CheckStandard(jParams, tObject))
            throw Exception(-49, "Unsupported type for name/address");

        /* Handle for the raw specifier. */
        std::vector<TAO::Operation::Contract> vContracts(1);

        /* Handle for raw format. */
        if(strFormat == "raw")
        {
            /* Check for our data parameter. */
            if(!CheckParameter(jParams, "data", "string"))
                throw Exception(-28, "Missing parameter [data] for command");

            /* Build a stream to deserlialize some data. */
            DataStream ssObject(tObject.GetState(), SER_REGISTER, 1);

            /* Deserialize state to get sizes. */
            uint16_t nState = 0;
            ssObject >> nState;

            /* Grab our previous string size now. */
            const uint32_t nMaxLength =
                ReadCompactSize(ssObject);

            /* Grab our raw data string. */
            std::string strValue =
                jParams["data"].get<std::string>();

            /* Make sure we are writing the correct size. */
            if(strValue.length() > nMaxLength)
                throw Exception(-158, "Field [data] size exceeds maximum length [", nMaxLength, "]");

            /* Resize our buffer with null data. */
            strValue.resize(nMaxLength);

            /* Serialise the incoming data into a state register */
            DataStream ssData(SER_REGISTER, 1);
            ssData << uint16_t(nUserType) << strValue;

            /* Submit the payload object. */
            vContracts[0] << uint8_t(TAO::Operation::OP::WRITE) << hashRegister << ssData.Bytes();
        }

        /* Handle for basic formats. */
        if(strFormat == "basic")
        {
            /* Create an operations stream for the update payload. */
            TAO::Operation::Stream ssPayload;

            /* Iterate through the paramers and infer the type for each value */
            for(auto it = jParams.begin(); it != jParams.end(); ++it)
            {
                /* Get our keyname. */
                const std::string strField = it.key();

                /* Skip any incoming parameters that are keywords used by this API method*/
                if(ToLower(strField) == "pin"
                || ToLower(strField) == "session"
                || ToLower(strField) == "name"
                || ToLower(strField) == "address"
                || ToLower(strField) == "format"
                || ToLower(strField) == "request")
                {
                    continue;
                }

                /* Make sure the name is not reserved. */
                if(TAO::Register::Reserved(strField))
                    throw Exception(-22, "Field [", strField, "] is a reserved field name");

                /* Grab our value name. */
                std::string strValue;

                /* Handle if string. */
                if(it->is_string())
                    strValue = it->get<std::string>();

                /* Handle if number. */
                if(it->is_number())
                {
                    /* Handle for unsigned. */
                    if(it->is_number_unsigned())
                        strValue = debug::safe_printstr(it->get<uint64_t>());

                    /* Handle for signed. */
                    else if(it->is_number_integer())
                        strValue = debug::safe_printstr(it->get<int64_t>());

                    /* Handle for float. */
                    else if(it->is_number_float())
                        strValue = debug::safe_printstr(it->get<double>());
                }

                /* Check that parameter was converted correctly. */
                if(strValue.empty())
                    throw Exception(-19, "Invalid type [", strField, "=", it->type_name(), "] for command");

                /* Check that field exists first but grab a value too. */
                const uint64_t nMaxLength = tObject.Size(strField);
                if(nMaxLength == 0)
                    throw Exception(-19, "Field [", strField, "] doesn't exist in object");

                /* Make sure we are writing the correct size. */
                if(strValue.length() > nMaxLength)
                    throw Exception(-158, "Field [", strField, "] size exceeds maximum length [", nMaxLength, "]");

                /* Check our object for data field. */
                if(!tObject.Check(strField, TAO::Register::TYPES::STRING, true))
                    throw Exception(-20, "Field [", strField, "] type is invalid for update, expecting [mutable string]");

                /* Resize our value to add the required padding. */
                strValue.resize(nMaxLength);

                /* Add our payload to the contract. */
                ssPayload << strField << uint8_t(TAO::Operation::OP::TYPES::STRING) << strValue;
            }

            /* Check for missing parameters. */
            if(ssPayload.size() == 0)
                throw Exception(-28, "Missing parameter [field=string] for command");

            /* Submit the payload object. */
            vContracts[0] << uint8_t(TAO::Operation::OP::WRITE) << hashRegister << ssPayload.Bytes();
        }

        /* Handle for json formats. */
        if(strFormat == "json")
        {
            /* Create an operations stream for the update payload. */
            TAO::Operation::Stream ssPayload;

            /* Iterate through the paramers and infer the type for each value */
            for(auto it = jParams.begin(); it != jParams.end(); ++it)
            {
                /* Get our keyname. */
                const std::string strField = it.key();

                /* Skip any incoming parameters that are keywords used by this API method*/
                if(ToLower(strField) == "pin"
                || ToLower(strField) == "session"
                || ToLower(strField) == "name"
                || ToLower(strField) == "address"
                || ToLower(strField) == "format"
                || ToLower(strField) == "request")
                {
                    continue;
                }

                /* Make sure the name is not reserved. */
                if(TAO::Register::Reserved(strField))
                    throw Exception(-22, "Field [", strField, "] is a reserved field name");

                /* Grab our field datatype. */
                uint8_t nFieldType = 0;
                if(!tObject.Type(strField, nFieldType))
                    throw Exception(-19, "Field [", strField, "] doesn't exist in object");

                /* Check our object for data field. */
                if(!tObject.Check(strField, nFieldType, true))
                    throw Exception(-20, "Field [", strField, "] type is invalid for update, expecting [mutable]");

                /* Add our fieldname payload first. */
                ssPayload << strField;

                /* Handle for 8-bit unsigned int. */
                if(nFieldType == TAO::Register::TYPES::UINT8_T)
                    ssPayload << uint8_t(TAO::Operation::OP::TYPES::UINT8_T) << ExtractInteger<uint8_t>(jParams, strField);

                /* Handle for 16-bit unsigned int. */
                if(nFieldType == TAO::Register::TYPES::UINT16_T)
                    ssPayload << uint8_t(TAO::Operation::OP::TYPES::UINT16_T) << ExtractInteger<uint16_t>(jParams, strField);

                /* Handle for 32-bit unsigned int. */
                if(nFieldType == TAO::Register::TYPES::UINT32_T)
                    ssPayload << uint8_t(TAO::Operation::OP::TYPES::UINT32_T) << ExtractInteger<uint32_t>(jParams, strField);

                /* Handle for 64-bit unsigned int. */
                if(nFieldType == TAO::Register::TYPES::UINT64_T)
                    ssPayload << uint8_t(TAO::Operation::OP::TYPES::UINT64_T) << ExtractInteger<uint64_t>(jParams, strField);

                /* Handle for 256-bit unsigned int. */
                if(nFieldType == TAO::Register::TYPES::UINT256_T)
                    ssPayload << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << ExtractHash<uint256_t>(jParams, strField);

                /* Handle for 512-bit unsigned int. */
                if(nFieldType == TAO::Register::TYPES::UINT512_T)
                    ssPayload << uint8_t(TAO::Operation::OP::TYPES::UINT512_T) << ExtractHash<uint512_t>(jParams, strField);

                /* Handle for 1024-bit unsigned int. */
                if(nFieldType == TAO::Register::TYPES::UINT1024_T)
                    ssPayload << uint8_t(TAO::Operation::OP::TYPES::UINT1024_T) << ExtractHash<uint1024_t>(jParams, strField);

                /* Handle for regular utf-8 string. */
                if(nFieldType == TAO::Register::TYPES::STRING)
                {
                    /* Handle if string. */
                    if(!it->is_string())
                        throw Exception(-19, "Invalid type [", strField, "=", it->type_name(), "] for command");

                    /* Grab our value string to check against. */
                    std::string strValue =
                        it->get<std::string>();

                    /* Check that field exists first but grab a value too. */
                    const uint64_t nMaxLength = tObject.Size(strField);
                    if(nMaxLength == 0)
                        throw Exception(-19, "Field [", strField, "] doesn't exist in object");

                    /* Make sure we are writing the correct size. */
                    if(strValue.length() > nMaxLength)
                        throw Exception(-158, "Field [", strField, "] size exceeds maximum length [", nMaxLength, "]");

                    /* Resize our value to add the required padding. */
                    strValue.resize(nMaxLength);

                    /* Add our payload to the contract. */
                    ssPayload << uint8_t(TAO::Operation::OP::TYPES::STRING) << strValue;
                }

                /* Handle for standard binary data. */
                if(nFieldType == TAO::Register::TYPES::BYTES)
                {
                    /* Handle if string. */
                    if(!it->is_string())
                        throw Exception(-19, "Invalid type [", strField, "=", it->type_name(), "] for command");

                    /* Grab our value string to check against. */
                    const std::string strValue =
                        it->get<std::string>();

                    /* Track if we have invalid encoding. */
                    bool fInvalid = false;

                    /* Grab our payload. */
                    const std::vector<uint8_t> vPayload =
                        encoding::DecodeBase64(strValue.c_str(), &fInvalid);

                    /* Check for invalid payload. */
                    if(fInvalid)
                        throw Exception(-35, "Invalid parameter [", strField, "], expecting [base64]");

                    /* Add our payload to the contract. */
                    ssPayload << uint8_t(TAO::Operation::OP::TYPES::BYTES) << vPayload;
                }
            }

            /* Check for missing parameters. */
            if(ssPayload.size() == 0)
                throw Exception(-28, "Missing parameter [field=string] for command");

            /* Submit the payload object. */
            vContracts[0] << uint8_t(TAO::Operation::OP::WRITE) << hashRegister << ssPayload.Bytes();
        }

        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
