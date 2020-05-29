/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/Ledger/types/tritium_pool_minter.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>

#include <TAO/API/include/global.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
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

        /* Initialize static variables */
        std::atomic<bool> TritiumPoolMinter::fStarted(false);

        std::thread TritiumPoolMinter::stakeMinterThread;


        TritiumPoolMinter& TritiumPoolMinter::GetInstance()
        {
            static TritiumPoolMinter tritiumMinter;

            return tritiumMinter;
        }


        /* Constructor */
        TritiumPoolMinter::TritiumPoolMinter()
        : txProducer()
        , nProducerSize(0)
        , nPoolCount(0)
        , nPoolStake(0)
        , nPoolFee(0)
        {
        }


        /* Start the stake minter. */
        bool TritiumPoolMinter::Start()
        {
            if(StakeMinter::fStarted.load())
                return debug::error(0, FUNCTION, "Attempt to start Stake Minter when one is already started.");

            /* Disable stake minter if not in sessionless mode. */
            if(config::fMultiuser.load())
            {
                debug::log(0, FUNCTION, "Stake minter disabled when use API sessions (multiuser).");
                return false;
            }

            /* Check that stake minter is configured to run. */
            if(!config::fPoolStaking.load())
            {
                debug::log(0, "Pooled Stake Minter not configured. Startup cancelled.");
                return false;
            }

            /* Stake minter does not run in private or hybrid mode (at least for now) */
            if(config::GetBoolArg("-private") || config::GetBoolArg("-hybrid"))
            {
                debug::log(0, "Stake Minter does not run in private/hybrid mode. Startup cancelled.");
                return false;
            }

            /* Verify that account is unlocked and can mint. */
            if(!CheckUser())
            {
                debug::log(0, FUNCTION, "User account not unlocked for staking. Stake minter not started.");
                return false;
            }

            /* If still in process of stopping a previous thread, wait */
            while(StakeMinter::fStop.load())
                runtime::sleep(100);

            TritiumPoolMinter::stakeMinterThread = std::thread(TritiumPoolMinter::StakeMinterThread, this);

            StakeMinter::fStarted.store(true);       //base class flag indicates any stake minter started
            TritiumPoolMinter::fStarted.store(true); //local class flag indicate a stake minter of type TritiumPoolMinter started

            return true;
        }


        /* Stop the stake minter. */
        bool TritiumPoolMinter::Stop()
        {
            if(TritiumPoolMinter::fStarted.load()) //Only process Stop() in TritiumPoolMinter if TritiumPoolMinter is running
            {
                debug::log(0, FUNCTION, "Shutting down Pooled Stake Minter");

                /* Set signal flag to tell minter thread to stop */
                StakeMinter::fStop.store(true);

                /* Wait for minter thread to stop */
                TritiumPoolMinter::stakeMinterThread.join();

                /* Reset internals */
                hashLastBlock = 0;
                nSleepTime = 1000;
                fWait.store(false);
                nWaitTime.store(0);
                nTrustWeight.store(0.0);
                nBlockWeight.store(0.0);
                nStakeRate.store(0.0);

                account = TAO::Register::Object();
                stateLast = BlockState();
                fStakeChange = false;
                stakeChange = StakeChange();
                block = TritiumBlock();
                fGenesis = false;
                nTrust = 0;
                nBlockAge = 0;

                txProducer = Transaction();

                /* Update minter status */
                StakeMinter::fStop.store(false);
                StakeMinter::fStarted.store(false);
                TritiumPoolMinter::fStarted.store(false);

                return true;
            }

            return false;
        }


        /* Create the coinstake transaction for a pooled Proof of Stake block and add it as the candidate block producer */
        bool TritiumPoolMinter::CreateCoinstake(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user,
                                                const SecureString& strPIN)
        {
            static uint32_t nCounter = 0; //Prevents log spam during wait period

            /* Reset any prior value of trust score and block age */
            nTrust = 0;
            nBlockAge = 0;
            uint64_t nTimeBegin;
            uint64_t nTimeEnd;
            uint256_t hashProof;
            uint64_t nPoolReward;

            /* Pooled staking keeps producer transaction separate from block so it can add it last after other pooled coinstakes */
            txProducer = block.vProducer[0];
            block.vProducer.clear();

            /* Get block previous to our candidate. */
            BlockState statePrev = BlockState();
            if(!LLD::Ledger->ReadBlock(block.hashPrevBlock, statePrev))
                return debug::error(FUNCTION, "Failed to get previous block");

            /* Calculate the pool coinstake proofs for the current block */
            BlockState stateCurrent(block);
            if(!GetStakeProofs(stateCurrent, statePrev, nTimeBegin, nTimeEnd, hashProof))
                return debug::error(FUNCTION, "Failed to get pooled stake proofs");

            if(!fGenesis)
            {
                /* Staking Trust for existing trust account */
                uint64_t nTrustPrev  = account.get<uint64_t>("trust");
                uint64_t nStake      = account.get<uint64_t>("stake");
                int64_t nStakeChange = 0;

                /* Get the previous stake tx for the trust account. */
                uint512_t hashLast;
                if(!FindLastStake(user->Genesis(), hashLast))
                {
                    nSleepTime = 5000;
                    return debug::error(FUNCTION, "Failed to get last stake for trust account");
                }

                /* Find a stake change request. */
                if(!FindStakeChange(user->Genesis(), hashLast))
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
                    /* Trust account has no stake balance. Increase sleep time to wait for balance. */
                    nSleepTime = 5000;

                    /* Update log every 60 iterations (5 minutes) */
                    if((++nCounter % 60) == 0)
                        debug::log(0, FUNCTION, "Trust account has no stake.");

                    return false;
                }

                /* Get the block containing the last stake tx for the trust account. */
                stateLast = BlockState();
                if(!LLD::Ledger->ReadBlock(hashLast, stateLast))
                    return debug::error(FUNCTION, "Failed to get last block for trust account");

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
                txProducer[0] << uint8_t(TAO::Operation::OP::TRUSTPOOL) << hashLast << hashProof << nTimeBegin << nTimeEnd
                              << nTrust << nStakeChange;

                /* Final block time won't be known until it is mined, so pool trust reward based on block age */
                nPoolReward = GetCoinstakeReward(account.get<uint64_t>("stake"), nBlockAge, nTrust);
            }
            else
            {
                /* Looking to stake Genesis for new trust account */

                /* Validate that have assigned balance for Genesis */
                if(account.get<uint64_t>("balance") == 0)
                {
                    /* Wallet has no balance, or balance unavailable for staking. Increase sleep time to wait for balance. */
                    nSleepTime = 5000;

                    /* Update log every 60 iterations (5 minutes) */
                    if((nCounter % 60) == 0)
                        debug::log(0, FUNCTION, "Trust account has no balance for Genesis.");

                    ++nCounter;

                    return false;
                }

                /* Pending stake change request not allowed while staking Genesis */
                fStakeChange = false;
                if(LLD::Local->ReadStakeChange(user->Genesis(), stakeChange))
                {
                    debug::log(0, FUNCTION, "Stake change request not allowed for trust account Genesis...removing");

                    if(!LLD::Local->EraseStakeChange(user->Genesis()))
                        debug::error(FUNCTION, "Failed to remove stake change request");
                }

                /* Initialize Genesis operation for block producer.
                 * The coinstake reward will be added based on time when block is found.
                 */
                txProducer[0] << uint8_t(TAO::Operation::OP::GENESISPOOL) << hashProof << nTimeBegin << nTimeEnd;

                const uint64_t nAge = statePrev.GetBlockTime() - account.nModified;

                /* Final block time won't be known until it is mined, so pool genesis reward based on age at prior block time */
                nPoolReward = GetCoinstakeReward(account.get<uint64_t>("balance"), nAge, 0, true);
            }

            /* Do not sign producer transaction, yet. Coinstake reward must be added to operation first. */

            /* Reset sleep time on successful completion */
            if(nSleepTime == 5000)
            {
                nSleepTime = 1000;
                nCounter = 0;
            }

            /* Update display stake rate */
            nStakeRate.store(StakeRate(nTrust, fGenesis));

            /* Adjust pool reward for fee */
            nPoolReward = nPoolReward - CalculatePoolStakeFee(nPoolReward);

            /* Setup the pool coinstake and sign */
            TAO::Ledger::Transaction txPool = txProducer;
            txPool[0] << nPoolReward;

            /* Execute operation pre- and post-state. */
            if(!txPool.Build())
                return debug::error(FUNCTION, "Coinstake transaction failed to build");

            /* Sign pool tx and submit to pool */
            txPool.Sign(user->Generate(txPool.nSequence, strPIN));

            /* If user is within minimum interval, skip pool setup and submission.*/
            if(CheckInterval(user))
            {
                /* Setup the pooled stake parameters for current mining round */
                if(!SetupPool())
                    return false;

//TODO relay pooled coinstake to network
            // if(SubmitToPool(txPool))
            //     return debug::error(FUNCTION, "Unable to submit coinstake to stake pool");
            }

            return true;
        }


        /* Initialize the staking process for solo Proof of Stake and call HashBlock() to perform block hashing */
        void TritiumPoolMinter::MintBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user,
                                          const SecureString& strPIN)
        {
            /* This are checked here before minting and after setting up the block/calculating weights so that all staking metrics
             * continue to be updated during a wait. This way, anyone who retrieves and display metrics will see them
             * change appropriately. For example, they will see block weight reset after minting a block.
             */

            /* Check whether this is a non-hashing node */
            if(fGenesis || config::fClient.load())
            {
                /* When staking genesis via pool or in client mode, local node will not hash. Wait until next block. */
                while(!StakeMinter::fStop.load() && !config::fShutdown.load()
                      && hashLastBlock == TAO::Ledger::ChainState::hashBestChain.load())
                {
                    runtime::sleep(nSleepTime);
                }

            }

            /* Skip trust minting until minimum interval requirement met */
            else if(!CheckInterval(user))
            {
                return;
            }

            else
            {
                /* On initial iteration, start with pool state of zero (it will add coinstakes if any are present in pool) */
                nPoolCount = 0;

                uint64_t nStake = account.get<uint64_t>("stake");

                /* Include added stake into the amount for threshold calculations. */
                if(fStakeChange && stakeChange.nAmount > 0)
                    nStake += stakeChange.nAmount;

                /* Pool minter will hash in a loop. If HashBlock() returns false, the block needs to include additional
                 * coinstakes from the pool, then continue hashing. When current round is complete, HashBlock() returns
                 * true and the loop ends.
                 */
                while(true)
                {
                    /* Each iteration will add coinstakes from pool into current block and update nPoolStake, as appropriate. */
                    if(!AddPoolCoinstakes())
                        return;

                    /* Calculate the minimum Required Energy Efficiency Threshold.
                     * Minter can only mine Proof of Stake when current threshold exceeds this value.
                     */
                    cv::softdouble nRequired = GetRequiredThreshold(cv::softdouble(nTrustWeight.load()),
                                                                    cv::softdouble(nBlockWeight.load()), (nStake + nPoolStake));

                    /* Log when starting new block */
                    if(block.nNonce == 1)
                        debug::log(0, FUNCTION, "Staking new pool block from ", hashLastBlock.SubString(),
                                                " at weight ", (nTrustWeight.load() + nBlockWeight.load()),
                                                " and stake rate ", nStakeRate.load());
                    else
                        debug::log(2, FUNCTION, "Added coinstake transactions to pooled stake block from stake pool");

                    if(HashBlock(user, strPIN, nRequired))
                        break; //hashing complete for current block
                }
            }

            return;
        }


        /* Check whether or not to stop hashing in HashBlock() to process block updates */
        bool TritiumPoolMinter::CheckBreak()
        {
            /* Return true if stake pool size has increased by at least 10% */
            if((TAO::Ledger::stakepool.Size() - nPoolCount) >= (nPoolCount / 10))
                return true;

            return false;
        }


        /* Check that producer coinstake won't orphan any new transaction for the same hashGenesis */
        bool TritiumPoolMinter::CheckStale()
        {
            /* Perform the stale check for every coinstake included in the block. Whole block is stale if any one tx is */
            for(const TAO::Ledger::Transaction& producer : block.vProducer)
            {
                TAO::Ledger::Transaction tx;

                bool fTrust = TAO::Ledger::stakepool.IsTrust(producer.GetHash());

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
        uint64_t TritiumPoolMinter::CalculateCoinstakeReward(uint64_t nTime)
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
                debug::error(FUNCTION, "Invalid request for genesis reward calculation in pooled staking.");

                nReward = 0;
            }

            return nReward + nPoolFee; //Add fees from pool to net reward
        }


        /* Setup pooled stake parameters. */
        bool TritiumPoolMinter::SetupPool()
        {
            /* Determine the maximum number of coinstakes to include based on block age */
            nProducerSize = TAO::Ledger::POOL_MAX_TX_BASE;

            /* After block age exceeds 2 hours, start bumping up producer size by one every hour */
            if(nBlockAge >= 7200)
                nProducerSize = std::min(nProducerSize + ((nBlockAge - 7200) /3600), TAO::Ledger::POOL_MAX_TX);

            /* Determine the maximum stake pool size for current producer requirements */
            uint32_t nSizeMax = TAO::Ledger::stakepool.GetMaxSize();

            /* On pool startup or after find a block and reset producer size, then set the pool size to base */
            if(nSizeMax == 0 || nProducerSize == TAO::Ledger::POOL_MAX_TX_BASE)
                TAO::Ledger::stakepool.SetMaxSize(TAO::Ledger::POOL_MAX_SIZE_BASE);

            /* If current producer size exceeds 60% of current pool size, bump up the pool size */
            else if((nProducerSize * 10 / 6) > nSizeMax)
            {
                nSizeMax += TAO::Ledger::POOL_MAX_SIZE_INCREMENT;

                TAO::Ledger::stakepool.SetMaxSize(nSizeMax);
            }

            return true;
        }


        /* Add coinstake transactions from the stake pool to the current candidate block */
        bool TritiumPoolMinter::AddPoolCoinstakes()
        {
            std::vector<uint512_t> vHashes;

            /* Save current pool size */
            nPoolCount = TAO::Ledger::stakepool.Size();

            block.vProducer.clear();

            /* Perform coinstake selection on pool to choose which coinstakes to use.
             * This is repeated every time new coinstakes are processed. (or block fills to max with first transactions in pool)
             */
            if(TAO::Ledger::stakepool.Select(vHashes, nPoolStake, nPoolFee, nProducerSize))
            {
                for(const uint512_t& hashTx : vHashes)
                {
                    TAO::Ledger::Transaction tx;

                    if(!TAO::Ledger::stakepool.Get(hashTx, tx))
                        return debug::error(FUNCTION, "Could not retrieve coinstake from stake pool");

                    block.vProducer.push_back(tx);
                }
            }

            /* Push our producer last */
            block.vProducer.push_back(txProducer);

            return true;
        }


        /* Method run on its own thread to oversee stake minter operation. */
        void TritiumPoolMinter::StakeMinterThread(TritiumPoolMinter* pTritiumPoolMinter)
        {

            debug::log(0, FUNCTION, "Pooled Stake Minter Started");
            pTritiumPoolMinter->nSleepTime = 5000;
            bool fLocalTestnet = config::fTestNet.load() && !config::GetBoolArg("-dns", true);

            /* Pooled staking activates with block version 9 */
            if(runtime::unifiedtimestamp() < TAO::Ledger::StartBlockTimelock(9))
            {
                debug::error(FUNCTION, "Pooled staking not yet available");

                /* Suspend operation and wait for stop/shutdown */
                while(!StakeMinter::fStop.load() && !config::fShutdown.load())
                    runtime::sleep(pTritiumPoolMinter->nSleepTime);
            }

            /* If the system is still syncing/connecting on startup, wait to run minter.
             * Local testnet does not wait for connections.
             */
            while((TAO::Ledger::ChainState::Synchronizing() || (LLP::TRITIUM_SERVER->GetConnectionCount() == 0 && !fLocalTestnet))
                    && !StakeMinter::fStop.load() && !config::fShutdown.load())
            {
                runtime::sleep(pTritiumPoolMinter->nSleepTime);
            }

            /* Check stop/shutdown status after sync/connect wait ends. */
            if(StakeMinter::fStop.load() || config::fShutdown.load())
            {
                /* If get here because fShutdown set, wait for join. Stop issues join, and must be called during shutdown */
                while(!StakeMinter::fStop.load())
                    runtime::sleep(100);

                return;
            }

            /* Pool stake minter will likely start in the middle of a block round. Wait until that round ends before continuing */
            if(!StakeMinter::fStop.load() && !config::fShutdown.load())
            {
                debug::log(0, FUNCTION, "Pooled Stake Minter waiting for next block");
                pTritiumPoolMinter->nSleepTime = 200;
                pTritiumPoolMinter->hashLastBlock = TAO::Ledger::ChainState::hashBestChain.load();

                while(!StakeMinter::fStop.load() && !config::fShutdown.load() && !fLocalTestnet
                      && pTritiumPoolMinter->hashLastBlock == TAO::Ledger::ChainState::hashBestChain.load())
                {
                    runtime::sleep(pTritiumPoolMinter->nSleepTime);
                }
            }

            debug::log(0, FUNCTION, "Pooled Stake Minter Initialized");

            pTritiumPoolMinter->nSleepTime = 1000;

            /* Minting thread will continue repeating this loop until stop minter or shutdown */
            while(!StakeMinter::fStop.load() && !config::fShutdown.load())
            {
                /* Save the current best block hash immediately in case it changes while we do setup */
                pTritiumPoolMinter->hashLastBlock = TAO::Ledger::ChainState::hashBestChain.load();

                TAO::Ledger::stakepool.Clear();

                /* Check that user account still unlocked for minting (locking should stop minter, but still verify) */
                if(!pTritiumPoolMinter->CheckUser())
                    break;

                /* Get the active, unlocked sigchain. Requires session 0 */
                memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = TAO::API::users->GetAccount(0);
                if(!user)
                {
                    debug::error(0, FUNCTION, "Stake minter could not retrieve the unlocked signature chain.");
                    break;
                }

                SecureString strPIN = TAO::API::users->GetActivePin();

                /* Retrieve the latest trust account data */
                if(!pTritiumPoolMinter->FindTrustAccount(user->Genesis()))
                    break;

                /* Set up the candidate block the minter is attempting to mine */
                if(!pTritiumPoolMinter->CreateCandidateBlock(user, strPIN))
                    continue;

                /* Updates weights for new candidate block */
                if(!pTritiumPoolMinter->CalculateWeights())
                    continue;

                /* Attempt to mine the current proof of stake block */
                pTritiumPoolMinter->MintBlock(user, strPIN);

                runtime::sleep(pTritiumPoolMinter->nSleepTime);
            }

            /* If break because cannot continue (error retrieving user account or FindTrust failed), wait for stop or shutdown */
            while(!StakeMinter::fStop.load() && !config::fShutdown.load())
                runtime::sleep(pTritiumPoolMinter->nSleepTime);

            /* If get here because fShutdown set, wait for join. Stop issues join, and must be called during shutdown */
            while(!StakeMinter::fStop.load())
                runtime::sleep(100);

            /* Stop has been issued. Now thread can end. */
        }

    }
}
