/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <LLP/include/session_store.h>
#include <LLP/include/canonical_session.h>
#include <LLP/include/stateless_miner.h>

#include <Util/include/runtime.h>

#include <unit/catch2/catch.hpp>

#include <cstring>
#include <thread>
#include <vector>

using namespace LLP;


/* ════════════════════════════════════════════════════════════════════════════════
 *  Helpers
 * ════════════════════════════════════════════════════════════════════════════════ */

namespace
{
    /** Create a CanonicalSession with random identity. **/
    CanonicalSession MakeSession(const std::string& strAddr = "192.168.1.1:9323")
    {
        uint256_t hashKeyID  = LLC::GetRand256();
        uint256_t hashGenesis = LLC::GetRand256();
        CanonicalSession s(hashKeyID, hashGenesis, strAddr);
        s.fAuthenticated = true;
        s.nLastActivity  = runtime::unifiedtimestamp();
        s.fStatelessLive = true;
        s.nProtocolLane  = ProtocolLane::STATELESS;
        s.nChannel       = 2;
        s.fSubscribedToNotifications = true;
        s.nSubscribedChannel = 2;
        return s;
    }

    /** Create a CanonicalSession with a specific hashKeyID. **/
    CanonicalSession MakeSessionWithKey(
        const uint256_t& hashKeyID,
        const std::string& strAddr = "192.168.1.1:9323")
    {
        uint256_t hashGenesis = LLC::GetRand256();
        CanonicalSession s(hashKeyID, hashGenesis, strAddr);
        s.fAuthenticated = true;
        s.nLastActivity  = runtime::unifiedtimestamp();
        s.fStatelessLive = true;
        s.nProtocolLane  = ProtocolLane::STATELESS;
        return s;
    }

} // anonymous namespace


/* ════════════════════════════════════════════════════════════════════════════════
 *  CanonicalSession — Construction & Identity
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("CanonicalSession - Constructor derives sessionId and IP", "[session]")
{
    uint256_t hashKeyID  = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    CanonicalSession s(hashKeyID, hashGenesis, "10.0.0.5:9323");

    REQUIRE(s.hashKeyID == hashKeyID);
    REQUIRE(s.hashGenesis == hashGenesis);
    REQUIRE(s.strAddress == "10.0.0.5:9323");
    REQUIRE(s.strIP == "10.0.0.5");
    REQUIRE(s.nSessionId == MiningContext::DeriveSessionId(hashKeyID));
    REQUIRE(s.nSessionId != 0);
}


TEST_CASE("CanonicalSession - Default constructor has safe defaults", "[session]")
{
    CanonicalSession s;

    REQUIRE(s.hashKeyID == uint256_t(0));
    REQUIRE(s.nSessionId == 0);
    REQUIRE(s.fAuthenticated == false);
    REQUIRE(s.fStatelessLive == false);
    REQUIRE(s.fLegacyLive == false);
    REQUIRE(s.fMarkedDisconnected == false);
    REQUIRE(s.nFailedPackets == 0);
    REQUIRE(s.AnyPortLive() == false);
}


TEST_CASE("CanonicalSession - DeriveIP strips port", "[session]")
{
    REQUIRE(CanonicalSession::DeriveIP("192.168.1.1:9323") == "192.168.1.1");
    REQUIRE(CanonicalSession::DeriveIP("10.0.0.1:8080") == "10.0.0.1");
    REQUIRE(CanonicalSession::DeriveIP("localhost") == "localhost");
    REQUIRE(CanonicalSession::DeriveIP("") == "");
}


TEST_CASE("CanonicalSession - AnyPortLive", "[session]")
{
    CanonicalSession s;
    REQUIRE(s.AnyPortLive() == false);

    s.fStatelessLive = true;
    REQUIRE(s.AnyPortLive() == true);

    s.fStatelessLive = false;
    s.fLegacyLive = true;
    REQUIRE(s.AnyPortLive() == true);

    s.fStatelessLive = true;
    REQUIRE(s.AnyPortLive() == true);
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  CanonicalSession — IsExpired & IsConsideredInactive
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("CanonicalSession - IsExpired returns false when port live", "[session]")
{
    CanonicalSession s;
    s.fStatelessLive = true;
    s.nLastActivity = 1000;

    /* Even with old activity, live port → not expired */
    REQUIRE(s.IsExpired(3600, 999999) == false);
}


TEST_CASE("CanonicalSession - IsExpired returns true when no port live and timed out", "[session]")
{
    CanonicalSession s;
    s.fStatelessLive = false;
    s.fLegacyLive = false;
    s.nLastActivity = 1000;

    /* nLastActivity(1000) + timeout(3600) = 4600 < nNow(5000) → expired */
    REQUIRE(s.IsExpired(3600, 5000) == true);
}


TEST_CASE("CanonicalSession - IsExpired returns false when within timeout", "[session]")
{
    CanonicalSession s;
    s.fStatelessLive = false;
    s.nLastActivity = 1000;

    /* nLastActivity(1000) + timeout(3600) = 4600 > nNow(3000) → not expired */
    REQUIRE(s.IsExpired(3600, 3000) == false);
}


TEST_CASE("CanonicalSession - IsConsideredInactive guards against clock skew", "[session]")
{
    CanonicalSession s;
    s.nLastActivity = 5000;

    /* nNow(3000) < nLastActivity(5000) → unsigned underflow, but function
     * should return false (not inactive) when activity is recent. */
    REQUIRE(s.IsConsideredInactive(3000, 3600) == false);
}


TEST_CASE("CanonicalSession - IsConsideredInactive with keepalive grace", "[session]")
{
    CanonicalSession s;
    s.nLastActivity = 1000;  /* Old activity */
    s.nLastKeepaliveTime = 4500;  /* Recent keepalive */

    /* Activity is old but keepalive within grace → not inactive */
    REQUIRE(s.IsConsideredInactive(5000, 3600) == false);
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  CanonicalSession — Projections
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("CanonicalSession - ToMiningContext round-trip preserves identity", "[session]")
{
    uint256_t hashKeyID  = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    CanonicalSession s(hashKeyID, hashGenesis, "10.0.0.1:9323");
    s.fAuthenticated = true;
    s.nChannel = 2;
    s.nHeight = 100000;
    s.nProtocolVersion = 42;
    s.nKeepaliveCount = 5;

    MiningContext ctx = s.ToMiningContext();

    REQUIRE(ctx.hashKeyID == hashKeyID);
    REQUIRE(ctx.hashGenesis == hashGenesis);
    REQUIRE(ctx.nSessionId == s.nSessionId);
    REQUIRE(ctx.fAuthenticated == true);
    REQUIRE(ctx.nChannel == 2);
    REQUIRE(ctx.nHeight == 100000);
    REQUIRE(ctx.nProtocolVersion == 42);
    REQUIRE(ctx.nKeepaliveCount == 5);
    REQUIRE(ctx.strAddress == "10.0.0.1:9323");
}


TEST_CASE("CanonicalSession - FromMiningContext preserves fields", "[session]")
{
    MiningContext ctx;
    ctx.hashKeyID = LLC::GetRand256();
    ctx.hashGenesis = LLC::GetRand256();
    ctx.nSessionId = 12345;
    ctx.strAddress = "192.168.0.1:9323";
    ctx.fAuthenticated = true;
    ctx.nChannel = 1;
    ctx.nHeight = 50000;
    ctx.nProtocolLane = ProtocolLane::STATELESS;
    ctx.nTimestamp = runtime::unifiedtimestamp();

    CanonicalSession s = CanonicalSession::FromMiningContext(ctx);

    REQUIRE(s.hashKeyID == ctx.hashKeyID);
    REQUIRE(s.hashGenesis == ctx.hashGenesis);
    REQUIRE(s.nSessionId == 12345);
    REQUIRE(s.strAddress == "192.168.0.1:9323");
    REQUIRE(s.strIP == "192.168.0.1");
    REQUIRE(s.fAuthenticated == true);
    REQUIRE(s.nChannel == 1);
    REQUIRE(s.nHeight == 50000);
    REQUIRE(s.fStatelessLive == true);
    REQUIRE(s.fLegacyLive == false);
    REQUIRE(s.nLastActivity == ctx.nTimestamp);
}


TEST_CASE("CanonicalSession - GetSessionBinding", "[session]")
{
    uint256_t hashKeyID  = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    uint256_t hashReward  = LLC::GetRand256();
    CanonicalSession s(hashKeyID, hashGenesis, "10.0.0.1:9323");
    s.hashRewardAddress = hashReward;
    s.fRewardBound = true;
    s.nProtocolLane = ProtocolLane::STATELESS;

    SessionBinding binding = s.GetSessionBinding();

    REQUIRE(binding.nSessionId == s.nSessionId);
    REQUIRE(binding.hashGenesis == hashGenesis);
    REQUIRE(binding.hashKeyID == hashKeyID);
    REQUIRE(binding.hashRewardAddress == hashReward);
    REQUIRE(binding.fRewardBound == true);
    REQUIRE(binding.nProtocolLane == ProtocolLane::STATELESS);
    REQUIRE(binding.strKeyFingerprint.size() == 8);
}


TEST_CASE("CanonicalSession - GetCryptoContext", "[session]")
{
    uint256_t hashKeyID  = LLC::GetRand256();
    uint256_t hashGenesis = LLC::GetRand256();
    CanonicalSession s(hashKeyID, hashGenesis, "10.0.0.1:9323");
    s.fEncryptionReady = true;
    s.vChaChaKey = LLC::GetRand256().GetBytes();
    s.nProtocolLane = ProtocolLane::LEGACY;

    CryptoContext crypto = s.GetCryptoContext();

    REQUIRE(crypto.nSessionId == s.nSessionId);
    REQUIRE(crypto.hashGenesis == hashGenesis);
    REQUIRE(crypto.hashKeyID == hashKeyID);
    REQUIRE(crypto.fEncryptionReady == true);
    REQUIRE(crypto.vChaCha20Key == s.vChaChaKey);
    REQUIRE(crypto.nProtocolLane == ProtocolLane::LEGACY);
    REQUIRE(crypto.strKeyFingerprint.size() == 8);
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  SessionStore — Registration & Lookup
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("SessionStore - Register creates new session", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.1:9323");
    bool fNew = SessionStore::Get().Register(s);

    REQUIRE(fNew == true);
    REQUIRE(SessionStore::Get().Count() == 1);
    REQUIRE(SessionStore::Get().Contains(s.hashKeyID) == true);
}


TEST_CASE("SessionStore - Register updates existing session", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.1:9323");
    SessionStore::Get().Register(s);

    /* Update the same key with new data */
    s.nHeight = 99999;
    s.nChannel = 1;
    bool fNew = SessionStore::Get().Register(s);

    REQUIRE(fNew == false);
    REQUIRE(SessionStore::Get().Count() == 1);

    auto opt = SessionStore::Get().LookupByKey(s.hashKeyID);
    REQUIRE(opt.has_value());
    REQUIRE(opt->nHeight == 99999);
    REQUIRE(opt->nChannel == 1);
}


TEST_CASE("SessionStore - LookupByKey returns correct session", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.2:9323");
    SessionStore::Get().Register(s);

    auto opt = SessionStore::Get().LookupByKey(s.hashKeyID);
    REQUIRE(opt.has_value());
    REQUIRE(opt->hashKeyID == s.hashKeyID);
    REQUIRE(opt->strAddress == "10.0.0.2:9323");
    REQUIRE(opt->fAuthenticated == true);
}


TEST_CASE("SessionStore - LookupByKey returns nullopt for missing key", "[session]")
{
    SessionStore::Get().Clear();

    uint256_t hashMissing = LLC::GetRand256();
    auto opt = SessionStore::Get().LookupByKey(hashMissing);
    REQUIRE(!opt.has_value());
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  SessionStore — Secondary Index Lookups
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("SessionStore - LookupBySessionId via index", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.3:9323");
    SessionStore::Get().Register(s);

    auto opt = SessionStore::Get().LookupBySessionId(s.nSessionId);
    REQUIRE(opt.has_value());
    REQUIRE(opt->hashKeyID == s.hashKeyID);
}


TEST_CASE("SessionStore - LookupByAddress via index", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("172.16.0.1:9323");
    SessionStore::Get().Register(s);

    auto opt = SessionStore::Get().LookupByAddress("172.16.0.1:9323");
    REQUIRE(opt.has_value());
    REQUIRE(opt->hashKeyID == s.hashKeyID);
}


TEST_CASE("SessionStore - LookupByGenesis via index", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.4:9323");
    SessionStore::Get().Register(s);

    auto opt = SessionStore::Get().LookupByGenesis(s.hashGenesis);
    REQUIRE(opt.has_value());
    REQUIRE(opt->hashKeyID == s.hashKeyID);
}


TEST_CASE("SessionStore - LookupByIP via index", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.5:9323");
    SessionStore::Get().Register(s);

    auto opt = SessionStore::Get().LookupByIP("10.0.0.5");
    REQUIRE(opt.has_value());
    REQUIRE(opt->hashKeyID == s.hashKeyID);
}


TEST_CASE("SessionStore - Missing index lookup returns nullopt", "[session]")
{
    SessionStore::Get().Clear();

    REQUIRE(!SessionStore::Get().LookupBySessionId(99999).has_value());
    REQUIRE(!SessionStore::Get().LookupByAddress("0.0.0.0:1234").has_value());
    REQUIRE(!SessionStore::Get().LookupByGenesis(LLC::GetRand256()).has_value());
    REQUIRE(!SessionStore::Get().LookupByIP("0.0.0.0").has_value());
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  SessionStore — Index Consistency on Update
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("SessionStore - Re-register with new address updates indexes", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.1:9323");
    SessionStore::Get().Register(s);

    /* Verify old address works */
    REQUIRE(SessionStore::Get().LookupByAddress("10.0.0.1:9323").has_value());

    /* Re-register with new address */
    s.strAddress = "10.0.0.2:9323";
    s.strIP = "10.0.0.2";
    SessionStore::Get().Register(s);

    /* New address should work */
    auto opt = SessionStore::Get().LookupByAddress("10.0.0.2:9323");
    REQUIRE(opt.has_value());
    REQUIRE(opt->hashKeyID == s.hashKeyID);

    /* Count should still be 1 */
    REQUIRE(SessionStore::Get().Count() == 1);
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  SessionStore — Remove
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("SessionStore - Remove cleans primary and all indexes", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.10:9323");
    SessionStore::Get().Register(s);

    REQUIRE(SessionStore::Get().Count() == 1);

    bool fRemoved = SessionStore::Get().Remove(s.hashKeyID);
    REQUIRE(fRemoved == true);
    REQUIRE(SessionStore::Get().Count() == 0);

    /* All indexes should be cleaned */
    REQUIRE(!SessionStore::Get().LookupByKey(s.hashKeyID).has_value());
    REQUIRE(!SessionStore::Get().LookupBySessionId(s.nSessionId).has_value());
    REQUIRE(!SessionStore::Get().LookupByAddress("10.0.0.10:9323").has_value());
    REQUIRE(!SessionStore::Get().LookupByGenesis(s.hashGenesis).has_value());
    REQUIRE(!SessionStore::Get().LookupByIP("10.0.0.10").has_value());
}


TEST_CASE("SessionStore - Remove non-existent returns false", "[session]")
{
    SessionStore::Get().Clear();

    bool fRemoved = SessionStore::Get().Remove(LLC::GetRand256());
    REQUIRE(fRemoved == false);
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  SessionStore — Transform (Atomic Mutations)
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("SessionStore - Transform modifies session atomically", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.20:9323");
    SessionStore::Get().Register(s);

    bool fOk = SessionStore::Get().Transform(s.hashKeyID,
        [](const CanonicalSession& cs)
        {
            CanonicalSession updated = cs;
            updated.nHeight = 200000;
            updated.nChannel = 1;
            return updated;
        });

    REQUIRE(fOk == true);

    auto opt = SessionStore::Get().LookupByKey(s.hashKeyID);
    REQUIRE(opt.has_value());
    REQUIRE(opt->nHeight == 200000);
    REQUIRE(opt->nChannel == 1);
}


TEST_CASE("SessionStore - Transform returns false for missing key", "[session]")
{
    SessionStore::Get().Clear();

    bool fOk = SessionStore::Get().Transform(LLC::GetRand256(),
        [](const CanonicalSession& cs) { return cs; });

    REQUIRE(fOk == false);
}


TEST_CASE("SessionStore - Transform updates indexes on address change", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.30:9323");
    SessionStore::Get().Register(s);

    /* Transform to change address */
    SessionStore::Get().Transform(s.hashKeyID,
        [](const CanonicalSession& cs)
        {
            CanonicalSession updated = cs;
            updated.strAddress = "10.0.0.31:9323";
            updated.strIP = "10.0.0.31";
            return updated;
        });

    /* New address should resolve */
    auto opt = SessionStore::Get().LookupByAddress("10.0.0.31:9323");
    REQUIRE(opt.has_value());
    REQUIRE(opt->hashKeyID == s.hashKeyID);

    /* New IP should resolve */
    auto optIP = SessionStore::Get().LookupByIP("10.0.0.31");
    REQUIRE(optIP.has_value());
    REQUIRE(optIP->hashKeyID == s.hashKeyID);
}


TEST_CASE("SessionStore - TransformBySessionId", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.40:9323");
    SessionStore::Get().Register(s);

    bool fOk = SessionStore::Get().TransformBySessionId(s.nSessionId,
        [](const CanonicalSession& cs)
        {
            CanonicalSession updated = cs;
            updated.nHeight = 300000;
            return updated;
        });

    REQUIRE(fOk == true);

    auto opt = SessionStore::Get().LookupByKey(s.hashKeyID);
    REQUIRE(opt->nHeight == 300000);
}


TEST_CASE("SessionStore - TransformByAddress", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.0.50:9323");
    SessionStore::Get().Register(s);

    bool fOk = SessionStore::Get().TransformByAddress("10.0.0.50:9323",
        [](const CanonicalSession& cs)
        {
            CanonicalSession updated = cs;
            updated.nKeepaliveCount = 42;
            return updated;
        });

    REQUIRE(fOk == true);

    auto opt = SessionStore::Get().LookupByKey(s.hashKeyID);
    REQUIRE(opt->nKeepaliveCount == 42);
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  SessionStore — Iteration & Aggregation
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("SessionStore - Count, CountLive, CountAuthenticated", "[session]")
{
    SessionStore::Get().Clear();

    /* Register 3 sessions */
    CanonicalSession s1 = MakeSession("10.0.1.1:9323");
    s1.fStatelessLive = true;
    s1.fAuthenticated = true;

    CanonicalSession s2 = MakeSession("10.0.1.2:9323");
    s2.fStatelessLive = false;
    s2.fLegacyLive = false;
    s2.fAuthenticated = true;

    CanonicalSession s3 = MakeSession("10.0.1.3:9323");
    s3.fStatelessLive = true;
    s3.fAuthenticated = false;

    SessionStore::Get().Register(s1);
    SessionStore::Get().Register(s2);
    SessionStore::Get().Register(s3);

    REQUIRE(SessionStore::Get().Count() == 3);
    REQUIRE(SessionStore::Get().CountLive() == 2);        /* s1, s3 */
    REQUIRE(SessionStore::Get().CountAuthenticated() == 2); /* s1, s2 */
}


TEST_CASE("SessionStore - GetAll returns all sessions", "[session]")
{
    SessionStore::Get().Clear();

    SessionStore::Get().Register(MakeSession("10.0.2.1:9323"));
    SessionStore::Get().Register(MakeSession("10.0.2.2:9323"));

    auto vAll = SessionStore::Get().GetAll();
    REQUIRE(vAll.size() == 2);
}


TEST_CASE("SessionStore - GetAllForChannel filters by channel", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s1 = MakeSession("10.0.3.1:9323");
    s1.nChannel = 1;  /* Prime */

    CanonicalSession s2 = MakeSession("10.0.3.2:9323");
    s2.nChannel = 2;  /* Hash */

    CanonicalSession s3 = MakeSession("10.0.3.3:9323");
    s3.nChannel = 2;  /* Hash */

    SessionStore::Get().Register(s1);
    SessionStore::Get().Register(s2);
    SessionStore::Get().Register(s3);

    REQUIRE(SessionStore::Get().GetAllForChannel(1).size() == 1);
    REQUIRE(SessionStore::Get().GetAllForChannel(2).size() == 2);
    REQUIRE(SessionStore::Get().GetAllForChannel(0).size() == 0);
}


TEST_CASE("SessionStore - ForEach visits every session", "[session]")
{
    SessionStore::Get().Clear();

    SessionStore::Get().Register(MakeSession("10.0.4.1:9323"));
    SessionStore::Get().Register(MakeSession("10.0.4.2:9323"));
    SessionStore::Get().Register(MakeSession("10.0.4.3:9323"));

    uint32_t nVisited = 0;
    SessionStore::Get().ForEach([&](const uint256_t&, const CanonicalSession&)
    {
        ++nVisited;
    });

    REQUIRE(nVisited == 3);
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  SessionStore — SweepExpired
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("SessionStore - SweepExpired removes timed-out sessions", "[session]")
{
    SessionStore::Get().Clear();

    /* Session 1: expired (old activity, no ports live) */
    CanonicalSession s1 = MakeSession("10.0.5.1:9323");
    s1.nLastActivity = 1000;
    s1.fStatelessLive = false;
    s1.fLegacyLive = false;

    /* Session 2: alive (port live) */
    CanonicalSession s2 = MakeSession("10.0.5.2:9323");
    s2.nLastActivity = 1000;
    s2.fStatelessLive = true;

    /* Session 3: not expired (recent activity) */
    CanonicalSession s3 = MakeSession("10.0.5.3:9323");
    s3.nLastActivity = 90000;
    s3.fStatelessLive = false;

    SessionStore::Get().Register(s1);
    SessionStore::Get().Register(s2);
    SessionStore::Get().Register(s3);

    REQUIRE(SessionStore::Get().Count() == 3);

    /* Sweep with timeout=3600, nNow=100000 */
    uint32_t nSwept = SessionStore::Get().SweepExpired(3600, 100000);

    REQUIRE(nSwept == 1);  /* Only s1 expired */
    REQUIRE(SessionStore::Get().Count() == 2);
    REQUIRE(!SessionStore::Get().Contains(s1.hashKeyID));
    REQUIRE(SessionStore::Get().Contains(s2.hashKeyID));
    REQUIRE(SessionStore::Get().Contains(s3.hashKeyID));
}


TEST_CASE("SessionStore - SweepExpired cleans indexes of removed sessions", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.6.1:9323");
    s.nLastActivity = 1000;
    s.fStatelessLive = false;
    SessionStore::Get().Register(s);

    SessionStore::Get().SweepExpired(3600, 100000);

    REQUIRE(!SessionStore::Get().LookupBySessionId(s.nSessionId).has_value());
    REQUIRE(!SessionStore::Get().LookupByAddress("10.0.6.1:9323").has_value());
    REQUIRE(!SessionStore::Get().LookupByGenesis(s.hashGenesis).has_value());
    REQUIRE(!SessionStore::Get().LookupByIP("10.0.6.1").has_value());
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  SessionStore — Health Tracking
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("SessionStore - RecordSendSuccess resets failure count", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.7.1:9323");
    s.nFailedPackets = 3;
    SessionStore::Get().Register(s);

    uint64_t nNow = runtime::unifiedtimestamp();
    bool fOk = SessionStore::Get().RecordSendSuccess(s.hashKeyID, nNow);
    REQUIRE(fOk == true);

    auto opt = SessionStore::Get().LookupByKey(s.hashKeyID);
    REQUIRE(opt->nFailedPackets == 0);
    REQUIRE(opt->nLastSuccessfulSend == nNow);
}


TEST_CASE("SessionStore - RecordSendFailure increments and marks at threshold", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.7.2:9323");
    s.nFailedPackets = 0;
    s.fMarkedDisconnected = false;
    SessionStore::Get().Register(s);

    /* Fail 4 times (threshold=5), should not disconnect */
    for (int i = 0; i < 4; ++i)
        SessionStore::Get().RecordSendFailure(s.hashKeyID, 5);

    auto opt = SessionStore::Get().LookupByKey(s.hashKeyID);
    REQUIRE(opt->nFailedPackets == 4);
    REQUIRE(opt->fMarkedDisconnected == false);

    /* 5th failure should trigger disconnect */
    SessionStore::Get().RecordSendFailure(s.hashKeyID, 5);

    opt = SessionStore::Get().LookupByKey(s.hashKeyID);
    REQUIRE(opt->nFailedPackets == 5);
    REQUIRE(opt->fMarkedDisconnected == true);
}


TEST_CASE("SessionStore - MarkDisconnected", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.0.7.3:9323");
    SessionStore::Get().Register(s);

    REQUIRE(SessionStore::Get().IsActive(s.hashKeyID) == true);

    SessionStore::Get().MarkDisconnected(s.hashKeyID);

    REQUIRE(SessionStore::Get().IsActive(s.hashKeyID) == false);

    auto opt = SessionStore::Get().LookupByKey(s.hashKeyID);
    REQUIRE(opt->fMarkedDisconnected == true);
}


TEST_CASE("SessionStore - IsActive and IsActiveBySessionId", "[session]")
{
    SessionStore::Get().Clear();

    CanonicalSession s1 = MakeSession("10.0.8.1:9323");
    s1.fStatelessLive = true;
    s1.fMarkedDisconnected = false;

    CanonicalSession s2 = MakeSession("10.0.8.2:9323");
    s2.fStatelessLive = true;
    s2.fMarkedDisconnected = true;

    CanonicalSession s3 = MakeSession("10.0.8.3:9323");
    s3.fStatelessLive = false;
    s3.fLegacyLive = false;
    s3.fMarkedDisconnected = false;

    SessionStore::Get().Register(s1);
    SessionStore::Get().Register(s2);
    SessionStore::Get().Register(s3);

    /* s1: live + not disconnected → active */
    REQUIRE(SessionStore::Get().IsActive(s1.hashKeyID) == true);
    REQUIRE(SessionStore::Get().IsActiveBySessionId(s1.nSessionId) == true);

    /* s2: live + disconnected → not active */
    REQUIRE(SessionStore::Get().IsActive(s2.hashKeyID) == false);

    /* s3: not live → not active */
    REQUIRE(SessionStore::Get().IsActive(s3.hashKeyID) == false);

    /* Missing key → not active */
    REQUIRE(SessionStore::Get().IsActive(LLC::GetRand256()) == false);
    REQUIRE(SessionStore::Get().IsActiveBySessionId(99999) == false);
}


TEST_CASE("SessionStore - GetActiveSessionIdsForChannel", "[session]")
{
    SessionStore::Get().Clear();

    /* Active Hash miner on stateless lane */
    CanonicalSession s1 = MakeSession("10.0.9.1:9323");
    s1.nChannel = 2;
    s1.fSubscribedToNotifications = true;
    s1.nSubscribedChannel = 2;
    s1.fStatelessLive = true;
    s1.fMarkedDisconnected = false;
    s1.nProtocolLane = ProtocolLane::STATELESS;

    /* Active Prime miner on stateless lane */
    CanonicalSession s2 = MakeSession("10.0.9.2:9323");
    s2.nChannel = 1;
    s2.fSubscribedToNotifications = true;
    s2.nSubscribedChannel = 1;
    s2.fStatelessLive = true;
    s2.fMarkedDisconnected = false;
    s2.nProtocolLane = ProtocolLane::STATELESS;

    /* Disconnected Hash miner — should be excluded */
    CanonicalSession s3 = MakeSession("10.0.9.3:9323");
    s3.nChannel = 2;
    s3.fSubscribedToNotifications = true;
    s3.nSubscribedChannel = 2;
    s3.fStatelessLive = true;
    s3.fMarkedDisconnected = true;
    s3.nProtocolLane = ProtocolLane::STATELESS;

    /* Polling miner (not subscribed) — should be excluded */
    CanonicalSession s4 = MakeSession("10.0.9.4:9323");
    s4.nChannel = 2;
    s4.fSubscribedToNotifications = false;
    s4.fStatelessLive = true;
    s4.nProtocolLane = ProtocolLane::STATELESS;

    SessionStore::Get().Register(s1);
    SessionStore::Get().Register(s2);
    SessionStore::Get().Register(s3);
    SessionStore::Get().Register(s4);

    auto vHash = SessionStore::Get().GetActiveSessionIdsForChannel(2, ProtocolLane::STATELESS);
    REQUIRE(vHash.size() == 1);
    REQUIRE(vHash[0] == s1.nSessionId);

    auto vPrime = SessionStore::Get().GetActiveSessionIdsForChannel(1, ProtocolLane::STATELESS);
    REQUIRE(vPrime.size() == 1);
    REQUIRE(vPrime[0] == s2.nSessionId);
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  SessionStore — Multiple Sessions
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("SessionStore - Multiple independent sessions", "[session]")
{
    SessionStore::Get().Clear();

    const uint32_t N = 20;
    std::vector<CanonicalSession> sessions;

    for (uint32_t i = 0; i < N; ++i)
    {
        std::string addr = "10.1." + std::to_string(i) + ".1:9323";
        CanonicalSession s = MakeSession(addr);
        sessions.push_back(s);
        SessionStore::Get().Register(s);
    }

    REQUIRE(SessionStore::Get().Count() == N);

    /* Verify each session is independently accessible */
    for (const auto& s : sessions)
    {
        auto opt = SessionStore::Get().LookupByKey(s.hashKeyID);
        REQUIRE(opt.has_value());
        REQUIRE(opt->strAddress == s.strAddress);
    }

    /* Remove half */
    for (uint32_t i = 0; i < N / 2; ++i)
        SessionStore::Get().Remove(sessions[i].hashKeyID);

    REQUIRE(SessionStore::Get().Count() == N / 2);
}


TEST_CASE("SessionStore - Clear removes everything", "[session]")
{
    SessionStore::Get().Clear();

    SessionStore::Get().Register(MakeSession("10.0.10.1:9323"));
    SessionStore::Get().Register(MakeSession("10.0.10.2:9323"));
    SessionStore::Get().Register(MakeSession("10.0.10.3:9323"));

    REQUIRE(SessionStore::Get().Count() == 3);

    SessionStore::Get().Clear();

    REQUIRE(SessionStore::Get().Count() == 0);
}


/* ════════════════════════════════════════════════════════════════════════════════
 *  SessionStore — Concurrency Safety
 * ════════════════════════════════════════════════════════════════════════════════ */

TEST_CASE("SessionStore - Concurrent register and lookup", "[session][concurrency]")
{
    SessionStore::Get().Clear();

    const uint32_t N = 50;
    std::vector<CanonicalSession> sessions;

    for (uint32_t i = 0; i < N; ++i)
    {
        std::string addr = "10.2." + std::to_string(i) + ".1:9323";
        sessions.push_back(MakeSession(addr));
    }

    /* Register from multiple threads concurrently */
    std::vector<std::thread> vThreads;
    for (uint32_t i = 0; i < N; ++i)
    {
        vThreads.emplace_back([&, i]()
        {
            SessionStore::Get().Register(sessions[i]);
        });
    }

    for (auto& t : vThreads)
        t.join();

    REQUIRE(SessionStore::Get().Count() == N);

    /* Verify all sessions are accessible */
    for (const auto& s : sessions)
    {
        auto opt = SessionStore::Get().LookupByKey(s.hashKeyID);
        REQUIRE(opt.has_value());
    }
}


TEST_CASE("SessionStore - Concurrent transform stress", "[session][concurrency]")
{
    SessionStore::Get().Clear();

    CanonicalSession s = MakeSession("10.3.0.1:9323");
    s.nKeepaliveCount = 0;
    SessionStore::Get().Register(s);

    const uint32_t N = 100;
    std::vector<std::thread> vThreads;

    /* Each thread increments keepalive count by 1 */
    for (uint32_t i = 0; i < N; ++i)
    {
        vThreads.emplace_back([&]()
        {
            SessionStore::Get().Transform(s.hashKeyID,
                [](const CanonicalSession& cs)
                {
                    CanonicalSession updated = cs;
                    updated.nKeepaliveCount += 1;
                    return updated;
                });
        });
    }

    for (auto& t : vThreads)
        t.join();

    auto opt = SessionStore::Get().LookupByKey(s.hashKeyID);
    REQUIRE(opt.has_value());
    REQUIRE(opt->nKeepaliveCount == N);
}
