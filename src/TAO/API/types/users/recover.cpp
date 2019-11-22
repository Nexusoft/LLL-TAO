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

        /* Recover a sig chain and set new credentials by supplying the recovery seed */
        json::json Users::Recover(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json jsonRet;

            /* Check for username parameter. */
            if(params.find("username") == params.end())
                throw APIException(-127, "Missing username");

            /* Check for recovery parameter. */
            if(params.find("recovery") == params.end())
                throw APIException(-220, "Missing recovery seed");
            
            /* Check for password parameter. */
            if(params.find("password") == params.end())
                throw APIException(-128, "Missing password");
            
            /* Check for pin parameter. Extract the pin. */
            if(params.find("pin") == params.end())
                throw APIException(-129, "Missing PIN");

            /* Extract username. */
            SecureString strUsername = SecureString(params["username"].get<std::string>().c_str());

            /* Get the genesis ID. */
            uint256_t hashGenesis = TAO::Ledger::SignatureChain::Genesis(strUsername);

            
            /* Extract the recovery seed */
            SecureString strRecovery = SecureString(params["recovery"].get<std::string>().c_str());
            
            /* Extract the new password */
            SecureString strPassword = SecureString(params["password"].get<std::string>().c_str());

            /* Existing new pin parameter. */
            SecureString strPin = SecureString(params["pin"].get<std::string>().c_str());
  
            /* Check the new password length */
            if(strPassword.length() < 8)
                throw APIException(-192, "Password must be a minimum of 8 characters");

            /* Check pin length */
            if(strPin.length() < 4)
                throw APIException(-193, "Pin must be a minimum of 4 characters");

            /* Check that the genesis exists. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->HasGenesis(hashGenesis))
            {
                throw APIException(-139, "Invalid credentials");
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

            /* Check that the recovery hash has been set on this signature chain */
            if(txPrev.hashRecovery == 0)
                throw APIException(-223, "Recovery seed not set on this signature chain");

            /* When signing with the recovery seed we need to set the transaction key type to be the same type that was used 
                to generate the current recovery hash.  To obtain this we iterate back through the sig chain until we find where
                the hashRecovery was first set, and use the nKeyType from that transaction */
            
            /* The key type to set */
            uint8_t nKeyType = 0;

            /* The recovery hash to search for */
            uint256_t hashRecovery = txPrev.hashRecovery;

            /* Iterate backwards until we reach the transaction where the hashRecovery differs */
            while(txPrev.hashRecovery == hashRecovery)
            {
                nKeyType = txPrev.nKeyType;

                if(!LLD::Ledger->ReadTx(txPrev.hashPrevTx, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-138, "No previous transaction found");  
            }


            /* Lock the signature chain in case another process attempts to create a transaction . */
            LOCK(CREATE_MUTEX);

            /* Create sig chain based on the new credentials */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> user = new TAO::Ledger::SignatureChain(strUsername, strPassword); 

            /* Validate the recovery seed is correct.  We do this at this stage so that we can return a helpful error response 
               from the API call, rather than waiting for mempool::Accept to fail  */
            if(user->RecoveryHash(strRecovery, nKeyType ) != hashRecovery)
                throw APIException(-139, "Invalid credentials");

            /* Create the update transaction */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(user, strPin, tx))
            {
                user.free();
                throw APIException(-17, "Failed to create transaction");
            }

            /* Now set the new credentials */
            tx.NextHash(user->Generate(tx.nSequence + 1, strPassword, strPin), txPrev.nNextType);

            /* Update the Crypto keys with the new pin */
            update_crypto_keys(user, strPin, tx);

            /* Calculate the prestates and poststates. */
            if(!tx.Build())
            {
                user.free();
                throw APIException(-30, "Operations failed to execute");
            }

            /* Set the key type */
            tx.nKeyType = nKeyType;

            /* Sign the transaction with private key generated from the recovery seed. */
            if(!tx.Sign(user->Generate(strRecovery)))
            {
                user.free();
                throw APIException(-31, "Ledger failed to sign transaction");
            }
            
            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
            {
                user.free();
                throw APIException(-32, "Failed to accept");
            }

            /* Free the sigchain. */
            user.free();

            /* Build a JSON response object. */
            jsonRet["genesis"]   = tx.hashGenesis.ToString();
            jsonRet["nexthash"]  = tx.hashNext.ToString();
            jsonRet["prevhash"]  = tx.hashPrevTx.ToString();
            jsonRet["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
            jsonRet["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
            jsonRet["txid"]      = tx.GetHash().ToString();
            
            return jsonRet;
        }
    }
}
