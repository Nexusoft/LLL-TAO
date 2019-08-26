/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/types/finance.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

#include <TAO/Ledger/types/sigchain.h>
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
                throw APIException(-10, "Invalid session ID");

            /* Retrieve the trust register address from the trust account name */
            TAO::Register::Address hashRegister = Names::ResolveAddress(params, std::string("trust"));

            /* Get trust account. Any trust account that has completed Genesis will be indexed. */
            TAO::Register::Object trust;

            /* Check for trust index. */
            if(!LLD::Register->ReadTrust(user->Genesis(), trust)
            && !LLD::Register->ReadState(hashRegister, trust, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-70, "Trust account not found");

            /* Parse the object. */
            if(!trust.Parse())
                throw APIException(-71, "Unable to parse trust account.");

            /* Check the object standard. */
            if(trust.Standard() != TAO::Register::OBJECTS::TRUST)
                throw APIException(-72, "Register is not a trust account");

            /* Check the account is a NXS account */
            if(trust.get<uint256_t>("token") != 0)
                throw APIException(-73, "Trust account is not a NXS account.");

            /* Set trust account values for return data */
            ret["address"] = hashRegister.ToString();

            ret["balance"] = (double)trust.get<uint64_t>("balance") / TAO::Ledger::NXS_COIN;

            ret["stake"] = (double)trust.get<uint64_t>("stake") / TAO::Ledger::NXS_COIN;

            /* Trust is returned as a percentage of maximum */
            uint64_t nTrustScore = trust.get<uint64_t>("trust");

            ret["trust"] = nTrustScore;

            TAO::Ledger::TritiumMinter& stakeMinter = TAO::Ledger::TritiumMinter::GetInstance();

            /* Return whether stake minter is started and actively running. */
            ret["staking"] = (bool)(stakeMinter.IsStarted() && trust.hashOwner == user->Genesis());

            /* Need the stake minter running for accessing current staking metrics.
             * Verifying current user ownership of trust account is a sanity check.
             */
            if(stakeMinter.IsStarted() && trust.hashOwner == user->Genesis())
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
                ret["stakerate"] = TAO::Ledger::StakeRate(nTrustScore, (nTrustScore == 0)) * 100.0;

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
                json::json temp = ret; //iterate on copy or erase will invalidate the iterator

                for(auto it = temp.begin(); it != temp.end(); ++it)
                    /* If this key is not the one that was requested then erase it */
                    if(it.key() != strFieldname)
                        ret.erase(it.key());
            }

            return ret;
        }
    }
}
