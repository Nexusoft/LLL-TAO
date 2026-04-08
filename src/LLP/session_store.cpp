/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/session_store.h>
#include <LLP/include/mining_timers.h>

#include <Util/include/runtime.h>
#include <Util/include/debug.h>

#include <algorithm>


namespace LLP
{

    /* ════════════════════════════════════════════════════════════════════════════
     *  CanonicalSession
     * ════════════════════════════════════════════════════════════════════════════ */

    CanonicalSession::CanonicalSession(
        const uint256_t& hashKeyID_,
        const uint256_t& hashGenesis_,
        const std::string& strAddress_)
    : hashKeyID(hashKeyID_)
    , nSessionId(MiningContext::DeriveSessionId(hashKeyID_))
    , hashGenesis(hashGenesis_)
    , strAddress(strAddress_)
    , strIP(DeriveIP(strAddress_))
    {
    }


    MiningContext CanonicalSession::ToMiningContext() const
    {
        MiningContext ctx;

        /* Identity */
        ctx.nSessionId        = nSessionId;
        ctx.hashKeyID         = hashKeyID;
        ctx.hashGenesis       = hashGenesis;
        ctx.strUserName       = strUserName;

        /* Connection */
        ctx.strAddress        = strAddress;
        ctx.nProtocolLane     = nProtocolLane;

        /* Lifecycle */
        ctx.nSessionState     = nSessionState;
        ctx.fAuthenticated    = fAuthenticated;
        ctx.fEncryptionReady  = fEncryptionReady;
        ctx.nSessionStart     = nSessionStart;
        ctx.nTimestamp         = nTimestamp;

        /* Channel / Mining */
        ctx.nChannel          = nChannel;
        ctx.strChannelName    = strChannelName;
        ctx.nHeight           = nHeight;
        ctx.nLastTemplateUnifiedHeight = nLastTemplateUnifiedHeight;
        ctx.hashLastBlock     = hashLastBlock;
        ctx.canonical_snap    = canonical_snap;
        ctx.nStakeHeight      = nStakeHeight;
        ctx.nMinerPrevblockSuffix = nMinerPrevblockSuffix;

        /* Auth / Crypto */
        ctx.vAuthNonce        = vAuthNonce;
        ctx.vMinerPubKey      = vMinerPubKey;
        ctx.nFalconVersion    = nFalconVersion;
        ctx.fFalconVersionDetected = fFalconVersionDetected;
        ctx.vChaChaKey        = vChaChaKey;
        ctx.vDisposablePubKey = vDisposablePubKey;
        ctx.hashDisposableKeyID = hashDisposableKeyID;

        /* Reward */
        ctx.hashRewardAddress = hashRewardAddress;
        ctx.fRewardBound      = fRewardBound;

        /* Notifications */
        ctx.fSubscribedToNotifications = fSubscribedToNotifications;
        ctx.nSubscribedChannel         = nSubscribedChannel;
        ctx.nLastNotificationTime      = nLastNotificationTime;
        ctx.nNotificationsSent         = nNotificationsSent;

        /* Keepalive */
        ctx.nKeepaliveCount    = nKeepaliveCount;
        ctx.nKeepaliveSent     = nKeepaliveSent;
        ctx.nLastKeepaliveTime = nLastKeepaliveTime;

        /* Recovery */
        ctx.nReconnectCount = nReconnectCount;

        /* Protocol */
        ctx.nProtocolVersion = nProtocolVersion;

        return ctx;
    }


    /* static */
    CanonicalSession CanonicalSession::FromMiningContext(const MiningContext& ctx)
    {
        CanonicalSession s;

        /* Identity */
        s.hashKeyID         = ctx.hashKeyID;
        s.nSessionId        = ctx.nSessionId;
        s.hashGenesis       = ctx.hashGenesis;
        s.strUserName       = ctx.strUserName;

        /* Connection */
        s.strAddress        = ctx.strAddress;
        s.strIP             = DeriveIP(ctx.strAddress);
        s.nProtocolLane     = ctx.nProtocolLane;
        s.fStatelessLive    = (ctx.nProtocolLane == ProtocolLane::STATELESS);
        s.fLegacyLive       = (ctx.nProtocolLane == ProtocolLane::LEGACY);

        /* Lifecycle */
        s.nSessionState     = ctx.nSessionState;
        s.fAuthenticated    = ctx.fAuthenticated;
        s.fEncryptionReady  = ctx.fEncryptionReady;
        s.nSessionStart     = ctx.nSessionStart;
        s.nLastActivity     = ctx.nTimestamp;
        s.nTimestamp         = ctx.nTimestamp;

        /* Channel / Mining */
        s.nChannel          = ctx.nChannel;
        s.strChannelName    = ctx.strChannelName;
        s.nHeight           = ctx.nHeight;
        s.nLastTemplateUnifiedHeight = ctx.nLastTemplateUnifiedHeight;
        s.hashLastBlock     = ctx.hashLastBlock;
        s.canonical_snap    = ctx.canonical_snap;
        s.nStakeHeight      = ctx.nStakeHeight;
        s.nMinerPrevblockSuffix = ctx.nMinerPrevblockSuffix;

        /* Auth / Crypto */
        s.vAuthNonce        = ctx.vAuthNonce;
        s.vMinerPubKey      = ctx.vMinerPubKey;
        s.nFalconVersion    = ctx.nFalconVersion;
        s.fFalconVersionDetected = ctx.fFalconVersionDetected;
        s.vChaChaKey        = ctx.vChaChaKey;
        s.vDisposablePubKey = ctx.vDisposablePubKey;
        s.hashDisposableKeyID = ctx.hashDisposableKeyID;

        /* Reward */
        s.hashRewardAddress = ctx.hashRewardAddress;
        s.fRewardBound      = ctx.fRewardBound;

        /* Notifications */
        s.fSubscribedToNotifications = ctx.fSubscribedToNotifications;
        s.nSubscribedChannel         = ctx.nSubscribedChannel;
        s.nLastNotificationTime      = ctx.nLastNotificationTime;
        s.nNotificationsSent         = ctx.nNotificationsSent;

        /* Keepalive */
        s.nKeepaliveCount    = ctx.nKeepaliveCount;
        s.nKeepaliveSent     = ctx.nKeepaliveSent;
        s.nLastKeepaliveTime = ctx.nLastKeepaliveTime;

        /* Recovery */
        s.nReconnectCount = ctx.nReconnectCount;

        /* Protocol */
        s.nProtocolVersion = ctx.nProtocolVersion;

        return s;
    }


    SessionBinding CanonicalSession::GetSessionBinding() const
    {
        SessionBinding binding;
        binding.nSessionId       = nSessionId;
        binding.hashGenesis      = hashGenesis;
        binding.hashKeyID        = hashKeyID;
        binding.hashRewardAddress = hashRewardAddress;
        binding.nProtocolLane    = nProtocolLane;
        binding.fRewardBound     = fRewardBound;

        /* Fingerprint: first 8 hex chars of hashKeyID */
        if (hashKeyID != 0)
            binding.strKeyFingerprint = hashKeyID.ToString().substr(0, 8);

        return binding;
    }


    CryptoContext CanonicalSession::GetCryptoContext() const
    {
        CryptoContext crypto;
        crypto.vChaCha20Key    = vChaChaKey;
        crypto.nSessionId      = nSessionId;
        crypto.hashGenesis     = hashGenesis;
        crypto.hashKeyID       = hashKeyID;
        crypto.nProtocolLane   = nProtocolLane;
        crypto.fEncryptionReady = fEncryptionReady;

        /* Fingerprint: first 8 hex chars of hashKeyID */
        if (hashKeyID != 0)
            crypto.strKeyFingerprint = hashKeyID.ToString().substr(0, 8);

        return crypto;
    }


    bool CanonicalSession::IsRecoverable(uint64_t nTimeoutSec, uint64_t nNow) const
    {
        if (!fSavedForRecovery)
            return false;

        if (nDisconnectTime == 0)
            return false;

        return (nNow < nDisconnectTime + nTimeoutSec);
    }


    bool CanonicalSession::IsExpired(uint64_t nTimeoutSec, uint64_t nNow) const
    {
        if (AnyPortLive())
            return false;

        if (nNow == 0)
            nNow = runtime::unifiedtimestamp();

        /* Not expired if recoverable */
        if (IsRecoverable(nTimeoutSec, nNow))
            return false;

        return (nLastActivity + nTimeoutSec < nNow);
    }


    bool CanonicalSession::IsConsideredInactive(uint64_t nNow, uint64_t nTimeoutSec) const
    {
        /* Recent activity within timeout → not inactive */
        if (nLastActivity != 0 && (nNow - nLastActivity) < nTimeoutSec)
            return false;

        /* Recent keepalive within grace period → not inactive */
        if (nLastKeepaliveTime != 0 &&
            (nNow - nLastKeepaliveTime) < MiningTimers::KEEPALIVE_GRACE_PERIOD_SEC)
            return false;

        return true;
    }


    /* static */
    std::string CanonicalSession::DeriveIP(const std::string& strAddress_)
    {
        auto nPos = strAddress_.rfind(':');
        if (nPos != std::string::npos)
            return strAddress_.substr(0, nPos);

        return strAddress_;
    }



    /* ════════════════════════════════════════════════════════════════════════════
     *  SessionStore — singleton
     * ════════════════════════════════════════════════════════════════════════════ */

    SessionStore& SessionStore::Get()
    {
        static SessionStore instance;
        return instance;
    }


    /* ──────────────────────────────────────────────────────────────────────────
     *  Registration
     * ────────────────────────────────────────────────────────────────────────── */

    bool SessionStore::Register(const CanonicalSession& session)
    {
        bool fNew = mapSessions.Insert(session.hashKeyID, session);
        if (!fNew)
        {
            /* Update existing entry */
            mapSessions.Update(session.hashKeyID, session);
        }

        /* Always update indexes to reflect current state */
        UpdateIndexes(session);

        return fNew;
    }


    bool SessionStore::Remove(const uint256_t& hashKeyID)
    {
        /* Get the session first so we can clean up indexes */
        auto opt = mapSessions.GetAndRemove(hashKeyID);
        if (!opt)
            return false;

        RemoveIndexes(opt.value());
        return true;
    }


    /* ──────────────────────────────────────────────────────────────────────────
     *  Lookups
     * ────────────────────────────────────────────────────────────────────────── */

    std::optional<CanonicalSession> SessionStore::LookupByKey(const uint256_t& hashKeyID) const
    {
        return mapSessions.Get(hashKeyID);
    }


    std::optional<CanonicalSession> SessionStore::LookupBySessionId(uint32_t nSessionId) const
    {
        auto optKey = mapSessionIdToKey.Get(nSessionId);
        if (!optKey)
            return std::nullopt;

        return mapSessions.Get(optKey.value());
    }


    std::optional<CanonicalSession> SessionStore::LookupByAddress(const std::string& strAddress) const
    {
        auto optKey = mapAddressToKey.Get(strAddress);
        if (!optKey)
            return std::nullopt;

        return mapSessions.Get(optKey.value());
    }


    std::optional<CanonicalSession> SessionStore::LookupByGenesis(const uint256_t& hashGenesis) const
    {
        auto optKey = mapGenesisToKey.Get(hashGenesis);
        if (!optKey)
            return std::nullopt;

        return mapSessions.Get(optKey.value());
    }


    std::optional<CanonicalSession> SessionStore::LookupByIP(const std::string& strIP) const
    {
        auto optKey = mapIPToKey.Get(strIP);
        if (!optKey)
            return std::nullopt;

        return mapSessions.Get(optKey.value());
    }


    bool SessionStore::Contains(const uint256_t& hashKeyID) const
    {
        return mapSessions.Contains(hashKeyID);
    }


    /* ──────────────────────────────────────────────────────────────────────────
     *  Atomic Mutations
     * ────────────────────────────────────────────────────────────────────────── */

    bool SessionStore::Transform(
        const uint256_t& hashKeyID,
        std::function<CanonicalSession(const CanonicalSession&)> transformer)
    {
        /* Get current state to compare indexes after transform */
        auto optBefore = mapSessions.Get(hashKeyID);
        if (!optBefore)
            return false;

        bool fOk = mapSessions.Transform(hashKeyID, transformer);
        if (!fOk)
            return false;

        /* Check if indexes need updating.
         * If the entry was removed concurrently between Transform() and Get(),
         * the transform still succeeded — return true.  The concurrent Remove()
         * already cleaned up the indexes. */
        auto optAfter = mapSessions.Get(hashKeyID);
        if (!optAfter)
            return true;  /* transform succeeded; entry removed concurrently — indexes already cleaned */

        const auto& before = optBefore.value();
        const auto& after  = optAfter.value();

        /* Update indexes if any indexed fields changed */
        if (before.strAddress != after.strAddress ||
            before.hashGenesis != after.hashGenesis ||
            before.strIP != after.strIP)
        {
            /* Remove old index entries */
            if (!before.strAddress.empty() && before.strAddress != after.strAddress)
                mapAddressToKey.Erase(before.strAddress);

            if (before.hashGenesis != 0 && before.hashGenesis != after.hashGenesis)
                mapGenesisToKey.Erase(before.hashGenesis);

            if (!before.strIP.empty() && before.strIP != after.strIP)
                mapIPToKey.Erase(before.strIP);

            /* Add new index entries */
            UpdateIndexes(after);
        }

        return true;
    }


    bool SessionStore::TransformBySessionId(
        uint32_t nSessionId,
        std::function<CanonicalSession(const CanonicalSession&)> transformer)
    {
        auto optKey = mapSessionIdToKey.Get(nSessionId);
        if (!optKey)
            return false;

        return Transform(optKey.value(), transformer);
    }


    bool SessionStore::TransformByAddress(
        const std::string& strAddress,
        std::function<CanonicalSession(const CanonicalSession&)> transformer)
    {
        auto optKey = mapAddressToKey.Get(strAddress);
        if (!optKey)
            return false;

        return Transform(optKey.value(), transformer);
    }


    /* ──────────────────────────────────────────────────────────────────────────
     *  Iteration & Aggregation
     * ────────────────────────────────────────────────────────────────────────── */

    void SessionStore::ForEach(
        std::function<void(const uint256_t&, const CanonicalSession&)> fn) const
    {
        mapSessions.ForEach(fn);
    }


    std::vector<CanonicalSession> SessionStore::GetAll() const
    {
        return mapSessions.GetAll();
    }


    std::vector<CanonicalSession> SessionStore::GetAllForChannel(uint32_t nChannel) const
    {
        std::vector<CanonicalSession> vResult;
        mapSessions.ForEach([&](const uint256_t&, const CanonicalSession& s)
        {
            if (s.nChannel == nChannel)
                vResult.push_back(s);
        });
        return vResult;
    }


    size_t SessionStore::Count() const
    {
        return mapSessions.Size();
    }


    size_t SessionStore::CountLive() const
    {
        size_t nCount = 0;
        mapSessions.ForEach([&](const uint256_t&, const CanonicalSession& s)
        {
            if (s.AnyPortLive())
                ++nCount;
        });
        return nCount;
    }


    size_t SessionStore::CountAuthenticated() const
    {
        size_t nCount = 0;
        mapSessions.ForEach([&](const uint256_t&, const CanonicalSession& s)
        {
            if (s.fAuthenticated)
                ++nCount;
        });
        return nCount;
    }


    /* ──────────────────────────────────────────────────────────────────────────
     *  Cleanup & Maintenance
     * ────────────────────────────────────────────────────────────────────────── */

    uint32_t SessionStore::SweepExpired(uint64_t nTimeoutSec, uint64_t nNow)
    {
        if (nNow == 0)
            nNow = runtime::unifiedtimestamp();

        /* Collect keys to remove */
        std::vector<uint256_t> vExpired;
        mapSessions.ForEach([&](const uint256_t& key, const CanonicalSession& s)
        {
            if (s.IsExpired(nTimeoutSec, nNow))
                vExpired.push_back(key);
        });

        /* Remove expired entries */
        for (const auto& key : vExpired)
            Remove(key);

        return static_cast<uint32_t>(vExpired.size());
    }


    uint32_t SessionStore::SweepRecoveryExpired(uint64_t nTimeoutSec, uint64_t nNow)
    {
        if (nNow == 0)
            nNow = runtime::unifiedtimestamp();

        /* Collect recovery entries that have expired */
        std::vector<uint256_t> vExpired;
        mapSessions.ForEach([&](const uint256_t& key, const CanonicalSession& s)
        {
            if (s.fSavedForRecovery && !s.AnyPortLive() &&
                !s.IsRecoverable(nTimeoutSec, nNow))
            {
                vExpired.push_back(key);
            }
        });

        for (const auto& key : vExpired)
            Remove(key);

        return static_cast<uint32_t>(vExpired.size());
    }


    void SessionStore::Clear()
    {
        mapSessions.Clear();
        mapSessionIdToKey.Clear();
        mapAddressToKey.Clear();
        mapGenesisToKey.Clear();
        mapIPToKey.Clear();
    }


    /* ──────────────────────────────────────────────────────────────────────────
     *  Recovery
     * ────────────────────────────────────────────────────────────────────────── */

    bool SessionStore::SaveForRecovery(const uint256_t& hashKeyID, ProtocolLane lane, uint64_t nNow)
    {
        if (nNow == 0)
            nNow = runtime::unifiedtimestamp();

        return Transform(hashKeyID, [&](const CanonicalSession& s)
        {
            CanonicalSession updated = s;
            updated.fSavedForRecovery = true;
            updated.nDisconnectTime   = nNow;

            /* Mark the disconnecting lane as dead */
            if (lane == ProtocolLane::STATELESS)
                updated.fStatelessLive = false;
            else
                updated.fLegacyLive = false;

            return updated;
        });
    }


    std::optional<CanonicalSession> SessionStore::RecoverSession(
        const uint256_t& hashKeyID,
        const std::string& strNewAddress,
        ProtocolLane lane,
        uint32_t nMaxReconnects)
    {
        auto opt = mapSessions.Get(hashKeyID);
        if (!opt)
            return std::nullopt;

        auto& session = opt.value();

        /* Must be saved for recovery */
        if (!session.fSavedForRecovery)
            return std::nullopt;

        /* Check reconnect limit */
        if (session.nReconnectCount >= nMaxReconnects)
        {
            /* Over limit — remove entirely */
            Remove(hashKeyID);
            return std::nullopt;
        }

        /* Perform recovery via Transform for atomicity */
        bool fOk = Transform(hashKeyID, [&](const CanonicalSession& s)
        {
            CanonicalSession updated = s;
            updated.fSavedForRecovery = false;
            updated.nDisconnectTime   = 0;
            updated.strAddress        = strNewAddress;
            updated.strIP             = CanonicalSession::DeriveIP(strNewAddress);
            updated.nReconnectCount  += 1;
            updated.nLastActivity     = runtime::unifiedtimestamp();
            updated.fMarkedDisconnected = false;
            updated.nFailedPackets    = 0;

            if (lane == ProtocolLane::STATELESS)
                updated.fStatelessLive = true;
            else
                updated.fLegacyLive = true;

            return updated;
        });

        if (!fOk)
            return std::nullopt;

        /* Return the updated session */
        return mapSessions.Get(hashKeyID);
    }


    bool SessionStore::HasRecoverableSession(const uint256_t& hashKeyID) const
    {
        auto opt = mapSessions.Get(hashKeyID);
        if (!opt)
            return false;

        return opt->fSavedForRecovery;
    }


    /* ──────────────────────────────────────────────────────────────────────────
     *  Health Tracking
     * ────────────────────────────────────────────────────────────────────────── */

    bool SessionStore::RecordSendSuccess(const uint256_t& hashKeyID, uint64_t nNow)
    {
        if (nNow == 0)
            nNow = runtime::unifiedtimestamp();

        return Transform(hashKeyID, [&](const CanonicalSession& s)
        {
            CanonicalSession updated = s;
            updated.nFailedPackets      = 0;
            updated.nLastSuccessfulSend = nNow;
            return updated;
        });
    }


    bool SessionStore::RecordSendFailure(const uint256_t& hashKeyID, uint32_t nThreshold)
    {
        return Transform(hashKeyID, [&](const CanonicalSession& s)
        {
            CanonicalSession updated = s;
            updated.nFailedPackets += 1;
            if (updated.nFailedPackets >= nThreshold)
                updated.fMarkedDisconnected = true;
            return updated;
        });
    }


    bool SessionStore::MarkDisconnected(const uint256_t& hashKeyID)
    {
        return Transform(hashKeyID, [](const CanonicalSession& s)
        {
            CanonicalSession updated = s;
            updated.fMarkedDisconnected = true;
            return updated;
        });
    }


    bool SessionStore::IsActive(const uint256_t& hashKeyID) const
    {
        auto opt = mapSessions.Get(hashKeyID);
        if (!opt)
            return false;

        return !opt->fMarkedDisconnected && opt->AnyPortLive();
    }


    bool SessionStore::IsActiveBySessionId(uint32_t nSessionId) const
    {
        auto optKey = mapSessionIdToKey.Get(nSessionId);
        if (!optKey)
            return false;

        return IsActive(optKey.value());
    }


    std::vector<uint32_t> SessionStore::GetActiveSessionIdsForChannel(
        uint32_t nChannel, ProtocolLane lane) const
    {
        std::vector<uint32_t> vResult;
        mapSessions.ForEach([&](const uint256_t&, const CanonicalSession& s)
        {
            if (s.fMarkedDisconnected)
                return;

            if (!s.fSubscribedToNotifications)
                return;

            if (s.nSubscribedChannel != nChannel)
                return;

            if (s.nProtocolLane != lane)
                return;

            if (!s.AnyPortLive())
                return;

            vResult.push_back(s.nSessionId);
        });
        return vResult;
    }


    /* ──────────────────────────────────────────────────────────────────────────
     *  Internal index management
     * ────────────────────────────────────────────────────────────────────────── */

    void SessionStore::UpdateIndexes(const CanonicalSession& session)
    {
        /* Session ID index (always present) */
        if (session.nSessionId != 0)
            mapSessionIdToKey.InsertOrUpdate(session.nSessionId, session.hashKeyID);

        /* Address index */
        if (!session.strAddress.empty())
            mapAddressToKey.InsertOrUpdate(session.strAddress, session.hashKeyID);

        /* Genesis index */
        if (session.hashGenesis != 0)
            mapGenesisToKey.InsertOrUpdate(session.hashGenesis, session.hashKeyID);

        /* IP index */
        if (!session.strIP.empty())
            mapIPToKey.InsertOrUpdate(session.strIP, session.hashKeyID);
    }


    void SessionStore::RemoveIndexes(const CanonicalSession& session)
    {
        if (session.nSessionId != 0)
            mapSessionIdToKey.Erase(session.nSessionId);

        if (!session.strAddress.empty())
            mapAddressToKey.Erase(session.strAddress);

        if (session.hashGenesis != 0)
            mapGenesisToKey.Erase(session.hashGenesis);

        if (!session.strIP.empty())
            mapIPToKey.Erase(session.strIP);
    }

} // namespace LLP
