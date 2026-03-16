/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_SESSION_KEY_LIFECYCLE_H
#define NEXUS_LLP_INCLUDE_SESSION_KEY_LIFECYCLE_H

#include <LLC/include/chacha20_evp_manager.h>

#include <cstdint>
#include <vector>

namespace LLP
{
    class SessionKeyLifecycle
    {
    public:
        static void EstablishSession(const uint32_t nSessionId, const std::vector<uint8_t>& vSessionKey)
        {
            LLC::ChaCha20EvpManager::Instance().InitSession(nSessionId, vSessionKey);
        }

        static void RotateSession(const uint32_t nSessionId, const std::vector<uint8_t>& vSessionKey)
        {
            LLC::ChaCha20EvpManager::Instance().RotateSession(nSessionId, vSessionKey);
        }

        static void TeardownSession(const uint32_t nSessionId)
        {
            LLC::ChaCha20EvpManager::Instance().TeardownSession(nSessionId);
        }

        static uint64_t SessionGeneration(const uint32_t nSessionId)
        {
            return LLC::ChaCha20EvpManager::Instance().SessionGeneration(nSessionId);
        }
    };
}

#endif
