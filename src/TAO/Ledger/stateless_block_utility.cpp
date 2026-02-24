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
#include <TAO/Ledger/include/supply.h>
#include <TAO/Ledger/include/retarget.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/include/process.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <LLP/include/version.h>
#include <LLP/include/falcon_constants.h>
#include <LLP/include/disposable_falcon.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <sstream>

/* Global TAO namespace. */
namespace TAO::Ledger
{
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
            
            /* ✅ ADD: Diagnostic logging */
            debug::log(0, FUNCTION, "=== CHAIN STATE DIAGNOSTIC ===");
            debug::log(0, FUNCTION, "  ChainState::nBestHeight: ", nChainHeight);
            debug::log(0, FUNCTION, "  statePrev.nHeight: ", statePrev.nHeight);
            debug::log(0, FUNCTION, "  statePrev.GetHash(): ", statePrev.GetHash().SubString());
            debug::log(0, FUNCTION, "  Synchronizing: ", ChainState::Synchronizing() ? "YES" : "NO");
            debug::log(0, FUNCTION, "  Template will be for height: ", statePrev.nHeight + 1);
            
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
            
            /* ✅ ADD: Validate consistency */
            if(statePrev.nHeight != nChainHeight)
            {
                debug::error(FUNCTION, "❌ Chain state inconsistency detected!");
                debug::error(FUNCTION, "   statePrev.nHeight: ", statePrev.nHeight);
                debug::error(FUNCTION, "   nBestHeight: ", nChainHeight);
                debug::error(FUNCTION, "   This indicates a race condition or chain state corruption");
                return nullptr;
            }
            
            TritiumBlock* pBlock = new TritiumBlock();
            
            /* Initialize block with proper chain context BEFORE CreateBlock()
             * This ensures CreateBlock() has the correct context to populate the producer transaction.
             * Without this, the block would have default-initialized values (zeros), causing validation failures.
             * This matches the flow in normal nodes: AddBlockData() sets these fields. */
            pBlock->hashPrevBlock = statePrev.GetHash();
            pBlock->nChannel = nChannel;
            
            /* Use UNIFIED height for pBlock->nHeight — matches NexusMiner #169/#170 contract.
             *
             * TritiumBlock::Accept() validates: statePrev.nHeight + 1 == nHeight
             * where statePrev is the block at hashPrevBlock (the unified best-chain tip).
             * nChannelHeight is computed by BlockState::SetBest() and is metadata only.
             */
            pBlock->nHeight = statePrev.nHeight + 1;

            debug::log(2, FUNCTION, "✓ Creating template for channel ", static_cast<uint32_t>(nChannel),
                       " at unified height ", pBlock->nHeight);
            
            /* Verify nChannel was set correctly */
            debug::log(2, FUNCTION, "✓ Block nChannel set to: ", pBlock->nChannel, 
                       " (", (nChannel == 1 ? "Prime" : nChannel == 2 ? "Hash" : "INVALID"), ")");
            
            if(pBlock->nChannel == 0)
            {
                debug::error(FUNCTION, "❌ CRITICAL: nChannel is 0 after assignment!");
                debug::error(FUNCTION, "   Input nChannel parameter: ", nChannel);
                debug::error(FUNCTION, "   This should never happen - investigate immediately");
                delete pBlock;
                return nullptr;
            }
            
            pBlock->nBits = GetNextTargetRequired(statePrev, nChannel, false);
            pBlock->nTime = std::max(
                statePrev.GetBlockTime() + 1, 
                runtime::unifiedtimestamp()
            );
            
            debug::log(2, FUNCTION, "Block initialized with chain state:");
            debug::log(2, FUNCTION, "  hashPrevBlock: ", pBlock->hashPrevBlock.SubString());
            debug::log(2, FUNCTION, "  nHeight (unified): ", pBlock->nHeight);
            debug::log(2, FUNCTION, "  nChannel: ", pBlock->nChannel);
            debug::log(2, FUNCTION, "  nBits: 0x", std::hex, pBlock->nBits, std::dec);
            debug::log(2, FUNCTION, "  nTime: ", pBlock->nTime);
            
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
        result.nChannelHeight = 0;  // Channel height not available from block; use BlockState after acceptance
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

        if(!block.Check())
        {
            result.reason = "block Check() failed";
            return result;
        }

        /* Stale detection uses unified chain tip shared across channels. */
        if(block.hashPrevBlock != TAO::Ledger::ChainState::hashBestChain.load())
        {
            result.reason = "submitted block is stale";
            return result;
        }

        result.valid = true;
        result.reason = "valid";
        return result;
    }
    /* Canonical acceptance entrypoint for mined Tritium blocks. */
    BlockAcceptanceResult AcceptMinedBlock(TAO::Ledger::TritiumBlock& block)
    {
        BlockAcceptanceResult result;
        result.nChannel = block.nChannel;
        result.nHeight = block.nHeight;
        result.nChannelHeight = 0;  // Channel height not available from block; use BlockState after acceptance
        result.nUnifiedHeight = block.nHeight;  // block.nHeight is unified height (NexusMiner #169)
        result.hashBlock = block.hashMerkleRoot;

        debug::log(2, FUNCTION, "Centralized acceptance for block ", block.hashMerkleRoot.SubString(),
                   " channel=", block.nChannel, " unified_height=", block.nHeight);

        /* Unlock sigchain to process mined block. */
        try
        {
            SecureString strPIN; // empty PIN expected; Authentication::Unlock fetches mining PIN for unlocked session
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING));
        }
        catch(const std::exception& e)
        {
            result.reason = e.what();
            return result;
        }

        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);
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
        result.nChannelHeight = 0;  // Channel height not available from block; use BlockState after acceptance
        result.nUnifiedHeight = block.nHeight;  // block.nHeight is unified height (NexusMiner #169)
        result.hashBlock = block.hashMerkleRoot;

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

}
