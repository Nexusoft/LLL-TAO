/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>
#include <TAO/Register/include/create.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/templates/datastream.h>

#include <Util/include/convert.h>
#include <Util/include/base64.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Create an asset or digital item. */
        json::json Assets::Create(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-25, "Account has not been unlocked for transactions");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction");

            /* Generate a random hash for this objects register address */
            uint256_t hashRegister = LLC::GetRand256();

            /* Check for format parameter. */
            std::string strFormat = "basic"; // default to basic format if no foramt is specified
            if(params.find("format") != params.end())
                strFormat = params["format"].get<std::string>();

            // parse the incoming asset definition based on the specified format
            if(strFormat == "raw")
            {
                /* If format = raw then use a raw state register rather than an object */
                if(params.find("data") == params.end())
                    throw APIException(-25, "Missing data parameter");

                /* Serialise the incoming data into a state register*/
                DataStream ssData(SER_REGISTER, 1);

                /* Then the raw data */
                ssData << params["data"].get<std::string>();

                /* Submit the payload object. */
                tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashRegister << uint8_t(TAO::Register::REGISTER::RAW) << ssData.Bytes();

            }
            else if(strFormat == "basic")
            {
                /* declare the object register to hold the asset data*/
                TAO::Register::Object asset = TAO::Register::CreateAsset();

                /* Track the number of fields so that we can check there is at least one */
                uint32_t nFieldCount = 0;

                /* Iterate through the paramers and infer the type for each value */
                for(auto it = params.begin(); it != params.end(); ++it)
                {
                    /* Skip any incoming parameters that are keywords used by this API method*/
                    if(it.key() == "pin"
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
                        throw APIException(-25, "Non-string types not supported in basic format.");
                }

                if(nFieldCount == 0)
                    throw APIException(-25, "Missing asset value fields.");

                /* Submit the payload object. */
                tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashRegister << uint8_t(TAO::Register::REGISTER::OBJECT) << asset.GetState();
            }
            else if(strFormat == "JSON")
            {
                /* If format = JSON then grab the asset definition from the json field */
                if(params.find("json") == params.end())
                    throw APIException(-25, "Missing json parameter.");

                if(!params["json"].is_array())
                    throw APIException(-25, "json field must be an array.");

                /* declare the object register to hold the asset data*/
                TAO::Register::Object asset = TAO::Register::CreateAsset();

                json::json jsonAssetDefinition = params["json"];

                /* Track the number of fields so that we can check there is at least one */
                int nFieldCount = 0;

                /* Iterate through each field definition */
                for(auto it = jsonAssetDefinition.begin(); it != jsonAssetDefinition.end(); ++it)
                {
                    /* Check that the required fields have been provided*/
                    if(it->find("name") == it->end())
                        throw APIException(-25, "Missing name field in json definition.");

                    if(it->find("type") == it->end())
                        throw APIException(-25, "Missing type field in json definition.");

                    if(it->find("value") == it->end())
                        throw APIException(-25, "Missing value field in json definition.");

                    if(it->find("mutable") == it->end())
                        throw APIException(-25, "Missing mutable field in json definition.");

                    /* Parse the values out of the definition json*/
                    std::string strName =  (*it)["name"].get<std::string>();
                    std::string strType =  (*it)["type"].get<std::string>();
                    std::string strValue = (*it)["value"].get<std::string>();
                    bool fMutable = (*it)["mutable"].get<std::string>() == "true";
                    bool fBytesInvalid = false;
                    std::vector<uint8_t> vchBytes;

                    /* Convert the value to bytes if the type is bytes */
                    if(strType == "bytes")
                        vchBytes = encoding::DecodeBase64(strValue.c_str(), &fBytesInvalid);

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
                                    throw APIException(-25, "maxlength value is less than the specified data length.");
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


                    /* Add the field to the Object based on the user defined type.
                       NOTE: all numeric values <= 64-bit are converted from string to the corresponding type.
                       Numeric values > 64-bit are assumed to be in hex and are converted via the uintXXX constructor */

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
                            throw APIException(-5, "Malformed base64 encoding");

                        /* Ensure that the serialized value is padded out to the max length */
                        vchBytes.resize(nMaxLength);

                        asset << uint8_t(TAO::Register::TYPES::BYTES) << vchBytes;
                    }

                    /* Increment total fields. */
                    ++nFieldCount;
                }

                if(nFieldCount == 0)
                    throw APIException(-25, "Missing asset field definitions");

                /* Submit the payload object. */
                tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashRegister << uint8_t(TAO::Register::REGISTER::OBJECT) << asset.GetState();
            }
            else
            {
                throw APIException(-25, "Unsupported format specified");
            }

            /* Check for name parameter. If one is supplied then we need to create a Name Object register for it. */
            if(params.find("name") != params.end())
                tx[1] = CreateNameContract(user->Genesis(), params["name"].get<std::string>(), hashRegister);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-26, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-26, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-26, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();
            ret["address"] = hashRegister.ToString();

            return ret;
        }


    }
}
