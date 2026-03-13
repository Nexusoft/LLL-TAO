/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/session_recovery.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

namespace LLP
{
    namespace
    {
        bool HasRewardBinding(const MiningContext& context)
        {
            return context.fRewardBound && context.hashRewardAddress != 0;
        }

        bool HasRewardBinding(const SessionRecoveryData& data)
        {
            return data.fRewardBound && data.hashRewardAddress != 0;
        }

        bool HasChaCha20KeyState(const MiningContext& context)
        {
            return context.fEncryptionReady && !context.vChaChaKey.empty();
        }

        bool HasChaCha20KeyState(const SessionRecoveryData& data)
        {
            return data.fEncryptionReady && !data.vChaCha20Key.empty();
        }

        SessionRecoveryData ResolveRecoveredSnapshot(
            const MiningContext& liveContext,
            const SessionRecoveryData& recovered,
            const bool fAddressHintPath,
            bool& fAccepted
        )
        {
            SessionRecoveryData resolved = recovered;
            fAccepted = true;

            if(liveContext.hashKeyID != 0 && recovered.hashKeyID != 0 && liveContext.hashKeyID != recovered.hashKeyID)
            {
                debug::warning(FUNCTION, "RECOVERY CONFLICT: live Falcon identity=",
                               liveContext.hashKeyID.GetHex(), " differs from recovered identity=",
                               recovered.hashKeyID.GetHex(), ". Rejecting recovery.");
                fAccepted = false;
                return resolved;
            }

            if(liveContext.hashGenesis != 0 && recovered.hashGenesis != 0 &&
               liveContext.hashGenesis != recovered.hashGenesis)
            {
                debug::warning(FUNCTION, "RECOVERY CONFLICT: live genesis=", liveContext.hashGenesis.GetHex(),
                               " differs from recovered genesis=", recovered.hashGenesis.GetHex(),
                               ". Rejecting recovery.");
                fAccepted = false;
                return resolved;
            }

            if(liveContext.nSessionId != 0 && recovered.nSessionId != 0 &&
               liveContext.nSessionId != recovered.nSessionId)
            {
                debug::warning(FUNCTION, "RECOVERY CONFLICT: ",
                               fAddressHintPath ? "address-based" : "key-based",
                               " recovery found same Falcon identity but different session ids (live=",
                               liveContext.nSessionId, ", recovered=", recovered.nSessionId,
                               "). Preferring recovered canonical session id.");
            }

            if(HasRewardBinding(liveContext))
            {
                if(HasRewardBinding(recovered) && liveContext.hashRewardAddress != recovered.hashRewardAddress)
                {
                    debug::warning(FUNCTION, "RECOVERY CONFLICT: reward hash mismatch on reconnect (live=",
                                   liveContext.hashRewardAddress.GetHex(), ", recovered=",
                                   recovered.hashRewardAddress.GetHex(),
                                   "). Preserving live reward binding.");
                }

                resolved.hashRewardAddress = liveContext.hashRewardAddress;
                resolved.fRewardBound = true;
            }

            if(HasChaCha20KeyState(liveContext))
            {
                if(HasChaCha20KeyState(recovered) && liveContext.vChaChaKey != recovered.vChaCha20Key)
                {
                    debug::warning(FUNCTION, "RECOVERY CONFLICT: live session and recovered snapshot disagree on "
                                   "ChaCha20 state for keyID=", liveContext.hashKeyID.SubString(),
                                   ". Preserving live encryption context.");
                }

                resolved.vChaCha20Key = liveContext.vChaChaKey;
                resolved.fEncryptionReady = true;
                resolved.hashChaCha20Key = uint256_t(liveContext.vChaChaKey);
            }

            if(!liveContext.vDisposablePubKey.empty())
            {
                if(!recovered.vDisposablePubKey.empty() && liveContext.vDisposablePubKey != recovered.vDisposablePubKey)
                {
                    debug::warning(FUNCTION, "RECOVERY CONFLICT: live session and recovered snapshot disagree on "
                                   "disposable Falcon key for keyID=", liveContext.hashKeyID.SubString(),
                                   ". Preserving live key material.");
                }

                resolved.vDisposablePubKey = liveContext.vDisposablePubKey;
                if(liveContext.hashDisposableKeyID != 0)
                    resolved.hashDisposableKeyID = liveContext.hashDisposableKeyID;
            }

            return resolved;
        }
    }

    /** Default session timeout: 1 hour **/
    static const uint64_t DEFAULT_SESSION_TIMEOUT = 3600;

    /** Default max reconnects **/
    static const uint32_t DEFAULT_MAX_RECONNECTS = 10;


    /***************************************************************************
     * SessionRecoveryData Implementation
     **************************************************************************/

    /** Default Constructor **/
    MinerSessionContainer::MinerSessionContainer()
    : miner_id()
    , nSessionId(0)
    , hashKeyID(0)
    , hashGenesis(0)
    , nChannel(0)
    , nLastActivity(0)
    , nCreated(0)
    , vPubKey()
    , strAddress()
    , nReconnectCount(0)
    , fAuthenticated(false)
    , fFreshAuth(false)
    , nLastLane(0)
    , nProtocolLane(ProtocolLane::LEGACY)
    , hashRewardAddress(0)
    , fRewardBound(false)
    , vChaCha20Key()
    , fEncryptionReady(false)
    , hashChaCha20Key(0)
    , nChaCha20Nonce(0)
    , vDisposablePubKey()
    , hashDisposableKeyID(0)
    {
    }


    /** Constructor from MiningContext **/
    MinerSessionContainer::MinerSessionContainer(const MiningContext& context)
    : MinerSessionContainer()
    {
        /* nCreated is initialization-time metadata for the authoritative container.
         * It is set here exactly once; MergeContext() updates only live session fields. */
        nCreated = runtime::unifiedtimestamp();
        MergeContext(context);
    }


    /** MergeContext **/
    void MinerSessionContainer::MergeContext(const MiningContext& context)
    {
        if(!context.strAddress.empty())
        {
            strAddress = context.strAddress;
            miner_id = context.strAddress;
        }

        if(context.hashKeyID != 0)
        {
            hashKeyID = context.hashKeyID;
            miner_id = context.hashKeyID.GetHex();
        }

        if(context.hashGenesis != 0)
            hashGenesis = context.hashGenesis;

        if(context.nSessionId != 0)
            nSessionId = context.nSessionId;

        nChannel = context.nChannel;
        nLastActivity = context.nTimestamp;
        if(context.fAuthenticated)
            fAuthenticated = true;
        nProtocolLane = context.nProtocolLane;
        nLastLane = static_cast<uint8_t>(context.nProtocolLane);

        /* nReconnectCount is authoritative recovery metadata owned by the container.
         * Live SaveSession/UpdateSession refreshes often come from fresh connection snapshots
         * whose default MiningContext carries nReconnectCount=0. Preserve the existing counter
         * unless the caller explicitly supplies a recovered value to carry forward. */
        if(context.nReconnectCount != 0 || nReconnectCount == 0)
            nReconnectCount = context.nReconnectCount;

        if(!context.vMinerPubKey.empty())
            vPubKey = context.vMinerPubKey;

        if(!context.vDisposablePubKey.empty() || context.hashDisposableKeyID != 0)
        {
            if(!context.vDisposablePubKey.empty())
                vDisposablePubKey = context.vDisposablePubKey;

            if(context.hashDisposableKeyID != 0)
                hashDisposableKeyID = context.hashDisposableKeyID;
        }

        if(context.fRewardBound && context.hashRewardAddress != 0)
        {
            hashRewardAddress = context.hashRewardAddress;
            fRewardBound = true;
        }
        /* MergeContext never clears reward binding: the protocol only supports binding
         * a reward identity, not an explicit unbind/reset operation. */

        if(context.fEncryptionReady && !context.vChaChaKey.empty())
        {
            vChaCha20Key = context.vChaChaKey;
            fEncryptionReady = true;
            hashChaCha20Key = uint256_t(context.vChaChaKey);
        }
    }


    /** ToContext **/
    MiningContext MinerSessionContainer::ToContext() const
    {
        MiningContext context = MiningContext(
            nChannel,
            0,  /* nHeight - will be updated on reconnect */
            runtime::unifiedtimestamp(),
            strAddress,
            0,  /* nProtocolVersion - will be updated */
            fAuthenticated,
            nSessionId,
            hashKeyID,
            hashGenesis
        ).WithPubKey(vPubKey)
         .WithDisposableKey(vDisposablePubKey, hashDisposableKeyID)
         .WithReconnectCount(nReconnectCount)
         .WithProtocolLane(nProtocolLane);

        if(fRewardBound && hashRewardAddress != 0)
            context = context.WithRewardAddress(hashRewardAddress);

        if(fEncryptionReady && !vChaCha20Key.empty())
            context = context.WithChaChaKey(vChaCha20Key);

        return context;
    }

    SessionBinding MinerSessionContainer::GetSessionBinding() const
    {
        SessionBinding binding;
        binding.nSessionId = nSessionId;
        binding.hashGenesis = hashGenesis;
        binding.hashKeyID = hashKeyID;
        binding.hashRewardAddress = hashRewardAddress;
        binding.nProtocolLane = nProtocolLane;
        binding.strKeyFingerprint = Diagnostics::KeyFingerprint(vPubKey);
        binding.fRewardBound = fRewardBound;
        return binding;
    }

    CryptoContext MinerSessionContainer::GetCryptoContext() const
    {
        CryptoContext crypto;
        crypto.vChaCha20Key = vChaCha20Key;
        crypto.strKeyFingerprint = Diagnostics::KeyFingerprint(vChaCha20Key);
        crypto.nSessionId = nSessionId;
        crypto.hashGenesis = hashGenesis;
        crypto.hashKeyID = hashKeyID;
        crypto.nProtocolLane = nProtocolLane;
        crypto.fEncryptionReady = fEncryptionReady;
        return crypto;
    }

    SessionConsistencyResult MinerSessionContainer::ValidateConsistency() const
    {
        if(fAuthenticated && nSessionId == 0)
            return SessionConsistencyResult::MissingSessionId;

        if(fAuthenticated && hashGenesis == 0)
            return SessionConsistencyResult::MissingGenesis;

        if(fAuthenticated && hashKeyID == 0)
            return SessionConsistencyResult::MissingFalconKey;

        if(fRewardBound && hashRewardAddress == 0)
            return SessionConsistencyResult::RewardBoundMissingHash;

        if(fEncryptionReady && vChaCha20Key.empty())
            return SessionConsistencyResult::EncryptionReadyMissingKey;

        return SessionConsistencyResult::Ok;
    }


    /** IsExpired **/
    bool MinerSessionContainer::IsExpired(uint64_t nTimeoutSec) const
    {
        uint64_t nNow = runtime::unifiedtimestamp();
        return (nNow - nLastActivity) > nTimeoutSec;
    }


    /** UpdateActivity **/
    void MinerSessionContainer::UpdateActivity()
    {
        nLastActivity = runtime::unifiedtimestamp();
    }


    /***************************************************************************
     * SessionRecoveryManager Implementation
     **************************************************************************/

    /** Private constructor **/
    SessionRecoveryManager::SessionRecoveryManager()
    : mapSessionsByKey()
    , mapAddressToKey()
    , nSessionTimeout(DEFAULT_SESSION_TIMEOUT)
    , nMaxReconnects(DEFAULT_MAX_RECONNECTS)
    {
    }


    /** Get singleton **/
    SessionRecoveryManager& SessionRecoveryManager::Get()
    {
        static SessionRecoveryManager instance;
        return instance;
    }


    /** SaveSession **/
    bool SessionRecoveryManager::SaveSession(const MiningContext& context)
    {
        /* Only save authenticated sessions with valid key IDs */
        if(!context.fAuthenticated || context.hashKeyID == 0)
        {
            debug::log(3, FUNCTION, "Cannot save unauthenticated session");
            return false;
        }

        auto optExistingSession = mapSessionsByKey.Get(context.hashKeyID);
        SessionRecoveryData data;
        std::string strPreviousAddress;
        if(optExistingSession.has_value())
        {
            data = optExistingSession.value();
            strPreviousAddress = data.strAddress;
            data.MergeContext(context);
        }
        else
        {
            data = SessionRecoveryData(context);
        }

        /* Store by key ID */
        mapSessionsByKey.InsertOrUpdate(context.hashKeyID, data);

        /* Store address mapping if available */
        if(!context.strAddress.empty())
        {
            if(!strPreviousAddress.empty() && strPreviousAddress != context.strAddress)
                mapAddressToKey.Erase(strPreviousAddress);

            mapAddressToKey.InsertOrUpdate(context.strAddress, context.hashKeyID);
        }

        debug::log(2, FUNCTION, "SESSION RECOVERY SAVE");
        debug::log(2, FUNCTION, "  falcon_key_id=", context.hashKeyID.GetHex());
        debug::log(2, FUNCTION, "  session_id=", context.nSessionId);
        debug::log(2, FUNCTION, "  session_genesis=", context.GenesisHex());
        debug::log(2, FUNCTION, "  session_address=", context.strAddress);
        debug::log(2, FUNCTION, "  reward_binding_persisted_in_recovery_cache=",
                   (data.fRewardBound && data.hashRewardAddress != 0) ? "YES" : "NO");
        debug::log(2, FUNCTION, "  recovered_reward_hash=", Diagnostics::FullHexOrUnset(data.hashRewardAddress));
        debug::log(2, FUNCTION, "  chacha20_context_persisted_in_recovery_cache=",
                   (data.fEncryptionReady && !data.vChaCha20Key.empty()) ? "YES" : "NO");
        debug::log(2, FUNCTION, "  recovered_chacha20_key_hash=", Diagnostics::FullHexOrUnset(data.hashChaCha20Key));
        debug::log(2, FUNCTION, "  session_consistency=", SessionConsistencyResultString(data.ValidateConsistency()));

        return true;
    }


    /** MarkFreshAuth **/
    bool SessionRecoveryManager::MarkFreshAuth(const uint256_t& hashKeyID)
    {
        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
        {
            debug::log(2, FUNCTION, "No session found for keyID=", hashKeyID.SubString());
            return false;
        }

        SessionRecoveryData data = optData.value();
        data.fFreshAuth = true;
        mapSessionsByKey.Update(hashKeyID, data);

        debug::log(2, FUNCTION, "Marked session as fresh auth for keyID=", hashKeyID.GetHex());
        return true;
    }


    /** PreviewRecoverableSession **/
    std::optional<SessionRecoveryData> SessionRecoveryManager::PreviewRecoverableSession(const uint256_t& hashKeyID)
    {
        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
        {
            debug::log(2, FUNCTION, "No recoverable session for keyID=", hashKeyID.SubString());
            return std::nullopt;
        }

        SessionRecoveryData data = optData.value();
        if(data.IsExpired(nSessionTimeout.load()))
        {
            debug::log(2, FUNCTION, "Session expired for keyID=", hashKeyID.SubString());
            RemoveSession(hashKeyID);
            return std::nullopt;
        }

        if(data.nReconnectCount >= nMaxReconnects.load())
        {
            debug::log(0, FUNCTION, "Session exceeded max reconnects for keyID=", hashKeyID.SubString());
            RemoveSession(hashKeyID);
            return std::nullopt;
        }

        return data;
    }


    /** PeekSession **/
    std::optional<SessionRecoveryData> SessionRecoveryManager::PeekSession(const uint256_t& hashKeyID) const
    {
        /* PeekSession — diagnostic read-only access.
         * Unlike RecoverSession(), this NEVER mutates nReconnectCount or session state.
         * Safe for high-frequency callers (ColinAgent, diagnostics, monitoring). */
        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return std::nullopt;

        const SessionRecoveryData& data = optData.value();
        /* Return nullopt for expired or over-limit sessions without mutating state */
        if(data.IsExpired(nSessionTimeout.load()))
            return std::nullopt;
        if(data.nReconnectCount >= nMaxReconnects.load())
            return std::nullopt;

        return data;
    }


    /** RecoverSession **/
    bool SessionRecoveryManager::RecoverSession(const uint256_t& hashKeyID, MiningContext& context)
    {
        auto optData = PreviewRecoverableSession(hashKeyID);
        if(!optData.has_value())
            return false;

        /* Increment reconnect count and update activity */
        SessionRecoveryData data = optData.value();
        data.nReconnectCount++;
        data.UpdateActivity();
        mapSessionsByKey.Update(hashKeyID, data);

        /* Restore context */
        context = data.ToContext();

        debug::log(0, FUNCTION, "SESSION RECOVERY RESTORE");
        debug::log(0, FUNCTION, "  falcon_key_id=", hashKeyID.GetHex());
        debug::log(0, FUNCTION, "  restored_session_id=", context.nSessionId);
        debug::log(0, FUNCTION, "  restored_session_genesis=", context.GenesisHex());
        debug::log(0, FUNCTION, "  reconnect_count=", data.nReconnectCount);
        debug::log(0, FUNCTION, "  fresh_auth_flag=", (data.fFreshAuth ? "YES" : "NO"));
        debug::log(0, FUNCTION, "  reward_binding_restored_from_recovery_cache=",
                   (data.fRewardBound && data.hashRewardAddress != 0) ? "YES" : "NO");
        debug::log(0, FUNCTION, "  restored_reward_hash=", Diagnostics::FullHexOrUnset(data.hashRewardAddress));
        debug::log(0, FUNCTION, "  restored_chacha20_key_hash=", Diagnostics::FullHexOrUnset(data.hashChaCha20Key));
        debug::log(0, FUNCTION, "  session_consistency=", SessionConsistencyResultString(data.ValidateConsistency()));

        return true;
    }


    /** RecoverSessionByAddress **/
    bool SessionRecoveryManager::RecoverSessionByAddress(const std::string& strAddress, MiningContext& context)
    {
        auto optKeyID = mapAddressToKey.Get(strAddress);
        if(!optKeyID.has_value())
        {
            debug::log(3, FUNCTION, "No session mapping for address=", strAddress);
            return false;
        }

        return RecoverSession(optKeyID.value(), context);
    }


    /** RecoverSessionByAddress (data) **/
    std::optional<SessionRecoveryData> SessionRecoveryManager::RecoverSessionByAddress(const std::string& strAddress)
    {
        auto optKeyID = mapAddressToKey.Get(strAddress);
        if(!optKeyID.has_value())
        {
            debug::log(3, FUNCTION, "No session mapping for address=", strAddress);
            return std::nullopt;
        }

        return PreviewRecoverableSession(optKeyID.value());
    }


    /** RecoverSessionByIdentity **/
    std::optional<SessionRecoveryData> SessionRecoveryManager::RecoverSessionByIdentity(
        const uint256_t& hashKeyID,
        const std::string& strAddress
    )
    {
        MiningContext context = MiningContext().WithKeyId(hashKeyID);
        context.strAddress = strAddress;
        return RecoverSessionByIdentity(context);
    }


    /** RecoverSessionByIdentity **/
    std::optional<SessionRecoveryData> SessionRecoveryManager::RecoverSessionByIdentity(const MiningContext& liveContext)
    {
        if(liveContext.hashKeyID != 0)
        {
            const auto optPreview = PreviewRecoverableSession(liveContext.hashKeyID);
            if(optPreview.has_value())
            {
                bool fAccepted = false;
                /* Validate conflicts against the authoritative snapshot before consuming a reconnect slot. */
                ResolveRecoveredSnapshot(liveContext, optPreview.value(), false, fAccepted);
                if(!fAccepted)
                    return std::nullopt;

                MiningContext recoveredContext;
                if(RecoverSession(liveContext.hashKeyID, recoveredContext))
                {
                    SessionRecoveryData resolved =
                        ResolveRecoveredSnapshot(liveContext, SessionRecoveryData(recoveredContext), false, fAccepted);
                    return fAccepted ? std::optional<SessionRecoveryData>(resolved) : std::nullopt;
                }
            }

            debug::log(3, FUNCTION, "No recoverable session for keyID=", liveContext.hashKeyID.SubString(),
                       " falling back to address hint");
        }

        if(liveContext.strAddress.empty())
            return std::nullopt;

        const auto optPreview = RecoverSessionByAddress(liveContext.strAddress);
        if(!optPreview.has_value())
            return std::nullopt;

        bool fAccepted = false;
        /* Validate conflicts against the authoritative snapshot before consuming a reconnect slot. */
        ResolveRecoveredSnapshot(liveContext, optPreview.value(), true, fAccepted);
        if(!fAccepted)
            return std::nullopt;

        MiningContext recoveredContext;
        if(!RecoverSession(optPreview->hashKeyID, recoveredContext))
            return std::nullopt;

        SessionRecoveryData resolved =
            ResolveRecoveredSnapshot(liveContext, SessionRecoveryData(recoveredContext), true, fAccepted);
        return fAccepted ? std::optional<SessionRecoveryData>(resolved) : std::nullopt;
    }


    /** RemoveSession **/
    bool SessionRecoveryManager::RemoveSession(const uint256_t& hashKeyID)
    {
        auto optData = mapSessionsByKey.GetAndRemove(hashKeyID);
        if(!optData.has_value())
            return false;

        /* Also remove address mapping */
        if(!optData.value().strAddress.empty())
        {
            mapAddressToKey.Erase(optData.value().strAddress);
        }

        debug::log(2, FUNCTION, "Removed session for keyID=", hashKeyID.SubString());
        return true;
    }


    /** UpdateSession **/
    bool SessionRecoveryManager::UpdateSession(const MiningContext& context)
    {
        if(context.hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(context.hashKeyID);
        if(!optData.has_value())
            return false;

        SessionRecoveryData data = optData.value();
        const std::string strPreviousAddress = data.strAddress;
        data.MergeContext(context);

        mapSessionsByKey.Update(context.hashKeyID, data);

        if(!context.strAddress.empty())
        {
            if(!strPreviousAddress.empty() && strPreviousAddress != context.strAddress)
                mapAddressToKey.Erase(strPreviousAddress);

            mapAddressToKey.InsertOrUpdate(context.strAddress, context.hashKeyID);
        }

        return true;
    }


    /** SaveChaCha20State **/
    bool SessionRecoveryManager::SaveChaCha20State(
        const uint256_t& hashKeyID,
        const uint256_t& hashKey,
        uint64_t nNonce
    )
    {
        if(hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        SessionRecoveryData data = optData.value();
        data.hashChaCha20Key = hashKey;
        /* vChaCha20Key is the authoritative recovery copy for lane-switch restore.
         * hashChaCha20Key is kept as the legacy diagnostic/compatibility mirror. */
        data.vChaCha20Key = hashKey.GetBytes();
        data.fEncryptionReady = (hashKey != 0);
        data.nChaCha20Nonce = nNonce;
        mapSessionsByKey.Update(hashKeyID, data);

        return true;
    }


    /** RestoreChaCha20State **/
    bool SessionRecoveryManager::RestoreChaCha20State(
        const uint256_t& hashKeyID,
        uint256_t& hashKey,
        uint64_t& nNonce
    )
    {
        if(hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        const SessionRecoveryData& data = optData.value();
        /* Prefer the authoritative byte container. Fall back to the legacy hash mirror
         * for older cached sessions that predate containerized ChaCha20 persistence. */
        if(!data.vChaCha20Key.empty() && data.hashChaCha20Key == 0)
        {
            debug::warning(FUNCTION, "ChaCha20 recovery container missing mirrored hash for keyID=",
                           hashKeyID.SubString());
        }
        else if(!data.vChaCha20Key.empty() && data.hashChaCha20Key != 0)
        {
            const uint256_t hashKeyFromBytes(data.vChaCha20Key);
            if(hashKeyFromBytes != data.hashChaCha20Key)
            {
                debug::warning(FUNCTION, "ChaCha20 recovery container mismatch for keyID=",
                               hashKeyID.SubString(), " authoritative_bytes=",
                               hashKeyFromBytes.SubString(), " mirrored_hash=",
                               data.hashChaCha20Key.SubString());
            }
        }

        if(!data.vChaCha20Key.empty())
            hashKey = uint256_t(data.vChaCha20Key);
        else
            hashKey = data.hashChaCha20Key;

        nNonce = data.nChaCha20Nonce;
        return data.fEncryptionReady && hashKey != 0;
    }


    /** SaveDisposableKey **/
    bool SessionRecoveryManager::SaveDisposableKey(
        const uint256_t& hashKeyID,
        const std::vector<uint8_t>& vPubKey,
        const uint256_t& hashDisposableKeyID
    )
    {
        if(hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        SessionRecoveryData data = optData.value();
        data.MergeContext(
            data.ToContext()
                .WithTimestamp(data.nLastActivity)
                .WithDisposableKey(vPubKey, hashDisposableKeyID)
        );
        mapSessionsByKey.Update(hashKeyID, data);

        return true;
    }


    /** RestoreDisposableKey **/
    bool SessionRecoveryManager::RestoreDisposableKey(
        const uint256_t& hashKeyID,
        std::vector<uint8_t>& vPubKey,
        uint256_t& hashDisposableKeyID
    )
    {
        if(hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        vPubKey = optData.value().vDisposablePubKey;
        hashDisposableKeyID = optData.value().hashDisposableKeyID;
        return !vPubKey.empty();
    }


    /** UpdateLane **/
    bool SessionRecoveryManager::UpdateLane(
        const uint256_t& hashKeyID,
        uint8_t nNewLane
    )
    {
        if(hashKeyID == 0)
            return false;

        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        SessionRecoveryData data = optData.value();
        data.MergeContext(
            data.ToContext()
                .WithTimestamp(data.nLastActivity)
                .WithProtocolLane(nNewLane == 0 ? ProtocolLane::LEGACY : ProtocolLane::STATELESS)
        );
        mapSessionsByKey.Update(hashKeyID, data);
        return true;
    }


    /** HasSession **/
    bool SessionRecoveryManager::HasSession(const uint256_t& hashKeyID) const
    {
        auto optData = mapSessionsByKey.Get(hashKeyID);
        if(!optData.has_value())
            return false;

        return !optData.value().IsExpired(nSessionTimeout.load());
    }


    /** CleanupExpired **/
    uint32_t SessionRecoveryManager::CleanupExpired(uint64_t nTimeoutSec)
    {
        uint32_t nRemoved = 0;
        auto pairs = mapSessionsByKey.GetAllPairs();

        for(const auto& pair : pairs)
        {
            if(pair.second.IsExpired(nTimeoutSec))
            {
                if(RemoveSession(pair.first))
                    ++nRemoved;
            }
        }

        if(nRemoved > 0)
        {
            debug::log(2, FUNCTION, "Cleaned up ", nRemoved, " expired sessions");
        }

        return nRemoved;
    }


    /** GetSessionCount **/
    size_t SessionRecoveryManager::GetSessionCount() const
    {
        return mapSessionsByKey.Size();
    }


    /** GetSessionTimeout **/
    uint64_t SessionRecoveryManager::GetSessionTimeout() const
    {
        return nSessionTimeout.load();
    }


    /** SetSessionTimeout **/
    void SessionRecoveryManager::SetSessionTimeout(uint64_t nTimeoutSec)
    {
        nSessionTimeout.store(nTimeoutSec);
    }


    /** GetMaxReconnects **/
    uint32_t SessionRecoveryManager::GetMaxReconnects() const
    {
        return nMaxReconnects.load();
    }


    /** SetMaxReconnects **/
    void SessionRecoveryManager::SetMaxReconnects(uint32_t nMax)
    {
        nMaxReconnects.store(nMax);
    }

} // namespace LLP
