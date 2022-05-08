/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/session-manager.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/include/extract.h>

#include <TAO/Register/types/object.h>

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
        encoding::json Users::Update(const encoding::json& jParams, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json jsonRet;

            /* Get the session to be used for this API call */
            Session& session = GetSession(jParams);

            /* Check for password parameter. */
            if(jParams.find("password") == jParams.end())
                throw Exception(-128, "Missing password");

            /* Extract the existing password */
            SecureString strPassword = SecureString(jParams["password"].get<std::string>().c_str());

            /* Existing pin parameter. */
            const SecureString strPIN = ExtractPIN(jParams);

            /* Extract the new password - default to old password if only changing pin*/
            SecureString strNewPassword = strPassword;

            /* New pin parameter. Default to old pin if only changing pin*/
            SecureString strNewPin = strPIN;

            /* New recovery seed to set. */
            SecureString strNewRecovery = "";

            /* Check for new password parameter. */
            if(jParams.find("new_password") != jParams.end() && !jParams["new_password"].get<std::string>().empty())
            {
                strNewPassword = SecureString(jParams["new_password"].get<std::string>().c_str());

                /* Check password length */
                if(strNewPassword.length() < 8)
                    throw Exception(-192, "Password must be a minimum of 8 characters");
            }

            /* Check for new pin parameter. */
            if(jParams.find("new_pin") != jParams.end() && !jParams["new_pin"].get<std::string>().empty())
            {
                strNewPin = SecureString(jParams["new_pin"].get<std::string>().c_str());

                /* Check pin length */
                if(strNewPin.length() < 4)
                    throw Exception(-193, "Pin must be a minimum of 4 characters");
            }

            /* Check for recovery seed parameter. */
            if(jParams.find("new_recovery") != jParams.end() && !jParams["new_recovery"].get<std::string>().empty())
            {
                strNewRecovery = jParams["new_recovery"].get<std::string>().c_str();

                /* Check recovery seed length */
                if(strNewRecovery.length() < 40)
                    throw Exception(-221, "Recovery seed must be a minimum of 40 characters");
            }

            /* Check something is being changed */
            if(strNewRecovery.empty() && strNewPassword == strPassword && strNewPin == strPIN)
                throw Exception(-218, "User password / pin not changed");


            /* Validate the existing credentials again */
            /* Get the genesis ID. */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* Get the sig chain transaction to authenticate with, using the same hash that was used at login . */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->ReadTx(session.hashAuth, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-138, "No previous transaction found");

            /* Flag indicating that the recovery seed should be used to sign the transaction */
            bool fRecovery = false;

            /* If a new recovery has been passed in AND a previous recovery had been set, then the caller must also supply the
               previous recovery seed.  This is required as we need to sign the new transaction using the previous recovery seed. */
            SecureString strRecovery = "";
            if(!strNewRecovery.empty() && txPrev.hashRecovery != 0 )
            {
                /* Check that the caller has supplied the previous recovery seed */
                if(jParams.find("recovery") == jParams.end())
                    throw Exception(-220, "Missing recovery seed. ");

                /* Get the previous recovery seed from the jParams */
                strRecovery = SecureString(jParams["recovery"].get<std::string>().c_str());

                fRecovery = true;
            }

            /* Authenticate the users credentials */
            if(!Commands::Instance<Users>()->Authenticate(jParams))
                throw Exception(-139, "Invalid credentials");

            /* Lock the signature chain in case another process attempts to create a transaction . */
            LOCK(session.CREATE_MUTEX);

            /* Create a temp sig chain to check the credentials again */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> userUpdated =
                new TAO::Ledger::SignatureChain(session.GetAccount()->UserName(), strPassword);

            /* Create the update transaction */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(session.GetAccount(), strPIN, tx))
                throw Exception(-17, "Failed to create transaction");

            /* Now set the new credentials */
            tx.NextHash(userUpdated->Generate(tx.nSequence + 1, strNewPassword, strNewPin));

            /* Set the recovery hash */
            if(!strNewRecovery.empty())
                tx.hashRecovery = userUpdated->RecoveryHash(strNewRecovery, txPrev.nNextType );

            /* Update the sig chain with the new password if it has changed */
            if(strNewPassword != strPassword)
            {
                userUpdated.free();
                userUpdated = new TAO::Ledger::SignatureChain(session.GetAccount()->UserName(), strNewPassword);
            }

            /* Update the Crypto keys with the new pin */
            update_crypto_keys(userUpdated, strNewPin, tx);

            /* Calculate the prestates and poststates. */
            if(!tx.Build())
            {
                userUpdated.free();
                throw Exception(-30, "Operations failed to execute");
            }

            /* The private key to sign with.  This will either be a key based on the previous pin, OR if the recovery is being changed
               it will be the previous hashRecovery */
            uint512_t hashSecret = 0;

            /* If the recovery seed should be used */
            if(fRecovery)
            {
                /* Generate the recovery private key from the recovery seed  */
                hashSecret = userUpdated->Generate(strRecovery);

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

                /* Set the key type */
                tx.nKeyType = nKeyType;
            }
            else
            {
                /* If we are not using the recovery seed then generate the private key based on the pin / last sequence */
                hashSecret = GetKey(tx.nSequence, strPIN, session);
            }

            /* Sign the transaction . */
            if(!tx.Sign(hashSecret))
            {
                userUpdated.free();
                throw Exception(-31, "Ledger failed to sign transaction");
            }


            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
            {
                userUpdated.free();
                throw Exception(-32, "Failed to accept");
            }

            {

                /* Update the Password */
                session.UpdatePassword(strNewPassword);

                /* Update the cached txid used for auth */
                session.hashAuth = tx.GetHash();

                /* Update the cached pin in memory with the new pin */
                if(!session.GetActivePIN().IsNull() && !session.GetActivePIN()->PIN().empty())
                {
                    /* Get the existing unlocked actions so that we can maintain them */
                    uint8_t nUnlockedActions = session.GetActivePIN()->UnlockedActions();

                    session.UpdatePIN(strNewPin, nUnlockedActions);
                }

                /* free the temp sig chain we used for updating */
                userUpdated.free();
            }


            /* Add the transaction ID to the response */
            jsonRet["txid"]  = tx.GetHash().ToString();

            return jsonRet;
        }


        /* Generates new keys in the Crypto object register for a signature chain, using the specified pin, and adds the update
        *  contract to the transaction. */
        void Users::update_crypto_keys(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN, TAO::Ledger::Transaction& tx )
        {
            /* Generate register address for crypto register deterministically */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), user->Genesis(), TAO::Register::Address::CRYPTO);

            /* The Crypto object for this sig chain */
            TAO::Register::Object crypto;

            /* Retrieve the existing Crypto register so that we can update the keys. */
            if(!LLD::Register->ReadState(hashCrypto, crypto))
                throw Exception(-219, "Could not retrieve Crypto object");

            /* Parse the crypto object register so that we can read the current fields. */
            if(!crypto.Parse())
                throw Exception(-14, "Object failed to parse");

            /* Declare operation stream to serialize all of the field updates*/
            TAO::Operation::Stream ssOperationStream;

            /* Update the keys */
            if(crypto.get<uint256_t>("auth") != 0)
                ssOperationStream << std::string("auth") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("auth", 0, strPIN, tx.nKeyType);

            if(crypto.get<uint256_t>("lisp") != 0)
                ssOperationStream << std::string("lisp") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("lisp", 0, strPIN, tx.nKeyType);

            if(crypto.get<uint256_t>("network") != 0)
                ssOperationStream << std::string("network") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("network", 0, strPIN, tx.nKeyType);

            if(crypto.get<uint256_t>("sign") != 0)
                ssOperationStream << std::string("sign") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("sign", 0, strPIN, tx.nKeyType);

            if(crypto.get<uint256_t>("verify") != 0)
                ssOperationStream << std::string("verify") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("verify", 0, strPIN, tx.nKeyType);

            /* Add the crypto update contract. */
            tx[tx.Size()] << uint8_t(TAO::Operation::OP::WRITE) << hashCrypto << ssOperationStream.Bytes();
        }
    }
}
