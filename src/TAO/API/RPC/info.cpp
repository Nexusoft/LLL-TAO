/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>
#include <Util/include/runtime.h>
#include <LLP/include/version.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/supply.h>
#include <LLP/include/global.h>
#include <LLP/include/baseaddress.h>
#include <LLP/include/trustaddress.h>
#include <Util/include/version.h>

#include <Legacy/types/minter.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>
#include <Legacy/include/money.h>
#include <TAO/Ledger/types/mempool.h>

#include <vector>
//#include <TAO/Ledger/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* getinfo
        Returns an object containing various state info */
        json::json RPC::GetInfo(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 0)
                return std::string(
                    "getinfo"
                    " - Returns an object containing various state info.");

            json::json obj;
            obj["version"] = version::CLIENT_VERSION_BUILD_STRING;
            obj["protocolversion"] = LLP::PROTOCOL_VERSION;
            obj["walletversion"] = Legacy::Wallet::GetInstance().GetVersion();
            obj["testnet"] = config::fTestNet;
            obj["balance"] = Legacy::SatoshisToAmount(Legacy::Wallet::GetInstance().GetBalance());
            obj["unconfirmedbalance"] = Legacy::SatoshisToAmount(Legacy::Wallet::GetInstance().GetUnconfirmedBalance());
            obj["newmint"] = Legacy::SatoshisToAmount(Legacy::Wallet::GetInstance().GetNewMint());
            obj["stake"] = Legacy::SatoshisToAmount(Legacy::Wallet::GetInstance().GetStake());

            /* Staking metrics */
            Legacy::StakeMinter stakeMinter = Legacy::StakeMinter::GetInstance();
            obj["stakerate"]   = stakeMinter.GetStakeRate();
            obj["stakeweight"] = stakeMinter.GetTrustWeight() + stakeMinter.GetBlockWeight(); // 100 max so is already a %
            obj["trustweight"] = stakeMinter.GetTrustWeightPercent();
            obj["blockweight"] = stakeMinter.GetBlockWeightPercent();

            obj["txtotal"] =(int)Legacy::Wallet::GetInstance().mapWallet.size();

            obj["blocks"] = (int)TAO::Ledger::ChainState::nBestHeight;

            obj["timestamp"] =  (int)runtime::unifiedtimestamp();

            obj["connections"] = GetTotalConnectionCount();
            obj["proxy"] = (config::fUseProxy ? LLP::addrProxy.ToString() : std::string());
            obj["ip"] = config::GetBoolArg("-beta") ? LLP::LEGACY_SERVER->addrThisNode.ToStringIP() : LLP::TRITIUM_SERVER->addrThisNode.ToStringIP();

            obj["testnet"] = config::fTestNet;
            obj["keypoololdest"] = (int64_t)Legacy::Wallet::GetInstance().GetKeyPool().GetOldestKeyPoolTime();
            obj["keypoolsize"] = Legacy::Wallet::GetInstance().GetKeyPool().GetKeyPoolSize();
            if (Legacy::Wallet::GetInstance().IsCrypted())
            {
                obj["locked"] = Legacy::Wallet::GetInstance().IsLocked();
                if( !Legacy::Wallet::GetInstance().IsLocked())
                {
                    if( (uint64_t) Legacy::Wallet::GetInstance().GetWalletUnlockTime() > 0 )
                        obj["unlocked_until"] = (uint64_t) Legacy::Wallet::GetInstance().GetWalletUnlockTime() ;

                    obj["minting_only"] = Legacy::fWalletUnlockMintOnly;
                }
            }

        //    obj.push_back(std::make_pair("errors",        Core::GetWarnings("statusbar")));


            return obj;
        }

        /* getpeerinfo
        Returns data about each connected network node */
        json::json RPC::GetPeerInfo(const json::json& params, bool fHelp)
        {
            json::json response;

            if (fHelp || params.size() != 0)
                    return std::string(
                        "getpeerinfo"
                        " - Returns data about each connected network node.");

            std::vector<LLP::TrustAddress> vLegacyInfo;
            std::vector<LLP::TrustAddress> vTritiumInfo;

            /* query address information from tritium server address manager */
            if(LLP::LEGACY_SERVER && LLP::LEGACY_SERVER->pAddressManager)
                 LLP::LEGACY_SERVER->pAddressManager->GetAddresses(vLegacyInfo, LLP::ConnectState::CONNECTED);

            std::sort(vLegacyInfo.begin(), vLegacyInfo.end());

            for(const auto& addr : vLegacyInfo)
            {
                json::json obj;

                obj["addr"]     = addr.ToString();
                obj["type"]     = std::string("Legacy");
                obj["latency"]  = debug::strprintf("%u ms", addr.nLatency);
                obj["lastseen"] = addr.nLastSeen;
                obj["connects"] = addr.nConnected;
                obj["drops"]    = addr.nDropped;
                obj["fails"]    = addr.nFailed;
                obj["score"]    = addr.Score();
                obj["version"]  = addr.IsIPv4() ? std::string("IPv4") : std::string("IPv6");

                response.push_back(obj);
            }

            /* query address information from legacy server address manager */
            if(LLP::TRITIUM_SERVER && LLP::TRITIUM_SERVER->pAddressManager)
                 LLP::TRITIUM_SERVER->pAddressManager->GetAddresses(vTritiumInfo, LLP::ConnectState::CONNECTED);

            std::sort(vTritiumInfo.begin(), vTritiumInfo.end());

            for(const auto& addr : vTritiumInfo)
            {
                json::json obj;

                obj["addr"]     = addr.ToString();
                obj["type"]     = std::string("Tritium");
                obj["latency"]  = debug::strprintf("%u ms", addr.nLatency);
                obj["lastseen"] = addr.nLastSeen;
                obj["connects"] = addr.nConnected;
                obj["drops"]    = addr.nDropped;
                obj["fails"]    = addr.nFailed;
                obj["score"]    = addr.Score();
                obj["version"]  = addr.IsIPv4() ? std::string("IPv4") : std::string("IPv6");

                response.push_back(obj);
            }

            return response;
        }


        /* getmininginfo
        Returns an object containing mining-related information.*/
        json::json RPC::GetMiningInfo(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 0)
                return std::string(
                    "getmininginfo"
                    " - Returns an object containing mining-related information.");

            // Prime
            uint64_t nPrimePS = 0;
            uint64_t nHashRate = 0;
            if( TAO::Ledger::ChainState::nBestHeight && TAO::Ledger::ChainState::stateBest != TAO::Ledger::ChainState::stateGenesis)
            {
                double nPrimeAverageDifficulty = 0.0;
                unsigned int nPrimeAverageTime = 0;
                unsigned int nPrimeTimeConstant = 2480;
                int nTotal = 0;
                TAO::Ledger::BlockState blockState = TAO::Ledger::ChainState::stateBest;

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
                nPrimePS = (nPrimeTimeConstant / nPrimeAverageTime) * std::pow(50.0, (nPrimeAverageDifficulty - 3.0));



                // Hash
                int nHTotal = 0;
                unsigned int nHashAverageTime = 0;
                double nHashAverageDifficulty = 0.0;
                uint64_t nTimeConstant = 276758250000;

                blockState = TAO::Ledger::ChainState::stateBest;

                bLastStateFound = TAO::Ledger::GetLastState(blockState, 2);
                for(;  (nHTotal < 1440 && bLastStateFound); nHTotal ++)
                {
                    uint64_t nLastBlockTime = blockState.GetBlockTime();
                    blockState = blockState.Prev();
                    bLastStateFound = TAO::Ledger::GetLastState(blockState, 2);

                    nHashAverageTime += (nLastBlockTime - blockState.GetBlockTime());
                    nHashAverageDifficulty += (TAO::Ledger::GetDifficulty(blockState.nBits, 2));

                }
                // protect against getmininginfo being called before hash channel start block
                if( nHTotal > 0)
                {
                    nHashAverageDifficulty /= nHTotal;
                    nHashAverageTime /= nHTotal;

                    nHashRate = (nTimeConstant / nHashAverageTime) * nHashAverageDifficulty;
                }
            }

            json::json obj;
            obj["blocks"] = (int)TAO::Ledger::ChainState::nBestHeight;
            obj["timestamp"] = (int)runtime::unifiedtimestamp();

            //PS TODO
            //obj["currentblocksize"] = (uint64_t)Core::nLastBlockSize;
            //obj["currentblocktx"] =(uint64_t)Core::nLastBlockTx;

            TAO::Ledger::BlockState lastStakeBlockState = TAO::Ledger::ChainState::stateBest;
            bool fHasStake = TAO::Ledger::GetLastState(lastStakeBlockState, 0);

            TAO::Ledger::BlockState lastPrimeBlockState = TAO::Ledger::ChainState::stateBest;
            bool fHasPrime = TAO::Ledger::GetLastState(lastPrimeBlockState, 1);

            TAO::Ledger::BlockState lastHashBlockState = TAO::Ledger::ChainState::stateBest;
            bool fHasHash = TAO::Ledger::GetLastState(lastHashBlockState, 2);


            if(fHasStake == false)
                debug::error(FUNCTION, "couldn't find last stake block state");
            if(fHasPrime == false)
                debug::error(FUNCTION, "couldn't find last prime block state");
            if(fHasHash == false)
                debug::error(FUNCTION, "couldn't find last hash block state");


            obj["stakeDifficulty"] = fHasStake ? TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastStakeBlockState, 0, false), 0) : 0;
            obj["primeDifficulty"] = fHasPrime ? TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastPrimeBlockState, 1, false), 1) : 0;
            obj["hashDifficulty"] = fHasHash ? TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastHashBlockState, 2, false), 2) : 0;
            obj["primeReserve"] =    Legacy::SatoshisToAmount(lastPrimeBlockState.nReleasedReserve[0]);
            obj["hashReserve"] =     Legacy::SatoshisToAmount(lastHashBlockState.nReleasedReserve[0]);
            obj["primeValue"] =        Legacy::SatoshisToAmount(TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::stateBest, 1, 0));
            obj["hashValue"] =         Legacy::SatoshisToAmount(TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::stateBest, 2, 0));
            obj["pooledtx"] =       (uint64_t)TAO::Ledger::mempool.Size();
            obj["primesPerSecond"] = nPrimePS;
            obj["hashPerSecond"] =  nHashRate;

            if(config::GetBoolArg("-mining", false))
            {
                //PS TODO
                //obj["totalConnections"] = LLP::MINING_LLP->TotalConnections();
            }

            //obj["genesisblockhash"] = TAO::Ledger::ChainState::stateGenesis.GetHash().GetHex();
            //obj["currentblockhash"] = TAO::Ledger::ChainState::stateBest.GetHash().GetHex();

            return obj;
        }
    }
}
