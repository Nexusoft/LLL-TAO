/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/Ledger/types/stake_minter_pooled.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/session.h>
#include <TAO/API/include/sessionmanager.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/stakepool.h>

#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <cmath>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger namespace. */
    namespace Ledger
    {

        /* Constructor */
        StakeMinterPooled::StakeMinterPooled(uint256_t& sessionId)
        : StakeMinter(sessionId)
        , txProducer()
        , nProducerSize(0)
        , nPoolSize(0)
        , nPoolStake(0)
        , nPoolFee(0)
        {
        }


        /* Creates a new pooled Proof of Stake block */
        bool StakeMinterPooled::CreateCandidateBlock()
        {
            /* Get setting for maximum number of relays. Assure it is positive with max value of 255 */
            static const uint8_t nTTL = static_cast<uint8_t>(
                                            std::min(
                                            (uint64_t)255,
                                            static_cast<uint64_t>(
                                                std::max(
                                                (int64_t)0,
                                                (int64_t)config::GetArg("-poolstakettl", POOL_MAX_TTL_COUNT)
                                                ))
                                            ));

            /* Reset any prior value of trust score and block age */
            nTrust = 0;
            nBlockAge = 0;

            /* Initialize process controls for this minting round */
            fRestart = false;
            nPoolSize = 0;

            /* Initialize stake pool values for this round */
            uint64_t nTimeBegin;
            uint64_t nTimeEnd;
            uint256_t hashProof;
            uint64_t nPoolReward;

            /* Create the block to work on */
            block = TritiumBlock();

            /* Populate the new block. */
            {
                /* Lock mutex so only one thread at a time may create a block. Assures all minters use a copy of the same block */
                LOCK(TAO::Ledger::STAKING_MUTEX);

                if(!TAO::Ledger::CreateStakeBlock(block))
                    return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                        "Unable to create candidate block");

            }

            /* Create a new producer transaction. */
            TAO::Ledger::Transaction newProducer;
            if(!TAO::Ledger::CreateTransaction(session.GetAccount(), session.GetActivePIN()->PIN(), newProducer))
                return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                    "Failed to create producer transaction");

            /* Save the new producer transactions (this is added as last producer in block.vProducer by AddPoolCoinstakes()) */
            txProducer = newProducer;

            /* Update the producer timestamp */
            TAO::Ledger::UpdateProducerTimestamp(txProducer);

            /* Get block previous to our candidate. */
            BlockState statePrev = BlockState();
            if(!LLD::Ledger->ReadBlock(block.hashPrevBlock, statePrev))
                return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                    "Failed to get previous block");

            /* Calculate the pool coinstake proofs for the current block */
            if(!GetStakeProofs(block, statePrev, nTimeBegin, nTimeEnd, hashProof))
                return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                    "Failed to get pooled stake proofs");

            if(!fGenesis)
            {
                /* Staking Trust for existing trust account */
                uint64_t nTrustPrev  = account.get<uint64_t>("trust");
                uint64_t nStake      = account.get<uint64_t>("stake");
                int64_t nStakeChange = 0;

                /* Get the previous stake tx for the trust account. */
                uint512_t hashLast;
                if(!FindLastStake(hashLast))
                    return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                        "Failed to get last stake for trust account");

                /* Find a stake change request. */
                if(!FindStakeChange(hashLast))
                {
                    /* Failed to retrieve stake change request. Process with no request. */
                    fStakeChange = false;
                    stakeChange.SetNull();
                }

                /* Set change amount using the stake change from database. */
                if(fStakeChange)
                    nStakeChange = stakeChange.nAmount;

                /* Check for available stake. */
                if(nStake == 0 && nStakeChange == 0)
                {
                    const uint64_t nCurrentTime = runtime::timestamp();

                    /* Update log every 5 minutes */
                    if((nCurrentTime - nLogTime) >= 300)
                    {
                        debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                   "Trust account has no stake.");

                        nLogTime = nCurrentTime;
                        nWaitReason = 1;
                    }

                    return false;
                }
                else if(nWaitReason == 1)
                {
                    /* Reset log time */
                    nLogTime = 0;
                    nWaitReason = 0;
                }

                /* Get the block containing the last stake tx for the trust account. */
                stateLast = BlockState();
                if(!LLD::Ledger->ReadBlock(hashLast, stateLast))
                    return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                        "Failed to get last block for trust account");

                /* Calculate time since last stake block (block age = age of previous stake block at time of current stateBest). */
                nBlockAge = statePrev.GetBlockTime() - stateLast.GetBlockTime();

                /* Check for previous version 7 and current version 8. */
                uint64_t nTrustRet = 0;
                if(block.nVersion == 8 && stateLast.nVersion == 7 && !CheckConsistency(hashLast, nTrustRet))
                    nTrust = GetTrustScore(nTrustRet, nBlockAge, nStake, nStakeChange, block.nVersion);
                else //when not consistency check, calculate like normal
                    nTrust = GetTrustScore(nTrustPrev, nBlockAge, nStake, nStakeChange, block.nVersion);

                /* Initialize Trust operation for block producer.
                 * This operation will have the pool reward added before sending it to the stake pool.
                 * If this is the block finder, the coinstake reward will be added based on time when block is found.
                 */
                txProducer[0] << uint8_t(TAO::Operation::OP::TRUSTPOOL) << hashLast << hashProof << nTimeBegin << nTimeEnd
                              << nTrust << nStakeChange;

                /* Final block time won't be known until it is mined, so pool trust reward based on block age */
                nPoolReward = GetCoinstakeReward(account.get<uint64_t>("stake"), nBlockAge, nTrust);
            }
            else
            {
                /* Looking to stake Genesis for new trust account */

                /* We don't need to clear transactions from pooled Genesis block because it does not hash. */

                /* Validate that have assigned balance for Genesis */
                if(account.get<uint64_t>("balance") == 0)
                {
                    const uint64_t nCurrentTime = runtime::timestamp();

                    /* Update log every 5 minutes */
                    if((nCurrentTime - nLogTime) >= 300)
                    {
                        debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                   "Trust account has no balance for Genesis.");

                        nLogTime = nCurrentTime;
                        nWaitReason = 2;
                    }

                    return false;
                }
                else if(nWaitReason == 2)
                {
                    /* Reset log time */
                    nLogTime = 0;
                    nWaitReason = 0;
                }

                /* Pending stake change request not allowed while staking Genesis */
                fStakeChange = false;
                if(LLD::Local->ReadStakeChange(session.GetAccount()->Genesis(), stakeChange))
                {
                    debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                               "Stake change request not allowed for Genesis...removing");

                    if(!LLD::Local->EraseStakeChange(session.GetAccount()->Genesis()))
                        debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                     "Failed to remove stake change request");
                }

                /* Initialize Genesis operation for block producer.
                 * Pooled Genesis does not hash, so this is only used for submitting to stake pool.
                 */
                txProducer[0] << uint8_t(TAO::Operation::OP::GENESISPOOL) << hashProof << nTimeBegin << nTimeEnd;

                /* Final block time won't be known until it is mined, so pool genesis reward based on age at prior block time */
                const uint64_t nAge = statePrev.GetBlockTime() - account.nModified;

                nPoolReward = GetCoinstakeReward(account.get<uint64_t>("balance"), nAge, 0, true);
            }

            /* Check if ok to submit to pool. Actual age and interval checks are in hashing phase after CalculateWeights.
             * This allows metrics to be updated for display.
             */
            bool fSubmitToPool = false;
            if(!fGenesis && (block.nHeight - stateLast.nHeight) > MinStakeInterval(block))
                fSubmitToPool = true;

            else if(fGenesis && (block.GetBlockTime() - account.nModified) >= MinCoinAge())
                fSubmitToPool = true;

            /* Submit the new coinstake to the stake pool and relay it to the network */
            if(fSubmitToPool)
            {
                /* Adjust pool reward for fee */
                nPoolReward = nPoolReward - GetPoolStakeFee(nPoolReward);

                /* Setup the pool coinstake and sign */
                TAO::Ledger::Transaction txPool = txProducer;
                txPool[0] << nPoolReward;

                /* Execute operation pre- and post-state. */
                if(!txPool.Build())
                    return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                        "Coinstake transaction failed to build");

                /* Sign pool tx and submit to pool */
                txPool.Sign(session.GetAccount()->Generate(txPool.nSequence, session.GetActivePIN()->PIN()));

                /* Setup the pooled stake parameters for current mining round */
                if(!SetupPool())
                    return false;

                {
                    /* Lock mutex so only one thread at a time may modify stake pool parameters. */
                    LOCK(TAO::Ledger::STAKING_MUTEX);

                    /* Record data for current block being worked on into the stake pool.
                     * This only needs to be done once per mining round (all sessions use same hash proofs), so check if already done
                     */
                    if(!TAO::Ledger::stakepool.HasProofs())
                        TAO::Ledger::stakepool.SetProofs(hashLastBlock, hashProof, nTimeBegin, nTimeEnd);
                }

                /* Put coinstake from this session into local pool for possible use by other sessions. */
                if(!TAO::Ledger::stakepool.Accept(txPool))
                    return false;

                /* Relay this node's coinstake to the network for pooled staking */
                if(!config::fShutdown.load())
                    LLP::TRITIUM_SERVER->Relay
                    (
                        LLP::Tritium::ACTION::NOTIFY,
                        uint8_t(LLP::Tritium::SPECIFIER::POOLSTAKE),
                        uint8_t(LLP::Tritium::TYPES::TRANSACTION),
                        txPool.GetHash(),
                        nTTL
                    );
            }

            /* Do not sign producer transaction, yet. Coinstake reward must be added to operation first. */

            /* Update display stake rate */
            nStakeRate.store(StakeRate(nTrust, fGenesis));

            return true;
        }


        /* Initialize the staking process for solo Proof of Stake and call HashBlock() to perform block hashing */
        bool StakeMinterPooled::MintBlock()
        {
            /* This are checked here before minting and after setting up the block/calculating weights so that all staking metrics
             * continue to be updated during a wait. This way, anyone who retrieves and display metrics will see them
             * change appropriately. For example, they will see block weight reset after minting a block.
             */

            /* Check whether this is a non-hashing node */
            if(fGenesis)
                return false;

            /* Skip trust minting until minimum interval requirement met */
            else if(!CheckInterval())
                return false;

            /* If the block for this stake minter is in a state that must be restarted (example, stale), stop hashing.
             * The next setup phase for this minter will create a new block.
             */
            if(fRestart)
                return false;

            /* Since only hash trust for pool, nStake is always current stake amount */
            uint64_t nStake = account.get<uint64_t>("stake");

            /* Include added stake into the amount for threshold calculations. */
            if(fStakeChange && stakeChange.nAmount > 0)
                nStake += stakeChange.nAmount;

            /* Check if the pool has received more transactions.
             * If pool size has increased 20% or more, add more coinstakes to this block.
             * This also updates nPoolStake with usable balance from pool for calculating required threshold
             * and nPoolFee with the net fee amount received from the pool.
             *
             * Note that nPoolSize starts at zero, so this will add coinstakes the first time it sees any in pool
             */
            uint32_t nSize = TAO::Ledger::stakepool.Size();

            if(nSize > 0 && (nSize - nPoolSize) >= (nPoolSize / 5))
            {
                if(!AddPoolCoinstakes(nStake))
                {
                    debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                 "Unable to add pooled coinstakes. Staking without them");

                    nPoolStake = 0;
                    nPoolFee = 0;
                    nPoolSize = 0;

                    block.vProducer.clear();
                    block.vProducer.push_back(txProducer);
                }
            }

            /* Calculate the minimum Required Energy Efficiency Threshold.
             * Minter can only mine Proof of Stake when current threshold exceeds this value.
             */
            cv::softdouble nRequired = GetRequiredThreshold(cv::softdouble(nTrustWeight.load()),
                                                            cv::softdouble(nBlockWeight.load()), (nStake + nPoolStake));

            /* Log when starting new block */
            if(block.nNonce == 1)
                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Staking new pool block from ", hashLastBlock.SubString(),
                           " at weight ", (nTrustWeight.load() + nBlockWeight.load()),
                           " and stake rate ", nStakeRate.load());

            HashBlock(nRequired);

            return true;
        }


        /* Check that producer coinstake won't orphan any new transaction for the same hashGenesis */
        bool StakeMinterPooled::CheckStale()
        {
            /* Perform the stale check for every coinstake included in the block. Whole block is stale if any one tx is */
            for(const TAO::Ledger::Transaction& producer : block.vProducer)
            {
                TAO::Ledger::Transaction tx;

                bool fTrust = producer.IsTrustPool();

                /* Trust tx allowed to have tx in mempool if it won't be orphaned */
                if(fTrust && TAO::Ledger::mempool.Get(producer.hashGenesis, tx) && producer.hashPrevTx != tx.GetHash())
                    return true;

                /* Genesis tx user cannot have any tx in mempool */
                else if (!fTrust && TAO::Ledger::mempool.Has(producer.hashGenesis))
                    return true;
            }

            return false;
        }


        /* Calculate the block finder coinstake reward for a pool mined Proof of Stake block. */
        uint64_t StakeMinterPooled::CalculateCoinstakeReward(uint64_t nTime)
        {
            uint64_t nReward = 0;

            /* Calculate the coinstake reward.
             * Block finder reward is based on final block time for block.
             */
            if(!fGenesis)
            {
                /* Trust reward based on stake amount, new trust score after block found, and time since last stake block.
                 * Note that, while nRequired for hashing includes any stake amount added by a stake change, the reward
                 * only includes the current stake amount.
                 */
                const uint64_t nTimeStake = nTime - stateLast.GetBlockTime();

                nReward = GetCoinstakeReward(account.get<uint64_t>("stake"), nTimeStake, nTrust);
            }
            else
            {
                /* Should never get here with genesis in pooled stake minter, as genesis does not hash */
                debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                             "Invalid request for genesis block finder reward in pooled staking.");

                nReward = 0;
            }

            return nReward + nPoolFee; //Add fees from pool to net reward
        }


        /* Setup pooled stake parameters. */
        bool StakeMinterPooled::SetupPool()
        {
            /* Determine the maximum number of coinstakes to include based on block age */
            nProducerSize = config::fTestNet.load() ? TAO::Ledger::POOL_MAX_TX_BASE_TESTNET
                                                    : TAO::Ledger::POOL_MAX_TX_BASE;

            /* After block age exceeds 2 hours (15 min for testnet), start bumping up producer size by one every hour (15 min) */
            if(config::fTestNet.load() && nBlockAge >= 900)
                nProducerSize = std::min(nProducerSize + ((nBlockAge - 900) / 900), TAO::Ledger::POOL_MAX_TX);

            else if(nBlockAge >= 7200)
                nProducerSize = std::min(nProducerSize + ((nBlockAge - 3600) / 3600), TAO::Ledger::POOL_MAX_TX);

            {
                /* Lock mutex so only one thread at a time may modify stake pool parameters. */
                LOCK(TAO::Ledger::STAKING_MUTEX);

                /* Get the current maximum stake pool size (0 for a new mining round) */
                const uint32_t nSizeMax = TAO::Ledger::stakepool.GetMaxSize();
                uint32_t nSizeNew = nSizeMax;

                /* For a new mining round, start pool at base size */
                if(nSizeMax == 0)
                    nSizeNew = config::fTestNet.load() ? TAO::Ledger::POOL_MAX_SIZE_BASE_TESTNET
                                                       : TAO::Ledger::POOL_MAX_SIZE_BASE_TESTNET;

                /* If current producer size exceeds 60% of current pool size, bump up the pool size
                 * For multisession staking, this sets the pool size to the maximum needed across all sessions.
                 */
                if((nProducerSize * 10 / 6) > nSizeMax)
                    nSizeNew += config::fTestNet.load() ? TAO::Ledger::POOL_MAX_SIZE_INCREMENT_TESTNET
                                                        : TAO::Ledger::POOL_MAX_SIZE_INCREMENT;

                /* Update max size setting if it has changed */
                if(nSizeNew > nSizeMax)
                {
                    TAO::Ledger::stakepool.SetMaxSize(nSizeNew);
                    debug::log(2, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                               "Stake pool size set to ", nSizeMax);
                }
            }

            return true;
        }


        /* Add coinstake transactions from the stake pool to the current candidate block */
        bool StakeMinterPooled::AddPoolCoinstakes(const uint64_t& nStake)
        {
            std::vector<uint512_t> vHashes;

            /* Save current pool size */
            nPoolSize = TAO::Ledger::stakepool.Size();

            block.vProducer.clear();

            /* Perform coinstake selection on pool to choose which coinstakes to use.
             * This is repeated every time new coinstakes are processed.
             *
             * Passing in txProducer hash ensures that pool coinstake from this session is not selected.
             */
            if(TAO::Ledger::stakepool.Select(vHashes, nPoolStake, nPoolFee, nStake, txProducer.GetHash(), nProducerSize))
            {
                for(const uint512_t& hashTx : vHashes)
                {
                    TAO::Ledger::Transaction tx;

                    if(!TAO::Ledger::stakepool.Get(hashTx, tx))
                        return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                            "Could not retrieve coinstake from stake pool");

                    block.vProducer.push_back(tx);
                }
            }

            /* Push our producer last */
            block.vProducer.push_back(txProducer);

            debug::log(2, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                       "Added coinstake transactions to pooled stake block from stake pool");

            return true;
        }

    }
}
