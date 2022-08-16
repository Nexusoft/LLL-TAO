/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
        /* Build our proof of stake data. */
        encoding::json jRet =
        {
            { "stake", ChannelToJSON(0) },
            { "prime", ChannelToJSON(1) },
            { "hash",  ChannelToJSON(2) },
        };

        /* Grab our best block. */
        const TAO::Ledger::BlockState tBestBlock =
            TAO::Ledger::ChainState::stateBest.load();

        /* We only need supply data when on a public network or testnet, private and hybrid do not have supply. */
        if(!config::fHybrid.load())
        {
            /* Add supply metrics */
            encoding::json jSupply;

            /* Read the stateBest using hashBestChain */
            TAO::Ledger::BlockState stateBest;
            if(!LLD::Ledger->ReadBlock(TAO::Ledger::ChainState::hashBestChain.load(), stateBest))
                return std::string("Block not found");

            /* Get our chain age. */
            const uint32_t nMinutes =
                TAO::Ledger::GetChainAge(stateBest.GetBlockTime());

            /* Get our total supply and target supply. */
            const int64_t nSupply   = stateBest.nMoneySupply;
            const int64_t nTarget   = TAO::Ledger::CompoundSubsidy(nMinutes);

            /* Calculate the number of years it has been since start of chain. */
            double nYears   = (nMinutes / 525960.0); //525960 is 1440 * 365.25 for minutes in a year
            double nYearly = (nSupply - nTarget) / nYears;

            /* Calculate inflation rate by comparing yearly emmission rates to total supply. */
            const uint64_t nInflation =
                (nYearly * TAO::Ledger::NXS_COIN) / nSupply; //we use NXS_COIN to get 4 significant figures

            /* Add this data to our supply json. */
            jSupply["total"]     = double(nSupply) / TAO::Ledger::NXS_COIN;
            jSupply["target"]    = double(nTarget) / TAO::Ledger::NXS_COIN;
            jSupply["inflation"] = double(nInflation * 100) / TAO::Ledger::NXS_COIN; //100 counts as 2 of 6 figures in NXS_COIN

            /* Apply the mining emmission rates from subsidy calculations. */
            jSupply["minute"] = double(TAO::Ledger::SubsidyInterval(nMinutes, 1)) / TAO::Ledger::NXS_COIN; //1
            jSupply["hour"]   = double(TAO::Ledger::SubsidyInterval(nMinutes, 60)) / TAO::Ledger::NXS_COIN; //60
            jSupply["day"]    = double(TAO::Ledger::SubsidyInterval(nMinutes, 1440)) / TAO::Ledger::NXS_COIN; //1440
            jSupply["week"]   = double(TAO::Ledger::SubsidyInterval(nMinutes, 10080)) / TAO::Ledger::NXS_COIN;//10080
            jSupply["month"]  = double(TAO::Ledger::SubsidyInterval(nMinutes, 40320)) / TAO::Ledger::NXS_COIN; //40320

            /* Add our supply data to ledger/get/info. */
            jRet["supply"]    = jSupply;
        }


        /* Add chain-state data. */
        jRet["height"]     = tBestBlock.nHeight;
        jRet["timestamp"]  = tBestBlock.GetBlockTime();
        jRet["checkpoint"] = tBestBlock.hashCheckpoint.GetHex();

        /* Filter our fieldname. */
        FilterFieldname(jParams, jRet);

        return jRet;
    }
} /* End Ledger Namespace*/
