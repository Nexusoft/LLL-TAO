/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/stateless_block_utility.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/difficulty.h>
#include <TAO/Ledger/include/prime.h>
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/process.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <LLD/include/global.h>

#include <LLP/include/version.h>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/disposable_falcon.h>

#include <LLC/include/flkey.h>
#include <LLC/include/eckey.h>

#include <Util/include/args.h>
#include <Util/include/convert.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <sstream>

/* Global TAO namespace. */
namespace TAO::Ledger
{
    namespace
    {
        bool SequenceDiagnosticsEnabled()
        {
            return config::GetBoolArg("-nseqdiag", false);
        }

        uint64_t ReadUint64LE(const std::vector<uint8_t>& vData, const size_t nOffset)
        {
            if(nOffset + sizeof(uint64_t) > vData.size())
                return 0;

            uint64_t nValue = 0;
            for(size_t i = 0; i < sizeof(uint64_t); ++i)
                nValue |= static_cast<uint64_t>(vData[nOffset + i]) << (8 * i);

            return nValue;
        }


        bool BuildFalconWrappedSubmitBlockCandidate(
            const std::vector<uint8_t>& vPayload,
            const size_t nSigLenOffset,
            FalconWrappedSubmitBlockParseResult& result)
        {
            if(nSigLenOffset < LLP::FalconConstants::TIMESTAMP_SIZE)
                return false;

            const uint16_t nSignatureLength =
                static_cast<uint16_t>(vPayload[nSigLenOffset]) |
                (static_cast<uint16_t>(vPayload[nSigLenOffset + 1]) << 8);

            if(nSignatureLength < LLP::FalconConstants::FALCON_SIG_MIN ||
               nSignatureLength > LLP::FalconConstants::FALCON_SIG_MAX_VALIDATION)
            {
                return false;
            }

            const size_t nSignatureOffset = nSigLenOffset + LLP::FalconConstants::LENGTH_FIELD_SIZE;
            if(nSignatureOffset + nSignatureLength != vPayload.size())
                return false;

            const size_t nTimestampOffset = nSigLenOffset - LLP::FalconConstants::TIMESTAMP_SIZE;
            const size_t nBlockBytesSize = nTimestampOffset;
            if(nBlockBytesSize < LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN)
                return false;

            const std::vector<uint8_t> vBlockBytes(vPayload.begin(), vPayload.begin() + nBlockBytesSize);
            const std::vector<uint8_t> vBlockBody(
                vBlockBytes.begin(),
                vBlockBytes.begin() + LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN);

            const uint32_t nChannel = convert::bytes2uint(
                vBlockBody, LLP::FalconConstants::FULL_BLOCK_TRITIUM_CHANNEL_OFFSET);
            if(nChannel != 1 && nChannel != 2)
                return false;

            if(nChannel == 2 && nBlockBytesSize != LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN)
                return false;

            result.success = true;
            result.vBlockBytes = vBlockBytes;
            result.vBlockBody = vBlockBody;
            result.vOffsets.assign(
                vBlockBytes.begin() + LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN,
                vBlockBytes.end());
            result.vSignature.assign(vPayload.begin() + nSignatureOffset, vPayload.end());
            result.hashMerkle.SetBytes(std::vector<uint8_t>(
                vBlockBody.begin() + LLP::FalconConstants::FULL_BLOCK_MERKLE_OFFSET,
                vBlockBody.begin() + LLP::FalconConstants::FULL_BLOCK_MERKLE_OFFSET +
                    LLP::FalconConstants::MERKLE_ROOT_SIZE));
            result.nonce = ReadUint64LE(vBlockBody, LLP::FalconConstants::FULL_BLOCK_TRITIUM_NONCE_OFFSET);
            result.timestamp = ReadUint64LE(vPayload, nTimestampOffset);
            result.nSignatureLength = nSignatureLength;
            result.nChannel = nChannel;
            result.nUnifiedHeight = convert::bytes2uint(
                vBlockBody, LLP::FalconConstants::FULL_BLOCK_TRITIUM_HEIGHT_OFFSET);
            return true;
        }
    }


    /* Create wallet-signed block for stateless mining */
    TritiumBlock* CreateBlockForStatelessMining(
        const uint32_t nChannel,
        const uint64_t nExtraNonce,
        const uint256_t& hashRewardAddress)
    {
        /* Early exit if shutdown is in progress */
        if(config::fShutdown.load())
        {
            debug::log(1, FUNCTION, "Shutdown in progress; skipping block creation");
            return nullptr;
        }
        
        /* Validate input nChannel parameter (defense in depth) */
        if(nChannel == 0)
        {
            debug::error(FUNCTION, "❌ Invalid input: nChannel is 0");
            debug::error(FUNCTION, "   Caller must provide valid channel (1=Prime, 2=Hash)");
            return nullptr;
        }
        
        if(nChannel != 1 && nChannel != 2)
        {
            debug::error(FUNCTION, "❌ Invalid input: nChannel = ", nChannel);
            debug::error(FUNCTION, "   Valid channels: 1 (Prime), 2 (Hash)");
            return nullptr;
        }
        
        /* All blocks MUST be wallet-signed per Nexus consensus */
        if (!TAO::API::Authentication::Unlocked(TAO::Ledger::PinUnlock::MINING))
        {
            debug::error(FUNCTION, "Mining not unlocked - use -unlock=mining or -autologin=username:password");
            debug::error(FUNCTION, "CRITICAL: Nexus consensus requires wallet-signed blocks");
            debug::error(FUNCTION, "Falcon authentication is for miner sessions, NOT block signing");
            return nullptr;
        }

        debug::log(1, FUNCTION, "Creating wallet-signed block (Nexus consensus requirement)");
        
        try {
            const uint256_t hashSession = uint256_t(TAO::API::Authentication::SESSION::DEFAULT);
            const auto& pCredentials = TAO::API::Authentication::Credentials(hashSession);
            
            SecureString strPIN;
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING, hashSession));
            
            /* Get current chain state (SAME as normal node does) */
            const BlockState statePrev = ChainState::tStateBest.load();
            const uint32_t nChainHeight = ChainState::nBestHeight.load();
            
            /* Diagnostic logging */
            debug::log(2, FUNCTION, "=== CHAIN STATE DIAGNOSTIC ===");
            debug::log(2, FUNCTION, "  ChainState::nBestHeight: ", nChainHeight);
            debug::log(2, FUNCTION, "  statePrev.nHeight: ", statePrev.nHeight);
            debug::log(2, FUNCTION, "  statePrev.GetHash(): ", statePrev.GetHash().SubString());
            debug::log(2, FUNCTION, "  Synchronizing: ", ChainState::Synchronizing() ? "YES" : "NO");
            debug::log(2, FUNCTION, "  Template will be for height: ", statePrev.nHeight + 1);
            
            /* Verify chain state is valid before proceeding */
            if(!statePrev || statePrev.GetHash() == 0)
            {
                debug::error(FUNCTION, "Chain state not initialized - cannot create block template");
                debug::error(FUNCTION, "  Node may still be starting up or synchronizing");
                return nullptr;
            }
            
            /* Don't create blocks while synchronizing */
            if(ChainState::Synchronizing())
            {
                debug::error(FUNCTION, "Cannot create block templates while synchronizing");
                return nullptr;
            }
            
            TritiumBlock* pBlock = new TritiumBlock();
            
            // CreateBlock() handles wallet signing per consensus requirements
            bool success = CreateBlock(
                pCredentials,
                strPIN,
                nChannel,
                *pBlock,
                nExtraNonce,
                nullptr,           // No coinbase recipients
                hashRewardAddress  // Route rewards to miner's address
            );
            
            if (!success) {
                delete pBlock;
                debug::error(FUNCTION, "CreateBlock failed");
                return nullptr;
            }
            
            /* DO NOT call Check() here - the block hasn't been mined yet.
             * Check() validates PoW which requires a valid nonce from the miner.
             * Validation happens in validate_block() AFTER miner submits solution. */
            
            /* Basic sanity check only - verify CreateBlock() produced valid output */
            if(pBlock->hashMerkleRoot == 0)
            {
                debug::error(FUNCTION, "CreateBlock() produced invalid merkle root");
                delete pBlock;
                return nullptr;
            }
            
            /* Log block creation result */
            debug::log(2, FUNCTION, "CreateBlock: channel ", pBlock->nChannel, 
                       " unified height ", pBlock->nHeight);
            debug::log(2, FUNCTION, "  Note: PoW validation deferred until miner submits nonce");
            debug::log(2, FUNCTION, "  Reward address: ", hashRewardAddress.SubString());

            return pBlock;
        }
        catch (const std::exception& e) {
            debug::error(FUNCTION, "Block creation failed: ", e.what());
            return nullptr;
        }
    }


    /* Canonical validation entrypoint for mined Tritium blocks. */
    BlockValidationResult ValidateMinedBlock(const TAO::Ledger::TritiumBlock& block)
    {
        BlockValidationResult result;
        result.nChannel = block.nChannel;
        result.nHeight = block.nHeight;
        result.nUnifiedHeight = block.nHeight;  // block.nHeight is unified height (NexusMiner #169)
        result.hashBlock = block.hashMerkleRoot;

        debug::log(2, FUNCTION, "Centralized validation for block ", block.hashMerkleRoot.SubString(),
                   " channel=", block.nChannel, " unified_height=", block.nHeight);

        if(config::fShutdown.load())
        {
            result.reason = "shutdown in progress";
            return result;
        }

        if(block.IsNull())
        {
            result.reason = "block is null";
            return result;
        }

        if(block.hashMerkleRoot == 0)
        {
            result.reason = "block merkle root is null";
            return result;
        }

        if(block.nChannel != 1 && block.nChannel != 2)
        {
            result.reason = "invalid block channel";
            return result;
        }

        if(block.nHeight == 0)
        {
            result.reason = "invalid block height";
            return result;
        }

        /* Stale detection uses unified chain tip shared across channels.
         * Perform this cheap check before the expensive block.Check() / VerifyWork()
         * so stale blocks are rejected immediately without running full PoW verification. */
        if(block.hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load())
        {
            result.reason = "submitted block is stale";
            return result;
        }

        if(!block.Check())
        {
            result.reason = "block Check() failed";
            return result;
        }

        result.valid = true;
        result.reason = "valid";
        return result;
    }

    /* Pre-validation producer refresh.  Called after sign_block() and BEFORE
     * ValidateMinedBlock() so that TritiumBlock::Check() (called inside
     * ValidateMinedBlock) sees a producer that is consistent with both the
     * block's vtx transactions and the on-disk sigchain state. */
    bool RefreshProducerIfStale(TAO::Ledger::TritiumBlock& block)
    {
        const bool fSeqDiag = SequenceDiagnosticsEnabled();

        /* Only PoW channels (Prime=1, Hash=2) have a sigchain producer that can
         * go stale.  Stake (0) and Private (3) use different producer semantics. */
        if(block.nChannel != 1 && block.nChannel != 2)
            return true;

        /* ── Step 1: find the highest-sequence vtx tx for the producer's genesis ──
         *
         * block.vtx transactions will be connected by BlockState::Connect() BEFORE
         * the producer.  Each one calls WriteLast(genesis, hash).  So by the time
         * Connect() reaches the producer, the on-disk last will be whatever is the
         * last same-genesis tx in vtx — not what was on disk when this function runs.
         *
         * We must therefore make the producer follow the last vtx tx for its genesis,
         * not the current disk last. */
        uint512_t hashVtxLast    = 0;
        uint32_t  nVtxLastSeq    = 0;
        bool      fHasVtxSameGen = false;
        bool      fVtxReadFailure = false;

        for(const auto& txpair : block.vtx)
        {
            /* We only store TRITIUM transactions in vtx for sigchain ordering. */
            if(txpair.first != TAO::Ledger::TRANSACTION::TRITIUM)
                continue;

            TAO::Ledger::Transaction txVtx;
            if(!LLD::Ledger->ReadTx(txpair.second, txVtx, TAO::Ledger::FLAGS::MEMPOOL))
            {
                fVtxReadFailure = true;
                debug::log(2, FUNCTION, "vtx ReadTx failed for ", txpair.second.SubString(),
                           " — skipping for producer staleness check");
                continue;
            }

            if(txVtx.hashGenesis != block.producer.hashGenesis)
                continue;

            /* Track the highest sequence. vtx is typically in ascending order, but
             * we use max() defensively to handle any reordering or edge cases. */
            if(!fHasVtxSameGen || txVtx.nSequence > nVtxLastSeq)
            {
                hashVtxLast    = txpair.second;
                nVtxLastSeq    = txVtx.nSequence;
                fHasVtxSameGen = true;
            }
        }

        /* ── Step 2: determine the true predecessor for the producer ── */
        uint512_t hashTrueLast = 0;
        uint32_t  nTrueLastSeq = 0;
        const char* strSeqSource = "none";
        bool fFallbackPath = false;

        if(fHasVtxSameGen)
        {
            /* vtx transactions win — they will be on-disk by connect time. */
            hashTrueLast = hashVtxLast;
            nTrueLastSeq = nVtxLastSeq;
            strSeqSource = "vtx";
        }
        else
        {
            /* No vtx tx for this genesis: use on-disk last (scenario B). */
            if(!LLD::Ledger->ReadLast(block.producer.hashGenesis, hashTrueLast))
                return true; /* genesis not yet on disk (first block) — no staleness */

            strSeqSource = "ledger_last";
            fFallbackPath = true;

            TAO::Ledger::Transaction txLast;
            if(LLD::Ledger->ReadTx(hashTrueLast, txLast, TAO::Ledger::FLAGS::MEMPOOL))
                nTrueLastSeq = txLast.nSequence;
        }

        if(fSeqDiag)
        {
            debug::log(0, FUNCTION,
                "[NSEQ_DIAG][RefreshProducerIfStale][SOURCE]"
                " genesis=", block.producer.hashGenesis.SubString(),
                " source=", strSeqSource,
                " fallback=", (fFallbackPath ? "yes" : "no"),
                " vtx_read_failure=", (fVtxReadFailure ? "yes" : "no"),
                " hashTrueLast=", hashTrueLast.SubString(),
                " nTrueLastSeq=", nTrueLastSeq,
                " current.hashPrevTx=", block.producer.hashPrevTx.SubString(),
                " current.nSequence=", block.producer.nSequence);
        }

        /* ── Step 3: check if refresh is actually needed ── */
        if(hashTrueLast == 0 || hashTrueLast == block.producer.hashPrevTx)
            return true; /* already consistent — nothing to do */

        /* ── Step 4: perform the refresh ── */
        const uint512_t hashOldProducer = block.producer.GetHash(true);
        const uint1024_t hashOldMerkle = block.hashMerkleRoot;
        const std::vector<uint8_t> vOldBlockSig = block.vchBlockSig;
        const bool fOldHadBlockSig = !vOldBlockSig.empty();

        debug::log(0, FUNCTION,
            "Producer pre-validation refresh:"
            " genesis=",          block.producer.hashGenesis.SubString(),
            " old.hashPrevTx=",   block.producer.hashPrevTx.SubString(),
            " old.nSequence=",    block.producer.nSequence,
            " trueLast=",         hashTrueLast.SubString(),
            " new.nSequence=",    nTrueLastSeq + 1,
            " vtxContrib=",       fHasVtxSameGen);

        /* Unlock the sigchain to obtain credentials for re-signing. */
        SecureString strPIN;
        try
        {
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING));
        }
        catch(const std::exception& e)
        {
            debug::error(FUNCTION, "producer refresh: unlock failed: ", e.what());
            strPIN.clear();
            return false;
        }

        const auto& pCredentials =
            TAO::API::Authentication::Credentials(uint256_t(TAO::API::Authentication::SESSION::DEFAULT));
        if(!pCredentials)
        {
            debug::error(FUNCTION, "producer refresh: null credentials");
            strPIN.clear();
            return false;
        }

        /* CreateTransaction gives us fresh key-chain metadata.  We override
         * nSequence and hashPrevTx with the block-correct values below. */
        TAO::Ledger::Transaction txFresh;
        if(!CreateTransaction(pCredentials, strPIN, txFresh))
        {
            debug::error(FUNCTION, "producer refresh: CreateTransaction failed");
            strPIN.clear();
            return false;
        }

        /* ── CRITICAL: override with block-correct sequence/prev, not disk state ── */
        block.producer.nSequence   = nTrueLastSeq + 1;
        block.producer.hashPrevTx  = hashTrueLast;

        /* Copy remaining sigchain metadata from fresh transaction. */
        block.producer.nKeyType    = txFresh.nKeyType;
        block.producer.nNextType   = txFresh.nNextType;
        block.producer.hashRecovery= txFresh.hashRecovery;
        block.producer.hashNext    = txFresh.hashNext;
        block.producer.nTimestamp  = txFresh.nTimestamp;
        block.producer.nVersion    = txFresh.nVersion;

        /* Re-sign producer with corrected sequence. */
        block.producer.Sign(pCredentials->Generate(block.producer.nSequence, strPIN));
        strPIN.clear();

        /* Rebuild merkle root (producer hash changed). */
        std::vector<uint512_t> vHashes;
        for(const auto& tx : block.vtx)
            vHashes.push_back(tx.second);
        vHashes.push_back(block.producer.GetHash(true));
        block.hashMerkleRoot = block.BuildMerkleTree(vHashes);

        /* Re-sign block (vchBlockSig covers hashMerkleRoot). */
        block.vchBlockSig.clear();
        if(!FinalizeWalletSignatureForSolvedBlock(block))
        {
            debug::error(FUNCTION, "producer refresh: block re-sign failed");
            return false;
        }

        debug::log(0, FUNCTION,
            "Producer refresh SUCCESS:"
            " nSequence=",  block.producer.nSequence,
            " hashPrevTx=", block.producer.hashPrevTx.SubString(),
            " merkle=",     block.hashMerkleRoot.SubString());

        if(fSeqDiag)
        {
            const uint512_t hashNewProducer = block.producer.GetHash(true);
            debug::log(0, FUNCTION,
                "[NSEQ_DIAG][RefreshProducerIfStale][MUTATION]"
                " producer_mutated=", (hashOldProducer != hashNewProducer ? "yes" : "no"),
                " producer_resigned=", (hashOldProducer != hashNewProducer ? "yes" : "no"),
                " old.producer=", hashOldProducer.SubString(),
                " new.producer=", hashNewProducer.SubString(),
                " merkle_changed=", (hashOldMerkle != block.hashMerkleRoot ? "yes" : "no"),
                " old.merkle=", hashOldMerkle.SubString(),
                " new.merkle=", block.hashMerkleRoot.SubString(),
                " blocksig_was_present=", (fOldHadBlockSig ? "yes" : "no"),
                " blocksig_changed=", (vOldBlockSig != block.vchBlockSig ? "yes" : "no"),
                " old.blocksig.size=", vOldBlockSig.size(),
                " new.blocksig.size=", block.vchBlockSig.size());
        }

        return true;
    }

    /* Canonical acceptance entrypoint for mined Tritium blocks. */
    BlockAcceptanceResult AcceptMinedBlock(TAO::Ledger::TritiumBlock& block)
    {
        BlockAcceptanceResult result;
        result.nChannel = block.nChannel;
        result.nHeight = block.nHeight;
        result.nUnifiedHeight = block.nHeight;  // block.nHeight is unified height (NexusMiner #169)
        result.hashBlock = block.hashMerkleRoot;

        debug::log(2, FUNCTION, "Centralized acceptance for block ", block.hashMerkleRoot.SubString(),
                   " channel=", block.nChannel, " unified_height=", block.nHeight);

        /* Unlock sigchain to process mined block. */
        SecureString strPIN;
        try
        {
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING));
        }
        catch(const std::exception& e)
        {
            result.reason = e.what();
            return result;
        }
        strPIN.clear();

        uint8_t nStatus = 0;
        /* Pass fSkipCheck=true because ValidateMinedBlock() already ran block.Check()
         * (including the expensive VerifyWork() / PrimeCheck() consensus validation).
         * Skipping the redundant Check() inside Process() eliminates the double
         * PoW verification that caused the triple-PrimeCheck stall in production. */
        TAO::Ledger::Process(block, nStatus, nullptr, true);
        result.status = nStatus;
        result.accepted = (nStatus & TAO::Ledger::PROCESS::ACCEPTED);

        if(!result.accepted)
        {
            if(nStatus & TAO::Ledger::PROCESS::ORPHAN)
                result.reason = "block is orphan";
            else if(nStatus & TAO::Ledger::PROCESS::DUPLICATE)
                result.reason = "duplicate block";
            else if(nStatus & TAO::Ledger::PROCESS::INCOMPLETE)
                result.reason = "block incomplete";
            else if(nStatus & TAO::Ledger::PROCESS::REJECTED)
                result.reason = "block rejected";
            else if(nStatus & TAO::Ledger::PROCESS::IGNORED)
                result.reason = "block ignored";
            else
                result.reason = "block not accepted";
            return result;
        }

        result.reason = "accepted";
        return result;
    }


    /* Canonical acceptance entrypoint for mined Tritium blocks. */
    SubmitResult SubmitMinedBlockForStatelessMining(TAO::Ledger::TritiumBlock& block)
    {
        SubmitResult result;
        result.nChannel = block.nChannel;
        result.nHeight = block.nHeight;
        result.nUnifiedHeight = block.nHeight;  // block.nHeight is unified height (NexusMiner #169)
        result.hashBlock = block.hashMerkleRoot;

        debug::log(0, FUNCTION, "[BLOCK SUBMIT] nHeight=", block.nHeight, " (unified)",
                   " channel=", block.nChannel,
                   " hashPrevBlock=", block.hashPrevBlock.SubString());

        const BlockValidationResult validationResult = ValidateMinedBlock(block);
        if(!validationResult.valid)
        {
            result.reason = validationResult.reason;
            return result;
        }

        const BlockAcceptanceResult acceptanceResult = AcceptMinedBlock(block);
        if(!acceptanceResult.accepted)
        {
            result.reason = acceptanceResult.reason;
            return result;
        }

        result.accepted = true;
        result.reason = acceptanceResult.reason;
        return result;
    }


    /* Parse stateless miner work submission payloads. */
    ParseResult ParseStatelessWorkSubmission(const std::vector<uint8_t>& vData)
    {
        ParseResult result;

        if(vData.size() < LLP::FalconConstants::MERKLE_ROOT_SIZE + LLP::FalconConstants::NONCE_SIZE)
        {
            result.reason = "submission payload too small";
            return result;
        }

        if(vData.size() >= LLP::FalconConstants::SUBMIT_BLOCK_WRAPPER_MIN)
        {
            LLP::DisposableFalcon::SignedWorkSubmission submission;
            if(submission.Deserialize(vData) && submission.IsValid())
            {
                result.hashMerkle = submission.hashMerkleRoot;
                result.nonce = submission.nNonce;
                result.timestamp = submission.nTimestamp;
                result.success = true;

                /* Opportunistically extract nUnifiedHeight from the block body when the
                 * payload is large enough to contain a full Tritium block header (>= 204 bytes
                 * covers offsets [0-203]).  We validate nChannel (must be 1 or 2) at offset
                 * 196 to discriminate full-block-body payloads from compact-format submissions
                 * that happen to be >= 204 bytes.  Channel 0 (Proof-of-Stake) is intentionally
                 * excluded because stateless mining only supports Prime (1) and Hash (2).
                 * nHeight lives at offset 200 (big-endian uint32_t). */
                if(vData.size() >= 204)
                {
                    const uint32_t nCh = convert::bytes2uint(vData, 196);
                    if(nCh == 1 || nCh == 2)
                    {
                        result.nUnifiedHeight = convert::bytes2uint(vData, 200);
                    }
                }

                return result;
            }
        }

        result.hashMerkle.SetBytes(std::vector<uint8_t>(
            vData.begin(),
            vData.begin() + LLP::FalconConstants::MERKLE_ROOT_SIZE));

        /* Nonce is little-endian per Falcon stateless protocol. */
        uint64_t nonce = 0;
        for(size_t i = 0; i < LLP::FalconConstants::NONCE_SIZE; ++i)
        {
            nonce |= static_cast<uint64_t>(vData[LLP::FalconConstants::MERKLE_ROOT_SIZE + i]) << (8 * i);
        }
        result.nonce = nonce;

        result.success = true;
        return result;
    }


    FalconWrappedSubmitBlockParseResult ParseFalconWrappedSubmitBlock(const std::vector<uint8_t>& vPayload)
    {
        FalconWrappedSubmitBlockParseResult result;

        const size_t nMinimumPayloadSize =
            LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN +
            LLP::FalconConstants::TIMESTAMP_SIZE +
            LLP::FalconConstants::LENGTH_FIELD_SIZE +
            LLP::FalconConstants::FALCON_SIG_MIN;

        if(vPayload.size() < nMinimumPayloadSize)
        {
            result.reason = "payload shorter than minimum Falcon full-block wrapper";
            return result;
        }

        const size_t nMinSigLenOffset =
            LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN + LLP::FalconConstants::TIMESTAMP_SIZE;
        const size_t nMaxSigLenOffset = vPayload.size() - LLP::FalconConstants::LENGTH_FIELD_SIZE;

        /* Scan for the Falcon trailer position.  The tail-anchor constraint in
         * BuildFalconWrappedSubmitBlockCandidate() — requiring that
         *   nSignatureOffset + nSignatureLength == vPayload.size()
         * — means at most ONE nSigLenOffset value can produce a structurally valid
         * candidate for a given payload.  BuildFalconWrappedSubmitBlockCandidate()
         * therefore returns true for at most one candidate per call to ParseFalconWrappedSubmitBlock(). */
        for(size_t nSigLenOffset = nMinSigLenOffset; nSigLenOffset <= nMaxSigLenOffset; ++nSigLenOffset)
        {
            FalconWrappedSubmitBlockParseResult candidate;
            if(BuildFalconWrappedSubmitBlockCandidate(vPayload, nSigLenOffset, candidate))
                return candidate;
        }

        result.reason = "unable to locate Falcon trailer in full-block payload";
        return result;
    }


    bool VerifyFalconWrappedSubmitBlock(
        const std::vector<uint8_t>& vPayload,
        const std::vector<uint8_t>& vPubKey,
        FalconWrappedSubmitBlockParseResult& result)
    {
        result = FalconWrappedSubmitBlockParseResult();

        LLC::FLKey verifyKey;
        if(!verifyKey.SetPubKey(vPubKey))
            return false;

        const size_t nMinSigLenOffset =
            LLP::FalconConstants::FULL_BLOCK_TRITIUM_MIN + LLP::FalconConstants::TIMESTAMP_SIZE;
        if(vPayload.size() < nMinSigLenOffset + LLP::FalconConstants::LENGTH_FIELD_SIZE + LLP::FalconConstants::FALCON_SIG_MIN)
            return false;

        const size_t nMaxSigLenOffset = vPayload.size() - LLP::FalconConstants::LENGTH_FIELD_SIZE;
        /* Scan for the Falcon trailer position.  The tail-anchor constraint in
         * BuildFalconWrappedSubmitBlockCandidate() — requiring that
         *   nSignatureOffset + nSignatureLength == vPayload.size()
         * — means at most ONE nSigLenOffset value can produce a structurally valid
         * candidate for a given payload.  FLKey::Verify() is therefore called at
         * most once per call to VerifyFalconWrappedSubmitBlock(). */
        for(size_t nSigLenOffset = nMinSigLenOffset; nSigLenOffset <= nMaxSigLenOffset; ++nSigLenOffset)
        {
            FalconWrappedSubmitBlockParseResult candidate;
            if(!BuildFalconWrappedSubmitBlockCandidate(vPayload, nSigLenOffset, candidate))
                continue;

            std::vector<uint8_t> vMessage = candidate.vBlockBytes;
            for(size_t i = 0; i < LLP::FalconConstants::TIMESTAMP_SIZE; ++i)
                vMessage.push_back(static_cast<uint8_t>((candidate.timestamp >> (8 * i)) & 0xff));

            if(verifyKey.Verify(vMessage, candidate.vSignature))
            {
                result = std::move(candidate);
                return true;
            }
        }

        return false;
    }


    /* Build a canonical solved Prime candidate from the immutable stored template. */
    TritiumBlock BuildSolvedPrimeCandidateFromTemplate(
        const TritiumBlock& tmpl,
        const uint64_t nNonce,
        const std::vector<uint8_t>& vOffsets)
    {
        /* Copy all consensus-critical fields from the original template.
         * This preserves: nVersion, hashPrevBlock, hashMerkleRoot, nChannel,
         * nHeight, nBits, nTime, producer, ssSystem, vtx, and hashMerkleRoot.
         *
         * nTime is deliberately preserved from the template rather than refreshed:
         * - For Prime: ProofHash = SK1024(nVersion..nBits) does NOT include nTime,
         *   so the miner's solved proof is independent of nTime.
         * - For Hash:  ProofHash = SK1024(nVersion..nNonce) also excludes nTime.
         * Preserving nTime avoids mutating template anchor fields after issuance.
         * Callers that require a fresh timestamp must call UpdateTime() separately. */
        TritiumBlock solved = tmpl;

        /* Apply the miner-submitted nonce. */
        solved.nNonce = nNonce;

        /* Apply miner-submitted Prime offsets for the Prime channel.
         * Clear offsets for all other channels (consensus invariant). */
        if(solved.nChannel == CHANNEL::PRIME)
            solved.vOffsets = vOffsets;
        else
            solved.vOffsets.clear();

        /* Clear the block signature.  SignatureHash() covers nNonce and vOffsets,
         * so any signature produced for the template (nNonce=1, vOffsets=empty)
         * is no longer valid.  Caller must invoke FinalizeWalletSignatureForSolvedBlock()
         * before submitting to ValidateMinedBlock() / AcceptMinedBlock(). */
        solved.vchBlockSig.clear();

        debug::log(2, FUNCTION, "Built solved candidate from template: channel=", solved.nChannel,
                   " height=", solved.nHeight, " nNonce=0x", std::hex, nNonce, std::dec,
                   " vOffsets.size()=", solved.vOffsets.size());

        return solved;
    }


    /* Build a canonical solved Hash (channel 2) candidate from the immutable stored template. */
    TritiumBlock BuildSolvedHashCandidateFromTemplate(
        const TritiumBlock& tmpl,
        const uint64_t nNonce)
    {
        /* Copy all consensus-critical fields from the original template.
         * This preserves: nVersion, hashPrevBlock, hashMerkleRoot, nChannel,
         * nHeight, nBits, nTime, producer, ssSystem, vtx.
         *
         * nTime is deliberately preserved from the template rather than refreshed:
         * - For Hash (channel 2): ProofHash = SK1024(nVersion..nNonce) does NOT
         *   include nTime.  The miner's solved proof is independent of nTime, so
         *   preserving nTime avoids mutating anchor fields after template issuance
         *   without any proof-correctness benefit.
         * Callers that require a fresh timestamp for network propagation may call
         * UpdateTime() on the returned candidate separately. */
        TritiumBlock solved = tmpl;

        /* Apply the miner-submitted nonce. */
        solved.nNonce = nNonce;

        /* Hash channel invariant: vOffsets must always be empty.
         * Clear unconditionally even if the template carried residual Prime offset
         * bytes from a prior channel switch or serialisation artefact. */
        solved.vOffsets.clear();

        /* Clear the block signature.  SignatureHash() covers nNonce; any prior
         * signature produced for the template (nNonce=1) is no longer valid after
         * the miner's nonce is applied.  Caller must invoke
         * FinalizeWalletSignatureForSolvedBlock() before submitting to
         * ValidateMinedBlock() / AcceptMinedBlock(). */
        solved.vchBlockSig.clear();

        debug::log(2, FUNCTION, "Built solved Hash candidate from template: channel=", solved.nChannel,
                   " height=", solved.nHeight, " nNonce=0x", std::hex, nNonce, std::dec);

        return solved;
    }


    /* Structurally validate miner-submitted Prime vOffsets without the broken
     * GetOffsets(GetPrime()) equivalence check. */
    bool VerifySubmittedPrimeOffsets(
        const TritiumBlock& solvedBlock,
        const std::vector<uint8_t>& vOffsets)
    {
        /* Prime blocks must carry non-empty offsets (enforced by Check()). */
        if(vOffsets.empty())
            return debug::error(FUNCTION, "Prime block requires non-empty vOffsets");

        /* Minimum structure: at least 1 chain-offset byte + 4 fractional bytes. */
        if(vOffsets.size() < 5)
            return debug::error(FUNCTION, "vOffsets too short: ", vOffsets.size(),
                                " bytes (minimum 5: ≥1 chain offset + 4 fractional)");

        /* Chain-offset bytes are all bytes except the last 4 (fractional difficulty).
         * Each chain-offset encodes the gap to the next prime in the Cunningham chain;
         * the maximum valid gap is 12 (hardcoded in GetOffsets / GetPrimeDifficulty). */
        const size_t nChainOffsets = vOffsets.size() - 4;
        for(size_t i = 0; i < nChainOffsets; ++i)
        {
            if(vOffsets[i] > 12)
                return debug::error(FUNCTION, "invalid Prime offset[", i, "]=",
                                    static_cast<int>(vOffsets[i]),
                                    " (maximum chain gap is 12)");
        }

        /* NOTE: We intentionally do NOT call GetOffsets(GetPrime()) and compare
         * the result against the miner-submitted vOffsets.  That approach was
         * broken: GetOffsets() returns an empty vector whenever PrimeCheck() fails
         * on the raw GetPrime() value, producing false rejections for valid chains
         * where the node cannot re-derive the starting prime independently.
         *
         * The authoritative proof-of-work validation is performed by VerifyWork()
         * (called from TritiumBlock::Check()), which evaluates
         *   GetPrimeBits(GetPrime(), vOffsets, !Synchronizing()) >= nBits
         * That gate remains the canonical acceptance criterion. */

        debug::log(2, FUNCTION, "Prime vOffsets structurally valid: ",
                   vOffsets.size(), " bytes, ", nChainOffsets, " chain offset(s)");
        return true;
    }


    /* Generate the canonical block signature for a solved TritiumBlock. */
    bool FinalizeWalletSignatureForSolvedBlock(TritiumBlock& block)
    {
        /* Unlock the mining sigchain to obtain the signing credentials.
         * Authentication::Unlock fetches the mining PIN for the unlocked session;
         * strPIN is populated by the call.
         * RECURSIVE is used here because this function may be called from within
         * the already-held authentication lock; the recursive variant allows
         * re-entry without deadlock. */
        SecureString strPIN;
        try
        {
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING));
        }
        catch(const std::exception& e)
        {
            return debug::error(FUNCTION, "Unable to unlock mining credentials: ", e.what());
        }

        /* Retrieve the default session credentials. */
        const auto& pCredentials =
            TAO::API::Authentication::Credentials(uint256_t(TAO::API::Authentication::SESSION::DEFAULT));

        if(!pCredentials)
            return debug::error(FUNCTION, "Null credentials — mining session not active");

        /* Derive the signing key for the producer's sequence position. */
        const std::vector<uint8_t> vBytes =
            pCredentials->Generate(block.producer.nSequence, strPIN).GetBytes();
        const LLC::CSecret vchSecret(vBytes.begin(), vBytes.end());

        /* Sign the block using the key type recorded in the producer transaction. */
        switch(block.producer.nKeyType)
        {
            case TAO::Ledger::SIGNATURE::FALCON:
            {
                LLC::FLKey key;
                if(!key.SetSecret(vchSecret))
                    return debug::error(FUNCTION, "FLKey::SetSecret failed for block ",
                                        block.hashMerkleRoot.SubString());

                if(!block.GenerateSignature(key))
                    return debug::error(FUNCTION, "GenerateSignature (Falcon) failed for block ",
                                        block.hashMerkleRoot.SubString());

                break;
            }

            case TAO::Ledger::SIGNATURE::BRAINPOOL:
            {
                LLC::ECKey key = LLC::ECKey(LLC::BRAINPOOL_P512_T1, 64);
                if(!key.SetSecret(vchSecret, true))
                    return debug::error(FUNCTION, "ECKey::SetSecret failed for block ",
                                        block.hashMerkleRoot.SubString());

                if(!block.GenerateSignature(key))
                    return debug::error(FUNCTION, "GenerateSignature (Brainpool) failed for block ",
                                        block.hashMerkleRoot.SubString());

                break;
            }

            default:
                return debug::error(FUNCTION, "Unknown producer key type: ",
                                    static_cast<int>(block.producer.nKeyType));
        }

        debug::log(2, FUNCTION, "Wallet signature generated for block ",
                   block.hashMerkleRoot.SubString(),
                   " channel=", block.nChannel,
                   " height=", block.nHeight);
        return true;
    }

}
