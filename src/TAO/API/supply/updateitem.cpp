/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/types/user_types.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/names/types/names.h>
#include <TAO/API/supply/types/supply.h>

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

        /* Submits an item. */
        json::json Supply::UpdateItem(const json::json& params, const bool fHelp)
        {
            json::json ret;

            /* Authenticate the users credentials */
            if(!Commands::Get<Users>()->Authenticate(params))
                throw APIException(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = Commands::Get<Users>()->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = Commands::Get<Users>()->GetSession(params);

            /* Check for data parameter. */
            if(params.find("data") == params.end())
                throw APIException(-18, "Missing data");

            /* Check that the data is a string */
            if(!params["data"].is_string())
                throw APIException(-19, "Data must be a string");

            /* Lock the signature chain. */
            LOCK(session.CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(session.GetAccount(), strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* Get the register address. */
            TAO::Register::Address hashRegister ;

            /* name of the object, default to blank */
            std::string strName = "";

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end() && !params["name"].get<std::string>().empty())
            {
                /* Get the called-supplied name */
                strName = params["name"].get<std::string>();

                /* If name is provided then use this to deduce the register address */
                hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetBase58(params["address"]);

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-33, "Missing name / address");

            /* Check the address */
            TAO::Register::State state;
            if(!LLD::Register->ReadState(hashRegister, state, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-117, "Item not found");

            /* Ensure that it is an append register */
            if(state.nType != TAO::Register::REGISTER::APPEND)
                throw APIException(-117, "Item not found");

            /* Declare the register stream for the update. */
            DataStream ssData(SER_REGISTER, 1);

            /* First add the leading 2 bytes to identify the state data */
            ssData << (uint16_t) USER_TYPES::SUPPLY;

            /* Add in the new data */
            ssData << params["data"].get<std::string>();

            /* Submit the payload object. */
            tx[0] << (uint8_t)TAO::Operation::OP::APPEND << hashRegister << ssData.Bytes();

            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-30, "Operations failed to execute");

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
