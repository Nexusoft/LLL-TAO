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

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <LLP/include/version.h>

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
            
            TritiumBlock* pBlock = new TritiumBlock();
            
            /* Initialize block with proper chain context BEFORE CreateBlock()
             * This ensures CreateBlock() has the correct context to populate the producer transaction.
             * Without this, the block would have default-initialized values (zeros), causing validation failures.
             * This matches the flow in normal nodes: AddBlockData() sets these fields. */
            pBlock->hashPrevBlock = statePrev.GetHash();
            pBlock->nHeight = statePrev.nHeight + 1;
            pBlock->nChannel = nChannel;
            pBlock->nBits = GetNextTargetRequired(statePrev, nChannel, false);
            pBlock->nTime = static_cast<uint32_t>(std::max(
                statePrev.GetBlockTime() + 1, 
                runtime::unifiedtimestamp()
            ));
            
            debug::log(2, FUNCTION, "Block initialized with chain state:");
            debug::log(2, FUNCTION, "  hashPrevBlock: ", pBlock->hashPrevBlock.SubString());
            debug::log(2, FUNCTION, "  nHeight: ", pBlock->nHeight);
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
            
            /* Validate block using TritiumBlock::Check()
             * This is the SAME validation normal nodes use before accepting blocks.
             * It checks: merkle root, timestamps, proof-of-work, and other consensus rules. */
            if (!pBlock->Check())
            {
                debug::error(FUNCTION, "Block failed TritiumBlock::Check() validation");
                debug::error(FUNCTION, "  Height: ", pBlock->nHeight);
                debug::error(FUNCTION, "  Channel: ", pBlock->nChannel);
                debug::error(FUNCTION, "  nBits: 0x", std::hex, pBlock->nBits, std::dec);
                debug::error(FUNCTION, "  hashMerkleRoot: ", pBlock->hashMerkleRoot.SubString());
                delete pBlock;
                return nullptr;
            }
            
            debug::log(2, FUNCTION, "✓ Wallet-signed block created successfully");
            debug::log(2, FUNCTION, "✓ Block passed TritiumBlock::Check() validation");
            debug::log(2, FUNCTION, "  Height: ", pBlock->nHeight);
            debug::log(2, FUNCTION, "  Channel: ", pBlock->nChannel);
            debug::log(2, FUNCTION, "  Reward address: ", hashRewardAddress.SubString());
            
            return pBlock;
        }
        catch (const std::exception& e) {
            debug::error(FUNCTION, "Block creation failed: ", e.what());
            return nullptr;
        }
    }

}
