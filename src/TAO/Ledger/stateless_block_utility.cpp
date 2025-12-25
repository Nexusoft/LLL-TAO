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

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/config.h>
#include <Util/include/mutex.h>

/* Global TAO namespace. */
namespace TAO::Ledger
{
    /* Helper to get default session ID */
    static const uint256_t GetDefaultSessionId()
    {
        return uint256_t(TAO::API::Authentication::SESSION::DEFAULT);
    }


    /* Detects which mining mode is available based on node state. */
    MiningMode DetectMiningMode()
    {
        /* Check for Mode 2 first (Interface Session - easiest/fastest) */
        try
        {
            /* Try to get the default session credentials */
            const uint256_t hashSession = GetDefaultSessionId();
            const auto& pCredentials = 
                TAO::API::Authentication::Credentials(hashSession);

            /* Try to unlock for mining - will throw if it fails */
            SecureString strPIN;
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING, hashSession));

            /* If we got here, both credentials exist and unlock succeeded */
            debug::log(1, FUNCTION, "Mining Mode: INTERFACE_SESSION");
            debug::log(1, FUNCTION, "  - Node has credentials (SESSION::DEFAULT available)");
            debug::log(1, FUNCTION, "  - Node will sign producers on behalf of miners");
            debug::log(1, FUNCTION, "  - Rewards routed via hashDynamicGenesis parameter");
            return MiningMode::INTERFACE_SESSION;
        }
        catch(const std::exception& e)
        {
            /* No session available - that's fine, we'll use Mode 1 */
            debug::log(2, FUNCTION, "SESSION::DEFAULT not available: ", e.what());
        }
        catch(...)
        {
            /* Any other error - also fine, we'll use Mode 1 */
            debug::log(2, FUNCTION, "SESSION::DEFAULT not available (unknown error)");
        }

        /* Mode 2 not available, use Mode 1 (Daemon Stateless) */
        debug::log(1, FUNCTION, "Mining Mode: DAEMON_STATELESS");
        debug::log(1, FUNCTION, "  - No node credentials available");
        debug::log(1, FUNCTION, "  - Expecting miner-signed producers");
        debug::log(1, FUNCTION, "  - Daemon will build blocks around pre-signed producers");
        
        /* NOTE: Mode 1 is always "available" from daemon's perspective.
         * It's the miner's responsibility to provide signed producers.
         * If miner can't provide producers, that's a miner-side error. */
        return MiningMode::DAEMON_STATELESS;
    }


    /* Creates a Tritium block using node credentials (Mode 2). */
    static TritiumBlock* CreateWithNodeCredentials(
        const uint32_t nChannel,
        const uint64_t nExtraNonce,
        const uint256_t& hashRewardAddress)
    {
        try
        {
            /* Get node credentials */
            const uint256_t hashSession = GetDefaultSessionId();
            const auto& pCredentials = 
                TAO::API::Authentication::Credentials(hashSession);

            /* Unlock PIN for mining */
            SecureString strPIN;
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING, hashSession));

            /* Validate reward address is provided
             * Note: Zero is now allowed - CreateBlock/CreateProducer will handle fallback
             * to node operator's genesis when hashRewardAddress is 0 */
            if(hashRewardAddress == 0)
            {
                debug::log(2, FUNCTION, "Warning: Zero reward address - will use node operator's genesis");
                debug::log(2, FUNCTION, "  This is legacy behavior when no explicit reward routing");
            }

            /* Log the dual-identity model */
            debug::log(2, FUNCTION, "Mode 2: Creating block with node credentials");
            debug::log(2, FUNCTION, "  Block signer: ", pCredentials->Genesis().SubString(), " (node operator)");
            if(hashRewardAddress != 0)
            {
                debug::log(2, FUNCTION, "  Reward recipient: ", hashRewardAddress.SubString(), " (stateless miner)");
            }
            else
            {
                debug::log(2, FUNCTION, "  Reward recipient: ", pCredentials->Genesis().SubString(), " (node operator fallback)");
            }
            debug::log(2, FUNCTION, "  Channel: ", nChannel == 1 ? "Prime" : nChannel == 2 ? "Hash" : "Private");

            /* Create the block using standard CreateBlock flow */
            TritiumBlock* pBlock = new TritiumBlock();
            
            bool success = CreateBlock(
                pCredentials,
                strPIN,
                nChannel,
                *pBlock,
                nExtraNonce,
                nullptr,  // No coinbase recipients
                hashRewardAddress  // Route rewards to miner
            );

            if(!success)
            {
                delete pBlock;
                debug::error(FUNCTION, "CreateBlock failed");
                return nullptr;
            }

            debug::log(2, FUNCTION, "Mode 2: Block created successfully");
            return pBlock;
        }
        catch(const std::exception& e)
        {
            debug::error(FUNCTION, "Exception in CreateWithNodeCredentials: ", e.what());
            return nullptr;
        }
    }


    /* Creates a Tritium block with miner-signed producer (Mode 1). */
    static TritiumBlock* CreateWithMinerProducer(
        const uint32_t nChannel,
        const uint64_t nExtraNonce,
        const uint256_t& hashRewardAddress,
        const Transaction& preSignedProducer)
    {
        /* This is the Mode 1 implementation - the challenging part.
         * 
         * CURRENT STATUS: NOT YET IMPLEMENTED
         * 
         * This requires:
         * 1. Validating the pre-signed producer transaction
         * 2. Building block around it (without calling CreateProducer)
         * 3. Adding ambassador rewards, developer fund, mempool transactions
         * 4. Calculating merkle root correctly
         * 5. Setting all block metadata
         * 
         * DESIGN CHALLENGE:
         * - Can't easily modify CreateBlock() to accept optional producer
         * - Would need to duplicate significant logic from create.cpp
         * - Ambassador/developer rewards logic is embedded in CreateProducer()
         * 
         * RECOMMENDED APPROACH (for future implementation):
         * - Refactor create.cpp to extract helper functions:
         *   * AddAmbassadorRewards()
         *   * AddDeveloperFund()
         *   * AddClientTransactions()
         *   * CalculateMerkleRoot()
         * - Then compose them here for Mode 1
         * 
         * ALTERNATIVE (safer for now):
         * - Require miners to use Mode 2 (node with credentials)
         * - Document Mode 1 as "future enhancement"
         * - This prevents breaking existing functionality
         */

        debug::error(FUNCTION, "Mode 1 (DAEMON_STATELESS) not yet implemented");
        debug::error(FUNCTION, "");
        debug::error(FUNCTION, "  CURRENT SOLUTION:");
        debug::error(FUNCTION, "    Start daemon with: ./nexus -daemon -unlock=mining");
        debug::error(FUNCTION, "    This enables Mode 2 (node credentials available)");
        debug::error(FUNCTION, "");
        debug::error(FUNCTION, "  FUTURE SOLUTION:");
        debug::error(FUNCTION, "    Mode 1 will allow pure stateless daemons");
        debug::error(FUNCTION, "    Miners will create and sign producers locally");
        debug::error(FUNCTION, "    Estimated implementation: 6-10 weeks");
        debug::error(FUNCTION, "");
        debug::error(FUNCTION, "  WHY NOT IMPLEMENTED YET:");
        debug::error(FUNCTION, "    Mode 1 requires refactoring consensus-critical code");
        debug::error(FUNCTION, "    Ambassador/developer reward logic embedded in CreateProducer()");
        debug::error(FUNCTION, "    Safer to deliver Mode 2 now, Mode 1 when ecosystem needs it");
        debug::error(FUNCTION, "");
        debug::error(FUNCTION, "See docs/DUAL_MODE_ARCHITECTURE.md for details");

        return nullptr;
    }


    /* Unified interface for stateless mining block creation. */
    TritiumBlock* CreateBlockForStatelessMining(
        const uint32_t nChannel,
        const uint64_t nExtraNonce,
        const uint256_t& hashRewardAddress,
        const Transaction* pPreSignedProducer)
    {
        /* Detect which mode to use */
        MiningMode mode = DetectMiningMode();

        switch(mode)
        {
            case MiningMode::INTERFACE_SESSION:
            {
                /* Mode 2: Use node credentials to sign producer */
                debug::log(2, FUNCTION, "Using Mode 2: Node credentials available");
                return CreateWithNodeCredentials(nChannel, nExtraNonce, hashRewardAddress);
            }

            case MiningMode::DAEMON_STATELESS:
            {
                /* Mode 1: Use miner-signed producer */
                debug::log(2, FUNCTION, "Using Mode 1: Expecting miner-signed producer");

                if(pPreSignedProducer == nullptr)
                {
                    debug::error(FUNCTION, "Mode 1 active but no pre-signed producer provided");
                    debug::error(FUNCTION, "  Miner must send signed producer transaction");
                    debug::error(FUNCTION, "  Or start daemon with -unlock=mining for Mode 2");
                    return nullptr;
                }

                return CreateWithMinerProducer(nChannel, nExtraNonce, hashRewardAddress, *pPreSignedProducer);
            }

            case MiningMode::UNAVAILABLE:
            default:
            {
                debug::error(FUNCTION, "No mining mode available");
                debug::error(FUNCTION, "  Mode 2 requires: -unlock=mining");
                debug::error(FUNCTION, "  Mode 1 requires: miner-signed producer");
                return nullptr;
            }
        }
    }


    /* Validates a miner-signed producer transaction. */
    bool ValidateMinerProducer(
        const Transaction& producer,
        const uint256_t& hashExpectedGenesis,
        const uint256_t& hashBoundRewardAddress)
    {
        /* This would validate:
         * 1. Producer signature matches Falcon-authenticated genesis
         * 2. Producer structure is correct
         * 3. Sequence number is valid
         * 4. Timestamp is recent
         * 5. Not a replay attack
         * 6. Reward address matches bound address
         * 
         * NOT YET IMPLEMENTED - Part of Mode 1 enhancement
         */

        debug::error(FUNCTION, "ValidateMinerProducer not yet implemented");
        return false;
    }


    /* Creates template data for miner-side producer creation. */
    bool CreateProducerTemplate(
        const uint32_t nChannel,
        uint1024_t& hashPrevBlock,
        uint32_t& nHeight,
        uint32_t& nVersion,
        uint64_t& nTimestamp)
    {
        /* This would provide:
         * - Previous block hash
         * - Current block height  
         * - Block version
         * - Suggested timestamp
         * 
         * Miner would use this to create producer locally.
         * 
         * NOT YET IMPLEMENTED - Part of Mode 1 enhancement
         */

        try
        {
            /* Get current best state */
            const TAO::Ledger::BlockState tStateBest =
                TAO::Ledger::ChainState::tStateBest.load();

            /* Fill in template data */
            hashPrevBlock = tStateBest.GetHash();
            nHeight = tStateBest.nHeight + 1;
            nTimestamp = runtime::unifiedtimestamp();

            /* Get block version */
            uint32_t nCurrent = CurrentBlockVersion();
            if(BlockVersionActive(nTimestamp, nCurrent))
                nVersion = nCurrent;
            else
                nVersion = nCurrent - 1;

            debug::log(2, FUNCTION, "Created producer template:");
            debug::log(2, FUNCTION, "  Height: ", nHeight);
            debug::log(2, FUNCTION, "  Version: ", nVersion);
            debug::log(2, FUNCTION, "  Prev Block: ", hashPrevBlock.SubString());
            debug::log(2, FUNCTION, "  Timestamp: ", nTimestamp);

            return true;
        }
        catch(const std::exception& e)
        {
            debug::error(FUNCTION, "Exception: ", e.what());
            return false;
        }
    }

}
