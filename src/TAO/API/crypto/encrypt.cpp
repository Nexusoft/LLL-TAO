/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLC/include/encrypt.h>
#include <LLC/include/flkey.h>

#include <TAO/API/objects/types/objects.h>

#include <TAO/API/include/json.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/names/types/names.h>
#include <TAO/API/crypto/types/crypto.h>

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
        json::json Crypto::Encrypt(const json::json& params, const bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* The data to be encrypted */
            std::vector<uint8_t> vchData;

            /* The symmetric key used to encrypt */
            std::vector<uint8_t> vchKey;

            /* Check the caller included the data */
            if(params.find("data") == params.end() || params["data"].get<std::string>().empty())
                throw APIException(-18, "Missing data.");

            /* Decode the data into a vector of bytes */
            std::string strData = params["data"].get<std::string>();
            vchData.insert(vchData.begin(), strData.begin(), strData.end());

            /* Check for explicit key */
            if(params.find("key") != params.end())
            {
                /* Decode the key into a vector of bytes */
                std::string strKey = params["key"].get<std::string>();
                vchKey.insert(vchKey.begin(), strKey.begin(), strKey.end());
            }
            else
            {
                /* Check the caller included the key name */
                if(params.find("name") == params.end() || params["name"].get<std::string>().empty())
                    throw APIException(-88, "Missing or empty name.");

                /* Get the requested key name */
                std::string strName = params["name"].get<std::string>();

                /* Ensure the user has not requested to encrypt using one of the default keys
                    as these are for signature verification only */
                std::set<std::string> setDefaults{"auth", "lisp", "network", "sign", "verify", "cert", "app1", "app2", "app3"};
                if(setDefaults.find(strName) != setDefaults.end())
                    throw APIException(-293, "Invalid key name.  Keys in the crypto register cannot be used for encryption, only signature generation and verification");

                /* Authenticate the users credentials */
                if(!Commands::Get<Users>()->Authenticate(params))
                    throw APIException(-139, "Invalid credentials");

                /* Get the PIN to be used for this API call */
                SecureString strPIN = Commands::Get<Users>()->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

                /* Get the session to be used for this API call */
                Session& session = Commands::Get<Users>()->GetSession(params);

                /* Get the private key. */
                uint512_t hashSecret = session.GetAccount()->Generate(strName, 0, strPIN);

                /* The public key to return */
                std::vector<uint8_t> vchPubKey;

                /* The key type*/
                uint8_t nScheme = TAO::Ledger::SIGNATURE::BRAINPOOL;

                /* If a peer key has been provided then generate a shared key */
                if(params.find("peerkey") != params.end() && !params["peerkey"].get<std::string>().empty())
                {
                    /* Decode the public key into a vector of bytes */
                    std::vector<uint8_t> vchPeerKey;
                    if(!encoding::DecodeBase58(params["peerkey"].get<std::string>(), vchPeerKey))
                        throw APIException(-266, "Malformed public key.");

                    /* Get the secret from new key. */
                    std::vector<uint8_t> vBytes = hashSecret.GetBytes();
                    LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

                    /* Create an ECKey for the private key */
                    LLC::ECKey keyPrivate = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret key. */
                    if(!keyPrivate.SetSecret(vchSecret, true))
                        throw APIException(269, "Malformed private key");

                    /* Create an ECKey for the public key */
                    LLC::ECKey keyPublic = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);
                    if(!keyPublic.SetPubKey(vchPeerKey))
                        throw APIException(-266, "Malformed public key.");

                    /* Generate the shared symmetric key */
                    if(!LLC::ECKey::MakeShared(keyPrivate, keyPublic, vchKey))
                        throw APIException(268, "Failed to generate shared key");

                    /* Set the public key for our private key*/
                    vchPubKey = keyPrivate.GetPubKey();

                }
                else
                {
                    /* Get the scheme */
                    nScheme = get_scheme(params);

                    /* Otherwise we use the private key as the symmetric key */
                    vchKey = hashSecret.GetBytes();

                    /* Get the secret from key. */
                    LLC::CSecret vchSecret(vchKey.begin(), vchKey.end());

                    /* Get the public key to populate into the response */
                    if(nScheme == TAO::Ledger::SIGNATURE::BRAINPOOL)
                    {
                        /* Create an ECKey for the private key */
                        LLC::ECKey keyPrivate = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                        /* Set the secret key. */
                        if(!keyPrivate.SetSecret(vchSecret, true))
                            throw APIException(269, "Malformed private key");

                        /* Set the public key for our private key*/
                        vchPubKey = keyPrivate.GetPubKey();
                    }
                    else if(nScheme == TAO::Ledger::SIGNATURE::FALCON)
                    {
                        /* Create an FLKey for the private key */
                        LLC::FLKey keyPrivate = LLC::FLKey();

                        /* Set the secret key. */
                        if(!keyPrivate.SetSecret(vchSecret))
                            throw APIException(269, "Malformed private key");

                        /* Set the public key for our private key*/
                        vchPubKey = keyPrivate.GetPubKey();
                    }
                }

                /* Add the public key to the response */
                ret["publickey"] = encoding::EncodeBase58(vchPubKey);

                /* convert the scheme type to a string */
                switch(nScheme)
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

                /* add the hash key */
                ret["hashkey"] = LLC::SK256(vchPubKey).ToString();

                /* For added security the actual private key is not directly used as the symmetric key.  Therefore we hash
                   the private key. NOTE: the AES256 function requires a 32-byte key, so we reduce the length if necessary by using
                   the 256-bit version of the Skein Keccak hashing function */
                uint256_t nKey = LLC::SK256(vchKey);

                /* Put the key back into the vector */
                vchKey = nKey.GetBytes();

            }

            /* The encrypted data */
            std::vector<uint8_t> vchCipherText;

            /* Encrypt the data */
            if(!LLC::EncryptAES256(vchKey, vchData, vchCipherText))
                throw APIException(-270, "Failed to encrypt data.");

            /* Add the ciphertext to the response, base64 encoded*/
            ret["data"] = encoding::EncodeBase64(&vchCipherText[0], vchCipherText.size());


            return ret;
        }
    }
}
