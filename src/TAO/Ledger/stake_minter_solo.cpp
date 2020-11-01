/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/Ledger/types/stake_minter_solo.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/session.h>
#include <TAO/API/include/sessionmanager.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/types/mempool.h>

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

        /** Constructor **/
        StakeMinterSolo::StakeMinterSolo(uint256_t& sessionId)
        : StakeMinter(sessionId)
        {}


        /* Creates a new solo Proof of Stake block */
        bool StakeMinterSolo::CreateCandidateBlock()
        {
            /* Reset any prior value of trust score and block age */
            nTrust = 0;
            nBlockAge = 0;
            fStakeChange = false;

            /* Initialize process controls for this minting round */
            fRestart = false;

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

            /* Create the producer transaction. */
            TAO::Ledger::Transaction txProducer;
            if(!TAO::Ledger::CreateTransaction(session.GetAccount(), session.GetActivePIN()->PIN(), txProducer))
                return debug::error(FUNCTION,(config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                    "Failed to create producer transaction");

            /* Update the producer timestamp */
            TAO::Ledger::UpdateProducerTimestamp(txProducer);

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

                /* Get block previous to our candidate. */
                BlockState statePrev = BlockState();
                if(!LLD::Ledger->ReadBlock(block.hashPrevBlock, statePrev))
                    return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                        "Failed to get previous block");

                /* Calculate time since last stake block (block age = age of previous stake block at time of current stateBest). */
                nBlockAge = statePrev.GetBlockTime() - stateLast.GetBlockTime();

                /* Check for previous version 7 and current version 8. */
                uint64_t nTrustRet = 0;
                if(block.nVersion == 8 && stateLast.nVersion == 7 && !CheckConsistency(hashLast, nTrustRet))
                    nTrust = GetTrustScore(nTrustRet, nBlockAge, nStake, nStakeChange, block.nVersion);
                else //when not consistency check, calculate like normal
                    nTrust = GetTrustScore(nTrustPrev, nBlockAge, nStake, nStakeChange, block.nVersion);

                /* Initialize Trust operation for block producer.
                 * The coinstake reward will be added based on time when block is found.
                 */
                txProducer[0] << uint8_t(TAO::Operation::OP::TRUST) << hashLast << nTrust << nStakeChange;
            }
            else
            {
                /* Looking to stake Genesis for new trust account */

                /* A solo Genesis block does not include transactions. */
                block.vtx.clear();

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
                 * The coinstake reward will be added based on time when block is found.
                 */
                txProducer[0] << uint8_t(TAO::Operation::OP::GENESIS);

            }

            /* Add the producer transaction to the block */
            if(block.nVersion < 9)
                block.producer = txProducer;
            else
            {
                block.vProducer.clear();
                block.vProducer.push_back(txProducer);
            }

            /* Do not sign producer transaction, yet. Coinstake reward must be added to operation first. */

            /* Update display stake rate */
            nStakeRate.store(StakeRate(nTrust, fGenesis));

            return true;
        }


        /* Initialize the staking process for solo Proof of Stake and call HashBlock() to perform block hashing */
        bool StakeMinterSolo::MintBlock()
        {
            /* These are checked here in the hashing phase so that all staking metrics (calculated in setup phase)
             * continue to be updated during an interval wait. This way, anyone who retrieves and display metrics will see them
             * change appropriately. For example, they will see block weight reset after minting a block.
             */

            /* Skip trust minting until minimum interval requirement met */
            if(!fGenesis && !CheckInterval())
                return false;

            /* Skip genesis minting if sig chain has transaction in the mempool */
            else if (fGenesis && !CheckMempool())
                return false;

            /* If the block for this stake minter is in a state that must be restarted (example, stale), stop hashing.
             * The next setup phase for this minter will create a new block.
             */
            if(fRestart)
                return false;

            /* When ok to stake, calculate the required threshold for solo staking and pass to hashing method */
            uint64_t nStake;
            if (fGenesis)
            {
                /* For Genesis, stake amount is the trust account balance. */
                nStake = account.get<uint64_t>("balance");
            }
            else
            {
                nStake = account.get<uint64_t>("stake");

                /* Include added stake into the amount for threshold calculations. Need this because, if not included and someone
                 * unstaked their full stake amount, their trust account would be stuck unable to ever stake another block.
                 * By allowing minter to proceed when adding stake to a zero stake trust account, it must be added in here also
                 * or there would be no stake amount (required threshold calculation would fail).
                 *
                 * For other cases, where more is added to existing stake, we also include it as an immediate benefit
                 * to improve the chances to stake the block that implements the change. If not, low balance accounts could
                 * potentially have difficulty finding a block to add stake, even if they were adding a large amount.
                 */
                if(fStakeChange && stakeChange.nAmount > 0)
                    nStake += stakeChange.nAmount;
            }

            /* Calculate the minimum Required Energy Efficiency Threshold.
             * Minter can only mine Proof of Stake when current threshold exceeds this value.
             */
            cv::softdouble nRequired =
                GetRequiredThreshold(cv::softdouble(nTrustWeight.load()), cv::softdouble(nBlockWeight.load()), nStake);

            /* Verify we still should hash */
            if(hashLastBlock != TAO::Ledger::ChainState::hashBestChain.load())
                return false;

            if(block.nNonce == 1)
                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Staking new block from ", hashLastBlock.SubString(),
                           " at weight ", (nTrustWeight.load() + nBlockWeight.load()),
                           " and stake rate ", nStakeRate.load());

            HashBlock(nRequired);

            return true;
        }


        /* Check that producer coinstake won't orphan any new transaction for the same hashGenesis */
        bool StakeMinterSolo::CheckStale()
        {
            TAO::Ledger::Transaction tx;
            TAO::Ledger::Transaction txProducer;

            if(block.nVersion < 9)
                txProducer = block.producer;

            else
                txProducer = block.vProducer[0]; //only one producer for solo stake

            if(mempool.Get(txProducer.hashGenesis, tx) && txProducer.hashPrevTx != tx.GetHash())
                return true;

            return false;
        }


        /* Calculate the coinstake reward for a solo mined Proof of Stake block */
        uint64_t StakeMinterSolo::CalculateCoinstakeReward(uint64_t nTime)
        {
            uint64_t nReward = 0;

            if(!fGenesis)
            {
                /* Trust reward based on current stake amount, new trust score, and time since last stake block. */
                const uint64_t nTimeLast = stateLast.GetBlockTime();

                if(nTime < nTimeLast)
                    nTime = nTimeLast;

                const uint64_t nTimeStake = nTime - nTimeLast;

                nReward = GetCoinstakeReward(account.get<uint64_t>("stake"), nTimeStake, nTrust);
            }
            else
            {
                /* Genesis reward based on trust account balance and coin age based on trust register timestamp. */
                if(nTime < account.nModified)
                    nTime = account.nModified;

                const uint64_t nAge = nTime - account.nModified;

                nReward = GetCoinstakeReward(account.get<uint64_t>("balance"), nAge, 0, true); //pass true for Genesis
            }

            return nReward;
        }

    }
}
