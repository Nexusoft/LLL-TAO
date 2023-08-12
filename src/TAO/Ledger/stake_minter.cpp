/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/stake_minter.h>
#include <TAO/Ledger/types/tritium_minter.h>

#include <LLC/hash/SK.h>
#include <LLC/include/eckey.h>
#include <LLC/types/bignum.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/stake.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/address.h>

#include <Util/include/config.h>
#include <Util/include/convert.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger namespace. */
    namespace Ledger
    {

        /* Initialize static variables */
        std::atomic<bool> StakeMinter::fStarted(false);
        std::atomic<bool> StakeMinter::fStop(false);


        /* Retrieves the stake minter instance to use for staking. */
        StakeMinter& StakeMinter::GetInstance()
        {
            return TritiumMinter::GetInstance();
        }


        /** Constructor **/
        StakeMinter::StakeMinter()
        : hashLastBlock(0)
        , nSleepTime(1000)
        , fWait(false)
        , nWaitTime(0)
        , nTrustWeight(0.0)
        , nBlockWeight(0.0)
        , nStakeRate(0.0)
        , account()
        , stateLast()
        , block()
        , fGenesis(false)
        , nTrust(0)
        , nBlockAge(0)
        {
        }


        /* Tests whether or not the stake minter is currently running. */
        bool StakeMinter::IsStarted() const
        {
            return StakeMinter::fStarted.load();
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


        /* Retrieves the current staking reward rate */
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
            return fWait.load();
        }


        /* When IsWaitPeriod() is true, this method returns the remaining wait time before staking is active. */
        uint64_t StakeMinter::GetWaitTime() const
        {

            if(!fWait.load())
                return 0;

            return nWaitTime.load();
        }


        /* Verify user account unlocked for minting. */
        bool StakeMinter::CheckUser()
        {
            /* Check that the account is unlocked for staking */
            if(!TAO::API::Authentication::Unlocked(PinUnlock::STAKING))
            {
                debug::log(0, FUNCTION, "Account has not been unlocked for staking");
                return false;
            }

            return true;
        }


        /*  Retrieves the most recent stake transaction for a user account. */
        bool StakeMinter::FindTrustAccount(const uint256_t& hashGenesis)
        {
            bool fIndexed;

            /* Retrieve the trust account for the user's hashGenesis */
            if(!TAO::Ledger::FindTrustAccount(hashGenesis, account, fIndexed))
                return false;

            fGenesis = !fIndexed;

            if(fGenesis)
            {
                /* Check that there is no stake. */
                if(account.get<uint64_t>("stake") != 0)
                    return debug::error(FUNCTION, "Cannot create Genesis with already existing stake");

                /* Check that there is no trust. Preset trust is allowed for testing on testnet when -trustboost is used. */
                if(account.get<uint64_t>("trust") !=
                ((config::fTestNet.load() && config::GetBoolArg("-trustboost")) ? TAO::Ledger::ONE_YEAR : 0))
                    return debug::error(FUNCTION, "Cannot create Genesis with already existing trust");
            }

            return true;
        }


        /** Retrieves the most recent stake transaction for a user account. */
        bool StakeMinter::FindLastStake(const uint256_t& hashGenesis, uint512_t& hashLast)
        {
            if(LLD::Ledger->ReadStake(hashGenesis, hashLast))
                return true;

            /* If last stake is not directly available, search for it */
            Transaction tx;
            if(TAO::Ledger::FindLastStake(hashGenesis, tx))
            {
                /* Use the hashLast of the found last stake */
                hashLast = tx.GetHash();

                return true;
            }

            return false;
        }


        /* Identifies any pending stake change request and populates the appropriate instance data. */
        bool StakeMinter::CheckStakeChange(const uint256_t& hashGenesis, const uint512_t hashLast,
                                            TAO::Ledger::StakeChange &tStakeChange)
        {
            /* Check for pending stake change request */
            try
            {
                /* Attempt to read the stake change from the disk. */
                if(!LLD::Local->ReadStakeChange(hashGenesis, tStakeChange))
                    return false;
            }
            catch(const std::exception& e) { return debug::error(FUNCTION, "obsolete serialization format for stake change request"); }

            /* Check if this stake change has processed already. */
            if(tStakeChange.fProcessed)
                return false;

            /* Verify stake change request is current version supported by minter */
            if(tStakeChange.nVersion != 1)
                return debug::error(FUNCTION, "Stake change request is unsupported version");

            /* Verify current hashGenesis matches requesting value */
            if(hashGenesis != tStakeChange.hashGenesis)
                return debug::error(FUNCTION, "Stake change request hashGenesis mismatch");

            /* Verify that no blocks have been staked since the change was requested */
            if(tStakeChange.hashLast != hashLast)
                return debug::error(FUNCTION, "Stake change request is stale");

            /* Check for expired stake change request */
            if(tStakeChange.nExpires != 0 && tStakeChange.nExpires < runtime::unifiedtimestamp())
                return debug::error(FUNCTION, "Stake change request has expired");

            /* Get the crypto register for the current user hashGenesis. */
            const TAO::Register::Address hashCrypto =
                TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

            /* Read our crypto object register. */
            TAO::Register::Object crypto;
            if(!LLD::Register->ReadObject(hashCrypto, crypto))
                return debug::error(FUNCTION, "Missing crypto register");

            /* Check that this is our crypto object register. */
            if(crypto.Standard() != TAO::Register::OBJECTS::CRYPTO)
                return debug::error(FUNCTION, "Invalid crypto register");

            /* Verify the signature on the stake change request */
            uint256_t hashPublic = crypto.get<uint256_t>("auth");
            if(hashPublic != 0) //no auth key disables check
            {
                /* Get hash of public key and set to same type as crypto register hash for verification */
                uint256_t hashCheck = LLC::SK256(tStakeChange.vchPubKey);
                hashCheck.SetType(hashPublic.GetType());

                /* Check the public key to expected authorization key. */
                if(hashCheck != hashPublic)
                {
                    LLD::Local->EraseStakeChange(hashGenesis);
                    return debug::error(FUNCTION, "Invalid public key on stake change request");
                }

                /* Switch based on signature type. */
                switch(hashPublic.GetType())
                {
                    /* Support for the FALCON signature scheeme. */
                    case TAO::Ledger::SIGNATURE::FALCON:
                    {
                        /* Create the FL Key object. */
                        LLC::FLKey key;

                        /* Set the public key and verify. */
                        key.SetPubKey(tStakeChange.vchPubKey);
                        if(!key.Verify(tStakeChange.GetHash().GetBytes(), tStakeChange.vchSig))
                            return debug::error(FUNCTION, "Invalid signature on stake change request");

                        break;
                    }

                    /* Support for the BRAINPOOL signature scheme. */
                    case TAO::Ledger::SIGNATURE::BRAINPOOL:
                    {
                        /* Create EC Key object. */
                        LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                        /* Set the public key and verify. */
                        key.SetPubKey(tStakeChange.vchPubKey);
                        if(!key.Verify(tStakeChange.GetHash().GetBytes(), tStakeChange.vchSig))
                            return debug::error(FUNCTION, "Invalid signature on stake change request");

                        break;
                    }

                    default:
                        return debug::error(FUNCTION, "Invalid signature type on stake change request");
                }
            }

            /* Check that we aren't out of range of our total stake. */
            const uint64_t nStake = account.get<uint64_t>("stake");
            if(tStakeChange.nAmount < 0 && int64_t(0 - tStakeChange.nAmount) > nStake)
            {
                debug::warning(FUNCTION, "Cannot unstake more than current stake, using current stake amount.");
                tStakeChange.nAmount = 0 - nStake;
            }

            /* Check that we aren't adding more than our current available balance. */
            const uint64_t nBalance = account.get<uint64_t>("balance");
            if(tStakeChange.nAmount > 0 && tStakeChange.nAmount > nBalance)
            {
                debug::warning(FUNCTION, "Cannot add more than current balance to stake, using current balance.");
                tStakeChange.nAmount = nBalance;
            }

            /* Check for empty value. */
            if(tStakeChange.nAmount == 0)
                return debug::error(FUNCTION, "cannot adjust stake amount for zero value");

            return true;
        }


        /* Creates a new legacy block that the stake minter will attempt to mine via the Proof of Stake process. */
        bool StakeMinter::CreateCandidateBlock()
        {
            /* Reset any prior value of trust score, block age, and stake update amount */
            nTrust = 0;
            nBlockAge = 0;

            /* Create the block to work on */
            block = TritiumBlock();

            /* Lock the sigchain to create block. */
            SecureString strPIN;
            {
                RECURSIVE(TAO::API::Authentication::Unlock(strPIN, PinUnlock::STAKING));

                /* Extract our credentials. */
                const auto& pCredentials =
                    TAO::API::Authentication::Credentials();

                /* Create the base Tritium block. */
                if(!TAO::Ledger::CreateStakeBlock(pCredentials, strPIN, block, fGenesis))
                    return debug::error(FUNCTION, "Unable to create candidate block");

                /* Add a coinstake producer to the new candidate block */
                if(!CreateCoinstake(pCredentials->Genesis()))
                    return false;
            }


            return true;
        }


        /* Calculates the Trust Weight and Block Weight values for the current trust account and candidate block. */
        bool StakeMinter::CalculateWeights()
        {
            static uint32_t nCounter = 0; //Prevents log spam during wait period

            /* Use local variables for calculations, then set instance variables at the end */
            cv::softdouble nTrustWeightCurrent = cv::softdouble(0.0);
            cv::softdouble nBlockWeightCurrent = cv::softdouble(0.0);

            if(!fGenesis)
            {
                /* Weight for Trust transactions combines trust weight and block weight. */
                nTrustWeightCurrent = TrustWeight(nTrust);
                nBlockWeightCurrent = BlockWeight(nBlockAge);
            }
            else
            {
                /* Weights for Genesis transactions only use trust weight with its value based on coin age. */

                /* For Genesis, coin age is current block time less last time trust account register was updated.
                 * This means that, if someone adds more balance to trust account while staking Genesis, coin age is reset and they
                 * must wait the full time before staking Genesis again.
                 */
                uint64_t nAge = block.GetBlockTime() - account.nModified;
                const uint64_t nCoinAgeMin = MinCoinAge();

                /* Genesis has to wait for coin age to reach minimum. */
                if(nAge < nCoinAgeMin)
                {
                    /* Record that stake minter is in wait period */
                    fWait.store(true);
                    nWaitTime.store(nCoinAgeMin - nAge);

                    /* Increase sleep time to wait for coin age (can't sleep too long or will hang until wakes up on shutdown) */
                    nSleepTime = 5000;

                    /* Update log every 60 iterations (5 minutes) */
                    if((nCounter % 60) == 0)
                        debug::log(0, FUNCTION, "Stake balance is immature. ",
                            (nWaitTime.load() / 60), " minutes remaining until staking available.");

                    ++nCounter;

                    return false;
                }
                else if(nSleepTime == 5000)
                {
                    /* Reset wait period setting */
                    fWait.store(false);
                    nWaitTime.store(0);

                    /* Reset sleep time after coin age meets requirement. */
                    nSleepTime = 1000;
                    nCounter = 0;
                }

                nTrustWeightCurrent = GenesisWeight(nAge);

                /* Block Weight remains zero while staking for Genesis */
                nBlockWeightCurrent = cv::softdouble(0.0);
            }

            /* Update minter settings */
            nBlockWeight.store(double(nBlockWeightCurrent));
            nTrustWeight.store(double(nTrustWeightCurrent));

            return true;
        }


        /* Verify whether or not a signature chain has met the interval requirement */
        bool StakeMinter::CheckInterval()
        {
            static uint32_t nCounter = 0; //Prevents log spam during wait period
            static const uint32_t nMinInterval = MinStakeInterval(block);

            /* Check the block interval for trust transactions. */
            if(!fGenesis)
            {
                const uint32_t nInterval = block.nHeight - stateLast.nHeight;

                if(nInterval <= nMinInterval)
                {
                    /* Below minimum interval for generating stake blocks. Increase sleep time until can continue normally. */
                    nSleepTime = 5000; //5 second wait is reset below (can't sleep too long or will hang until wakes up on shutdown)

                    /* Update log every 100 iterations */
                    if((nCounter % 100) == 0)
                        debug::log(0, FUNCTION, "Too soon after last stake block. ",
                                   (nMinInterval - nInterval + 1), " blocks remaining until staking available.");

                    ++nCounter;

                    return false;
                }
                else if(nSleepTime == 5000)
                {
                    /* Reset sleep time after interval requirement met */
                    nSleepTime = 1000;
                }
            }

            nCounter = 0;
            return true;
        }


        /* Verify no transactions for same signature chain in the mempool */
        bool StakeMinter::CheckMempool(const uint256_t& hashGenesis)
        {
            static uint32_t nCounter = 0; //Prevents log spam during wait period

            /* Check there are no mempool transactions for this sig chain. */
            if(fGenesis && mempool.Has(hashGenesis))
            {
                /* 5 second wait is reset below (can't sleep too long or will hang until wakes up on shutdown) */
                nSleepTime = 5000;

                /* Update log every 10 iterations (50 seconds, which is average block time) */
                if((nCounter % 10) == 0)
                    debug::log(0, FUNCTION, "Skipping stake genesis for current block as mempool transactions would be orphaned.");

                ++nCounter;

                return false;
            }
            else if(nSleepTime == 5000)
            {
                /* Reset sleep time after mempool clear */
                nSleepTime = 1000;
            }

            nCounter = 0;
            return true;

        }


        /* Attempt to solve the hashing algorithm at the current staking difficulty for the candidate block */
        bool StakeMinter::HashBlock(const cv::softdouble nRequired)
        {
            uint64_t nTimeStart = 0;

            /* Calculate the target value based on difficulty. */
            LLC::CBigNum bnTarget;
            bnTarget.SetCompact(block.nBits);
            const uint1024_t nHashTarget = bnTarget.getuint1024();

            /* Search for the proof of stake hash solution until it mines a block, minter is stopped,
             * or network generates a new block (minter must start over with new candidate)
             */
            while(!StakeMinter::fStop.load() && !config::fShutdown.load()
                && hashLastBlock == TAO::Ledger::ChainState::hashBestChain.load())
            {
                /* Check that the producer(s) won't orphan any new transaction from the same hashGenesis. */
                if(CheckStale())
                {
                    debug::log(2, FUNCTION, "Stake block producer is stale, rebuilding...");
                    break; //Start over with a new block
                }

                /* Update the block time for threshold accuracy. */
                block.UpdateTime();
                uint64_t nTimeCurrent = block.GetBlockTime();
                uint32_t nBlockTime;

                /* How long working on this block */
                nBlockTime = nTimeCurrent - block.producer.nTimestamp;

                /* Start the check interval on first loop iteration */
                if(nTimeStart == 0)
                    nTimeStart = nTimeCurrent;

                /* After 4 second interval, check whether need to break for block updates. */
                else if((nTimeCurrent - nTimeStart) >= 4)
                {
                    if(CheckBreak())
                        return false;

                    /* Start a new interval */
                    nTimeStart = nTimeCurrent;
                }

                /* If just starting on block, wait */
                if(nBlockTime == 0)
                {
                    runtime::sleep(1);
                    continue;
                }

                /* Calculate the new Efficiency Threshold for the current nonce.
                 * To stake, this value must be larger than required threshhold.
                 * Block time increases the value while nonce decreases it.
                 * nNonce = 1 at start of new block.
                 */
                cv::softdouble nThreshold = GetCurrentThreshold(nBlockTime, block.nNonce);

                /* If threshhold not larger than required, wait and keep trying the same nonce value until threshold increases */
                if(nThreshold < nRequired)
                {
                    runtime::sleep(10);
                    continue;
                }

                /* Every 1000 attempts, log progress. */
                if(block.nNonce % 1000 == 0)
                    debug::log(3, FUNCTION, "Threshold ", nThreshold, " exceeds required ",
                        nRequired,", mining Proof of Stake with nonce ", block.nNonce);

                /* Handle if block is found. */
                uint1024_t hashProof = block.StakeHash();
                if(hashProof <= nHashTarget)
                {
                    debug::log(0, FUNCTION, "Found new stake hash ", hashProof.SubString());

                    ProcessBlock();
                    break;
                }

                /* Increment nonce for next iteration. */
                ++block.nNonce;
            }

            return true;
        }


        bool StakeMinter::ProcessBlock()
        {
            /* Calculate coinstake reward for producer.
             * Reward is based on final block time for block. Block time is updated with each iteration so we
             * have deferred reward calculation until after the block is found to get the exact correct amount.
             */
            uint64_t nReward = CalculateCoinstakeReward(block.GetBlockTime());

            /* Add coinstake reward */
            block.producer[0] << nReward;

            /* Execute operation pre- and post-state. */
            if(!block.producer.Build())
                return debug::error(FUNCTION, "Coinstake transaction failed to build");

            /* Build the Merkle Root. */
            std::vector<uint512_t> vHashes;
            for(const auto& item : block.vtx)
                vHashes.push_back(item.second);

            /* Unlock our sigchain and lock when signging. */
            {
                SecureString strPIN;
                RECURSIVE(TAO::API::Authentication::Unlock(strPIN, PinUnlock::STAKING));

                /* Extract our credentials from sessions. */
                const auto& pCredentials =
                    TAO::API::Authentication::Credentials();

                /* Coinstake producer now complete. Sign the transaction. */
                block.producer.Sign(pCredentials->Generate(block.producer.nSequence, strPIN));

                /* producers are not part of vtx, add to vHashes last */
                vHashes.push_back(block.producer.GetHash());
                block.hashMerkleRoot = block.BuildMerkleTree(vHashes);

                /* Sign the block. */
                if(!SignBlock(pCredentials, strPIN))
                    return false;

                /* Print the newly found block. */
                block.print();

                /* Log block found */
                if(config::nVerbose > 0)
                {
                    std::string strTimestamp = std::string(convert::DateTimeStrFormat(runtime::unifiedtimestamp()));

                    debug::log(1, FUNCTION, "Nexus Stake Minter: New nPoS channel block found at unified time ", strTimestamp);
                    debug::log(1, " blockHash: ", block.GetHash().SubString(30), " block height: ", block.nHeight);
                }

                if(block.hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load())
                {
                    debug::log(0, FUNCTION, "Generated block is stale");
                    return false;
                }

                /* Lock the sigchain that is being mined. */
                {
                    /* Process the block and relay to network if it gets accepted */
                    uint8_t nStatus = 0;
                    TAO::Ledger::Process(block, nStatus);

                    /* Check the statues. */
                    if(!(nStatus & PROCESS::ACCEPTED))
                        return debug::error(FUNCTION, "generated block not accepted");
                }
            }

            /* After successfully generated genesis for trust account, reset to genereate trust for continued staking */
            if(fGenesis)
                fGenesis = false;

            return true;
        }


        /* Sign a candidate block after it is successfully mined. */
        bool StakeMinter::SignBlock(const memory::encrypted_ptr<TAO::Ledger::Credentials>& user, const SecureString& strPIN)
        {
            /* Sign the submitted block */
            std::vector<uint8_t> vBytes = user->Generate(block.producer.nSequence, strPIN).GetBytes();
            uint8_t nKeyType = block.producer.nKeyType;

            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(nKeyType)
            {
                /* Support for the FALCON signature scheeme. */
                case TAO::Ledger::SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret parameter. */
                    if(!key.SetSecret(vchSecret))
                        return debug::error(FUNCTION, "StakeMinter: Unable to set key for signing Tritium Block ",
                                            block.GetHash().SubString());

                    /* Generate the signature. */
                    if(!block.GenerateSignature(key))
                        return debug::error(FUNCTION, "StakeMinter: Unable to sign Tritium Block ",
                                            block.GetHash().SubString());

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case TAO::Ledger::SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret parameter. */
                    if(!key.SetSecret(vchSecret, true))
                        return debug::error(FUNCTION, "StakeMinter: Unable to set key for signing Tritium Block ",
                                            block.GetHash().SubString());

                    /* Generate the signature. */
                    if(!block.GenerateSignature(key))
                        return debug::error(FUNCTION, "StakeMinter: Unable to sign Tritium Block ",
                                            block.GetHash().SubString());

                    break;
                }

                default:
                    return debug::error(FUNCTION, "TritiumMinter: Unknown signature type");
            }

            return true;
         }

    } // End Ledger namespace
} // End TAO namespace
