/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/ledger.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/supply.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns an object containing mining-related information. */
    encoding::json Ledger::GetInfo(const encoding::json& jParams, const bool fHelp)
    {
        /* Update cache on best block. */
        encoding::json jRet;

        /* Populate the main block production channels. */
        jRet["stake"]  = ChannelToJSON(0);
        jRet["prime"] = ChannelToJSON(1);
        jRet[ "hash"]  = ChannelToJSON(2);

        /* Grab our best block. */
        const TAO::Ledger::BlockState tBestBlock =
            TAO::Ledger::ChainState::tStateBest.load();

        /* We only need supply data when on a public network or testnet, private and hybrid do not have supply. */
        if(!config::fHybrid.load())
        {
            /* We need to gate keep our emmission rates cache. */
            static std::mutex MUTEX;

            /* Add supply metrics */
            encoding::json jSupply;

            /* Read the tStateBest using hashBestChain */
            TAO::Ledger::BlockState tStateBest;
            if(!LLD::Ledger->ReadBlock(TAO::Ledger::ChainState::hashBestChain.load(), tStateBest))
                return std::string("Block not found");

            /* Get our chain age. */
            const uint32_t nMinutes =
                TAO::Ledger::GetChainAge(tStateBest.GetBlockTime());

            /* Get our total supply and target supply. */
            const int64_t nSupply   = tStateBest.nMoneySupply;
            const int64_t nTarget   = TAO::Ledger::CompoundSubsidy(nMinutes);

            /* Calculate the number of years it has been since start of chain. */
            double nYears   = (nMinutes / 525960.0); //525960 is 1440 * 365.25 for minutes in a year
            double nYearly = (nSupply - nTarget) / nYears;

            /* Calculate inflation rate by comparing yearly emmission rates to total supply. */
            const uint64_t nInflation =
                (nYearly * TAO::Ledger::NXS_COIN) / nSupply; //we use NXS_COIN to get 4 significant figures

            /* Add this data to our supply json. */
            jSupply["total"]     = double(nSupply) / TAO::Ledger::NXS_COIN;
            jSupply["burned"]    = double(tStateBest.nFeesBurned) / TAO::Ledger::NXS_COIN;
            jSupply["target"]    = double(nTarget) / TAO::Ledger::NXS_COIN;
            jSupply["inflation"] = double(nInflation * 100) / TAO::Ledger::NXS_COIN; //100 counts as 2 of 6 figures in NXS_COIN

            {
                LOCK(MUTEX);

                /* Apply the mining emmission rates from subsidy calculations. */
                static std::array<uint64_t, 5> aCache = {TAO::Ledger::SubsidyInterval(nMinutes, 1)
                                                        ,TAO::Ledger::SubsidyInterval(nMinutes, 60)
                                                        ,TAO::Ledger::SubsidyInterval(nMinutes, 1440)
                                                        ,TAO::Ledger::SubsidyInterval(nMinutes, 10080)
                                                        ,TAO::Ledger::SubsidyInterval(nMinutes, 40320)};

                /* We use this value to trigger recalculating. */
                static uint32_t nMinutesCache = nMinutes;

                /* We do some quick math to calculate subsidy difference. */
                if(nMinutesCache < nMinutes)
                {
                    /* Get the difference. */
                    const uint32_t nElapsed =
                        (nMinutes - nMinutesCache);

                    /* We then add the new values on. */
                    aCache[0] = TAO::Ledger::SubsidyInterval(nMinutes, 1); //we can ignore this one

                    /* If we have elapsed interval, re-calculate the cache. */
                    if(nElapsed > 60)
                        aCache[1] = TAO::Ledger::SubsidyInterval(nMinutes, 60);
                    else
                    {
                        /* First we reduce by the previous time elapsed, then add the difference. */
                        aCache[1] -= TAO::Ledger::SubsidyInterval(nMinutesCache, nElapsed);
                        aCache[1] += TAO::Ledger::SubsidyInterval(nMinutesCache + 60, nElapsed);
                    }

                    /* If we have elapsed interval, re-calculate the cache. */
                    if(nElapsed > 1440)
                        aCache[2] = TAO::Ledger::SubsidyInterval(nMinutes, 1440);
                    else
                    {
                        /* First we reduce by the previous time elapsed, then add the difference. */
                        aCache[2] -= TAO::Ledger::SubsidyInterval(nMinutesCache, nElapsed);
                        aCache[2] += TAO::Ledger::SubsidyInterval(nMinutesCache + 1440, nElapsed);
                    }

                    /* If we have elapsed interval, re-calculate the cache. */
                    if(nElapsed > 10080)
                        aCache[3] = TAO::Ledger::SubsidyInterval(nMinutes, 10080);
                    else
                    {
                        /* First we reduce by the previous time elapsed, then add the difference. */
                        aCache[3] -= TAO::Ledger::SubsidyInterval(nMinutesCache, nElapsed);
                        aCache[3] += TAO::Ledger::SubsidyInterval(nMinutesCache + 10080, nElapsed);
                    }

                    /* If we have elapsed interval, re-calculate the cache. */
                    if(nElapsed > 40320)
                        aCache[4] = TAO::Ledger::SubsidyInterval(nMinutes, 40320);
                    else
                    {
                        /* First we reduce by the previous time elapsed, then add the difference. */
                        aCache[4] -= TAO::Ledger::SubsidyInterval(nMinutesCache, nElapsed);
                        aCache[4] += TAO::Ledger::SubsidyInterval(nMinutesCache + 40320, nElapsed);
                    }

                    /* Set our minutes cache now. */
                    nMinutesCache = nMinutes;
                }

                /* Now we populate our values. */
                jSupply["minute"] = double(aCache[0]) / TAO::Ledger::NXS_COIN; //1
                jSupply["hour"]   = double(aCache[1]) / TAO::Ledger::NXS_COIN; //60
                jSupply["day"]    = double(aCache[2]) / TAO::Ledger::NXS_COIN; //1440
                jSupply["week"]   = double(aCache[3]) / TAO::Ledger::NXS_COIN;//10080
                jSupply["month"]  = double(aCache[4]) / TAO::Ledger::NXS_COIN; //40320
            }

            /* Add our supply data to ledger/get/info. */
            jRet["supply"]    = jSupply;
        }


        /* Add chain-state data. */
        jRet["height"]     = tBestBlock.nHeight;
        jRet["timestamp"]  = tBestBlock.GetBlockTime();
        jRet["checkpoint"] = tBestBlock.hashCheckpoint.GetHex();

        return jRet;
    }
} /* End Ledger Namespace*/
