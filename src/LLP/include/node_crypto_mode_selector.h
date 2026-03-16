/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_NODE_CRYPTO_MODE_SELECTOR_H
#define NEXUS_LLP_INCLUDE_NODE_CRYPTO_MODE_SELECTOR_H

#include <Util/include/args.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

namespace LLP
{
    enum class NodeCryptoMode : uint8_t
    {
        LEGACY = 0,
        EVP,
        TLS
    };

    inline NodeCryptoMode ParseNodeCryptoMode(const std::string& strModeRaw)
    {
        std::string strMode = strModeRaw;
        std::transform(strMode.begin(), strMode.end(), strMode.begin(),
            [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if(strMode == "evp")
            return NodeCryptoMode::EVP;

        if(strMode == "tls")
            return NodeCryptoMode::TLS;

        return NodeCryptoMode::LEGACY;
    }

    inline NodeCryptoMode GetNodeCryptoMode()
    {
        return ParseNodeCryptoMode(config::GetArg("-crypto_mode", std::string("legacy")));
    }

    constexpr const char* NodeCryptoModeString(const NodeCryptoMode mode)
    {
        return mode == NodeCryptoMode::EVP ? "evp" : (mode == NodeCryptoMode::TLS ? "tls" : "legacy");
    }
}

#endif
