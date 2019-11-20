/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/utils.h>

#include <TAO/API/types/users.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Update a user's credentials given older credentials to authorize the update. */
        json::json Users::Update(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json jsonRet;

            /* Get the session to be used for this API call */
            uint256_t nSession = GetSession(params);

            /* Get the account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Check for password parameter. */
            if(params.find("password") == params.end())
                throw APIException(-128, "Missing password");
            
            /* Check for pin parameter. Extract the pin. */
            if(params.find("pin") == params.end())
                throw APIException(-129, "Missing PIN");

            /* Extract the existing password */
            SecureString strPassword = params["password"].get<std::string>().c_str();

            /* Existing pin parameter. */
            SecureString strPin = SecureString(params["pin"].get<std::string>().c_str());

            /* Extract the new password - default to old password if only changing pin*/
            SecureString strNewPassword = strPassword;

            /* New pin parameter. Default to old pin if only changing pin*/
            SecureString strNewPin = strPin;

            /* Check for new password parameter. */
            if(params.find("new_password") != params.end())
                strNewPassword = params["new_password"].get<std::string>().c_str();

            /* Check password length */
            if(strNewPassword.length() < 8)
                throw APIException(-192, "Password must be a minimum of 8 characters");

            /* Check for new pin parameter. */
            if(params.find("new_pin") != params.end())
                strNewPin = SecureString(params["new_pin"].get<std::string>().c_str());

            /* Check pin length */
            if(strNewPin.length() < 4)
                throw APIException(-193, "Pin must be a minimum of 4 characters");

            /* Check something is being changed */
            if(strNewPassword == strPassword && strNewPin == strPin)
                throw APIException(-218, "User password / pin not changed");

            /* Validate the existing credentials again */
            /* Get the genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* Check for duplicates in ledger db. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->HasGenesis(hashGenesis))
            {
                /* Check the memory pool and compare hashes. */
                if(!TAO::Ledger::mempool.Has(hashGenesis))
                {
                    /* Account doesn't exist returns invalid credentials */
                    user.free();
                    throw APIException(-139, "Invalid credentials");
                }

                /* Get the memory pool tranasction. */
                if(!TAO::Ledger::mempool.Get(hashGenesis, txPrev))
                    throw APIException(-137, "Couldn't get transaction");
            }
            else
            {
                /* Get the last transaction. */
                uint512_t hashLast;
                if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-138, "No previous transaction found");

                /* Get previous transaction */
                if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-138, "No previous transaction found");
            }

            /* Lock the signature chain in case another process attempts to create a transaction . */
            LOCK(CREATE_MUTEX);

            /* Create a temp sig chain to check the credentials again */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> userUpdated = new TAO::Ledger::SignatureChain(user->UserName(), strPassword); 

            /* Create temp Transaction to check credentials. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(userUpdated->Generate(txPrev.nSequence + 1, strPin, false), txPrev.nNextType);

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
                throw APIException(-139, "Invalid credentials");

            /* Create the update transaction */
            if(!Users::CreateTransaction(user, strPin, tx))
                throw APIException(-17, "Failed to create transaction");

            /* Now set the new credentials */
            tx.NextHash(userUpdated->Generate(tx.nSequence + 1, strNewPassword, strNewPin), txPrev.nNextType);

            /* Update the Crypto keys with the new pin */
            update_crypto_keys(userUpdated, strNewPin, tx);

            /* Calculate the prestates and poststates. */
            if(!tx.Build())
            {
                userUpdated.free();
                throw APIException(-30, "Operations failed to execute");
            }

            /* Sign the transaction with the old pin. */
            if(!tx.Sign(GetKey(tx.nSequence, strPin, nSession)))
            {
                userUpdated.free();
                throw APIException(-31, "Ledger failed to sign transaction");
            }

            
            {
                LOCK(MUTEX);

                /* Free the existing sigchain. */
                user.free();

                /* Update the sig chain in session. */
                mapSessions[nSession] = std::move(userUpdated);

                if(!pActivePIN.IsNull())
                    pActivePIN.free();
            }

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept");

            /* Add the transaction ID to the response */
            jsonRet["txid"]  = tx.GetHash().ToString();
            
            return jsonRet;
        }


        /* Generates new keys in the Crypto object register for a signature chain, using the specified pin, and adds the update
        *  contract to the transaction. */
        void Users::update_crypto_keys(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPin, TAO::Ledger::Transaction& tx )
        {
            /* Generate register address for crypto register deterministically */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), user->Genesis(), TAO::Register::Address::CRYPTO);

            /* The Crypto object for this sig chain */
            TAO::Register::Object crypto;

            /* Retrieve the existing Crypto register so that we can update the keys. */
            if(!LLD::Register->ReadState(hashCrypto, crypto))
                throw APIException(-219, "Could not retrieve Crypto object");

            /* Parse the crypto object register so that we can read the current fields. */
            if(!crypto.Parse())
                throw APIException(-14, "Object failed to parse");

            /* Declare operation stream to serialize all of the field updates*/
            TAO::Operation::Stream ssOperationStream;

            /* Update the keys */
            if(crypto.get<uint256_t>("auth") != 0)
                ssOperationStream << std::string("auth") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("auth", 0, strPin, tx.nKeyType);
            
            if(crypto.get<uint256_t>("lisp") != 0)
                ssOperationStream << std::string("lisp") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("lisp", 0, strPin, tx.nKeyType);

            if(crypto.get<uint256_t>("network") != 0)
                ssOperationStream << std::string("network") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("network", 0, strPin, tx.nKeyType);

            if(crypto.get<uint256_t>("sign") != 0)
                ssOperationStream << std::string("sign") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("sign", 0, strPin, tx.nKeyType);

            if(crypto.get<uint256_t>("verify") != 0)
                ssOperationStream << std::string("verify") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("verify", 0, strPin, tx.nKeyType);

            if(crypto.get<uint256_t>("cert") != 0)
                ssOperationStream << std::string("cert") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("cert", 0, strPin, tx.nKeyType);

            if(crypto.get<uint256_t>("app1") != 0)
                ssOperationStream << std::string("app1") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("app1", 0, strPin, tx.nKeyType);

            if(crypto.get<uint256_t>("app2") != 0)
                ssOperationStream << std::string("app2") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("app2", 0, strPin, tx.nKeyType);

            if(crypto.get<uint256_t>("app3") != 0)
                ssOperationStream << std::string("app3") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("app3", 0, strPin, tx.nKeyType);

            /* Add the crypto update contract. */
            tx[tx.Size()] << uint8_t(TAO::Operation::OP::WRITE) << hashCrypto << ssOperationStream.Bytes();
        }
    
    }
}
