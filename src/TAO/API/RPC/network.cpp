/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>

namespace TAO::API
{
    // Value getnetworkhashps(const json::json& jsonParams, bool fHelp)
    // {
    //     if (fHelp || params.size() != 0)
    //         throw runtime_error(
    //             "getnetworkhashps\n"
    //             "Get network hashrate for the hashing channel.");

    //     /* This constant was determined by finding the time it takes to find hash of given difficulty at a given hash rate.
    //      * It is the total hashes per second required to find a hash of difficulty 1.0 every second.
    //      * This can then be used in calculing the network hash rate by block times over average of 1440 blocks.
    //      * */
    //     uint64 nTimeConstant = 276758250000;

    //     const Core::CBlockIndex* pindex = Core::GetLastChannelIndex(Core::pindexBest, 2);
    //     unsigned int nAverageTime = 0, nTotal = 0;
    //     double nAverageDifficulty = 0.0;

    //     for( ; nTotal < 1440 && pindex->pprev; nTotal ++) {

    //         nAverageTime += (pindex->GetBlockTime() - Core::GetLastChannelIndex(pindex->pprev, 2)->GetBlockTime());
    //         nAverageDifficulty += (Core::GetDifficulty(pindex->nBits, 2));

    //         pindex = Core::GetLastChannelIndex(pindex->pprev, 2);
    //     }

    //     if(nTotal == 0)
    //     throw runtime_error("getnetworkhashps\n"
    //                         "No Blocks produced on Hashing Channel.");

    //     nAverageTime       /= nTotal;
    //     nAverageDifficulty /= nTotal;
    //     /* TODO END **/

    //     Object obj;
    //     obj.push_back(Pair("averagetime", (int) nAverageTime));
    //     obj.push_back(Pair("averagedifficulty", nAverageDifficulty));

    //     /* Calculate the hashrate based on nTimeConstant. */
    //     uint64 nHashRate = (nTimeConstant / nAverageTime) * nAverageDifficulty;

    //     obj.push_back(Pair("hashrate", (boost::uint64_t)nHashRate));

    //     return obj;
    // }


    // Value getnetworkpps(const json::json& jsonParams, bool fHelp)
    // {
    //     if (fHelp || params.size() != 0)
    //         throw runtime_error(
    //             "getnetworkpps\n"
    //             "Get network prime searched per second.");

    //     /* This constant was determined by finding the time it takes to find primes at given difficulty and Primes Per Second'
    //      * This constant was found that 2,450 Prime Per Second is required to find 1 3ch Cluster per Second.
    //      * The difficulty changes are exponential or in other words require 50x more work per difficulty increase
    //      */
    //     unsigned int nTimeConstant = 2480;

    //     const Core::CBlockIndex* pindex = Core::GetLastChannelIndex(Core::pindexBest, 1);
    //     unsigned int nAverageTime = 0, nTotal = 0;
    //     double nAverageDifficulty = 0.0;

    //     for( ; nTotal < 1440 && pindex->pprev; nTotal ++) {

    //         nAverageTime += (pindex->GetBlockTime() - Core::GetLastChannelIndex(pindex->pprev, 1)->GetBlockTime());
    //         nAverageDifficulty += (Core::GetDifficulty(pindex->nBits, 1));

    //         pindex = Core::GetLastChannelIndex(pindex->pprev, 1);
    //     }

    //     nAverageTime       /= nTotal;
    //     nAverageDifficulty /= nTotal;

    //     Object obj;
    //     obj.push_back(Pair("averagetime", (int) nAverageTime));
    //     obj.push_back(Pair("averagedifficulty", nAverageDifficulty));

    //     /* Calculate the hashrate based on nTimeConstant. Set the reference from 3ch and
    //      * Above since 1x and 2x were never useful starting difficulties. */
    //     uint64 nHashRate = (nTimeConstant / nAverageTime) * std::pow(50.0, (nAverageDifficulty - 3.0));

    //     obj.push_back(Pair("primespersecond", (boost::uint64_t)nHashRate));

    //     return obj;
    // }

    // Value getnetworktrustkeys(const json::json& jsonParams, bool fHelp)
    // {
    //     if (fHelp || params.size() != 0)
    //         throw runtime_error(
    //             "getnetworktrustkeys\n"
    //             "List all the Trust Keys on the Network");


    //     unsigned int nTotalActive = 0;
    //     Array trustkeys;
    //     Object ret;

    //     std::vector<uint576> vKeys;
    //     LLD::CIndexDB indexdb("cr");
    //     if(!indexdb.GetTrustKeys(vKeys))
    //         return ret;

    //     /* Search through the trust keys. */
    //     for(auto key : vKeys)
    //     {
    //         Core::CTrustKey trustKey;
    //         if(!indexdb.ReadTrustKey(key, trustKey))
    //             continue;

    //         uint1024 hashLastBlock = trustKey.hashLastBlock;
    //         if(!Core::mapBlockIndex.count(hashLastBlock))
    //             continue;

    //         if(Core::mapBlockIndex[hashLastBlock]->GetBlockTime() < (fTestNet ? Core::TESTNET_VERSION_TIMELOCK[3] : Core::NETWORK_VERSION_TIMELOCK[3]))
    //             continue;

    //         Object obj;
    //         Wallet::NexusAddress address;
    //         address.SetPubKey(trustKey.vchPubKey);

    //         /* Read the previous block from disk. */
    //         Core::CBlock block;
    //         if(!block.ReadFromDisk(Core::mapBlockIndex[hashLastBlock], true))
    //             continue;

    //         obj.push_back(Pair("address", address.ToString()));
    //         obj.push_back(Pair("interestrate", 100.0 * trustKey.InterestRate(block, GetUnifiedTimestamp())));
    //         obj.push_back(Pair("trustkey", trustKey.ToString()));

    //         trustkeys.push_back(obj);
    //     }

    //     ret.push_back(Pair("keys", trustkeys));
    //     ret.push_back(Pair("total", (int)trustkeys.size()));

    //     return ret;
    // }


    // Value getblockcount(const json::json& jsonParams, bool fHelp)
    // {
    //     if (fHelp || params.size() != 0)
    //         throw runtime_error(
    //             "getblockcount\n"
    //             "Returns the number of blocks in the longest block chain.");

    //     return (int)Core::nBestHeight;
    // }


    // // deprecated
    // Value getblocknumber(const json::json& jsonParams, bool fHelp)
    // {
    //     if (fHelp || params.size() != 0)
    //         throw runtime_error(
    //             "getblocknumber\n"
    //             "Deprecated.  Use getblockcount.");

    //     return (int)Core::nBestHeight;
    // }

    // Value getdifficulty(const json::json& jsonParams, bool fHelp)
    // {
    //     if (fHelp || params.size() != 0)
    //         throw runtime_error(
    //             "getdifficulty\n"
    //             "Returns difficulty as a multiple of the minimum difficulty.");

    //     const Core::CBlockIndex* pindexPOS = Core::GetLastChannelIndex(Core::pindexBest, 0);
    //     const Core::CBlockIndex* pindexCPU = Core::GetLastChannelIndex(Core::pindexBest, 1);
    //     const Core::CBlockIndex* pindexGPU = Core::GetLastChannelIndex(Core::pindexBest, 2);


    //     Object obj;
    //     obj.push_back(Pair("prime",        Core::GetDifficulty(pindexCPU->nBits, 1)));
    //     obj.push_back(Pair("hash",        Core::GetDifficulty(pindexGPU->nBits, 2)));

    //     obj.push_back(Pair("stake",       Core::GetDifficulty(pindexPOS->nBits, 0)));
    //     return obj;
    // }

    // Value getsupplyrates(const json::json& jsonParams, bool fHelp)
    // {
    //     if (fHelp || params.size() != 0)
    //         throw runtime_error(
    //             "getsupplyrates\n"
    //             "Returns an object containing current Nexus production rates in set time intervals.\n"
    //             "Time Frequency is in base 13 month, 28 day totalling 364 days.\n"
    //             "This is to prevent error from Gregorian Figures.");

    //     Object obj;
    //     unsigned int nMinutes = Core::GetChainAge(Core::pindexBest->GetBlockTime());

    //     obj.push_back(Pair("chainAge",       (int)nMinutes));

    //     int64 nSupply = Core::pindexBest->nMoneySupply;
    //     int64 nTarget = Core::CompoundSubsidy(nMinutes);

    //     obj.push_back(Pair("moneysupply",   ValueFromAmount(nSupply)));
    //     obj.push_back(Pair("targetsupply",   ValueFromAmount(nTarget)));
    //     obj.push_back(Pair("inflationrate",   ((nSupply * 100.0) / nTarget) - 100.0));

    //     obj.push_back(Pair("minuteSupply",  ValueFromAmount(Core::SubsidyInterval(nMinutes, 1)))); //1
    //     obj.push_back(Pair("hourSupply",    ValueFromAmount(Core::SubsidyInterval(nMinutes, 60)))); //60
    //     obj.push_back(Pair("daySupply",     ValueFromAmount(Core::SubsidyInterval(nMinutes, 1440)))); //1440
    //     obj.push_back(Pair("weekSupply",    ValueFromAmount(Core::SubsidyInterval(nMinutes, 10080)))); //10080
    //     obj.push_back(Pair("monthSupply",   ValueFromAmount(Core::SubsidyInterval(nMinutes, 40320)))); //40320
    //     obj.push_back(Pair("yearSupply",    ValueFromAmount(Core::SubsidyInterval(nMinutes, 524160)))); //524160

    //     return obj;
    // }

    // Value getmoneysupply(const json::json& jsonParams, bool fHelp)
    // {
    //     if(fHelp || params.size() != 0)
    //         throw runtime_error(
    //             "getmoneysupply <timestamp>\n\n"
    //             "Returns the total supply of Nexus produced by miners, holdings, developers, and ambassadors.\n"
    //             "Default timestamp is the current Unified Timestamp. The timestamp is recorded as a UNIX timestamp");

    //     Object obj;
    //     unsigned int nMinutes = Core::GetChainAge(Core::pindexBest->GetBlockTime());

    //     obj.push_back(Pair("chainAge",       (int)nMinutes));
    //     obj.push_back(Pair("miners", ValueFromAmount(Core::CompoundSubsidy(nMinutes, 0))));
    //     obj.push_back(Pair("ambassadors", ValueFromAmount(Core::CompoundSubsidy(nMinutes, 1))));
    //     obj.push_back(Pair("developers", ValueFromAmount(Core::CompoundSubsidy(nMinutes, 2))));

    //     return obj;
    // }

    
    
}