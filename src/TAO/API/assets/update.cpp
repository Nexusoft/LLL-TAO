/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/users.h>
#include <TAO/API/include/assets.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

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

        /* Update the data in an asset */
        json::json Assets::Update(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users.GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users.GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users.GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Check that the account is unlocked for creating transactions */
            if( !users.CanTransact())
                throw APIException(-25, "Account has not been unlocked for transactions");


            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction");


            /* Get the Register ID. */
            uint256_t hashRegister = 0;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = RegisterAddressFromName(params, "asset", params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing name / address");


            /* Check for format parameter. */
            std::string strFormat = "JSON"; // default to json format if no foramt is specified
            if(params.find("format") != params.end())
                strFormat = params["format"].get<std::string>();

            /* Get the asset from the register DB.  We can read it as an Object.
               If this fails then we try to read it as a base State type and assume it was
               created as a raw format asset */
            TAO::Register::Object asset;
            if(!LLD::regDB->ReadState(hashRegister, asset))
                throw APIException(-24, "Asset not found");

            /* parse object so that the data fields can be accessed */
            asset.Parse();

            /* Declare operation stream to serialize all of the field updates*/
            TAO::Operation::Stream ssOperationStream;

            if(strFormat == "JSON")
            {
                /* Iterate through each field definition */
                for (auto it = params.begin(); it != params.end(); ++it)
                {
                    /* Skip any incoming parameters that are keywords used by this API method*/
                    if(it.key() == "pin"
                    || it.key() == "session"
                    || it.key() == "name"
                    || it.key() == "address"
                    || it.key() == "format")
                    {
                        continue;
                    }

                    if(it->is_string())
                    {
                        /* Get the key / value from the JSON*/
                        std::string strDataField = it.key();
                        std::string strValue = it->get<std::string>();

                        /* Check that the data field exists in the asset */
                        uint8_t nType = TAO::Register::TYPES::UNSUPPORTED;
                        if(!asset.Type(strDataField, nType))
                            throw APIException(-25, debug::safe_printstr("Field not found in asset: ", strDataField));

                        if(!asset.Check(strDataField, nType, true))
                            throw APIException(-25, debug::safe_printstr("Field not mutable in asset: ", strDataField));

                        /* Convert the incoming value to the correct type and write it into the asset object */
                        if( nType == TAO::Register::TYPES::UINT8_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT8_T) << uint8_t(stol(strValue));
                        else if(nType == TAO::Register::TYPES::UINT16_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT16_T) << uint16_t(stol(strValue));
                        else if(nType == TAO::Register::TYPES::UINT32_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT32_T) << uint32_t(stol(strValue));
                        else if(nType == TAO::Register::TYPES::UINT64_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT64_T) << uint64_t(stol(strValue));
                        else if(nType == TAO::Register::TYPES::UINT256_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << uint256_t(strValue);
                        else if(nType == TAO::Register::TYPES::UINT512_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT512_T) << uint512_t(strValue);
                        else if(nType == TAO::Register::TYPES::UINT1024_T)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::UINT1024_T) << uint1024_t(strValue);
                        else if(nType == TAO::Register::TYPES::STRING)
                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::STRING) << strValue;
                        else if(nType == TAO::Register::TYPES::BYTES)
                        {
                            bool fInvalid = false;
                            std::vector<uint8_t> vchBytes = encoding::DecodeBase64(strValue.c_str(), &fInvalid);

                            if (fInvalid)
                                throw APIException(-5, "Malformed base64 encoding");

                            ssOperationStream << strDataField << uint8_t(TAO::Operation::OP::TYPES::BYTES) << vchBytes;
                        }
                    }
                    else
                    {
                        throw APIException(-25, "Non-string types not supported in basic format.");
                    }
                }
            }

            /* Create the transaction object script. */
            tx << uint8_t(TAO::Operation::OP::WRITE) << hashRegister << ssOperationStream.Bytes();

            /* Execute the operations layer. */
            if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
                throw APIException(-26, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(users.GetKey(tx.nSequence, strPIN, nSession)))
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
