/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <LLD/include/global.h>

#include <TAO/API/users/types/users.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/extract.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Operation/include/enum.h>

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

        /* Get our genesis-id. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Check that the genesis exists. */
        TAO::Ledger::Transaction txPrev;
        if(!LLD::Ledger->HasFirst(hashGenesis))
        {
            throw Exception(-139, "Invalid credentials");
        }
        else
        {
            /* Get the last transaction. */
            uint512_t hashLast;
            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-138, "No previous transaction found");

            /* Get previous transaction */
            if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-138, "No previous transaction found");
        }

        /* Check that the recovery hash has been set on this signature chain */
        if(txPrev.hashRecovery == 0)
            throw Exception(-223, "Recovery seed not set on this signature chain");

        /* When signing with the recovery seed we need to set the transaction key type to be the same type that was used
            to generate the current recovery hash.  To obtain this we iterate back through the sig chain until we find where
            the hashRecovery was first set, and use the nKeyType from that transaction */

        /* The key type to set */
        uint8_t nKeyType = 0;

        /* The recovery hash to search for */
        uint256_t hashRecovery = txPrev.hashRecovery;

        /* Iterate backwards until we reach the transaction where the hashRecovery differs */
        while(txPrev.hashRecovery == hashRecovery)
        {
            nKeyType = txPrev.nKeyType;

            if(!LLD::Ledger->ReadTx(txPrev.hashPrevTx, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-138, "No previous transaction found");
        }

        /* Create sig chain based on the new credentials */
        memory::encrypted_ptr<TAO::Ledger::SignatureChain> user = new TAO::Ledger::SignatureChain(strUsername, strPassword);

        /* Validate the recovery seed is correct.  We do this at this stage so that we can return a helpful error response
           from the API call, rather than waiting for mempool::Accept to fail  */
        if(user->RecoveryHash(strRecovery, nKeyType ) != hashRecovery)
            throw Exception(-139, "Invalid credentials");

        /* Create the update transaction */
        TAO::Ledger::Transaction tx;
        if(!Users::CreateTransaction(user, strPin, tx))
        {
            user.free();
            throw Exception(-17, "Failed to create transaction");
        }

        /* Now set the new credentials */
        tx.NextHash(user->Generate(tx.nSequence + 1, strPassword, strPin));

        /* Update the Crypto keys with the new pin */
        update_crypto_keys(user, strPin, tx);

        /* Calculate the prestates and poststates. */
        if(!tx.Build())
        {
            user.free();
            throw Exception(-30, "Operations failed to execute");
        }

        /* Set the key type */
        tx.nKeyType = nKeyType;

        /* Sign the transaction with private key generated from the recovery seed. */
        if(!tx.Sign(user->Generate(strRecovery)))
        {
            user.free();
            throw Exception(-31, "Ledger failed to sign transaction");
        }

        /* Execute the operations layer. */
        if(!TAO::Ledger::mempool.Accept(tx))
        {
            user.free();
            throw Exception(-32, "Failed to accept");
        }

        /* Free the sigchain. */
        user.free();

        /* Build a JSON response object. */
        jsonRet["genesis"]   = tx.hashGenesis.ToString();
        jsonRet["nexthash"]  = tx.hashNext.ToString();
        jsonRet["prevhash"]  = tx.hashPrevTx.ToString();
        jsonRet["pubkey"]    = HexStr(tx.vchPubKey.begin(), tx.vchPubKey.end());
        jsonRet["signature"] = HexStr(tx.vchSig.begin(),    tx.vchSig.end());
        jsonRet["txid"]      = tx.GetHash().ToString();

        return jsonRet;
    }
}
