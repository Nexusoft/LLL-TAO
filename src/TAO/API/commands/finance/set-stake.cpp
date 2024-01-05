/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/finance.h>

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake_change.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/credentials.h>

#include <TAO/Register/types/object.h>

#include <Util/include/memory.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Set the stake amount for trust account (add/remove stake). */
    encoding::json Finance::SetStake(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the calling genesis-id. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Verify the user login. */
        if(!LLD::Ledger->HasFirst(hashGenesis))
        {
            /* Check the memory pool for hashGenesis transaction. */
            if(!TAO::Ledger::mempool.Has(hashGenesis))
                throw Exception(-136, "Account doesn't exist");

            /* If present in mempool when HasFirst() false, it is user create and not yet confirmed. Cannot set stake */
            throw Exception(-222, "User create pending confirmation");
        }

        /* Pin parameter. */
        const SecureString strPIN =
            ExtractPIN(jParams);

        /* Get a copy of our linked credentials. */
        const auto& pCredentials =
            Authentication::Credentials(jParams);

        /* Check pin length */
        if(strPIN.length() < 4)
            throw Exception(-193, "Pin must be a minimum of 4 characters");

        /* Get trust account. Any trust account that has completed Genesis will be indexed. */
        TAO::Register::Object oTrust;
        if(!LLD::Register->HasTrust(hashGenesis))
            throw Exception(-76, "Cannot set stake for trust account until after Genesis transaction");

        /* Trust account is indexed */
        if(!LLD::Register->ReadTrust(hashGenesis, oTrust))
           throw Exception(-75, "Unable to retrieve trust account");

          /* Parse our trust account. */
        if(!oTrust.Parse())
            throw Exception(-71, "Unable to parse trust account");

        /* Get our current balances and stake. */
        const uint64_t nBalancePrev = oTrust.get<uint64_t>("balance");
        const uint64_t nStakePrev   = oTrust.get<uint64_t>("stake");

        /* Retrieve last stake tx hash for the user trust account to include in stake change request */
        uint512_t hashLast;
        if(!LLD::Ledger->ReadStake(hashGenesis, hashLast))
            throw Exception(-205, "Unable to retrieve last stake");

        /* Get the requested amount to stake. */
        const uint64_t nAmount =
            ExtractAmount(jParams, TAO::Ledger::NXS_COIN, "amount");

        /* Get the current timestamp. */
        const uint64_t nTimestamp =
            runtime::unifiedtimestamp();

        /* Get our expiration if parameter supplied. */
        const uint64_t nExpires =
            ExtractInteger<uint64_t>(jParams, "expires", nTimestamp + (3 * 86400));

        /* Check our ranges compared to stake. */
        if(nAmount > nStakePrev && (nAmount - nStakePrev) > nBalancePrev)
            throw Exception(-77, "Insufficient trust account balance to add to stake");

        /* Retrieve any existing stake change request */
        TAO::Ledger::StakeChange tChangeRequest;
        bool fExists =
            LLD::Local->ReadStakeChange(hashGenesis, tChangeRequest);

        /* Check if existing stake request needs to be erased. */
        if(fExists && tChangeRequest.nExpires != 0 && (tChangeRequest.nExpires < runtime::unifiedtimestamp()))
        {
            /* Erase our current stake change request. */
            if(!LLD::Local->EraseStakeChange(hashGenesis))
                throw Exception(-206, "Failed to erase expired stake change request");

            fExists = false;
        }

        /* Set up the stake change request and store for inclusion in next stake block found */
        if(nAmount == nStakePrev)
        {
            /* Erase if setting it back to zero. */
            if(fExists)
            {
                /* Setting amount to the current stake value (no change) removes the prior stake change request */
                if(!LLD::Local->EraseStakeChange(hashGenesis))
                    throw Exception(-207, "Failed to erase previous stake change request");
            }

            throw Exception(-78, "Stake not changed"); //Request to set stake to current value when no prior request
        }

        /* Populate our stake change data. */
        tChangeRequest.hashGenesis = hashGenesis;
        tChangeRequest.hashLast    = hashLast;
        tChangeRequest.nTime       = nTimestamp;
        tChangeRequest.nExpires    = nExpires;

        /* Set amount is new stake value, request stores the amount of change */
        tChangeRequest.nAmount = (nAmount - nStakePrev);

        /* Get our crypto object register address. */
        const uint256_t hashCrypto =
            TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

        /* Get the crypto register so we can determine the key type used to generate the public key */
        TAO::Register::Object oCrypto;
        if(!LLD::Register->ReadObject(hashCrypto, oCrypto, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-130, "missing crypto register");

        /* Read the key type from crypto object register. */
        const uint256_t hashAuth =
            oCrypto.get<uint256_t>("auth");

        /* Check if the auth has is deactivated. */
        if(hashAuth == 0)
            throw Exception(-130, "Auth hash deactivated, please call crypto/create/auth");

        /* Get the encryption key type from the hash of the public key */
        const uint8_t nKeyType =
            hashAuth.GetType();

        /* Generate a key to check credentials against. */
        const uint256_t hashCheck =
            pCredentials->KeyHash("auth", 0, strPIN, nKeyType);

        /* Check for invalid authorization hash. */
        if(hashAuth != hashCheck)
            throw Exception(-139, "Invalid credentials");

        /* Retrieve the private "auth" key for use in signing */
        const uint512_t hashSecret =
            pCredentials->Generate("auth", 0, strPIN);

        /* Generate the public key and signature */
        pCredentials->Sign(nKeyType, tChangeRequest.GetHash().GetBytes(), hashSecret, tChangeRequest.vchPubKey, tChangeRequest.vchSig);

        /* Finally write our stake change to our local database. */
        if(!LLD::Local->WriteStakeChange(hashGenesis, tChangeRequest))
            throw Exception(-208, "Failed to save stake change request");

        /* Build a JSON response object. */
        const encoding::json jRet =
        {
            { "success", true }
        };

        return jRet;
    }
}
