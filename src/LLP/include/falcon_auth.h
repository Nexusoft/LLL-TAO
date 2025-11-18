/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_FALCON_AUTH_H
#define NEXUS_LLP_INCLUDE_FALCON_AUTH_H

#include <LLC/types/uint1024.h>

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace LLP
{
namespace FalconAuth
{
    /** Falcon Security Profile **/
    enum class Profile : uint8_t
    {
        FALCON_512 = 0,
        FALCON_1024 = 1
    };

    /** KeyMetadata
     *
     *  Metadata about a stored Falcon key.
     *
     **/
    struct KeyMetadata
    {
        uint256_t keyId;           // Stable identifier derived from public key
        std::vector<uint8_t> pubkey; // Falcon public key
        Profile profile;             // Security profile (512 or 1024)
        uint64_t created;           // Creation timestamp
        uint64_t lastUsed;          // Last usage timestamp
        std::string label;          // User-friendly label
    };

    /** VerifyResult
     *
     *  Result of signature verification.
     *
     **/
    struct VerifyResult
    {
        bool fValid;              // Whether signature is valid
        uint256_t keyId;          // Derived key identifier
        std::string strError;     // Error message if invalid

        static VerifyResult Success(const uint256_t& keyId);
        static VerifyResult Failure(const std::string& error);
    };

    /** IFalconAuth
     *
     *  Interface for Falcon post-quantum signature operations.
     *  Supports key generation, signing, verification, and Tritium identity binding.
     *
     **/
    class IFalconAuth
    {
    public:
        virtual ~IFalconAuth() = default;

        /** GenerateKey
         *
         *  Generate a new Falcon key pair.
         *
         *  @param[in] profile Security profile (512 or 1024 bits)
         *  @param[in] label Optional user-friendly label
         *
         *  @return Metadata for the generated key
         *
         **/
        virtual KeyMetadata GenerateKey(
            Profile profile = Profile::FALCON_512,
            const std::string& label = ""
        ) = 0;

        /** ListKeys
         *
         *  List all stored Falcon keys.
         *
         *  @return Vector of key metadata
         *
         **/
        virtual std::vector<KeyMetadata> ListKeys() const = 0;

        /** GetKey
         *
         *  Retrieve metadata for a specific key.
         *
         *  @param[in] keyId Key identifier
         *
         *  @return Key metadata if found, std::nullopt otherwise
         *
         **/
        virtual std::optional<KeyMetadata> GetKey(const uint256_t& keyId) const = 0;

        /** Sign
         *
         *  Sign a message with a Falcon private key.
         *
         *  @param[in] keyId Key identifier
         *  @param[in] message Message bytes to sign
         *
         *  @return Signature bytes, or empty vector on error
         *
         **/
        virtual std::vector<uint8_t> Sign(
            const uint256_t& keyId,
            const std::vector<uint8_t>& message
        ) = 0;

        /** Verify
         *
         *  Verify a Falcon signature.
         *
         *  @param[in] pubkey Public key bytes
         *  @param[in] message Message bytes that were signed
         *  @param[in] signature Signature bytes
         *
         *  @return VerifyResult with validity and derived key ID
         *
         **/
        virtual VerifyResult Verify(
            const std::vector<uint8_t>& pubkey,
            const std::vector<uint8_t>& message,
            const std::vector<uint8_t>& signature
        ) = 0;

        /** DeriveKeyId
         *
         *  Derive a stable key identifier from a public key.
         *
         *  @param[in] pubkey Public key bytes
         *
         *  @return 256-bit key identifier
         *
         **/
        virtual uint256_t DeriveKeyId(const std::vector<uint8_t>& pubkey) const = 0;

        /** BindGenesis
         *
         *  Phase 2: Associate a Falcon key with a Tritium account identity.
         *
         *  @param[in] keyId Falcon key identifier
         *  @param[in] hashGenesis Tritium genesis hash
         *
         *  @return true on success, false on error
         *
         **/
        virtual bool BindGenesis(
            const uint256_t& keyId,
            const uint256_t& hashGenesis
        ) = 0;

        /** GetBoundGenesis
         *
         *  Phase 2: Retrieve the Tritium genesis bound to a Falcon key.
         *
         *  @param[in] keyId Falcon key identifier
         *
         *  @return Genesis hash if bound, std::nullopt otherwise
         *
         **/
        virtual std::optional<uint256_t> GetBoundGenesis(const uint256_t& keyId) const = 0;
    };

    /** Get
     *
     *  Get the global Falcon auth instance.
     *
     *  @return Pointer to IFalconAuth, or nullptr if not initialized
     *
     **/
    IFalconAuth* Get();

    /** Initialize
     *
     *  Initialize the global Falcon auth instance.
     *
     **/
    void Initialize();

    /** Shutdown
     *
     *  Shutdown and cleanup the global Falcon auth instance.
     *
     **/
    void Shutdown();

    /** KeyMetadataToJSON
     *
     *  Convert key metadata to JSON representation.
     *
     *  @param[in] meta Key metadata
     *
     *  @return JSON string
     *
     **/
    std::string KeyMetadataToJSON(const KeyMetadata& meta);

} // namespace FalconAuth
} // namespace LLP

#endif
