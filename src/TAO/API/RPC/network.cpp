/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>
#include <Util/include/hex.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/supply.h>
#include <Legacy/include/money.h>
#include <TAO/Ledger/include/difficulty.h>
#include <LLD/include/global.h>
#include <TAO/Ledger/types/tritium.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Get network hashrate for the hashing channel */
        json::json RPC::GetNetworkHashps(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 0)
                return std::string(
                    "getnetworkhashps"
                    " - Get network hashrate for the hashing channel.");

            /* This constant was determined by finding the time it takes to find hash of given difficulty at a given hash rate.
             * It is the total hashes per second required to find a hash of difficulty 1.0 every second.
             * This can then be used in calculing the network hash rate by block times over average of 1440 blocks.
             * */
            uint64_t nHashRate = 0;
            int nHTotal = 0;
            unsigned int nHashAverageTime = 0;
            double nHashAverageDifficulty = 0.0;
            if( TAO::Ledger::ChainState::nBestHeight && TAO::Ledger::ChainState::stateBest != TAO::Ledger::ChainState::stateGenesis)
            {
                uint64_t nTimeConstant = 276758250000;

                TAO::Ledger::BlockState blockState = TAO::Ledger::ChainState::stateBest;

                bool bLastStateFound = TAO::Ledger::GetLastState(blockState, 2);
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

                    /* Calculate the hashrate based on nTimeConstant. */
                    nHashRate = (uint64_t) ((nTimeConstant / nHashAverageTime) * nHashAverageDifficulty);
                }
            }

            json::json obj;
            obj["averagetime"] =  (int) nHashAverageTime;
            obj["averagedifficulty"] = nHashAverageDifficulty;
            obj["hashrate"] = nHashRate;

            return obj;
        }

        /* Get network prime searched per second */
        json::json RPC::GetNetworkPps(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 0)
                return std::string(
                    "getnetworkpps"
                    " - Get network prime searched per second.");

            /* This constant was determined by finding the time it takes to find primes at given difficulty and Primes Per Second'
             * This constant was found that 2,450 Prime Per Second is required to find 1 3ch Cluster per Second.
             * The difficulty changes are exponential or in other words require 50x more work per difficulty increase
             */
            uint64_t nPrimePS = 0;
            double nPrimeAverageDifficulty = 0.0;
            unsigned int nPrimeAverageTime = 0;
            if( TAO::Ledger::ChainState::nBestHeight && TAO::Ledger::ChainState::stateBest != TAO::Ledger::ChainState::stateGenesis)
            {

                unsigned int nPrimeTimeConstant = 2480;
                int nTotal = 0;
                TAO::Ledger::BlockState blockState = TAO::Ledger::ChainState::stateBest;

                bool bLastStateFound = TAO::Ledger::GetLastState(blockState, 1);
                for(; (nTotal < 1440 && bLastStateFound); nTotal ++)
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




            json::json obj;
            obj["averagetime"] = (int) nPrimeAverageTime;
            obj["averagedifficulty"] = nPrimeAverageDifficulty;
            obj["primespersecond"] = nPrimePS;

            return obj;
        }

        /* List all the Trust Keys on the Network */
        json::json RPC::GetNetworkTrustKeys(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 0)
                return std::string(
                    "getnetworktrustkeys"
                    " - List all the Trust Keys on the Network");


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
        //         Legacy::NexusAddress address;
        //         address.SetPubKey(trustKey.vchPubKey);

        //         /* Read the previous block from disk. */
        //         Core::CBlock block;
        //         if(!block.ReadFromDisk(Core::mapBlockIndex[hashLastBlock], true))
        //             continue;

        //         obj["address", address.ToString()));
        //         obj["interestrate", 100.0 * trustKey.InterestRate(block, GetUnifiedTimestamp())));
        //         obj["trustkey", trustKey.ToString()));

        //         trustkeys.push_back(obj);
        //     }

        //     ret["keys", trustkeys));
        //     ret["total", (int)trustkeys.size()));

        //     return ret;
            json::json ret;
            ret = "NOT AVAILABLE IN THIS RELEASE";
            return ret;
        }

        /* Returns the number of blocks in the longest block chain */
        json::json RPC::GetBlockCount(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 0)
                return std::string(
                    "getblockcount"
                    " - Returns the number of blocks in the longest block chain.");

            return (int)TAO::Ledger::ChainState::nBestHeight;
        }


        /* Deprecated.  Use getblockcount */
        json::json RPC::GetBlockNumber(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 0)
                return std::string(
                    "getblocknumber"
                    " - Deprecated.  Use getblockcount.");

            return (int)TAO::Ledger::ChainState::nBestHeight;
        }

        /* Returns difficulty as a multiple of the minimum difficulty */
        json::json RPC::GetDifficulty(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 0)
                return std::string(
                    "getdifficulty"
                    " - Returns difficulty as a multiple of the minimum difficulty.");

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

            json::json obj;
            obj["stake"] = fHasStake ? TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastStakeBlockState, 0, false), 0) : 0;
            obj["prime"] = fHasPrime ? TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastPrimeBlockState, 1, false), 1) : 0;
            obj["hash"] = fHasHash ? TAO::Ledger::GetDifficulty(TAO::Ledger::GetNextTargetRequired(lastHashBlockState, 2, false), 2) : 0;

            return obj;

        }

        /* getsupplyrates
        Returns an object containing current Nexus production rates in set time intervals.
        Time Frequency is in base 13 month, 28 day totalling 364 days.
        This is to prevent error from Gregorian Figures */
        json::json RPC::GetSupplyRates(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 0)
                return std::string(
                    "getsupplyrates"
                    " - Returns an object containing current Nexus production rates in set time intervals."
                    " Time Frequency is in base 13 month, 28 day totalling 364 days."
                    " This is to prevent error from Gregorian Figures.");

            json::json obj;
            unsigned int nMinutes = TAO::Ledger::GetChainAge(TAO::Ledger::ChainState::stateBest.GetBlockTime());

            obj["chainAge"] = (int)nMinutes;

            int64_t nSupply = TAO::Ledger::ChainState::stateBest.nMoneySupply;
            int64_t nTarget = TAO::Ledger::CompoundSubsidy(nMinutes);

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
        json::json RPC::GetMoneySupply(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 0)
                return std::string(
                    "getmoneysupply <timestamp>"
                    " - Returns the total supply of Nexus produced by miners, holdings, developers, and ambassadors."
                    " Default timestamp is the current Unified timestamp. The timestamp is recorded as a UNIX timestamp");

            json::json obj;
            unsigned int nMinutes = TAO::Ledger::GetChainAge(TAO::Ledger::ChainState::stateBest.GetBlockTime());

            obj["chainAge"] = (int)nMinutes;
            obj["miners"] = Legacy::SatoshisToAmount(TAO::Ledger::CompoundSubsidy(nMinutes, 0));
            obj["ambassadors"] = Legacy::SatoshisToAmount(TAO::Ledger::CompoundSubsidy(nMinutes, 1));
            obj["developers"] = Legacy::SatoshisToAmount(TAO::Ledger::CompoundSubsidy(nMinutes, 2));

            return obj;
        }

        /* getblockhash <index>"
        *  Returns hash of block in best-block-chain at <index> */
        json::json RPC::GetBlockHash(const json::json& params, bool fHelp)
        {

            if (fHelp || params.size() != 1)
                return std::string(
                    "getblockhash <index>"
                    " - Returns hash of block in best-block-chain at <index>.");

            if(!config::GetBoolArg("-indexheight"))
            {
                return std::string("getblockhash requires the wallet to be started with the -indexheight flag.");
            }
            int nHeight = params[0];
            if (nHeight < 0 || nHeight > TAO::Ledger::ChainState::nBestHeight)
                return std::string("Block number out of range.");

            TAO::Ledger::BlockState blockState;
            if(!LLD::legDB->ReadBlock(nHeight, blockState))
                return std::string("Block not found");

            return blockState.GetHash().GetHex();

        }

        /* isorphan <hash>"
        *  Returns whether a block is an orphan or not*/
        json::json RPC::IsOrphan(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 1 )
                return std::string(
                    "isorphan <hash> "
                    " - Returns whether a block is an orphan or not");


            TAO::Ledger::BlockState block;
            uint1024_t blockId = 0;
            blockId.SetHex(params[0].get<std::string>());

            if (!LLD::legDB->ReadBlock(blockId, block))
            {
                throw APIException(-5, "Block not found");
                return "";
            }

            return !block.IsInMainChain();

        }

        json::json blockToJSON(const TAO::Ledger::BlockState& block,  bool fPrintTransactionDetail)
        {
            json::json result;
            result["hash"] = block.GetHash().GetHex();
            // the hash that was relevant for Proof of Stake or Proof of Work (depending on block version)
            result["proofhash"] =
                                    block.nVersion < 5 ? block.GetHash().GetHex() :
                                    ((block.nChannel == 0) ? block.StakeHash().GetHex() : block.ProofHash().GetHex());

            result["size"] = (int)::GetSerializeSize(block, SER_NETWORK, LLP::PROTOCOL_VERSION);
            result["height"] = (int)block.nHeight;
            result["channel"] = (int)block.nChannel;
            result["version"] = (int)block.nVersion;
            result["merkleroot"] = block.hashMerkleRoot.GetHex();
            result["time"] = convert::DateTimeStrFormat(block.GetBlockTime());
            result["nonce"] = (uint64_t)block.nNonce;
            result["bits"] = HexBits(block.nBits);
            result["difficulty"] = TAO::Ledger::GetDifficulty(block.nBits, block.nChannel);
            result["mint"] = Legacy::SatoshisToAmount(block.nMint);
            if (block.hashPrevBlock != 0)
                result["previousblockhash"] = block.hashPrevBlock.GetHex();
            if (block.hashNextBlock != 0)
                result["nextblockhash"] = block.hashNextBlock.GetHex();

            json::json txinfo = json::json::array();

            for (const auto& vtx : block.vtx)
            {
                if(vtx.first == TAO::Ledger::TYPE::TRITIUM_TX)
                {

                    /* Get the tritium transaction  from the database*/
                    TAO::Ledger::Transaction tx;
                    if(LLD::legDB->ReadTx(vtx.second, tx))
                    {
                        if (fPrintTransactionDetail)
                        {
                            txinfo.push_back(tx.ToStringShort());
                            txinfo.push_back(convert::DateTimeStrFormat(tx.nTimestamp));
                        }
                        else
                            txinfo.push_back(tx.GetHash().GetHex());
                    }
                }
                else if(vtx.first == TAO::Ledger::TYPE::LEGACY_TX)
                {
                    /* Get the legacy transaction from the database. */
                    Legacy::Transaction tx;
                    if(LLD::legacyDB->ReadTx(vtx.second, tx))
                    {
                        if (fPrintTransactionDetail)
                        {
                            txinfo.push_back(tx.ToStringShort());
                            txinfo.push_back(convert::DateTimeStrFormat(tx.nTime));
                            for(const Legacy::TxIn& txin : tx.vin)
                                txinfo.push_back(txin.ToStringShort());
                            for(const Legacy::TxOut& txout : tx.vout)
                                txinfo.push_back(txout.ToStringShort());
                        }
                        else
                            txinfo.push_back(tx.GetHash().GetHex());
                    }
                }
            }

            result["tx"] = txinfo;
            return result;
        }

        /* getblock <hash> [txinfo]"
        *  txinfo optional to print more detailed tx info."
        *  Returns details of a block with given block-hash */
        json::json RPC::GetBlock(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() < 1 || params.size() > 2)
                return std::string(
                    "getblock <hash> [txinfo]"
                    " - txinfo optional to print more detailed tx info."
                    " Returns details of a block with given block-hash.");

            TAO::Ledger::BlockState block;
            uint1024_t blockId = 0;
            blockId.SetHex(params[0].get<std::string>());

            if (!LLD::legDB->ReadBlock(blockId, block))
            {
                throw APIException(-5, "Block not found");
                return "";
            }

            return blockToJSON(block, params.size() > 1 ? params[1].get<bool>() : false);
        }
    }
}
