/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <TAO/API/include/users.h>
#include <TAO/API/include/assets.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/templates/datastream.h>

#include <Util/include/convert.h>

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

            /* Submit the transaction payload. */
            uint256_t hashRegister = 0;

            /* Check for data parameter. */
            if(params.find("name") != params.end())
            {
                /* Get the address from the name. */
                std::string strName = GetName() + ":" + params["name"].get<std::string>();

                /* Build the address from an SK256 hash of API:NAME. */
                hashRegister = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));
            }
            else
                hashRegister = LLC::GetRand256();


            /* Check for format parameter. */
            std::string strFormat = "basic"; // default to basic format if no foramt is specified
            if(params.find("format") != params.end())
                strFormat = params["format"].get<std::string>();
            
            // parse the incoming asset definition based on the specified format
            if( strFormat == "raw")
            {
                /* If format = raw then use a raw state register rather than an object */
                if(params.find("data") == params.end())
                    throw APIException(-25, "Missing data parameter");

                /* Serialise the incoming data into a state register*/
                DataStream ssData(SER_REGISTER, 1);
                ssData << params["data"].get<std::string>();

                /* Submit the payload object. */
                tx << (uint8_t)TAO::Operation::OP::REGISTER << hashRegister << (uint8_t)TAO::Register::REGISTER::RAW << ssData.Bytes();

            }
            else if( strFormat == "basic")
            {
                /* declare the object register to hold the asset data*/
                TAO::Register::Object asset;

                 /* Iterate through the paramers and infer the type for each value */
                for (auto it = params.begin(); it != params.end(); ++it)
                {
                    /* Skip any incoming parameters that are keywords used by this API method*/
                    if( it.key() == "pin" 
                        || it.key() == "session"
                        || it.key() == "name"
                        || it.key() == "format"
                        || it.key() == "token_name"
                        || it.key() == "token_value")
                    {
                        continue;
                    }

                    /* Deduce the type from the incoming value */
                    if( it->is_string())
                    {
                        std::string strValue = it->get<std::string>();
                        asset << it.key()  << uint8_t(TAO::Register::TYPES::STRING) << strValue;
                    }
                    else
                    {
                        throw APIException(-25, "Non-string types not supported in basic format.");
                    }
                     

                }

                //asset << std::string("metadata")    << uint8_t(TAO::Register::TYPES::MUTABLE)  << uint8_t(TAO::Register::TYPES::STRING) << params["metadata"].get<std::string>();
                //asset << std::string("testdata")    << uint8_t(TAO::Register::TYPES::MUTABLE)  << uint8_t(TAO::Register::TYPES::UINT32_T) << (uint32_t(123456789));

                /* Submit the payload object. */
                tx << uint8_t(TAO::Operation::OP::REGISTER) << hashRegister << uint8_t(TAO::Register::REGISTER::OBJECT) << asset.GetState();

            }
            else if( strFormat == "JSON")
            {

            }
            else
            {
                throw APIException(-25, "Unsupported format specified");
            }
            



    
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
