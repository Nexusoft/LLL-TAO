/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLC/hash/argon2.h>

#include <TAO/API/types/objects.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/create.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>
#include <Util/include/base58.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Returns the public key from the crypto object register for the specified key name, from the specified signature chain. */
        json::json Crypto::Get(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Get the Genesis ID. */
            uint256_t hashGenesis = 0;

            /* Check for specified username / genesis. If no specific genesis or username
             * have been provided then fall back to the active sigchain. */
            if(params.find("genesis") != params.end())
                hashGenesis.SetHex(params["genesis"].get<std::string>());

            /* Check for username. */
            else if(params.find("username") != params.end())
                hashGenesis = TAO::Ledger::SignatureChain::Genesis(params["username"].get<std::string>().c_str());
            
            /* use logged in session. */
            else 
                hashGenesis = users->GetGenesis(users->GetSession(params));
           
            /* Prevent foreign data lookup in client mode */
            if(config::fClient.load() && hashGenesis != users->GetCallersGenesis(params))
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Check genesis exists */
            if(!LLD::Ledger->HasGenesis(hashGenesis))
                throw APIException(-258, "Unknown genesis");

            /* Check the caller included the key name */
            if(params.find("name") == params.end() || params["name"].get<std::string>().empty())
                throw APIException(-88, "Missing name.");
            
            /* Get the requested key name */
            std::string strName = params["name"].get<std::string>();

            /* The address of the crypto object register, which is deterministic based on the genesis */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
            
            /* Read the crypto object register */
            TAO::Register::Object crypto;
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-259, "Could not read crypto object register");

            /* Parse the object. */
            if(!crypto.Parse())
                throw APIException(-36, "Failed to parse object register");
            
            /* Check the key exists */
            if(!crypto.CheckName(strName))
                throw APIException(-260, "Invalid key name");

            /* Get List of key names in the crypto object */
            std::vector<std::string> vKeys = crypto.GetFieldNames();        

            /* Get the public key */
            uint256_t hashPublic = crypto.get<uint256_t>(strName);

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
            ret["hashkey"] = hashPublic == 0 ? "" : encoding::EncodeBase58(hashPublic.GetBytes());

            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            FilterResponse(params, ret);

            return ret;
        }


        /* Generates and returns the public key for a key stored in the crypto object register. */
        json::json Crypto::GetPublic(const json::json& params, bool fHelp)
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

            /* The scheme to use to generate the key. */
            uint8_t nKeyType = get_scheme(params);
            
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
                        
            /* Generate the public key */
            std::vector<uint8_t> vchPubKey = user->Key(strName, 0, strPIN, nKeyType);

            /* Populate the key, base64 encoded */
            ret["publickey"] = encoding::EncodeBase58(vchPubKey);

            /* convert the scheme type to a string */
            switch(nKeyType)
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

            return ret;
        }


        /* Generates and returns the private key for a key stored in the crypto object register. */
        json::json Crypto::GetPrivate(const json::json& params, bool fHelp)
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

            /* Ensure the user has not requested the private key for one of the default keys, only the app or additional 3rd party keys */
            std::set<std::string> setDefaults{"auth", "lisp", "network", "sign", "verify", "cert"};
            if(setDefaults.find(strName) != setDefaults.end())
                throw APIException(-263, "Private key can only be retrieved for app1, app2, app3, and other 3rd-party keys");

            /* The logged in sig chain genesis hash */
            uint256_t hashGenesis = user->Genesis();            

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
                        
            /* Generate the private key */
            uint512_t hashPrivate = user->Generate(strName, 0, strPIN);

            /* Populate the key, hex encoded */
            ret["privatekey"] = hashPrivate.ToString();

            return ret;
        }

        /* Generates a hash of the data using the requested hashing function. */
        json::json Crypto::GetHash(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

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

            
            /* The hash function to use.  This default to SK256 */
            std::string strFunction = "sk256";
            
            /* Check they included the hashing function */
            if(params.find("function") != params.end())
                strFunction = ToLower(params["function"].get<std::string>());

            /* hash based on the requested function */
            if(strFunction == "sk256")
            {
                uint256_t hashData = LLC::SK256(vchData);
                ret["hash"] = hashData.ToString();
            }
            else if(strFunction == "sk512")
            {
                uint512_t hashData = LLC::SK512(vchData);
                ret["hash"] = hashData.ToString();
            }
            else if(strFunction == "argon2")
            {
            
                // vectors used by the argon2 API
                std::vector<uint8_t> vHash(64);
                std::vector<uint8_t> vSalt(16);
                std::vector<uint8_t> vSecret(16);
                
                /* Create the hash context. */
                argon2_context context =
                {
                    /* Hash Return Value. */
                    &vHash[0],
                    64,

                    /* The secret (not used). */
                    &vSecret[0],
                    static_cast<uint32_t>(vSecret.size()),

                    /* The salt (not used) */
                    &vSalt[0],
                    static_cast<uint32_t>(vSalt.size()),

                    /* Optional secret data */
                    NULL, 0,

                    /* Optional associated data */
                    NULL, 0,

                    /* Computational Cost. */
                    64,

                    /* Memory Cost (64 MB). */
                    (1 << 16),

                    /* The number of threads and lanes */
                    1, 1,

                    /* Algorithm Version */
                    ARGON2_VERSION_13,

                    /* Custom memory allocation / deallocation functions. */
                    NULL, NULL,

                    /* By default only internal memory is cleared (pwd is not wiped) */
                    ARGON2_DEFAULT_FLAGS
                };

                /* Run the argon2 computation. */
                int32_t nRet = argon2id_ctx(&context);
                if(nRet != ARGON2_OK)
                    throw APIException(-276, "Error generating hash");
                
                uint512_t hashData(vHash);

                ret["hash"] = hashData.ToString();
            }
            else
            {
                throw APIException(-277, "Invalid hash function");
            }
            
             

            return ret;
        }
    }
}
