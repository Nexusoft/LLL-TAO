/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLC/include/encrypt.h>

#include <TAO/API/types/objects.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>
#include <Util/include/base64.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Generates private key based on keyname/user/pass/pin and stores it in the keyname slot in the crypto register. */
        json::json Crypto::Decrypt(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Create an ECKey for the private key */
            LLC::ECKey keyPrivate = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

            /* The 512-bit private key data*/
            std::vector<uint8_t> vchPrivate;

            /* If the caller has provided the private key parameter then use that, otherwise they must supply a key name and pin
               to generate the key based on the logged in signature chain */
            if(params.find("privatekey") != params.end())
            {
                /* Decode the hex private key into a 512-bit integer  */
                uint512_t nPrivate;
                nPrivate.SetHex(params["privatekey"].get<std::string>());

                /* Grab the bytes into a vector for later use */
                vchPrivate = nPrivate.GetBytes();
            }
            else
            {
            
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
    
                /* Check to see if the the has been generated.  Even though the key is deterministic,  */
                if(crypto.get<uint256_t>(strName) == 0)
                    throw APIException(-264, "Key not yet created");

                /* Check that it is a brainpool key */
                if(crypto.get<uint256_t>(strName).GetType() != TAO::Ledger::SIGNATURE::BRAINPOOL)
                    throw APIException(-267, "Encryption only supported for EC (Brainpool) keys");

                /* Get the last transaction. */
                uint512_t hashLast;
                if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-138, "No previous transaction found");

                /* Get previous transaction */
                TAO::Ledger::Transaction txPrev;
                if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-138, "No previous transaction found");

                /* Generate a new transaction hash next using the current credentials so that we can verify them. */
                TAO::Ledger::Transaction tx;
                tx.NextHash(user->Generate(txPrev.nSequence + 1, strPIN, false), txPrev.nNextType);

                /* Check the calculated next hash matches the one on the last transaction in the sig chain. */
                if(txPrev.hashNext != tx.hashNext)
                    throw APIException(-139, "Invalid credentials");

                /* Get the private key. */
                vchPrivate = user->Generate(strName, 0, strPIN).GetBytes();
            
            }

            /* Get the secret from the private key data. */
            LLC::CSecret vchSecret(vchPrivate.begin(), vchPrivate.end());            

            /* Set the secret key. */
            if(!keyPrivate.SetSecret(vchSecret, true))
                throw APIException(267, "Encryption only supported for EC (Brainpool) keys");

            /* Check the caller included the publickey  */
            if(params.find("publickey") == params.end() || params["publickey"].get<std::string>().empty())
                throw APIException(-265, "Missing public key.");

            /* Decode the public key into a vector of bytes */
            std::vector<uint8_t> vchPublic;
            if(!encoding::DecodeBase58(params["publickey"].get<std::string>(), vchPublic))
                throw APIException(-266, "Malformed public key.");
            

            /* Check the caller included the data */
            if(params.find("data") == params.end() || params["data"].get<std::string>().empty())
                throw APIException(-18, "Missing data.");

            /* Decode the data into a vector of bytes */
            std::vector<uint8_t> vchData;
            try
            {
                vchData = encoding::DecodeBase64(params["data"].get<std::string>().c_str());
            }
            catch(const std::exception& e)
            {
                throw APIException(-27, "Malformed base64 encoding.");
            }
            

            /* Create an ECKey for the public key */
            LLC::ECKey keyPublic = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);
            if(!keyPublic.SetPubKey(vchPublic))
                throw APIException(-266, "Malformed public key.");

            /* Generate the shared symmetric key */
            std::vector<uint8_t> vchShared;
            if(!LLC::ECKey::MakeShared(keyPrivate, keyPublic, vchShared))
                throw APIException(268, "Failed to generate shared key");

            /* The decrypted data */
            std::vector<uint8_t> vchPlainText;

            /* Encrypt the data */
            if(!LLC::DecryptAES256(vchShared, vchData, vchPlainText))
                throw APIException(-271, "Failed to decrypt data.");
            
            /* Add the decrypted data to the response, base64 encoded*/
            ret["data"] = encoding::EncodeBase64(&vchPlainText[0], vchPlainText.size());

            return ret;
        }
    }
}
