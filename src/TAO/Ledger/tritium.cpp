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

#include <TAO/Register/types/object.h>

#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/chainstate.h>

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
        , producer()
        , ssSystem()
        , vtx()
        {
            SetNull();
        }


        /** Copy Constructor. **/
        TritiumBlock::TritiumBlock(const TritiumBlock& block)
        : Block(block)
        , producer(block.producer)
        , ssSystem(block.ssSystem)
        , vtx(block.vtx)
        {
        }


        /** Copy Constructor. **/
        TritiumBlock::TritiumBlock(const BlockState& state)
        : Block(state)
        , producer()
        , ssSystem(state.ssSystem)
        , vtx(state.vtx)
        {
            vtx.erase(vtx.begin());

            /* Read the producer transaction from disk. */
            if(!LLD::legDB->ReadTx(state.vtx[0].second, producer))
                debug::error(FUNCTION, "failed to read producer");
        }


        /** Default Destructor **/
        TritiumBlock::~TritiumBlock()
        {
        }


        /*  Set the block to Null state. */
        void TritiumBlock::SetNull()
        {
            Block::SetNull();

            vtx.clear();
            producer = Transaction();
        }


        /* For debugging Purposes seeing block state data dump */
        std::string TritiumBlock::ToString() const
        {
            return debug::safe_printstr("Tritium Block("
                VALUE("hash")     " = ", GetHash().ToString().substr(0, 20), " ",
                VALUE("nVersion") " = ", nVersion, ", ",
                VALUE("hashPrevBlock") " = ", hashPrevBlock.ToString().substr(0, 20), ", ",
                VALUE("hashMerkleRoot") " = ", hashMerkleRoot.ToString().substr(0, 20), ", ",
                VALUE("nChannel") " = ", nChannel, ", ",
                VALUE("nHeight") " = ", nHeight, ", ",
                VALUE("nBits") " = ", nBits, ", ",
                VALUE("nNonce") " = ", nNonce, ", ",
                VALUE("nTime") " = ", nTime, ", ",
                VALUE("vchBlockSig") " = ", HexStr(vchBlockSig.begin(), vchBlockSig.end()), ", ",
                VALUE("vtx.size()") " = ", vtx.size(), ")");
        }


        /* Checks if a block is valid if not connected to chain. */
        bool TritiumBlock::Check() const
        {
            /* Check the Size limits of the Current Block. */
            if (::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
                return debug::error(FUNCTION, "size ", ::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION), " limits failed ", MAX_BLOCK_SIZE);


            /* Make sure the Block was Created within Active Channel. */
            if (GetChannel() > (config::GetBoolArg("-private") ? 3 : 2))
                return debug::error(FUNCTION, "channel out of Range.");

            /* Check that the time was within range. */
            if (GetBlockTime() > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT * 60)
                return debug::error(FUNCTION, "block timestamp too far in the future");


            /* Do not allow blocks to be accepted above the current block version. */
            if(nVersion == 0 || nVersion > (config::fTestNet.load() ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION))
                return debug::error(FUNCTION, "invalid block version");


            /* Only allow POS blocks in Version 4. */
            if(IsProofOfStake() && nVersion < 4)
                return debug::error(FUNCTION, "proof-of-stake rejected until version 4");


            /* Check the Proof of Work Claims. */
            if (IsProofOfWork() && !VerifyWork())
               return debug::error(FUNCTION, "invalid proof of work");


            /* Check the Network Launch Time-Lock. */
            if (nHeight > 0 && GetBlockTime() <= (config::fTestNet.load() ? NEXUS_TESTNET_TIMELOCK : NEXUS_NETWORK_TIMELOCK))
                return debug::error(FUNCTION, "block created before network time-lock");


            /* Check the Current Channel Time-Lock. */
            if (nHeight > 0 && GetBlockTime() < (config::fTestNet.load() ? CHANNEL_TESTNET_TIMELOCK[GetChannel()] : CHANNEL_NETWORK_TIMELOCK[GetChannel()]))
                return debug::error(FUNCTION, "block created before channel time-lock, please wait ", (config::fTestNet.load() ? CHANNEL_TESTNET_TIMELOCK[GetChannel()] : CHANNEL_NETWORK_TIMELOCK[GetChannel()]) - runtime::unifiedtimestamp(), " seconds");


            /* Check the Current Version Block Time-Lock. Allow Version (Current -1) Blocks for 1 Hour after Time Lock. */
            if (nVersion > 1 && nVersion == (config::fTestNet.load() ? TESTNET_BLOCK_CURRENT_VERSION - 1 : NETWORK_BLOCK_CURRENT_VERSION - 1) && (GetBlockTime() - 3600) > (config::fTestNet.load() ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
                return debug::error(FUNCTION, "version ", nVersion, " blocks have been obsolete for ", (runtime::unifiedtimestamp() - (config::fTestNet.load() ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2])), " seconds");


            /* Check the Current Version Block Time-Lock. */
            if (nVersion >= (config::fTestNet.load() ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION) && GetBlockTime() <= (config::fTestNet.load() ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
                return debug::error(FUNCTION, "version ", nVersion, " blocks are not accepted for ", (runtime::unifiedtimestamp() - (config::fTestNet.load() ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2])), " seconds");


            /* Check the producer transaction. */
            if(nHeight > 0 && IsProofOfWork() && !producer.IsCoinbase())
                return debug::error(FUNCTION, "producer transaction has to be coinbase for proof of work");

            /* Check the producer transaction. */
            if(nHeight > 0 && IsPrivate() && !producer.IsPrivate())
                return debug::error(FUNCTION, "producer transaction has to be authorize for proof of work");


            /* Check the producer transaction. */
            if(GetChannel() == 0 && !producer.IsTrust())
                return debug::error(FUNCTION, "producer transaction has to be trust for proof of stake");


            /* Check coinbase/coinstake timestamp is at least 20 minutes before block time */
            if (GetBlockTime() > (uint64_t)producer.nTimestamp + ((nVersion < 4) ? 1200 : 3600))
                return debug::error(FUNCTION, "producer transaction timestamp is too early");


            /* Proof of stake specific checks. */
            if(IsProofOfStake())
            {
                /* Check for nonce of zero values. */
                if(nNonce == 0)
                    return debug::error(FUNCTION, "proof of stake can't have Nonce value of zero");

                /* Check the trust time is before Unified timestamp. */
                if(producer.nTimestamp > (runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT))
                    return debug::error(FUNCTION, "trust timestamp too far in the future");

                /* Make Sure Trust Transaction Time is Before Block. */
                if (producer.nTimestamp > GetBlockTime())
                    return debug::error(FUNCTION, "trust timestamp is ahead of block timestamp");
            }


            /* Check for duplicate txid's */
            std::set<uint512_t> uniqueTx;


            /* Missing transactions. */
            std::vector< std::pair<uint8_t, uint512_t> > missingTx;


            /* Get the hashes for the merkle root. */
            std::vector<uint512_t> vHashes;


            /* Only do producer transaction on non genesis. */
            if(nHeight > 0)
            {
                /* Get producer hash. */
                uint512_t hashProducer = producer.GetHash();

                /* Add producer to merkle tree list. */
                vHashes.push_back(hashProducer);

                /* Add producer to unique transactions. */
                uniqueTx.insert(hashProducer);

                /* Calculate merkle root with system memory. */
                if(ssSystem.size() != 0)
                {
                    /* Get the hash of the system register. */
                    uint512_t hashSystem = LLC::SK512(ssSystem.Bytes());

                    /* Add system hash to merkle tree list. */
                    vHashes.push_back(hashSystem);

                    /* Add system hash to unique hashes. */
                    uniqueTx.insert(hashSystem);
                }
            }


            /* Get the signature operations for legacy tx's. */
            uint32_t nSigOps = 0;


            /* Check all the transactions. */
            for(const auto& proof : vtx)
            {
                /* Insert txid into set to check for duplicates. */
                uniqueTx.insert(proof.second);

                /* Push back this hash for merkle root. */
                vHashes.push_back(proof.second);

                /* Basic checks for legacy transactions. */
                if(proof.first == TYPE::LEGACY_TX)
                {
                    /* Check the memory pool. */
                    Legacy::Transaction tx;
                    if(!mempool.Get(proof.second, tx) && !LLD::legacyDB->ReadTx(proof.second, tx))
                    {
                        missingTx.push_back(proof);
                        continue;
                    }

                    /* Check the transaction timestamp. */
                    if(GetBlockTime() < (uint64_t) tx.nTime)
                        return debug::error(FUNCTION, "block timestamp earlier than transaction timestamp");

                    /* Check the transaction for validitity. */
                    if(!tx.CheckTransaction())
                        return debug::error(FUNCTION, "check transaction failed.");
                }

                /* Basic checks for tritium transactions. */
                else if(proof.first == TYPE::TRITIUM_TX)
                {
                    /* Check the memory pool. */
                    TAO::Ledger::Transaction tx;
                    if(!mempool.Has(proof.second) && !LLD::legDB->ReadTx(proof.second, tx))
                    {
                        missingTx.push_back(proof);
                        continue;
                    }
                }
                else
                    return debug::error(FUNCTION, "unknown transaction type");
            }

            /* Fail and ask for response of missing transctions. */
            if(missingTx.size() > 0 && nHeight > 0)
            {
                std::vector<LLP::CInv> vInv;
                for(const auto& tx : missingTx)
                    vInv.push_back(LLP::CInv(tx.second, tx.first == TYPE::TRITIUM_TX ? LLP::MSG_TX_TRITIUM : LLP::MSG_TX_LEGACY));

                vInv.push_back(LLP::CInv(GetHash(), LLP::MSG_BLOCK_TRITIUM));
                if(LLP::TRITIUM_SERVER)
                    LLP::TRITIUM_SERVER->Relay(LLP::GET_DATA, vInv);

                //NodeType* pnode;
                //pnode->PushMessage("GetInv("....")");
                //send pnode as a template for this method.

                //TODO: ask the sending node for the missing transactions
                //Keep a list of missing transactions and then send a work queue once done
                return debug::error(FUNCTION, "block contains missing transactions");
            }


            /* Check for duplicate txid's. */
            if (uniqueTx.size() != vHashes.size())
                return debug::error(FUNCTION, "duplicate transaction");


            /* Check the signature operations for legacy. */
            if (nSigOps > MAX_BLOCK_SIGOPS)
                return debug::error(FUNCTION, "out-of-bounds SigOpCount");


            /* Check the merkle root. */
            if (hashMerkleRoot != BuildMerkleTree(vHashes))
                return debug::error(FUNCTION, "hashMerkleRoot mismatch");

            /* Get the key from the producer. */
            if(nHeight > 0)
            {
                /* Create the key to check. */
                #if defined USE_FALCON
                LLC::FLKey key;
                #else
                LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);
                #endif
                key.SetPubKey(producer.vchPubKey);

                /* Check the Block Signature. */
                if (!VerifySignature(key))
                    return debug::error(FUNCTION, "bad block signature");
            }

            return true;
        }


        /** Accept a tritium block. **/
        bool TritiumBlock::Accept() const
        {
            /* Read leger DB for duplicate block. */
            if(LLD::legDB->HasBlock(GetHash()))
                return debug::error(FUNCTION, "already have block ", GetHash().ToString().substr(0, 20));


            /* Read leger DB for previous block. */
            TAO::Ledger::BlockState statePrev;
            if(!LLD::legDB->ReadBlock(hashPrevBlock, statePrev))
                return debug::error(FUNCTION, "previous block state not found");


            /* Check the Height of Block to Previous Block. */
            if(statePrev.nHeight + 1 != nHeight)
                return debug::error(FUNCTION, "incorrect block height.");


            /* Get the proof hash for this block. */
            uint1024_t hash = (nVersion < 5 ? GetHash() : GetChannel() == 0 ? StakeHash() : ProofHash());


            /* Get the target hash for this block. */
            uint1024_t hashTarget = LLC::CBigNum().SetCompact(nBits).getuint1024();


            /* Verbose logging of proof and target. */
            debug::log(2, "  proof:  ", hash.ToString().substr(0, 30));


            /* Channel switched output. */
            if(GetChannel() == 1)
                debug::log(2, "  prime cluster verified of size ", GetDifficulty(nBits, 1));
            else
                debug::log(2, "  target: ", hashTarget.ToString().substr(0, 30));


            /* Check that the nBits match the current Difficulty. **/
            if (nBits != GetNextTargetRequired(statePrev, GetChannel()))
                return debug::error(FUNCTION, "incorrect proof-of-work/proof-of-stake");


            /* Check That Block timestamp is not before previous block. */
            if (GetBlockTime() <= statePrev.GetBlockTime())
                return debug::error(FUNCTION, "block's timestamp too early Block: ", GetBlockTime(), " Prev: ",
                statePrev.GetBlockTime());


            /* Check that Block is Descendant of Hardened Checkpoints. */
            if(!ChainState::Synchronizing() && !IsDescendant(statePrev))
                return debug::error(FUNCTION, "not descendant of last checkpoint");


            /* Check the block proof of work rewards. */
            if(IsProofOfWork())
            {
                /* Get the stream from coinbase. */
                producer.ssOperation.seek(1, STREAM::BEGIN); //set the read position to where reward will be.

                /* Read the mining reward. */
                uint64_t nMiningReward;
                producer.ssOperation >> nMiningReward;

                /* Check that the Mining Reward Matches the Coinbase Calculations. */
                if (nMiningReward != GetCoinbaseReward(statePrev, GetChannel(), 0))
                    return debug::error(FUNCTION, "miner reward mismatch ", nMiningReward, " : ",
                         GetCoinbaseReward(statePrev, GetChannel(), 0));

                 /* Check that the producer is a valid transaction. */
                 if(!producer.IsValid())
                     return debug::error(FUNCTION, "producer transaction is invalid");
            }
            else if (IsProofOfStake())
            {
                /* Check that the Coinbase / CoinstakeTimstamp is after Previous Block. */
                if (producer.nTimestamp < statePrev.GetBlockTime())
                    return debug::error(FUNCTION, "coinstake transaction too early");

                /* Check the proof of stake. */
                if(!CheckStake())
                    return debug::error(FUNCTION, "proof of stake is invalid");
            }
            else if (IsPrivate())
            {
                /* Check that the producer is a valid transaction. */
                if(!producer.IsValid())
                    return debug::error(FUNCTION, "producer transaction is invalid");

                /* Check producer for correct genesis. */
                if(producer.hashGenesis != uint256_t("0xb5a74c14508bd09e104eff93d86cbbdc5c9556ae68546895d964d8374a0e9a41"))
                    return debug::error(FUNCTION, "invalid genesis generated");
            }


            /* Check legacy transactions for finality. */
            for(const auto & tx : vtx)
            {
                /* Only work on tritium transactions for now. */
                if(tx.first == TYPE::LEGACY_TX)
                {
                    /* Check if in memory pool. */
                    Legacy::Transaction txCheck;
                    if(!mempool.Get(tx.second, txCheck))
                        return debug::error(FUNCTION, "transaction is not in memory pool");

                    /* Check legacy transaction for finality. */
                    if (!txCheck.IsFinal(nHeight, GetBlockTime()))
                        return debug::error(FUNCTION, "contains a non-final transaction");
                }
                else if(tx.first == TYPE::TRITIUM_TX)
                {
                    /* Check if in memory pool. */
                    Transaction txCheck;
                    if(!mempool.Get(tx.second, txCheck))
                        return debug::error(FUNCTION, "transaction is not in memory pool");

                    /* Check the transaction for validity. */
                    if (!txCheck.IsValid())
                        return debug::error(FUNCTION, "contains an invalid transaction");
                }
            }

            /* Process the block state. */
            TAO::Ledger::BlockState state(*this);

            /* Add the producer transaction */
            TAO::Ledger::mempool.AddUnchecked(producer);

            /* Accept the block state. */
            if(!state.Index())
            {
                /* Remove producer from temporary mempool. */
                TAO::Ledger::mempool.Remove(producer.GetHash());

                return false;
            }

            return true;
        }


        /* Get the score of the current trust block. */
        bool TritiumBlock::TrustScore(uint64_t& nScore) const
        {
            /* Reset the operation stream. */
            producer.ssOperation.seek(65, STREAM::BEGIN);

            /* The current calculated trust score. */
            producer.ssOperation >> nScore;

            return true;
        }


        /* Check the proof of stake calculations. */
        bool TritiumBlock::CheckStake() const
        {
            /* Check the proof hash of the stake block on version 5 and above. */
            LLC::CBigNum bnTarget;
            bnTarget.SetCompact(nBits);
            if(StakeHash() > bnTarget.getuint1024())
                return debug::error(FUNCTION, "proof of stake hash not meeting target");

            /* Get the trust object register. */
            TAO::Register::Object object;

            /* Deserialize from the stream. */
            producer.ssRegister.seek(1, STREAM::BEGIN);
            producer.ssRegister >> object;

            /* Parse the object. */
            if(!object.Parse())
                return debug::error(FUNCTION, "failed to parse object register from pre-state");

            /* Weight for Trust transactions combine block weight and stake weight. */
            double nTrustWeight = 0.0, nBlockWeight = 0.0;
            uint64_t nTrustAge = 0, nBlockAge = 0, nStake = 0;
            if(producer.IsTrust())
            {
                /* Get the score and make sure it all checks out. */
                if(!TrustScore(nTrustAge))
                    return debug::error(FUNCTION, "failed to get trust score");

                /* Get the weights with the block age. */
                if(!BlockAge(nBlockAge))
                    return debug::error(FUNCTION, "failed to get block age");

                /* Trust Weight Continues to grow the longer you have staked and higher your interest rate */
                nTrustWeight = std::min(90.0, (((44.0 * log(((2.0 * nTrustAge) /
                    (60 * 60 * 24 * 28 * 3)) + 1.0)) / log(3))) + 1.0);

                /* Block Weight Reaches Maximum At Trust Key Expiration. */
                nBlockWeight = std::min(10.0, (((9.0 * log(((2.0 * nBlockAge) /
                    ((config::fTestNet.load() ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN))) + 1.0)) / log(3))) + 1.0);

                /* Get stake from the object register stake balance. */
                nStake = object.get<uint64_t>("stake");
            }

            /* Weight for Gensis transactions are based on your coin age. */
            else
            {
                /* Genesis transaction can't have any transactions. */
                if(vtx.size() != 1)
                    return debug::error(FUNCTION, "genesis can't include transactions");

                /* Calculate the Average Coinstake Age. */
                uint64_t nCoinAge = (uint64_t(nTime) - object.nTimestamp);

                /* Genesis has to wait for one full trust key timespan. */
                if(nCoinAge < (config::fTestNet.load() ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN))
                    return debug::error(FUNCTION, "genesis age is immature");

                /* Trust Weight For Genesis Transaction Reaches Maximum at 90 day Limit. */
                nTrustWeight = std::min(10.0, (((9.0 * log(((2.0 * nCoinAge) / (60 * 60 * 24 * 28 * 3)) + 1.0)) / log(3))) + 1.0);

                /* Get stake from the object register stake balance. */
                nStake = object.get<uint64_t>("balance");
            }

            /* Check the stake balance. */
            if(nStake == 0)
                return debug::error(FUNCTION, "cannot stake if stake balance is zero");

            /* Calculate the energy efficiency thresholds. */
            double nThreshold = ((nTime - producer.nTimestamp) * 100.0) / nNonce;
            double nRequired  = ((108.0 - nTrustWeight - nBlockWeight) * TAO::Ledger::MAX_STAKE_WEIGHT) / nStake;

            /* Check that the threshold was not violated. */
            if(nThreshold < nRequired)
                return debug::error(FUNCTION, "energy threshold too low ", nThreshold, " required ", nRequired);

            /* Verbose logging. */
            debug::log(2, FUNCTION,
                "hash=", StakeHash().ToString().substr(0, 20), ", ",
                "target=", bnTarget.getuint1024().ToString().substr(0, 20), ", ",
                "trustscore=", nTrustAge, ", ",
                "blockage=", nBlockAge, ", ",
                "trustweight=", nTrustWeight, ", ",
                "blockweight=", nBlockWeight, ", ",
                "threshold=", nThreshold, ", ",
                "required=", nRequired, ", ",
                "time=", (nTime - producer.nTimestamp), ", ",
                "nonce=", nNonce, ")");

            return true;
        }


        /* Get the current block age of the trust key. */
        bool TritiumBlock::BlockAge(uint64_t& nAge) const
        {
            /* No age for non proof of stake or non version 5 blocks */
            if(!IsProofOfStake() || nVersion < 5)
                return debug::error(FUNCTION, "not proof of stake / version < 5");

            /* Genesis has an age 0. */
            if(producer.IsGenesis())
            {
                nAge = 0;
                return true;
            }

            /* Reset the operation stream. */
            producer.ssOperation.seek(1, STREAM::BEGIN);

            /* Get the previous producer. */
            uint512_t hashLastTrust;
            producer.ssOperation >> hashLastTrust;

            /* The current calculated trust score. */
            uint64_t nTrustScore;
            producer.ssOperation >> nTrustScore;

            /* Check that the last trust block is in the block database. */
            TAO::Ledger::BlockState stateLast;
            if(!LLD::legDB->ReadBlock(hashLastTrust, stateLast))
                return debug::error(FUNCTION, "last block not in database");

            /* Check that the previous block is in the block database. */
            TAO::Ledger::BlockState statePrev;
            if(!LLD::legDB->ReadBlock(hashPrevBlock, statePrev))
                return debug::error(FUNCTION, "prev block not in database");

            /* Read the previous block from disk. */
            nAge = statePrev.GetBlockTime() - stateLast.GetBlockTime();

            return true;
        }


        /* Verify the Proof of Work satisfies network requirements. */
        bool TritiumBlock::VerifyWork() const
        {
            /* This override adds support for verifying the stake hash on the staking channel */
            if (nChannel == 0)
            {
                LLC::CBigNum bnTarget;
                bnTarget.SetCompact(nBits);

                /* Check that the hash is within range. */
                if (bnTarget <= 0 || bnTarget > bnProofOfWorkLimit[nChannel])
                    return debug::error(FUNCTION, "Proof of stake hash not in range");

                if (StakeHash() > bnTarget.getuint1024())
                    return debug::error(FUNCTION, "Proof of stake not meeting target");

                return true;
            }

            return Block::VerifyWork();
        }


        /* Sign the block with the key that found the block. */
        #if defined USE_FALCON
        bool TritiumBlock::GenerateSignature(const LLC::FLKey& key)
        {
            return key.Sign(GetHash().GetBytes(), vchBlockSig);
        }
        #endif


        /* Check that the block signature is a valid signature. */
        #if defined USE_FALCON
        bool TritiumBlock::VerifySignature(const LLC::FLKey& key) const
        {
            if (vchBlockSig.empty())
                return false;

            return key.Verify(GetHash().GetBytes(), vchBlockSig);
        }
        #endif


        /* Prove that you staked a number of seconds based on weight */
        uint1024_t TritiumBlock::StakeHash() const
        {
            return Block::StakeHash( producer.IsGenesis(), producer.hashGenesis);
        }
    }
}
