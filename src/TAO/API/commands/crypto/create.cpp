/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLC/include/x509_cert.h>

#include <LLD/include/global.h>

#include <LLP/include/network.h>



#include <TAO/API/include/build.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/commands/crypto.h>

#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

#include <Util/include/base64.h>
#include <Util/include/debug.h>
#include <Util/include/encoding.h>
#include <Util/include/string.h>

#include <openssl/ssl.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Generates private key based on keyname/user/pass/pin and stores it in the keyname slot in the crypto register. */
        encoding::json Crypto::Create(const encoding::json& params, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json ret;

            /* Authenticate the users credentials */
            if(!Commands::Instance<Users>()->Authenticate(params))
                throw Exception(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = Commands::Instance<Users>()->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = Commands::Instance<Users>()->GetSession(params);

            /* Check the caller included the key name */
            if(params.find("name") == params.end() || params["name"].get<std::string>().empty())
                throw Exception(-88, "Missing or empty name.");

            /* Get the requested key name */
            std::string strName = params["name"].get<std::string>();

            /* Check they have not requested cert, as this requires a separate API method */
            if(strName == "cert")
                throw Exception(-291, "The cert key cannot be used to create a key pair as it is reserved for a TLS certificate.  Please use the create/certificate API method instead.");

            /* The logged in sig chain genesis hash */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* The address of the crypto object register, which is deterministic based on the genesis */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

            /* Read the crypto object register */
            TAO::Register::Object crypto;
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-259, "Could not read crypto object register");

            /* Parse the object. */
            if(!crypto.Parse())
                throw Exception(-36, "Failed to parse object register");

            /* Check to see if the key name is valid */
            if(!crypto.Check(strName))
                throw Exception(-260, "Invalid key name");

            /* Check to see if the the key already exists */
            if(crypto.get<uint256_t>(strName) != 0)
                throw Exception(-261, "Key already exists");

            /* Lock the signature chain. */
            LOCK(session.CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(session.GetAccount(), strPIN, tx))
                throw Exception(-17, "Failed to create transaction");

            /* The scheme to use to generate the key. If no specific scheme paramater has been passed in then this defaults to
               the key type set on the previous transaction */
            uint8_t nKeyType = tx.nKeyType;

            if(params.find("scheme") != params.end() )
            {
                std::string strScheme = ToLower(params["scheme"].get<std::string>());

                if(strScheme == "falcon")
                    nKeyType = TAO::Ledger::SIGNATURE::FALCON;
                else if(strScheme == "brainpool")
                    nKeyType = TAO::Ledger::SIGNATURE::BRAINPOOL;
                else
                    throw Exception(-262, "Invalid scheme");

            }

            /* Generate the new public key */
            uint256_t hashPublic = session.GetAccount()->KeyHash(strName, 0, strPIN, nKeyType);

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
                throw Exception(-44, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(session.GetAccount()->Generate(tx.nSequence, strPIN)))
                throw Exception(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw Exception(-32, "Failed to accept");

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
            ret["hashkey"] = hashPublic.ToString();

            return ret;
        }


        /* Generates an x509 certificate and stores a hash of the certificate data in the "cert" slot in the crypto register. */
        encoding::json Crypto::CreateCertificate(const encoding::json& params, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json ret;

            /* Authenticate the users credentials */
            if(!Commands::Instance<Users>()->Authenticate(params))
                throw Exception(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = Commands::Instance<Users>()->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = Commands::Instance<Users>()->GetSession(params);

            /* The logged in sig chain genesis hash */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* The address of the crypto object register, which is deterministic based on the genesis */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

            /* Read the crypto object register */
            TAO::Register::Object crypto;
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-259, "Could not read crypto object register");

            /* Parse the object. */
            if(!crypto.Parse())
                throw Exception(-36, "Failed to parse object register");

            /* Lock the signature chain. */
            LOCK(session.CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(session.GetAccount(), strPIN, tx))
                throw Exception(-17, "Failed to create transaction");

            /* X509 certificate instance*/
            LLC::X509Cert cert;

            /* Generate private key for the  */
            uint256_t hashSecret = session.GetAccount()->Generate("cert", 0, strPIN);

            /* Generate a new certificate using the sig chains genesis hash as the common name (CN) and the transaction timestamp
               as the valid from.  By using the transaction timestamp as the valid from time, we can reconstruct this exact
               certificate with identical hash at any time */
            cert.GenerateEC(hashSecret, hashGenesis.ToString(), tx.nTimestamp);

            /* Verify that it generated correctly */
            cert.Verify();

            /* The x509 certificate data in PEM format */
            std::vector<uint8_t> vchCertificate;

            /* Convert certificate to PEM-encoded bytes */
            cert.GetPEM(vchCertificate);

            /* Obtain a 256-bit hash of this certificate data to store in the crypto register */
            uint256_t hashCert = cert.Hash();

            /* Declare operation stream to serialize all of the field updates*/
            TAO::Operation::Stream ssOperationStream;

            /* Generate the new key */
            ssOperationStream << std::string("cert") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << hashCert;

            /* Add the crypto update contract. */
            tx[0] << uint8_t(TAO::Operation::OP::WRITE) << hashCrypto << ssOperationStream.Bytes();

            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw Exception(-44, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(session.GetAccount()->Generate(tx.nSequence, strPIN)))
                throw Exception(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw Exception(-32, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();

            /* Include the certificate data. This is a multiline string containing base64-encoded data, therefore we must base64
               encode it again in order to write it into a single JSON field*/
            ret["certificate"] = encoding::EncodeBase64(&vchCertificate[0], vchCertificate.size());

            /* Return the hash of the certificate*/
            ret["hashcert"] = hashCert.ToString();

            return ret;
        }
    }
}
