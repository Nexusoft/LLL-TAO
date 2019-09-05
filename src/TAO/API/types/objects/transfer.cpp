/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/conditions.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Transfer an object register. */
        json::json Objects::Transfer(const json::json& params, uint8_t nType, const std::string& strType)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Watch for destination genesis. */
            uint256_t hashTo = 0;
            if(params.find("destination") != params.end())
                hashTo.SetHex(params["destination"].get<std::string>());
            else if(params.find("username") != params.end())
                hashTo = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            else
                throw APIException(-112, "Missing username / destination");

            /* Check that the destination exists. */
            if(!LLD::Ledger->HasGenesis(hashTo))
                throw APIException(-113, "Destination user doesn't exist");

            /* Get the register address. */
            TAO::Register::Address hashRegister ;

            /* The Object to get history for */
            TAO::Register::Object object;

            /* Check whether the caller has provided the name parameter. */
            if(params.find("name") != params.end())
            {
                /* Edge case for NAME objects as these do not need to be resolved to an address */
                if(nType == TAO::Register::OBJECTS::NAME)
                    object = Names::GetName(params, params["name"].get<std::string>(), hashRegister);
                /* Edge case for namespace objects as the address is a hash of the name */
                else if(nType == TAO::Register::OBJECTS::NAMESPACE)
                    hashRegister = LLC::SK256(params["name"].get<std::string>());
                else
                    /* If name is provided then use this to deduce the register address */
                    hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            }
            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetBase58(params["address"]);
            /* Fail if no required parameters supplied. */
            else
                throw APIException(-33, "Missing name / address");


            /* Get the object from the register DB if we haven't already. */
            if(object.IsNull())
            {
                if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-114, strType +" not found.");

                /* Only include raw and non-standard object types (assets)*/
                if(object.nType != TAO::Register::REGISTER::OBJECT
                && object.nType != TAO::Register::REGISTER::APPEND
                && object.nType != TAO::Register::REGISTER::RAW)
                    throw APIException(-114, strType + "not found.");

                /* parse object so that the data fields can be accessed and check that it is an asset*/
                if(object.nType == TAO::Register::REGISTER::OBJECT
                && (!object.Parse() || object.Standard() != nType))
                    throw APIException(-115, "Name / address is not of type " +strType);
            }

            /* Edge case logic for NAME objects as these can only be transferred if they are created in a namespace */
            if(nType == TAO::Register::OBJECTS::NAME && object.get<std::string>("namespace").empty())
                throw APIException(-116, "Cannot transfer names created without a namespace");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-16, "Account has not been unlocked for transactions");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* Submit the payload object.
               NOTE we pass false for the fForceTransfer parameter so that the Transfer requires a corresponding Claim */
            tx[0] << (uint8_t)TAO::Operation::OP::TRANSFER << hashRegister << hashTo << uint8_t(TAO::Operation::TRANSFER::CLAIM);

            /* Add expiration condition if caller has passed an expires value */
            if(params.find("expires") != params.end())
                AddExpires( params, user->Genesis(), tx[0]);
            
            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-30, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();
            ret["address"] = hashRegister.ToString();

            return ret;
        }
    }
}
