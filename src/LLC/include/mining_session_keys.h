/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_MINING_SESSION_KEYS_H
#define NEXUS_LLC_MINING_SESSION_KEYS_H

#include <LLC/types/uint1024.h>
#include <LLC/hash/SK.h>
#include <Util/include/hex.h>

#include <openssl/sha.h>

#include <vector>
#include <string>

namespace LLC
{
namespace MiningSessionKeys
{
    /** Domain separation strings for key derivation */
    static const std::string DOMAIN_CHACHA20 = "nexus-mining-chacha20-v1";
    static const std::string DOMAIN_FALCON_SESSION = "nexus-mining-falcon-session-v1";


    /** DeriveChaCha20Key
     *
     *  Derives a ChaCha20-Poly1305 session key from Tritium genesis hash.
     *  
     *  This key is used throughout the mining protocol for encrypting:
     *  - Falcon public keys during MINER_AUTH_INIT (proves genesis knowledge)
     *  - Reward addresses during MINER_SET_REWARD (privacy)
     *  - Result packets like MINER_REWARD_RESULT (confidentiality)
     *
     *  The genesis-based derivation serves as a TLS replacement for mining pools,
     *  providing session-specific encryption without requiring persistent key storage.
     *
     *  Algorithm: SHA256(domain_string || genesis_bytes)
     *  Domain: "nexus-mining-chacha20-v1"
     *
     *  IMPORTANT: This implementation uses OpenSSL SHA256 (not SK256) to maintain
     *  compatibility with NexusMiner's key derivation. The genesis bytes are obtained
     *  via GetHex() + ParseHex() to ensure consistent big-endian representation.
     *
     *  @param[in] hashGenesis Tritium genesis hash (account payout address)
     *
     *  @return 32-byte ChaCha20 key derived from genesis
     *
     **/
    inline std::vector<uint8_t> DeriveChaCha20Key(const uint256_t& hashGenesis)
    {
        /* Build preimage: domain || genesis_bytes */
        std::vector<uint8_t> preimage;
        preimage.reserve(DOMAIN_CHACHA20.size() + 32);
        preimage.insert(preimage.end(), DOMAIN_CHACHA20.begin(), DOMAIN_CHACHA20.end());
        
        /* Use GetHex() + ParseHex() for consistent big-endian representation.
         * This avoids the GetBytes() little-endian issue and ensures compatibility
         * with NexusMiner's implementation. */
        std::string genesis_hex = hashGenesis.GetHex();
        std::vector<uint8_t> genesis_bytes = ParseHex(genesis_hex);
        
        /* Validate parsed bytes - should always be 32 bytes for uint256_t */
        if(genesis_bytes.size() != 32)
        {
            /* Error case: return zeros */
            return std::vector<uint8_t>(32, 0);
        }
        
        preimage.insert(preimage.end(), genesis_bytes.begin(), genesis_bytes.end());
        
        /* Use OpenSSL SHA256 directly (same as NexusMiner).
         * This ensures identical output on both sides for authentication. */
        std::vector<uint8_t> vKey(32);  // SHA256 always outputs 32 bytes
        unsigned char* result = SHA256(preimage.data(), preimage.size(), vKey.data());
        
        if(!result)
        {
            /* OpenSSL error: return zeros */
            return std::vector<uint8_t>(32, 0);
        }
        
        return vKey;
    }


    /** DeriveFalconSessionId
     *
     *  Derives a Falcon session identifier from genesis + pubkey + timestamp.
     *  
     *  This was the ORIGINAL FalconHandshake design for session binding.
     *  Currently used for session recovery and session-specific security.
     *  The timestamp is rounded to the hour for session stability.
     *
     *  Algorithm: SK256(genesis || pubkey || rounded_timestamp)
     *  Note: Unlike ChaCha20 keys, this uses SK256 (Skein-Keccak) which is
     *  Nexus's standard hash function for internal operations.
     *
     *  @param[in] hashGenesis Tritium genesis hash
     *  @param[in] vPubKey Falcon public key bytes
     *  @param[in] nTimestamp Session start timestamp
     *
     *  @return Session identifier hash
     *
     **/
    inline uint256_t DeriveFalconSessionId(
        const uint256_t& hashGenesis,
        const std::vector<uint8_t>& vPubKey,
        uint64_t nTimestamp
    )
    {
        /* Build session key derivation input */
        std::vector<uint8_t> vInput;
        
        /* Add genesis hash */
        std::vector<uint8_t> vGenesis = hashGenesis.GetBytes();
        vInput.insert(vInput.end(), vGenesis.begin(), vGenesis.end());
        
        /* Add miner public key */
        vInput.insert(vInput.end(), vPubKey.begin(), vPubKey.end());
        
        /* Add timestamp (rounded to hour for session stability) */
        uint64_t nRoundedTime = (nTimestamp / 3600) * 3600;
        for(size_t i = 0; i < 8; ++i)
        {
            vInput.push_back(static_cast<uint8_t>((nRoundedTime >> (i * 8)) & 0xFF));
        }
        
        /* Hash to derive session key using SK256 */
        return LLC::SK256(vInput);
    }


    /** DeriveKeyId
     *
     *  Derives a stable key identifier from Falcon public key.
     *  
     *  Used throughout the codebase for tracking and identifying Falcon keys:
     *  - FalconAuth key storage and lookup
     *  - Session ID generation (lower 32 bits of key ID)
     *  - Miner authentication tracking
     *
     *  Algorithm: SK256(pubkey_bytes)
     *
     *  @param[in] vPubKey Falcon public key bytes
     *
     *  @return Key identifier hash
     *
     **/
    inline uint256_t DeriveKeyId(const std::vector<uint8_t>& vPubKey)
    {
        return LLC::SK256(vPubKey);
    }

} // namespace MiningSessionKeys
} // namespace LLC

#endif
