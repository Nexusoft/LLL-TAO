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
            
            /* CRITICAL: Use channel-specific height, NOT unified height
             * 
             * For multi-channel mining:
             * - Unified height = 6.5M (total across all channels)
             * - Prime channel = 2.3M blocks
             * - Hash channel = 2.1M blocks  
             * - Stake channel = 2.1M blocks
             * 
             * The miner needs the CHANNEL height to:
             * 1. Know which block they're mining (Prime block 2302585, not unified 6537246)
             * 2. Compare with GET_ROUND response to detect staleness
             * 3. Set proper hashPrevBlock for the channel
             */
            BlockState stateChannel = statePrev;
            if(GetLastState(stateChannel, nChannel))
            {
                /* Template is for the NEXT block in this channel */
                pBlock->nHeight = stateChannel.nChannelHeight + 1;
                
                debug::log(2, FUNCTION, "✓ Creating template for channel ", static_cast<uint32_t>(nChannel),
                           " at channel height ", pBlock->nHeight);
                debug::log(2, FUNCTION, "   (Unified height: ", statePrev.nHeight, " - for reference only)");
            }
            else
            {
                /* Channel doesn't exist yet - mining first block in this channel
                 * GetLastState returns false and sets stateChannel to genesis (nChannelHeight = 1)
                 * So first block in channel has nChannelHeight = 2 */
                pBlock->nHeight = stateChannel.nChannelHeight + 1;
                debug::log(2, FUNCTION, "Mining first block in channel ", static_cast<uint32_t>(nChannel),
                           " at channel height ", pBlock->nHeight);
                debug::log(2, FUNCTION, "   (Genesis has nChannelHeight = ", stateChannel.nChannelHeight, ")");
            }
            
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
            debug::log(2, FUNCTION, "  nHeight (channel): ", pBlock->nHeight, " ← This is what miner sees");
            debug::log(2, FUNCTION, "  nChannel: ", pBlock->nChannel);
            debug::log(2, FUNCTION, "  Unified height: ", statePrev.nHeight, " (reference only)");
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
            
            debug::log(2, FUNCTION, "✓ Wallet-signed block created successfully");
            debug::log(2, FUNCTION, "  Note: PoW validation deferred until miner submits nonce");
            debug::log(2, FUNCTION, "  Channel height: ", pBlock->nHeight, " ← This is what miner sees");
            debug::log(2, FUNCTION, "  Channel: ", pBlock->nChannel);
            debug::log(2, FUNCTION, "  Unified height: ", statePrev.nHeight, " (reference only)");
            debug::log(2, FUNCTION, "  Reward address: ", hashRewardAddress.SubString());
            
            return pBlock;
        }
        catch (const std::exception& e) {
            debug::error(FUNCTION, "Block creation failed: ", e.what());
            return nullptr;
        }
    }


    /* Canonical acceptance entrypoint for mined Tritium blocks. */
    SubmitResult SubmitMinedBlockForStatelessMining(TAO::Ledger::TritiumBlock& block)
    {
        SubmitResult result;
        result.nChannel = block.nChannel;
        result.nHeight = block.nHeight;
        result.hashBlock = block.GetHash();

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

        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        if(!(nStatus & TAO::Ledger::PROCESS::ACCEPTED))
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

        result.accepted = true;
        result.reason = "accepted";
        return result;
    }

}
