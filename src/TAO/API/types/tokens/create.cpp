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
#include <TAO/Register/include/create.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Create an asset or digital item. */
        json::json Tokens::Create(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Check for identifier parameter. */
            if(params.find("type") == params.end())
                throw APIException(-25, "Missing Type (<type>)");

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

            /* name of the object, default to blank */
            std::string strName = "";

            if(params["type"].get<std::string>() == "account")
            {
                std::string strTokenIdentifier = "";

                /* Check for token name/address parameter. */
                if(params.find("token") != params.end())
                    strTokenIdentifier = params["token"].get<std::string>();
                else if(params.find("token_name") != params.end())
                    strTokenIdentifier = params["token_name"].get<std::string>();
                else
                    throw APIException(-25, "Missing token name / address");

                uint256_t hashIdentifier = 0;

                /* Convert token name to a register address if a name has been passed in */
                /* Edge case to allow identifer NXS or 0 to be specified for NXS token */
                if(strTokenIdentifier == "NXS" || strTokenIdentifier == "0")
                    hashIdentifier = uint256_t(0);
                else if(IsRegisterAddress(strTokenIdentifier))
                    hashIdentifier = uint256_t(strTokenIdentifier);
                else
                    hashIdentifier = AddressFromName(params, strTokenIdentifier);

                /* Create an account object register. */
                TAO::Register::Object account = TAO::Register::CreateAccount(hashIdentifier);

                /* Submit the payload object. */
                tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashRegister << uint8_t(TAO::Register::REGISTER::OBJECT) << account.GetState();

            }
            else if(params["type"].get<std::string>() == "token")
            {
                /* Check for supply parameter. */
                if(params.find("supply") == params.end())
                    throw APIException(-25, "Missing Supply");

                /* Extract the supply parameter */
                double dSupply = 0;
                if(params.find("supply") != params.end())
                    dSupply = std::stoll(params["supply"].get<std::string>());

                /* For tokens being created without a global namespaced name, the identifier is equal to the register address */
                uint256_t hashIdentifier = hashRegister;

                /* Check for nDigits parameter. */
                uint64_t nDigits = 0;
                if(params.find("digits") != params.end())
                    nDigits = std::stod(params["digits"].get<std::string>());

                /* Multiply the supply by 10^digits to give the supply in the divisible units */
                uint64_t nSupply = dSupply * pow(10, nDigits);

                /* Create a token object register. */
                TAO::Register::Object token = TAO::Register::CreateToken(hashIdentifier,
                                                                         nSupply,
                                                                         nDigits);

                /* Submit the payload object. */
                tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashRegister << uint8_t(TAO::Register::REGISTER::OBJECT) << token.GetState();
            }
            else
                throw APIException(-27, "Unknown object register");

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
