/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLC/hash/argon2.h>
#include <LLC/include/x509_cert.h>

#include <TAO/API/objects/types/objects.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>

#include <TAO/Operation/include/enum.h>

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
                hashGenesis = users->GetSession(params).GetAccount()->Genesis();
           
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
            ret["hashkey"] = hashPublic == 0 ? "" : hashPublic.ToString();

            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            FilterResponse(params, ret);

            return ret;
        }


        /* Generates and returns the public key for a key stored in the crypto object register. */
        json::json Crypto::GetPublic(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Authenticate the users credentials */
            if(!users->Authenticate(params))
                throw APIException(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Check the caller included the key name */
            if(params.find("name") == params.end() || params["name"].get<std::string>().empty())
                throw APIException(-88, "Missing name.");
            
            /* Get the requested key name */
            std::string strName = params["name"].get<std::string>();

            /* The logged in sig chain genesis hash */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* The scheme to use to generate the key. */
            uint8_t nKeyType = get_scheme(params);

            /* Generate the public key */
            std::vector<uint8_t> vchPubKey = session.GetAccount()->Key(strName, 0, strPIN, nKeyType);

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

            /* Authenticate the users credentials */
            if(!users->Authenticate(params))
                throw APIException(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Check the caller included the key name */
            if(params.find("name") == params.end() || params["name"].get<std::string>().empty())
                throw APIException(-88, "Missing name.");
            
            /* Get the requested key name */
            std::string strName = params["name"].get<std::string>();

            /* Ensure the user has not requested the private key for one of the default keys 
               as these are for signature verification only */
            std::set<std::string> setDefaults{"auth", "lisp", "network", "sign", "verify", "cert", "app1", "app2", "app3"};
            if(setDefaults.find(strName) != setDefaults.end())
                throw APIException(-263, "Private keys cannot only be retrieved for keys in the crypto register");
     
            /* Generate the private key */
            uint512_t hashPrivate = session.GetAccount()->Generate(strName, 0, strPIN);

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
            std::string strData = params["data"].get<std::string>();
            std::vector<uint8_t> vchData(strData.begin(), strData.end());
      
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


        /* Returns the last generated x509 certificate for this sig chain. */
        json::json Crypto::GetCertificate(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Authenticate the users credentials */
            if(!users->Authenticate(params))
                throw APIException(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* The logged in sig chain genesis hash */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* The address of the crypto object register, which is deterministic based on the genesis */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
            
            /* Read the crypto object register */
            TAO::Register::Object crypto;
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-259, "Could not read crypto object register");

            /* Parse the object. */
            if(!crypto.Parse())
                throw APIException(-36, "Failed to parse object register");
            
            /* Get the existing certificate hash */
            uint256_t hashCert = crypto.get<uint256_t>("cert");

            /* Check that the certificate has been created */
            if(hashCert == 0)
                throw APIException(-294, "Certificate has not yet been created for this signature chain.  Please use crypto/create/key to create the certificate first.");

            /* The vertificate valid from timestamp, which is taken from the transaction time when the cert was last updated*/
            uint64_t nCertValid = 0;
            
            /* Scan the sig chain to find the transaction that created the certificate */
            /* Get the last transaction. */
            uint512_t hashLast = 0;

            /* Get the last transaction for this genesis.  NOTE that we include the mempool here as there may be registers that
               have been created recently but not yet included in a block*/
            LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL);

            /* The previous hash in the chain */
            uint512_t hashPrev = hashLast;

            /* Loop until genesis. */
            while(hashPrev != 0 && nCertValid == 0)
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashPrev, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-108, "Failed to read transaction");

                /* Set the next last. */
                hashPrev = !tx.IsFirst() ? tx.hashPrevTx : 0;

                /* Iterate through all contracts. */
                for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                {
                    /* Get the contract output. */
                    const TAO::Operation::Contract& contract = tx[nContract];

                    /* Reset all streams */
                    contract.Reset();

                    /* Seek the contract operation stream to the position of the primitive. */
                    contract.SeekToPrimitive();

                    /* Deserialize the OP. */
                    uint8_t nOP = 0;
                    contract >> nOP;

                    /* Check the current opcode. */
                    switch(nOP)
                    {

                        /* These are the register-based operations that prove ownership if encountered before a transfer*/
                        case TAO::Operation::OP::WRITE:
                        {
                            /* Extract the address from the contract. */
                            TAO::Register::Address hashAddress;
                            contract >> hashAddress;

                            /* If the address is the crypto register then check which fields were updated */
                            if(hashAddress == hashCrypto)
                            {
                                /* The operation data */
                                std::vector<uint8_t> vchData;

                                /* Deserialize the operation stream data */
                                contract >> vchData;

                                /* Stream to allow parsing of the operation data */
                                TAO::Operation::Stream ssOperation(vchData);

                                /* The field within the crypto register that was updated by this contract. */
                                std::string strField;

                                /* Check te entire stream as there may be multiple fields updated in one contract */
                                while(!ssOperation.end())
                                {
                                    /*  */
                                    /* Deserialize the field being written */
                                    ssOperation >> strField;

                                    /* Check if it is the cert field */
                                    if(strField == "cert")
                                    {
                                        /* If so then use thus as the cert timestamp and break out */
                                        nCertValid = tx.nTimestamp;
                                        break;
                                    }
                                    else
                                        /* seek to next op */
                                        ssOperation.seek(33);
                                }
                            }

                            break;
                        }

                    }
                }

            }


            /* X509 certificate instance*/
            LLC::X509Cert cert;

            /* Generate private key for the  */
            uint256_t hashSecret = session.GetAccount()->Generate("cert", 0, strPIN);

            /* Generate a new certificate using the sig chains genesis hash as the common name (CN) and the transaction timestamp 
               as the valid from.  By using the transaction timestamp as the valid from time, we can reconstruct this exact 
               certificate with identical hash at any time */
            cert.GenerateEC(hashSecret, hashGenesis.ToString(), nCertValid);

            /* Verify that it generated correctly */
            cert.Verify();

            /* The x509 certificate data in PEM format */
            std::vector<uint8_t> vCertificate;

            /* Convert certificate to PEM-encoded bytes */
            cert.GetPEM(vCertificate);

            /* Obtain a 256-bit hash of this certificate data to check against crypto register */
            uint256_t hashCertCheck = cert.Hash();  

            if(hashCertCheck != hashCert)
                throw APIException(-295, "Unable to generate certificate");
            
            /* Include the certificate data. This is a multiline string containing base64-encoded data, therefore we must base64
               encode it again in order to write it into a single JSON field*/
            ret["certificate"] = encoding::EncodeBase64(&vCertificate[0], vCertificate.size());
            
            /* Return the hash of the certificate data */
            ret["hashcert"] = hashCert.ToString();

            return ret;
        }
    }
}
