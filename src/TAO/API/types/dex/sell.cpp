/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

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

        /* Create an asset or digital item. */
        json::json DEX::Sell(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Check for from parameter. */
            uint256_t hashFrom = 0;
            if(params.find("name_from") != params.end())
                hashFrom = Names::ResolveAddress(params, params["name_from"].get<std::string>());
            else if(params.find("address_from") != params.end())
                hashFrom.SetHex(params["address_from"].get<std::string>());
            else
                throw APIException(-39, "Missing name_from / address_from");

            /* Check for from parameter. */
            uint256_t hashTo = 0;
            if(params.find("name_to") != params.end())
                hashTo = Names::ResolveAddress(params, params["name_to"].get<std::string>());
            else if(params.find("address_to") != params.end())
                hashTo.SetHex(params["address_to"].get<std::string>());
            else
                throw APIException(-45, "Missing name_to / address_to");

            /* Check for credit parameter. */
            if(params.find("amount") == params.end())
                throw APIException(-46, "Missing amount.");

            /* Get the amount to debit. */
            uint64_t nAmount = std::stoul(params["amount"].get<std::string>());

            /* Check for credit parameter. */
            if(params.find("price") == params.end())
                throw APIException(-47, "Missing price.");

            /* Get the amount to debit. */
            uint64_t nPrice = std::stoul(params["price"].get<std::string>());

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Lock the signature chain. */
            LOCK(user->CREATE_MUTEX);

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-16, "Account has not been unlocked for transactions.");

            /* Get the from register. */
            TAO::Register::Object objectTo;
            if(!LLD::Register->ReadState(hashTo, objectTo))
                throw APIException(-48, "Failed to read to state");

            /* Parse the object register. */
            if(!objectTo.Parse())
                throw APIException(-49, "Failed to parse to state");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction.");

            /* Transation payload. */
            tx[0] << uint8_t(TAO::Operation::OP::CONDITION) << uint8_t(TAO::Operation::OP::DEBIT) << hashFrom << ~uint256_t(0) << nAmount;

            /* Conditional Requirement. */
            TAO::Operation::Stream compare;
            compare << uint8_t(TAO::Operation::OP::DEBIT) << uint256_t(0) << hashTo << nPrice;

            /* Conditions. */
            tx[0] <= uint8_t(TAO::Operation::OP::GROUP);
            tx[0] <= uint8_t(TAO::Operation::OP::CALLER::OPERATIONS) <= uint8_t(TAO::Operation::OP::CONTAINS);
            tx[0] <= uint8_t(TAO::Operation::OP::TYPES::BYTES) <= compare.Bytes();
            tx[0] <= uint8_t(TAO::Operation::OP::AND);
            tx[0] <= uint8_t(TAO::Operation::OP::CALLER::PRESTATE::VALUE) <= std::string("token");
            tx[0] <= uint8_t(TAO::Operation::OP::EQUALS);
            tx[0] <= uint8_t(TAO::Operation::OP::TYPES::UINT256_T) <= objectTo.get<uint256_t>("token");
            tx[0] <= uint8_t(TAO::Operation::OP::UNGROUP);

            tx[0] <= uint8_t(TAO::Operation::OP::OR);

            tx[0] <= uint8_t(TAO::Operation::OP::CALLER::GENESIS);
            tx[0] <= uint8_t(TAO::Operation::OP::EQUALS);
            tx[0] <= uint8_t(TAO::Operation::OP::THIS::GENESIS);

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
            ret["txid"] = tx.GetHash().ToString();

            return ret;
        }
    }
}
