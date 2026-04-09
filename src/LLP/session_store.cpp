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
#include <Util/include/config.h>

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




    bool CanonicalSession::IsExpired(uint64_t nTimeoutSec, uint64_t nNow) const
    {
        if (AnyPortLive())
            return false;

        if (nNow == 0)
            nNow = runtime::unifiedtimestamp();

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
        /* Pre-populate version for new entries (avoids a redundant Transform). */
        CanonicalSession sessionToInsert = session;
        sessionToInsert.nVersion = 1;

        bool fNew = mapSessions.Insert(sessionToInsert.hashKeyID, sessionToInsert);
        if (!fNew)
        {
            /* Update existing entry — bump version and clear negative health
             * state to prevent stale disconnects from killing re-registered
             * sessions (AUTH/DISCONNECT race fix).
             *
             * Full replacement with incoming session is intentional: Register()
             * is called from UpdateMiner() which builds a complete CanonicalSession
             * from the live MiningContext.  The incoming session has all current
             * fields; we only preserve the version counter from the old entry. */
            Transform(session.hashKeyID, [&](const CanonicalSession& old)
            {
                CanonicalSession updated = session;
                updated.nVersion = old.nVersion + 1;
                updated.nFailedPackets = 0;
                updated.fMarkedDisconnected = false;
                updated.nCooldownExpiry = 0;
                return updated;
            });
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



    void SessionStore::Clear()
    {
        mapSessions.Clear();
        mapSessionIdToKey.Clear();
        mapAddressToKey.Clear();
        mapGenesisToKey.Clear();
        mapIPToKey.Clear();
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
            updated.nCooldownExpiry     = 0;
            updated.fMarkedDisconnected = false;
            return updated;
        });
    }


    bool SessionStore::RecordSendFailure(const uint256_t& hashKeyID, uint32_t nThreshold)
    {
        return Transform(hashKeyID, [&](const CanonicalSession& s)
        {
            CanonicalSession updated = s;
            updated.nFailedPackets += 1;

            if (updated.nFailedPackets >= nThreshold && updated.nCooldownExpiry == 0)
            {
                /* Enter timed cooldown instead of permanent ban.
                 * The session will auto-recover when the cooldown expires.
                 * This prevents the permanent shadow-ban that occurred with the old
                 * fMarkedDisconnected-only approach. */
                const uint64_t nDuration = static_cast<uint64_t>(
                    config::GetArg("-miningcooldownseconds",
                                   SessionStore::DEFAULT_COOLDOWN_DURATION_SEC));
                const uint64_t nNow = runtime::unifiedtimestamp();
                updated.nCooldownExpiry = nNow + nDuration;
                updated.fMarkedDisconnected = true;

                debug::log(0, FUNCTION,
                    "Session entered cooldown: session=", updated.nSessionId,
                    " failures=", updated.nFailedPackets, "/", nThreshold,
                    " cooldown=", nDuration, "s",
                    " — PUSH paused (will auto-recover)");
            }
            return updated;
        });
    }


    bool SessionStore::MarkDisconnected(const uint256_t& hashKeyID, uint64_t nExpectedVersion)
    {
        return Transform(hashKeyID, [&](const CanonicalSession& s)
        {
            /* Version check: if the session has been re-registered since the
             * caller captured the version, reject this stale disconnect.
             * This prevents the AUTH/DISCONNECT race where an old connection's
             * DISCONNECT handler kills a freshly re-registered session. */
            if (nExpectedVersion != 0 && s.nVersion != nExpectedVersion)
            {
                debug::log(0, FUNCTION,
                    "Rejected stale MarkDisconnected: session=", s.nSessionId,
                    " expected_version=", nExpectedVersion,
                    " current_version=", s.nVersion,
                    " — session was re-registered; disconnect is stale");
                return s;  /* No-op: return unmodified */
            }

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

        /* Check cooldown: if expiry is set but has elapsed, treat as active */
        if (opt->fMarkedDisconnected && opt->nCooldownExpiry != 0)
        {
            const uint64_t nNow = runtime::unifiedtimestamp();
            if (nNow >= opt->nCooldownExpiry)
                return opt->AnyPortLive();  /* Cooldown expired — session is active */
        }

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
        const uint64_t nNow = runtime::unifiedtimestamp();

        mapSessions.ForEach([&](const uint256_t&, const CanonicalSession& s)
        {
            /* Check cooldown: if expiry is set and has elapsed, treat as active
             * (the actual nCooldownExpiry/fMarkedDisconnected reset happens in
             * SweepCooldowns or RecordSendSuccess — we just read-through here). */
            if (s.fMarkedDisconnected)
            {
                if (s.nCooldownExpiry == 0 || nNow < s.nCooldownExpiry)
                    return;  /* Permanently disconnected or still in cooldown */
                /* else: cooldown expired — treat as active, fall through */
            }

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


    uint64_t SessionStore::GetVersion(const uint256_t& hashKeyID) const
    {
        auto opt = mapSessions.Get(hashKeyID);
        if (!opt)
            return 0;

        return opt->nVersion;
    }


    uint32_t SessionStore::SweepCooldowns()
    {
        const uint64_t nNow = runtime::unifiedtimestamp();
        uint32_t nRecovered = 0;

        /* Collect keys that need recovery (can't Transform during ForEach) */
        std::vector<uint256_t> vExpiredKeys;
        mapSessions.ForEach([&](const uint256_t& key, const CanonicalSession& s)
        {
            if (s.fMarkedDisconnected && s.nCooldownExpiry != 0 && nNow >= s.nCooldownExpiry)
                vExpiredKeys.push_back(key);
        });

        /* Transform each expired session to recover it */
        for (const auto& key : vExpiredKeys)
        {
            bool fTransformed = Transform(key, [&](const CanonicalSession& s)
            {
                /* Double-check under lock: still in cooldown and still expired */
                if (!s.fMarkedDisconnected || s.nCooldownExpiry == 0 || nNow < s.nCooldownExpiry)
                    return s;  /* No-op */

                CanonicalSession updated = s;
                updated.nCooldownExpiry = 0;
                updated.fMarkedDisconnected = false;
                updated.nFailedPackets = 0;
                return updated;
            });

            if (fTransformed)
            {
                ++nRecovered;
                debug::log(2, FUNCTION, "Session auto-recovered from cooldown: key=",
                    key.SubString());
            }
        }

        if (nRecovered > 0)
            debug::log(0, FUNCTION, "Recovered ", nRecovered, " sessions from cooldown");

        return nRecovered;
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
