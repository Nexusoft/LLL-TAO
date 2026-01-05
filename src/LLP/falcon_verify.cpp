/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/falcon_verify.h>
#include <LLC/include/flkey.h>
#include <Util/include/debug.h>

namespace LLP
{
namespace FalconVerify
{
    /* Verify that a public key is a valid Falcon-512 key. */
    bool VerifyPublicKey512(const std::vector<uint8_t>& pubkey)
    {
        /* Check size */
        if(pubkey.size() != LLC::FalconSizes::FALCON512_PUBLIC_KEY_SIZE)
        {
            debug::log(3, FUNCTION, "Invalid Falcon-512 pubkey size: ", pubkey.size(), 
                      " (expected ", LLC::FalconSizes::FALCON512_PUBLIC_KEY_SIZE, ")");
            return false;
        }

        /* Check header byte (first byte should be 0x00 for Falcon-512 logn=9) */
        if(pubkey.empty() || (pubkey[0] != 0x00 && pubkey[0] != 0x09))
        {
            debug::log(3, FUNCTION, "Invalid Falcon-512 pubkey header byte: ", 
                      (pubkey.empty() ? 0 : static_cast<int>(pubkey[0])));
            return false;
        }

        return true;
    }


    /* Verify that a public key is a valid Falcon-1024 key. */
    bool VerifyPublicKey1024(const std::vector<uint8_t>& pubkey)
    {
        /* Check size */
        if(pubkey.size() != LLC::FalconSizes::FALCON1024_PUBLIC_KEY_SIZE)
        {
            debug::log(3, FUNCTION, "Invalid Falcon-1024 pubkey size: ", pubkey.size(), 
                      " (expected ", LLC::FalconSizes::FALCON1024_PUBLIC_KEY_SIZE, ")");
            return false;
        }

        /* Check header byte (first byte should be 0x01 for Falcon-1024 logn=10) */
        if(pubkey.empty() || (pubkey[0] != 0x01 && pubkey[0] != 0x0a))
        {
            debug::log(3, FUNCTION, "Invalid Falcon-1024 pubkey header byte: ", 
                      (pubkey.empty() ? 0 : static_cast<int>(pubkey[0])));
            return false;
        }

        return true;
    }


    /* Verify a public key and auto-detect its version. */
    bool VerifyPublicKey(const std::vector<uint8_t>& pubkey, LLC::FalconVersion& detected)
    {
        /* Try Falcon-512 first */
        if(pubkey.size() == LLC::FalconSizes::FALCON512_PUBLIC_KEY_SIZE)
        {
            if(VerifyPublicKey512(pubkey))
            {
                detected = LLC::FalconVersion::FALCON_512;
                return true;
            }
        }
        /* Try Falcon-1024 */
        else if(pubkey.size() == LLC::FalconSizes::FALCON1024_PUBLIC_KEY_SIZE)
        {
            if(VerifyPublicKey1024(pubkey))
            {
                detected = LLC::FalconVersion::FALCON_1024;
                return true;
            }
        }

        debug::log(2, FUNCTION, "Unable to detect Falcon version from pubkey size: ", pubkey.size());
        return false;
    }


    /* Verify a Falcon signature with a specific version. */
    bool VerifySignature(const std::vector<uint8_t>& pubkey,
                        const std::vector<uint8_t>& message,
                        const std::vector<uint8_t>& signature,
                        LLC::FalconVersion version)
    {
        try
        {
            /* Create FLKey instance */
            LLC::FLKey key;

            /* Set public key (will auto-detect version from size) */
            if(!key.SetPubKey(pubkey))
            {
                debug::log(2, FUNCTION, "Failed to set public key");
                return false;
            }

            /* Verify that the key version matches expected version */
            if(key.GetVersion() != version)
            {
                debug::log(2, FUNCTION, "Key version mismatch: expected ", 
                          static_cast<int>(version), " got ", static_cast<int>(key.GetVersion()));
                return false;
            }

            /* Verify signature */
            if(!key.Verify(message, signature))
            {
                debug::log(2, FUNCTION, "Signature verification failed");
                return false;
            }

            return true;
        }
        catch(const std::exception& e)
        {
            debug::log(1, FUNCTION, "Exception during signature verification: ", e.what());
            return false;
        }
    }


    /* Auto-detect Falcon version from public key. */
    LLC::FalconVersion DetectVersionFromPubkey(const std::vector<uint8_t>& pubkey)
    {
        return LLC::FLKey::DetectVersion(pubkey.size(), true);
    }


    /* Auto-detect Falcon version from signature size. */
    LLC::FalconVersion DetectVersionFromSignature(const std::vector<uint8_t>& signature)
    {
        /* Note: Signature size detection is less reliable due to compression.
         * With constant-time signing (ct=1), which is the default:
         * Falcon-512 signatures are exactly 809 bytes (CT).
         * Falcon-1024 signatures are exactly 1577 bytes (CT).
         * We use a threshold approach for detection. */

        size_t sigSize = signature.size();

        /* If signature is clearly in Falcon-1024 CT range */
        if(sigSize >= 1400)
        {
            return LLC::FalconVersion::FALCON_1024;
        }
        /* If signature is in Falcon-512 CT range */
        else if(sigSize >= 700 && sigSize <= 900)
        {
            return LLC::FalconVersion::FALCON_512;
        }
        else
        {
            throw std::invalid_argument("Cannot determine Falcon version from signature size: " + 
                                       std::to_string(sigSize));
        }
    }

} // namespace FalconVerify
} // namespace LLP
