/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/address.h>
#include <Legacy/wallet/addressbook.h>

#include <LLC/types/bignum.h>

#include <LLP/include/global.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/trust.h>
#include <TAO/Ledger/types/state.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>


namespace Legacy
{

    /* Initialize static variables */
    bool StakeMinter::fisStarted = false;
    bool StakeMinter::fstopMinter = false;
    bool StakeMinter::fdestructMinter = false;

    std::mutex StakeMinter::cs_stakeMinter;


    StakeMinter& StakeMinter::GetInstance()
    {
        /* This will create a default initialized, memory only wallet file on first call (lazy instantiation) */
        static StakeMinter stakeMinter;

        return stakeMinter;
    }


    /* Destructor signals the stake minter thread to shut down and waits for it to join */
    ~StakeMinter::StakeMinter()
    {
        {
            LOCK(cs_stakeMinter);
            StakeMinter::fdestructMinter = true;
        }

        minterThread.join();

        if (pTrustKey != nullptr)
        {
            delete pTrustKey;
            pTrustKey = nullptr;
        }

        if (pTrustKey != nullptr)
        {
            delete pReservedTrustKey;
            pReservedTrustKey = nullptr;
        }

        pStakingWallet = nullptr;
    }


    /* Tests whether or not the stake minter is currently running. */
    bool StakeMinter::IsStarted()
    {
        LOCK(cs_stakeMinter);
        return fisStarted;
    }


    /* Retrieves the current internal value for the block weight metric. */
    double GetBlockWeight() const
    {
        LOCK(cs_stakeMinter);

        return nBlockWeight;
    }


    /* Retrieves the current block weight metric as a percentage of maximum. */
    double GetBlockWeightPercent() const
    {
        LOCK(cs_stakeMinter);

        return (nBlockWeight / 10.0);
    }


    /* Retrieves the current internal value for the trust weight metric. */
    double GetTrustWeight() const
    {
        LOCK(cs_stakeMinter);
        
        return nTrustWeight;
    }


    /* Retrieves the current trust weight metric as a percentage of maximum. */
    double GetTrustWeightPercent() const
    {
        LOCK(cs_stakeMinter);
        
        return (nTrustWeight / 90.0);
    }


    /* Checks whether the stake minter is waiting for average coin
     * age to reach the required minimum before staking Genesis.
     */
    bool IsWaitPeriod() const
    {
        LOCK(cs_stakeMinter);
        
        return fIsWaitPeriod;
    }


    /* Start the stake minter. */
    bool StakeMinter::StartStakeMinter()
    {
        LOCK(cs_stakeMinter);

        if (!fisStarted)
        {
            if (pStakingWallet == nullptr)
                pStakingWallet = &(CWallet::GetInstance());

            fisStarted = true;
            return true;
        }

        return false;
    }


    /* Stop the stake minter. */
    bool StakeMinter::StopStakeMinter();
    {
        LOCK(cs_stakeMinter);

        if (fisStarted)
        {
            fstopMinter = true;
            return true;
        }

        return false;
    }


    void StakeMinter::FindTrustKey()
    {
        trustKey.SetNull();
        std::vector<uint8_t> vchTrustKey;

        /* Attempt to use the trust key cached in the wallet */
        if (pStakingWallet->GetTrustKey(vchTrustKey) && !vchTrustKey.isEmpty())
        {
            uint576_t cKey;
            cKey.SetBytes(vchTrustKey);

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

        /* Scan for trust key within the trust database.
         * Only have to do this if new wallet (no trust key), or converting only wallet that doesn't have its key cached yet. 
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
                TrustKey trustKeyCheck;

                if (!LLD::trustDB->ReadTrustKey(cKey, trustKeyCheck))
                    continue;

                /* Check whether trust key is part of current wallet */
                Legacy::types::NexusAddress address;
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
                    pStakingWallet->SetTrustKey(trustKey);

                    debug::log(0, FUNCTION, "Found Trust Key from wallet keys");
                }

            }
        }

        if (trustKey.IsNull())
        {
            /* No trust key found. Reserve a new key to use as the trust key for Genesis */
            debug::log(0, FUNCTION, "Staking for Genesis with new trust key");
            pReservedTrustKey = new CReserveKey(pStakingWallet);
            pReservedTrustKey.GetReservedKey();
        }

    }


    bool StakeMinter::ResetMinter()
    {

    }


    bool StakeMinter::CreateCandidateBlock()
    {
            // /* Create the block to work on. */
            // // Need legacy create
            // //candidateBlock = CreateNewBlock(reservekey, pwalletMain, 0);
            // if(block.IsNull())
            //     continue;

            // /* Write the trust key into the output script. */
            // block.vtx[0].vout[0].scriptPubKey << vchTrustKey << Wallet::OP_CHECKSIG;

            // /* Trust transaction. */
            // if(!trustKey.IsNull())
            // {
            //     /* Set the key from bytes. */
            //     uint576 key;
            //     key.SetBytes(trustKey.vchPubKey);

            //     /* Check that the database has key. */
            //     LLD::CIndexDB indexdb("r+");
            //     CTrustKey keyCheck;
            //     if(!indexdb.ReadTrustKey(key, keyCheck))
            //     {
            //         error("Stake Minter : trust key was disconnected");

            //         /* Erase my key from trustdb. */
            //         trustdb.EraseMyKey();

            //         /* Set the trust key to null state. */
            //         trustKey.SetNull();

            //         continue;
            //     }

            //     /* Previous out needs to be 0 in coinstake transaction. */
            //     block.vtx[0].vin[0].prevout.n = 0;

            //     /* Previous out hash is trust key hash */
            //     block.vtx[0].vin[0].prevout.hash = trustKey.GetHash();

            //     /* Get the last block of this trust key. */
            //     uint1024 hashLastBlock = hashBest;
            //     if(!LastTrustBlock(trustKey, hashLastBlock))
            //     {
            //         error("Stake Minter : failed to find last block for trust key");
            //         continue;
            //     }

            //     /* Get the last block index from map block index. */
            //     CBlock blockPrev;
            //     if(!blockPrev.ReadFromDisk(mapBlockIndex[hashLastBlock], true))
            //     {
            //         error("Stake Minter : failed to read last block for trust key");
            //         continue;
            //     }

            //     /* Enforce the minimum trust key interval of 120 blocks. */
            //     if(block.nHeight - blockPrev.nHeight < (fTestNet ? TESTNET_MINIMUM_INTERVAL : MAINNET_MINIMUM_INTERVAL))
            //     {
            //         //error("Stake Minter : trust key interval below minimum interval %u", block.nHeight - blockPrev.nHeight);
            //         continue;
            //     }

            //     /* Get the sequence and previous trust. */
            //     unsigned int nTrustPrev = 0, nSequence = 0, nScore = 0;

            //     /* Handle if previous block was a genesis. */
            //     if(blockPrev.vtx[0].IsGenesis())
            //     {
            //         nSequence   = 1;
            //         nTrustPrev  = 0;
            //     }

            //     /* Handle if previous block was version 4 */
            //     else if(blockPrev.nVersion < 5)
            //     {
            //         nSequence   = 1;
            //         nTrustPrev  = trustKey.Age(mapBlockIndex[block.hashPrevBlock]->GetBlockTime());
            //     }

            //     /* Handle if previous block is version 5 trust block. */
            //     else
            //     {
            //         /* Extract the trust from the previous block. */
            //         uint1024 hashDummy;
            //         if(!blockPrev.ExtractTrust(hashDummy, nSequence, nTrustPrev))
            //         {
            //             error("Stake Minter : failed to extract trust from previous block");
            //             continue;
            //         }

            //         /* Increment Sequence Number. */
            //         nSequence ++;

            //     }

            //     /* The time it has been since the last trust block for this trust key. */
            //     int nTimespan = (mapBlockIndex[block.hashPrevBlock]->GetBlockTime() - blockPrev.nTime);

            //     /* Timespan less than required timespan is awarded the total seconds it took to find. */
            //     if(nTimespan < (fTestNet ? TRUST_KEY_TIMESPAN_TESTNET : TRUST_KEY_TIMESPAN))
            //         nScore = nTrustPrev + nTimespan;

            //     /* Timespan more than required timespan is penalized 3 times the time it took past the required timespan. */
            //     else
            //     {
            //         /* Calculate the penalty for score (3x the time). */
            //         int nPenalty = (nTimespan - (fTestNet ? TRUST_KEY_TIMESPAN_TESTNET : TRUST_KEY_TIMESPAN)) * 3;

            //         /* Catch overflows and zero out if penalties are greater than previous score. */
            //         if(nPenalty < nTrustPrev)
            //             nScore = (nTrustPrev - nPenalty);
            //         else
            //             nScore = 0;
            //     }

            //     /* Set maximum trust score to seconds passed for interest rate. */
            //     if(nScore > (60 * 60 * 24 * 28 * 13))
            //         nScore = (60 * 60 * 24 * 28 * 13);

            //     /* Serialize the sequence and last block into vin. */
            //     CDataStream scriptPub(block.vtx[0].vin[0].scriptSig, SER_NETWORK, PROTOCOL_VERSION);
            //     scriptPub << hashLastBlock << nSequence << nScore;

            //     /* Set the script sig (CScript doesn't support serializing all types needed) */
            //     block.vtx[0].vin[0].scriptSig.clear();
            //     block.vtx[0].vin[0].scriptSig.insert(block.vtx[0].vin[0].scriptSig.end(), scriptPub.begin(), scriptPub.end());
            // }
            // else
            //     block.vtx[0].vin[0].prevout.SetNull();

            // /* Add the coinstake inputs */
            // if (!pwalletMain->AddCoinstakeInputs(block))
            // {
            //     /* Wallet has no balance, or balance unavailable for staking. Increase sleep time to wait for balance. */
            //     nSleepTime = 30000;
            //     printf("Stake Minter : no spendable inputs available\n");
            //     continue;
            // }
            // else if (nSleepTime == 30000) {
            //     /* Reset sleep time after inputs become available. */
            //     nSleepTime = 1000;
            // }

    }


    bool StakeMinter::CalculateWeights()
    {
        /* Use local variables for calculations, then set instance variables with a lock scope at the end */
        double nCurrentTrustWeight = 0.0
        double nCurrentBlockWeight = 0.0
        bool fNewIsWaitPeriod = false;

        /* Weight for Trust transactions combines trust weight and block weight. */
        if(candidateBlock.vtx[0].IsTrust())
        {
            uint32_t nTrustScore;
            uint32_t nBlockAge;
            uint32_t nMaximumBlockAge = (fTestNet ? TRUST_KEY_TIMESPAN_TESTNET : TRUST_KEY_TIMESPAN);


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
            nCurrentTrustWeight = min(90.0, (((44.0 * log(((2.0 * nTrustScore) / (60 * 60 * 24 * 28 * 3)) + 1.0)) / log(3))) + 1.0);

            /* Block Weight reaches maximum of 10.0 when Block Age equals the defined timespan */
            nCurrentBlockWeight = min(10.0, (((9.0 * log(((2.0 * nBlockAge) / nMaximumBlockAge) + 1.0)) / log(3))) + 1.0);

        }

        /* Weights for Genesis transactions only uses trust weight with its value based on average coin age. */
        else
        {
            uint64_t nCoinAge;
            uint32_t nMinimumCoinAge = (fTestNet ? TRUST_KEY_TIMESPAN_TESTNET : TRUST_KEY_TIMESPAN);

            /* Calculate the average Coin Age for coinstake inputs of candidate block. */
            if(!block.vtx[0].CoinstakeAge(nCoinAge))
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

                uint32_t nRemainingWaitTime = (nMinimumCoinAge = nCoinAge) / 60

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
            nCurrentTrustWeight = min(10.0, (((9.0 * log(((2.0 * nCoinAge) / (60 * 60 * 24 * 28 * 3)) + 1.0)) / log(3))) + 1.0);

            /* Block Weight remains zero while staking for Genesis */
        }

        {
            LOCK(cs_stakeMinter);

            /* These are updated via lock scope because someone may be checking these values using the available methods */
            nBlockWeight = nCurrentBlockWeight;
            nTrustWeight = nCurrentTrustWeight;
            fIsWaitPeriod = fNewIsWaitPeriod;
        }

        return true;
    }


    bool StakeMinter::MineProofOfStake()
    {
            bool fstop = false; //initialized false, will always perform at least one iteration even if miner was just stopped

            /* Calculate the minimum Required Energy Efficiency Threshold. 
             * Minter can only mine Proof of Stake when current threshold exceeds this value. 
             *
             * Staking weights (trust and block) reduce the required threshold by reducing the numerator of this calculation.
             * Weight from staking balance (based on nValue out of coinstake) reduces the required threshold by increasing the denominator.
             */
            uint64_t nStake = block.vtx[0].vout[0].nValue
            double nRequired = ((108.0 - nTrustWeight - nBlockWeight) * TAO::Ledger::MAX_STAKE_WEIGHT) / nStake;

            /* Calculate the target value based on difficulty. */
            CBigNum bnTarget;
            bnTarget.SetCompact(candidateBlock.nBits);
            uint1024_t nHashTarget = bnTarget.getuint1024();

            debug::log(0, FUNCTION, "Staking new block from %s at weight %f and stake rate %f", 
                hashLastBlock.ToString().substr(0, 20).c_str(), (nTrustWeight + nBlockWeight), nInterestRate);

            /* Search for the proof of stake hash solution until it mines a block, minter is stopped, 
             * or network generates a new block (minter must start over with new candidate) 
             */
            while (!fstop && !config::fShutdown && hashLastBlock == TAO::Ledger::ChainState::hashBestChain)
            {
                /* Update the block time for difficulty accuracy. */
                candidateBlock.UpdateTime();
                uint32_t nCurrentBlockTime = candidateBlock.GetBlockTime() - candidateBlock.vtx[0].nTime; // How long have we been working on this block

                /* If just starting on block, wait */
                if (nCurrentBlockTime)
                {
                    runtime::Sleep(1);
                    continue;
                }

                /* Calculate the new Efficiency Threshold for the next nonce. */
                double nThreshold = (nCurrentBlockTime * 100.0) / (candidateBlock.nNonce + 1);

                {
                    /* Check stop flag now in case we are below required threshold (lower balance wallets may remain below it for awhile) */
                    LOCK(cs_stakeMinter);
                    fstop = StakeMinter::fstopMinter;

                    if (fstop)
                        continue;
                }

                /* If energy efficiency requirement exceeds threshold, wait and keep trying with the same nonce value until it threshold increases */
                if(nThreshold < nRequired)
                {
                    runtime::Sleep(1);
                    continue;
                }

                /* Increment the nonce only after we know we can use it (threshold exceeds required). */
                candidateBlock.nNonce++;

                /* Log every 1000 attempts */
                if (candidateBlock.nNonce % 1000 == 0)
                    debug::log(3, FUNCTION, "Threshold %f exceeds required %f, mining Proof of Stake with nonce %" PRIu64, nThreshold, nRequired, candidateBlock.nNonce);

                /* Handle if block is found. */
                if (candidateBlock.StakeHash() < hashTarget)
                {
                    ProcessMinedBlock();
                    break;
                }

            }

    }


    bool StakeMinter::ProcessMinedBlock()
    {
        // /* Sign the new Proof of Stake Block. */
        // if(GetArg("-verbose", 0) >= 0)
        //     printf("Stake Minter : found new stake hash %s\n", block.StakeHash().ToString().substr(0, 20).c_str());

        // /* Set the staking thread priorities. */
        // SetThreadPriority(THREAD_PRIORITY_NORMAL);

        // /* Add the transactions into the block from memory pool. */
        // if (!block.vtx[0].IsGenesis())
        //     AddTransactions(block.vtx, pindexBest);

        // /* Build the Merkle Root. */
        // block.hashMerkleRoot   = block.BuildMerkleTree();

        // /* Sign the block. */
        // if (!block.SignBlock(*pwalletMain))
        // {
        //     printf("Stake Minter : failed to sign block");
        //     break;
        // }

        // /* Check the block. */
        // if (!block.CheckBlock())
        // {
        //     error("Stake Minter : check block failed");
        //     break;
        // }

        // /* Check the stake. */
        // if (!block.CheckStake())
        // {
        //     error("Stake Minter : check stake failed");
        //     break;
        // }

        // /* Check the stake. */
        // if (!block.CheckTrust())
        // {
        //     error("Stake Minter : check stake failed");
        //     break;
        // }

        // /* Check the work for the block. */
        // if(!CheckWork(&block, *pwalletMain, reservekey))
        // {
        //     error("Stake Minter : check work failed");
        //     break;
        // }

        // /* Write the trust key to the key db. */
        // if(trustKey.IsNull())
        // {
        //     CTrustKey trustKeyNew(vchTrustKey, block.GetHash(), block.vtx[0].GetHash(), block.nTime);
        //     trustdb.WriteMyKey(trustKeyNew);

        //     trustKey = trustKeyNew;
        //     printf("Stake Minter : new trust key written\n");
        // }

    }



    /* Method run on its own thread to oversee stake minter operation. */
    void StakeMinter::StakeMinterThread(StakeMinter* pStakeMinter)
    {

        /* Local copies of stake minter flags. These support testing conditions while only reading the shared static flags within a lock scope. */
        bool fstarted = false;
        bool fstop = false;
        bool fdestruct = false;

        debug::log(0, FUNCTION, "Stake Minter Started");
        nSleepTime = 5000;

        /* If minter is not started, or the system is still syncing/connecting on startup, wait to start minter */
        while ((!fstarted || TAO::Ledger::ChainState::Synchronizing() || LLP::LEGACY_SERVER->GetConnectionCount() == 0)  && !config::fShutdown)
        {
            Sleep(nSleepTime);

            {
                LOCK(cs_stakeMinter);
                fstarted = StakeMinter::fisStarted;
                fdestruct = StakeMinter::fdestructMinter;
            }
        }

        pStakeMinter->FindTrustKey();

        debug::log(0, FUNCTION, "Stake Minter Initialized");
        nSleepTime = 1000;

        /* Minting thread will continue repeating this loop until shutdown */
        while(!config::fShutdown)
        {
            runtime::Sleep(nSleepTime);

            /* Save the current best block hash immediately in case it changes */
            hashLastBlock = TAO::Ledger::ChainState::hashBestChain;

            /* Set up the candidate block the minter is attempting to mine */
            pStakeMinter->CreateCandidateBlock();

            /* Updates weights for new candidate block */
            pStakeMinter->CalculateWeights();

            /* Attempt to mine the current proof of stake block */
            pStakeMinter->MineProofOfStake();

            /* Reset candidate block for next iteration */
            pStakeMinter->ResetMinter();

            /* Check whether the stake minter has been stopped */
            {
                LOCK(cs_stakeMinter);
                fstop = StakeMinter::fstopMinter;

                if (fstop)
                {
                    /* Suspend the minter and set for idle time until restarted */
                    fstarted = false;
                    StakeMinter::fisStarted = false;
                    nSleepTime = 5000;
                }
            }

            /* If the minter is stopped, wait for it to start again */
            while (!fstarted and !config::fShutdown)
            {
                runtime::Sleep(nSleepTime);

                {
                    LOCK(cs_stakeMinter);
                    fstarted = StakeMinter::fisStarted;
                }
            }

            if (fstop and !config::fShutdown)
            {
                /* When minter is restarted after a stop, reset for normal running */
                fstop = false;
                nSleepTime = 1000;

                {
                    LOCK(cs_stakeMinter);
                    StakeMinter::fstopMinter = false;
                }
            }
        }

        /* On shutdown, delete the minter instance and wait for it to destruct before ending */ 
        delete pStakeMinter;

        while (!fdestruct)
        {
            runtime::Sleep(1);

            {
                LOCK(cs_stakeMinter);

                fdestruct = StakeMinter::fdestructMinter;
            }
        }

        /* Thread ends. Join is issued in StakeMinter destructor */
    }

}
