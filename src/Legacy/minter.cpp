/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/include/create.h>
#include <Legacy/types/minter.h>
#include <Legacy/types/address.h>
#include <Legacy/wallet/addressbook.h>

#include <LLC/types/bignum.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/include/version.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/trust.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/tritium.h> //for LEGACY_TX enum

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/templates/serialize.h>


namespace Legacy
{

    /* Initialize static variables */
    bool StakeMinter::fisStarted = false;
    bool StakeMinter::fstopMinter = false;
    bool StakeMinter::fdestructMinter = false;

    std::mutex StakeMinter::cs_stakeMinter;

    std::thread StakeMinter::minterThread;


    StakeMinter& StakeMinter::GetInstance()
    {
        /* This will create a default initialized, memory only wallet file on first call (lazy instantiation) */
        static StakeMinter stakeMinter;

        return stakeMinter;
    }


    /* Destructor signals the stake minter thread to shut down and waits for it to join */
    StakeMinter::~StakeMinter()
    {
        {
            LOCK(StakeMinter::cs_stakeMinter);
            StakeMinter::fdestructMinter = true;
        }

        StakeMinter::minterThread.join();

        if (pReservedTrustKey != nullptr)
        {
            pReservedTrustKey->ReturnKey();
            delete pReservedTrustKey;
            pReservedTrustKey = nullptr;
        }

        /* Don't delete wallet, just null out the reference to it */
        pStakingWallet = nullptr;
    }


    /* Tests whether or not the stake minter is currently running. */
    bool StakeMinter::IsStarted() const
    {
        LOCK(StakeMinter::cs_stakeMinter);
        return fisStarted;
    }


    /* Retrieves the current internal value for the block weight metric. */
    double StakeMinter::GetBlockWeight() const
    {
        LOCK(StakeMinter::cs_stakeMinter);

        return nBlockWeight;
    }


    /* Retrieves the current block weight metric as a percentage of maximum. */
    double StakeMinter::GetBlockWeightPercent() const
    {
        LOCK(StakeMinter::cs_stakeMinter);

        return (nBlockWeight / 10.0);
    }


    /* Retrieves the current internal value for the trust weight metric. */
    double StakeMinter::GetTrustWeight() const
    {
        LOCK(StakeMinter::cs_stakeMinter);
        
        return nTrustWeight;
    }


    /* Retrieves the current trust weight metric as a percentage of maximum. */
    double StakeMinter::GetTrustWeightPercent() const
    {
        LOCK(StakeMinter::cs_stakeMinter);
        
        return (nTrustWeight / 90.0);
    }


    /* Retrieves the current staking reward rate (previously, interest rate) */
    double StakeMinter::GetStakeRate() const
    {
        LOCK(StakeMinter::cs_stakeMinter);
        
        return nStakeRate;
    }


    /* Checks whether the stake minter is waiting for average coin
     * age to reach the required minimum before staking Genesis.
     */
    bool StakeMinter::IsWaitPeriod() const
    {
        LOCK(StakeMinter::cs_stakeMinter);
        
        return fIsWaitPeriod;
    }


    /* Start the stake minter. */
    bool StakeMinter::StartStakeMinter()
    {
        LOCK(StakeMinter::cs_stakeMinter);

        if (!fisStarted)
        {
            if (pStakingWallet == nullptr)
                pStakingWallet = &(Wallet::GetInstance());

            fisStarted = true;
            return true;
        }

        return false;
    }


    /* Stop the stake minter. */
    bool StakeMinter::StopStakeMinter()
    {
        LOCK(StakeMinter::cs_stakeMinter);

        if (fisStarted)
        {
            fstopMinter = true;
            return true;
        }

        return false;
    }


    /*  Gets the trust key for the current wallet. If none exists, retrieves a new
     *  key from the key pool to use as the trust key for Genesis.
     */
    void StakeMinter::FindTrustKey()
    {
        trustKey.SetNull();

        /* Attempt to use the trust key cached in the wallet */
        std::vector<uint8_t> vchTrustKey = pStakingWallet->GetTrustKey();

        if (!vchTrustKey.empty())
        {
            uint576_t cKey;
            cKey.SetBytes(vchTrustKey);

            /* Read the key cached in wallet from the trustDB */
            if (!LLD::trustDB->ReadTrustKey(cKey, trustKey))
            {
                /* Cached wallet trust key not found in trust db, reset it */
                trustKey.SetNull();
                pStakingWallet->RemoveTrustKey();
            }
            else
            {
                /* Found trust key cached in wallet */
                debug::log(0, FUNCTION, "Staking with existing Trust Key");
            }
        }

        /* Scan for trust key within the trust database if none found in wallet.
         * Should only have to do this if new wallet (no trust key), or converting old wallet that doesn't have its key cached yet. 
         */
        if(trustKey.IsNull())
        {
            /* Retrieve all the trust key public keys from the trust db */
            std::vector< std::vector<uint8_t> > vchTrustKeyList = LLD::trustDB->GetKeys();

            for (const auto& vchHashKey : vchTrustKeyList)
            {
                /* Read the full trust key from the trust db */
                uint576_t cKey;
                cKey.SetBytes(vchHashKey);
                TAO::Ledger::TrustKey trustKeyCheck;

                if (!LLD::trustDB->ReadTrustKey(cKey, trustKeyCheck))
                    continue;

                /* Check whether trust key is part of current wallet */
                NexusAddress address;
                address.SetPubKey(trustKeyCheck.vchPubKey);

                if (pStakingWallet->HaveKey(address))
                {
                    /* Trust key belongs to current wallet. Verify this is the one to use. */
                    TAO::Ledger::BlockState blockStateCheck;

                    /* Check for keys that are expired version 4. */
                    if (TAO::Ledger::GetLastTrust(trustKeyCheck, blockStateCheck) && blockStateCheck.nVersion < 5)
                    {
                        /* Expired pre-v5 Trust Key. Do not use. */
                        debug::log(2, FUNCTION, "Found expired version 4 trust key in wallet. Not using.");
                        continue;
                    }

                    /* Set the trust key if found. */
                    trustKey = trustKeyCheck;

                    /* Store trust key */
                    pStakingWallet->SetTrustKey(trustKey.vchPubKey);

                    debug::log(0, FUNCTION, "Found Trust Key matching current wallet");
                }

            }
        }

        if (trustKey.IsNull())
        {
            /* No trust key found. Reserve a new key to use as the trust key for Genesis */
            debug::log(0, FUNCTION, "Staking for Genesis with new trust key");
            pReservedTrustKey = new CReserveKey(pStakingWallet);
            pReservedTrustKey->GetReservedKey();
        }

        return;
    }


    /* Creates a new legacy block that the stake minter will attempt to mine via the Proof of Stake process. */
    bool StakeMinter::CreateCandidateBlock()
    {
        /* Create the block to work on. 
         * Don't just reset old block. May still have a reference to it floating around in process of relaying it.
         */
        candidateBlock = LegacyBlock();

        CReserveKey dummyReserveKey(pStakingWallet); //Reserve key not used by CreateLegacyBlock for nChannel=0

        if (!CreateLegacyBlock(dummyReserveKey, 0, 0, candidateBlock))
            return debug::error(FUNCTION, "Unable to create candidate block");

        if (!trustKey.IsNull())
        {

            /* Looking to stake Trust for existing key */
            uint576_t cKey;
            cKey.SetBytes(trustKey.vchPubKey);

            /* Check that the database still has key. */
            TAO::Ledger::TrustKey keyCheck;
            if(!LLD::trustDB->ReadTrustKey(cKey, keyCheck))
            {
                debug::error(FUNCTION, "Trust key was disconnected");

                /* Ensure it is erased */
                LLD::trustDB->Erase(cKey);

                /* Remove trust key from wallet */
                pStakingWallet->RemoveTrustKey();

                /* Set the trust key to null state. */
                trustKey.SetNull();

                /* For next iteration, go back to staking for Genesis */ 
                if (pReservedTrustKey != nullptr)
                    delete pReservedTrustKey;  // should never happen, this is a precaution

                pReservedTrustKey = new CReserveKey(pStakingWallet);
                pReservedTrustKey->GetReservedKey();

                return false;
            }

            /* Prevout index needs to be 0 in coinstake transaction. */
            candidateBlock.vtx[0].vin[0].prevout.n = 0;

            /* Prevout hash is trust key hash */
            candidateBlock.vtx[0].vin[0].prevout.hash = trustKey.GetHash();

            /* Get the last stake block for this trust key. */
            TAO::Ledger::BlockState prevBlockState;
            if (!TAO::Ledger::GetLastTrust(trustKey, prevBlockState))
                return debug::error(FUNCTION, "Failed to get last trust for trust key");

            /* Enforce the minimum staking transaction interval. */
            if ((candidateBlock.nHeight - prevBlockState.nHeight) < (config::fTestNet 
                                                                    ? TAO::Ledger::TESTNET_MINIMUM_INTERVAL 
                                                                    : TAO::Ledger::MAINNET_MINIMUM_INTERVAL))
            {
                /* Below minimum interval. Previous stake block still maturing. Increase sleep time until can continue normally. */
                nSleepTime = 30000; //30 second wait (is reset below)
                return false;
            }

            /* Get the sequence and previous trust. */
            uint32_t nSequence = 0;
            uint32_t nScore = 0;
            uint32_t nPrevScore = 0;

            /* Handle if previous block was a genesis. */
            if (prevBlockState.vtx[0].first != TAO::Ledger::TYPE::LEGACY_TX)
            {
                /* This should not happen with legacy stake minter, but check it just in case. It means wallet is using Tritium staking, so stop the minter */
                {
                    LOCK(StakeMinter::cs_stakeMinter);
                    fstopMinter = true;
                }

                return debug::error(FUNCTION, "Trust key previous block is not a legacy coinstake. Stopping stake minter.");
            }

            /* Retrieve the previous coinstake transaction */
            uint512_t prevCoinstakeTxHash = prevBlockState.vtx[0].second;
            Transaction prevCoinstakeTx;
            if (!LLD::legacyDB->ReadTx(prevCoinstakeTxHash, prevCoinstakeTx))
                return debug::error(FUNCTION, "Failed to read previous coinstake for trust key");

            if (prevCoinstakeTx.IsGenesis())
            {
                nSequence   = 1;
                nPrevScore  = 0;
            }
            else
            {
                /* Extract the trust from the previous block. */
                uint1024_t hashDummy;

                if(!prevCoinstakeTx.ExtractTrust(hashDummy, nSequence, nPrevScore))
                    return debug::error("Failed to extract trust from previous block");

                /* Increment sequence number for next trust transaction. */
                nSequence ++;
            }

            /* Calculate time since the last trust block for this trust key (block age = age of previous trust block). */
            uint32_t nBlockAge = TAO::Ledger::ChainState::stateBest.GetBlockTime() - prevBlockState.GetBlockTime();
            uint32_t nMaxTrustScore = (60 * 60 * 24 * 28 * 13);
            uint32_t nMaxBlockAge = config::fTestNet ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN;

            /* Block age less than maximum awards trust score increase equal to the current block age. */
            if (nBlockAge <= nMaxBlockAge)
                nScore = std::min((nPrevScore + nBlockAge), nMaxTrustScore);

            /* Block age more than maximum allowed is penalized 3 times the time it has exceeded the maximum. */
            else
            {
                /* Calculate the penalty for score (3x the time). */
                uint32_t nPenalty = (nBlockAge - nMaxBlockAge) * 3;

                /* Catch overflows and zero out if penalties are greater than previous score. */
                if(nPenalty < nPrevScore)
                    nScore = nPrevScore - nPenalty;
                else
                    nScore = 0;
            }

            /* Double check that the trust score cannot exceed the maximum */
            if (nScore > nMaxTrustScore)
                nScore = nMaxTrustScore;

            /* Serialize previous trust block hash, new sequence, and new trust score into vin. */
            DataStream scriptPub(candidateBlock.vtx[0].vin[0].scriptSig, SER_NETWORK, LLP::PROTOCOL_VERSION);
            scriptPub << prevBlockState.GetHash() << nSequence << nScore;

            /* Set the script sig (CScript doesn't support serializing all types needed) */
            candidateBlock.vtx[0].vin[0].scriptSig.clear();
            candidateBlock.vtx[0].vin[0].scriptSig.insert(candidateBlock.vtx[0].vin[0].scriptSig.end(), scriptPub.begin(), scriptPub.end());

            /* Write the trust key into the output script. */
            candidateBlock.vtx[0].vout.resize(1);
            candidateBlock.vtx[0].vout[0].scriptPubKey << trustKey.vchPubKey << Legacy::OP_CHECKSIG;

        }
        else
        {
            /* Looking to stake Genesis for new key */
            /* Genesis prevout is null */
            candidateBlock.vtx[0].vin[0].prevout.SetNull();

            /* Write the reserved key into the output script. */
            candidateBlock.vtx[0].vout.resize(1);
            candidateBlock.vtx[0].vout[0].scriptPubKey << pReservedTrustKey->GetReservedKey() << Legacy::OP_CHECKSIG;
        }

        /* Add the coinstake inputs. Also generates coinstake output with staking reward */
        if (!pStakingWallet->AddCoinstakeInputs(candidateBlock))
        {
            /* Wallet has no balance, or balance unavailable for staking. Increase sleep time to wait for balance. */
            nSleepTime = 30000;
            debug::log(0, FUNCTION, "Wallet has no balance or no spendable inputs available.");
            return false;
        }
        else if (nSleepTime == 30000) {
            /* Reset sleep time after inputs become available. */
            nSleepTime = 1000;
        }

        /* Update the current stake rate in the minter (not used for calculations, retrievable for display) */
        TAO::Ledger::BlockState candidateBlockState(candidateBlock);
        double nCurrentStakeRate = trustKey.InterestRate(candidateBlockState, candidateBlock.GetBlockTime());

        {
            LOCK(StakeMinter::cs_stakeMinter);

            /* Use lock scope because someone may be retrieving this value */
            nStakeRate = nCurrentStakeRate;
        }

        return true;
    }


    /* Calculates the Trust Weight and Block Weight values for the current trust key and candidate block. */
    bool StakeMinter::CalculateWeights()
    {
        /* Use local variables for calculations, then set instance variables with a lock scope at the end */
        double nCurrentTrustWeight = 0.0;
        double nCurrentBlockWeight = 0.0;
        bool fNewIsWaitPeriod = false;

        /* Weight for Trust transactions combines trust weight and block weight. */
        if(candidateBlock.vtx[0].IsTrust())
        {
            uint32_t nTrustScore;
            uint32_t nBlockAge;
            uint32_t nMaximumBlockAge = (config::fTestNet ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN);


            /* Retrieve the current Trust Score from the candidate block */
            if(!candidateBlock.TrustScore(nTrustScore))
            {
                debug::error(FUNCTION, "Failed to get trust score");
                return false;
            }

            /* Retrieve the current Block Age from the candidate block */
            if(!candidateBlock.BlockAge(nBlockAge))
            {
                debug::error(FUNCTION, "Failed to get block age");
                return false;
            }

            /* Trust Weight continues to grow with Trust Score until it reaches max of 90.0
             * This formula will reach 45.0 (50%) after accumulating 84 days worth of Trust Score, while requiring close to a year to reach maximum.
             */
            nCurrentTrustWeight = std::min(90.0, (((44.0 * log(((2.0 * nTrustScore) / (60 * 60 * 24 * 28 * 3)) + 1.0)) / log(3))) + 1.0);

            /* Block Weight reaches maximum of 10.0 when Block Age equals the defined timespan */
            nCurrentBlockWeight = std::min(10.0, (((9.0 * log(((2.0 * nBlockAge) / nMaximumBlockAge) + 1.0)) / log(3))) + 1.0);

        }

        /* Weights for Genesis transactions only uses trust weight with its value based on average coin age. */
        else
        {
            uint64_t nCoinAge;
            uint32_t nMinimumCoinAge = (config::fTestNet ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN);

            /* Calculate the average Coin Age for coinstake inputs of candidate block. */
            if(!candidateBlock.vtx[0].CoinstakeAge(nCoinAge))
            {
                debug::error(FUNCTION, "Failed to get coinstake age");
                return false;
            }

            /* Genesis has to wait for average coin age to reach one full trust key timespan. */
            if(nCoinAge < nMinimumCoinAge)
            {
                /* Record that stake minter is in wait period */
                fNewIsWaitPeriod = true;

                /* Increase sleep time to wait for coin age to meet requirement (5 minute check) */
                nSleepTime = 300000;

                uint32_t nRemainingWaitTime = (nMinimumCoinAge - nCoinAge) / 60; //minutes

                debug::log(0, FUNCTION, "Average coin age is immature. %" PRIu32 " minutes remaining until staking available.", nRemainingWaitTime);
                return false;
            }
            else  
            {
                /* Reset wait period setting */
                fNewIsWaitPeriod = false;

                /* Reset sleep time after coin age meets requirement. */
                nSleepTime = 1000;
            }

            /* Trust Weight For Genesis is based on Coin Age, grows more slowly than post-Genesis Trust Weight,
             * and only reaches a maximum of 10.0 after average Coin Age reaches 84 days. 
             */
            nCurrentTrustWeight = std::min(10.0, (((9.0 * log(((2.0 * nCoinAge) / (60 * 60 * 24 * 28 * 3)) + 1.0)) / log(3))) + 1.0);

            /* Block Weight remains zero while staking for Genesis */
        }

        {
            LOCK(StakeMinter::cs_stakeMinter);

            /* These are updated via lock scope because someone may be checking these values using the available methods */
            nBlockWeight = nCurrentBlockWeight;
            nTrustWeight = nCurrentTrustWeight;
            fIsWaitPeriod = fNewIsWaitPeriod;
        }

        return true;
    }


    /* Attempt to solve the hashing algorithm at the current staking difficulty for the candidate block */
    void StakeMinter::MineProofOfStake()
    {
        bool fstop = false; // initialized false to assure it enters process loop

        /* Calculate the minimum Required Energy Efficiency Threshold. 
         * Minter can only mine Proof of Stake when current threshold exceeds this value. 
         *
         * Staking weights (trust and block) reduce the required threshold by reducing the numerator of this calculation.
         * Weight from staking balance (based on nValue out of coinstake) reduces the required threshold by increasing the denominator.
         */
        uint64_t nStake = candidateBlock.vtx[0].vout[0].nValue;
        double nRequired = ((108.0 - nTrustWeight - nBlockWeight) * TAO::Ledger::MAX_STAKE_WEIGHT) / nStake;

        /* Calculate the target value based on difficulty. */
        LLC::CBigNum bnTarget;
        bnTarget.SetCompact(candidateBlock.nBits);
        uint1024_t nHashTarget = bnTarget.getuint1024();

        debug::log(0, FUNCTION, "Staking new block from %s at weight %f and stake rate %f", 
            hashLastBlock.ToString().substr(0, 20).c_str(), (nTrustWeight + nBlockWeight), nStakeRate);

        /* Search for the proof of stake hash solution until it mines a block, minter is stopped, 
         * or network generates a new block (minter must start over with new candidate) 
         */
        while (!fstop && !config::fShutdown && hashLastBlock == TAO::Ledger::ChainState::hashBestChain)
        {
            /* Update the block time for difficulty accuracy. */
            candidateBlock.UpdateTime();
            uint32_t nCurrentBlockTime = candidateBlock.GetBlockTime() - candidateBlock.vtx[0].nTime; // How long have we been working on this block

            /* If just starting on block, wait */
            if (nCurrentBlockTime == 0)
            {
                runtime::sleep(1);
                continue;
            }

            /* Calculate the new Efficiency Threshold for the next nonce. */
            double nThreshold = (nCurrentBlockTime * 100.0) / (candidateBlock.nNonce + 1);

            {
                /* Check stop flag now in case we are below required threshold (lower balance wallets may remain below it for awhile) */
                LOCK(StakeMinter::cs_stakeMinter);
                fstop = StakeMinter::fstopMinter;

                if (fstop || config::fShutdown)
                    continue;
            }

            /* If energy efficiency requirement exceeds threshold, wait and keep trying with the same nonce value until it threshold increases */
            if(nThreshold < nRequired)
            {
                runtime::sleep(10);
                continue;
            }

            /* Increment the nonce only after we know we can use it (threshold exceeds required). */
            candidateBlock.nNonce++;

            /* Log every 1000 attempts */
            if (candidateBlock.nNonce % 1000 == 0)
                debug::log(3, FUNCTION, "Threshold %f exceeds required %f, mining Proof of Stake with nonce %" PRIu64, nThreshold, nRequired, candidateBlock.nNonce);

            /* Handle if block is found. */
            uint1024_t stakeHash = candidateBlock.StakeHash();
            if (stakeHash < nHashTarget)
            {
                debug::log(0, FUNCTION, "Found new stake hash %sn", stakeHash.ToString().substr(0, 20).c_str());

                ProcessMinedBlock();
                break;
            }
        }

        return;
    }


    bool StakeMinter::ProcessMinedBlock()
    {
        /* Add the transactions into the block from memory pool, but only if not Genesis (Genesis block for trust key has no transactions except coinstake). */
        if (!candidateBlock.vtx[0].IsGenesis())
            AddTransactions(candidateBlock.vtx);

        /* Build the Merkle Root. */
        std::vector<uint512_t> vMerkleTree;
        candidateBlock.hashMerkleRoot = candidateBlock.BuildMerkleTree(vMerkleTree);

        /* Sign the block. */
        if (!SignBlock(candidateBlock, *pStakingWallet))
            return debug::error(FUNCTION, "Failed to sign block");

        /* Check the block. */
        if (!candidateBlock.Check())
            return debug::error(FUNCTION, "Check block failed");

        /* Check the stake. */
        if (!candidateBlock.CheckStake())
            return debug::error(FUNCTION, "Check state failed");

        /* Check the stake. */
        if (!candidateBlock.vtx[0].CheckTrust(TAO::Ledger::ChainState::stateBest))
            return debug::error(FUNCTION, "Check trust failed");

        /* Check the work for the block. 
         * After a successful check, CheckWork() calls LLP::Process() for the new block.
         * That method will call LegacyBlock::Accept() and BlockState::Accept()
         * After all is accepted, BlockState::Accept() will call BlockState::SetBest() 
         * to set the new best chain. This final method relays the new block to the
         * network.
         */
        if(!CheckWork(candidateBlock, *pStakingWallet))
            return debug::error(FUNCTION, "Check work failed");

        if(pReservedTrustKey != nullptr)
        {
            /* New block was Genesis using reserved key. Create new trust key from it. Have to call GetReservedKey before KeepKey, which resets it  */
            uint576_t cKey;
            cKey.SetBytes(pReservedTrustKey->GetReservedKey());

            /* Create new trust key with Genesis block hash, Genesis tx hash, and Genesis time based on current block */
            TAO::Ledger::TrustKey trustKeyNew(pReservedTrustKey->GetReservedKey(), candidateBlock.GetHash(), 
                                              candidateBlock.vtx[0].GetHash(), candidateBlock.GetBlockTime());

            /* Add trust key to trust db */
            LLD::trustDB->WriteTrustKey(cKey, trustKeyNew); 

            /* Cache trust key in wallet */
            pStakingWallet->SetTrustKey(pReservedTrustKey->GetReservedKey());

            pReservedTrustKey->KeepKey();

            /* Use new key for further staking */
            trustKey = trustKeyNew;

            debug::log(0, FUNCTION, "New trust key generated and stored");

            delete pReservedTrustKey;
        }

        return true;
    }


    /* Method run on its own thread to oversee stake minter operation. */
    void StakeMinter::StakeMinterThread(StakeMinter* pStakeMinter)
    {

        /* Local copies of stake minter flags. These support testing conditions while only reading the shared static flags within a lock scope. */
        bool fstarted = false;
        bool fstop = false;
        bool fdestruct = false;

        debug::log(0, FUNCTION, "Stake Minter Started");
        pStakeMinter->nSleepTime = 5000;

        /* If minter is not started, or the system is still syncing/connecting on startup, wait to start minter */
        while ((!fstarted || TAO::Ledger::ChainState::Synchronizing() || LLP::LEGACY_SERVER->GetConnectionCount() == 0)  && !config::fShutdown)
        {
            runtime::sleep(pStakeMinter->nSleepTime);

            {
                LOCK(StakeMinter::cs_stakeMinter);
                fstarted = StakeMinter::fisStarted;
                fdestruct = StakeMinter::fdestructMinter;
            }
        }

        pStakeMinter->FindTrustKey();

        debug::log(0, FUNCTION, "Stake Minter Initialized");
        pStakeMinter->nSleepTime = 1000;

        /* Minting thread will continue repeating this loop until shutdown */
        while(!config::fShutdown)
        {
            runtime::sleep(pStakeMinter->nSleepTime);

            /* Check whether the stake minter has been stopped */
            {
                LOCK(StakeMinter::cs_stakeMinter);
                fstop = StakeMinter::fstopMinter;

                if (fstop)
                {
                    /* Suspend the minter and set for idle time until restarted */
                    fstarted = false;
                    StakeMinter::fisStarted = false;
                    pStakeMinter->nSleepTime = 5000;
                }
            }

            /* If the minter is stopped, wait for it to start again */
            while (!fstarted and !config::fShutdown)
            {
                runtime::sleep(pStakeMinter->nSleepTime);

                {
                    LOCK(StakeMinter::cs_stakeMinter);
                    fstarted = StakeMinter::fisStarted;
                }
            }

            if (fstop and !config::fShutdown)
            {
                /* When minter is restarted after a stop, reset for normal running */
                fstop = false;
                pStakeMinter->nSleepTime = 1000;

                {
                    LOCK(StakeMinter::cs_stakeMinter);
                    StakeMinter::fstopMinter = false;
                }
            }

            /* If we got a shutdown while it was sleeping or waiting to be restarted, skip the rest of this iteration */
            if (config::fShutdown)
                continue;

            /* Save the current best block hash immediately in case it change while we do setup */
            pStakeMinter->hashLastBlock = TAO::Ledger::ChainState::hashBestChain;

            /* Set up the candidate block the minter is attempting to mine */
            if (!pStakeMinter->CreateCandidateBlock())
                continue;

            /* Updates weights for new candidate block */
            if (!pStakeMinter->CalculateWeights())
                continue;

            /* Attempt to mine the current proof of stake block */
            pStakeMinter->MineProofOfStake();

        }

        /* On shutdown, delete the minter instance and wait for it to destruct before ending */ 
        delete pStakeMinter;

        while (!fdestruct)
        {
            runtime::sleep(1);

            {
                LOCK(StakeMinter::cs_stakeMinter);

                fdestruct = StakeMinter::fdestructMinter;
            }
        }

        /* Thread ends. Join is issued in StakeMinter destructor */
    }

}
