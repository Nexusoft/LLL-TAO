/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_FALCON_VERIFY_H
#define NEXUS_LLP_INCLUDE_FALCON_VERIFY_H

#include <vector>
#include <cstdint>
#include <LLC/include/flkey.h>

namespace LLP
{
namespace FalconVerify
{
    /** VerifyPublicKey512
     *
     *  Verify that a public key is a valid Falcon-512 key.
     *
     *  @param[in] pubkey Public key bytes
     *
     *  @return True if valid Falcon-512 public key
     *
     **/
    bool VerifyPublicKey512(const std::vector<uint8_t>& pubkey);


    /** VerifyPublicKey1024
     *
     *  Verify that a public key is a valid Falcon-1024 key.
     *
     *  @param[in] pubkey Public key bytes
     *
     *  @return True if valid Falcon-1024 public key
     *
     **/
    bool VerifyPublicKey1024(const std::vector<uint8_t>& pubkey);


    /** VerifyPublicKey
     *
     *  Verify a public key and auto-detect its version.
     *
     *  @param[in] pubkey Public key bytes
     *  @param[out] detected Detected Falcon version
     *
     *  @return True if valid public key, with version in detected
     *
     **/
    bool VerifyPublicKey(const std::vector<uint8_t>& pubkey, LLC::FalconVersion& detected);


    /** VerifySignature
     *
     *  Verify a Falcon signature with a specific version.
     *
     *  @param[in] pubkey Public key bytes
     *  @param[in] message Message bytes that were signed
     *  @param[in] signature Signature bytes
     *  @param[in] version Falcon version to use for verification
     *
     *  @return True if signature is valid
     *
     **/
    bool VerifySignature(const std::vector<uint8_t>& pubkey,
                        const std::vector<uint8_t>& message,
                        const std::vector<uint8_t>& signature,
                        LLC::FalconVersion version);


    /** DetectVersionFromPubkey
     *
     *  Auto-detect Falcon version from public key.
     *
     *  @param[in] pubkey Public key bytes
     *
     *  @return Detected Falcon version, or throws on error
     *
     **/
    LLC::FalconVersion DetectVersionFromPubkey(const std::vector<uint8_t>& pubkey);


    /** DetectVersionFromSignature
     *
     *  Auto-detect Falcon version from signature size.
     *  Note: This is less reliable than pubkey detection.
     *
     *  @param[in] signature Signature bytes
     *
     *  @return Detected Falcon version, or throws on error
     *
     **/
    LLC::FalconVersion DetectVersionFromSignature(const std::vector<uint8_t>& signature);


    /** VerifyPhysicalFalconSignature
     *
     *  Verify a Physical Falcon signature (ALWAYS Falcon-512).
     *  Physical Falcon signatures are permanently stored on blockchain.
     *  
     *  This function enforces that Physical Falcon ONLY uses Falcon-512
     *  to minimize permanent blockchain overhead.
     *
     *  @param[in] pubkey Public key bytes (must be Falcon-512)
     *  @param[in] message Message bytes that were signed
     *  @param[in] signature Signature bytes (must be Falcon-512 CT size: 809)
     *
     *  @return True if signature is valid Falcon-512 signature
     *
     **/
    bool VerifyPhysicalFalconSignature(const std::vector<uint8_t>& pubkey,
                                       const std::vector<uint8_t>& message,
                                       const std::vector<uint8_t>& signature);


    /** IsPhysicalFalconEnabled
     *
     *  Check if Physical Falcon signatures are enabled.
     *
     *  @return True if physicalsigner config is enabled
     *
     **/
    bool IsPhysicalFalconEnabled();

} // namespace FalconVerify
} // namespace LLP

#endif
