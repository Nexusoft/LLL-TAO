/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/rpc/types/rpc.h>
#include <Util/include/json.h>
#include <Util/include/runtime.h>
#include <LLP/include/version.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/supply.h>

#include <LLP/include/global.h>
#include <LLP/include/base_address.h>
#include <LLP/include/legacy_address.h>
#include <LLP/include/manager.h>

#include <LLP/templates/data.h>

#include <Util/include/version.h>

#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>
#include <Legacy/include/money.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/API/types/commands/system.h>
#include <LLP/include/lisp.h>

#include <vector>


/* Global TAO namespace. */
namespace Legacy
{

    /* getinfo
    Returns an object containing various state info */
    encoding::json RPC::GetInfo(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 0)
            return std::string(
                "getinfo - Returns an object containing various state info.");

        encoding::json obj;
        obj["version"] = version::CLIENT_VERSION_BUILD_STRING;
        obj["protocolversion"] = LLP::PROTOCOL_VERSION;
        obj["walletversion"] = Legacy::Wallet::Instance().GetVersion();
        obj["testnet"] = config::fTestNet.load();
        obj["balance"] = Legacy::SatoshisToAmount(Legacy::Wallet::Instance().GetBalance());
        obj["unconfirmedbalance"] = Legacy::SatoshisToAmount(Legacy::Wallet::Instance().GetUnconfirmedBalance());
        obj["newmint"] = Legacy::SatoshisToAmount(Legacy::Wallet::Instance().GetNewMint());
        obj["stake"] = Legacy::SatoshisToAmount(Legacy::Wallet::Instance().GetStake());

        /* Staking metrics */
        obj["staking"] = "Not Started";
        obj["genesismature"] = false;
        obj["stakerate"]   = 0.0;
        obj["trustweight"] = 0.0;
        obj["blockweight"] = 0.0;
        obj["stakeweight"] = 0.0;
        obj["txtotal"] = (int32_t)Legacy::Wallet::Instance().mapWallet.size();

        obj["blocks"] = (int32_t)TAO::Ledger::ChainState::nBestHeight.load();

        obj["timestamp"] =  (uint64_t)runtime::unifiedtimestamp();
        obj["offset"]    =  (int32_t)UNIFIED_AVERAGE_OFFSET.load();
        obj["connections"] = GetTotalConnectionCount();
        obj["synccomplete"] = (int32_t)TAO::Ledger::ChainState::PercentSynchronized();

        // get the EID's if using LISP
        std::map<std::string, LLP::EID> mapEIDs = LLP::GetEIDs();
        if(mapEIDs.size() > 0)
        {
            encoding::json jsonEIDs = encoding::json::array();
            for(const auto& eid : mapEIDs)
            {
                jsonEIDs.push_back(eid.first);
            }
            obj["eids"] = jsonEIDs;
        }

        obj["testnet"]       = config::fTestNet.load();
        obj["keypoololdest"] = (int64_t)Legacy::Wallet::Instance().GetKeyPool().GetOldestKeyPoolTime();
        obj["keypoolsize"]   = Legacy::Wallet::Instance().GetKeyPool().GetKeyPoolSize();
        obj["paytxfee"]      = Legacy::SatoshisToAmount(Legacy::TRANSACTION_FEE);

        if(Legacy::Wallet::Instance().IsCrypted())
        {
            obj["locked"] = Legacy::Wallet::Instance().IsLocked();
            if(!Legacy::Wallet::Instance().IsLocked())
            {
                if((uint64_t) Legacy::Wallet::Instance().GetWalletUnlockTime() > 0)
                    obj["unlocked_until"] = (uint64_t) Legacy::Wallet::Instance().GetWalletUnlockTime();

                obj["minting_only"] = Legacy::fWalletUnlockMintOnly;
            }
        }

    //    obj.push_back(std::make_pair("errors",        Core::GetWarnings("statusbar")));


        return obj;
    }

    /* getpeerinfo
    Returns data about each connected network node */
    encoding::json RPC::GetPeerInfo(const encoding::json& params, const bool fHelp)
    {
        encoding::json response;

        if(fHelp || params.size() != 0)
                return std::string(
                    "getpeerinfo - Returns data about each connected network node.");

        /* Get the connections from the tritium server */
        std::vector<std::shared_ptr<LLP::TritiumNode>> vConnections = LLP::TRITIUM_SERVER->GetConnections();

        /* Iterate the connections*/
        for(const auto& connection : vConnections)
        {
            /* Skip over inactive connections. */
            if(!connection.get())
                continue;

            /* Push the active connection. */
            if(connection.get()->Connected())
            {
                encoding::json obj;

                obj["addr"]     = connection.get()->addr.ToString();
                obj["type"]     = connection.get()->strFullVersion;
                obj["version"]  = connection.get()->nProtocolVersion;
                obj["session"]  = connection.get()->nCurrentSession;
                obj["height"]   = connection.get()->nCurrentHeight;
                obj["best"]     = connection.get()->hashBestChain.SubString();
                obj["latency"]  = connection.get()->nLatency.load();
                obj["lastseen"] = connection.get()->nLastPing.load();
                obj["session"]  = connection.get()->nCurrentSession;
                obj["outgoing"] = connection.get()->fOUTGOING.load();

                response.push_back(obj);

            }
        }

        return response;
    }


    /* getmininginfo
    Returns an object containing mining-related information.*/
    encoding::json RPC::GetMiningInfo(const encoding::json& params, const bool fHelp)
    {
        if(fHelp || params.size() != 0)
            return std::string(
                "getmininginfo - Returns an object containing mining-related information.");

        // Prime
        uint64_t nPrimePS = 0;
        uint64_t nHashRate = 0;
        if(TAO::Ledger::ChainState::nBestHeight.load()
        && TAO::Ledger::ChainState::tStateBest.load() != TAO::Ledger::ChainState::tStateGenesis)
        {
            double nPrimeAverageDifficulty = 0.0;
            uint32_t nPrimeAverageTime = 0;
            uint32_t nPrimeTimeConstant = 2480;
            int32_t nTotal = 0;
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
            if(nTotal > 0)
            {
                nPrimeAverageDifficulty /= nTotal;
                nPrimeAverageTime /= nTotal;
                nPrimePS = (nPrimeTimeConstant / nPrimeAverageTime) * std::pow(50.0, (nPrimeAverageDifficulty - 3.0));
            }
            else
            {
                /* Edge case where there are no prime blocks so use the difficulty from genesis */
                blockState = TAO::Ledger::ChainState::tStateGenesis;
                nPrimeAverageDifficulty += (TAO::Ledger::GetDifficulty(blockState.nBits, 1));
            }



            // Hash
            int32_t nHTotal = 0;
            uint32_t nHashAverageTime = 0;
            double nHashAverageDifficulty = 0.0;
            uint64_t nTimeConstant = 276758250000;

            blockState = TAO::Ledger::ChainState::tStateBest.load();

            bLastStateFound = TAO::Ledger::GetLastState(blockState, 2);
            for(;  (nHTotal < 1440 && bLastStateFound); ++nHTotal)
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

                nHashRate = (nTimeConstant / nHashAverageTime) * nHashAverageDifficulty;
            }
            else
            {
                /* Edge case where there are no hash blocks so use the difficulty from genesis */
                blockState = TAO::Ledger::ChainState::tStateGenesis;
                nPrimeAverageDifficulty += (TAO::Ledger::GetDifficulty(blockState.nBits, 2));
            }
        }

        encoding::json obj;
        obj["blocks"] = (int32_t)TAO::Ledger::ChainState::nBestHeight.load();
        obj["timestamp"] = (int32_t)runtime::unifiedtimestamp();

        //PS TODO
        //obj["currentblocksize"] = (uint64_t)Core::nLastBlockSize;
        //obj["currentblocktx"] =(uint64_t)Core::nLastBlockTx;

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


        obj["stakeDifficulty"] = fHasStake ?
            TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastStakeBlockState, 0, false), 0) : 0;

        obj["primeDifficulty"] = fHasPrime ?
            TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastPrimeBlockState, 1, false), 1) : 0;

        obj["hashDifficulty"] = fHasHash ?
            TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastHashBlockState, 2, false), 2) : 0;

        obj["primeReserve"] =    Legacy::SatoshisToAmount(lastPrimeBlockState.nReleasedReserve[0]);
        obj["hashReserve"] =     Legacy::SatoshisToAmount(lastHashBlockState.nReleasedReserve[0]);
        obj["primeValue"] =        Legacy::SatoshisToAmount(TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::tStateBest.load(), 1, 0));
        obj["hashValue"] =         Legacy::SatoshisToAmount(TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::tStateBest.load(), 2, 0));
        obj["pooledtx"] =       (uint64_t)TAO::Ledger::mempool.Size();
        obj["primesPerSecond"] = nPrimePS;
        obj["hashPerSecond"] =  nHashRate;

        if(config::GetBoolArg("-mining", false))
        {
            //PS TODO
            //obj["totalConnections"] = LLP::MINING_LLP->TotalConnections();
        }

        //obj["genesisblockhash"] = TAO::Ledger::ChainState::tStateGenesis.GetHash().GetHex();
        //obj["currentblockhash"] = TAO::Ledger::ChainState::tStateBest.load().GetHash().GetHex();

        return obj;
    }
}
