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

/* Global TAO namespace. */
namespace TAO::Ledger
{
    /* Dual-mode block creation for stateless mining */
    TritiumBlock* CreateBlockForStatelessMining(
        const uint32_t nChannel,
        const uint64_t nExtraNonce,
        const uint256_t& hashRewardAddress,
        const bool fFalconAuthenticated)
    {
        /* Mode 1: Try wallet mode first (upstream compatibility) */
        if(!fFalconAuthenticated && TAO::API::Authentication::Unlocked(TAO::Ledger::PinUnlock::MINING))
        {
            debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== BLOCK CREATION: Wallet Mode ===", ANSI_COLOR_RESET);
            debug::log(0, "   [1/6] Accessing Session::DEFAULT...");
            
            try {
                const uint256_t hashSession = uint256_t(TAO::API::Authentication::SESSION::DEFAULT);
                const auto& pCredentials = TAO::API::Authentication::Credentials(hashSession);
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "         Success", ANSI_COLOR_RESET);
                
                debug::log(0, "   [2/6] Unlocking mining credentials...");
                SecureString strPIN;
                RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING, hashSession));
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "         Success", ANSI_COLOR_RESET);
                
                debug::log(0, "   [3/6] Creating block structure...");
                TritiumBlock* pBlock = new TritiumBlock();
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "         Success", ANSI_COLOR_RESET);
                
                debug::log(0, "   [4/6] Calling CreateBlock (channel=", nChannel, 
                          ", extraNonce=", nExtraNonce, ")...");
                bool success = CreateBlock(pCredentials, strPIN, nChannel, *pBlock, 
                                          nExtraNonce, nullptr, hashRewardAddress);
                
                if(!success) {
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "         Failed: CreateBlock returned false", ANSI_COLOR_RESET);
                    delete pBlock;
                    return nullptr;
                }
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "         Success", ANSI_COLOR_RESET);
                
                debug::log(0, "   [5/6] Verifying block contents...");
                debug::log(0, "         Height: ", pBlock->nHeight);
                debug::log(0, "         Channel: ", pBlock->nChannel);
                debug::log(0, "         Transactions: ", pBlock->vtx.size());
                debug::log(0, "         Producer size: ", ::GetSerializeSize(pBlock->producer, SER_NETWORK, LLP::PROTOCOL_VERSION), " bytes");
                
                debug::log(0, "   [6/6] Block created successfully");
                debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== BLOCK CREATION: Complete (Wallet Mode) ===", ANSI_COLOR_RESET);
                return pBlock;
            }
            catch(const std::exception& e) {
                debug::error(FUNCTION, "Wallet mode failed: ", e.what());
            }
        }
        
        /* Mode 2: Stateless mode (Falcon authenticated) */
        if(fFalconAuthenticated)
        {
            debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== BLOCK CREATION: Stateless Mode (Falcon) ===", ANSI_COLOR_RESET);
            
            try {
                debug::log(0, "   [1/8] Loading best block state...");
                const BlockState tStateBest = ChainState::tStateBest.load();
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "         Height: ", tStateBest.nHeight, ANSI_COLOR_RESET);
                
                debug::log(0, "   [2/8] Creating block structure...");
                TritiumBlock* pBlock = new TritiumBlock();
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "         Success", ANSI_COLOR_RESET);
                
                debug::log(0, "   [3/8] Setting block parameters...");
                /* Get current block version */
                uint32_t nCurrent = CurrentBlockVersion();
                if(BlockVersionActive(runtime::unifiedtimestamp(), nCurrent))
                    pBlock->nVersion = nCurrent;
                else
                    pBlock->nVersion = nCurrent - 1;
                
                pBlock->hashPrevBlock = tStateBest.GetHash();
                pBlock->nChannel = nChannel;
                pBlock->nHeight = tStateBest.nHeight + 1;
                pBlock->nBits = GetNextTargetRequired(tStateBest, nChannel, false);
                pBlock->nNonce = nExtraNonce;
                pBlock->nTime = runtime::unifiedtimestamp();
                debug::log(0, "         Version: ", pBlock->nVersion);
                debug::log(0, "         Height: ", pBlock->nHeight);
                debug::log(0, "         Channel: ", pBlock->nChannel);
                debug::log(0, "         Target: 0x", std::hex, pBlock->nBits, std::dec);
                
                debug::log(0, "   [4/8] Creating producer transaction...");
                Transaction txProducer;
                if(!CreateProducerStateless(txProducer, tStateBest, pBlock->nVersion,
                                           nChannel, nExtraNonce, hashRewardAddress))
                {
                    debug::log(0, ANSI_COLOR_BRIGHT_RED, "         Failed: CreateProducerStateless returned false", ANSI_COLOR_RESET);
                    delete pBlock;
                    return nullptr;
                }
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "         Success", ANSI_COLOR_RESET);
                debug::log(0, "         Producer size: ", ::GetSerializeSize(txProducer, SER_NETWORK, LLP::PROTOCOL_VERSION), " bytes");
                
                pBlock->producer = txProducer;
                
                /* Add mempool transactions */
                debug::log(0, "   [5/8] Adding mempool transactions...");
                size_t nBefore = pBlock->vtx.size();
                AddTransactions(*pBlock);
                size_t nAfter = pBlock->vtx.size();
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "         Added: ", (nAfter - nBefore), " transactions", ANSI_COLOR_RESET);
                
                debug::log(0, "   [6/8] Building merkle tree...");
                /* Build merkle tree from transactions (empty for now) and producer */
                std::vector<uint512_t> vHashes;
                for(const auto& tx : pBlock->vtx)
                    vHashes.push_back(tx.second);
                
                /* Producer transaction is last hash in list. */
                vHashes.push_back(pBlock->producer.GetHash(true));
                
                /* Build the block's merkle root. */
                pBlock->hashMerkleRoot = pBlock->BuildMerkleTree(vHashes);
                debug::log(0, ANSI_COLOR_BRIGHT_GREEN, "         Merkle root: ", pBlock->hashMerkleRoot.SubString(), ANSI_COLOR_RESET);
                
                debug::log(0, "   [7/8] Final block stats...");
                debug::log(0, "         Total transactions: ", pBlock->vtx.size());
                debug::log(0, "         Block size: ", ::GetSerializeSize(*pBlock, SER_NETWORK, LLP::PROTOCOL_VERSION), " bytes");
                
                debug::log(0, "   [8/8] Stateless block created successfully");
                debug::log(0, ANSI_COLOR_BRIGHT_CYAN, "=== BLOCK CREATION: Complete (Stateless Mode) ===", ANSI_COLOR_RESET);
                return pBlock;
            }
            catch(const std::exception& e) {
                debug::error(FUNCTION, "Stateless mode failed: ", e.what());
                return nullptr;
            }
        }
        
        debug::error(FUNCTION, "No mining mode available");
        return nullptr;
    }

}
