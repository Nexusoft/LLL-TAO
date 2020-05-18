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

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>
#include <Util/include/base64.h>
#include <Util/include/string.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Generates a signature for the data based on the private key for the keyname/user/pass/pin combination. */
        json::json Crypto::Sign(const json::json& params, bool fHelp)
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

            /* The public key from the crypto object register */
            uint256_t hashPublic = crypto.get<uint256_t>(strName); 
            
            /* Check to see if the the has been generated.  Even though the key is deterministic,  */
            if(hashPublic == 0)
                throw APIException(-264, "Key not yet created");

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
            
            /* Get the private key. */
            uint512_t hashSecret = user->Generate(strName, 0, strPIN);

            /* Buffer to receive the signature */
            std::vector<uint8_t> vchSig;

            /* Buffer to receive the public key */
            std::vector<uint8_t> vchPubKey;

            /* Get the scheme */
            uint8_t nScheme = get_scheme(params);

            /* Generate the signature */
            if(!user->Sign(nScheme, vchData, hashSecret, vchPubKey, vchSig))
                throw APIException(-273, "Failed to generate signature");

            ret["publickey"] = encoding::EncodeBase58(vchPubKey);

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

            ret["signature"] = encoding::EncodeBase64(&vchSig[0], vchSig.size());

            return ret;
        }
    }
}
