/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/commands/profiles.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/credentials.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Recover a sig chain and set new credentials by supplying the recovery seed */
    encoding::json Profiles::Recover(const encoding::json& jParams, const bool fHelp)
    {
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

        /* Check for username parameter. */
        if(!CheckParameter(jParams, "recovery", "string"))
            throw Exception(-127, "Missing recovery");

        /* Parse out username. */
        const SecureString strRecovery =
            SecureString(jParams["recovery"].get<std::string>().c_str());

        /* Pin parameter. */
        const SecureString strPIN =
            ExtractPIN(jParams);

        /* Create a temp sig chain for checking credentials */
        memory::encrypted_ptr<TAO::Ledger::Credentials> pCredentials =
            new TAO::Ledger::Credentials(strUser, strPass);

        /* Get our genesis-id for local checks. */
        const uint256_t hashGenesis =
            pCredentials->Genesis();

        /* Check for duplicates in ledger db. */
        uint512_t hashLast;
        if(!LLD::Ledger->HasFirst(hashGenesis) || !LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
        {
            pCredentials.free();
            throw Exception(-130, "Account doesn't exist");
        }

        /* Get previous transaction */
        TAO::Ledger::Transaction txPrev;
        if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-138, "No previous transaction found");

        /* Check that the recovery hash has been set on this signature chain */
        if(txPrev.hashRecovery == 0)
            throw Exception(-223, "Recovery seed not set on this signature chain");

        /* The recovery hash to search for */
        const uint256_t hashRecovery =
            txPrev.hashRecovery;

        /* Iterate backwards until we reach the transaction where the hashRecovery differs */
        uint8_t nKeyType = 0;
        while(txPrev.hashRecovery == hashRecovery)
        {
            /* Set our key type from iterated transaction. */
            nKeyType = txPrev.nKeyType;

            /* If we fail to read, we have reached the end of sigchain. */
            if(!LLD::Ledger->ReadTx(txPrev.hashPrevTx, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-138, "No previous transaction found");
        }

        /* Check if we are using legacy recovery or not. */
        bool fDeprecatedRecovery = false;

        /* Check our recovery hash is correct. */
        const uint256_t hashRecoveryCheck = pCredentials->RecoveryHash(strRecovery, nKeyType);
        if(hashRecoveryCheck != hashRecovery)
        {
            /* Give ourselves a little log output to know we are proceeding with legacy recovery. */
            debug::warning("Invalid recovery, ", hashRecoveryCheck.ToString(), " is not ", hashRecovery.ToString(), ", checking our deprecated recovery hash");

            /* Do a backup check with old recovery. */
            const uint256_t hashRecoveryDeprecated = pCredentials->RecoveryDeprecated(strRecovery, nKeyType);
            if(hashRecoveryDeprecated == hashRecovery)
                fDeprecatedRecovery = true;
            else
            {
                pCredentials.free();
                throw Exception(-33, "Recovery hash mismatch ", hashRecoveryDeprecated.ToString(), " is not ", hashRecovery.ToString());
            }
        }

        /* Create the transaction. */
        TAO::Ledger::Transaction tx;
        if(!BuildCredentials(pCredentials, strPIN, nKeyType, tx))
        {
            pCredentials.free();
            throw Exception(-17, "Failed to create transaction");
        }

        /* Sign the transaction. */
        tx.nKeyType = nKeyType;
        if(!fDeprecatedRecovery)
        {
            /* Build using current recovery system. */
            if(!tx.Sign(pCredentials->Generate(strRecovery)))
            {
                pCredentials.free();
                throw Exception(-31, "Ledger failed to sign transaction");
            }
        }
        else
        {
            /* Handle if we switched into deprecated recovery mode. */
            if(!tx.Sign(pCredentials->GenerateDeprecated(strRecovery)))
            {
                pCredentials.free();
                throw Exception(-31, "Ledger failed to sign transaction");
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
