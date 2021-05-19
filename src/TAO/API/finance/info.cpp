/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/finance/types/finance.h>

#include <TAO/API/include/format.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/include/stake_change.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/stake_minter.h>

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
            /* The user genesis hash */
            const uint256_t hashGenesis =
                users->GetSession(params).GetAccount()->Genesis();

            /* Retrieve the trust register address, which is based on the users genesis */
            const TAO::Register::Address hashRegister =
                TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

            /* Attempt to read our trust register. */
            TAO::Register::Object trust;
            if(!LLD::Register->ReadObject(hashRegister, trust, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-70, "Trust account not found");

            /* Grab our trust score since we will use in further calculations. */
            const uint64_t nTrustScore = trust.get<uint64_t>("trust");

            /* Set trust account values for return data */
            json::json jRet;
            jRet["address"] = hashRegister.ToString();
            jRet["balance"] = FormatBalance(trust.get<uint64_t>("balance"), 0);
            jRet["stake"]   = FormatBalance(trust.get<uint64_t>("stake"),   0);
            jRet["trust"]   = nTrustScore;

            /* Need the stake minter running for accessing current staking metrics.*/
            TAO::Ledger::StakeMinter& rStakeMinter = TAO::Ledger::StakeMinter::GetInstance();
            if(rStakeMinter.IsStarted())
            {
                /* The trust account is on hold when it does not have genesis, and is waiting to reach minimum age to stake */
                const bool fOnHold = (!LLD::Register->HasTrust(hashGenesis) || rStakeMinter.IsWaitPeriod());

                /* When trust account is on hold pending minimum age return the time remaining in hold period. */
                jRet["onhold"] = fOnHold;
                if(fOnHold)
                    jRet["holdtime"] = (uint64_t)rStakeMinter.GetWaitTime();

                /* Populate our running statistics. */
                jRet["stakerate"]   = rStakeMinter.GetStakeRatePercent();
                jRet["trustweight"] = rStakeMinter.GetTrustWeightPercent();
                jRet["blockweight"] = rStakeMinter.GetBlockWeightPercent();
                jRet["stakeweight"] = rStakeMinter.GetTrustWeight() + rStakeMinter.GetBlockWeight();
                jRet["staking"]     = true;
            }
            else
            {
                /* When stake minter not running, return latest known value calculated from trust score (as an annual percent). */
                jRet["stakerate"] = TAO::Ledger::StakeRate(nTrustScore, (nTrustScore == 0)) * 100.0;

                /* Other staking metrics not available if stake minter not running */
                jRet["trustweight"] = 0.0;
                jRet["blockweight"] = 0.0;
                jRet["stakeweight"] = 0.0;
                jRet["staking"]     = false;
            }

            /* Check if we have any pending stake changes. */
            TAO::Ledger::StakeChange tStakeChange;
            if(LLD::Local->ReadStakeChange(hashGenesis, tStakeChange) && !tStakeChange.fProcessed)
            {
                /* Populate our stake change values. */
                jRet["change"]    = true;
                jRet["amount"]    = FormatBalance(tStakeChange.nAmount, 0);
                jRet["requested"] = tStakeChange.nTime;
                jRet["expires"]   = tStakeChange.nExpires;
            }
            else
                jRet["change"] = false;

            /* Filter based on requested fieldname. */
            FilterResponse(params, jRet);

            return jRet;
        }
    }
}
