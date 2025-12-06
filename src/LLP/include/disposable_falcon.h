/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_DISPOSABLE_FALCON_H
#define NEXUS_LLP_INCLUDE_DISPOSABLE_FALCON_H

#include <LLC/types/uint1024.h>
#include <LLC/include/flkey.h>
#include <LLP/include/falcon_constants.h>

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace LLP
{
namespace DisposableFalcon
{
    /** SignedWorkSubmission
     *
     *  Structure containing a work submission wrapped with a disposable
     *  Falcon signature. The signature provides real-time security for
     *  the Submit Block workflow without adding any data to the blockchain.
     *
     *  The signature is "disposable" because:
     *  1. It uses an ephemeral session-specific keypair
     *  2. It wraps only the work submission data (merkle root + nonce)
     *  3. It is unwrapped and verified by the Node before block processing
     *  4. No signature data is stored on the blockchain (zero overhead)
     *
     *  PROTOCOL FORMAT (Little-Endian for all multi-byte integers):
     *  [merkle_root (64 bytes)][nonce (8 bytes LE)][timestamp (8 bytes LE)][sig_len (2 bytes LE)][signature]
     *
     **/
    struct SignedWorkSubmission
    {
        uint512_t hashMerkleRoot;           // Block's merkle root
        uint64_t nNonce;                    // Proof-of-work nonce
        std::vector<uint8_t> vSignature;    // Disposable Falcon signature
        uint256_t hashKeyID;                // Key ID of the signer (for verification)
        uint64_t nTimestamp;                // Submission timestamp (for replay protection)
        bool fSigned;                       // Whether submission is signed

        /** Default Constructor **/
        SignedWorkSubmission();

        /** Constructor with work data **/
        SignedWorkSubmission(const uint512_t& hashMerkle, uint64_t nNonce_);

        /** GetMessageBytes
         *
         *  Returns the message bytes that should be signed.
         *  Format: merkle_root || nonce || timestamp
         *
         *  @return Message bytes for signing
         *
         **/
        std::vector<uint8_t> GetMessageBytes() const;

        /** Serialize
         *
         *  Serialize the signed submission into a byte vector.
         *  Format: [merkle_root][nonce][timestamp][sig_len][signature]
         *
         *  @return Serialized bytes
         *
         **/
        std::vector<uint8_t> Serialize() const;

        /** Deserialize
         *
         *  Deserialize a signed submission from bytes.
         *
         *  @param[in] vData Input bytes
         *
         *  @return true on success, false on failure
         *
         **/
        bool Deserialize(const std::vector<uint8_t>& vData);

        /** IsValid
         *
         *  Check if the submission has valid structure.
         *
         *  @return true if structure is valid
         *
         **/
        bool IsValid() const;

        /** DebugString
         *
         *  Returns a debug string representation.
         *
         *  @return Human-readable string
         *
         **/
        std::string DebugString() const;
    };


    /** WrapperResult
     *
     *  Result of wrapping or unwrapping a work submission.
     *
     **/
    struct WrapperResult
    {
        bool fSuccess;                      // Whether operation succeeded
        std::string strError;               // Error message if failed
        SignedWorkSubmission submission;    // The wrapped/unwrapped submission
        uint256_t hashKeyID;                // Verified key ID (for unwrap)

        static WrapperResult Success(const SignedWorkSubmission& sub);
        static WrapperResult Success(const SignedWorkSubmission& sub, const uint256_t& keyId);
        static WrapperResult Failure(const std::string& error);
    };


    /** IDisposableFalconWrapper
     *
     *  Interface for the Disposable Falcon Wrapper that provides real-time
     *  security for Submit Block workflows using ephemeral Falcon signatures.
     *
     *  DESIGN PRINCIPLES:
     *  1. Zero blockchain overhead - signatures are verified and discarded
     *  2. Session-scoped security - new keypair per mining session
     *  3. Non-repudiation - Node can verify miner identity for each submission
     *  4. Scalability - Ephemeral keys avoid key management overhead
     *
     *  WORKFLOW:
     *  1. Miner authenticates using Falcon handshake (session key established)
     *  2. For each SUBMIT_BLOCK, miner wraps work data with disposable signature
     *  3. Node unwraps and verifies signature before processing block
     *  4. Signature is discarded - only block data goes to blockchain
     *
     **/
    class IDisposableFalconWrapper
    {
    public:
        virtual ~IDisposableFalconWrapper() = default;

        /** GenerateSessionKey
         *
         *  Generate a new ephemeral Falcon keypair for this mining session.
         *  Should be called at the start of each authenticated session.
         *
         *  @param[in] hashSessionId Session identifier for key binding
         *
         *  @return true on success, false on failure
         *
         **/
        virtual bool GenerateSessionKey(const uint256_t& hashSessionId) = 0;

        /** WrapWorkSubmission
         *
         *  Sign a work submission with the session's disposable Falcon key.
         *  This wraps the work data with a signature for verification.
         *
         *  @param[in] hashMerkleRoot Block's merkle root
         *  @param[in] nNonce Proof-of-work nonce
         *
         *  @return WrapperResult containing signed submission or error
         *
         **/
        virtual WrapperResult WrapWorkSubmission(
            const uint512_t& hashMerkleRoot,
            uint64_t nNonce
        ) = 0;

        /** UnwrapWorkSubmission
         *
         *  Verify and unwrap a signed work submission.
         *  Validates the disposable Falcon signature and extracts work data.
         *
         *  @param[in] vData Serialized signed submission
         *  @param[in] vPubKey Miner's Falcon public key (from auth handshake)
         *
         *  @return WrapperResult containing verified submission or error
         *
         **/
        virtual WrapperResult UnwrapWorkSubmission(
            const std::vector<uint8_t>& vData,
            const std::vector<uint8_t>& vPubKey
        ) = 0;

        /** GetSessionKeyId
         *
         *  Get the key ID of the current session's disposable key.
         *
         *  @return Session key ID, or 0 if no session key exists
         *
         **/
        virtual uint256_t GetSessionKeyId() const = 0;

        /** GetSessionPubKey
         *
         *  Get the public key of the current session's disposable key.
         *
         *  @return Session public key, or empty vector if no session key
         *
         **/
        virtual std::vector<uint8_t> GetSessionPubKey() const = 0;

        /** HasActiveSession
         *
         *  Check if a session key is currently active.
         *
         *  @return true if session key exists
         *
         **/
        virtual bool HasActiveSession() const = 0;

        /** ClearSession
         *
         *  Clear the current session key (e.g., on disconnect).
         *
         **/
        virtual void ClearSession() = 0;
    };


    /** Create
     *
     *  Create a new Disposable Falcon Wrapper instance.
     *
     *  @return Unique pointer to wrapper instance
     *
     **/
    std::unique_ptr<IDisposableFalconWrapper> Create();


    /** DebugLogPacket
     *
     *  Log detailed packet information for debugging Falcon handshake issues.
     *  Captures incoming packet contents, size, and deserialization progress.
     *
     *  @param[in] strContext Context string for log message
     *  @param[in] vData Packet data bytes
     *  @param[in] nVerbose Verbosity level (default 3)
     *
     **/
    void DebugLogPacket(
        const std::string& strContext,
        const std::vector<uint8_t>& vData,
        uint32_t nVerbose = 3
    );


    /** DebugLogDeserialize
     *
     *  Log deserialization progress for debugging GetBytes() issues.
     *
     *  @param[in] strField Field being deserialized
     *  @param[in] nOffset Current byte offset
     *  @param[in] nSize Size of field being read
     *  @param[in] nTotalSize Total packet size
     *  @param[in] nVerbose Verbosity level (default 4)
     *
     **/
    void DebugLogDeserialize(
        const std::string& strField,
        size_t nOffset,
        size_t nSize,
        size_t nTotalSize,
        uint32_t nVerbose = 4
    );


} // namespace DisposableFalcon
} // namespace LLP

#endif
