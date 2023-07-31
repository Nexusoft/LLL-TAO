/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/include/json.h>
#include <Legacy/rpc/types/rpc.h>

#include <Legacy/include/money.h>
#include <Legacy/types/address.h>
#include <Legacy/types/trustkey.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>
#include <Util/include/softfloat.h>

#include <LLD/include/global.h>


/* Global TAO namespace. */
namespace Legacy
{
    /* Get network hashrate for the hashing channel */
    encoding::json RPC::GetNetworkHashps(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 0)
            return std::string(
                "getnetworkhashps - Get network hashrate for the hashing channel.");

        /* This constant was determined by finding the time it takes to find hash of given difficulty at a given hash rate.
         * It is the total hashes per second required to find a hash of difficulty 1.0 every second.
         * This can then be used in calculating the network hash rate by block times over average of 1440 blocks.
         * */
        uint64_t nHashRate = 0;
        int nHTotal = 0;
        unsigned int nHashAverageTime = 0;
        double nHashAverageDifficulty = 0.0;
        if(TAO::Ledger::ChainState::nBestHeight.load() > 0
        && TAO::Ledger::ChainState::tStateBest != TAO::Ledger::ChainState::tStateGenesis)
        {
            uint64_t nTimeConstant = 276758250000;

            TAO::Ledger::BlockState blockState = TAO::Ledger::ChainState::tStateBest.load();

            bool bLastStateFound = TAO::Ledger::GetLastState(blockState, 2);
            for(; (nHTotal < 1440 && bLastStateFound); ++nHTotal)
            {
                uint64_t nLastBlockTime = blockState.GetBlockTime();
                blockState = blockState.Prev();
                bLastStateFound = TAO::Ledger::GetLastState(blockState, 2);

                nHashAverageTime += (nLastBlockTime - blockState.GetBlockTime());
                nHashAverageDifficulty += (TAO::Ledger::GetDifficulty(blockState.nBits, 2));
            }
            // protect against getmininginfo being called before hash channel start block
            if(nHTotal > 0)
            {
                nHashAverageDifficulty /= nHTotal;
                nHashAverageTime /= nHTotal;

                /* Calculate the hashrate based on nTimeConstant. */
                nHashRate = (uint64_t)((nTimeConstant / nHashAverageTime) * nHashAverageDifficulty);
            }
        }

        encoding::json obj;
        obj["averagetime"] =  (int)nHashAverageTime;
        obj["averagedifficulty"] = nHashAverageDifficulty;
        obj["hashrate"] = nHashRate;

        return obj;
    }


    /* Get network prime searched per second */
    encoding::json RPC::GetNetworkPps(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 0)
            return std::string(
                "getnetworkpps"
                " - Get network prime searched per second.");

        /* This constant was determined by finding the time it takes to find primes at given difficulty and Primes Per Second'
         * This constant was found that 2,450 Prime Per Second is required to find 1 3ch Cluster per Second.
         * The difficulty changes are exponential or in other words require 50x more work per difficulty increase
         */
        uint64_t nPrimePS = 0;
        double nPrimeAverageDifficulty = 0.0;
        uint32_t nPrimeAverageTime = 0;
        if(TAO::Ledger::ChainState::nBestHeight.load() > 0 && TAO::Ledger::ChainState::tStateBest.load() != TAO::Ledger::ChainState::tStateGenesis)
        {

            uint32_t nPrimeTimeConstant = 2480;
            uint32_t nTotal = 0;
            TAO::Ledger::BlockState blockState = TAO::Ledger::ChainState::tStateBest.load();

            bool bLastStateFound = TAO::Ledger::GetLastState(blockState, 1);
            for(; (nTotal < 1440 && bLastStateFound); ++nTotal)
            {
                uint64_t nLastBlockTime = blockState.GetBlockTime();
                blockState = blockState.Prev();
                bLastStateFound = TAO::Ledger::GetLastState(blockState, 1);

                nPrimeAverageTime += (nLastBlockTime - blockState.GetBlockTime());
                nPrimeAverageDifficulty += (TAO::Ledger::GetDifficulty(blockState.nBits, 1));

            }
            nPrimeAverageDifficulty /= nTotal;
            nPrimeAverageTime /= nTotal;

            /* Calculate the hashrate based on nTimeConstant. Set the reference from 3ch and
            * Above since 1x and 2x were never useful starting difficulties. */
            nPrimePS = (uint64_t)((nPrimeTimeConstant / nPrimeAverageTime) * std::pow(50.0, (nPrimeAverageDifficulty - 3.0)));
        }

        encoding::json obj;
        obj["averagetime"] = (int)nPrimeAverageTime;
        obj["averagedifficulty"] = nPrimeAverageDifficulty;
        obj["primespersecond"] = nPrimePS;

        return obj;
    }


    /* List all the Trust Keys on the Network */
    encoding::json RPC::GetNetworkTrustKeys(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 0)
            return std::string("getnetworktrustkeys - List all the Trust Keys on the Network");

        encoding::json response;
        std::vector<encoding::json> trustKeyList;

        /* Map will store trust keys, keyed by stake rate, sorted in descending order */
        std::multimap<double, Legacy::TrustKey, std::greater<double> > mapTrustKeys;

        /* Retrieve all raw trust database keys from keychain */
        std::vector<Legacy::TrustKey> vKeys;
        if(!LLD::Trust->BatchRead("NONE", vKeys, -1))
            return debug::safe_printstr("No Trust Keys ", vKeys.size());

        /* Cutoff time for v4 trust keys. Anything prior to v4 end plus the original one timespan grace period.
         * This addresses an issue that some v4 keys produced one v5 block during grace period, but then incorrectly "expired"
         * and were replaced with a new v5 key.
         */
        uint64_t nCutoff = TAO::Ledger::EndBlockTimelock(4) + (uint64_t)(config::fTestNet ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET
                                                                                     : TAO::Ledger::TRUST_KEY_TIMESPAN);

        /* Trust keys are considered active if have stake blocks within timespan x 10 = 30 days mainnet */
        uint64_t nActiveTime = (uint64_t)(config::fTestNet ? (TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET * 10)
                                                           : (TAO::Ledger::TRUST_KEY_TIMESPAN * 10));

        /* Search through the trust keys. */
        for(const auto& trustKey : vKeys)
        {
            /* Ignore v4 trust keys */
            if(trustKey.nLastBlockTime < nCutoff)
                continue;

            /* Ignore inactive trust keys */
            if((trustKey.nLastBlockTime + nActiveTime) < TAO::Ledger::ChainState::tStateBest.load().GetBlockTime())
                continue;

            /* Put trust keys into a map keyed by stake rate (sorts them by rate) */
            double stakeRate = ((uint32_t)(trustKey.nStakeRate * 10000)) / 100.0;
            mapTrustKeys.insert (std::make_pair(stakeRate, trustKey));
        }

        /* Now have map of all trust keys. Assemble into response data */
        for(auto& item : mapTrustKeys)
        {
            encoding::json obj;
            Legacy::NexusAddress address;
            address.SetPubKey(item.second.vchPubKey);

            obj["address"] = address.ToString();
            obj["stakerate"] = item.first;
            obj["trustkey"] = item.second.ToString();

            trustKeyList.push_back(obj);
        }

        response["keys"] = trustKeyList;
        response["total"] = (uint32_t)trustKeyList.size();

        return response;
    }


    /* Returns the number of blocks in the longest block chain */
    encoding::json RPC::GetBlockCount(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 0)
            return std::string(
                "getblockcount"
                " - Returns the number of blocks in the longest block chain.");

        return (int)TAO::Ledger::ChainState::nBestHeight.load();
    }


    /* Deprecated.  Use getblockcount */
    encoding::json RPC::GetBlockNumber(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 0)
            return std::string(
                "getblocknumber - Deprecated.  Use getblockcount.");

        return (int)TAO::Ledger::ChainState::nBestHeight.load();
    }


    /* Returns difficulty as a multiple of the minimum difficulty */
    encoding::json RPC::GetDifficulty(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 0)
            return std::string(
                "getdifficulty - Returns difficulty as a multiple of the minimum difficulty.");

        TAO::Ledger::BlockState lastStakeBlockState = TAO::Ledger::ChainState::tStateBest.load();
        bool fHasStake = TAO::Ledger::GetLastState(lastStakeBlockState, 0);

        TAO::Ledger::BlockState lastPrimeBlockState = TAO::Ledger::ChainState::tStateBest.load();
        bool fHasPrime = TAO::Ledger::GetLastState(lastPrimeBlockState, 1);

        TAO::Ledger::BlockState lastHashBlockState = TAO::Ledger::ChainState::tStateBest.load();
        bool fHasHash = TAO::Ledger::GetLastState(lastHashBlockState, 2);


        if(fHasStake == false)
            debug::error(FUNCTION, "couldn't find last stake block state");
        if(fHasPrime == false)
            debug::error(FUNCTION, "couldn't find last prime block state");
        if(fHasHash == false)
            debug::error(FUNCTION, "couldn't find last hash block state");

        encoding::json obj;
        obj["stake"] = fHasStake ? TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastStakeBlockState, 0, false), 0) : 0;
        obj["prime"] = fHasPrime ? TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastPrimeBlockState, 1, false), 1) : 0;
        obj["hash"] = fHasHash ? TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastHashBlockState, 2, false), 2) : 0;

        return obj;

    }


    /* getsupplyrates
    Returns an object containing current Nexus production rates in set time intervals.
    Time Frequency is in base 13 month, 28 day totalling 364 days.
    This is to prevent error from Gregorian Figures */
    encoding::json RPC::GetSupplyRates(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 0)
            return std::string(
                "getsupplyrates"
                " - Returns an object containing current Nexus production rates in set time intervals."
                " Time Frequency is in base 13 month, 28 day totalling 364 days."
                " This is to prevent error from Gregorian Figures.");

        encoding::json obj;

        /* Read the tStateBest using hashBestChain
         * Cannot use ChainState::tStateBest for this because certain fields (like money supply) are not populated
         */
        TAO::Ledger::BlockState tStateBest;
        if(!LLD::Ledger->ReadBlock(TAO::Ledger::ChainState::hashBestChain.load(), tStateBest))
            return std::string("Block not found");

        uint32_t nMinutes = TAO::Ledger::GetChainAge(tStateBest.GetBlockTime());
        int64_t nSupply = tStateBest.nMoneySupply;
        int64_t nTarget = TAO::Ledger::CompoundSubsidy(nMinutes);

        obj["chainAge"] = (int)nMinutes;

        obj["moneysupply"] = Legacy::SatoshisToAmount(nSupply);
        obj["targetsupply"] = Legacy::SatoshisToAmount(nTarget);
        obj["inflationrate"] = ((nSupply * 100.0) / nTarget) - 100.0;

        obj["minuteSupply"] = Legacy::SatoshisToAmount(TAO::Ledger::SubsidyInterval(nMinutes, 1)); //1
        obj["hourSupply"] = Legacy::SatoshisToAmount(TAO::Ledger::SubsidyInterval(nMinutes, 60)); //60
        obj["daySupply"] = Legacy::SatoshisToAmount(TAO::Ledger::SubsidyInterval(nMinutes, 1440)); //1440
        obj["weekSupply"] = Legacy::SatoshisToAmount(TAO::Ledger::SubsidyInterval(nMinutes, 10080)); //10080
        obj["monthSupply"] = Legacy::SatoshisToAmount(TAO::Ledger::SubsidyInterval(nMinutes, 40320)); //40320
        obj["yearSupply"] = Legacy::SatoshisToAmount(TAO::Ledger::SubsidyInterval(nMinutes, 524160)); //524160

        return obj;

    }


    /* getmoneysupply <timestamp>
    Returns the total supply of Nexus produced by miners, holdings, developers, and ambassadors.
    Default timestamp is the current Unified timestamp. The timestamp is recorded as a UNIX timestamp */
    encoding::json RPC::GetMoneySupply(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() > 1)
            return std::string(
                "getmoneysupply <timestamp>"
                " - Returns the total supply of Nexus produced by miners, holdings, developers, and ambassadors."
                " Default timestamp is the current Unified timestamp. The timestamp is recorded as a UNIX timestamp");

        encoding::json obj;

        uint32_t nMinutes;

        if(params.size() > 0)
        {
            const uint32_t nNetworkStart = config::fTestNet.load() ?
                                           TAO::Ledger::NEXUS_TESTNET_TIMELOCK : TAO::Ledger::NEXUS_NETWORK_TIMELOCK;

            std::string strValue = params[0];
            uint32_t timestamp = uint32_t(std::stoul(strValue));

            if(timestamp < nNetworkStart)
                timestamp = nNetworkStart;

            nMinutes = TAO::Ledger::GetChainAge(timestamp);
        }
        else
            nMinutes = TAO::Ledger::GetChainAge(TAO::Ledger::ChainState::tStateBest.load().GetBlockTime());

        obj["chainAge"] = (int)nMinutes;
        obj["miners"] = Legacy::SatoshisToAmount(TAO::Ledger::CompoundSubsidy(nMinutes, 0));
        obj["ambassadors"] = Legacy::SatoshisToAmount(TAO::Ledger::CompoundSubsidy(nMinutes, 1));
        obj["developers"] = Legacy::SatoshisToAmount(TAO::Ledger::CompoundSubsidy(nMinutes, 2));

        return obj;
    }


    /* getblockhash <index>"
    *  Returns hash of block in best-block-chain at <index> */
    encoding::json RPC::GetBlockHash(const encoding::json& params, const bool fHelp)
    {

        if(fHelp || params.size() != 1)
            return std::string(
                "getblockhash <index>"
                " - Returns hash of block in best-block-chain at <index>.");

        if(!config::GetBoolArg("-indexheight"))
        {
            return std::string("getblockhash requires the wallet to be started with the -indexheight flag.");
        }
        int nHeight = params[0];
        if(nHeight < 0 || nHeight > TAO::Ledger::ChainState::nBestHeight.load())
            return std::string("Block number out of range.");

        TAO::Ledger::BlockState blockState;
        if(!LLD::Ledger->ReadBlock(nHeight, blockState))
            return std::string("Block not found");

        return blockState.GetHash().GetHex();

    }


    /* isorphan <hash>"
    *  Returns whether a block is an orphan or not*/
    encoding::json RPC::IsOrphan(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 1)
            return std::string(
                "isorphan <hash> "
                " - Returns whether a block is an orphan or not");


        TAO::Ledger::BlockState block;
        uint1024_t blockId = 0;
        blockId.SetHex(params[0].get<std::string>());

        if(!LLD::Ledger->ReadBlock(blockId, block))
        {
            throw TAO::API::Exception(-5, "Block not found");
            return "";
        }

        return !block.IsInMainChain();

    }


    /* getblock <hash> [txinfo]"
    *  txinfo optional to print more detailed tx info."
    *  Returns details of a block with given block-hash */
    encoding::json RPC::GetBlock(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() < 1 || params.size() > 2)
            return std::string(
                "getblock <hash> [txinfo]"
                " - txinfo optional to print more detailed tx info."
                " Returns details of a block with given block-hash.");

        TAO::Ledger::BlockState block;
        uint1024_t blockId = 0;
        blockId.SetHex(params[0].get<std::string>());

        if(!LLD::Ledger->ReadBlock(blockId, block))
        {
            throw TAO::API::Exception(-5, "Block not found");
            return "";
        }

        int nVerbose = 1; /* Default to verbosity 1 which includes only the hash */
        if(params.size() > 1 && params[1].get<bool>())
            nVerbose = 2;

        return TAO::API::BlockToJSON(block, nVerbose);
    }
}
