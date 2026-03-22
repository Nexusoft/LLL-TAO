/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_FALCON_HANDSHAKE_H
#define NEXUS_LLP_INCLUDE_FALCON_HANDSHAKE_H

#include <LLC/types/uint1024.h>

#include <string>
#include <vector>
#include <cstdint>

namespace LLP
{
namespace FalconHandshake
{
    /** HandshakeConfig
     *
     *  Configuration for Falcon handshake process.
     *
     **/
    struct HandshakeConfig
    {
        bool fEnableEncryption;          // Enable ChaCha20 encryption for public keys
        bool fRequireEncryption;         // Require encryption (mandatory for public nodes)
        bool fAllowLocalhostPlaintext;   // Allow plaintext for localhost connections
        uint64_t nHandshakeTimeout;      // Handshake timeout in seconds
        
        /** Default Constructor **/
        HandshakeConfig()
        : fEnableEncryption(true)
        , fRequireEncryption(false)
        , fAllowLocalhostPlaintext(true)
        , nHandshakeTimeout(60)
        {
        }
    };


    /** HandshakeData
     *
     *  Data exchanged during Falcon handshake.
     *  Contains Falcon public key, genesis hash, and session information.
     *
     **/
    struct HandshakeData
    {
        std::vector<uint8_t> vFalconPubKey;  // Falcon public key (may be encrypted)
        uint256_t hashGenesis;               // Tritium genesis hash (payout address)
        uint256_t hashSessionKey;            // Dynamic session key tied to genesis
        uint64_t nTimestamp;                 // Handshake timestamp
        bool fEncrypted;                     // Whether public key is encrypted
        std::string strMinerID;              // Miner identifier (optional)

        /** Default Constructor **/
        HandshakeData()
        : vFalconPubKey()
        , hashGenesis(0)
        , hashSessionKey(0)
        , nTimestamp(0)
        , fEncrypted(false)
        , strMinerID("")
        {
        }
    };


    /** HandshakeResult
     *
     *  Result of handshake operation.
     *
     **/
    struct HandshakeResult
    {
        bool fSuccess;                   // Whether operation succeeded
        std::string strError;            // Error message if failed
        HandshakeData data;              // Handshake data
        uint256_t hashKeyID;             // Derived Falcon key ID

        static HandshakeResult Success(const HandshakeData& data_, const uint256_t& keyId_);
        static HandshakeResult Failure(const std::string& error);
    };


    /** EncryptFalconPublicKey
     *
     *  Encrypt a Falcon public key using ChaCha20.
     *  Used during handshake to protect key exchange over network.
     *
     *  SECURITY NOTE: ChaCha20 provides confidentiality but not authentication.
     *  The encrypted public key should be validated after decryption via:
     *  1. Falcon signature verification (primary authentication)
     *  2. TLS 1.3 transport security (prevents MITM attacks)
     *  3. GenesisHash binding (ties key to blockchain identity)
     *
     *  For maximum security in public pools, use TLS 1.3 with ChaCha20-Poly1305-SHA256
     *  cipher suite which provides both encryption and authentication at transport layer.
     *
     *  @param[in] vPubKey Falcon public key bytes
     *  @param[in] vKey ChaCha20 key (32 bytes)
     *  @param[in] vNonce ChaCha20 nonce (12 bytes)
     *
     *  @return Encrypted public key bytes, or empty on error
     *
     **/
    std::vector<uint8_t> EncryptFalconPublicKey(
        const std::vector<uint8_t>& vPubKey,
        const std::vector<uint8_t>& vKey,
        const std::vector<uint8_t>& vNonce
    );


    /** DecryptFalconPublicKey
     *
     *  Decrypt a Falcon public key using ChaCha20.
     *
     *  @param[in] vEncrypted Encrypted public key bytes
     *  @param[in] vKey ChaCha20 key (32 bytes)
     *  @param[in] vNonce ChaCha20 nonce (12 bytes)
     *
     *  @return Decrypted public key bytes, or empty on error
     *
     **/
    std::vector<uint8_t> DecryptFalconPublicKey(
        const std::vector<uint8_t>& vEncrypted,
        const std::vector<uint8_t>& vKey,
        const std::vector<uint8_t>& vNonce
    );


    /** GenerateSessionKey
     *
     *  Generate a dynamic Falcon session key tied to Tritium genesis hash.
     *  This provides session-specific security without requiring persistent key storage.
     *
     *  @param[in] hashGenesis Tritium genesis hash
     *  @param[in] vMinerPubKey Miner's Falcon public key
     *  @param[in] nTimestamp Session timestamp
     *
     *  @return Derived session key hash
     *
     **/
    uint256_t GenerateSessionKey(
        const uint256_t& hashGenesis,
        const std::vector<uint8_t>& vMinerPubKey,
        uint64_t nTimestamp
    );


    /** BuildHandshakePacket
     *
     *  Build a handshake packet with Falcon public key and genesis hash.
     *  Optionally encrypts the public key using ChaCha20.
     *
     *  @param[in] config Handshake configuration
     *  @param[in] data Handshake data to encode
     *  @param[in] vEncryptionKey Optional encryption key (for ChaCha20)
     *
     *  @return Handshake packet bytes, or empty on error
     *
     **/
    std::vector<uint8_t> BuildHandshakePacket(
        const HandshakeConfig& config,
        const HandshakeData& data,
        const std::vector<uint8_t>& vEncryptionKey = std::vector<uint8_t>()
    );


    /** ParseHandshakePacket
     *
     *  Parse a handshake packet and extract Falcon public key and genesis hash.
     *  Optionally decrypts the public key if encrypted.
     *
     *  @param[in] vPacket Packet bytes
     *  @param[in] config Handshake configuration
     *  @param[in] vEncryptionKey Optional encryption key (for ChaCha20)
     *
     *  @return HandshakeResult with parsed data or error
     *
     **/
    HandshakeResult ParseHandshakePacket(
        const std::vector<uint8_t>& vPacket,
        const HandshakeConfig& config,
        const std::vector<uint8_t>& vEncryptionKey = std::vector<uint8_t>()
    );


    /** ValidateHandshakeData
     *
     *  Validate handshake data for completeness and correctness.
     *
     *  @param[in] data Handshake data to validate
     *
     *  @return true if valid, false otherwise
     *
     **/
    bool ValidateHandshakeData(const HandshakeData& data);


    /** IsLocalhostHandshake
     *
     *  Check if handshake is from localhost connection.
     *  Used to determine if encryption can be optional.
     *
     *  @param[in] strAddress Connection address
     *
     *  @return true if localhost
     *
     **/
    bool IsLocalhostHandshake(const std::string& strAddress);

} // namespace FalconHandshake
} // namespace LLP

#endif
