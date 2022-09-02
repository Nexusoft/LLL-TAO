/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/commands/names.h>
#include <TAO/API/types/commands/profiles.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/create.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/memory.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Login to a user account. */
    encoding::json Profiles::Create(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our type we are processing. */
        const std::string strType = ExtractType(jParams);

        /* The new key scheme */
        const uint8_t nScheme =
            ExtractScheme(jParams, "brainpool, falcon");

        /* Check for username parameter. */
        if(!CheckParameter(jParams, "username", "string"))
            throw Exception(-127, "Missing username");

        /* Parse out username. */
        const SecureString strUser =
            SecureString(jParams["username"].get<std::string>().c_str());

        /* Check username length */
        if(strUser.length() < 2)
            throw Exception(-191, "Username must be a minimum of 2 characters");

        /* Check for password parameter. */
        if(!CheckParameter(jParams, "password", "string"))
            throw Exception(-128, "Missing password");

        /* Parse out password. */
        const SecureString strPass =
            SecureString(jParams["password"].get<std::string>().c_str());

        /* Check password length */
        if(strPass.length() < 8)
            throw Exception(-192, "Password must be a minimum of 8 characters");

        /* Pin parameter. */
        const SecureString strPIN = ExtractPIN(jParams);

        /* Check pin length */
        if(strPIN.length() < 4)
            throw Exception(-193, "Pin must be a minimum of 4 characters");

        /* Create a temp sig chain for checking credentials */
        memory::encrypted_ptr<TAO::Ledger::SignatureChain> pCredentials =
            new TAO::Ledger::SignatureChain(strUser, strPass);

        /* Get our genesis-id for local checks. */
        const uint256_t hashGenesis = pCredentials->Genesis();

        /* Only allow crypto if we already have a sigchain. */
        if(strType == "auth")
        {
            /* Generate register address for crypto register deterministically */
            const uint256_t hashCrypto =
                TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

            /* Read our crypto object register. */
            TAO::Register::Object oCrypto;
            if(!LLD::Register->ReadObject(hashCrypto, oCrypto))
                throw Exception(-130, "Can't generate crypto object register if no sigchain");

            /* Check for a disabled auth key. */
            if(oCrypto.get<uint256_t>("auth") != 0)
                throw Exception(-130, "Can't generate auth key if already enabled");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!BuildCredentials(pCredentials, strPIN, nScheme, tx))
                throw Exception(-17, "Failed to create transaction");

            /* Sign the transaction. */
            if(!tx.Sign(pCredentials->Generate(tx.nSequence, strPIN)))
            {
                pCredentials.free();
                throw Exception(-31, "Ledger failed to sign transaction");
            }

            /* Free our credentials object. */
            pCredentials.free();

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw Exception(-32, "Failed to accept");

            /* Build a JSON response object. */
            encoding::json jRet;
            jRet["success"] = true; //just a little response for if using -autotx
            jRet["txid"] = tx.GetHash().ToString();

            return jRet;
        }

        /* Check for duplicates in ledger db. */
        if(LLD::Ledger->HasFirst(hashGenesis) || TAO::Ledger::mempool.Has(hashGenesis))
        {
            pCredentials.free();
            throw Exception(-130, "Account already exists");
        }


        /* Create the transaction. */
        TAO::Ledger::Transaction tx;
        if(!TAO::Ledger::CreateTransaction(pCredentials, strPIN, tx, nScheme))
        {
            pCredentials.free();
            throw Exception(-17, "Failed to create transaction");
        }

        /* Don't create trust/default accounts in private mode */
        #ifndef UNIT_TESTS //API unit tests use -private flag, so we must disable this check
        if(!config::fHybrid.load())
        #endif
        {
            /* Generate register address for the trust account deterministically so that we can retrieve it easily later. */
            const uint256_t hashTrust =
                TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

            /* Add a Name record for the trust account */
            tx[0] = Names::CreateName(hashGenesis, "trust", "", hashTrust);

            /* Set up tx operation to create the trust account register at the same time as sig chain genesis. */
            tx[1] << uint8_t(TAO::Operation::OP::CREATE)      << hashTrust
                  << uint8_t(TAO::Register::REGISTER::OBJECT) << TAO::Register::CreateTrust().GetState();

            /* Generate a random hash for the default account register address */
            const uint256_t hashDefault =
                TAO::Register::Address(TAO::Register::Address::ACCOUNT);

            /* Add a Name record for the default account */
            tx[2] = Names::CreateName(hashGenesis, "default", "", hashDefault);

            /* Add the default account register operation to the transaction */
            tx[3] << uint8_t(TAO::Operation::OP::CREATE)      << hashDefault
                  << uint8_t(TAO::Register::REGISTER::OBJECT) << TAO::Register::CreateAccount(0).GetState();
        }

        /* Generate register address for crypto register deterministically */
        const uint256_t hashCrypto =
            TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

        /* Create the crypto object. */
        const TAO::Register::Object oCrypto =
            TAO::Register::CreateCrypto
            (
                pCredentials->KeyHash("auth", 0, strPIN,    TAO::Ledger::SIGNATURE::BRAINPOOL),
                0, //lisp key disabled for now
                pCredentials->KeyHash("network", 0, strPIN, TAO::Ledger::SIGNATURE::BRAINPOOL),
                pCredentials->KeyHash("sign", 0, strPIN,    TAO::Ledger::SIGNATURE::BRAINPOOL),
                0, //verify key disabled for now
                0, //cert disabled for now
                0, //app1 disabled for now
                0, //app2 disabled for now
                0  //app3 disabled for now
            );

        /* Add the crypto register operation to the transaction */
        tx[tx.Size()] << uint8_t(TAO::Operation::OP::CREATE)      << hashCrypto
                      << uint8_t(TAO::Register::REGISTER::OBJECT) << oCrypto.GetState();

        /* Add the contract fees. */
        AddFee(tx);

        /* Execute the operations layer. */
        if(!tx.Build())
        {
            pCredentials.free();
            throw Exception(-44, "Transaction failed to build");
        }

        /* Sign the transaction. */
        if(!tx.Sign(pCredentials->Generate(tx.nSequence, strPIN)))
        {
            pCredentials.free();
            throw Exception(-31, "Ledger failed to sign transaction");
        }

        /* Double check our next hash if -safemode enabled. */
        if(config::GetBoolArg("-safemode", false))
        {
            /* Re-calculate our next hash if safemode forcing not to use cache. */
            const uint256_t hashNext =
                TAO::Ledger::Transaction::NextHash(pCredentials->Generate(tx.nSequence + 1, strPIN, false), tx.nNextType);

            /* Check that this next hash is what we are expecting. */
            if(tx.hashNext != hashNext)
            {
                pCredentials.free();
                throw Exception(-67, "-safemode next hash mismatch, broadcast terminated");
            }
        }

        /* Execute the operations layer. */
        if(!TAO::Ledger::mempool.Accept(tx))
        {
            pCredentials.free();
            throw Exception(-32, "Failed to accept");
        }

        /* Free our credentials object now. */
        pCredentials.free();

        /* Build a JSON response object. */
        encoding::json jRet;
        jRet["success"] = true; //just a little response for if using -autotx
        jRet["txid"] = tx.GetHash().ToString();

        return jRet;
    }
}
