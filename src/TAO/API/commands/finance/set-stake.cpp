/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/finance.h>

#include <LLD/include/global.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake_change.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Register/types/object.h>

#include <Util/include/memory.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Set the stake amount for trust account (add/remove stake). */
    encoding::json Finance::SetStake(const encoding::json& jParams, const bool fHelp)
    {
        encoding::json ret;

        /* Get the PIN to be used for this API call */
        SecureString strPin = Commands::Instance<Users>()->GetPin(jParams, TAO::Ledger::PinUnlock::TRANSACTIONS);

        /* Check for pin size. */
        if(strPin.size() == 0)
            throw Exception(-135, "Zero-length PIN");

        /* Get the session to be used for this API call */
        Session& session = Commands::Instance<Users>()->GetSession(jParams);

        /* Get the user account. */
        const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = session.GetAccount();
        if(!user)
            throw Exception(-10, "Invalid session ID");

        /* Authenticate the users credentials */
        if(!Commands::Instance<Users>()->Authenticate(jParams))
            throw Exception(-139, "Invalid credentials");

        /* Get the genesis ID. */
        uint256_t hashGenesis = user->Genesis();

        /* Verify the user login. */
        if(!LLD::Ledger->HasGenesis(hashGenesis))
        {
            /* Check the memory pool for hashGenesis transaction. */
            if(!TAO::Ledger::mempool.Has(hashGenesis))
                throw Exception(-136, "Account doesn't exist");

            /* If present in mempool when HasGenesis() false, it is user create and not yet confirmed. Cannot set stake */
            throw Exception(-222, "User create pending confirmation");
        }

        /* Check for amount parameter. */
        if(jParams.find("amount") == jParams.end())
            throw Exception(-46, "Missing amount");

        else if(std::stod(jParams["amount"].get<std::string>()) < 0)
            throw Exception(-204, "Cannot set stake to a negative amount");

        /* Lock the signature chain. */
        LOCK(session.CREATE_MUTEX);

        /* Check that the sig chain is mature after the last coinbase/coinstake transaction in the chain. */
        CheckMature(hashGenesis);

        /* Get trust account. Any trust account that has completed Genesis will be indexed. */
        TAO::Register::Object trustAccount;

        if(LLD::Register->HasTrust(hashGenesis))
        {
            /* Trust account is indexed */
            if(!LLD::Register->ReadTrust(hashGenesis, trustAccount))
               throw Exception(-75, "Unable to retrieve trust account");

            if(!trustAccount.Parse())
                throw Exception(-71, "Unable to parse trust account");
        }
        else
        {
            throw Exception(-76, "Cannot set stake for trust account until after Genesis transaction");
        }

        uint64_t nBalancePrev = trustAccount.get<uint64_t>("balance");
        uint64_t nStakePrev = trustAccount.get<uint64_t>("stake");

        /* Retrieve last stake tx hash for the user trust account to include in stake change request */
        uint512_t hashLast;
        if(!LLD::Ledger->ReadStake(hashGenesis, hashLast))
            throw Exception(-205, "Unable to retrieve last stake");

        /* Get the requested amount to stake. */
        uint64_t nAmount = std::stod(jParams["amount"].get<std::string>()) * TAO::Ledger::NXS_COIN;

        /* Get the time to expiration (optional). */
        uint64_t nExpires = 0;
        if(jParams.find("expires") != jParams.end())
            nExpires = std::stoull(jParams["expires"].get<std::string>());

        uint64_t nTime = runtime::unifiedtimestamp();

        /* Convert time to expiration to an expiration timestamp (0 = does not expire) */
        if(nExpires > 0)
        {

            if((std::numeric_limits<uint64_t>::max() - nExpires) < nTime) //check for overflow
                nExpires = std::numeric_limits<uint64_t>::max();

            else
                nExpires = nExpires + nTime;
        }

        /* Only need to validate amount for add to stake. An amount between zero and current stake balance
         * is an unstake (remove from stake), and cannot set less than zero so cannot attempt to remove
         * more than current stake amount.
         */
        if(nAmount > nStakePrev)
        {
            /* Validate add to stake */
            if((nAmount - nStakePrev) > nBalancePrev)
                throw Exception(-77, "Insufficient trust account balance to add to stake");
        }

        TAO::Ledger::StakeChange request;
        bool hasPrev = false;

        /* Retrieve any existing stake change request */
        if(LLD::Local->ReadStakeChange(hashGenesis, request))
            hasPrev = true;

        if(hasPrev && request.nExpires != 0 && (request.nExpires < runtime::unifiedtimestamp()))
        {
            request.SetNull();
            hasPrev = false;

            if(!LLD::Local->EraseStakeChange(hashGenesis))
                throw Exception(-206, "Failed to erase expired stake change request");
        }

        /* Set up the stake change request and store for inclusion in next stake block found */
        if(nAmount == nStakePrev)
        {
            if(hasPrev)
            {
                /* Setting amount to the current stake value (no change) removes the prior stake change request */
                if(!LLD::Local->EraseStakeChange(hashGenesis))
                    throw Exception(-207, "Failed to erase previous stake change request");
            }
            else
                throw Exception(-78, "Stake not changed"); //Request to set stake to current value when no prior request
        }

        else
        {
            request.SetNull();

            request.hashGenesis = hashGenesis;
            request.hashLast = hashLast;
            request.nTime = nTime;
            request.nExpires = nExpires;

            /* set/stake amount is new stake value, request stores the amount of change */
            request.nAmount = nAmount - nStakePrev;

            /* Retrieve the private "auth" key for use in signing */
            uint512_t hashSecret = user->Generate("auth", 0, strPin);

            /* The public key for the "auth" key*/
            std::vector<uint8_t> vchPubKey;

            /* The stake change request signature */
            std::vector<uint8_t> vchSig;

            /* The crypto register object */
            TAO::Register::Object crypto;

            /* Get the crypto register. This is needed so that we can determine the key type used to generate the public key */
            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::MEMPOOL))
                throw debug::exception(FUNCTION, "Could not sign - missing crypto register");

            /* Parse the object. */
            if(!crypto.Parse())
                throw debug::exception(FUNCTION, "failed to parse crypto register");

            /* Check that the requested key is in the crypto register */
            if(!crypto.Check("auth"))
                throw debug::exception(FUNCTION, "Key type not found in crypto register: ", "auth");

            /* Get the encryption key type from the hash of the public key */
            uint8_t nType = crypto.get<uint256_t>("auth").GetType();

            /* Generate the public key and signature */
            user->Sign(nType, request.GetHash().GetBytes(), hashSecret, vchPubKey, vchSig);

            request.vchPubKey = vchPubKey;
            request.vchSig = vchSig;

            if(!LLD::Local->WriteStakeChange(hashGenesis, request))
                throw Exception(-208, "Failed to save stake change request");
        }

        /* Build a JSON response object. */
        ret["success"] = true;

        return ret;
    }
}
