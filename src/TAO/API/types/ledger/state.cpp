/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>

#include <TAO/API/types/ledger.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/mempool.h>

#include <Legacy/include/money.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Returns an object containing mining-related information. */
        json::json Ledger::MiningInfo(const json::json& params, bool fHelp)
        {
            // Prime
            uint64_t nPrimePS = 0;
            uint64_t nHashRate = 0;

            /* Check edge case where we are still at genesis */
            if(TAO::Ledger::ChainState::nBestHeight.load() && TAO::Ledger::ChainState::stateBest.load() != TAO::Ledger::ChainState::stateGenesis)
            {
                /* Declare variables for working out prime difficulty */
                double nPrimeAverageDifficulty = 0.0;
                unsigned int nPrimeAverageTime = 0;
                unsigned int nPrimeTimeConstant = 2480;
                int nTotal = 0;

                /* Get the latest block */
                TAO::Ledger::BlockState blockState = TAO::Ledger::ChainState::stateBest.load();

                /* Get the last block for the prime channel up to the latest block*/
                bool bLastStateFound = TAO::Ledger::GetLastState(blockState, 1);

                /* Take a max of 1440 samples, which equates to ~20 hours*/
                for(; (nTotal < 1440 && bLastStateFound); ++nTotal)
                {
                    uint64_t nLastBlockTime = blockState.GetBlockTime();

                    /* Move back a block and find the last prime block up to this one*/
                    blockState = blockState.Prev();
                    bLastStateFound = TAO::Ledger::GetLastState(blockState, 1);

                    nPrimeAverageTime += (nLastBlockTime - blockState.GetBlockTime());
                    nPrimeAverageDifficulty += (TAO::Ledger::GetDifficulty(blockState.nBits, 1));

                }
                /* If we have at least 1 prime block, work out the averages*/
                if(nTotal > 0)
                {
                    nPrimeAverageDifficulty /= nTotal;
                    nPrimeAverageTime /= nTotal;
                    nPrimePS = (nPrimeTimeConstant / nPrimeAverageTime) * std::pow(50.0, (nPrimeAverageDifficulty - 3.0));
                }
                else
                {
                    /* Edge case where there are no prime blocks so use the difficulty from genesis */
                    blockState = TAO::Ledger::ChainState::stateGenesis;
                    nPrimeAverageDifficulty += (TAO::Ledger::GetDifficulty(blockState.nBits, 1));
                }


                /* Declare variables for working out hash difficulty */
                int nHTotal = 0;
                unsigned int nHashAverageTime = 0;
                double nHashAverageDifficulty = 0.0;
                uint64_t nTimeConstant = 276758250000;

                /* Get the latest block */
                blockState = TAO::Ledger::ChainState::stateBest.load();

                /* Get the last block for the hash channel up to the latest block*/
                bLastStateFound = TAO::Ledger::GetLastState(blockState, 2);

                /* Take a max of 1440 samples, which equates to ~20 hours*/
                for(; (nHTotal < 1440 && bLastStateFound); ++nHTotal)
                {
                    /* Get the block time for this block so that we can work out the time between successive hash blocks */
                    uint64_t nLastBlockTime = blockState.GetBlockTime();

                    /* Move back a block and find the last hash block up to this one*/
                    blockState = blockState.Prev();
                    bLastStateFound = TAO::Ledger::GetLastState(blockState, 2);

                    nHashAverageTime += (nLastBlockTime - blockState.GetBlockTime());
                    nHashAverageDifficulty += (TAO::Ledger::GetDifficulty(blockState.nBits, 2));

                }
                /* protect against getmininginfo being called before hash channel start block */
                /* If we have at least 1 hash block, work out the averages */
                if(nHTotal > 0)
                {
                    nHashAverageDifficulty /= nHTotal;
                    nHashAverageTime /= nHTotal;

                    nHashRate = (nTimeConstant / nHashAverageTime) * nHashAverageDifficulty;
                }
                else
                {
                    /* Edge case where there are no hash blocks so use the difficulty from genesis */
                    blockState = TAO::Ledger::ChainState::stateGenesis;
                    nPrimeAverageDifficulty += (TAO::Ledger::GetDifficulty(blockState.nBits, 2));
                }
            }

            json::json obj;
            obj["blocks"] = (int)TAO::Ledger::ChainState::nBestHeight.load();
            obj["timestamp"] = (int)runtime::unifiedtimestamp();

            //PS TODO
            //obj["currentblocksize"] = (uint64_t)Core::nLastBlockSize;
            //obj["currentblocktx"] =(uint64_t)Core::nLastBlockTx;

            TAO::Ledger::BlockState lastStakeBlockState = TAO::Ledger::ChainState::stateBest.load();
            bool fHasStake = TAO::Ledger::GetLastState(lastStakeBlockState, 0);

            TAO::Ledger::BlockState lastPrimeBlockState = TAO::Ledger::ChainState::stateBest.load();
            bool fHasPrime = TAO::Ledger::GetLastState(lastPrimeBlockState, 1);

            TAO::Ledger::BlockState lastHashBlockState = TAO::Ledger::ChainState::stateBest.load();
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
            obj["primeValue"] =        Legacy::SatoshisToAmount(TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::stateBest.load(), 1, 0));
            obj["hashValue"] =         Legacy::SatoshisToAmount(TAO::Ledger::GetCoinbaseReward(TAO::Ledger::ChainState::stateBest.load(), 2, 0));
            obj["pooledtx"] =       (uint64_t)TAO::Ledger::mempool.Size();
            obj["primesPerSecond"] = nPrimePS;
            obj["hashPerSecond"] =  nHashRate;

            if(config::GetBoolArg("-mining", false))
            {
                obj["totalConnections"] = LLP::MINING_SERVER->GetConnectionCount();
            }

            return obj;
        }


    } /* End API Namespace*/
} /* End Ledger Namespace*/
