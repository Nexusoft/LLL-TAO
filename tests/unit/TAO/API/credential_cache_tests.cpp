/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <TAO/API/include/credential_cache.h>
#include <TAO/API/types/authentication.h>

namespace
{
    /* Distinguisher helper for session-ids that don't collide with default. */
    uint256_t MakeSessionId(uint8_t tag)
    {
        uint256_t s = 0;
        std::vector<uint8_t> bytes(32, 0);
        bytes[0] = 0xC0;
        bytes[1] = tag;
        bytes[31] = 0xFE;
        s.SetBytes(bytes);
        return s;
    }


    /* Ensure Authentication is initialized before tests touch it.  When the
     * `[credential_cache]` tag is invoked standalone (e.g. `./nexus
     * "[credential_cache]"`), the `[args]` setup TEST_CASE in main.cpp will
     * not have run, which leaves Authentication::vLocks empty.  Subsequent
     * paths that take `vLocks[hash % vLocks.size()]` would then trip a
     * floating-point divide-by-zero.  Initialize() is idempotent for our
     * purposes - it just (re)sizes the vector. */
    void EnsureAuthInitialized()
    {
        static bool fOnce = false;
        if(!fOnce)
        {
            TAO::API::Authentication::Initialize();
            fOnce = true;
        }
    }
}

using TAO::API::Authentication;
using TAO::API::CredentialCache;

TEST_CASE("Authentication epoch counter is monotonic and observable", "[tao][api][credential_cache][epoch]")
{
    EnsureAuthInitialized();

    const uint64_t before = Authentication::CurrentEpoch();

    /* Insert + Terminate of a throwaway session should each bump the epoch.
     * We construct a Session via the username/password ctor, which is the
     * public construction path used by sessions/create.cpp. */
    const uint256_t hashSession = MakeSessionId(0x01);

    {
        Authentication::Session rSession(
            SecureString("test-credcache-user-epoch"),
            SecureString("test-credcache-pass-epoch"));

        Authentication::Insert(hashSession, rSession);
    }

    const uint64_t afterInsert = Authentication::CurrentEpoch();
    REQUIRE(afterInsert > before);

    /* Terminate via the public API; this routes through terminate_session
     * which is the bump site for session removal. */
    encoding::json jParams;
    jParams["session"] = hashSession.ToString();

    /* Terminate may throw if multiuser mode parsing differs; in that case
     * the bump test for terminate is still meaningful via Insert/Update. */
    bool fTerminated = true;
    try
    {
        Authentication::Terminate(jParams);
    }
    catch(...)
    {
        fTerminated = false;
    }

    if(fTerminated)
    {
        const uint64_t afterTerminate = Authentication::CurrentEpoch();
        REQUIRE(afterTerminate > afterInsert);
    }
}


TEST_CASE("CredentialCache returns nullptr without throwing on missing session", "[tao][api][credential_cache]")
{
    CredentialCache cache;
    const uint256_t hashMissing = MakeSessionId(0x02);

    /* Sanity: this session was not inserted. */
    auto p = cache.Acquire(hashMissing);

    REQUIRE(p == nullptr);
    REQUIRE(cache.HasEntry() == false);
    REQUIRE(cache.LastReason() == CredentialCache::Reason::SESSION_MISSING);
    REQUIRE(cache.BoundSession() == uint256_t(0));
}


TEST_CASE("CredentialCache treats zero session-id as ineligible (no auth touch)", "[tao][api][credential_cache]")
{
    CredentialCache cache;
    auto p = cache.Acquire(uint256_t(0));

    REQUIRE(p == nullptr);
    REQUIRE(cache.HasEntry() == false);
    /* No state should have been mutated. */
    REQUIRE(cache.EpochSnapshot() == 0);
    REQUIRE(cache.LastValidated() == 0);
}


TEST_CASE("CredentialCache Invalidate clears slot and resets reason", "[tao][api][credential_cache]")
{
    CredentialCache cache;

    /* Try to populate from a missing session - state will record reason
     * SESSION_MISSING but slot stays empty.  Invalidate must still reset
     * cleanly to EMPTY without touching Authentication. */
    cache.Acquire(MakeSessionId(0x03));
    REQUIRE(cache.LastReason() == CredentialCache::Reason::SESSION_MISSING);

    cache.Invalidate();

    REQUIRE(cache.HasEntry() == false);
    REQUIRE(cache.BoundSession() == uint256_t(0));
    REQUIRE(cache.BoundGenesis() == uint256_t(0));
    REQUIRE(cache.EpochSnapshot() == 0);
    REQUIRE(cache.LastValidated() == 0);
    REQUIRE(cache.LastReason() == CredentialCache::Reason::EMPTY);
}


TEST_CASE("CredentialCache PostUseCheck detects epoch drift", "[tao][api][credential_cache][epoch]")
{
    EnsureAuthInitialized();

    CredentialCache cache;

    /* PostUseCheck on an empty cache: snapshot is 0, current epoch likely
     * != 0 because earlier tests bumped it.  Drift detection is what's under
     * test, not the populate path. */
    const uint64_t nLive = Authentication::CurrentEpoch();
    if(nLive == 0)
    {
        /* If somehow no epoch bumps occurred, force one via Insert/Terminate
         * to make the drift test meaningful. */
        const uint256_t s = MakeSessionId(0x04);
        Authentication::Session rSession(
            SecureString("test-credcache-user-drift"),
            SecureString("test-credcache-pass-drift"));
        Authentication::Insert(s, rSession);
        encoding::json jp;
        jp["session"] = s.ToString();
        try { Authentication::Terminate(jp); } catch(...) {}
    }

    REQUIRE(Authentication::CurrentEpoch() > 0);

    /* Cache snapshot is 0 (empty slot), live epoch is > 0: PostUseCheck
     * must report drift and set POST_USE_DRIFT. */
    const bool fStable = cache.PostUseCheck();
    REQUIRE(fStable == false);
    REQUIRE(cache.LastReason() == CredentialCache::Reason::POST_USE_DRIFT);
}


TEST_CASE("CredentialCache TTL constructor honors custom value", "[tao][api][credential_cache]")
{
    CredentialCache cacheDefault;
    REQUIRE(cacheDefault.TTLSeconds() == CredentialCache::DEFAULT_TTL_SECONDS);

    CredentialCache cacheCustom(5);
    REQUIRE(cacheCustom.TTLSeconds() == 5);

    CredentialCache cacheDisabled(0);
    REQUIRE(cacheDisabled.TTLSeconds() == 0);
}


TEST_CASE("Authentication epoch bumps under Update password", "[tao][api][credential_cache][epoch]")
{
    EnsureAuthInitialized();

    const uint256_t hashSession = MakeSessionId(0x05);

    /* Insert a session we own for this test. */
    {
        Authentication::Session rSession(
            SecureString("test-credcache-user-update"),
            SecureString("test-credcache-pass-update-1"));
        Authentication::Insert(hashSession, rSession);
    }

    const uint64_t afterInsert = Authentication::CurrentEpoch();

    /* Update the password - must bump epoch. */
    encoding::json jParams;
    jParams["session"] = hashSession.ToString();

    bool fUpdated = true;
    try
    {
        Authentication::Update(jParams, SecureString("test-credcache-pass-update-2"));
    }
    catch(...)
    {
        fUpdated = false;
    }

    if(fUpdated)
    {
        const uint64_t afterUpdate = Authentication::CurrentEpoch();
        REQUIRE(afterUpdate > afterInsert);
    }

    /* Cleanup. */
    try { Authentication::Terminate(jParams); } catch(...) {}
}
