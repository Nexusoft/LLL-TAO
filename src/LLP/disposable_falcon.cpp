/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/disposable_falcon.h>

#include <LLC/hash/SK.h>
#include <LLC/include/flkey.h>
#include <LLC/include/random.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/hex.h>
#include <Util/include/args.h>
#include <Util/include/config.h>

#include <sstream>
#include <iomanip>
#include <mutex>

namespace LLP
{
namespace DisposableFalcon
{
    /* Constants for timestamp validation */
    static const uint64_t TIMESTAMP_FUTURE_TOLERANCE_SEC = 60;   // Allow up to 60 seconds in the future
    static const uint64_t TIMESTAMP_PAST_TOLERANCE_SEC = 3600;   // Allow up to 1 hour in the past

    /*******************************************************************************
     * SignedWorkSubmission Implementation
     *******************************************************************************/

    /* Default constructor */
    SignedWorkSubmission::SignedWorkSubmission()
    : hashMerkleRoot(0)
    , nNonce(0)
    , vSignature()
    , hashKeyID(0)
    , nTimestamp(0)
    , fSigned(false)
    {
    }


    /* Constructor with work data */
    SignedWorkSubmission::SignedWorkSubmission(const uint512_t& hashMerkle, uint64_t nNonce_)
    : hashMerkleRoot(hashMerkle)
    , nNonce(nNonce_)
    , vSignature()
    , hashKeyID(0)
    , nTimestamp(runtime::unifiedtimestamp())
    , fSigned(false)
    {
    }


    /* Get message bytes for signing */
    std::vector<uint8_t> SignedWorkSubmission::GetMessageBytes() const
    {
        std::vector<uint8_t> vMessage;

        /* Add merkle root (64 bytes) */
        std::vector<uint8_t> vMerkle = hashMerkleRoot.GetBytes();
        vMessage.insert(vMessage.end(), vMerkle.begin(), vMerkle.end());

        /* Add nonce (8 bytes, little-endian) */
        for(size_t i = 0; i < 8; ++i)
        {
            vMessage.push_back(static_cast<uint8_t>((nNonce >> (i * 8)) & 0xFF));
        }

        /* Add timestamp (8 bytes, little-endian) for replay protection */
        for(size_t i = 0; i < 8; ++i)
        {
            vMessage.push_back(static_cast<uint8_t>((nTimestamp >> (i * 8)) & 0xFF));
        }

        return vMessage;
    }


    /* Serialize the signed submission */
    std::vector<uint8_t> SignedWorkSubmission::Serialize() const
    {
        std::vector<uint8_t> vData;

        /* Add merkle root (64 bytes) */
        std::vector<uint8_t> vMerkle = hashMerkleRoot.GetBytes();
        vData.insert(vData.end(), vMerkle.begin(), vMerkle.end());

        /* Add nonce (8 bytes, little-endian) */
        for(size_t i = 0; i < 8; ++i)
        {
            vData.push_back(static_cast<uint8_t>((nNonce >> (i * 8)) & 0xFF));
        }

        /* Add timestamp (8 bytes, little-endian) */
        for(size_t i = 0; i < 8; ++i)
        {
            vData.push_back(static_cast<uint8_t>((nTimestamp >> (i * 8)) & 0xFF));
        }

        /* Add signature length (2 bytes, big-endian) */
        uint16_t nSigLen = static_cast<uint16_t>(vSignature.size());
        vData.push_back(static_cast<uint8_t>(nSigLen & 0xFF));  // Low byte first
        vData.push_back(static_cast<uint8_t>(nSigLen >> 8));  // High byte second

        /* Add signature */
        vData.insert(vData.end(), vSignature.begin(), vSignature.end());

        return vData;
    }


    /* Deserialize from bytes */
    bool SignedWorkSubmission::Deserialize(const std::vector<uint8_t>& vData)
    {
        /* Minimum size: merkle(64) + nonce(8) + timestamp(8) + sig_len(2) = 82 bytes */
        const size_t MIN_SIZE = 82;
        if(vData.size() < MIN_SIZE)
        {
            DebugLogDeserialize("SignedWorkSubmission", 0, MIN_SIZE, vData.size(), 2);
            return false;
        }

        size_t nOffset = 0;

        /* Parse merkle root (64 bytes) */
        DebugLogDeserialize("merkle_root", nOffset, 64, vData.size());
        hashMerkleRoot.SetBytes(std::vector<uint8_t>(vData.begin() + nOffset, vData.begin() + nOffset + 64));
        nOffset += 64;

        /* Parse nonce (8 bytes, little-endian) */
        DebugLogDeserialize("nonce", nOffset, 8, vData.size());
        nNonce = 0;
        for(size_t i = 0; i < 8; ++i)
        {
            nNonce |= static_cast<uint64_t>(vData[nOffset + i]) << (i * 8);
        }
        nOffset += 8;

        /* Parse timestamp (8 bytes, little-endian) */
        DebugLogDeserialize("timestamp", nOffset, 8, vData.size());
        nTimestamp = 0;
        for(size_t i = 0; i < 8; ++i)
        {
            nTimestamp |= static_cast<uint64_t>(vData[nOffset + i]) << (i * 8);
        }
        nOffset += 8;

        // AFTER (Little-Endian - matches Serialize and NexusMiner)
            uint16_t nSigLen = static_cast<uint16_t>(vData[nOffset]) |
            (static_cast<uint16_t>(vData[nOffset + 1]) << 8);
        nOffset += 2;

        /* Validate remaining data for signature */
        if(vData.size() < nOffset + nSigLen)
        {
            DebugLogDeserialize("signature", nOffset, nSigLen, vData.size(), 2);
            return false;
        }

        /* Parse signature */
        DebugLogDeserialize("signature", nOffset, nSigLen, vData.size());
        vSignature.assign(vData.begin() + nOffset, vData.begin() + nOffset + nSigLen);

        fSigned = (nSigLen > 0);
        return true;
    }


    /* Check validity */
    bool SignedWorkSubmission::IsValid() const
    {
        /* Merkle root must be non-zero */
        if(hashMerkleRoot == 0)
            return false;

        /* Timestamp must be reasonable (within tolerance windows) */
        uint64_t nNow = runtime::unifiedtimestamp();
        if(nTimestamp > nNow + TIMESTAMP_FUTURE_TOLERANCE_SEC ||
           nTimestamp < nNow - TIMESTAMP_PAST_TOLERANCE_SEC)
            return false;

        return true;
    }


    /* Debug string */
    std::string SignedWorkSubmission::DebugString() const
    {
        std::ostringstream oss;
        oss << "SignedWorkSubmission{"
            << "merkle=" << hashMerkleRoot.SubString()
            << ", nonce=" << nNonce
            << ", timestamp=" << nTimestamp
            << ", sig_len=" << vSignature.size()
            << ", signed=" << (fSigned ? "true" : "false")
            << ", keyId=" << hashKeyID.SubString()
            << "}";
        return oss.str();
    }


    /*******************************************************************************
     * WrapperResult Implementation
     *******************************************************************************/

    WrapperResult WrapperResult::Success(const SignedWorkSubmission& sub)
    {
        WrapperResult result;
        result.fSuccess = true;
        result.strError = "";
        result.submission = sub;
        result.hashKeyID = sub.hashKeyID;
        return result;
    }

    WrapperResult WrapperResult::Success(const SignedWorkSubmission& sub, const uint256_t& keyId)
    {
        WrapperResult result;
        result.fSuccess = true;
        result.strError = "";
        result.submission = sub;
        result.hashKeyID = keyId;
        return result;
    }

    WrapperResult WrapperResult::Failure(const std::string& error)
    {
        WrapperResult result;
        result.fSuccess = false;
        result.strError = error;
        result.submission = SignedWorkSubmission();
        result.hashKeyID = 0;
        return result;
    }


    /*******************************************************************************
     * DisposableFalconWrapperImpl - Implementation Class
     *******************************************************************************/

    class DisposableFalconWrapperImpl : public IDisposableFalconWrapper
    {
    private:
        /* Session-specific ephemeral Falcon key */
        LLC::FLKey sessionKey;

        /* Session identifier */
        uint256_t hashSessionId;

        /* Derived key ID from public key */
        uint256_t hashKeyID;

        /* Flag indicating active session */
        bool fHasSession;

        /* Mutex for thread safety */
        mutable std::mutex MUTEX;

    public:
        DisposableFalconWrapperImpl()
        : sessionKey()
        , hashSessionId(0)
        , hashKeyID(0)
        , fHasSession(false)
        , MUTEX()
        {
        }

        ~DisposableFalconWrapperImpl() override
        {
            ClearSession();
        }

        /* Generate ephemeral session key */
        bool GenerateSessionKey(const uint256_t& hashSession) override
        {
            std::lock_guard<std::mutex> lock(MUTEX);

            debug::log(2, FUNCTION, "Generating disposable Falcon session key for session ",
                       hashSession.SubString());

            /* Generate new Falcon keypair */
            sessionKey.MakeNewKey();

            if(!sessionKey.IsValid())
            {
                debug::error(FUNCTION, "Failed to generate disposable Falcon key");
                return false;
            }

            hashSessionId = hashSession;

            /* Derive key ID from public key */
            std::vector<uint8_t> vPubKey = sessionKey.GetPubKey();
            hashKeyID = LLC::SK256(vPubKey);

            fHasSession = true;

            debug::log(1, FUNCTION, "Disposable Falcon session key generated, keyId=",
                       hashKeyID.SubString(), " pubkey_len=", vPubKey.size());

            return true;
        }


        /* Wrap work submission with disposable signature */
        WrapperResult WrapWorkSubmission(
            const uint512_t& hashMerkleRoot,
            uint64_t nNonce
        ) override
        {
            std::lock_guard<std::mutex> lock(MUTEX);

            if(!fHasSession)
            {
                return WrapperResult::Failure("No active session key - call GenerateSessionKey first");
            }

            if(!sessionKey.IsValid())
            {
                return WrapperResult::Failure("Session key is invalid");
            }

            /* Create submission */
            SignedWorkSubmission submission(hashMerkleRoot, nNonce);
            submission.hashKeyID = hashKeyID;

            /* Get message bytes to sign */
            std::vector<uint8_t> vMessage = submission.GetMessageBytes();

            debug::log(3, FUNCTION, "Wrapping work submission: merkle=", hashMerkleRoot.SubString(),
                       " nonce=", nNonce, " message_len=", vMessage.size());

            /* Debug log the message bytes */
            DebugLogPacket("WrapWorkSubmission::message", vMessage, 4);

            /* Sign with disposable Falcon key */
            if(!sessionKey.Sign(vMessage, submission.vSignature))
            {
                return WrapperResult::Failure("Failed to sign work submission with disposable key");
            }

            submission.fSigned = true;

            debug::log(2, FUNCTION, "Work submission wrapped successfully, sig_len=",
                       submission.vSignature.size());

            return WrapperResult::Success(submission);
        }


        /* Unwrap and verify signed work submission */
        WrapperResult UnwrapWorkSubmission(
            const std::vector<uint8_t>& vData,
            const std::vector<uint8_t>& vPubKey
        ) override
        {
            /* Lock not needed for verification - stateless operation */

            debug::log(3, FUNCTION, "Unwrapping work submission, data_len=", vData.size(),
                       " pubkey_len=", vPubKey.size());

            /* Debug log incoming packet */
            DebugLogPacket("UnwrapWorkSubmission::input", vData, 3);

            /* Deserialize the submission */
            SignedWorkSubmission submission;
            if(!submission.Deserialize(vData))
            {
                return WrapperResult::Failure("Failed to deserialize signed work submission");
            }

            /* Validate structure */
            if(!submission.IsValid())
            {
                return WrapperResult::Failure("Invalid work submission structure");
            }

            /* If not signed, return the data without verification */
            if(!submission.fSigned || submission.vSignature.empty())
            {
                debug::log(2, FUNCTION, "Work submission is not signed - proceeding without verification");
                return WrapperResult::Success(submission);
            }

            /* Set up Falcon key for verification */
            LLC::FLKey verifyKey;
            if(!verifyKey.SetPubKey(vPubKey))
            {
                return WrapperResult::Failure("Failed to set public key for verification");
            }

            /* Get message bytes */
            std::vector<uint8_t> vMessage = submission.GetMessageBytes();

            /* Debug log verification data */
            DebugLogPacket("UnwrapWorkSubmission::message", vMessage, 4);
            DebugLogPacket("UnwrapWorkSubmission::signature", submission.vSignature, 4);

            /* Verify disposable Falcon signature */
            if(!verifyKey.Verify(vMessage, submission.vSignature))
            {
                debug::log(0, FUNCTION, "Disposable Falcon signature verification FAILED");
                return WrapperResult::Failure("Signature verification failed");
            }

            /* Derive key ID from public key */
            uint256_t verifiedKeyId = LLC::SK256(vPubKey);
            submission.hashKeyID = verifiedKeyId;

            debug::log(2, FUNCTION, "Work submission unwrapped and verified, keyId=",
                       verifiedKeyId.SubString());

            return WrapperResult::Success(submission, verifiedKeyId);
        }


        /* Get session key ID */
        uint256_t GetSessionKeyId() const override
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            return fHasSession ? hashKeyID : uint256_t(0);
        }


        /* Get session public key */
        std::vector<uint8_t> GetSessionPubKey() const override
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            if(!fHasSession || !sessionKey.IsValid())
                return std::vector<uint8_t>();

            return sessionKey.GetPubKey();
        }


        /* Check if session is active */
        bool HasActiveSession() const override
        {
            std::lock_guard<std::mutex> lock(MUTEX);
            return fHasSession && sessionKey.IsValid();
        }


        /* Clear session */
        void ClearSession() override
        {
            std::lock_guard<std::mutex> lock(MUTEX);

            debug::log(2, FUNCTION, "Clearing disposable Falcon session");

            sessionKey.Reset();
            hashSessionId = 0;
            hashKeyID = 0;
            fHasSession = false;
        }
    };


    /*******************************************************************************
     * Factory Function
     *******************************************************************************/

    std::unique_ptr<IDisposableFalconWrapper> Create()
    {
        return std::make_unique<DisposableFalconWrapperImpl>();
    }


    /*******************************************************************************
     * Debug Logging Functions
     *******************************************************************************/

    void DebugLogPacket(
        const std::string& strContext,
        const std::vector<uint8_t>& vData,
        uint32_t nVerbose
    )
    {
        if(config::nVerbose < nVerbose)
            return;

        std::ostringstream oss;
        oss << strContext << " [" << vData.size() << " bytes]: ";

        /* Show first 64 bytes as hex */
        size_t nShow = std::min<size_t>(64, vData.size());
        for(size_t i = 0; i < nShow; ++i)
        {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<uint32_t>(vData[i]);
        }
        if(vData.size() > 64)
            oss << "...";

        debug::log(nVerbose, FUNCTION, oss.str());
    }


    void DebugLogDeserialize(
        const std::string& strField,
        size_t nOffset,
        size_t nSize,
        size_t nTotalSize,
        uint32_t nVerbose
    )
    {
        if(config::nVerbose < nVerbose)
            return;

        std::ostringstream oss;
        oss << "Deserialize[" << strField << "] offset=" << nOffset
            << " size=" << nSize << " total=" << nTotalSize;

        if(nOffset + nSize > nTotalSize)
            oss << " *** OVERFLOW ***";

        debug::log(nVerbose, FUNCTION, oss.str());
    }


} // namespace DisposableFalcon
} // namespace LLP
