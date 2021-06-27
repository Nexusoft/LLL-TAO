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

#include <LLP/include/global.h>

#include <TAO/API/names/types/names.h>
#include <TAO/API/users/types/users.h>

#include <TAO/API/include/build.h>

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
        encoding::json Users::Create(const encoding::json& params, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json ret;

            /* Pin parameter. */
            SecureString strPin;

            /* Check for username parameter. */
            if(params.find("username") == params.end())
                throw Exception(-127, "Missing username");

            /* Check for password parameter. */
            if(params.find("password") == params.end())
                throw Exception(-128, "Missing password");

            /* Check for pin parameter. Extract the pin. */
            if(params.find("pin") != params.end())
                strPin = SecureString(params["pin"].get<std::string>().c_str());
            else if(params.find("PIN") != params.end())
                strPin = SecureString(params["PIN"].get<std::string>().c_str());
            else
                throw Exception(-129, "Missing PIN");

            /* Extract the username  */
            SecureString strUsername = params["username"].get<std::string>().c_str();

            /* Extract the password */
            SecureString strPassword = params["password"].get<std::string>().c_str();

            /* Check username length */
            if(strUsername.length() < 2)
                throw Exception(-191, "Username must be a minimum of 2 characters");

            /* Check password length */
            if(strPassword.length() < 8)
                throw Exception(-192, "Password must be a minimum of 8 characters");

            /* Check pin length */
            if(strPin.length() < 4)
                throw Exception(-193, "Pin must be a minimum of 4 characters");

            /* The genesis transaction  */
            TAO::Ledger::Transaction tx;

            /* Create the sig chain genesis transaction */
            create_sig_chain(strUsername, strPassword, strPin, tx);

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
        void Users::create_sig_chain(const SecureString& strUsername, const SecureString& strPassword,
                                   const SecureString& strPin, TAO::Ledger::Transaction& tx)
        {
            /* Generate the signature chain. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain> user = new TAO::Ledger::SignatureChain(strUsername, strPassword);

            /* Get the Genesis ID. */
            uint256_t hashGenesis = user->Genesis();

            /* In client mode, in order to check whether the username already exists we have to do things differently as it is
               possible that the local db does not have the genesis as it has never been used by the node.  In which case we
               need to request the genesis transaction from a peer and then check again */
            if(config::fClient.load() && !LLD::Ledger->HasGenesis(hashGenesis))
            {
                 /* Check tritium server enabled. */
                if(LLP::TRITIUM_SERVER)
                {
                    std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
                    if(pNode != nullptr)
                    {
                        /* Request the genesis hash from the peer. */
                        debug::log(1, FUNCTION, "CLIENT MODE: Requesting GET::GENESIS for ", hashGenesis.SubString());

                        LLP::TritiumNode::BlockingMessage(10000, pNode.get(), LLP::TritiumNode::ACTION::GET, uint8_t(LLP::TritiumNode::TYPES::GENESIS), hashGenesis);

                        debug::log(1, FUNCTION, "CLIENT MODE: GET::GENESIS received for ", hashGenesis.SubString());
                    }
                }

            }

            /* Check for duplicates in ledger db. */
            if(LLD::Ledger->HasGenesis(hashGenesis) || TAO::Ledger::mempool.Has(hashGenesis))
            {
                user.free();
                throw Exception(-130, "Account already exists");
            }

            /* Create the transaction. */
            if(!Users::CreateTransaction(user, strPin, tx))
            {
                user.free();
                throw Exception(-17, "Failed to create transaction");
            }

            TAO::Register::Address hashRegister;

            /* Don't create trust/default accounts in private mode */
            #ifndef UNIT_TESTS //API unit tests use -private flag, so we must disable this check
            if(!config::fHybrid.load())
            #endif
            {
                /* Generate register address for the trust account deterministically so that we can retrieve it easily later. */
                hashRegister = TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

                /* Add a Name record for the trust account */
                tx[0] = Names::CreateName(user->Genesis(), "trust", "", hashRegister);

                /* Set up tx operation to create the trust account register at the same time as sig chain genesis. */
                tx[1] << uint8_t(TAO::Operation::OP::CREATE)      << hashRegister
                      << uint8_t(TAO::Register::REGISTER::OBJECT) << TAO::Register::CreateTrust().GetState();

                /* Generate a random hash for the default account register address */
                hashRegister = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

                /* Add a Name record for the default account */
                tx[2] = Names::CreateName(user->Genesis(), "default", "", hashRegister);

                /* Add the default account register operation to the transaction */
                tx[3] << uint8_t(TAO::Operation::OP::CREATE)      << hashRegister
                      << uint8_t(TAO::Register::REGISTER::OBJECT) << TAO::Register::CreateAccount(0).GetState();
            }

            /* Generate register address for crypto register deterministically */
            hashRegister = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

            /* The key type to use for the crypto keys */
            uint8_t nKeyType = config::GetBoolArg("-falcon") ? TAO::Ledger::SIGNATURE::FALCON : TAO::Ledger::SIGNATURE::BRAINPOOL;

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
                throw Exception(-30, "Operations failed to execute");
            }

            /* Sign the transaction. */
            if(!tx.Sign(user->Generate(tx.nSequence, strPin)))
            {
                user.free();
                throw Exception(-31, "Ledger failed to sign transaction");
            }

            /* Free the sigchain. */
            user.free();

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw Exception(-32, "Failed to accept");
        }


    }
}
