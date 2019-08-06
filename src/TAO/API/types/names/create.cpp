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
#include <TAO/Register/include/names.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <LLD/include/global.h>

#include <Util/templates/datastream.h>

#include <Util/include/convert.h>
#include <Util/include/base64.h>

/* Global TAO namespace. */
namespace TAO
{
    /* API Layer namespace. */
    namespace API
    {
        /* Create an name . */
        json::json Names::Create(const json::json& params, bool fHelp)
        {
            /* Return JSON object */
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

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

            /* Check caller has provided the name parameter */
            if(params.find("name") == params.end())
                throw APIException(-88, "Missing name.");

            /* The register address to create the name for */
            uint256_t hashRegister = 0;

            /* Check caller has provided the register address parameter */
            if(params.find("register_address") != params.end() )
            {
                /* Check that the register address is a valid address */
                if(!IsRegisterAddress(params["register_address"].get<std::string>()))
                    throw APIException(-89, "Invalid register_address");


                hashRegister.SetHex(params["register_address"].get<std::string>());
            }

            /* Create the Name object contract */
            tx[0] = Names::CreateName(user->Genesis(), params["name"].get<std::string>(), hashRegister);

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

            /* The address for the name register is derived from the name / namespace / genesis.  To save working all this out again
               we can just deserialize it from the contract created by Name::CreateName  */
            tx[0].Reset();
            uint8_t OPERATION = 0;
            tx[0] >> OPERATION;
            uint256_t hashAddress = 0;
            tx[0]  >> hashAddress;

            ret["address"] = hashAddress.ToString();

            return ret;
        }


        /* Create an namespace . */
        json::json Names::CreateNamespace(const json::json& params, bool fHelp)
        {
            /* Return JSON object */
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

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

            /* Check caller has provided the name parameter */
            if(params.find("name") == params.end())
                throw APIException(-88, "Missing name");

            /* Get the namespace name */
            std::string strNamespace = params["name"].get<std::string>();

            /* Ensure namespace names are more than 3 characters */
            if(strNamespace.length() < 3)
                throw APIException(-167, "Name cannot be less than 3 characters long");

            /* Don't allow @ in namespace names even if it is escaped */
            if(strNamespace.find("@") != strNamespace.npos )
                throw APIException(-162, "Namespace contains invalid characters");

            /* Generate register address for namespace, which must be a hash of the name */
            uint256_t hashRegister = TAO::Register::Address(strNamespace, TAO::Register::Address::NAMESPACE);

            /* check that the namespace object doesn't already exist*/
            TAO::Register::Object namespaceObject;
            
            /* Read the Name Object */
            if(LLD::Register->ReadState(hashRegister, namespaceObject, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-90, "Namespace already exists");

            /* Create the new namespace object */
            namespaceObject = TAO::Register::CreateNamespace(strNamespace);

            /* Submit the payload object. */
            tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashRegister << uint8_t(TAO::Register::REGISTER::OBJECT) << namespaceObject.GetState();
            
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
