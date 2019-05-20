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
#include <TAO/API/include/tokens.h>
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
            SecureString strPIN = users.GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users.GetSession(params);

            /* Check for identifier parameter. */
            if(params.find("type") == params.end())
                throw APIException(-25, "Missing Type");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users.GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Check that the account is unlocked for creating transactions */
            if(!users.CanTransact())
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
                /* Get the namespace hash to use for this object.  By default the namespace is the username for the sig chain */
                uint256_t nNamespaceHash = NamespaceHash(user->UserName());

                /* register address is a hash of a name in the format of namespacehash:objecttype:name */
                std::string strName = nNamespaceHash.ToString() + ":token:" + params["name"].get<std::string>();

                /* Build the address from an SK256 hash of API:NAME. */
                hashRegister = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));

            }
            else
                hashRegister = LLC::GetRand256();

            if(params["type"].get<std::string>() == "account")
            {
                /* Check for identifier parameter. */
                if(params.find("identifier") == params.end())
                    throw APIException(-25, "Missing Identifier");
                    
                /* The identifier is the register address of the token that this account is being created for.  The API supports passing the identifier
                   in by name in the format of namespace:name, in which case we need to convert it to the register hash */
                uint256_t nIdentifier = 0;
                std::string strIdentifier = params["identifier"].get<std::string>();
                
                /* Edge case to allow identifer NXS or 0 to be specified for NXS token */
                if( strIdentifier == "NXS" || strIdentifier == "0")
                    nIdentifier = uint256_t(0);
                else if(IsRegisterAddress(strIdentifier))
                    nIdentifier = uint256_t(strIdentifier);
                else
                    nIdentifier = RegisterAddressFromName(params, "token", strIdentifier);

                /* Create an account object register. */
                TAO::Register::Object account = TAO::Register::CreateAccount(nIdentifier);

                /* Submit the payload object. */
                tx << uint8_t(TAO::Operation::OP::REGISTER) << hashRegister << uint8_t(TAO::Register::REGISTER::OBJECT) << account.GetState();

            }
            else if(params["type"].get<std::string>() == "token")
            {
                /* Check for supply parameter. */
                if(params.find("supply") == params.end())
                    throw APIException(-25, "Missing Supply");

                /* For tokens being created without a global namespaced name, the identifier is equal to the register address */
                uint256_t nIdentifier = hashRegister;

                /* Create a token object register. */
                TAO::Register::Object token = TAO::Register::CreateToken(nIdentifier,
                                                                         std::stoull(params["supply"].get<std::string>()),
                                                                         1000000);

                /* Submit the payload object. */
                tx << uint8_t(TAO::Operation::OP::REGISTER) << hashRegister << uint8_t(TAO::Register::REGISTER::OBJECT) << token.GetState();
            }
            else
                throw APIException(-27, "Unknown object register");

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
