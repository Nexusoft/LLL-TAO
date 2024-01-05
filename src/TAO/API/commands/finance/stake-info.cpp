/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/types/commands/finance.h>

#include <TAO/API/include/filter.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/finance.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/include/stake_change.h>

#include <TAO/Ledger/types/credentials.h>
#include <TAO/Ledger/types/stake_minter.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get staking metrics for a trust account */
    encoding::json Finance::GetStakeInfo(const encoding::json& jParams, const bool fHelp)
    {
        /* The user genesis hash */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Retrieve the trust register address, which is based on the users genesis */
        const TAO::Register::Address hashRegister =
            TAO::Register::Address(std::string("trust"), hashGenesis, TAO::Register::Address::TRUST);

        /* Attempt to read our trust register. */
        TAO::Register::Object objTrust;
        if(!LLD::Register->ReadObject(hashRegister, objTrust, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-70, "Trust account not found");

        /* Grab our trust score since we will use in further calculations. */
        const uint64_t nTrustScore = objTrust.get<uint64_t>("trust");

        /* Set trust account values for return data */
        encoding::json jRet;
        jRet["address"] = hashRegister.ToString();
        jRet["balance"] = FormatBalance(objTrust.get<uint64_t>("balance"));
        jRet["stake"]   = FormatBalance(objTrust.get<uint64_t>("stake"));
        jRet["trust"]   = nTrustScore;

        /* Indexed trust account has genesis */
        const bool fTrustIndexed =
            LLD::Register->HasTrust(hashGenesis);

        /* Need the stake minter running for accessing current staking metrics.*/
        const TAO::Ledger::StakeMinter& rStakeMinter = TAO::Ledger::StakeMinter::GetInstance();
        if(rStakeMinter.IsStarted())
        {
            /* The trust account is on hold when it does not have genesis, and is waiting to reach minimum age to stake */
            const bool fOnHold = (!fTrustIndexed || rStakeMinter.IsWaitPeriod());

            /* When trust account is on hold pending minimum age return the time remaining in hold period. */
            jRet["onhold"] = fOnHold;
            if(fOnHold)
                jRet["holdtime"] = (uint64_t)rStakeMinter.GetWaitTime();

            /* Populate our running statistics. */
            jRet["stakerate"]   = rStakeMinter.GetStakeRatePercent();
            jRet["trustweight"] = rStakeMinter.GetTrustWeightPercent();
            jRet["blockweight"] = rStakeMinter.GetBlockWeightPercent();
            jRet["stakeweight"] = rStakeMinter.GetTrustWeight() + rStakeMinter.GetBlockWeight();

            /* Flag to tell if staking genesis. */
            jRet["new"] = (bool)(!fTrustIndexed);
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

            /* Flag to tell if staking genesis. */
            jRet["new"] = (bool)(!fTrustIndexed);
            jRet["staking"]     = false;
        }

        /* Check if we have any pending stake changes. */
        TAO::Ledger::StakeChange tStakeChange;
        if(LLD::Local->ReadStakeChange(hashGenesis, tStakeChange) && !tStakeChange.fProcessed)
        {
            /* Populate our stake change values. */
            jRet["change"]    = true;
            jRet["amount"]    = FormatStake(tStakeChange.nAmount);
            jRet["requested"] = tStakeChange.nTime;
            jRet["expires"]   = tStakeChange.nExpires;
        }
        else
            jRet["change"] = false;

        /* Filter based on requested fieldname. */
        FilterFieldname(jParams, jRet);

        return jRet;
    }
}
