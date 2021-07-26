/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <LLD/include/global.h>

#include <TAO/API/users/types/users.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/extract.h>

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
        encoding::json Users::Recover(const encoding::json& jParams, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json jsonRet;

            /* Check for username parameter. */
            if(jParams.find("username") == jParams.end())
                throw Exception(-127, "Missing username");

            /* Check for recovery parameter. */
            if(jParams.find("recovery") == jParams.end())
                throw Exception(-220, "Missing recovery seed");

            /* Check for password parameter. */
            if(jParams.find("password") == jParams.end())
                throw Exception(-128, "Missing password");

            /* Extract username. */
            SecureString strUsername = SecureString(jParams["username"].get<std::string>().c_str());

            /* Get the genesis ID. */
            uint256_t hashGenesis = TAO::Ledger::SignatureChain::Genesis(strUsername);


            /* Extract the recovery seed */
            SecureString strRecovery = SecureString(jParams["recovery"].get<std::string>().c_str());

            /* Extract the new password */
            SecureString strPassword = SecureString(jParams["password"].get<std::string>().c_str());

            /* Existing new pin parameter. */
            const SecureString strPin = ExtractPIN(jParams);

            /* Check the new password length */
            if(strPassword.length() < 8)
                throw Exception(-192, "Password must be a minimum of 8 characters");

            /* Check pin length */
            if(strPin.length() < 4)
                throw Exception(-193, "Pin must be a minimum of 4 characters");

            /* Check that the genesis exists. */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->HasGenesis(hashGenesis))
            {
                throw Exception(-139, "Invalid credentials");
            }
            else
            {
                /* Get the last transaction. */
                uint512_t hashLast;
                if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                    throw Exception(-138, "No previous transaction found");

                /* Get previous transaction */
                if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                    throw Exception(-138, "No previous transaction found");
            }

            /* Check that the recovery hash has been set on this signature chain */
            if(txPrev.hashRecovery == 0)
                throw Exception(-223, "Recovery seed not set on this signature chain");

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
                    throw Exception(-138, "No previous transaction found");
            }

            /* Create sig chain based on the new credentials */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> user = new TAO::Ledger::SignatureChain(strUsername, strPassword);

            /* Validate the recovery seed is correct.  We do this at this stage so that we can return a helpful error response
               from the API call, rather than waiting for mempool::Accept to fail  */
            if(user->RecoveryHash(strRecovery, nKeyType ) != hashRecovery)
                throw Exception(-139, "Invalid credentials");

            /* Create the update transaction */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(user, strPin, tx))
            {
                user.free();
                throw Exception(-17, "Failed to create transaction");
            }

            /* Now set the new credentials */
            tx.NextHash(user->Generate(tx.nSequence + 1, strPassword, strPin));

            /* Update the Crypto keys with the new pin */
            update_crypto_keys(user, strPin, tx);

            /* Calculate the prestates and poststates. */
            if(!tx.Build())
            {
                user.free();
                throw Exception(-30, "Operations failed to execute");
            }

            /* Set the key type */
            tx.nKeyType = nKeyType;

            /* Sign the transaction with private key generated from the recovery seed. */
            if(!tx.Sign(user->Generate(strRecovery)))
            {
                user.free();
                throw Exception(-31, "Ledger failed to sign transaction");
            }

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
            {
                user.free();
                throw Exception(-32, "Failed to accept");
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
