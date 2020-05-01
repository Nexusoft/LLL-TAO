/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/objects.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Generates private key based on keyname/user/pass/pin and stores it in the keyname slot in the crypto register. */
        json::json Crypto::Create(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Check the caller included the key name */
            if(params.find("name") == params.end() || params["name"].get<std::string>().empty())
                throw APIException(-88, "Missing name.");
            
            /* Get the requested key name */
            std::string strName = params["name"].get<std::string>();

            /* The logged in sig chain genesis hash */
            uint256_t hashGenesis = user->Genesis();

            /* The address of the crypto object register, which is deterministic based on the genesis */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
            
            /* Read the crypto object register */
            TAO::Register::Object crypto;
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-259, "Could not read crypto object register");

            /* Parse the object. */
            if(!crypto.Parse())
                throw APIException(-36, "Failed to parse object register");
            
            /* Check to see if the key name is valid */
            if(!crypto.CheckName(strName))
                throw APIException(-260, "Invalid key name");

            /* Check to see if the the key already exists */
            if(crypto.get<uint256_t>(strName) != 0)
                throw APIException(-261, "Key already exists");

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* The scheme to use to generate the key. If no specific scheme paramater has been passed in then this defaults to 
               the key type set on the previous transaction */
            uint8_t nKeyType = tx.nKeyType;

            /* Check the caller included a specific scheme */
            if(params.find("scheme") != params.end() )
            {
                std::string strScheme = params["scheme"].get<std::string>(); 

                if(strScheme == "FALCON")
                    nKeyType = TAO::Ledger::SIGNATURE::FALCON;
                else if(strScheme == "BRAINPOOL")
                    nKeyType = TAO::Ledger::SIGNATURE::BRAINPOOL;
                else
                    throw APIException(-262, "Invalid scheme");
            }
            
            /* Generate the new public key */
            uint256_t hashPublic = user->KeyHash(strName, 0, strPIN, nKeyType);

            /* Declare operation stream to serialize all of the field updates*/
            TAO::Operation::Stream ssOperationStream;

            /* Generate the new key */
            ssOperationStream << std::string(strName) << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << hashPublic;
             
            /* Add the crypto update contract. */
            tx[0] << uint8_t(TAO::Operation::OP::WRITE) << hashCrypto << ssOperationStream.Bytes();
        
            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-44, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();

            /* Populate response JSON */
            ret["name"] = strName;

            /* convert the scheme type to a string */
            switch(hashPublic.GetType())
            {
                case TAO::Ledger::SIGNATURE::FALCON:
                    ret["scheme"] = "FALCON";
                    break;
                case TAO::Ledger::SIGNATURE::BRAINPOOL:
                    ret["scheme"] = "BRAINPOOL";
                    break;
                default:
                    ret["scheme"] = "";

            }
            
            /* Populate the key, base58 encoded */
            ret["key"] = encoding::EncodeBase58(hashPublic.GetBytes());

            return ret;
        }
    }
}
