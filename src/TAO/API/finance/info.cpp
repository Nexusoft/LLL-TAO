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
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/types/tritium_minter.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get staking metrics for a trust account */
        json::json Finance::Info(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the session to be used for this API call */
            uint64_t nSession = users->GetSession(params);

            /* Get the user account. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-25, "Invalid session ID");

            /* Retrieve the trust register address from the trust account name */
            uint256_t hashRegister = RegisterAddressFromName(params, std::string("trust"));

            /* Get trust account. Any trust account that has completed Genesis will be indexed. */
            TAO::Register::Object trustAccount;

            if(!LLD::regDB->ReadState(hashRegister, trustAccount, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-24, "Trust account not found");

            if(!trustAccount.Parse())
                throw APIException(-24, "Unable to parse trust account.");

            /* Check the object standard. */
            if( trustAccount.Standard() != TAO::Register::OBJECTS::TRUST)
                throw APIException(-24, "Register is not a trust account");

            /* Check the account is a NXS account */
            if(trustAccount.get<uint256_t>("token") != 0)
                throw APIException(-24, "Trust account is not a NXS account.");

            /* Set trust account values for return data */
            ret["address"] = hashRegister.ToString();

            ret["balance"] = (double)trustAccount.get<uint64_t>("balance") / TAO::Ledger::NXS_COIN;

            ret["stake"] = (double)trustAccount.get<uint64_t>("stake") / TAO::Ledger::NXS_COIN;

            /* Trust is returned as a percentage of maximum */
            uint64_t nTrustScore = trustAccount.get<uint64_t>("trust");
            if(nTrustScore > 0)
                ret["trust"] = ((double)nTrustScore * 100.0) / (double)TAO::Ledger::MaxTrustScore();
            else
                ret["trust"] = 0.0;

            TAO::Ledger::TritiumMinter& stakeMinter = TAO::Ledger::TritiumMinter::GetInstance();

            /* Need the stake minter running for accessing current staking metrics.
             * Verifying current user ownership of trust account is a sanity check.
             */
            if (stakeMinter.IsStarted() && trustAccount.hashOwner == user->Genesis())
            {
                /* If stake minter is running, get current stake rate it is using. */
                ret["stakerate"] = stakeMinter.GetStakeRatePercent();

                /* Other staking metrics also are available with running minter */
                ret["trustweight"] = stakeMinter.GetTrustWeightPercent();
                ret["blockweight"] = stakeMinter.GetBlockWeightPercent();

                /* Raw trust weight and block weight total to 100, so can use the total as a % directly */
                ret["stakeweight"] = stakeMinter.GetTrustWeight() + stakeMinter.GetBlockWeight();
            }
            else
            {
                /* When stake minter not running, return latest known value calculated from trust score (as an annual percent). */
                ret["stakerate"] = TAO::Ledger::StakeRate(nTrustScore, (nTrustScore == 0)) * 100;

                /* Other staking metrics not available if stake minter not running */
                ret["trustweight"] = 0.0;
                ret["blockweight"] = 0.0;
                ret["stakeweight"] = 0.0;
            }

            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            if(params.find("fieldname") != params.end())
            {
                /* First get the fieldname from the response */
                std::string strFieldname =  params["fieldname"].get<std::string>();

                /* Iterate through the response keys */
                for (auto it = ret.begin(); it != ret.end(); ++it)
                    /* If this key is not the one that was requested then erase it */
                    if( it.key() != strFieldname)
                        ret.erase(it);
            }

            return ret;
        }
    }
}
