/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/types/address.h>

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/global.h>
#include <TAO/API/types/user_types.h>

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
        json::json Supply::CreateItem(const json::json& params, bool fHelp)
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

            /* Generate a random hash for this objects register address */
            TAO::Register::Address hashRegister = TAO::Register::Address(TAO::Register::Address::APPEND);

            /* name of the object, default to blank */
            std::string strName = "";

            /* Test the payload feature. */
            DataStream ssData(SER_REGISTER, 1);

            /* First add the leading 2 bytes to identify the state data */
            ssData << (uint16_t) USER_TYPES::SUPPLY;

            /* Then the raw data */
            ssData << params["data"].get<std::string>();

            /* Submit the payload object. */
            tx[0] << (uint8_t)TAO::Operation::OP::CREATE << hashRegister << (uint8_t)TAO::Register::REGISTER::APPEND << ssData.Bytes();

            /* Check for name parameter. If one is supplied then we need to create a Name Object register for it. */
            if(params.find("name") != params.end() && !params["name"].is_null() && !params["name"].get<std::string>().empty())
                tx[1] = Names::CreateName(session.GetAccount()->Genesis(), params["name"].get<std::string>(), "", hashRegister);

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
