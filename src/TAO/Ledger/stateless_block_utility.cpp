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

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

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
            debug::log(1, FUNCTION, "Using WALLET mode (Session::DEFAULT)");
            
            try {
                const uint256_t hashSession = uint256_t(TAO::API::Authentication::SESSION::DEFAULT);
                const auto& pCredentials = TAO::API::Authentication::Credentials(hashSession);
                
                SecureString strPIN;
                RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING, hashSession));
                
                TritiumBlock* pBlock = new TritiumBlock();
                bool success = CreateBlock(pCredentials, strPIN, nChannel, *pBlock, 
                                          nExtraNonce, nullptr, hashRewardAddress);
                
                if(!success) {
                    delete pBlock;
                    return nullptr;
                }
                
                return pBlock;
            }
            catch(const std::exception& e) {
                debug::error(FUNCTION, "Wallet mode failed: ", e.what());
            }
        }
        
        /* Mode 2: Stateless mode (Falcon authenticated) */
        if(fFalconAuthenticated)
        {
            debug::log(1, FUNCTION, "Using STATELESS mode (Falcon authenticated)");
            
            try {
                const BlockState tStateBest = ChainState::tStateBest.load();
                
                TritiumBlock* pBlock = new TritiumBlock();
                pBlock->nVersion = CurrentBlockVersion(tStateBest.nHeight + 1);
                pBlock->hashPrevBlock = tStateBest.GetHash();
                pBlock->nChannel = nChannel;
                pBlock->nHeight = tStateBest.nHeight + 1;
                pBlock->nBits = GetNextTargetRequired(tStateBest, nChannel, false);
                pBlock->nNonce = nExtraNonce;
                pBlock->nTime = runtime::unifiedtimestamp();
                
                Transaction txProducer;
                if(!CreateProducerStateless(txProducer, tStateBest, pBlock->nVersion,
                                           nChannel, nExtraNonce, hashRewardAddress))
                {
                    delete pBlock;
                    return nullptr;
                }
                
                pBlock->producer = txProducer;
                
                /* Build merkle tree from transactions (empty for now) and producer */
                std::vector<uint512_t> vHashes;
                for(const auto& tx : pBlock->vtx)
                    vHashes.push_back(tx.second);
                
                /* Producer transaction is last hash in list. */
                vHashes.push_back(pBlock->producer.GetHash(true));
                
                /* Build the block's merkle root. */
                pBlock->hashMerkleRoot = pBlock->BuildMerkleTree(vHashes);
                
                debug::log(2, FUNCTION, "Stateless block created successfully");
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
