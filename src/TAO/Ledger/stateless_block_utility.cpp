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

#include <TAO/API/include/global.h>
#include <TAO/API/types/authentication.h>

#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO::Ledger
{
    /* Simplified block creation for stateless mining - single path implementation */
    TritiumBlock* CreateBlockForStatelessMining(
        const uint32_t nChannel,
        const uint64_t nExtraNonce,
        const uint256_t& hashRewardAddress)
    {
        try
        {
            /* Check mining is unlocked */
            if(!TAO::API::Authentication::Unlocked(TAO::Ledger::PinUnlock::MINING))
            {
                debug::error(FUNCTION, "Mining not unlocked - use -unlock=mining");
                return nullptr;
            }

            /* Get node credentials */
            const uint256_t hashSession = uint256_t(TAO::API::Authentication::SESSION::DEFAULT);
            const auto& pCredentials = TAO::API::Authentication::Credentials(hashSession);

            /* Unlock PIN for mining */
            SecureString strPIN;
            RECURSIVE(TAO::API::Authentication::Unlock(strPIN, TAO::Ledger::PinUnlock::MINING, hashSession));

            /* Create producer transaction and build block */
            TritiumBlock* pBlock = new TritiumBlock();
            
            bool success = CreateBlock(
                pCredentials,
                strPIN,
                nChannel,
                *pBlock,
                nExtraNonce,
                nullptr,           // No coinbase recipients
                hashRewardAddress  // Route rewards to miner
            );

            if(!success)
            {
                delete pBlock;
                debug::error(FUNCTION, "CreateBlock failed");
                return nullptr;
            }

            debug::log(2, FUNCTION, "Block created successfully");
            return pBlock;
        }
        catch(const std::exception& e)
        {
            debug::error(FUNCTION, "Exception in CreateBlockForStatelessMining: ", e.what());
            return nullptr;
        }
    }

}
