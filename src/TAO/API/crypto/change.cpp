/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/objects/types/objects.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/crypto/types/crypto.h>

#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/encoding.h>
#include <Util/include/string.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Change the signature scheme used to generate the public-private keys for the users signature chain as well
           as keys stored in the crypto object register. */
        encoding::json Crypto::ChangeScheme(const encoding::json& params, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json ret;

            /* The new key scheme */
            uint8_t nScheme = 0;

            /* Authenticate the users credentials */
            if(!Commands::Get<Users>()->Authenticate(params))
                throw APIException(-139, "Invalid credentials");

            /* Get the PIN to be used for this API call */
            SecureString strPIN = Commands::Get<Users>()->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = Commands::Get<Users>()->GetSession(params);

            /* Get the account. */
            const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = session.GetAccount();
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Check the caller included the key name */
            if(params.find("scheme") == params.end() || params["scheme"].get<std::string>().empty())
                throw APIException(-275, "Missing scheme.");

            /* Get the scheme string */
            std::string strScheme = ToLower(params["scheme"].get<std::string>());

            /* Convert to scheme type */
            if(strScheme == "falcon")
                nScheme = TAO::Ledger::SIGNATURE::FALCON;
            else if(strScheme == "brainpool")
                nScheme = TAO::Ledger::SIGNATURE::BRAINPOOL;
            else
                throw APIException(-262, "Invalid scheme.");

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

            /* Lock the signature chain. */
            LOCK(session.CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* Check the existing scheme */
            if(tx.nNextType == nScheme)
                throw APIException(-278, "Scheme already in use");

            /* Set the new key scheme */
            tx.nNextType = nScheme;

            /* Regenerate the next hashed public key based on the new scheme */
            tx.NextHash(user->Generate(tx.nSequence + 1, strPIN), tx.nNextType);

            /* Declare operation stream to serialize all of the field updates*/
            TAO::Operation::Stream ssOperationStream;

            /* Regenerate any keys in the crypto object register */
            if(crypto.get<uint256_t>("auth") != 0)
                ssOperationStream << std::string("auth") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("auth", 0, strPIN, tx.nNextType);

            if(crypto.get<uint256_t>("lisp") != 0)
                ssOperationStream << std::string("lisp") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("lisp", 0, strPIN, tx.nNextType);

            if(crypto.get<uint256_t>("network") != 0)
                ssOperationStream << std::string("network") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("network", 0, strPIN, tx.nNextType);

            if(crypto.get<uint256_t>("sign") != 0)
                ssOperationStream << std::string("sign") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("sign", 0, strPIN, tx.nNextType);

            if(crypto.get<uint256_t>("verify") != 0)
                ssOperationStream << std::string("verify") << uint8_t(TAO::Operation::OP::TYPES::UINT256_T) << user->KeyHash("verify", 0, strPIN, tx.nNextType);

            /* Add the crypto update contract. */
            tx[tx.Size()] << uint8_t(TAO::Operation::OP::WRITE) << hashCrypto << ssOperationStream.Bytes();

            /* Add the fee */
            AddFee(tx);

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-44, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(session.GetAccount()->Generate(tx.nSequence, strPIN)))
                throw APIException(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-32, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"]  = tx.GetHash().ToString();

            return ret;
        }
    }
}
