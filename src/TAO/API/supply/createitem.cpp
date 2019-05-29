/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>



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

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Check for data parameter. */
            if(params.find("data") == params.end())
                throw APIException(-25, "Missing data");

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Check that the account is unlocked for creating transactions */
            if( !users->CanTransact())
                throw APIException(-25, "Account has not been unlocked for transactions");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction");

            /* Submit the transaction payload. */
            uint256_t hashRegister = 0;

            /* name of the object, default to blank */
            std::string strName = "";

            /* Check for data parameter. */
            if(params.find("name") != params.end())
            {
                /* Get the called-supplied name */
                strName = params["name"].get<std::string>();

                /* Get the namespace hash to use for this object.  By default the namespace is the username for the sig chain */
                uint256_t nNamespaceHash = NamespaceHash(user->UserName());

                /* register address is a hash of a name in the format of namespacehash:objecttype:name */
                std::string strAddressName = nNamespaceHash.ToString() + ":item:" + strName;

                /* Build the address from an SK256 hash of API:NAME. */
                hashRegister = LLC::SK256(std::vector<uint8_t>(strAddressName.begin(), strAddressName.end()));
            }
            else
                hashRegister = LLC::GetRand256();

            /* Test the payload feature. */
            DataStream ssData(SER_REGISTER, 1);
            
            /* Add the name first */
            ssData << strName;

            /* Then the raw data */
            ssData << params["data"].get<std::string>();

            /* Submit the payload object. */
            tx << (uint8_t)TAO::Operation::OP::REGISTER << hashRegister << (uint8_t)TAO::Register::REGISTER::APPEND << ssData.Bytes();

            /* Execute the operations layer. */
            if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
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
