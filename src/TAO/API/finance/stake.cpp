/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/finance.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Set the stake amount for trust account (add/remove stake). */
        json::json Finance::Stake(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params);

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Get the user account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Check for amount parameter. */
            if(params.find("amount") == params.end())
                throw APIException(-25, "Missing Amount. (<amount>)");

            /* Check that the account is unlocked for creating transactions */
            if(!users->CanTransact())
                throw APIException(-25, "Account has not been unlocked for transactions.");

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                throw APIException(-25, "Failed to create transaction.");

            /* Register address is a hash of a name in the format of namespacehash:objecttype:name */
            std::string strRegisterName = TAO::Register::NamespaceHash(user->UserName().c_str()).ToString() + ":token:trust";

            /* Build the address from an SK256 hash of register name. Need for indexed trust accounts, also, as return value */
            uint256_t hashRegister = LLC::SK256(std::vector<uint8_t>(strRegisterName.begin(), strRegisterName.end()));

            /* Get trust account. Any trust account that has completed Genesis will be indexed. */
            TAO::Register::Object trustAccount;

            if(LLD::Register->HasTrust(user->Genesis()))
            {
                /* Trust account is indexed */
                if(!LLD::Register->ReadTrust(user->Genesis(), trustAccount))
                   throw APIException(-24, "Unable to retrieve trust account.");

                if(!trustAccount.Parse())
                    throw APIException(-24, "Unable to parse trust account register.");
            }
            else
            {
                /* TODO - Add "pending stake" to trust account and allow set/stake for pre-Genesis */
                throw APIException(-25, "Cannot set stake for trust account until after Genesis transaction");

                if(!LLD::Register->ReadState(hashRegister, trustAccount, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-24, "Trust account not found");

                if(!trustAccount.Parse())
                    throw APIException(-24, "Unable to parse trust account.");

                /* Check the object standard. */
                if(trustAccount.Standard() != TAO::Register::OBJECTS::TRUST)
                    throw APIException(-24, "Register is not a trust account");

                /* Check the account is a NXS account */
                if(trustAccount.get<uint256_t>("token") != 0)
                    throw APIException(-24, "Trust account is not a NXS account.");
            }

            uint64_t nBalancePrev = trustAccount.get<uint64_t>("balance");
            uint64_t nStakePrev = trustAccount.get<uint64_t>("stake");

            /* Get the amount to debit. */
            uint64_t nAmount = std::stod(params["amount"].get<std::string>()) * TAO::Ledger::NXS_COIN;
            if(nAmount > nStakePrev)
            {
                /* Adding to stake from balance */
                if((nAmount - nStakePrev) > nBalancePrev)
                    throw APIException(-25, "Insufficient trust account balance to add to stake");

                /* Set the transaction payload for stake operation */
                uint64_t nAddStake = nAmount - nStakePrev;

                tx[0] << uint8_t(TAO::Operation::OP::STAKE) << nAddStake;
            }
            else if(nAmount < nStakePrev)
            {
                /* Removing from stake to balance */
                uint64_t nRemoveStake = nStakePrev - nAmount;

                uint64_t nTrustPrev = trustAccount.get<uint64_t>("trust");
                uint64_t nTrustPenalty = TAO::Ledger::GetUnstakePenalty(nTrustPrev, nStakePrev, nAmount);

                tx[0] << uint8_t(TAO::Operation::OP::UNSTAKE) << nRemoveStake << nTrustPenalty;
            }

            /* Execute the operations layer. */
            if(!tx.Build())
                throw APIException(-26, "Transaction failed to build");

            /* Sign the transaction. */
            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                throw APIException(-26, "Ledger failed to sign transaction");

            /* Execute the operations layer. */
            if(!TAO::Ledger::mempool.Accept(tx))
                throw APIException(-26, "Failed to accept");

            /* Build a JSON response object. */
            ret["txid"] = tx.GetHash().ToString();

            return ret;
        }
    }
}
