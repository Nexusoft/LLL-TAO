/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/bignum.h>

#include <LLD/include/global.h>

#include <LLP/packets/message.h>
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

        /* Default constructor. */
        TritiumBlock::TritiumBlock()
        : Block     ( )
        , nTime     (runtime::unifiedtimestamp())
        , producer  ( )
        , ssSystem  ( )
        , vtx       ( )
        {
        }


        /* Copy constructor. */
        TritiumBlock::TritiumBlock(const TritiumBlock& block)
        : Block     (block)
        , nTime     (block.nTime)
        , producer  (block.producer)
        , ssSystem  (block.ssSystem)
        , vtx       (block.vtx)
        {
        }


        /* Move constructor. */
        TritiumBlock::TritiumBlock(TritiumBlock&& block) noexcept
        : Block     (std::move(block))
        , nTime     (std::move(block.nTime))
        , producer  (std::move(block.producer))
        , ssSystem  (std::move(block.ssSystem))
        , vtx       (std::move(block.vtx))
        {
        }


        /* Copy assignment. */
        TritiumBlock& TritiumBlock::operator=(const TritiumBlock& block)
        {
            nVersion       = block.nVersion;
            hashPrevBlock  = block.hashPrevBlock;
            hashMerkleRoot = block.hashMerkleRoot;
            nChannel       = block.nChannel;
            nHeight        = block.nHeight;
            nBits          = block.nBits;
            nNonce         = block.nNonce;
            vOffsets       = block.vOffsets;
            vchBlockSig    = block.vchBlockSig;
            vMissing       = block.vMissing;
            hashMissing    = block.hashMissing;
            fConflicted    = block.fConflicted;

            nTime          = block.nTime;
            ssSystem       = block.ssSystem;
            vtx            = block.vtx;

            producer   = block.producer;

            return *this;
        }


        /* Move assignment. */
        TritiumBlock& TritiumBlock::operator=(TritiumBlock&& block) noexcept
        {
            nVersion       = std::move(block.nVersion);
            hashPrevBlock  = std::move(block.hashPrevBlock);
            hashMerkleRoot = std::move(block.hashMerkleRoot);
            nChannel       = std::move(block.nChannel);
            nHeight        = std::move(block.nHeight);
            nBits          = std::move(block.nBits);
            nNonce         = std::move(block.nNonce);
            vOffsets       = std::move(block.vOffsets);
            vchBlockSig    = std::move(block.vchBlockSig);
            vMissing       = std::move(block.vMissing);
            hashMissing    = std::move(block.hashMissing);
            fConflicted    = std::move(block.fConflicted);

            nTime          = std::move(block.nTime);
            ssSystem       = std::move(block.ssSystem);
            vtx            = std::move(block.vtx);

            producer   = std::move(block.producer);

            return *this;
        }


        /* Default Destructor */
        TritiumBlock::~TritiumBlock()
        {
        }


        /* Copy Constructor. */
        TritiumBlock::TritiumBlock(const Block& block)
        : Block     (block)
        , nTime     (runtime::unifiedtimestamp())
        , producer  ( )
        , ssSystem  ( )
        , vtx       ( )
        {
        }


        /* Copy Constructor. */
        TritiumBlock::TritiumBlock(const BlockState& state)
        : Block    (state)
        , nTime    (state.nTime)
        , producer ( )
        , ssSystem (state.ssSystem)
        , vtx      (state.vtx.begin(), state.vtx.end() - 1)
        {
            /* Read the producer transaction from disk. */
            if(!LLD::Ledger->ReadTx(state.vtx.back().second, producer))
                throw debug::exception(FUNCTION, "failed to read producer");
        }


        /* Copy Constructor. */
        TritiumBlock::TritiumBlock(const SyncBlock& block)
        : Block     (block)
        , nTime     (block.nTime)
        , producer  ( )
        , ssSystem  (block.ssSystem)
        , vtx       ( )
        {
            /* Check for version conversions. */
            if(block.nVersion < 7)
                throw debug::exception(FUNCTION, "invalid sync block version for tritium block");

            /* Loop through transactions. */
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
                            /* Get the transaction hash */
                            uint512_t hash = tx.GetHash();

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

                        /* Get the transaction hash */
                        uint512_t hash = tx.GetHash();

                        /* Accept into memory pool. */
                        if(!LLD::Legacy->HasTx(hash))
                            mempool.AddUnchecked(tx);

                        /* Add transaction to binary data. */
                        vtx.push_back(std::make_pair(block.vtx[n].first, hash));

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
            nTime = static_cast<uint32_t>(std::max(ChainState::tStateBest.load().GetBlockTime() + 1, runtime::unifiedtimestamp()));
        }


        /* Return the Block's current UNIX timestamp. */
        uint64_t TritiumBlock::GetBlockTime() const
        {
            return nTime;
        }


        /* For debugging Purposes seeing block state data dump */
        std::string TritiumBlock::ToString() const
        {
            return debug::safe_printstr(ANSI_COLOR_FUNCTION, "Tritium Block", ANSI_COLOR_RESET, "("
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
            if(GetChannel() > (config::fHybrid.load() ? 3 : 2))
                return debug::error(FUNCTION, "channel out of range");

            /* Check that the time was within range. */
            if(nVersion < 8 && GetBlockTime() > runtime::unifiedtimestamp() + runtime::maxdrift() * 60)
                return debug::error(FUNCTION, "block timestamp too far in the future");

            /* Check that the time was within range. */
            if(nVersion >= 8 && GetBlockTime() > runtime::unifiedtimestamp() + runtime::maxdrift())
                return debug::error(FUNCTION, "block timestamp too far in the future");

            /* Check the Current Version Block Time-Lock. */
            if(!BlockVersionActive(GetBlockTime(), nVersion))
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

            /* Print the block if it gets this far into processing. */
            if(config::nVerbose >= 2)
                debug::log(2, ToString());

            /* Proof of stake specific checks. */
            if(IsProofOfStake())
            {
                /* Check for nonce of zero values. */
                if(nNonce == 0)
                    return debug::error(FUNCTION, "proof of stake can't have Nonce value of zero");

                /* Check producer is coinstake */
                if(!(producer.IsTrust() || producer.IsGenesis()))
                    return debug::error(FUNCTION, "producer transaction must be trust/genesis for proof of stake");

                /* Check the trust time is before Unified timestamp. */
                if(producer.nTimestamp > (runtime::unifiedtimestamp() + runtime::maxdrift()))
                    return debug::error(FUNCTION, "coinstake timestamp too far in the future");

                 /* Check coinstake transaction Time is Before Block. */
                if(producer.nTimestamp > GetBlockTime())
                    return debug::error(FUNCTION, "coinstake timestamp is after block timestamp");

                /* Check the Proof of Stake Claims. */
                if(!TAO::Ledger::ChainState::Synchronizing() && !VerifyWork())
                    return debug::error(FUNCTION, "invalid proof of stake");
            }

            /* Proof of work specific checks. */
            else if(IsProofOfWork())
            {
                /* Check the producer transaction. */
                if(!producer.IsCoinBase())
                    return debug::error(FUNCTION, "producer transaction has to be coinbase for proof of work");

                /* Check for prime offsets. */
                if(GetChannel() == CHANNEL::PRIME && vOffsets.empty())
                    return debug::error(FUNCTION, "prime block requires valid offsets");

                /* Check that other channels do not have offsets. */
                if(GetChannel() != CHANNEL::PRIME && !vOffsets.empty())
                    return debug::error(FUNCTION, "offsets included in non prime block");

                /* Check the Proof of Work Claims. */
                if(!TAO::Ledger::ChainState::Synchronizing() && !VerifyWork())
                    return debug::error(FUNCTION, "invalid proof of work");
            }

            /* Private specific checks. */
            else if(IsHybrid())
            {
                /* Check the producer transaction. */
                if(!producer.IsHybrid())
                    return debug::error(FUNCTION, "producer transaction needs to use correct channel");

                /* Check producer for correct genesis. */
                if(producer.hashGenesis != (config::fTestNet ?
                    uint256_t("0xb7a74c14508bd09e104eff93d86cbbdc5c9556ae68546895d964d8374a0e9a41") :
                    uint256_t("0xa7a74c14508bd09e104eff93d86cbbdc5c9556ae68546895d964d8374a0e9a41")))
                    return debug::error(FUNCTION, "producer transaction has to be authorized for hybrid mode");
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

            /* Get list of producer transactions. */
            std::map<uint256_t, uint512_t> mapLast;

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
                    /* Track our conflicted flags here. */
                    bool fHasConflict = false;

                    /* Check the memory pool. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(vtx[i].second, tx, fHasConflict, FLAGS::MEMPOOL))
                    {
                        vMissing.push_back(vtx[i]);
                        continue;
                    }

                    /* Check for conflicts. */
                    if(fHasConflict)
                        this->fConflicted = true;

                    /* Check for coinbase / coinstake. */
                    if(tx.IsCoinBase() || tx.IsCoinStake())
                        return debug::error(FUNCTION, "cannot have non-producer coinbase / coinstake transaction");

                    /* Check the transaction timestamp. */
                    if(GetBlockTime() < uint64_t(tx.nTime))
                        return debug::error(FUNCTION, "block timestamp earlier than transaction timestamp");

                    /* Check the transaction for validity. */
                    if(!tx.Check())
                        return debug::error(FUNCTION, "check transaction failed.");

                    /* Check legacy transaction for finality. */
                    if(!tx.IsFinal(nHeight, GetBlockTime()))
                        return debug::error(FUNCTION, "contains a non-final transaction");
                }

                /* Basic checks for tritium transactions. */
                else if(vtx[i].first == TRANSACTION::TRITIUM)
                {
                    /* Track our conflicted flags here. */
                    bool fHasConflict = false;

                    /* Check the memory pool. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(vtx[i].second, tx, fHasConflict, FLAGS::MEMPOOL))
                    {
                        vMissing.push_back(vtx[i]);
                        continue;
                    }

                    /* Check for conflicts. */
                    if(fHasConflict)
                        this->fConflicted = true;

                    /* Check for coinbase / coinstake. */
                    if(tx.IsCoinBase() || tx.IsCoinStake() || tx.IsHybrid())
                        return debug::error(FUNCTION, "cannot have non-producer coinbase / coinstake transaction");

                    /* Check the sequencing. */
                    if(mapLast.count(tx.hashGenesis) && tx.hashPrevTx != mapLast[tx.hashGenesis])
                        return debug::error(FUNCTION, "transaction in sigchain out of sequence");

                    /* Set the last hash for given genesis. */
                    mapLast[tx.hashGenesis] = tx.GetHash();
                }
                else
                    return debug::error(FUNCTION, "unknown transaction type");
            }

            /* Check producer */
            if(mapLast.count(producer.hashGenesis) && producer.hashPrevTx != mapLast[producer.hashGenesis])
                return debug::error(FUNCTION, "producer transaction out of sequence");

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

            /* Verify producer signature(s) (if not synchronizing) */
            if(!TAO::Ledger::ChainState::Synchronizing())
            {
                /* Switch based on signature type. */
                switch(producer.nKeyType)
                {
                    /* Support for the FALCON signature scheme. */
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

            /* Channel switched output. */
            if(GetChannel() != CHANNEL::PRIME && config::nVerbose >= 2)
            {
                debug::log(2, "  proof:  ", (GetChannel() == 0 ? StakeHash() : ProofHash()).SubString());
                debug::log(2, "  target: ", LLC::CBigNum().SetCompact(nBits).getuint1024().SubString());
            }

            /* Check that the nBits match the current Difficulty. **/
            if(nBits != GetNextTargetRequired(statePrev, GetChannel()))
                return debug::error(FUNCTION, "incorrect proof-of-work/proof-of-stake");

            /* Check That Block timestamp is not before previous block. */
            if(GetBlockTime() <= statePrev.GetBlockTime())
                return debug::error(FUNCTION, "block's timestamp too early");

            /* Check that Block is Descendant of Hardened Checkpoints. */
            #ifndef UNIT_TESTS
            if(!ChainState::Synchronizing() && !IsDescendant(statePrev))
                return debug::error(FUNCTION, "not descendant of last checkpoint");
            #endif

            /* Validate proof of stake. */
            if(IsProofOfStake() && !CheckStake())
                return debug::error(FUNCTION, "invalid proof of stake");

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
                /* Get the tritium transaction. */
                if(proof.first == TRANSACTION::TRITIUM)
                {
                    /* Track our conflicted flags here. */
                    bool fHasConflict = false;

                    /* Get the transaction hash. */
                    const uint512_t& hash = proof.second;

                    /* Check the memory pool. */
                    TAO::Ledger::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hash, tx, fHasConflict, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "transaction is not in memory pool");

                    /* Check for conflicts. */
                    if(fHasConflict)
                        state.fConflicted = true;

                    /* Write to disk. */
                    if(!LLD::Ledger->WriteTx(hash, tx))
                        return debug::error(FUNCTION, "failed to write tx to disk");
                }

                /* Get the legacy transaction. */
                else if(proof.first == TRANSACTION::LEGACY)
                {
                    /* Track our conflicted flags here. */
                    bool fHasConflict = false;

                    /* Get the transaction hash. */
                    const uint512_t& hash = proof.second;

                    /* Check if in memory pool. */
                    Legacy::Transaction tx;
                    if(!LLD::Legacy->ReadTx(hash, tx, fHasConflict, FLAGS::MEMPOOL))
                        return debug::error(FUNCTION, "transaction is not in memory pool");

                    /* Check for conflicts. */
                    if(fHasConflict)
                        state.fConflicted = true;

                    /* Write to disk. */
                    if(!LLD::Legacy->WriteTx(hash, tx))
                        return debug::error(FUNCTION, "failed to write tx to disk");
                }

                /* Checkpoints DISABLED for now. */
                else
                    return debug::error(FUNCTION, "using an unknown transaction type");
            }

            /* Add the producer transaction(s) */
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

            /* Check for best chain. */
            if(GetHash() == ChainState::hashBestChain.load())
            {
                /* Do a quick mempool processing check for ORPHANS. */
                runtime::timer timer;
                timer.Reset();
                mempool.Check();

                /* Log the mempool consistency checking. */
                uint64_t nElapsed = timer.ElapsedMilliseconds();
                debug::log(TAO::Ledger::ChainState::Synchronizing() ? 1 : 0, FUNCTION, "Mempool Consistency Check Complete in ", nElapsed,  " ms");
            }

            return true;
        }


        /* Check the proof of stake calculations. */
        bool TritiumBlock::CheckStake() const
        {
            /* Get previous block. Used for block age/coin age and pooled stake proof calculations */
            TAO::Ledger::BlockState statePrev;
            if(!LLD::Ledger->ReadBlock(hashPrevBlock, statePrev))
                return debug::error(FUNCTION, "prev block not in database");

            /* Weights for threshold calculations */
            cv::softdouble nTrustWeight = cv::softdouble(0.0);
            cv::softdouble nBlockWeight = cv::softdouble(0.0);

            /* Stake amount and stake change for threshold calculations */
            int64_t nStakeChange = 0;
            uint64_t nStake      = 0;

            /* Reset the coinstake contract streams. */
            producer[0].Seek(1, TAO::Operation::Contract::REGISTERS, STREAM::BEGIN);

            /* Deserialize from the stream. */
            TAO::Register::Object account;
            producer[0] >>= account;

            /* Parse the object. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse object register from pre-state");

            /* Validate that it is a trust account. */
            if(account.Standard() != TAO::Register::OBJECTS::TRUST)
                return debug::error(FUNCTION, "stake producer account is not a trust account");

            /* Process trust transaction. */
            if(producer.IsTrust())
            {
                /* Seek to last trust. */
                producer[0].Seek(1, TAO::Operation::Contract::OPERATIONS, STREAM::BEGIN);

                /* Get last trust hash. */
                uint512_t hashLastTrust = 0;
                producer[0] >> hashLastTrust;

                /* Get the trust score and stake change. */
                uint64_t nTrustScore = 0;
                producer[0] >> nTrustScore;
                producer[0] >> nStakeChange;

                /* Get the last stake block. */
                TAO::Ledger::BlockState stateLast;
                if(!LLD::Ledger->ReadBlock(hashLastTrust, stateLast))
                    return debug::error(FUNCTION, "last block not in database");

                /* Calculate Block Age (time from last stake block until previous block) */
                const uint64_t nBlockAge = statePrev.GetBlockTime() - stateLast.GetBlockTime();

                /* Get expected trust and block weights. */
                nTrustWeight = TrustWeight(nTrustScore);
                nBlockWeight = BlockWeight(nBlockAge);

                /* Set the stake to pre-state value. */
                nStake = account.get<uint64_t>("stake");
            }

            /* Process genesis transaction. */
            else if(producer.IsGenesis())
            {
                /* Genesis transaction can't have any transactions. */
                if(vtx.size() != 0)
                    return debug::error(FUNCTION, "stake genesis cannot include transactions");

                /* Calculate the Coin Age. */
                const uint64_t nAge = GetBlockTime() - account.nModified;

                /* Validate that Genesis coin age exceeds required minimum. */
                if(nAge < MinCoinAge())
                    return debug::error(FUNCTION, "stake genesis age is immature");

                /* Trust Weight For Genesis Transaction based on coin age. */
                nTrustWeight = GenesisWeight(nAge);

                /* Set the stake to pre-state value. */
                nStake = account.get<uint64_t>("balance");
            }

            else
                return debug::error(FUNCTION, "invalid solo stake operation");

            /* If stake added in block finder, apply to threshold calculation. */
            uint64_t nStakeApplied = nStake;
            if(nStakeChange > 0)
                nStakeApplied += nStakeChange;

            /* Check the stake balance. */
            if(nStakeApplied == 0)
                return debug::error(FUNCTION, "cannot stake if stake balance is zero");

            /* Calculate the energy efficiency thresholds. */
            uint64_t nBlockTime       = GetBlockTime() - producer.nTimestamp;
            cv::softdouble nThreshold = GetCurrentThreshold(nBlockTime, nNonce);
            cv::softdouble nRequired  = GetRequiredThreshold(nTrustWeight, nBlockWeight, nStakeApplied);

            /* Check that the threshold was not violated. */
            if(nThreshold < nRequired)
                return debug::error(FUNCTION, "energy threshold too low ", nThreshold, " required ", nRequired);

            /* Set target for logging */
            LLC::CBigNum bnTarget;
            bnTarget.SetCompact(nBits);

            /* Check the target is reached. */
            if(StakeHash() > bnTarget.getuint1024())
                return debug::error(FUNCTION, "proof of stake hash not meeting target");

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


        /* Get the Signature Hash of the block. Used to verify work claims. */
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
            return Block::StakeHash(producer.hashGenesis);
        }
    }
}
