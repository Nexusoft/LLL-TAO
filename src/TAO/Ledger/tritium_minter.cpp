/*__________________________________________________________________________________________

        (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

        (c) Copyright The Nexus Developers 2014 - 2018

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/Ledger/types/tritium_minter.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <cmath>


/* Global TAO namespace. */
namespace TAO::Ledger
{
    /* Initialize static variables */
    std::atomic<bool> TritiumMinter::fStarted(false);

    std::thread TritiumMinter::stakeMinterThread;


    TritiumMinter& TritiumMinter::GetInstance()
    {
        static TritiumMinter tritiumMinter;

        return tritiumMinter;
    }


    /* Start the stake minter. */
    bool TritiumMinter::Start()
    {
        if(StakeMinter::fStarted.load())
            return debug::error(0, FUNCTION, "Attempt to start Stake Minter when one is already started.");

        /* Disable stake minter if not in sessionless mode. */
        if(config::fMultiuser.load())
        {
            debug::log(3, FUNCTION, "Stake minter disabled when use API sessions (multiuser).");
            return false;
        }

        /* Disable stake minter in client mode. */
        if(config::fClient.load())
        {
            debug::log(0, FUNCTION, "Stake minter disabled in client mode.");
            return false;
        }

        /* Check that stake minter is configured to run. */
        if(!config::fStaking.load())
        {
            debug::log(0, "Stake Minter not configured. Startup cancelled.");
            return false;
        }

        /* Stake minter does not run in private or hybrid mode (at least for now) */
        if(config::fHybrid.load())
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

        TritiumMinter::stakeMinterThread = std::thread(TritiumMinter::StakeMinterThread, this);

        StakeMinter::fStarted.store(true);   //base class flag indicates any stake minter started
        TritiumMinter::fStarted.store(true); //local class flag indicate a stake minter of type TritiumMinter started

        return true;
    }


    /* Stop the stake minter. */
    bool TritiumMinter::Stop()
    {
        if(TritiumMinter::fStarted.load()) //Only process Stop() in TritiumMinter if TritiumMinter is running
        {
            debug::log(0, FUNCTION, "Shutting down Stake Minter");

            /* Set signal flag to tell minter thread to stop */
            StakeMinter::fStop.store(true);

            /* Wait for minter thread to stop */
            TritiumMinter::stakeMinterThread.join();

            /* Update minter status */
            StakeMinter::fStop.store(false);
            StakeMinter::fStarted.store(false);
            TritiumMinter::fStarted.store(false);

            return true;
        }

        return false;
    }


    /* Create the coinstake transaction for a solo Proof of Stake block and add it as the candidate block producer */
    bool TritiumMinter::CreateCoinstake(const uint256_t& hashGenesis)
    {
        static uint32_t nCounter = 0; //Prevents log spam during wait period

        /* Reset any prior value of trust score and block age */
        nTrust = 0;
        nBlockAge = 0;

        /* Staking Trust for existing trust account */
        if(!fGenesis)
        {
            /* Get our total stake available. */
            int64_t nStakeChange = 0;

            /* Get the previous stake tx for the trust account. */
            uint512_t hashLast;
            if(!FindLastStake(hashGenesis, hashLast))
            {
                nSleepTime = 5000;
                return debug::error(FUNCTION, "Failed to get last stake for trust account");
            }

            /* Find a stake change request. */
            TAO::Ledger::StakeChange tStakeChange;
            if(CheckStakeChange(hashGenesis, hashLast, tStakeChange))
                nStakeChange = tStakeChange.nAmount; //we set our stake change if we have successfully sanitized request

            /* If we return false here, it indicates we have a bad stake change and need to erase it. */
            else
                LLD::Local->EraseStakeChange(hashGenesis);

            /* Check for available stake. */
            const uint64_t nStake = account.get<uint64_t>("stake");
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

            /* Get block previous to our candidate. */
            BlockState tStatePrev;
            if(!LLD::Ledger->ReadBlock(block.hashPrevBlock, tStatePrev))
                return debug::error(FUNCTION, "Failed to get previous block");

            /* Calculate time since last stake block (block age = age of previous stake block at time of current tStateBest). */
            nBlockAge = tStatePrev.GetBlockTime() - stateLast.GetBlockTime();

            /* Check for previous version 7 and current version 8. */
            const uint64_t nTrustPrev  =
                account.get<uint64_t>("trust");

            /* Calculate our new trust score now. */
            nTrust =
                GetTrustScore(nTrustPrev, nBlockAge, nStake, nStakeChange, block.nVersion);

            /* Build our staking contract for TRUST operator. */
            block.producer[0] << uint8_t(TAO::Operation::OP::TRUST) << hashLast << nTrust << nStakeChange;
        }
        else
        {
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
            TAO::Ledger::StakeChange tStakeChange;
            if(LLD::Local->ReadStakeChange(hashGenesis, tStakeChange))
            {
                debug::log(0, FUNCTION, "Stake change request not allowed for trust account Genesis...removing");
                LLD::Local->EraseStakeChange(hashGenesis);
            }

            /* Build our staking contract for GENESIS operator*/
            block.producer[0] << uint8_t(TAO::Operation::OP::GENESIS);
        }

        /* Reset sleep time on successful completion */
        if(nSleepTime == 5000)
        {
            nSleepTime = 1000;
            nCounter = 0;
        }

        /* Update display stake rate */
        nStakeRate.store(StakeRate(nTrust, fGenesis));

        return true;
    }


    /* Initialize the staking process for solo Proof of Stake and call HashBlock() to perform block hashing */
    void TritiumMinter::MintBlock(const uint256_t& hashGenesis)
    {
        /* This are checked here before minting and after setting up the block/calculating weights so that all staking metrics
         * continue to be updated during a wait. This way, anyone who retrieves and display metrics will see them
         * change appropriately. For example, they will see block weight reset after minting a block.
         */

        /* Skip trust minting until minimum interval requirement met */
        if(!fGenesis && !CheckInterval())
            return;

        /* Skip genesis minting if sig chain has transaction in the mempool */
        else if (fGenesis && !CheckMempool(hashGenesis))
            return;

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
            TAO::Ledger::StakeChange tStakeChange;
            if(CheckStakeChange(hashGenesis, stateLast.GetHash(), tStakeChange))
                nStake += tStakeChange.nAmount;
        }

        /* Calculate the minimum Required Energy Efficiency Threshold.
         * Minter can only mine Proof of Stake when current threshold exceeds this value.
         */
        const cv::softdouble nRequired =
            GetRequiredThreshold(cv::softdouble(nTrustWeight.load()), cv::softdouble(nBlockWeight.load()), nStake);

        debug::log(0, FUNCTION, "Staking new block from ", hashLastBlock.SubString(),
                                " at weight ", (nTrustWeight.load() + nBlockWeight.load()),
                                " and stake rate ", nStakeRate.load());

        /* Engage in the staking process pure nakamoto style. */
        HashBlock(nRequired);

        return;
    }


    /* Check whether or not to stop hashing in HashBlock() to process block updates */
    bool TritiumMinter::CheckBreak()
    {
        /* Solo stake minter will never need to alter the block during hashing */
        return false;
    }


    /* Check that producer coinstake won't orphan any new transaction for the same hashGenesis */
    bool TritiumMinter::CheckStale()
    {
        TAO::Ledger::Transaction tx;
        if(mempool.Get(block.producer.hashGenesis, tx) && block.producer.hashPrevTx != tx.GetHash())
            return true;

        return false;
    }


    /* Calculate the coinstake reward for a solo mined Proof of Stake block */
    uint64_t TritiumMinter::CalculateCoinstakeReward(uint64_t nTime)
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


    /* Method run on its own thread to oversee stake minter operation. */
    void TritiumMinter::StakeMinterThread(TritiumMinter* pTritiumMinter)
    {

        debug::log(0, FUNCTION, "Stake Minter Started");
        pTritiumMinter->nSleepTime = 5000;
        bool fLocalTestnet = config::fTestNet.load() && !config::GetBoolArg("-dns", true);

        /* If the system is still syncing/connecting on startup, wait to run minter.
         * Local testnet does not wait for connections.
         */
        while((TAO::Ledger::ChainState::Synchronizing() || (LLP::TRITIUM_SERVER->GetConnectionCount() == 0 && !fLocalTestnet))
                && !StakeMinter::fStop.load() && !config::fShutdown.load())
        {
            runtime::sleep(pTritiumMinter->nSleepTime);
        }

        /* Check stop/shutdown status after sync/connect wait ends. */
        if(StakeMinter::fStop.load() || config::fShutdown.load())
        {
            /* If get here because fShutdown set, wait for join. Stop issues join, and must be called during shutdown */
            while(!StakeMinter::fStop.load())
                runtime::sleep(100);

            return;
        }

        debug::log(0, FUNCTION, "Stake Minter Initialized");

        pTritiumMinter->nSleepTime = 1000;

        /* Minting thread will continue repeating this loop until stop minter or shutdown */
        while(!StakeMinter::fStop.load() && !config::fShutdown.load())
        {
            /* Sleep the appropriate amount of time before next iteration */
            runtime::sleep(pTritiumMinter->nSleepTime);

            /* Check status again after wake from sleep */
            if(StakeMinter::fStop.load() || config::fShutdown.load())
                break;

            /* Save the current best block hash immediately in case it changes while we do setup */
            pTritiumMinter->hashLastBlock = TAO::Ledger::ChainState::hashBestChain.load();

            /* Check that user account still unlocked for minting (locking should stop minter, but still verify) */
            if(!pTritiumMinter->CheckUser())
                break;

            /* Get our current genesis-id. */
            const uint256_t hashGenesis =
                TAO::API::Authentication::Caller();

            /* Retrieve the latest trust account data */
            if(!pTritiumMinter->FindTrustAccount(hashGenesis))
                break;

            /* Set up the candidate block the minter is attempting to mine */
            if(!pTritiumMinter->CreateCandidateBlock())
                continue;

            /* Updates weights for new candidate block */
            if(!pTritiumMinter->CalculateWeights())
                continue;

            /* Attempt to mine the current proof of stake block */
            pTritiumMinter->MintBlock(hashGenesis);
        }

        /* If break because cannot continue (error retrieving user account or FindTrust failed), wait for stop or shutdown */
        while(!StakeMinter::fStop.load() && !config::fShutdown.load())
            runtime::sleep(pTritiumMinter->nSleepTime);

        /* If get here because fShutdown set, wait for join. Stop issues join, and must be called during shutdown */
        while(!StakeMinter::fStop.load())
            runtime::sleep(100);

        /* Stop has been issued. Now thread can end. */
    }
}
