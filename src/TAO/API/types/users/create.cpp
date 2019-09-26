/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/types/address.h>
#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/types/names.h>
#include <TAO/API/types/users.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/create.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Create's a user account. */
        json::json Users::Create(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Pin parameter. */
            SecureString strPin;

            /* Check for username parameter. */
            if(params.find("username") == params.end())
                throw APIException(-127, "Missing username");

            /* Check for password parameter. */
            if(params.find("password") == params.end())
                throw APIException(-128, "Missing password");

            /* Check for pin parameter. Extract the pin. */
            if(params.find("pin") != params.end())
                strPin = SecureString(params["pin"].get<std::string>().c_str());
            else if(params.find("PIN") != params.end())
                strPin = SecureString(params["PIN"].get<std::string>().c_str());
            else
                throw APIException(-129, "Missing PIN");

            /* Extract the username  */
            SecureString strUsername = params["username"].get<std::string>().c_str();

            /* Extract the password */
            SecureString strPassword = params["password"].get<std::string>().c_str();

            /* Check username length */
            if(strUsername.length() < 3)
                throw APIException(-191, "Username must be a minimum of 3 characters");

            /* Check password length */
            if(strPassword.length() < 8)
                throw APIException(-192, "Password must be a minimum of 8 characters");

            /* Check pin length */
            if(strPin.length() < 4)
                throw APIException(-193, "Pin must be a minimum of 4 characters");

            /* The genesis transaction  */
            TAO::Ledger::Transaction tx;

            /* Create the sig chain genesis transaction */
            CreateSigchain(strUsername, strPassword, strPin, tx);

            /* Build a JSON response object. */
            ret["version"]   = tx.nVersion;
            ret["sequence"]  = tx.nSequence;
            ret["timestamp"] = tx.nTimestamp;
            ret["genesis"]   = tx.hashGenesis.ToString();
            ret["nexthash"]  = tx.hashNext.ToString();
            ret["prevhash"]  = tx.hashPrevTx.ToString();
            ret["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
            ret["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
            ret["hash"]      = tx.GetHash().ToString();

            return ret;
        }


        /* Creates a signature chain for the given credentials and returns the transaction object if successful */
        void Users::CreateSigchain(const SecureString& strUsername, const SecureString& strPassword,
                                   const SecureString& strPin, TAO::Ledger::Transaction& tx)
        {
            /* Generate the signature chain. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> user = new TAO::Ledger::SignatureChain(strUsername, strPassword);

            /* Get the Genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* Check for duplicates in ledger db. */
            if(LLD::Ledger->HasGenesis(hashGenesis) || TAO::Ledger::mempool.Has(hashGenesis))
            {
                user.free();
                throw APIException(-130, "Account already exists");
            }

            /* Create the transaction. */
            if(!TAO::Ledger::CreateTransaction(user, strPin, tx))
            {
                user.free();
                throw APIException(-17, "Failed to create transaction");
            }

            /* Generate register address for the trust account deterministically so that we can retrieve it easily later. */
            TAO::Register::Address hashRegister = TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

            /* Add a Name record for the trust account */
            tx[0] = Names::CreateName(user->Genesis(), "trust", "", hashRegister);

            /* Set up tx operation to create the trust account register at the same time as sig chain genesis. */
            tx[1] << uint8_t(TAO::Operation::OP::CREATE)      << hashRegister
                  << uint8_t(TAO::Register::REGISTER::OBJECT) << TAO::Register::CreateTrust().GetState();

            /* Generate a random hash for this objects register address */
            hashRegister = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

            /* Add a Name record for the trust account */
            tx[2] = Names::CreateName(user->Genesis(), "default", "", hashRegister);

            /* Add the default account register operation to the transaction */
            tx[3] << uint8_t(TAO::Operation::OP::CREATE)      << hashRegister
                  << uint8_t(TAO::Register::REGISTER::OBJECT) << TAO::Register::CreateAccount(0).GetState();

            /* Generate register address for crypto register deterministically */
            hashRegister = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

            /* The key type to use for the crypto keys */
            uint8_t nKeyType = config::GetBoolArg("-brainpool") ? TAO::Ledger::SIGNATURE::BRAINPOOL : TAO::Ledger::SIGNATURE::FALCON;

            /* Create the crypto object. */
            TAO::Register::Object crypto = TAO::Register::CreateCrypto(
                                                user->KeyHash("auth", 0, strPin, nKeyType),
                                                0, //lisp key disabled for now
                                                user->KeyHash("network", 0, strPin, nKeyType),
                                                user->KeyHash("sign", 0, strPin, nKeyType),
                                                0, //verify key disabled for now
                                                0, //cert disabled for now
                                                0, //app1 disabled for now
                                                0, //app2 disabled for now
                                                0);//app3 disabled for now

            /* Add the crypto register operation to the transaction */
            tx[4] << uint8_t(TAO::Operation::OP::CREATE)      << hashRegister
                  << uint8_t(TAO::Register::REGISTER::OBJECT) << crypto.GetState();

            /* Add the fee */
            AddFee(tx);

            /* Calculate the prestates and poststates. */
            if(!tx.Build())
            {
                user.free();
                throw APIException(-30, "Operations failed to execute");
            }

            /* Sign the transaction. */
            if(!tx.Sign(user->Generate(tx.nSequence, strPin)))
            {
                user.free();
                throw APIException(-31, "Ledger failed to sign transaction");
            }

            /* Free the sigchain. */
            user.free();

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept");
        }


    }
}
