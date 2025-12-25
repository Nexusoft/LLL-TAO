/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_STATELESS_BLOCK_UTILITY_H
#define NEXUS_TAO_LEDGER_INCLUDE_STATELESS_BLOCK_UTILITY_H

#include <TAO/Ledger/types/tritium.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** CreateBlockForStatelessMining
         *
         *  Dual-mode block creation for stateless mining.
         *
         *  Mode 1: Wallet mode (Session::DEFAULT with autologin)
         *  - Uses existing CreateBlock() with node credentials
         *  - Node operator signs block, rewards routed per hashRewardAddress
         *
         *  Mode 2: Stateless mode (Falcon authenticated)
         *  - Creates block WITHOUT wallet credentials
         *  - Uses CreateProducerStateless() for consensus logic
         *  - Miner provides Falcon signature
         *
         *  @param[in] nChannel The mining channel (1 = Prime, 2 = Hash)
         *  @param[in] nExtraNonce Extra nonce for block iteration
         *  @param[in] hashRewardAddress Miner's reward recipient address
         *  @param[in] fFalconAuthenticated True for stateless mode, false for wallet mode
         *
         *  @return Pointer to created block, or nullptr on failure
         *
         **/
        TritiumBlock* CreateBlockForStatelessMining(
            const uint32_t nChannel,
            const uint64_t nExtraNonce,
            const uint256_t& hashRewardAddress,
            const bool fFalconAuthenticated = false);

    }
}

#endif
