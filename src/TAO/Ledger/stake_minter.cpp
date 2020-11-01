/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/stake_minter.h>

#include <LLC/hash/SK.h>
#include <LLC/include/eckey.h>
#include <LLC/types/bignum.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/sessionmanager.h>

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

        /** Initialize mutex for coordinating stake minters on different threads. */
        std::mutex STAKING_MUTEX;


        /** Constructor **/
        StakeMinter::StakeMinter(uint256_t& sessionId)
        : fWait(false)
        , nWaitTime(0)
        , nTrustWeight(0.0)
        , nBlockWeight(0.0)
        , nStakeRate(0.0)
        , account()
        , stateLast()
        , fStakeChange(false)
        , stakeChange()
        , hashLastBlock(0)
        , block()
        , fGenesis(false)
        , fRestart(false)
        , nTrust(0)
        , nBlockAge(0)
        , session(TAO::API::GetSessionManager().Get(sessionId, false))
        , nSession(sessionId)
        , nLogTime(0)
        , nWaitReason(0)
        {
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
            /* Check whether unlocked account available. */
            if(session.Locked())
            {
                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "No account available for staking");

                return false;
            }

            /* Check that the account is unlocked for staking */
            if(!session.CanStake())
            {
                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Account has not been unlocked for staking");

                return false;
            }

            return true;
        }


        /*  Retrieves the most recent stake transaction for a user account. */
        bool StakeMinter::FindTrustAccount()
        {
            bool fIndexed;
            /* Retrieve the trust account for the user's hashGenesis */
            if(!TAO::Ledger::FindTrustAccount(session.GetAccount()->Genesis(), account, fIndexed))
                return false;

            fGenesis = !fIndexed;

            if(fGenesis)
            {
                /* Check that there is no stake. */
                if(account.get<uint64_t>("stake") != 0)
                    return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                        "Cannot create Genesis with already existing stake");

                /* Check that there is no trust. Preset trust is allowed for testing on testnet when -trustboost is used. */
                if(account.get<uint64_t>("trust") !=
                ((config::fTestNet.load() && config::GetBoolArg("-trustboost")) ? TAO::Ledger::ONE_YEAR : 0))
                    return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                        "Cannot create Genesis with already existing trust");
            }

            return true;
        }


        /** Retrieves the most recent stake transaction for a user account. */
        bool StakeMinter::FindLastStake(uint512_t& hashLast)
        {
            const uint256_t hashGenesis = session.GetAccount()->Genesis();

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
        bool StakeMinter::FindStakeChange(const uint512_t& hashLast)
        {
            const uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* Check for pending stake change request */
            bool fRemove = false;
            TAO::Ledger::StakeChange request;
            uint64_t nStake = account.get<uint64_t>("stake");
            uint64_t nBalance = account.get<uint64_t>("balance");

            try
            {
                if(!LLD::Local->ReadStakeChange(hashGenesis, request) || request.fProcessed)
                {
                    /* No stake change request to process */
                    fStakeChange = false;
                    stakeChange.SetNull();
                    return true;
                }
            }
            catch(const std::exception& e)
            {
                std::string msg(e.what());
                std::size_t nPos = msg.find("end of stream");
                if(nPos != std::string::npos)
                {
                    /* Attempts to read/deserialize old format for StakeChange will throw an end of stream exception. */
                    fRemove = true;
                    debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                               "Obsolete format for stake change request...removing");
                }
                else
                    throw; //rethrow exception if not end of stream

            }

            /* Verify stake change request is current version supported by minter */
            if(!fRemove && request.nVersion != 1)
            {
                fRemove = true;
                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Stake change request is unsupported version...removing");
            }

            /* Verify current hashGenesis matches requesting value */
            if(!fRemove && hashGenesis != request.hashGenesis)
            {
                fRemove = true;
                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Stake change request hashGenesis mismatch...removing");
            }

            /* Verify that no blocks have been staked since the change was requested */
            if(!fRemove && request.hashLast != hashLast)
            {
                fRemove = true;
                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Stake change request is stale...removing");
            }

            /* Check for expired stake change request */
            if(!fRemove && request.nExpires != 0 && request.nExpires < runtime::unifiedtimestamp())
            {
                fRemove = true;
                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Stake change request has expired...removing");
            }

            /* Validate the change request crypto signature */
            if(!fRemove)
            {
                /* Get the crypto register for the current user hashGenesis. */
                TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);
                TAO::Register::Object crypto;
                if(!LLD::Register->ReadState(hashCrypto, crypto))
                    return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                        "Missing crypto register");

                /* Parse the object. */
                if(!crypto.Parse())
                    return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                        "Failed to parse crypto register");

                if(crypto.Standard() != TAO::Register::OBJECTS::CRYPTO)
                    return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                        "Invalid crypto register");

                /* Verify the signature on the stake change request */
                uint256_t hashPublic = crypto.get<uint256_t>("auth");

                if(hashPublic != 0) //no auth key disables check
                {
                    /* Get hash of public key and set to same type as crypto register hash for verification */
                    uint256_t hashCheck = LLC::SK256(request.vchPubKey);
                    hashCheck.SetType(hashPublic.GetType());

                    /* Check the public key to expected authorization key. */
                    if(hashCheck != hashPublic)
                    {
                        LLD::Local->EraseStakeChange(hashGenesis);
                        return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                            "Invalid public key on stake change request...removing");
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
                            key.SetPubKey(request.vchPubKey);
                            if(!key.Verify(request.GetHash().GetBytes(), request.vchSig))
                            {
                                LLD::Local->EraseStakeChange(hashGenesis);
                                return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                                    "Invalid signature on stake change request...removing");
                            }

                            break;
                        }

                        /* Support for the BRAINPOOL signature scheme. */
                        case TAO::Ledger::SIGNATURE::BRAINPOOL:
                        {
                            /* Create EC Key object. */
                            LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                            /* Set the public key and verify. */
                            key.SetPubKey(request.vchPubKey);
                            if(!key.Verify(request.GetHash().GetBytes(), request.vchSig))
                            {
                                LLD::Local->EraseStakeChange(hashGenesis);
                                return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                                    "Invalid signature on stake change request...removing");
                            }

                            break;
                        }

                        default:
                        {
                            LLD::Local->EraseStakeChange(hashGenesis);
                            return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                                "Invalid signature type on stake change request...removing");
                        }
                    }
                }
            }

            /* Verify stake/unstake limits */
            if(!fRemove)
            {
                if(request.nAmount < 0 && (0 - request.nAmount) > nStake)
                {
                    debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                               "Cannot unstake more than current stake, using current stake amount.");

                    request.nAmount = 0 - nStake;
                }
                else if(request.nAmount > 0 && request.nAmount > nBalance)
                {
                    debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                               "Cannot add more than current balance to stake, using current balance.");

                    request.nAmount = nBalance;
                }
                else if(request.nAmount == 0)
                {
                    /* Remove request if no change to stake amount */
                    fRemove = true;
                }
            }

            if(fRemove)
            {
                fStakeChange = false;
                stakeChange.SetNull();

                if (!LLD::Local->EraseStakeChange(hashGenesis))
                    debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                 "Failed to remove stake change request");

                return true;
            }

            /* Set the results into stake minter instance */
            fStakeChange = true;
            stakeChange = request;

            return true;
        }


        /* Calculates the Trust Weight and Block Weight values for the current trust account and candidate block. */
        bool StakeMinter::CalculateWeights()
        {
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

                    const uint64_t nCurrentTime = runtime::timestamp();

                    /* Update log every 5 minutes */
                    if((nCurrentTime - nLogTime) >= 300)
                    {
                        debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                   "Stake balance is immature. ", (nWaitTime.load() / 60),
                                   " minutes remaining until staking available.");

                        nLogTime = nCurrentTime;
                        nWaitReason = 3;
                    }

                    return false;
                }
                else if(nWaitReason == 3)
                {
                    /* Reset wait period setting */
                    fWait.store(false);
                    nWaitTime.store(0);

                    nLogTime = 0;
                    nWaitReason = 0;
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


        /* Assigns the last block hash (best chain) for the current minting round. */
        void StakeMinter::SetLastBlock(const uint1024_t& hashBlock)
        {
            hashLastBlock = hashBlock;

            return;
        }


        /* Verify whether or not a signature chain has met the interval requirement */
        bool StakeMinter::CheckInterval()
        {
            const uint32_t nMinInterval = MinStakeInterval(block); //cannot be static, value can change with version activation

            /* Check the block interval for trust transactions. */
            if(!fGenesis)
            {
                const uint32_t nInterval = block.nHeight - stateLast.nHeight;

                if(nInterval <= nMinInterval)
                {
                    const uint64_t nCurrentTime = runtime::timestamp();

                    /* Update log every 5 minutes */
                    if((nCurrentTime - nLogTime) >= 300)
                    {
                        debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                   "Too soon after last stake block. ", (nMinInterval - nInterval + 1),
                                   " blocks remaining until staking available.");

                        nLogTime = nCurrentTime;
                        nWaitReason = 4;
                    }

                    return false;
                }
                else if(nWaitReason == 4)
                {
                    /* Reset log time */
                    nLogTime = 0;
                    nWaitReason = 0;
                }
            }

            return true;
        }


        /* Verify no transactions for same signature chain in the mempool */
        bool StakeMinter::CheckMempool()
        {
            /* Check there are no mempool transactions for this sig chain. */
            if(fGenesis && mempool.Has(session.GetAccount()->Genesis()))
            {
                const uint64_t nCurrentTime = runtime::timestamp();

                /* Update log every 5 minutes -- use this here in case this is checked more than once for single block */
                if((nCurrentTime - nLogTime) >= 300)
                {
                    debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                               " Skipping stake genesis for current block as mempool transactions would be orphaned.");

                    nLogTime = nCurrentTime;
                    nWaitReason = 5;
                }

                return false;
            }
            else if(nWaitReason == 5)
            {
                /* Reset log time */
                nLogTime = 0;
                nWaitReason = 0;
            }

            return true;

        }


        /* Attempt to solve the hashing algorithm at the current staking difficulty for the candidate block */
        void StakeMinter::HashBlock(const cv::softdouble& nRequired)
        {
            /* Calculate the target value based on difficulty. */
            LLC::CBigNum bnTarget;
            bnTarget.SetCompact(block.nBits);
            const uint1024_t nHashTarget = bnTarget.getuint1024();

            /* Search for the proof of stake hash solution until it mines a block, minter is stopped,
             * or network generates a new block (minter must start over with new candidate)
             */
            while(hashLastBlock == TAO::Ledger::ChainState::hashBestChain.load())
            {
                /* Check that the producer(s) won't orphan any new transaction from the same hashGenesis. */
                if(CheckStale())
                {
                    fRestart = true;
                    debug::log(2, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                               "Stake block producer is stale, rebuilding...");

                    break; //This stake minter must wait until the next setup phase to hash
                }

                /* Update the block time for threshold accuracy. */
                block.UpdateTime();
                uint64_t nTimeCurrent = block.GetBlockTime();
                uint32_t nBlockTime;

                /* How long working on this block */
                if(block.nVersion < 9)
                    nBlockTime = nTimeCurrent - block.producer.nTimestamp;
                else
                    nBlockTime = nTimeCurrent - block.vProducer.back().nTimestamp;

                /* If just starting on block and haven't reached at least one second block time, wait a moment */
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

                /* If threshhold not larger than required, stop hashing and return control */
                if(nThreshold < nRequired)
                    break;

                /* Every 1000 attempts, log progress. */
                if(block.nNonce % 1000 == 0)
                    debug::log(3, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                               "Threshold ", nThreshold, " exceeds required ", nRequired,
                               ", mining Proof of Stake with nonce ", block.nNonce);

                /* Handle if block is found. */
                const uint1024_t stakeHash = block.StakeHash();
                if(stakeHash <= nHashTarget)
                {
                    debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                               "Found new stake hash ", stakeHash.SubString());

                    if(!ProcessBlock())
                    {
                        /* If block that has a process error or is not accepted, restart with setup phase */
                        fRestart = true;
                    }

                    break;
                }

                /* Increment nonce for next iteration. */
                ++block.nNonce;
            }

            return;
        }


        bool StakeMinter::ProcessBlock()
        {
            /* Calculate coinstake reward for producer.
             * Reward is based on final block time for block. Block time is updated with each iteration so we
             * have deferred reward calculation until after the block is found to get the exact correct amount.
             */
            uint64_t nReward = CalculateCoinstakeReward(block.GetBlockTime());

            TAO::Ledger::Transaction txProducer;
            if(block.nVersion < 9)
                txProducer = block.producer;
            else
                txProducer = block.vProducer.back();

            /* Add coinstake reward */
            txProducer[0] << nReward;

            /* Execute operation pre- and post-state. */
            if(!txProducer.Build())
                return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                    "Coinstake transaction failed to build");

            /* Coinstake producer now complete. Sign the transaction. */
            txProducer.Sign(session.GetAccount()->Generate(txProducer.nSequence, session.GetActivePIN()->PIN()));

            if(block.nVersion < 9)
                block.producer = txProducer;
            else
            {
                /* Replace last producer in block with the completed & signed one */
                block.vProducer.erase(block.vProducer.begin() + (block.vProducer.size() - 1));
                block.vProducer.push_back(txProducer);
            }

            /* Build the Merkle Root. */
            std::vector<uint512_t> vHashes;

            for(const auto& item : block.vtx)
                vHashes.push_back(item.second);

            /* producers are not part of vtx, add to vHashes last */
            if(block.nVersion < 9)
                vHashes.push_back(block.producer.GetHash());
            else
            {
                for(const TAO::Ledger::Transaction& tx : block.vProducer)
                    vHashes.push_back(tx.GetHash());
            }

            block.hashMerkleRoot = block.BuildMerkleTree(vHashes);

            /* Sign the block. */
            if(!SignBlock())
                return false;

            /* Print the newly found block. */
            block.print();

            /* Log block found */
            if(config::nVerbose > 0)
            {
                std::string strTimestamp = std::string(convert::DateTimeStrFormat(runtime::unifiedtimestamp()));

                debug::log(1, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Stake Minter: New nPoS channel block found at unified time ", strTimestamp);

                debug::log(1, " blockHash: ", block.GetHash().SubString(30), " block height: ", block.nHeight);
            }

            /* Lock the staking mutex (only one staking thread at a time may process a block). */
            LOCK2(TAO::Ledger::STAKING_MUTEX);

            if(block.hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load())
            {
                debug::log(0, FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                           "Generated block is stale");

                return false;
            }

            /* Lock the sigchain that is being mined. */
            LOCK(session.CREATE_MUTEX);

            /* Process the block and relay to network if it gets accepted into main chain.
             * This method will call TritiumBlock::Check() TritiumBlock::Accept() and BlockState::Index()
             * After all is approved, BlockState::Index() will call BlockState::SetBest()
             * to set the new best chain. That method relays the new block to the network.
             */
            uint8_t nStatus = 0;
            TAO::Ledger::Process(block, nStatus);

            /* Check the statues. */
            if(!(nStatus & PROCESS::ACCEPTED))
                return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                    "Generated block not accepted");

            /* After successfully generated genesis for trust account, reset to genereate trust for continued staking */
            if(fGenesis)
                fGenesis = false;

            return true;
        }


        /* Sign a candidate block after it is successfully mined. */
        bool StakeMinter::SignBlock()
        {
            /* Sign the submitted block */
            TAO::Ledger::Transaction txProducer;

            if(block.nVersion < 9)
                txProducer = block.producer;
            else
                txProducer = block.vProducer.back();

            std::vector<uint8_t> vBytes = session.GetAccount()->Generate(txProducer.nSequence, session.GetActivePIN()->PIN()).GetBytes();
            uint8_t nKeyType = txProducer.nKeyType;

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
                        return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                            "StakeMinter: Unable to set key for signing Tritium Block ",
                                            block.GetHash().SubString());

                    /* Generate the signature. */
                    if(!block.GenerateSignature(key))
                        return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                            "StakeMinter: Unable to sign Tritium Block ",
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
                        return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                            "StakeMinter: Unable to set key for signing Tritium Block ",
                                            block.GetHash().SubString());

                    /* Generate the signature. */
                    if(!block.GenerateSignature(key))
                        return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                            "StakeMinter: Unable to sign Tritium Block ",
                                            block.GetHash().SubString());

                    break;
                }

                default:
                    return debug::error(FUNCTION, (config::fMultiuser.load() ? (nSession.SubString() + " ") : ""),
                                        "StakeMinter: Unknown signature type");
            }

            return true;
         }

    } // End Ledger namespace
} // End TAO namespace
