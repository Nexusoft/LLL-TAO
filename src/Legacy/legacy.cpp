/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <cmath>

#include <LLC/include/eckey.h>

#include <Legacy/types/legacy.h>
#include <Legacy/types/script.h>
#include <Legacy/include/ambassador.h>
#include <Legacy/include/constants.h>
#include <Legacy/include/evaluate.h>
#include <Legacy/include/trust.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/checkpoints.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/timelocks.h>

#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/syncblock.h>

#include <Util/include/args.h>
#include <Util/include/hex.h>
#include <Util/include/runtime.h>
#include <Util/include/softfloat.h>

#include <cmath>


namespace Legacy
{

    /* Verify the Signature is Valid for Last 2 Coinbase Tx Outputs. */
    bool VerifyAddress(const std::vector<uint8_t> script, const std::vector<uint8_t> sig)
    {
        if(script.size() != 37)
            return debug::error(FUNCTION, "Script Size not 37 Bytes");

        for(uint32_t i = 0; i < 37; ++i)
            if(script[i] != sig[i])
                return false;

        return true;
    }


    /* Compare Two Vectors Element by Element. */
    bool VerifyAddressList(const std::vector<uint8_t> script, const std::vector<uint8_t> sigs[13])
    {
        for(uint32_t i = 0; i < 13; ++i)
            if(VerifyAddress(script, sigs[i]))
                return true;

        return false;
    }


    /* The default constructor. */
    LegacyBlock::LegacyBlock()
    : Block ( )
    , nTime (static_cast<uint32_t>(runtime::unifiedtimestamp()))
    , vtx   ( )
    {
        SetNull();
    }


    /* Copy Constructor. */
    LegacyBlock::LegacyBlock(const LegacyBlock& block)
    : Block (block)
    , nTime (block.nTime)
    , vtx   (block.vtx)
    {
    }


    /* Move Constructor. */
    LegacyBlock::LegacyBlock(LegacyBlock&& block) noexcept
    : Block (std::move(block))
    , nTime (std::move(block.nTime))
    , vtx   (std::move(block.vtx))
    {
    }


    /* Copy Assignment. */
    LegacyBlock& LegacyBlock::operator=(const LegacyBlock& block)
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
        vtx            = block.vtx;

        return *this;
    }


    /** Move Assignment. **/
    LegacyBlock& LegacyBlock::operator=(LegacyBlock&& block) noexcept
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
        vtx            = std::move(block.vtx);

        return *this;
    }


    /* Default Destructor */
    LegacyBlock::~LegacyBlock()
    {
    }

    /* Copy Constructor. */
    LegacyBlock::LegacyBlock(const TAO::Ledger::BlockState& state)
    : Block(state)
    , nTime(state.nTime)
    , vtx()
    {
        /* Push back all the transactions from the state object. */
        for(const auto& item : state.vtx)
        {
            if(item.first == TAO::Ledger::TRANSACTION::LEGACY)
            {
                /* Read transaction from database */
                Transaction tx;
                if(!LLD::Legacy->ReadTx(item.second, tx))
                    throw debug::exception(FUNCTION, "failed to read tx ", item.second.SubString());

                vtx.push_back(tx);
            }
        }
    }


    /* Copy Constructor. */
    LegacyBlock::LegacyBlock(const TAO::Ledger::SyncBlock& block)
    : Block(block)
    , nTime(block.nTime)
    , vtx()
    {
        /* Check for version conversions. */
        if(block.nVersion >= 7)
            throw debug::exception(FUNCTION, "invalid sync block version for legacy block");

        /* Push back all the transactions from the state object. */
        for(const auto& item : block.vtx)
        {
            switch(item.first)
            {
                /* Check for legacy transactions. */
                case TAO::Ledger::TRANSACTION::LEGACY:
                {
                    /* Serialize stream. */
                    DataStream ssData(item.second, SER_DISK, LLD::DATABASE_VERSION);

                    /* Build the transaction. */
                    Legacy::Transaction tx;
                    ssData >> tx;

                    /* Add transaction to binary data. */
                    vtx.push_back(tx);

                    break;
                }

                default:
                    throw debug::exception(FUNCTION, "invalid transaction type for legacy block");
            }
        }
    }


    /*  Set the block to Null state. */
    void LegacyBlock::SetNull()
    {
        Block::SetNull();

        vtx.clear();
    }


    /* Update the nTime of the current block. */
    void LegacyBlock::UpdateTime()
    {
        nTime = static_cast<uint32_t>(std::max(TAO::Ledger::ChainState::tStateBest.load().GetBlockTime() + 1, runtime::unifiedtimestamp()));
    }


    /* Return the Block's current UNIX timestamp. */
    uint64_t LegacyBlock::GetBlockTime() const
    {
        return uint64_t(nTime);
    }


    /* For debugging Purposes seeing block state data dump */
    std::string LegacyBlock::ToString() const
    {
        return debug::safe_printstr("Legacy Block("
            VALUE("hash")     " = ", GetHash().SubString(), " ",
            VALUE("nVersion") " = ", nVersion, ", ",
            VALUE("hashPrevBlock") " = ", hashPrevBlock.SubString(), ", ",
            VALUE("hashMerkleRoot") " = ", hashMerkleRoot.SubString(), ", ",
            VALUE("nChannel") " = ", nChannel, ", ",
            VALUE("nHeight") " = ", nHeight, ", ",
            VALUE("nBits") " = ", nBits, ", ",
            VALUE("nNonce") " = ", nNonce, ", ",
            VALUE("nTime") " = ", nTime, ", ",
            VALUE("vchBlockSig") " = ", HexStr(vchBlockSig.begin(), vchBlockSig.end()), ", ",
            VALUE("vtx.size()") " = ", vtx.size(), ")"
        );
    }


    /* Checks if a block is valid if not connected to chain. */
    bool LegacyBlock::Check() const
    {
        /* Read ledger DB for duplicate block. */
        if(LLD::Ledger->HasBlock(GetHash()))
            return false;//debug::error(FUNCTION, "already have block ", GetHash().SubString(), " height ", nHeight);

        /* Check the Size limits of the Current Block. */
        if(::GetSerializeSize(*this, SER_NETWORK, LLP::PROTOCOL_VERSION) > TAO::Ledger::MAX_BLOCK_SIZE)
            return debug::error(FUNCTION, "size limits failed");

        /* Make sure the Block was Created within Active Channel. */
        if(GetChannel() > 2)
            return debug::error(FUNCTION, "channel out of range");

        /* Check that the time was within range. If before v7 activation, keep legacy block compatible with legacy setting. */
        if(GetBlockTime() > runtime::unifiedtimestamp() + runtime::maxdrift())
            return debug::error(FUNCTION, "block timestamp too far in the future");

        /* Check the Current Version Block Time-Lock. */
        if(!TAO::Ledger::BlockVersionActive(GetBlockTime(), nVersion))
            return debug::error(FUNCTION, "block created with invalid version");

        /* Check the Network Launch Time-Lock. */
        if(!TAO::Ledger::NetworkActive(GetBlockTime()))
            return debug::error(FUNCTION, "block created before network time-lock");

        /* Check the Current Channel Time-Lock. */
        if(!TAO::Ledger::ChannelActive(GetBlockTime(), GetChannel()))
            return debug::error(FUNCTION, "block created with invalid version");

        /* Check the Required Mining Outputs. */
        if(IsProofOfWork())
        {
            /* Get output size. */
            uint32_t nSize = vtx[0].vout.size();

            /* Check the Coinbase Tx Size. */
            if(nSize < 3)
                return debug::error(FUNCTION, "coinbase too small");

            /* Check the ambassador and developer addresses. */
            if(!config::fTestNet.load() && nHeight != 112283) //112283 is an exception before clean rules implemented for sigs
            {
                /* Check the ambassador signatures. */
                if(!VerifyAddressList(vtx[0].vout[nSize - 2].scriptPubKey,
                    (nVersion < 5) ? AMBASSADOR_SCRIPT_SIGNATURES : AMBASSADOR_SCRIPT_SIGNATURES_RECYCLED))
                    return debug::error(FUNCTION, "block ", nHeight, " ambassador signatures invalid");

                /* Check the developer signatures. */
                if(!VerifyAddressList(vtx[0].vout[nSize - 1].scriptPubKey,
                    (nVersion < 5) ? DEVELOPER_SCRIPT_SIGNATURES : DEVELOPER_SCRIPT_SIGNATURES_RECYCLED))
                    return debug::error(FUNCTION, "block ", nHeight, " developer signatures invalid");
            }

            /* Check the Coinbase Transaction is First, with no repetitions. */
            if(vtx.empty() || !vtx[0].IsCoinBase())
                return debug::error(FUNCTION, "first tx is not coinbase for proof of work");

            /* Check the Proof of Work Claims. */
            if(!TAO::Ledger::ChainState::Synchronizing() && !VerifyWork())
                return debug::error(FUNCTION, "invalid proof of work");
        }

         /* Ensure the Block is for Proof of Stake Only. */
        else if(IsProofOfStake())
        {
            /* Check for nNonce zero. */
            if(nHeight > 2392970 && nNonce == 0)
                return debug::error(FUNCTION, "stake cannot have nonce of 0");

            /* Check timestamp is within tolerated drift. */
            if(vtx[0].nTime > (runtime::unifiedtimestamp() + runtime::maxdrift()))
                return debug::error(FUNCTION, "coinstake too far in Future.");

            /* Make Sure Coinstake Transaction is First. */
            if(!vtx[0].IsCoinStake())
                return debug::error(FUNCTION, "first transaction non-coinstake ", vtx[0].GetHash().ToString());

            /* Make Sure Coinstake Transaction Time is Before Block. */
            if(vtx[0].nTime > nTime)
                return debug::error(FUNCTION, "coinstake ahead of block time");

            /* Check the Coinstake Transaction is First, with no repetitions. */
            if(vtx.empty() || !vtx[0].IsCoinStake())
                return debug::error(FUNCTION, "first tx is not coinstake for proof of stake");
        }

        /* Default catch. */
        else
            return debug::error(FUNCTION, "unknown block type");

        /* Check coinbase/coinstake timestamp is at least 20 minutes before block time */
        if(nTime > (uint64_t)vtx[0].nTime + ((nVersion < 4) ? 1200 : 3600))
            return debug::error(FUNCTION, "coinbase/coinstake timestamp is too early");

        /* Check for duplicate txid's */
        std::set<uint512_t> setUniqueTx;
        std::vector<uint512_t> vHashes;

        /* Get the signature operations for legacy tx's. */
        uint32_t nSigOps = 0;

        /* Check all the transactions. */
        uint32_t nSize = (uint32_t)vtx.size();
        for(uint32_t i = 0; i < nSize; ++i)
        {
            /* Check for coinbase / coinstake. */
            if(i != 0 && (vtx[i].IsCoinBase() || vtx[i].IsCoinStake()))
                return debug::error(FUNCTION, "more than one coinbase / coinstake");

            /* Get the tx hash. */
            uint512_t hashTx  = vtx[i].GetHash();

            /* Insert txid into set to check for duplicates. */
            setUniqueTx.insert(hashTx);
            vHashes.push_back(hashTx);

            /* Check the transaction timestamp. */
            if(nTime < (uint64_t)vtx[i].nTime)
                return debug::error(FUNCTION, "block timestamp earlier than transaction timestamp");

            /* Check the transaction for validity. */
            if(!vtx[i].Check())
                return debug::error(FUNCTION, "check transaction failed.");

            /* Calculate the signature operations. */
            nSigOps += vtx[i].GetLegacySigOpCount();
        }

        /* Check for duplicate txid's. */
        if(setUniqueTx.size() != vtx.size())
            return debug::error(FUNCTION, "duplicate transaction");

        /* Check the signature operations for legacy. */
        if(nSigOps > TAO::Ledger::MAX_BLOCK_SIGOPS)
            return debug::error(FUNCTION, "out-of-bounds SigOpCount");

        /* Check the merkle root. */
        if(hashMerkleRoot != BuildMerkleTree(vHashes))
            return debug::error(FUNCTION, "hashMerkleRoot mismatch");

        /* Get the key from the producer. */
        if(!TAO::Ledger::ChainState::Synchronizing())
        {
            /* Get a vector for the solver solutions. */
            std::vector<std::vector<uint8_t> > vSolutions;

            /* Switch for the transaction type. */
            TransactionType whichType;
            if(!Solver(vtx[0].vout[0].scriptPubKey, whichType, vSolutions))
                return debug::error(FUNCTION, "solver failed for check signature.");

            /* Check for public key type. */
            if(whichType == TX_PUBKEY)
            {
                /* Get the public key. */
                const std::vector<uint8_t>& vchPubKey = vSolutions[0];

                /* Get the key object. */
                LLC::ECKey key;
                if(!key.SetPubKey(vchPubKey))
                    return false;

                /* Check the Block Signature. */
                if(!VerifySignature(key))
                    return debug::error(FUNCTION, "bad block signature");
            }
        }

        return true;
    }


    /* Accept a block into the chain. */
    bool LegacyBlock::Accept() const
    {
        /* Print the block on verbose 2. */
        if(config::nVerbose >= 2)
            print();

        /* Read leger DB for previous block. */
        TAO::Ledger::BlockState statePrev;
        if(!LLD::Ledger->ReadBlock(hashPrevBlock, statePrev))
            return debug::error(FUNCTION, "previous block state not found");

        /* Check the Height of Block to Previous Block. */
        if(statePrev.nHeight + 1 != nHeight)
            return debug::error(FUNCTION, "incorrect block height.");

        /* Verbose logging of proof and target. */
        debug::log(2, "  proof:  ", (nVersion < 5 ? GetHash() : nChannel == 0 ? StakeHash() : ProofHash()).SubString(30));

        /* Channel switched output. */
        if(nChannel == 1)
            debug::log(2, "  prime cluster verified of size ", TAO::Ledger::GetDifficulty(nBits, 1));
        else
            debug::log(2, "  target: ", LLC::CBigNum().SetCompact(nBits).getuint1024().SubString(30));

        /* Check that the nBits match the current Difficulty. **/
        if(nBits != TAO::Ledger::GetNextTargetRequired(statePrev, nChannel))
            return debug::error(FUNCTION, "incorrect ", nBits,
                " proof-of-work/proof-of-stake ", TAO::Ledger::GetNextTargetRequired(statePrev, nChannel));

        /* Get the block time for this block. */
        uint64_t nBlockTime = GetBlockTime();

        /* Check That Block timestamp is not before previous block. */
        if(nBlockTime <= statePrev.GetBlockTime())
            return debug::error(FUNCTION, "block's timestamp too early Block: ", nBlockTime, " Prev: ", statePrev.GetBlockTime());

        /* Check that Block is Descendant of Hardened Checkpoints. */
        if(!TAO::Ledger::IsDescendant(statePrev))
            return debug::error(FUNCTION, "not descendant of last checkpoint");

        /* Check the block proof of work rewards. */
        if(IsProofOfWork() && nVersion != 2 && nHeight != 2061881 && nHeight != 2191756)
        {
            //This is skipped in version 2 blocks due to the disk coinbase bug from early 2014.
            //height of 2061881 is an exclusion height due to mutation attack 06/2018
            //height of 2191756 is an exclusion height due to mutation attack 09/2018

            //Reward checks were re-enabled in version 3 blocks
            uint32_t nSize = vtx[0].vout.size();

            /* Add up the Miner Rewards from Coinbase Tx Outputs. */
            uint64_t nMiningReward = 0;
            for(int32_t nIndex = 0; nIndex < nSize - 2; ++nIndex)
                nMiningReward += vtx[0].vout[nIndex].nValue;

            /* Check that the Mining Reward Matches the Coinbase Calculations. */
            if (nMiningReward / 1000 != TAO::Ledger::GetCoinbaseReward(statePrev, nChannel, 0) / 1000)
                return debug::error(FUNCTION, "miner reward mismatch ",
                    nMiningReward / 1000, " to ", TAO::Ledger::GetCoinbaseReward(statePrev, nChannel, 0) / 1000);

            /* Check that the Ambassador Reward Matches the Coinbase Calculations. */
            if (vtx[0].vout[nSize - 2].nValue / 1000 != TAO::Ledger::GetCoinbaseReward(statePrev, nChannel, 1) / 1000)
                return debug::error(FUNCTION, "ambassador reward mismatch ",
                    vtx[0].vout[nSize - 2].nValue / 1000, " to ", TAO::Ledger::GetCoinbaseReward(statePrev, nChannel, 1) / 1000);

            /* Check that the Developer Reward Matches the Coinbase Calculations. */
            if (vtx[0].vout[nSize - 1].nValue / 1000 != TAO::Ledger::GetCoinbaseReward(statePrev, nChannel, 2) / 1000)
                return debug::error(FUNCTION, "developer reward mismatch ",
                    vtx[0].vout[nSize - 1].nValue / 1000, " to ", TAO::Ledger::GetCoinbaseReward(statePrev, nChannel, 2) / 1000);

        }
        else if(IsProofOfStake())
        {
            /* Check that the Coinbase / CoinstakeTimestamp is after Previous Block. */
            if(vtx[0].nTime < statePrev.GetBlockTime())
                return debug::error(FUNCTION, "coinstake transaction too early");

            /* Check the claimed stake limits are met. */
            if(nVersion >= 5 && !CheckStake())
                return debug::error(FUNCTION, "invalid proof of stake");

            /* Check stake for version 4. */
            if(nVersion < 5 && !VerifyStake())
                return debug::error(FUNCTION, "invalid proof of stake");
        }

        /* Check that Transactions are Finalized. */
        for(const auto& tx : vtx)
            if(!tx.IsFinal(nHeight, nBlockTime))
                return debug::error(FUNCTION, "contains a non-final transaction");

        /* Process the block state. */
        TAO::Ledger::BlockState state(*this);

        /* Start the database transaction. */
        LLD::TxnBegin();

        /* Write the transactions. */
        for(const auto& tx : vtx)
        {
            /* Get the transaction hash. */
            const uint512_t hash = tx.GetHash();

            /* Write to disk. */
            if(!LLD::Legacy->WriteTx(hash, tx))
                return debug::error(FUNCTION, "failed to write tx to disk");
        }

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
    bool LegacyBlock::VerifyStake() const
    {
        /* Check the Block Hash with Weighted Hash to Target. */
        LLC::CBigNum bnTarget;
        bnTarget.SetCompact(nBits);
        if(GetHash() > bnTarget.getuint1024())
            return debug::error(FUNCTION, "proof of stake not meeting target");

        return true;
    }


    /* Check the proof of stake calculations. */
    bool LegacyBlock::CheckStake() const
    {
        /* Make static const for reducing repeated computation. */
        static const double LOG3 = log(3);

        /* Use appropriate settings for Testnet or Mainnet */
        static const uint32_t nTrustWeightBase = config::fTestNet ? TAO::Ledger::TRUST_WEIGHT_BASE_TESTNET : TAO::Ledger::TRUST_WEIGHT_BASE;
        static const uint32_t nMaxBlockAge = config::fTestNet ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN;
        static const uint32_t nMinimumCoinAge = config::fTestNet ? TAO::Ledger::MINIMUM_GENESIS_COIN_AGE_TESTNET : TAO::Ledger::MINIMUM_GENESIS_COIN_AGE;

        /* Check the proof hash of the stake block on version 5 and above. */
        LLC::CBigNum bnTarget;
        bnTarget.SetCompact(nBits);
        if(StakeHash() > bnTarget.getuint1024())
            return debug::error(FUNCTION, "proof of stake hash not meeting target");

        /* Weight for Trust transactions combine block weight and stake weight. */
        double nTrustWeight = 0.0;
        double nBlockWeight = 0.0;
        uint32_t nTrustScore = 0;
        uint32_t nBlockAge = 0;

        if(vtx[0].IsTrust())
        {
            /* Get the score and make sure it all checks out. */
            if(!TrustScore(nTrustScore))
                return debug::error(FUNCTION, "failed to get trust score");

            /* Get the weights with the block age. */
            if(!BlockAge(nBlockAge))
                return debug::error(FUNCTION, "failed to get block age");

            /* Trust Weight Continues to grow the longer you have staked and higher your interest rate */
            double nTrustWeightRatio = (double)nTrustScore / (double)nTrustWeightBase;
            nTrustWeight = std::min(90.0, (44.0 * log((2.0 * nTrustWeightRatio) + 1.0) / LOG3) + 1.0);

            /* Block Weight Reaches Maximum At Trust Key Expiration. */
            double nBlockAgeRatio = (double)nBlockAge / (double)nMaxBlockAge;
            nBlockWeight = std::min(10.0, (9.0 * log((2.0 * nBlockAgeRatio) + 1.0) / LOG3) + 1.0);

        }

        /* Weight for Genesis transactions are based on your coin age. */
        else
        {
            /* Genesis transaction can't have any transactions. */
            if(vtx.size() != 1)
                return debug::error(FUNCTION, "genesis can't include transactions");

            /* Calculate the Average Coinstake Age. */
            uint64_t nCoinAge = 0;
            if(!vtx[0].CoinstakeAge(nCoinAge))
                return debug::error(FUNCTION, "failed to get coinstake age");

            /* Genesis has to wait for one full trust key timespan. */
            if(nCoinAge < nMinimumCoinAge)
                return debug::error(FUNCTION, "genesis age is immature");

            /* Trust Weight For Genesis Transaction Reaches Maximum at 90 day Limit. */
            double nGenesisTrustRatio = (double)nCoinAge / (double)nTrustWeightBase;
            nTrustWeight = std::min(10.0, (9.0 * log((2.0 * nGenesisTrustRatio) + 1.0) / LOG3) + 1.0);

            /* Block Weight remains zero while staking for Genesis */
            nBlockWeight = 0.0;
        }

        /* Check the energy efficiency requirements. */
        double nRequired  = ((108.0 - nTrustWeight - nBlockWeight) * TAO::Ledger::MAX_STAKE_WEIGHT) / vtx[0].vout[0].nValue;
        double nThreshold = ((nTime - vtx[0].nTime) * 100.0) / nNonce;

        if(nThreshold < nRequired)
            return debug::error(FUNCTION, "energy threshold too low ", nThreshold, " required ", nRequired);

        /* Verbose logging. */
        debug::log(2, FUNCTION,
            "hash=", StakeHash().SubString(), ", ",
            "target=", bnTarget.getuint1024().SubString(), ", ",
            "trustscore=", nTrustScore, ", ",
            "blockage=", nBlockAge, ", ",
            "trustweight=", nTrustWeight, ", ",
            "blockweight=", nBlockWeight, ", ",
            "threshold=", nThreshold, ", ",
            "required=", nRequired, ", ",
            "time=", (nTime - vtx[0].nTime), ", ",
            "nonce=", nNonce, ")");

        return true;
    }


    /* Get the current block age of the trust key. */
    bool LegacyBlock::BlockAge(uint32_t& nAge) const
    {
        /* No age for non proof of stake or non version 5 blocks */
        if(!IsProofOfStake() || nVersion < 5)
            return debug::error(FUNCTION, "not proof of stake / version < 5");

        /* Genesis has an age 0. */
        if(vtx[0].IsGenesis())
        {
            nAge = 0;
            return true;
        }

        /* Version 5 - last trust block. */
        uint1024_t hashLastBlock;
        uint32_t   nSequence;
        uint32_t   nTrustScore;

        /* Extract values from coinstake vin. */
        if(!vtx[0].ExtractTrust(hashLastBlock, nSequence, nTrustScore))
            return debug::error(FUNCTION, "failed to extract values from script");

        /* Check that the previous block is in the block database. */
        TAO::Ledger::BlockState stateLast;
        if(!LLD::Ledger->ReadBlock(hashLastBlock, stateLast))
            return debug::error(FUNCTION, "last block not in database");

        /* Check that the previous block is in the block database. */
        TAO::Ledger::BlockState statePrev;
        if(!LLD::Ledger->ReadBlock(hashPrevBlock, statePrev))
            return debug::error(FUNCTION, "prev block not in database");

        /* Read the previous block from disk. */
        nAge = statePrev.GetBlockTime() - stateLast.GetBlockTime();

        return true;
    }


    /* Get the score of the current trust block. */
    bool LegacyBlock::TrustScore(uint32_t& nScore) const
    {
        /* Genesis has an age 0. */
        if(vtx[0].IsGenesis())
        {
            nScore = 0;
            return true;
        }

        /* Version 5 - last trust block. */
        uint1024_t hashLastBlock;
        uint32_t   nSequence;
        uint32_t   nTrustScore;

        /* Extract values from coinstake vin. */
        if(!vtx[0].ExtractTrust(hashLastBlock, nSequence, nTrustScore))
            return debug::error(FUNCTION, "failed to extract values from script");

        /* Get the score from extract process. */
        nScore = nTrustScore;

        return true;
    }


    /* Get the Signature Hash of the block. Used to verify work claims. */
    uint1024_t LegacyBlock::SignatureHash() const
    {
        /* Signature hash for version 7 blocks. */
        if(nVersion >= 7)
        {
            /* Create a data stream to get the hash. */
            DataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);
            ss.reserve(256);

            /* Serialize the data to hash into a stream. */
            ss << nVersion << hashPrevBlock << hashMerkleRoot << nChannel << nHeight << nBits << nNonce << nTime << vOffsets;

            return LLC::SK1024(ss.begin(), ss.end());
        }

        /* Create a data stream to get the hash. */
        DataStream ss(SER_GETHASH, LLP::PROTOCOL_VERSION);
        ss.reserve(256);

        /* Serialize the data to hash into a stream. */
        ss << nVersion << hashPrevBlock << hashMerkleRoot << nChannel << nHeight << nBits << nNonce << uint32_t(nTime);

        return LLC::SK1024(ss.begin(), ss.end());
    }


    /* Prove that you staked a number of seconds based on weight */
    uint1024_t LegacyBlock::StakeHash() const
    {
        /* Get the trust key. */
        uint576_t keyTrust;
        vtx[0].TrustKey(keyTrust);

        return Block::StakeHash(vtx[0].IsGenesis(), keyTrust);
    }
}
