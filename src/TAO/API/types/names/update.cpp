/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/templates/datastream.h>

#include <Util/include/convert.h>
#include <Util/include/base64.h>

/* Global TAO namespace. */
namespace TAO
{
    /* API Layer namespace. */
    namespace API
    {
        /* Update the data in an asset */
        json::json Names::UpdateName(const json::json& params, bool fHelp)
        {
            /* Return JSON object */
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* Get the Register ID. */
            TAO::Register::Address hashRegister ;

            /* name object to update */
            TAO::Register::Object name;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
                /* If name is provided then use this to retrieve the name object */
                name = Names::GetName(params,params["name"].get<std::string>(), hashRegister);
            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
            {
                hashRegister.SetBase58(params["address"].get<std::string>());

                /* Retrieve the name by hash */
                if(!LLD::Register->ReadState(hashRegister, name, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-102, "Invalid address.");

                /* Check that the name object is proper type. */
                if(name.nType != TAO::Register::REGISTER::OBJECT
                || !name.Parse()
                || name.Standard() != TAO::Register::OBJECTS::NAME )
                    throw APIException(-102, "Invalid address");
            }
            /* Fail if no required parameters supplied. */
            else
                throw APIException(-33, "Missing name / address");

            if(params.find("register_address") == params.end())
                throw APIException(-103, "Missing register_address parameter");


            /* Declare operation stream to serialize all of the field updates*/
            TAO::Operation::Stream ssOperationStream;

            /* The new register address to set for the name object */
            TAO::Register::Address hashNewRegisterAddress;
            hashNewRegisterAddress.SetBase58(params["register_address"].get<std::string>());

            /* Write the new register address value into the operation stream */
            ssOperationStream << std::string("address") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << hashNewRegisterAddress;

            /* Create the transaction object script. */
            tx[0] << uint8_t(TAO::Operation::OP::WRITE) << hashRegister << ssOperationStream.Bytes();

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
