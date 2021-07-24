/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/build.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/commands/assets.h>
#include <TAO/API/names/types/names.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Tokenize an asset into fungible tokens that represent ownership. */
        encoding::json Assets::Tokenize(const encoding::json& jParams, const bool fHelp)
        {
            encoding::json ret;

            /* Authenticate the users credentials */
            if(!Commands::Get<Users>()->Authenticate(jParams))
                throw Exception(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = Commands::Get<Users>()->GetPin(jParams, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = Commands::Get<Users>()->GetSession(jParams);

            /* Get the register address. */
            TAO::Register::Address hashToken;

            /* Check for data parameter. */
            if(jParams.find("token_name") != jParams.end() && !jParams["token_name"].get<std::string>().empty())
            {
                /* If name is provided then use this to deduce the register address */
                hashToken = Names::ResolveAddress(jParams, jParams["token_name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(jParams.find("token") != jParams.end() && CheckAddress(jParams["token"]))
                hashToken.SetBase58(jParams["token"]);

            /* Fail if no required parameters supplied. */
            else
                throw Exception(-37, "Missing token name / address");

            /* Validate the token requested */
            /* Get the token object. */
            TAO::Register::Object token;
            if(!LLD::Register->ReadObject(hashToken, token, TAO::Ledger::FLAGS::LOOKUP))
                throw Exception(-125, "Token not found");

            /* Check the object standard. */
            if(token.Standard() != TAO::Register::OBJECTS::TOKEN)
                throw Exception(-123, "Object is not a token");

            /* Get the register address of the asset. */
            TAO::Register::Address hashRegister ;

            /* Check whether the caller has provided the asset name parameter. */
            if(jParams.find("name") != jParams.end() && !jParams["name"].get<std::string>().empty())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = Names::ResolveAddress(jParams, jParams["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(jParams.find("address") != jParams.end())
                hashRegister.SetBase58(jParams["address"]);

            /* Fail if no required parameters supplied. */
            else
                throw Exception(-38, "Missing asset name / address");

            /* Validate the asset */
            TAO::Register::Object asset;
            if(!LLD::Register->ReadState(hashRegister, asset, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-34, "Asset not found");

            if(config::fClient.load() && asset.hashOwner != Commands::Get<Users>()->GetCallersGenesis(jParams))
                throw Exception(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Now lets check our expected types match. */
            if(!CheckStandard(jParams, asset))
                throw Exception(-49, "Unsupported type for name/address");

            /* Get the account. */
            const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = session.GetAccount();
            if(!user)
                throw Exception(-10, "Invalid session ID");

            /* Lock the signature chain. */
            LOCK(session.CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(user, strPIN, tx))
                throw Exception(-17, "Failed to create transaction");

            /* Submit the payload object.
               NOTE we pass true for the fForceTransfer parameter so that the transfer is made immediately to the
               token without requiring a Claim */
            tx[0] << (uint8_t)TAO::Operation::OP::TRANSFER << hashRegister << hashToken << uint8_t(TAO::Operation::TRANSFER::FORCE);

            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw Exception(-30, "Operations failed to execute");

            /* Sign the transaction. */
            if(!tx.Sign(session.GetAccount()->Generate(tx.nSequence, strPIN)))
                throw Exception(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw Exception(-32, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();
            ret["address"] = hashRegister.ToString();

            return ret;
        }
    }
}
