/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/minter.h>

#include <Legacy/include/create.h>
#include <Legacy/types/address.h>
#include <Legacy/types/transaction.h>
#include <Legacy/wallet/addressbook.h>

#include <LLC/types/bignum.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/include/version.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/trust.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/tritium.h> //for LEGACY_TX enum

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/templates/datastream.h>


namespace Legacy
{

    /* Define constants for use by minter */


    /* Initialize static variables */
    std::atomic<bool> StakeMinter::fisStarted(false);
    std::atomic<bool> StakeMinter::fstopMinter(false);

    std::thread StakeMinter::minterThread;


    StakeMinter& StakeMinter::GetInstance()
    {
        static StakeMinter stakeMinter;

        return stakeMinter;
    }


    /* Destructor - Returns reserve key if not used */
    StakeMinter::~StakeMinter()
    {
        /* If minter has reserved a key to stake for Genesis, return it to the key pool and clean up the pointer. */
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
        return StakeMinter::fisStarted.load();
    }


    /* Retrieves the current internal value for the block weight metric. */
    double StakeMinter::GetBlockWeight() const
    {
        return nBlockWeight.load();
    }


    /* Retrieves the current block weight metric as a percentage of maximum. */
    double StakeMinter::GetBlockWeightPercent() const
    {
        return (nBlockWeight.load() * 100.0 / 10.0);
    }


    /* Retrieves the current internal value for the trust weight metric. */
    double StakeMinter::GetTrustWeight() const
    {
        return nTrustWeight.load();
    }


    /* Retrieves the current trust weight metric as a percentage of maximum. */
    double StakeMinter::GetTrustWeightPercent() const
    {
        return (nTrustWeight.load() * 100.0 / 90.0);
    }


    /* Retrieves the current staking reward rate (previously, interest rate) */
    double StakeMinter::GetStakeRate() const
    {
        return nStakeRate.load();
    }


    /* Retrieves the current staking reward rate as an annual percentage */
    double StakeMinter::GetStakeRatePercent() const
    {
        return nStakeRate.load() * 100.0;
    }


    /* Checks whether the stake minter is waiting for average coin
     * age to reach the required minimum before staking Genesis.
     */
    bool StakeMinter::IsWaitPeriod() const
    {
        return fIsWaitPeriod.load();
    }


    /* Start the stake minter. */
    bool StakeMinter::StartStakeMinter()
    {
        if (StakeMinter::fisStarted.load())
        {
        	debug::error(FUNCTION, "Attempt to start Stake Minter when already started.");
        	return false;
        }

        //beta mode removed for release
        // /* Disable stake minter if not in beta mode. */
        // if(!config::GetBoolArg("-beta"))
        // {
        //     debug::error(FUNCTION, "Stake minter disabled if not in -beta mode");

        //     return false;
        // }

    	/* Check that stake minter is configured to run.
    	 * Stake Minter default is to run for non-server and not to run for server
    	 */
        if(!config::GetBoolArg("-stake"))
    	{
    		debug::log(2, "Stake Minter not configured. Startup cancelled.");

    		return false;
    	}

		if (pStakingWallet == nullptr)
			pStakingWallet = &(Wallet::GetInstance());

        /* Wallet should be unlocked. */
        if(pStakingWallet->IsLocked())
        {
            debug::error(FUNCTION, "Cannot start stake minter for locked wallet.");

            return false;
        }

		/* Ensure stop flag is reset or thread will immediately exit */
		StakeMinter::fstopMinter.store(false);

		StakeMinter::minterThread = std::thread(StakeMinter::StakeMinterThread, this);

		StakeMinter::fisStarted.store(true);

		return true;
    }


    /* Stop the stake minter. */
    bool StakeMinter::StopStakeMinter()
    {
        if (StakeMinter::fisStarted.load())
        {
            debug::log(0, FUNCTION, "Shutting down Stake Minter");

        	StakeMinter::fstopMinter.store(true);

            if(StakeMinter::minterThread.joinable())
                StakeMinter::minterThread.join();

            if (pReservedTrustKey != nullptr)
            {
                /* If stop while staking for Genesis, return the key. It will reserve a new one if restarted */
                pReservedTrustKey->ReturnKey();
                delete pReservedTrustKey;
                pReservedTrustKey = nullptr;
            }

            StakeMinter::fisStarted.store(false);
            StakeMinter::fstopMinter.store(false);
            return true;
        }

        return false;
    }


    /*  Gets the trust key for the current wallet. If none exists, retrieves a new
     *  key from the key pool to use as the trust key for Genesis.
     */
    void StakeMinter::FindTrustKey()
    {
        static bool fVerified = false;

        trustKey.SetNull();

        /* Attempt to use the trust key cached in the wallet */
        std::vector<uint8_t> vchTrustKey = pStakingWallet->GetTrustKey();

        if(!vchTrustKey.empty())
        {
            uint576_t cKey;
            cKey.SetBytes(vchTrustKey);

            /* Read the key cached in wallet from the trustDB */
            if(!LLD::trustDB->ReadTrustKey(cKey, trustKey))
            {
                /* Cached wallet trust key not found in trust db, reset it (can happen if Genesis is orphaned). */
                trustKey.SetNull();
                pStakingWallet->RemoveTrustKey();
            }
            else
            {
                /* Found trust key cached in wallet */

                if(!fVerified)
                {
                    /* Only need to log this first time it is retrieved (ie, before verified) */
                    debug::log(0, FUNCTION, "Staking with existing trust key");
                }
            }
        }


        /* If wallet does not contain trust key, need tos can for it within the trust database.
         * Should only have to do this if new wallet (no trust key), or converting old wallet that doesn't have its key cached yet.
         */
        if(trustKey.IsNull())
        {
            /* Retrieve all raw trust database keys from keychain */
            std::vector<TAO::Ledger::TrustKey> vKeys;
            if(LLD::trustDB->BatchRead("trust", vKeys, -1))
            {
                /* Search through the trust keys. */
                for(const auto& trustKeyCheck : vKeys)
                {
                    /* Check whether trust key is part of current wallet */
                    NexusAddress address;
                    address.SetPubKey(trustKeyCheck.vchPubKey);

                    if(pStakingWallet->HaveKey(address))
                    {
                        /* Trust key is in wallet, check version of most recent block */
                        debug::log(2, FUNCTION, "Checking trustKey ", address.ToString());

                        /* Read the block for hashLastBlock to check its version. We can use this, even if it has not
                         * been verified, yet, because all we need is the version of the block .
                         */
                        TAO::Ledger::BlockState state;
                        if(LLD::legDB->ReadBlock(trustKeyCheck.hashLastBlock, state))
                        {
                            debug::log(2, FUNCTION, "Checking last stake height=", state.nHeight, " version=", state.nVersion);

                            if(state.nVersion >= 5)
                            {
                                /* Set the trust key if found. */
                                trustKey = trustKeyCheck;

                                /* Store trust key */
                                pStakingWallet->SetTrustKey(trustKey.vchPubKey);

                                debug::log(0, FUNCTION, "Found Trust Key matching current wallet");
                                break;
                            }
                            else
                            {
                                /* Expired pre-v5 Trust Key. Do not use. */
                                debug::log(2, FUNCTION, "Found expired version 4 Trust key. Not using.");
                            }
                        }
                    }
                }
            }
        }


        /* When trust key found, verify data stored against actual last trust block.
         * Older versions of code did not revert trust key data after an orphan and it may be out of date.
         * The GetLastTrust method can be slow, so only do it the first time after minter is started.
         */
        if(!fVerified && !trustKey.IsNull() && !config::fShutdown.load())
        {
            TAO::Ledger::BlockState stateLast = TAO::Ledger::ChainState::stateBest.load();
            if(TAO::Ledger::GetLastTrust(trustKey, stateLast))
            {
                uint1024_t hashLast = stateLast.GetHash();

                if(trustKey.hashLastBlock != hashLast)
                {
                    /* Trust key is out of date. Update it */
                    debug::log(2, FUNCTION, "Updating trust key");

                    uint64_t nBlockTime = stateLast.GetBlockTime();

                    trustKey.hashLastBlock = hashLast;
                    trustKey.nLastBlockTime = nBlockTime;
                    trustKey.nStakeRate = trustKey.StakeRate(stateLast, nBlockTime);

                    uint576_t cKey;
                    cKey.SetBytes(trustKey.vchPubKey);

                    LLD::trustDB->WriteTrustKey(cKey, trustKey);
                }
            }

            fVerified = true;
        }


        if(trustKey.IsNull() && !config::fShutdown.load())
        {
            /* No trust key found. Reserve a new key to use as the trust key for Genesis */
            debug::log(0, FUNCTION, "Staking for Genesis with new trust key");

            pReservedTrustKey = new ReserveKey(pStakingWallet);
            pReservedTrustKey->GetReservedKey();
        }

        return;
    }


    /* Creates a new legacy block that the stake minter will attempt to mine via the Proof of Stake process. */
    bool StakeMinter::CreateCandidateBlock()
    {
        /* Use appropriate settings for Testnet or Mainnet */
        static const uint32_t nMaxTrustScore = config::fTestNet ? TAO::Ledger::TRUST_SCORE_MAX_TESTNET : TAO::Ledger::TRUST_SCORE_MAX;
        static const uint32_t nMaxBlockAge = config::fTestNet ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN;

        static uint32_t nWaitCounter = 0; //Prevents log spam during wait period

        /* New Mainnet interval will go into effect with activation of v7. Can't be static so it goes live immediately (can update after activation) */
        const uint32_t nMinimumInterval = config::fTestNet
                                            ? TAO::Ledger::TESTNET_MINIMUM_INTERVAL
                                            : (TAO::Ledger::NETWORK_BLOCK_CURRENT_VERSION < 7)
                                                ? TAO::Ledger::MAINNET_MINIMUM_INTERVAL_LEGACY
                                                : (runtime::timestamp() > TAO::Ledger::NETWORK_VERSION_TIMELOCK[5])
                                                        ? TAO::Ledger::MAINNET_MINIMUM_INTERVAL
                                                        : TAO::Ledger::MAINNET_MINIMUM_INTERVAL_LEGACY;

        /* Create the block to work on.
         * Don't just reset old block. May still have a reference to it floating around in process of relaying it.
         */
        candidateBlock = LegacyBlock();

        ReserveKey dummyReserveKey(pStakingWallet); //Reserve key not used by CreateLegacyBlock for nChannel=0
        Coinbase dummyCoinbase; // Coinbase not used for staking

        if(!CreateLegacyBlock(dummyReserveKey, dummyCoinbase, 0, 0, candidateBlock))
            return debug::error(FUNCTION, "Unable to create candidate block");

        if(!trustKey.IsNull())
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
                {
                    delete pReservedTrustKey;  // should never happen, this is a precaution
                    pReservedTrustKey = nullptr;
                }

                pReservedTrustKey = new ReserveKey(pStakingWallet);
                pReservedTrustKey->GetReservedKey();

                return false;
            }

            /* Get the last stake block for this trust key. */
            TAO::Ledger::BlockState statePrev;
            if(!LLD::legDB->ReadBlock(trustKey.hashLastBlock, statePrev))
                return debug::error(FUNCTION, "Failed to get last stake for trust key");

            /* Enforce the minimum staking transaction interval. (current height is candidate height - 1) */
            uint32_t nCurrentInterval = candidateBlock.nHeight - 1 - statePrev.nHeight;
            if(nCurrentInterval < nMinimumInterval)
            {
                /* Below minimum interval for generating stake blocks. Increase sleep time until can continue normally. */
                nSleepTime = 5000; //5 second wait is reset below (can't sleep too long or will hang until wakes up on shutdown)

                /* Update log every 60 iterations (5 minutes) */
                if ((nWaitCounter % 60) == 0)
                    debug::log(0, FUNCTION, "Stake Minter: Too soon after mining last stake block. ",
                               (nMinimumInterval - nCurrentInterval), " blocks remaining until staking available.");

                ++nWaitCounter;

                return false;
            }

            /* Get the sequence and previous trust. */
            uint32_t nSequence = 0;
            uint32_t nScore = 0;
            uint32_t nPrevScore = 0;

            /* Handle if previous block was a genesis. */
            if(statePrev.vtx[0].first != TAO::Ledger::TYPE::LEGACY_TX)
            {
                debug::error(FUNCTION, "Trust key for Legacy Stake Minter does not have Legacy transaction in Genesis coinstake.");

                throw std::runtime_error("Trust key for Legacy Stake Minter does not have Legacy transaction in Genesis coinstake.");
            }

            /* Retrieve the previous coinstake transaction */
            uint512_t prevHash = statePrev.vtx[0].second;
            Transaction txPrev;
            if(!LLD::legacyDB->ReadTx(prevHash, txPrev))
                return debug::error(FUNCTION, "Failed to read previous coinstake for trust key");

            if(txPrev.IsGenesis())
            {
                nSequence   = 1;
                nPrevScore  = 0;
            }
            else
            {
                /* Extract the trust from the previous block. */
                uint1024_t hashDummy;

                if(!txPrev.ExtractTrust(hashDummy, nSequence, nPrevScore))
                    return debug::error("Failed to extract trust from previous block");

                /* Increment sequence number for next trust transaction. */
                ++nSequence;
            }

            /* Calculate time since the last trust block for this trust key (block age = age of previous trust block). */
            uint32_t nBlockAge = TAO::Ledger::ChainState::stateBest.load().GetBlockTime() - statePrev.GetBlockTime();

            /* Block age less than maximum awards trust score increase equal to the current block age. */
            if(nBlockAge <= nMaxBlockAge)
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
            if(nScore > nMaxTrustScore)
                nScore = nMaxTrustScore;

            /* Prevout index needs to be 0 in coinstake transaction. */
            candidateBlock.vtx[0].vin[0].prevout.n = 0;

            /* Prevout hash is trust key hash */
            candidateBlock.vtx[0].vin[0].prevout.hash = trustKey.GetHash();

            /* Serialize previous trust block hash, new sequence, and new trust score into vin. */
            DataStream scriptPub(candidateBlock.vtx[0].vin[0].scriptSig, SER_NETWORK, LLP::PROTOCOL_VERSION);
            scriptPub << statePrev.GetHash() << nSequence << nScore;

            /* Set the script sig (Script doesn't support serializing all types needed) */
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
        if(!pStakingWallet->AddCoinstakeInputs(candidateBlock))
        {
            /* Wallet has no balance, or balance unavailable for staking. Increase sleep time to wait for balance. */
            nSleepTime = 5000;

            /* Update log every 60 iterations (5 minutes) */
            if ((nWaitCounter % 60) == 0)
                debug::log(0, FUNCTION, "Stake Minter: Wallet has no balance or no spendable inputs available.");

            ++nWaitCounter;

            return false;
        }
        else if(nSleepTime == 5000) {
            /* Normal stake operation now available. Reset sleep time and wait counter. */
            nSleepTime = 1000;
            nWaitCounter = 0;
        }

        /* Update the current stake rate in the minter (not used for calculations, retrievable for display) */
        nStakeRate.store(trustKey.StakeRate(candidateBlock, candidateBlock.GetBlockTime()));

        return true;
    }


    /* Calculates the Trust Weight and Block Weight values for the current trust key and candidate block. */
    bool StakeMinter::CalculateWeights()
    {
        static const double LOG3 = log(3); // Constant for use in calculations

        /* Use appropriate settings for Testnet or Mainnet */
        static const uint32_t nTrustWeightBase = config::fTestNet ? TAO::Ledger::TRUST_WEIGHT_BASE_TESTNET : TAO::Ledger::TRUST_WEIGHT_BASE;
        static const uint32_t nMaxBlockAge = config::fTestNet ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN;
        static const uint32_t nMinimumCoinAge = config::fTestNet ? TAO::Ledger::MINIMUM_GENESIS_COIN_AGE_TESTNET : TAO::Ledger::MINIMUM_GENESIS_COIN_AGE;

        /* Use local variables for calculations, then set instance variables with a lock scope at the end */
        double nCurrentTrustWeight = 0.0;
        double nCurrentBlockWeight = 0.0;

        static uint32_t nWaitCounter = 0; //Prevents log spam during wait period

        /* Weight for Trust transactions combines trust weight and block weight. */
        if(candidateBlock.vtx[0].IsTrust())
        {
            uint32_t nTrustScore;
            uint32_t nBlockAge;


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
             * This formula will reach 45.0 (50%) after accumulating 84 days worth of Trust Score (Mainnet), while requiring close to a year to reach maximum.
             */
            double nTrustWeightRatio = (double)nTrustScore / (double)nTrustWeightBase;
            nCurrentTrustWeight = std::min(90.0, (44.0 * log((2.0 * nTrustWeightRatio) + 1.0) / LOG3) + 1.0);

            /* Block Weight reaches maximum of 10.0 when Block Age equals the defined timespan */
            double nBlockAgeRatio = (double)nBlockAge / (double)nMaxBlockAge;
            nCurrentBlockWeight = std::min(10.0, (9.0 * log((2.0 * nBlockAgeRatio) + 1.0) / LOG3) + 1.0);
        }

        /* Weights for Genesis transactions only uses trust weight with its value based on average coin age. */
        else
        {
            uint64_t nCoinAge;

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
                fIsWaitPeriod.store(true);

                /* Increase sleep time to wait for coin age to meet requirement (can't sleep too long or will hang until wakes up on shutdown) */
                nSleepTime = 5000;

                /* Update log every 60 iterations (5 minutes) */
                if ((nWaitCounter % 60) == 0)
                {
					uint32_t nRemainingWaitTime = (nMinimumCoinAge - nCoinAge) / 60; //minutes

					debug::log(0, FUNCTION, "Stake Minter: Average coin age is immature. ",
                               nRemainingWaitTime, " minutes remaining until staking available.");
                }

                ++nWaitCounter;

                return false;
            }
            else
            {
                /* Reset wait period setting */
                fIsWaitPeriod.store(false);

                /* Reset sleep time after coin age meets requirement. */
                nSleepTime = 1000;
                nWaitCounter = 0;
            }

            /* Trust Weight For Genesis is based on Coin Age, grows more slowly than post-Genesis Trust Weight,
             * and only reaches a maximum of 10.0 after average Coin Age reaches 84 days.
             */
            double nGenesisTrustRatio = (double)nCoinAge / (double)nTrustWeightBase;
            nCurrentTrustWeight = std::min(10.0, (9.0 * log((2.0 * nGenesisTrustRatio) + 1.0) / LOG3) + 1.0);

            /* Block Weight remains zero while staking for Genesis */
            nCurrentBlockWeight = 0.0;
        }

    		/* Update instance settings */
    		nBlockWeight.store(nCurrentBlockWeight);
    		nTrustWeight.store(nCurrentTrustWeight);

        return true;
    }


    /* Attempt to solve the hashing algorithm at the current staking difficulty for the candidate block */
    void StakeMinter::MineProofOfStake()
    {
        /* Calculate the minimum Required Energy Efficiency Threshold.
         * Minter can only mine Proof of Stake when current threshold exceeds this value.
         *
         * Staking weights (trust and block) reduce the required threshold by reducing the numerator of this calculation.
         * Weight from staking balance (based on nValue out of coinstake) reduces the required threshold by increasing the denominator.
         */
        uint64_t nStake = candidateBlock.vtx[0].vout[0].nValue;
        double nRequired = ((108.0 - nTrustWeight.load() - nBlockWeight.load()) * TAO::Ledger::MAX_STAKE_WEIGHT) / nStake;

        /* Calculate the target value based on difficulty. */
        LLC::CBigNum bnTarget;
        bnTarget.SetCompact(candidateBlock.nBits);
        uint1024_t nHashTarget = bnTarget.getuint1024();

        debug::log(0, FUNCTION, "Staking new block from ", hashLastBlock.ToString().substr(0, 20),
                                " at weight ", (nTrustWeight.load() + nBlockWeight.load()),
                                " and stake rate ", nStakeRate.load());

        /* Search for the proof of stake hash solution until it mines a block, minter is stopped,
         * or network generates a new block (minter must start over with new candidate)
         */
        while(!StakeMinter::fstopMinter.load() && !config::fShutdown.load() && hashLastBlock == TAO::Ledger::ChainState::hashBestChain.load())
        {
            /* Update the block time for difficulty accuracy. */
            candidateBlock.UpdateTime();
            uint32_t nCurrentBlockTime = candidateBlock.GetBlockTime() - candidateBlock.vtx[0].nTime; // How long have we been working on this block

            /* If just starting on block, wait */
            if(nCurrentBlockTime == 0)
            {
                runtime::sleep(1);
                continue;
            }

            /* Calculate the new Efficiency Threshold for the next nonce. */
            double nThreshold = (nCurrentBlockTime * 100.0) / candidateBlock.nNonce;

            /* If energy efficiency requirement exceeds threshold, wait and keep trying with the same nonce value until it threshold increases */
            if(nThreshold < nRequired)
            {
                runtime::sleep(10);
                continue;
            }

            /* Log every 1000 attempts */
            if(candidateBlock.nNonce % 1000 == 0)
                debug::log(3, FUNCTION, "Threshold ", nThreshold, " exceeds required ", nRequired,", mining Proof of Stake with nonce ", candidateBlock.nNonce);

            /* Handle if block is found. */
            uint1024_t stakeHash = candidateBlock.StakeHash();
            if(stakeHash < nHashTarget)
            {
                debug::log(0, FUNCTION, "Found new stake hash ", stakeHash.ToString().substr(0, 20));

                ProcessMinedBlock();
                break;
            }

            /* Increment nonce for next iteration. */
            ++candidateBlock.nNonce;
        }

        return;
    }


    bool StakeMinter::ProcessMinedBlock()
    {
        /* Add the transactions into the block from memory pool, but only if not Genesis (Genesis block for trust key has no transactions except coinstake). */
        if(!candidateBlock.vtx[0].IsGenesis())
            AddTransactions(candidateBlock.vtx);

        /* Build the Merkle Root. */
        std::vector<uint512_t> vMerkleTree;
        for(const auto& tx : candidateBlock.vtx)
            vMerkleTree.push_back(tx.GetHash());

        candidateBlock.hashMerkleRoot = candidateBlock.BuildMerkleTree(vMerkleTree);

        /* Sign the block. */
        if(!SignBlock(candidateBlock, *pStakingWallet))
            return debug::error(FUNCTION, "Failed to sign block");

        /* Check the block. */
        if(!candidateBlock.Check())
            return debug::error(FUNCTION, "Check block failed");

        /* Check the work and process the block.
         *
         * After a successful check, CheckWork() calls LLP::Process() for the new block.
         * That method will call LegacyBlock::Accept() and BlockState::Accept()
         * After all is accepted, BlockState::Accept() will call BlockState::SetBest()
         * to set the new best chain. This final method relays the new block to the
         * network. It also connects the block.
         */
        if(!CheckWork(candidateBlock, *pStakingWallet))
            return debug::error(FUNCTION, "Check work failed");

        if(pReservedTrustKey != nullptr)
        {
            /* New block was Genesis using reserved key.
             * Block processing creates the actual TrustKey and writes it to trust db.
             * Here just need to save it to the wallet for future retrieval.
             */
            pStakingWallet->SetTrustKey(pReservedTrustKey->GetReservedKey());

            /* Marks the key as used by the wallet and removes permanently from key pool. */
            pReservedTrustKey->KeepKey();

            delete pReservedTrustKey;
            pReservedTrustKey = nullptr;

            debug::log(0, FUNCTION, "New trust key generated and stored");
        }

        return true;
    }


    /* Method run on its own thread to oversee stake minter operation. */
    void StakeMinter::StakeMinterThread(StakeMinter* pStakeMinter)
    {

        debug::log(0, FUNCTION, "Stake Minter Started");
        pStakeMinter->nSleepTime = 5000;
        bool fLocalTestnet = config::fTestNet && config::GetBoolArg("-nodns", false);

        /* If the system is still syncing/connecting on startup, wait to run minter */
        while((TAO::Ledger::ChainState::Synchronizing() || (LLP::LEGACY_SERVER->GetConnectionCount() == 0 && !fLocalTestnet))
        		&& !StakeMinter::fstopMinter.load() && !config::fShutdown.load())
        {
            runtime::sleep(pStakeMinter->nSleepTime);
        }

        /* Check stop/shutdown status after wait ends */
        if(StakeMinter::fstopMinter.load() || config::fShutdown.load())
            return;

        debug::log(0, FUNCTION, "Stake Minter Initialized");

        pStakeMinter->nSleepTime = 1000;

        /* Minting thread will continue repeating this loop until shutdown */
        while(!StakeMinter::fstopMinter.load() && !config::fShutdown.load())
        {
            runtime::sleep(pStakeMinter->nSleepTime);

            /* Save the current best block hash immediately after sleep in case it changes while we do setup */
            pStakeMinter->hashLastBlock = TAO::Ledger::ChainState::hashBestChain.load();

            /* Check stop/shutdown status after wakeup */
            if(StakeMinter::fstopMinter.load() || config::fShutdown.load())
            	continue;

            /* Reload trust key each block iteration to assure we get updates after new block or orphan disconnect.
             * Don't need to do this if staking Genesis.
             */
            if(pStakeMinter->pReservedTrustKey == nullptr)
                pStakeMinter->FindTrustKey();

            /* Set up the candidate block the minter is attempting to mine */
            if(!pStakeMinter->CreateCandidateBlock())
                continue;

            /* Updates weights for new candidate block */
            if(!pStakeMinter->CalculateWeights())
                continue;

            /* Attempt to mine the current proof of stake block */
            pStakeMinter->MineProofOfStake();

        }

        /* If get here because fShutdown set, have to wait for join. Join is issued in StopStakeMinter, which needs to be called by shutdown process, too. */
        while(!StakeMinter::fstopMinter.load())
            runtime::sleep(100);


        /* Stop has been issued. Now thread can end. */
    }

}
