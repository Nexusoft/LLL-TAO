/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/types/object.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Create a NXS account. */
        json::json Finance::Create(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Authenticate the users credentials */
            if(!users->Authenticate(params))
                throw APIException(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Lock the signature chain. */
            LOCK(session.CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(session.GetAccount(), strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* Generate a random hash for this objects register address */
            TAO::Register::Address hashRegister = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

            /* name of the object, default to blank */
            std::string strName = "";

            /* The token to create the account for. Default to 0 (NXS) */
            TAO::Register::Address hashToken;

            /* See if the caller has requested a particular token type */
            if(params.find("token_name") != params.end() && !params["token_name"].get<std::string>().empty() && params["token_name"].get<std::string>() != "NXS")
                /* If name is provided then use this to deduce the register address */
                hashToken = Names::ResolveAddress(params, params["token_name"].get<std::string>());
            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("token") != params.end() && IsRegisterAddress(params["token"]))
                hashToken.SetBase58(params["token"]);

            /* If this is not a NXS token account, verify that the token identifier is for a valid token */
            if(hashToken != 0)
            {
                if(hashToken.GetType() != TAO::Register::Address::TOKEN)
                    throw APIException(-212, "Invalid token");

                TAO::Register::Object token;
                if(!LLD::Register->ReadState(hashToken, token, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-125, "Token not found");

                /* Parse the object register. */
                if(!token.Parse())
                    throw APIException(-14, "Object failed to parse");

                /* Check the standard */
                if(token.Standard() != TAO::Register::OBJECTS::TOKEN)
                    throw APIException(-212, "Invalid token");
            }

            /* Create an account object register. */
            TAO::Register::Object account = TAO::Register::CreateAccount(hashToken);

            /* If the user has supplied the data parameter than add this to the account register */
            if(params.find("data") != params.end())
                account << std::string("data") << uint8_t(TAO::Register::TYPES::STRING) << params["data"].get<std::string>();

            /* Submit the payload object. */
            tx[0] << uint8_t(TAO::Operation::OP::CREATE) << hashRegister << uint8_t(TAO::Register::REGISTER::OBJECT) << account.GetState();

            /* Check for name parameter. If one is supplied then we need to create a Name Object register for it. */
            if(params.find("name") != params.end() && !params["name"].is_null() && !params["name"].get<std::string>().empty())
                tx[1] = Names::CreateName(session.GetAccount()->Genesis(), params["name"].get<std::string>(), "", hashRegister);

            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-44, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(session.GetAccount()->Generate(tx.nSequence, strPIN)))
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
