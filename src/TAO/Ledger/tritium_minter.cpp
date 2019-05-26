/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/



#include <LLC/include/eckey.h>
#include <LLC/types/bignum.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/API/include/global.h>

#include <TAO/Ledger/types/tritium_minter.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/stake.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/unpack.h>

#include <Util/include/config.h>
#include <Util/include/convert.h>
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
        std::atomic<bool> TritiumMinter::fisStarted(false);
        std::atomic<bool> TritiumMinter::fstopMinter(false);

        std::thread TritiumMinter::tritiumMinterThread;


        TritiumMinter& TritiumMinter::GetInstance()
        {
            static TritiumMinter tritiumMinter;

            return tritiumMinter;
        }


        /* Tests whether or not the stake minter is currently running. */
        bool TritiumMinter::IsStarted() const
        {
            return TritiumMinter::fisStarted.load();
        }


        /* Start the stake minter. */
        bool TritiumMinter::StartStakeMinter()
        {
            if (TritiumMinter::fisStarted.load())
                return debug::error(0, FUNCTION, "Attempt to start Stake Minter when already started.");

            /* Disable stake minter if not in sessionless mode. */
            if (config::fAPISessions.load())
            {
                debug::log(0, FUNCTION, "Stake minter disabled when use API sessions (multiuser).");
                return false;
            }

            /* Check that stake minter is configured to run.
             * Stake Minter default is to run for non-server and not to run for server
             */
            if ((!config::fServer && !config::GetBoolArg("-stake", true)) || (config::fServer && !config::GetBoolArg("-stake", false)))
            {
                debug::log(2, "Stake Minter not configured. Startup cancelled.");
                return false;
            }

            /* Verify that account is unlocked and can mint. */
            if (!CheckUser())
            {
                debug::log(0, FUNCTION, "Cannot start stake minter.");
                return false;
            }

            /* Ensure stop flag is reset or thread will immediately exit */
            TritiumMinter::fstopMinter.store(false);

            TritiumMinter::tritiumMinterThread = std::thread(TritiumMinter::TritiumMinterThread, this);

            TritiumMinter::fisStarted.store(true);

            return true;
        }


        /* Stop the stake minter. */
        bool TritiumMinter::StopStakeMinter()
        {
            if (TritiumMinter::fisStarted.load())
            {
                debug::log(0, FUNCTION, "Shutting down Stake Minter");

                /* Set signal flag to tell minter thread to stop */
                TritiumMinter::fstopMinter.store(true);

                /* Wait for minter thread to stop */
                TritiumMinter::tritiumMinterThread.join();

                TritiumMinter::fisStarted.store(false);
                TritiumMinter::fstopMinter.store(false);
                return true;
            }

            return false;
        }


        /* Verify user account unlocked for minting. */
        bool TritiumMinter::CheckUser()
        {
            /* Check whether unlocked account available. */
            if (TAO::API::users->Locked())
            {
                debug::log(0, FUNCTION, "No unlocked account available for staking");
                return false;
            }

            /* Check that the account is unlocked for minting */
            if (!TAO::API::users->CanMint())
            {
                debug::log(0, FUNCTION, "Account has not been unlocked for minting");
                return false;
            }

            return true;
        }


        /*  Retrieves the most recent stake transaction for a user account. */
        bool TritiumMinter::FindTrustAccount(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user)
        {

            /*
             * Every user account should have a corresponding trust account created with its first transaction.
             * Upon staking Genesis, that account is indexed into the register DB and is directly retrievable.
             * Pre-Genesis, we have to find its creation in the account transaction history.
             *
             * If this process fails in any way, the user account has no trust account available and cannot stake.
             * This is logged as an error and the stake minter should be suspended pending stop/shutdown.
             */

            if (LLD::regDB->HasTrust(user->Genesis()))
            {
                isGenesis = false;

                /* Staking Trust transaction */

                /* Retrieve the trust account register */
                if (!LLD::regDB->ReadTrust(user->Genesis(), trustAccount))
                   return debug::error(FUNCTION, "Stake Minter unable to retrieve trust account.");

                if (!trustAccount.Parse())
                    return debug::error(FUNCTION, "Stake Minter unable to parse trust account register.");

                return true;
            }
            else
            {
                isGenesis = true;

                /* Staking Genesis for trust account */
                uint512_t hashLast = 0;
                bool fTrustAccountFound = false;

                /* Get the most recent tx hash for the user account. */
                if(!LLD::legDB->ReadLast(user->Genesis(), hashLast))
                    return false;

                /* Loop until find trust account register operation or reach first transaction on user acount (hashLast == 0). */
                while (hashLast != 0)
                {
                    /* Get the transaction for the current hashLast. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::legDB->ReadTx(hashLast, tx))
                        return false;

                    /* Attempt to unpack register operation from the transaction */
                    uint256_t hashAddressTemp;
                    TAO::Register::Object reg;

                    if (TAO::Register::Unpack(tx, reg, hashAddressTemp))
                    {
                        /* Transaction contains a register operation. Check if it is the trust account register for the user account */
                        if (reg.Parse() && reg.Standard() == TAO::Register::OBJECTS::TRUST)
                        {
                            /* Found the trust account register transaction */
                            hashAddress = hashAddressTemp;
                            trustAccount = reg;

                            fTrustAccountFound = true;
                            break;
                        }
                    }

                    /* Trust account creation not found, yet, iterate to next previous user tx */
                    hashLast = tx.hashPrevTx;
                }

                if (fTrustAccountFound)
                {
                    /* Validate that this is a new trust account staking Genesis */

                    /* Check that there is no stake. */
                    if(trustAccount.get<uint64_t>("stake") != 0)
                        return debug::error(FUNCTION, "Cannot create Genesis with already existing stake");

                    /* Check that there is no trust. */
                    if(trustAccount.get<uint64_t>("trust") != 0)
                        return debug::error(FUNCTION, "Cannot create Genesis with already existing trust");

                    return true;
                }
            }

            return false;
        }


        /** Retrieves the most recent stake transaction for a user account. */
        bool TritiumMinter::FindLastStake(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, TAO::Ledger::Transaction& tx)
        {
            uint512_t hashLast = 0;

            /* Get the most recent tx hash for the user account. */
            if (!LLD::legDB->ReadLast(user->Genesis(), hashLast))
                return false;

            /* Loop until find stake transaction or reach first transaction on user acount (hashLast == 0). */
            while (hashLast != 0)
            {
                /* Get the transaction for the current hashLast. */
                TAO::Ledger::Transaction txCheck;
                if(!LLD::legDB->ReadTx(hashLast, txCheck))
                    return false;

                /* Test whether the transaction contains a staking operation */
                if (TAO::Register::Unpack(txCheck, TAO::Operation::OP::TRUST) || TAO::Register::Unpack(txCheck, TAO::Operation::OP::GENESIS))
                {
                    /* Found last stake transaction. */
                    tx = txCheck;
                    return true;
                }

                /* Stake tx not found, yet, iterate to next previous user tx */
                hashLast = txCheck.hashPrevTx;
            }

            return false;
        }


        /* Creates a new legacy block that the stake minter will attempt to mine via the Proof of Stake process. */
        bool TritiumMinter::CreateCandidateBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN)
        {
            static const uint32_t stakingChannel = (uint32_t)0;

            static uint32_t nWaitCounter = 0; //Prevents log spam during wait period

            /* Reset any prior value of trust score, block age, and stake update amount */
            nTrust = 0;
            nBlockAge = 0;
            nTimeLastStake = 0;

            /* Create the block to work on */
            candidateBlock = TritiumBlock();

            /* Create the base Tritium block. */
            if (!TAO::Ledger::CreateBlock(user, strPIN, stakingChannel, candidateBlock))
                return debug::error(FUNCTION, "Unable to create candidate block");

            if (!isGenesis)
            {
                /* Staking Trust for existing trust account */

                if (trustAccount.get<uint64_t>("stake") == 0)
                {
                    /* Trust account has no stake balance. Increase sleep time to wait for balance. */
                    nSleepTime = 5000;

                    /* Update log every 60 iterations (5 minutes) */
                    if ((nWaitCounter % 60) == 0)
                        debug::log(0, FUNCTION, "Stake Minter: Trust account has no stake balance.");

                    ++nWaitCounter;

                    return false;
                }

                /* Get the previous stake tx for the trust account. */
                TAO::Ledger::Transaction txLast;
                if (!FindLastStake(user, txLast))
                    return debug::error(FUNCTION, "Failed to get last stake for trust account");

                /* Get the block containing the last stake tx for the trust account. */
                TAO::Ledger::BlockState stateLast;
                if(!LLD::legDB->ReadBlock(txLast.GetHash(), stateLast))
                    return debug::error(FUNCTION, "Failed to get last block for trust account");

                nTimeLastStake = stateLast.GetBlockTime();

                /* Enforce the minimum stake transaction interval. (current height is candidate height - 1) */
                const uint32_t nCurrentInterval = candidateBlock.nHeight - stateLast.nHeight;

                if (nCurrentInterval <= MinStakeInterval())
                {
                    /* Below minimum interval for generating stake blocks. Increase sleep time until can continue normally. */
                    nSleepTime = 5000; //5 second wait is reset below (can't sleep too long or will hang until wakes up on shutdown)

                    /* Update log every 60 iterations (5 minutes) */
                    if ((nWaitCounter % 60) == 0)
                        debug::log(0, FUNCTION, "Stake Minter: Too soon after mining last stake block. ",
                                   (MinStakeInterval() - nCurrentInterval), " blocks remaining until staking available.");

                    ++nWaitCounter;

                    return false;
                }

                /* Calculate the new trust score. */
                uint64_t nTrustPrev = trustAccount.get<uint64_t>("trust");
                uint64_t nStake = trustAccount.get<uint64_t>("stake");

                /* Calculate time since the last stake block (block age = age of previous trust block). */
                nBlockAge = TAO::Ledger::ChainState::stateBest.load().GetBlockTime() - nTimeLastStake;

                /* Calculate the new trust score */
                nTrust = TrustScore(nTrustPrev, nStake, nBlockAge);

                /* Initialize block producer for Trust operation with hashLastTrust, new trust score.
                 * The coinstake reward will be added based on time when block is found.
                 */
                candidateBlock.producer << (uint8_t)TAO::Operation::OP::TRUST << txLast.GetHash() << nTrust;
            }
            else
            {
                /* Looking to stake Genesis for new trust account */

                /* Validate that have assigned balance for Genesis */
                if (trustAccount.get<uint64_t>("balance") == 0)
                {
                    /* Wallet has no balance, or balance unavailable for staking. Increase sleep time to wait for balance. */
                    nSleepTime = 5000;

                    /* Update log every 60 iterations (5 minutes) */
                    if ((nWaitCounter % 60) == 0)
                        debug::log(0, FUNCTION, "Stake Minter: Trust account has no balance assigned for Genesis.");

                    ++nWaitCounter;

                    return false;
                }

                /* Initialize block producer for Genesis operation with hashAddress of trust account register.
                 * The coinstake reward will be added based on time when block is found.
                 */
                candidateBlock.producer << (uint8_t)TAO::Operation::OP::GENESIS << hashAddress;

            }

            /* Reset sleep time on successful completion */
            if (nSleepTime == 5000)
            {
                /* Reset wait period setting */
                fIsWaitPeriod.store(false);

                /* Reset sleep time after coin age meets requirement. */
                nSleepTime = 1000;
                nWaitCounter = 0;
            }

            /* Update display stake rate */
            nStakeRate.store(StakeRate(nTrust, isGenesis));

            return true;
        }


        /* Calculates the Trust Weight and Block Weight values for the current trust account and candidate block. */
        bool TritiumMinter::CalculateWeights()
        {
            /* Use local variables for calculations, then set instance variables at the end */
            double nTrustWeightCurrent = 0.0;
            double nBlockWeightCurrent = 0.0;

            static uint32_t nWaitCounter = 0; //Prevents log spam during wait period

            nTrust = 0;

            if (!isGenesis)
            {
                /* Weight for Trust transactions combines trust weight and block weight. */
                nTrustWeightCurrent = TrustWeight(nTrust);

                nBlockWeightCurrent = BlockWeight(nBlockAge);
            }
            else
            {
                /* Weights for Genesis transactions only use trust weight with its value based on average coin age. */

                /* For Genesis, coin age is current block time less last time trust account register was updated.
                 * This means that, if someone adds more balance to trust account while staking Genesis, coin age is reset and they must wait
                 * the full time before staking Genesis again.
                 */
                uint64_t nCoinAge = candidateBlock.GetBlockTime() - trustAccount.nTimestamp;

                /* Genesis has to wait for coin age to reach minimum. */
                if (nCoinAge < MinCoinAge())
                {
                    /* Record that stake minter is in wait period */
                    fIsWaitPeriod.store(true);

                    /* Increase sleep time to wait for coin age to meet requirement (can't sleep too long or will hang until wakes up on shutdown) */
                    nSleepTime = 5000;

                    /* Update log every 60 iterations (5 minutes) */
                    if ((nWaitCounter % 60) == 0)
                    {
                        uint64_t nRemainingWaitTime = (MinCoinAge() - nCoinAge) / 60; //minutes

                        debug::log(0, FUNCTION, "Stake Minter: Stake balance is immature. ", nRemainingWaitTime, " minutes remaining until staking available.");
                    }

                    ++nWaitCounter;

                    return false;
                }
                else if (nSleepTime == 5000)
                {
                    /* Reset wait period setting */
                    fIsWaitPeriod.store(false);

                    /* Reset sleep time after coin age meets requirement. */
                    nSleepTime = 1000;
                    nWaitCounter = 0;
                }

                nTrustWeightCurrent = GenesisWeight(nCoinAge);

                /* Block Weight remains zero while staking for Genesis */
                nBlockWeightCurrent = 0.0;
            }

            /* Update minter settings */
            nBlockWeight.store(nBlockWeightCurrent);
            nTrustWeight.store(nTrustWeightCurrent);

            return true;
        }


        /* Attempt to solve the hashing algorithm at the current staking difficulty for the candidate block */
        void TritiumMinter::MineProofOfStake(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN)
        {
            /* Calculate the minimum Required Energy Efficiency Threshold.
             * Minter can only mine Proof of Stake when current threshold exceeds this value.
             *
             * Staking weights (trust and block) reduce the required threshold by reducing the numerator of this calculation.
             * Weight from staking balance (based on nValue out of coinstake) reduces the required threshold by increasing the denominator.
             */
            uint64_t nStake = trustAccount.get<uint64_t>("stake");

            double nRequired = RequiredThreshold(nTrustWeight.load(), nBlockWeight.load(), nStake);

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
            while (!TritiumMinter::fstopMinter.load() && !config::fShutdown.load() && hashLastBlock == TAO::Ledger::ChainState::hashBestChain.load())
            {
                /* Update the block time for difficulty accuracy. */
                candidateBlock.UpdateTime();
                uint32_t nCurrentBlockTime = candidateBlock.GetBlockTime() - candidateBlock.producer.nTimestamp; // How long have we been working on this block

                /* If just starting on block, wait */
                if (nCurrentBlockTime == 0)
                {
                    runtime::sleep(1);
                    continue;
                }

                /* Calculate the new Efficiency Threshold for the next nonce.
                 * To stake, this value must be larger than required threshhold.
                 * Block time increases the value while nonce decreases it.
                 */
                double nThreshold = CurrentThreshold(nCurrentBlockTime, candidateBlock.nNonce);

                /* If threshhold is not larger than required, wait and keep trying with the same nonce value until threshold increases */
                if(nThreshold < nRequired)
                {
                    runtime::sleep(10);
                    continue;
                }

                /* Log every 1000 attempts */
                if (candidateBlock.nNonce % 1000 == 0)
                    debug::log(3, FUNCTION, "Threshold ", nThreshold, " exceeds required ", nRequired,", mining Proof of Stake with nonce ", candidateBlock.nNonce);

                /* Handle if block is found. */
                uint1024_t stakeHash = candidateBlock.StakeHash();
                if (stakeHash <= nHashTarget)
                {
                    debug::log(0, FUNCTION, "Found new stake hash ", stakeHash.ToString().substr(0, 20));

                    ProcessMinedBlock(user, strPIN);
                    break;
                }

                /* Increment nonce for next iteration. */
                ++candidateBlock.nNonce;
            }

            return;
        }


        bool TritiumMinter::ProcessMinedBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN)
        {
            /* Used to calculate coinstake reward */
            uint64_t nStake = trustAccount.get<uint64_t>("stake");
            uint64_t nStakeTime = 0;
            uint64_t nCoinstakeReward = 0;

            /* Calculate the coinstake reward */
            if (!isGenesis)
            {
                nStakeTime = candidateBlock.GetBlockTime() - nTimeLastStake;
                nCoinstakeReward = CoinstakeReward(nStake, nStakeTime, nTrust, isGenesis);
            }
            else
            {
                nStakeTime = candidateBlock.GetBlockTime() - trustAccount.nTimestamp; //"coin age";
                nCoinstakeReward = CoinstakeReward(nStake, nStakeTime, 0, isGenesis);
            }

            /* Add coinstake reward to producer */
            candidateBlock.producer << nCoinstakeReward;

            /* Execute operation pre- and post-state.
             *   - for OP::TRUST, the operation can obtain the trust account from candidateBlock.producer.hashGenesis
             *   - for OP::GENESIS, the hashAddress of the trust account register is encoded into the block producer
             *
             * This process also calculates the appropriate stake reward and encodes it into the post-state.
             */
            if (!TAO::Operation::Execute(candidateBlock.producer, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
                return debug::error(FUNCTION, "Operation layer failed to execute pre-state/post-state for coinstake transaction");

            /* Sign the block producer */
            candidateBlock.producer.Sign(user->Generate(candidateBlock.producer.nSequence, strPIN));

            /* Add transactions into the block from memory pool, but only for Trust (Genesis block for trust account has no transactions except coinstake producer). */
            if (!isGenesis)
                AddTransactions(candidateBlock);

            /* Build the Merkle Root. */
            std::vector<uint512_t> vHashes;
            vHashes.push_back(candidateBlock.producer.GetHash()); //producer is not part of vtx

            for (const auto& txEntry : candidateBlock.vtx)
                vHashes.push_back(txEntry.second);

            candidateBlock.hashMerkleRoot = candidateBlock.BuildMerkleTree(vHashes);

            /* Print the newly found block. */
            candidateBlock.print();

            /* Sign the block. */
            if (!SignBlock(user, strPIN))
                return false;

            /* Check the block. */
            if (!candidateBlock.Check())
                return debug::error(FUNCTION, "Check block failed");

            /* Check the stake. */
            if (!candidateBlock.CheckStake())
                return debug::error(FUNCTION, "Check stake failed");

            /* Check block for difficulty requirement. */
            if (!candidateBlock.VerifyWork())
                return debug::error(FUNCTION, "Verify work failed");

            /* Log block found */
            if (config::GetArg("-verbose", 0) > 0)
            {
                std::string timestampString(convert::DateTimeStrFormat(runtime::unifiedtimestamp()));

                debug::log(1, FUNCTION, "Nexus Stake Minter: New nPoS channel block found at unified time ", timestampString);
                debug::log(1, " blockHash: ", candidateBlock.StakeHash().ToString().substr(0, 30), " block height: ", candidateBlock.nHeight);
            }

            if (candidateBlock.hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load())
            {
                debug::log(0, FUNCTION, "Generated block is stale");
                return false;
            }

            /* Process the block and relay to network if it gets accepted into main chain.
             * This method will call TritiumBlock::Accept() and BlockState::Index()
             * After all is approved, BlockState::Index() will call BlockState::SetBest()
             * to set the new best chain. This final method relays the new block to the
             * network.
             */
            if (!LLP::TritiumNode::Process(candidateBlock, nullptr))
            {
                debug::log(0, FUNCTION, "Generated block not accepted");
                return false;
            }

            if (isGenesis)
                isGenesis = false;

            return true;
        }


        /* Sign a candidate block after it is successfully mined. */
        bool TritiumMinter::SignBlock(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& strPIN)
        {
            /* Get the sigchain and the PIN. */
            /* Sign the submitted block */
            std::vector<uint8_t> vBytes = user->Generate(candidateBlock.producer.nSequence, strPIN).GetBytes();
            LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

            /* Switch based on signature type. */
            switch(candidateBlock.producer.nKeyType)
            {
                /* Support for the FALCON signature scheeme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the secret parameter. */
                    if(!key.SetSecret(vchSecret, true))
                        return debug::error(FUNCTION, "TritiumMiner: Unable to set key for signing Tritium Block ", candidateBlock.hashMerkleRoot.ToString().substr(0, 20));

                    /* Generate the signature. */
                    if(!candidateBlock.GenerateSignature(key))
                        return debug::error(FUNCTION, "TritiumMiner: Unable to sign Tritium Block ", candidateBlock.hashMerkleRoot.ToString().substr(0, 20));

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the secret parameter. */
                    if(!key.SetSecret(vchSecret, true))
                        return debug::error(FUNCTION, "TritiumMiner: Unable to set key for signing Tritium Block ", candidateBlock.hashMerkleRoot.ToString().substr(0, 20));

                    /* Generate the signature. */
                    if(!candidateBlock.GenerateSignature(key))
                        return debug::error(FUNCTION, "TritiumMiner: Unable to sign Tritium Block ", candidateBlock.hashMerkleRoot.ToString().substr(0, 20));

                    break;
                }

                default:
                    return debug::error(FUNCTION, "unknown signature type");
            }

            return true;
         }


        /* Method run on its own thread to oversee stake minter operation. */
        void TritiumMinter::TritiumMinterThread(TritiumMinter* pTritiumMinter)
        {

            debug::log(0, FUNCTION, "Stake Minter Started");
            pTritiumMinter->nSleepTime = 5000;
            bool fLocalTestnet = config::fTestNet.load() && config::GetBoolArg("-nodns", false);

            /* If the system is still syncing/connecting on startup, wait to run minter */
            while ((TAO::Ledger::ChainState::Synchronizing() || (LLP::TRITIUM_SERVER->GetConnectionCount() == 0 && !fLocalTestnet))
                    && !TritiumMinter::fstopMinter.load() && !config::fShutdown.load())
            {
                runtime::sleep(pTritiumMinter->nSleepTime);
            }

            /* Check stop/shutdown status after wait ends */
            if (TritiumMinter::fstopMinter.load() || config::fShutdown.load())
                return;

            debug::log(0, FUNCTION, "Stake Minter Initialized");

            pTritiumMinter->nSleepTime = 1000;

            /* Minting thread will continue repeating this loop until stop minter or shutdown */
            while (!TritiumMinter::fstopMinter.load() && !config::fShutdown.load())
            {
                runtime::sleep(pTritiumMinter->nSleepTime);

                /* Check stop/shutdown status after wakeup */
                if (TritiumMinter::fstopMinter.load() || config::fShutdown.load())
                    continue;

                /* Save the current best block hash immediately in case it changes while we do setup */
                pTritiumMinter->hashLastBlock = TAO::Ledger::ChainState::hashBestChain.load();

                /* Check that user account still unlocked for minting (locking should stop minter, but still verify) */
                if (!pTritiumMinter->CheckUser())
                    break;

                /* Get the active, unlocked sigchain. Requires session 0 */
                memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = TAO::API::users->GetAccount(0);
                if (!user)
                {
                    debug::error(0, FUNCTION, "Stake minter could not retrieve the unlocked signature chain.");
                    break;
                }

                SecureString strPIN = TAO::API::users->GetActivePin();

                /* Retrieve the latest trust account data */
                if (!pTritiumMinter->FindTrustAccount(user))
                    break;

                /* Set up the candidate block the minter is attempting to mine */
                if (!pTritiumMinter->CreateCandidateBlock(user, strPIN))
                    continue;

                /* Updates weights for new candidate block */
                if (!pTritiumMinter->CalculateWeights())
                    continue;

                /* Attempt to mine the current proof of stake block */
                pTritiumMinter->MineProofOfStake(user, strPIN);

            }

            /* If break because cannot continue (error retrieving user account or FindTrust failed) must wait for stop or shutdown */
            while (!TritiumMinter::fstopMinter.load() && !config::fShutdown.load())
                runtime::sleep(pTritiumMinter->nSleepTime);

            /* If get here because fShutdown set, have to wait for join. Join is issued in StopStakeMinter, which needs to be called by shutdown process, too. */
            while (!TritiumMinter::fstopMinter.load())
                runtime::sleep(100);

            /* Stop has been issued. Now thread can end. */
        }

    }
}
