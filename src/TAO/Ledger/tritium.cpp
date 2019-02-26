/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <cmath>

#include <LLC/include/key.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/chainstate.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Copy Constructor. **/
        TritiumBlock::TritiumBlock(const BlockState& state)
        : Block(state)
        , producer()
        , vtx(state.vtx)
        {
            vtx.erase(vtx.begin());

            /* Read the producer transaction from disk. */
            if(!LLD::legDB->ReadTx(state.vtx[0].second, producer))
                debug::error(FUNCTION, "failed to read producer");
        }

        /* For debugging Purposes seeing block state data dump */
        std::string TritiumBlock::ToString() const
        {
            return debug::safe_printstr("Block("
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


        /* For debugging purposes, printing the block to stdout */
        void TritiumBlock::print() const
        {
            debug::log(0, ToString());
        }


        /* Checks if a block is valid if not connected to chain. */
        bool TritiumBlock::Check() const
        {
            /* Check the Size limits of the Current Block. */
            if (::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
                return debug::error(FUNCTION, "size ", ::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION), " limits failed ", MAX_BLOCK_SIZE);


            /* Make sure the Block was Created within Active Channel. */
            if (GetChannel() > 2)
                return debug::error(FUNCTION, "channel out of Range.");


            /* Check that the time was within range. */
            if (GetBlockTime() > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
                return debug::error(FUNCTION, "block timestamp too far in the future");


            /* Do not allow blocks to be accepted above the current block version. */
            if(nVersion == 0 || nVersion > (config::fTestNet ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION))
                return debug::error(FUNCTION, "invalid block version");


            /* Only allow POS blocks in Version 4. */
            if(IsProofOfStake() && nVersion < 4)
                return debug::error(FUNCTION, "proof-of-stake rejected until version 4");


            /* Check the Proof of Work Claims. */
            //if (IsProofOfWork() && !VerifyWork())
            //    return debug::error(FUNCTION, "invalid proof of work");


            /* Check the Network Launch Time-Lock. */
            if (nHeight > 0 && GetBlockTime() <= (config::fTestNet ? NEXUS_TESTNET_TIMELOCK : NEXUS_NETWORK_TIMELOCK))
                return debug::error(FUNCTION, "block created before network time-lock");


            /* Check the Current Channel Time-Lock. */
            if (nHeight > 0 && GetBlockTime() < (config::fTestNet ? CHANNEL_TESTNET_TIMELOCK[GetChannel()] : CHANNEL_NETWORK_TIMELOCK[GetChannel()]))
                return debug::error(FUNCTION, "block created before channel time-lock, please wait ", (config::fTestNet ? CHANNEL_TESTNET_TIMELOCK[GetChannel()] : CHANNEL_NETWORK_TIMELOCK[GetChannel()]) - runtime::unifiedtimestamp(), " seconds");


            /* Check the Current Version Block Time-Lock. Allow Version (Current -1) Blocks for 1 Hour after Time Lock. */
            if (nVersion > 1 && nVersion == (config::fTestNet ? TESTNET_BLOCK_CURRENT_VERSION - 1 : NETWORK_BLOCK_CURRENT_VERSION - 1) && (GetBlockTime() - 3600) > (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
                return debug::error(FUNCTION, "version ", nVersion, " blocks have been obsolete for ", (runtime::unifiedtimestamp() - (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2])), " seconds");


            /* Check the Current Version Block Time-Lock. */
            if (nVersion >= (config::fTestNet ? TESTNET_BLOCK_CURRENT_VERSION : NETWORK_BLOCK_CURRENT_VERSION) && GetBlockTime() <= (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2]))
                return debug::error(FUNCTION, "version ", nVersion, " blocks are not accepted for ", (runtime::unifiedtimestamp() - (config::fTestNet ? TESTNET_VERSION_TIMELOCK[TESTNET_BLOCK_CURRENT_VERSION - 2] : NETWORK_VERSION_TIMELOCK[NETWORK_BLOCK_CURRENT_VERSION - 2])), " seconds");


            /* Check the producer transaction. */
            if(nHeight > 0)
            {
                /* Check the coinbase if not genesis. */
                if(GetChannel() > 0 && !producer.IsCoinbase())
                    return debug::error(FUNCTION, "producer transaction has to be coinbase for proof of work");

                /* Check that the producer is a valid transactions. */
                if(!producer.IsValid())
                    return debug::error(FUNCTION, "producer transaction is invalid");
            }


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
            std::vector<uint512_t> missingTx;


            /* Get the hashes for the merkle root. */
            std::vector<uint512_t> vHashes;


            /* Only do producer transaction on non genesis. */
            if(nHeight > 0)
                vHashes.push_back(producer.GetHash());


            /* Get the signature operations for legacy tx's. */
            uint32_t nSigOps = 0;


            /* Check all the transactions. */
            for(const auto& tx : vtx)
            {

                /* Insert txid into set to check for duplicates. */
                uniqueTx.insert(tx.second);

                /* Push back this hash for merkle root. */
                vHashes.push_back(tx.second);

                /* Basic checks for legacy transactions. */
                if(tx.first == TYPE::LEGACY_TX)
                {
                    /* Check the memory pool. */
                    Legacy::Transaction txMem;
                    if(!mempool.Get(tx.second, txMem))
                    {
                        missingTx.push_back(tx.second);
                        continue;
                    }

                    /* Check the transaction timestamp. */
                    if(GetBlockTime() < (uint64_t) txMem.nTime)
                        return debug::error(FUNCTION, "block timestamp earlier than transaction timestamp");

                    /* Check the transaction for validitity. */
                    if(!txMem.CheckTransaction())
                        return debug::error(FUNCTION, "check transaction failed.");
                }

                /* Basic checks for tritium transactions. */
                else if(tx.first == TYPE::TRITIUM_TX)
                {
                    /* Check the memory pool. */
                    TAO::Ledger::Transaction txMem;
                    if(!mempool.Has(tx.second))
                    {
                        missingTx.push_back(tx.second);
                        continue;
                    }
                }
                else
                    return debug::error(FUNCTION, "unknown transaction type");
            }

            /* Fail and ask for response of missing transctions. */
            if(missingTx.size() > 0 && nHeight > 0)
            {
                //NodeType* pnode;
                //pnode->PushMessage("GetInv("....")");
                //send pnode as a template for this method.

                //TODO: ask the sending node for the missing transactions
                //Keep a list of missing transactions and then send a work queue once done
                return debug::error(FUNCTION, "block contains missing transactions");
            }


            /* Check for duplicate txid's. */
            if (uniqueTx.size() != vtx.size())
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
                LLC::ECKey key(NID_brainpoolP512t1, 64);
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
            //if (GetBlockTime() <= statePrev.GetBlockTime())
            //    return debug::error(FUNCTION, "block's timestamp too early Block: ", GetBlockTime(), " Prev: ",
            //     statePrev.GetBlockTime());


            /* Check that Block is Descendant of Hardened Checkpoints. */
            if(!ChainState::Synchronizing() && !IsDescendant(statePrev))
                return debug::error(FUNCTION, "not descendant of last checkpoint");


            /* Check the block proof of work rewards. */
            if(IsProofOfWork() && nVersion >= 3)
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

                    if (!txCheck.IsFinal(nHeight, GetBlockTime()))
                        return debug::error(FUNCTION, "contains a non-final transaction");
                }
            }

            /* Process the block state. */
            TAO::Ledger::BlockState state(*this);

            /* Accept the block state. */
            if(!state.Index())
                return false;

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

            /* Weight for Trust transactions combine block weight and stake weight. */
            double nTrustWeight = 0.0, nBlockWeight = 0.0;
            uint32_t nTrustAge = 0, nBlockAge = 0;
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
                    ((config::fTestNet ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN))) + 1.0)) / log(3))) + 1.0);

            }

            /* Weight for Gensis transactions are based on your coin age. */
            else
            {
                /* Genesis transaction can't have any transactions. */
                if(vtx.size() != 1)
                    return debug::error(FUNCTION, "genesis can't include transactions");

                /* Calculate the Average Coinstake Age. */
                uint64_t nCoinAge = 0;
                //TODO: get the age of this transaction with genesis flags.
                //if(!vtx[0].CoinstakeAge(nCoinAge))
                //    return debug::error(FUNCTION, "failed to get coinstake age");

                /* Genesis has to wait for one full trust key timespan. */
                if(nCoinAge < (config::fTestNet ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN))
                    return debug::error(FUNCTION, "genesis age is immature");

                /* Trust Weight For Genesis Transaction Reaches Maximum at 90 day Limit. */
                nTrustWeight = std::min(10.0, (((9.0 * log(((2.0 * nCoinAge) / (60 * 60 * 24 * 28 * 3)) + 1.0)) / log(3))) + 1.0);
            }

            /* Check the energy efficiency requirements. */
            uint64_t nStake = 0;
            if(!producer.ExtractStake(nStake))
                return debug::error(FUNCTION, "failed to extract the stake amount");

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


        /* Check the calculated trust score meets published one. */
        bool TritiumBlock::CheckTrust() const
        {
            /* No trust score for non proof of stake (for now). */
            if(!IsProofOfStake())
                return debug::error(FUNCTION, "not proof of stake");

            /* Extract the trust key from the coinstake. */
            uint256_t cKey = producer.hashGenesis;

            /* Genesis has a trust score of 0. */
            if(producer.IsGenesis())
                return true;

            /* Version 5 - last trust block. */
            uint1024_t hashLastBlock;
            uint32_t   nSequence;
            uint32_t   nTrustScore;

            /* Extract values from coinstake vin. */
            if(!producer.ExtractTrust(hashLastBlock, nSequence, nTrustScore))
                return debug::error(FUNCTION, "failed to extract values from script");

            /* Check that the last trust block is in the block database. */
            TAO::Ledger::BlockState stateLast;
            if(!LLD::legDB->ReadBlock(hashLastBlock, stateLast))
                return debug::error(FUNCTION, "last block not in database");

            /* Check that the previous block is in the block database. */
            TAO::Ledger::BlockState statePrev;
            if(!LLD::legDB->ReadBlock(hashPrevBlock, statePrev))
                return debug::error(FUNCTION, "prev block not in database");

            /* Get the last coinstake transaction. */
            Transaction txLast;
            //TODO: handle a previous block that is from before tritium activation time-lock

            /* Enforce the minimum trust key interval of 120 blocks. */
            if(nHeight - stateLast.nHeight < (config::fTestNet ? TAO::Ledger::TESTNET_MINIMUM_INTERVAL : TAO::Ledger::MAINNET_MINIMUM_INTERVAL))
                return debug::error(FUNCTION, "trust key interval below minimum interval ", nHeight - stateLast.nHeight);

            /* Extract the last trust key */
            uint256_t keyLast = txLast.hashGenesis;

            /* Ensure the last block being checked is the same trust key. */
            if(keyLast != cKey)
                return debug::error(FUNCTION,
                    "trust key in previous block ", cKey.ToString().substr(0, 20),
                    " to this one ", keyLast.ToString().substr(0, 20));

            /* Placeholder in case previous block is a version 4 block. */
            uint32_t nScorePrev = 0;
            uint32_t nScore     = 0;

            /* If previous block is genesis, set previous score to 0. */
            if(txLast.IsGenesis())
            {
                /* Enforce sequence number 1 if previous block was genesis */
                if(nSequence != 1)
                    return debug::error("CBlock::CheckTrust() : first trust block and sequence is not 1 (%u)", nSequence);

                /* Genesis results in a previous score of 0. */
                nScorePrev = 0;
            }

            /* Version 4 blocks need to get score from previous blocks calculated score from the trust pool. */
            else if(stateLast.nVersion < 5)
            {
                //TODO: handle version 4 trust keys here
            }

            /* Version 5 blocks that are trust must pass sequence checks. */
            else
            {
                /* The last block of previous. */
                uint1024_t hashBlockPrev = 0; //dummy variable unless we want to do recursive checking of scores all the way back to genesis

                /* Extract the value from the previous block. */
                uint32_t nSequencePrev;
                if(!txLast.ExtractTrust(hashBlockPrev, nSequencePrev, nScorePrev))
                    return debug::error(FUNCTION, "failed to extract trust");

                /* Enforce Sequence numbering, must be +1 always. */
                if(nSequence != nSequencePrev + 1)
                    return debug::error(FUNCTION, "previous sequence broken");
            }

            /* The time it has been since the last trust block for this trust key. */
            uint32_t nTimespan = (statePrev.GetBlockTime() - stateLast.GetBlockTime());

            /* Timespan less than required timespan is awarded the total seconds it took to find. */
            if(nTimespan < (config::fTestNet ? TRUST_KEY_TIMESPAN_TESTNET : TRUST_KEY_TIMESPAN))
                nScore = nScorePrev + nTimespan;

            /* Timespan more than required timespan is penalized 3 times the time it took past the required timespan. */
            else
            {
                /* Calculate the penalty for score (3x the time). */
                uint32_t nPenalty = (nTimespan - (config::fTestNet ?
                    TRUST_KEY_TIMESPAN_TESTNET : TRUST_KEY_TIMESPAN)) * 3;

                /* Catch overflows and zero out if penalties are greater than previous score. */
                if(nPenalty > nScorePrev)
                    nScore = 0;
                else
                    nScore = (nScorePrev - nPenalty);
            }

            /* Set maximum trust score to seconds passed for interest rate. */
            if(nScore > (60 * 60 * 24 * 28 * 13))
                nScore = (60 * 60 * 24 * 28 * 13);

            /* Debug output. */
            debug::log(2, FUNCTION,
                "score=", nScore, ", ",
                "prev=", nScorePrev, ", ",
                "timespan=", nTimespan, ", ",
                "change=", (int32_t)(nScore - nScorePrev), ")"
            );

            /* Check that published score in this block is equivilent to calculated score. */
            if(nTrustScore != nScore)
                return debug::error(FUNCTION, "published trust score ", nTrustScore, " not meeting calculated score ", nScore);

            return true;
        }


        /* Get the current block age of the trust key. */
        bool TritiumBlock::BlockAge(uint32_t& nAge) const
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

            /* Version 5 - last trust block. */
            uint1024_t hashLastBlock;
            uint32_t   nSequence;
            uint32_t   nTrustScore;

            /* Extract values from coinstake vin. */
            if(!producer.ExtractTrust(hashLastBlock, nSequence, nTrustScore))
                return debug::error(FUNCTION, "failed to extract values from script");

            /* Check that the previous block is in the block database. */
            TAO::Ledger::BlockState stateLast;
            if(!LLD::legDB->ReadBlock(hashLastBlock, stateLast))
                return debug::error(FUNCTION, "last block not in database");

            /* Check that the previous block is in the block database. */
            TAO::Ledger::BlockState statePrev;
            if(!LLD::legDB->ReadBlock(hashPrevBlock, statePrev))
                return debug::error(FUNCTION, "prev block not in database");

            /* Read the previous block from disk. */
            nAge = statePrev.GetBlockTime() - stateLast.GetBlockTime();

            return true;
        }


        /* Get the score of the current trust block. */
        bool TritiumBlock::TrustScore(uint32_t& nScore) const
        {
            /* Genesis has an age 0. */
            if(producer.IsGenesis())
            {
                nScore = 0;
                return true;
            }

            /* Version 5 - last trust block. */
            uint1024_t hashLastBlock;
            uint32_t   nSequence;
            uint32_t   nTrustScore;

            /* Extract values from coinstake vin. */
            if(!producer.ExtractTrust(hashLastBlock, nSequence, nTrustScore))
                return debug::error(FUNCTION, "failed to extract values from script");

            /* Get the score from extract process. */
            nScore = nTrustScore;

            return true;
        }


        /* Prove that you staked a number of seconds based on weight */
        uint1024_t TritiumBlock::StakeHash() const
        {
            return Block::StakeHash( producer.IsGenesis(), producer.hashGenesis);
        }
    }
}
