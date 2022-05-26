/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/profiles.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Recover a sig chain and set new credentials by supplying the recovery seed */
    encoding::json Profiles::Update(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our genesis-id for local checks. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Check for duplicates in ledger db. */
        uint512_t hashLast;
        if(!LLD::Logical->ReadFirst(hashGenesis, hashLast) || !LLD::Logical->ReadLast(hashGenesis, hashLast))
            throw Exception(-130, "Account doesn't exist or has never logged in. Try profiles/recover instead");

        /* The new key scheme */
        const uint8_t nScheme =
            ExtractScheme(jParams, "brainpool, falcon");

        /* Get previous transaction */
        TAO::Ledger::Transaction txPrev;
        if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-138, "No previous transaction found");

        /* Get our type we are processing. */
        const std::string strType = ExtractType(jParams);

        /* Check for recovery update. */
        if(strType == "recovery")
        {
            /* Handle generating recovery level data. */
            if(CheckParameter(jParams, "new_recovery", "string"))
            {
                /* Create a temp sig chain for checking credentials */
                const auto& pCredentials =
                    Authentication::Credentials(jParams);

                /* The PIN to be used for this API call */
                SecureString strPIN;

                /* Unlock grabbing the pin, while holding a new authentication lock */
                RECURSIVE(Authentication::Unlock(jParams, strPIN, TAO::Ledger::PinUnlock::TRANSACTIONS));

                /* Create the transaction. */
                TAO::Ledger::Transaction tx;
                if(!BuildCredentials(pCredentials, strPIN, nScheme, tx))
                    throw Exception(-17, "Failed to create transaction");

                /* Handle if recovery phrase already exists. */
                uint8_t nRecoveryType = txPrev.nKeyType;
                if(txPrev.hashRecovery != 0)
                {
                    /* Check for existing recovery phrase now. */
                    if(!CheckParameter(jParams, "recovery", "string"))
                        throw Exception(-233, "Missing parameter [recovery], expecting [exists]");

                    /* Parse out username. */
                    const SecureString strRecovery =
                        SecureString(jParams["recovery"].get<std::string>().c_str());

                    /* The recovery hash to search for */
                    const uint256_t hashRecovery =
                        txPrev.hashRecovery;

                    /* Iterate backwards until we reach the transaction where the hashRecovery differs */
                    while(txPrev.hashRecovery == hashRecovery)
                    {
                        /* Set our key type from iterated transaction. */
                        nRecoveryType = txPrev.nKeyType;

                        /* If we fail to read, we have reached the end of sigchain. */
                        if(!LLD::Ledger->ReadTx(txPrev.hashPrevTx, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                            throw Exception(-138, "No previous transaction found");
                    }

                    /* Set our current key type from recovery. */
                    tx.nKeyType = nRecoveryType;

                    /* Sign the transaction. */
                    if(!tx.Sign(pCredentials->Generate(strRecovery)))
                        throw Exception(-31, "Ledger failed to sign transaction");
                }
                else
                {
                    /* Sign the transaction. */
                    if(!tx.Sign(pCredentials->Generate(tx.nSequence, strPIN)))
                        throw Exception(-31, "Ledger failed to sign transaction");
                }

                /* Parse out new recovery phrase. */
                const SecureString strRecoveryNew =
                    SecureString(jParams["new_recovery"].get<std::string>().c_str());

                /* Set our recovery phrase now with new credentials. */
                tx.hashRecovery = pCredentials->RecoveryHash(strRecoveryNew, nRecoveryType);

                /* Execute the operations layer. */
                if(!TAO::Ledger::mempool.Accept(tx))
                    throw Exception(-32, "Failed to accept");
            }
            else
                throw Exception(-28, "Missing parameter [new_recovery] for command");
        }

        /* Check for credentials update. */
        else if(strType == "credentials")
        {
            /* Get our old set of credentials to build transaction with. */
            const auto& pCredentialsOld =
                Authentication::Credentials(jParams);

            /* The PIN to be used for this API call */
            SecureString strPIN;

            /* Unlock grabbing the pin, while holding a new authentication lock */
            RECURSIVE(Authentication::Unlock(jParams, strPIN, TAO::Ledger::PinUnlock::TRANSACTIONS));

            /* Handle if changing password with no recovery. */
            if(CheckParameter(jParams, "new_password", "string"))
            {
                /* Parse out username. */
                const SecureString strPassword =
                    SecureString(jParams["new_password"].get<std::string>().c_str());

                /* Update the password in our authenticated session now. */
                Authentication::Update(jParams, strPassword);
            }

            /* The new PIN to be used for this API call */
            SecureString strNewPIN = strPIN;

            /* Handle if changing password with no recovery. */
            if(CheckParameter(jParams, "new_pin", "string"))
            {
                /* Parse out username. */
                strNewPIN =
                    SecureString(jParams["new_pin"].get<std::string>().c_str());

                /* Update the PIN in our authenticated session now if we are unlocked. */
                uint8_t nUnlockedActions = TAO::Ledger::PinUnlock::NONE;
                if(Authentication::Unlocked(jParams, nUnlockedActions))
                    Authentication::Update(jParams, nUnlockedActions, strNewPIN);
            }

            /* Get our new set of credentials to build transaction with. */
            const auto& pCredentials =
                Authentication::Credentials(jParams);

            /* Check if we haven't updated our credentials. */
            if(*pCredentialsOld != *pCredentials)
                throw Exception(-233, "Missing parameter [new_password or new_pin]");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!BuildCredentials(pCredentials, strNewPIN, nScheme, tx))
                throw Exception(-17, "Failed to create transaction");

            /* Sign the transaction. */
            if(!tx.Sign(pCredentialsOld->Generate(tx.nSequence, strPIN)))
                throw Exception(-31, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw Exception(-32, "Failed to accept");

            /* Build a JSON response object. */
            encoding::json jRet;
            jRet["success"] = true; //just a little response for if using -autotx
            jRet["txid"] = tx.GetHash().ToString();

            return jRet;
        }

        /* If we didn't find a supported type, throw exception. */
        throw Exception(-36, "Unsupported type [", strType, "] for command");
    }
}
