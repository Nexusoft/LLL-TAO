/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/bignum.h>

#include <LLD/include/global.h>

#include <LLP/packets/tritium.h>
#include <LLP/include/global.h>
#include <LLP/include/inv.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/stake.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/syncblock.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/address.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>

#include <cmath>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** The default constructor. **/
        TritiumBlock::TritiumBlock()
        : Block()
        , nTime(runtime::unifiedtimestamp())
        , producer()
        , ssSystem()
        , vtx()
        {
            SetNull();
        }


        /** Copy constructor from base block. **/
        TritiumBlock::TritiumBlock(const Block& block)
        : Block(block)
        , nTime(runtime::unifiedtimestamp())
        , producer()
        , ssSystem()
        , vtx(0)
        {

        }


        /** Copy Constructor. **/
        TritiumBlock::TritiumBlock(const TritiumBlock& block)
        : Block(block)
        , nTime(block.nTime)
        , producer(block.producer)
        , ssSystem(block.ssSystem)
        , vtx(block.vtx)
        {
        }


        /** Copy Constructor. **/
        TritiumBlock::TritiumBlock(const BlockState& state)
        : Block(state)
        , nTime(state.nTime)
        , producer()
        , ssSystem(state.ssSystem)
        , vtx(state.vtx)
        {
            /* Read the producer transaction from disk. */
            if(!LLD::Ledger->ReadTx(state.vtx.back().second, producer))
                throw debug::exception(FUNCTION, "failed to read producer");

            /* Erase the producer. */
            vtx.erase(vtx.end());
        }


        /** Copy Constructor. **/
        TritiumBlock::TritiumBlock(const SyncBlock& block)
        : Block(block)
        , nTime(block.nTime)
        , producer()
        , ssSystem(block.ssSystem)
        , vtx()
        {
            /* Check for version conversions. */
            if(block.nVersion < 7)
                throw debug::exception(FUNCTION, "invalid sync block version for tritium block");

            /* Loop through transctions. */
            for(uint32_t n = 0; n < block.vtx.size(); ++n)
            {
                /* Switch for type. */
                switch(block.vtx[n].first)
                {
                    /* Check for tritium. */
                    case TRANSACTION::TRITIUM:
                    {
                        /* Serialize stream. */
                        DataStream ssData(block.vtx[n].second, SER_DISK, LLD::DATABASE_VERSION);

                        /* Build the transaction. */
                        Transaction tx;
                        ssData >> tx;

                        /* Add transaction to binary data. */
                        if(n == block.vtx.size() - 1)
                            producer = tx; //handle for the producer transaction
                        else
                        {
                            /* Accept into memory pool. */
                            if(!LLD::Ledger->HasTx(tx.GetHash()))
                                mempool.AddUnchecked(tx);

                            vtx.push_back(std::make_pair(block.vtx[n].first, tx.GetHash()));
                        }

                        break;
                    }

                    /* Check for legacy. */
                    case TRANSACTION::LEGACY:
                    {
                        /* Serialize stream. */
                        DataStream ssData(block.vtx[n].second, SER_DISK, LLD::DATABASE_VERSION);

                        /* Build the transaction. */
                        Legacy::Transaction tx;
                        ssData >> tx;

                        /* Accept into memory pool. */
                        if(!LLD::Legacy->HasTx(tx.GetHash()))
                            mempool.AddUnchecked(tx);

                        /* Add transaction to binary data. */
                        vtx.push_back(std::make_pair(block.vtx[n].first, tx.GetHash()));

                        break;
                    }

                    /* Check for checkpoint. */
                    case TRANSACTION::CHECKPOINT:
                    {
                        /* Serialize stream. */
                        DataStream ssData(block.vtx[n].second, SER_DISK, LLD::DATABASE_VERSION);

                        /* Build the transaction. */
                        uint512_t proof;
                        ssData >> proof;

                        /* Add transaction to binary data. */
                        vtx.push_back(std::make_pair(block.vtx[n].first, proof));

                        break;
                    }
                }
            }
        }


        /** Default Destructor **/
        TritiumBlock::~TritiumBlock()
        {
        }


        /*  Allows polymorphic copying of blocks
         *  Overridden to return an instance of the TritiumBlock class. */
        TritiumBlock* TritiumBlock::Clone() const
        {
            return new TritiumBlock(*this);
        }


        /*  Set the block to Null state. */
        void TritiumBlock::SetNull()
        {
            Block::SetNull();

            vtx.clear();
            producer = Transaction();
        }


        /* Update the nTime of the current block. */
        void TritiumBlock::UpdateTime()
        {
            nTime = static_cast<uint32_t>(std::max(ChainState::stateBest.load().GetBlockTime() + 1, runtime::unifiedtimestamp()));
        }


        /* Return the Block's current UNIX timestamp. */
        uint64_t TritiumBlock::GetBlockTime() const
        {
            return nTime;
        }


        /* For debugging Purposes seeing block state data dump */
        std::string TritiumBlock::ToString() const
        {
            return debug::safe_printstr("Tritium Block("
                VALUE("hash")     " = ", GetHash().SubString(), " ",
                VALUE("nVersion") " = ", nVersion, ", ",
                VALUE("hashPrevBlock") " = ", hashPrevBlock.SubString(), ", ",
                VALUE("hashMerkleRoot") " = ", hashMerkleRoot.SubString(), ", ",
                VALUE("nChannel") " = ", nChannel, ", ",
                VALUE("nHeight") " = ", nHeight, ", ",
                VALUE("nBits") " = ", nBits, ", ",
                VALUE("nNonce") " = ", nNonce, ", ",
                VALUE("nTime") " = ", nTime, ", ",
                VALUE("vchBlockSig") " = ", HexStr(vchBlockSig.begin(), vchBlockSig.end()).substr(0, 20), ", ",
                VALUE("vtx.size()") " = ", vtx.size(), ")");
        }


        /* Checks if a block is valid if not connected to chain. */
        bool TritiumBlock::Check() const
        {
            /* Read ledger DB for duplicate block. */
            if(LLD::Ledger->HasBlock(GetHash()))
                return false;//debug::error(FUNCTION, "already have block ", GetHash().SubString());

            /* Check the Size limits of the Current Block. */
            if(::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
                return debug::error(FUNCTION, "size limits failed ", MAX_BLOCK_SIZE);

            /* Make sure the Block was Created within Active Channel. */
            if(GetChannel() > (config::GetBoolArg("-private") ? 3 : 2))
                return debug::error(FUNCTION, "channel out of range");

            /* Check that the time was within range. */
            if(GetBlockTime() > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT * 60)
                return debug::error(FUNCTION, "block timestamp too far in the future");

            /* Check the Current Version Block Time-Lock. */
            if(!VersionActive(GetBlockTime(), nVersion))
                return debug::error(FUNCTION, "block created with invalid version");

            /* Check the Network Launch Time-Lock. */
            if(!NetworkActive(GetBlockTime()))
                return debug::error(FUNCTION, "block created before network time-lock");

            /* Check the channel time-locks. */
            if(!ChannelActive(GetBlockTime(), GetChannel()))
                return debug::error(FUNCTION, "block created before channel time-lock");

            /* Check coinbase/coinstake timestamp against block time */
            if(GetBlockTime() > (uint64_t)producer.nTimestamp + ((nVersion < 4) ? 1200 : 3600))
                return debug::error(FUNCTION, "producer transaction timestamp is too early");

            /* Check that the producer is a valid transaction. */
            if(!producer.Check())
                return debug::error(FUNCTION, "producer transaction is invalid");

            /* Proof of stake specific checks. */
            if(IsProofOfStake())
            {
                /* Check the producer transaction. */
                if(!(producer.IsCoinStake()))
                    return debug::error(FUNCTION, "producer transaction has to be trust/genesis for proof of stake");

                /* Check for nonce of zero values. */
                if(nNonce == 0)
                    return debug::error(FUNCTION, "proof of stake can't have Nonce value of zero");

                /* Check the trust time is before Unified timestamp. */
                if(producer.nTimestamp > (runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT))
                    return debug::error(FUNCTION, "trust timestamp too far in the future");

                /* Make Sure Trust Transaction Time is Before Block. */
                if(producer.nTimestamp > GetBlockTime())
                    return debug::error(FUNCTION, "coinstake timestamp is after block timestamp");

                /* Check the Proof of Stake Claims. */
                if(!VerifyWork())
                    return debug::error(FUNCTION, "invalid proof of stake");
            }

            /* Proof of work specific checks. */
            else if(IsProofOfWork())
            {
                /* Check the producer transaction. */
                if(!producer.IsCoinBase())
                    return debug::error(FUNCTION, "producer transaction has to be coinbase for proof of work");

                /* Check the Proof of Work Claims. */
                if(!VerifyWork())
                    return debug::error(FUNCTION, "invalid proof of work");
            }

            /* Private specific checks. */
            else if(IsPrivate())
            {
                /* Check the producer transaction. */
                if(!producer.IsPrivate())
                    return debug::error(FUNCTION, "producer transaction has to be authorize for private mode");
            }

            /* Default catch. */
            else
                return debug::error(FUNCTION, "unknown block type");

            /* Make sure there is no system memory. */
            if(ssSystem.size() != 0)
                return debug::error(FUNCTION, "cannot allocate system memory");

            /* Check for duplicate txid's */
            std::set<uint512_t> setUnique;
            std::vector<uint512_t> vHashes;

            /* Get the signature operations for legacy tx's. */
            uint32_t nSigOps = 0;

            /* Get the signature operations for legacy tx's. */
            uint32_t nSize = (uint32_t)vtx.size();
            for(uint32_t i = 0; i < nSize; ++i)
            {
                /* Insert txid into set to check for duplicates. */
                setUnique.insert(vtx[i].second);
                vHashes.push_back(vtx[i].second);

                /* Basic checks for legacy transactions. */
                if(vtx[i].first == TRANSACTION::LEGACY)
                {
                    /* Check the memory pool. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(vtx[i].second, tx, FLAGS::MEMPOOL))
                    {
                        /* Push missing transaction to memory. */
                        vMissing.push_back(vtx[i]);

                        continue;
                    }

                    /* Check for coinbase / coinstake. */
                    if(tx.IsCoinBase() || tx.IsCoinStake())
                        return debug::error(FUNCTION, "more than one coinbase / coinstake");

                    /* Check the transaction timestamp. */
                    if(GetBlockTime() < (uint64_t) tx.nTime)
                        return debug::error(FUNCTION, "block timestamp earlier than transaction timestamp");

                    /* Check the transaction for validity. */
                    if(!tx.CheckTransaction())
                        return debug::error(FUNCTION, "check transaction failed.");

                    /* Check legacy transaction for finality. */
                    if(!tx.IsFinal(nHeight, GetBlockTime()))
                        return debug::error(FUNCTION, "contains a non-final transaction");
                }

                /* Basic checks for tritium transactions. */
                else if(vtx[i].first == TRANSACTION::TRITIUM)
                {
                    /* Check the memory pool. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(vtx[i].second, tx, FLAGS::MEMPOOL))
                    {
                        /* Push missing transaction to memory. */
                        vMissing.push_back(vtx[i]);

                        continue;
                    }

                    /* Check for coinbase / coinstake. */
                    if(tx.IsCoinBase() || tx.IsCoinStake())
                        return debug::error(FUNCTION, "more than one coinbase / coinstake");

                    /* Check the transaction for validity. */
                    //if(!tx.Check()) //NOTE: this is pre-processing stuff
                    //    return debug::error(FUNCTION, "contains an invalid transaction");
                }
                else
                    return debug::error(FUNCTION, "unknown transaction type");
            }

            /* Get producer hash. */
            uint512_t hashProducer = producer.GetHash();

            /* Add producer to merkle tree list. */
            vHashes.push_back(hashProducer);
            setUnique.insert(hashProducer);

            /* Check for missing transactions. */
            if(vMissing.size() != 0)
                return debug::error(FUNCTION, "missing ", vMissing.size(), " transactions");

            /* Check for duplicate txid's. */
            if(setUnique.size() != vHashes.size())
                return debug::error(FUNCTION, "duplicate transaction");

            /* Check the signature operations for legacy. */
            if(nSigOps > MAX_BLOCK_SIGOPS)
                return debug::error(FUNCTION, "out-of-bounds SigOpCount");

            /* Check the merkle root. */
            if(hashMerkleRoot != BuildMerkleTree(vHashes))
                return debug::error(FUNCTION, "hashMerkleRoot mismatch");

            /* Switch based on signature type. */
            switch(producer.nKeyType)
            {
                /* Support for the FALCON signature scheeme. */
                case SIGNATURE::FALCON:
                {
                    /* Create the FL Key object. */
                    LLC::FLKey key;

                    /* Set the public key and verify. */
                    key.SetPubKey(producer.vchPubKey);

                    /* Check the Block Signature. */
                    if(!VerifySignature(key))
                        return debug::error(FUNCTION, "bad block signature");

                    break;
                }

                /* Support for the BRAINPOOL signature scheme. */
                case SIGNATURE::BRAINPOOL:
                {
                    /* Create EC Key object. */
                    LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);

                    /* Set the public key and verify. */
                    key.SetPubKey(producer.vchPubKey);

                    /* Check the Block Signature. */
                    if(!VerifySignature(key))
                        return debug::error(FUNCTION, "bad block signature");

                    break;
                }

                default:
                    return debug::error(FUNCTION, "unknown signature type");
            }

            return true;
        }


        /** Accept a tritium block. **/
        bool TritiumBlock::Accept() const
        {
            /* Read ledger DB for previous block. */
            TAO::Ledger::BlockState statePrev;
            if(!LLD::Ledger->ReadBlock(hashPrevBlock, statePrev))
                return debug::error(FUNCTION, "previous block state not found");

            /* Check the Height of Block to Previous Block. */
            if(statePrev.nHeight + 1 != nHeight)
                return debug::error(FUNCTION, "incorrect block height.");

            /* Verbose logging of proof and target. */
            debug::log(2, "  proof:  ", (GetChannel() == 0 ? StakeHash() : ProofHash()).SubString());

            /* Channel switched output. */
            if(GetChannel() == 1)
                debug::log(2, "  prime cluster verified of size ", GetDifficulty(nBits, 1));
            else
                debug::log(2, "  target: ", LLC::CBigNum().SetCompact(nBits).getuint1024().SubString());

            /* Check that the nBits match the current Difficulty. **/
            if(nBits != GetNextTargetRequired(statePrev, GetChannel()))
                return debug::error(FUNCTION, "incorrect proof-of-work/proof-of-stake");

            /* Check That Block timestamp is not before previous block. */
            if(GetBlockTime() <= statePrev.GetBlockTime())
                return debug::error(FUNCTION, "block's timestamp too early");

            /* Check that Block is Descendant of Hardened Checkpoints. */
            //if(!ChainState::Synchronizing() && !IsDescendant(statePrev))
            //    return debug::error(FUNCTION, "not descendant of last checkpoint");

            /* Check proof of stake requirements. */
            if(IsProofOfStake())
            {
                /* Check the proof of stake. */
                if(!CheckStake())
                    return debug::error(FUNCTION, "proof of stake is invalid");
            }
            else if(IsPrivate())
            {
                /* Check producer for correct genesis. */
                if(producer.hashGenesis != (config::fTestNet ?
                    uint256_t("0xa2a74c14508bd09e104eff93d86cbbdc5c9556ae68546895d964d8374a0e9a41") :
                    uint256_t("0xa1a74c14508bd09e104eff93d86cbbdc5c9556ae68546895d964d8374a0e9a41")))
                    return debug::error(FUNCTION, "invalid genesis generated");
            }

            /* Default catch. */
            else if(!IsProofOfWork())
                return debug::error(FUNCTION, "unknown block type");

            /* Check that producer isn't before previous block time. */
            if(producer.nTimestamp <= statePrev.GetBlockTime())
                return debug::error(FUNCTION, "producer can't be before previous block");

            /* Process the block state. */
            TAO::Ledger::BlockState state(*this);

            /* Start the database transaction. */
            LLD::TxnBegin();

            /* Write the transactions. */
            for(const auto& proof : vtx)
            {
                if(proof.first == TRANSACTION::TRITIUM)
                {
                    /* Get the transaction hash. */
                    const uint512_t& hash = proof.second;

                    /* Check the memory pool. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hash, tx, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "transaction is not in memory pool");

                    /* Write to disk. */
                    if(!LLD::Ledger->WriteTx(hash, tx))
                        return debug::error(FUNCTION, "failed to write tx to disk");
                }
                else if(proof.first == TRANSACTION::LEGACY)
                {
                    /* Get the transaction hash. */
                    const uint512_t& hash = proof.second;

                    /* Check if in memory pool. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(hash, tx, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "transaction is not in memory pool");

                    /* Write to disk. */
                    if(!LLD::Legacy->WriteTx(hash, tx))
                        return debug::error(FUNCTION, "failed to write tx to disk");
                }
                else
                    return debug::error(FUNCTION, "using an unknown transaction type");
            }

            /* Add the producer transaction */
            if(!LLD::Ledger->WriteTx(producer.GetHash(), producer))
                return debug::error(FUNCTION, "failed to write producer to disk");

            /* Accept the block state. */
            if(!state.Index())
            {
                LLD::TxnAbort();

                return false;
            }

            /* Commit the transaction to database. */
            LLD::TxnCommit();

            return true;
        }


        /* Check the proof of stake calculations. */
        bool TritiumBlock::CheckStake() const
        {
            /* Reset the coinstake contract streams. */
            producer[0].Reset();

            /* Get the trust object register. */
            TAO::Register::Object account;

            /* Deserialize from the stream. */
            uint8_t nState = 0;
            producer[0] >>= nState;

            /* Get the pre-state. */
            producer[0] >>= account;

            /* Parse the object. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse object register from pre-state");

            /* Validate that it is a trust account. */
            if(account.Standard() != TAO::Register::OBJECTS::TRUST)
                return debug::error(FUNCTION, "stake producer account is not a trust account");

            /* Get previous block. Block time used for block age/coin age calculation */
            TAO::Ledger::BlockState statePrev;
            if(!LLD::Ledger->ReadBlock(hashPrevBlock, statePrev))
                return debug::error(FUNCTION, "prev block not in database");

            /* Calculate weights */
            double nTrustWeight = 0.0;
            double nBlockWeight = 0.0;

            uint64_t nTrust = 0;
            uint64_t nTrustPrev = 0;
            uint64_t nReward = 0;
            uint64_t nBlockAge = 0;
            uint64_t nStake = 0;
            int64_t nStakeChange = 0;

            if(producer.IsTrust())
            {
                /* Extract values from producer operation */
                uint8_t OP = 0;
                producer[0] >> OP;

                /* Double check OP code. */
                if(OP != TAO::Operation::OP::TRUST)
                    return debug::error(FUNCTION, "invalid producer operation for trust");

                /* Get last trust hash. */
                uint512_t hashLastClaimed = 0;
                producer[0] >> hashLastClaimed;

                uint64_t nClaimedTrust = 0;
                producer[0] >> nClaimedTrust;
                producer[0] >> nStakeChange;

                uint64_t nClaimedReward = 0;
                producer[0] >> nClaimedReward;

                /* Validate the claimed hash last stake */
                uint512_t hashLast;
                if(!LLD::Ledger->ReadStake(producer.hashGenesis, hashLast))
                    return debug::error(FUNCTION, "last stake not in database");

                if(hashLast != hashLastClaimed)
                {
                    Transaction tx1;
                    if(!LLD::Ledger->ReadTx(hashLast, tx1))
                        return debug::error(FUNCTION, "failed to read ", tx1.GetHash().SubString());

                    Transaction tx2;
                    if(!LLD::Ledger->ReadTx(hashLastClaimed, tx2))
                        return debug::error(FUNCTION, "failed to read ", tx2.GetHash().SubString());

                    tx1.print();
                    tx2.print();

                    return debug::error(FUNCTION, "claimed last stake ", hashLastClaimed.SubString(),
                                                  " does not match actual last stake ", hashLast.SubString());
                }


                /* Get the last stake block. */
                TAO::Ledger::BlockState stateLast;
                if(!LLD::Ledger->ReadBlock(hashLastClaimed, stateLast))
                    return debug::error(FUNCTION, "last block not in database");

                /* Enforce the minimum interval between stake blocks. */
                const uint32_t nInterval = nHeight - stateLast.nHeight;

                if(nInterval <= MinStakeInterval())
                    return debug::error(FUNCTION, "stake block interval ", nInterval, " below minimum interval");

                /* Get pre-state trust account values */
                nTrustPrev = account.get<uint64_t>("trust");
                nStake = account.get<uint64_t>("stake");

                /* Calculate Block Age (time from last stake block until previous block) */
                nBlockAge = statePrev.GetBlockTime() - stateLast.GetBlockTime();

                /* Calculate the new trust score */
                nTrust = GetTrustScore(nTrustPrev, nBlockAge, nStake, nStakeChange);

                /* Validate the trust score calculation */
                if(nClaimedTrust != nTrust)
                    return debug::error(FUNCTION, "claimed trust score ", nClaimedTrust,
                                                  " does not match calculated trust score ", nTrust);

                /* Calculate the coinstake reward */
                const uint64_t nTime = GetBlockTime() - stateLast.GetBlockTime();
                nReward = GetCoinstakeReward(nStake, nTime, nTrust, false);

                /* Validate the coinstake reward calculation */
                if(nClaimedReward != nReward)
                    return debug::error(FUNCTION, "claimed stake reward ", nClaimedReward,
                                                  " does not match calculated reward ", nReward);

                /* Calculate Trust Weight corresponding to new trust score. */
                nTrustWeight = TrustWeight(nTrust);

                /* Calculate Block Weight from current block age. */
                nBlockWeight = BlockWeight(nBlockAge);
            }

            else if(producer.IsGenesis())
            {
                /* Extract values from producer operation */
                uint8_t OP = 0;
                producer[0] >> OP;

                /* Double check OP code. */
                if(OP != TAO::Operation::OP::GENESIS)
                    return debug::error(FUNCTION, "invalid producer operation for genesis");

                uint64_t nClaimedReward = 0;
                producer[0] >> nClaimedReward;

                /* Get Genesis stake from the trust account pre-state balance. Genesis reward based on balance (that will move to stake) */
                nStake = account.get<uint64_t>("balance");

                /* Genesis transaction can't have any transactions. */
                if(vtx.size() != 0)
                    return debug::error(FUNCTION, "genesis cannot include transactions");

                /* Calculate the Coin Age. */
                const uint64_t nAge = GetBlockTime() - account.nModified;

                /* Validate that Genesis coin age exceeds required minimum. */
                if(nAge < MinCoinAge())
                    return debug::error(FUNCTION, "genesis age is immature");

                /* Calculate the coinstake reward */
                nReward = GetCoinstakeReward(nStake, nAge, 0, true);

                /* Validate the coinstake reward calculation */
                if(nClaimedReward != nReward)
                    return debug::error(FUNCTION, "claimed hashGenesis reward ", nClaimedReward, " does not match calculated reward ", nReward);

                /* Trust Weight For Genesis Transaction Reaches Maximum at 90 day Limit. */
                nTrustWeight = GenesisWeight(nAge);
            }

            else
                return debug::error(FUNCTION, "invalid stake operation");

            uint64_t nStakeApplied = nStake;
            if(nStakeChange > 0)
                nStakeApplied += nStakeChange; //If stake added, apply to threshold calculation

            /* Check the stake balance. */
            if(nStakeApplied == 0)
                return debug::error(FUNCTION, "cannot stake if stake balance is zero");

            /* Calculate the energy efficiency thresholds. */
            uint64_t nBlockTime = GetBlockTime() - producer.nTimestamp;
            double nThreshold = GetCurrentThreshold(nBlockTime, nNonce);
            double nRequired  = GetRequiredThreshold(nTrustWeight, nBlockWeight, nStakeApplied);

            /* Check that the threshold was not violated. */
            if(nThreshold < nRequired)
                return debug::error(FUNCTION, "energy threshold too low ", nThreshold, " required ", nRequired);

            /* Set target for logging */
            LLC::CBigNum bnTarget;
            bnTarget.SetCompact(nBits);

            /* Verbose logging. */
            debug::log(2, FUNCTION,
                "stake hash=", StakeHash().SubString(), ", ",
                "target=", bnTarget.getuint1024().SubString(), ", ",
                "type=", (producer.IsTrust() ? "Trust" : "Genesis"), ", ",
                "trust score=", nTrust, ", ",
                "prev trust score=", nTrustPrev, ", ",
                "trust change=", int64_t(nTrust - nTrustPrev), ", ",
                "block age=", nBlockAge, ", ",
                "stake=", nStake, ", ",
                "reward=", nReward, ", ",
                "trust weight=", nTrustWeight, ", ",
                "block weight=", nBlockWeight, ", ",
                "block time=", nBlockTime, ", ",
                "threshold=", nThreshold, ", ",
                "required=", nRequired, ", ",
                "nonce=", nNonce, ", ",
                "add stake=", ((nStakeChange > 0) ? nStakeChange : 0), ", ",
                "unstake=", ((nStakeChange < 0) ? (0 - nStakeChange) : 0));

            return true;
        }


        /* Verify the Proof of Work satisfies network requirements. */
        bool TritiumBlock::VerifyWork() const
        {
            /* This override adds support for verifying the stake hash on the staking channel */
            if(nChannel == 0)
            {
                LLC::CBigNum bnTarget;
                bnTarget.SetCompact(nBits);

                /* Check that the hash is within range. */
                if(bnTarget <= 0 || bnTarget > bnProofOfWorkLimit[nChannel])
                    return debug::error(FUNCTION, "Proof of stake hash not in range");

                if(StakeHash() > bnTarget.getuint1024())
                    return debug::error(FUNCTION, "Proof of stake not meeting target");

                return true;
            }

            return Block::VerifyWork();
        }


        /* Get the Signarture Hash of the block. Used to verify work claims. */
        uint1024_t TritiumBlock::SignatureHash() const
        {
            /* Create a data stream to get the hash. */
            DataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);
            ss.reserve(256);

            /* Serialize the data to hash into a stream. */
            ss << nVersion << hashPrevBlock << hashMerkleRoot << nChannel << nHeight << nBits << nNonce << nTime << vOffsets;

            return LLC::SK1024(ss.begin(), ss.end());
        }


        /* Prove that you staked a number of seconds based on weight */
        uint1024_t TritiumBlock::StakeHash() const
        {
            return Block::StakeHash(producer.IsGenesis(), producer.hashGenesis);
        }
    }
}
