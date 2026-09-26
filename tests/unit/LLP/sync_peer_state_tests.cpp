/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People
__________________________________________________________________________________________*/

#include <LLP/include/sync_peer_state.h>

#include <unit/catch2/catch.hpp>

#include <atomic>
#include <limits>
#include <thread>
#include <vector>

TEST_CASE("Sync peers reject zero sessions and disabled capacity", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE_FALSE(peers.Begin(0, 100, 4));
    REQUIRE_FALSE(peers.Begin(1, 100, 0));
    REQUIRE_FALSE(peers.Active(0));
    REQUIRE_FALSE(peers.Complete(0));
    REQUIRE_FALSE(peers.Expire(0, 200));
    peers.Progress(0, 200);
    peers.Pause(0, 200);
    peers.Remove(0);
    REQUIRE(peers.Primary() == 0);
    REQUIRE(peers.Size() == 0);
    REQUIRE(peers.Begin(1, 100, 1));
}

TEST_CASE("Sync peers keep only one outstanding request per session", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE(peers.Begin(1, 100, 1));
    REQUIRE_FALSE(peers.Begin(1, 101, 4));
    REQUIRE_FALSE(peers.Begin(2, 101, 1));
    REQUIRE(peers.Active(1));
    REQUIRE(peers.Primary() == 1);
    REQUIRE(peers.Size() == 1);
    REQUIRE(peers.Complete(1));
    REQUIRE_FALSE(peers.Complete(1));
    REQUIRE_FALSE(peers.Active(1));
    REQUIRE(peers.Primary() == 0);
    REQUIRE(peers.Size() == 0);
    REQUIRE(peers.Begin(1, 101, 1));
    REQUIRE(peers.Complete(1));
    REQUIRE(peers.Begin(2, 101, 1));
}

TEST_CASE("Sync peers bound concurrent streams and replace completed peers", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    for(uint64_t session = 1; session <= 4; ++session)
        REQUIRE(peers.Begin(session, 100, 4));

    REQUIRE(peers.Size() == 4);
    REQUIRE_FALSE(peers.Begin(5, 100, 4));
    REQUIRE_FALSE(peers.Begin(5, 100, 1));
    REQUIRE(peers.Complete(2));
    REQUIRE(peers.Size() == 3);
    REQUIRE(peers.Active(1));
    REQUIRE(peers.Active(3));
    REQUIRE(peers.Active(4));
    REQUIRE(peers.Begin(5, 100, 4));
    REQUIRE(peers.Size() == 4);
    REQUIRE(peers.Complete(1));
    REQUIRE(peers.Primary() != 0);
    REQUIRE(peers.Active(peers.Primary()));
}

TEST_CASE("Sync peers track progress and expiration independently", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE(peers.Begin(1, 100, 4));
    REQUIRE(peers.Begin(2, 100, 4));
    REQUIRE(peers.Begin(3, 100, 4));
    peers.Progress(2, 150);
    REQUIRE_FALSE(peers.Expire(1, 159));
    REQUIRE(peers.Expire(1, 160));
    REQUIRE_FALSE(peers.Expire(1, 200));
    REQUIRE_FALSE(peers.Expire(2, 160));
    REQUIRE(peers.Expire(3, 160));
    REQUIRE(peers.Size() == 1);
    REQUIRE(peers.Primary() == 2);
    REQUIRE_FALSE(peers.Expire(2, 209));
    REQUIRE(peers.Expire(2, 210));
    REQUIRE(peers.Size() == 0);
    REQUIRE(peers.Primary() == 0);
}

TEST_CASE("Sync peers support custom and zero inactivity timeouts", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE(peers.Begin(1, 100, 4));
    REQUIRE_FALSE(peers.Expire(1, 109, 10));
    REQUIRE(peers.Expire(1, 110, 10));
    REQUIRE(peers.Begin(2, 100, 4));
    REQUIRE(peers.Expire(2, 100, 0));
}

TEST_CASE("Sync peers cannot reuse timed out streams until their old tail drains", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE(peers.Begin(1, 100, 1));
    REQUIRE(peers.Expire(1, 160));
    REQUIRE(peers.Size() == 0);
    REQUIRE_FALSE(peers.Active(1));
    REQUIRE(peers.Begin(2, 160, 1));
    REQUIRE(peers.Complete(2));
    REQUIRE_FALSE(peers.Begin(1, 190, 1));
    REQUIRE_FALSE(peers.Begin(1, 1000, 1));
    peers.Progress(1, 1000);
    REQUIRE_FALSE(peers.Active(1));
    REQUIRE_FALSE(peers.Complete(1));
    REQUIRE(peers.Begin(1, 1000, 1));
    REQUIRE(peers.Active(1));
    REQUIRE(peers.Size() == 1);
}

TEST_CASE("Sync peers preserve cooldown after draining a retired response", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE(peers.Begin(1, 100, 1));
    peers.Pause(1, 110);
    REQUIRE_FALSE(peers.Complete(1));
    REQUIRE_FALSE(peers.Complete(1));
    REQUIRE_FALSE(peers.Begin(1, 139, 1));
    REQUIRE(peers.Begin(1, 140, 1));
    REQUIRE(peers.Complete(1));
    REQUIRE(peers.Begin(1, 140, 1));
}

TEST_CASE("Sync peers roll back unsent requests without phantom pending tails", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE(peers.Begin(1, 100, 1));
    REQUIRE(peers.Pending(1));
    REQUIRE(peers.Complete(1));
    peers.Pause(1, 100);
    REQUIRE_FALSE(peers.Pending(1));
    REQUIRE(peers.Size() == 0);
    REQUIRE_FALSE(peers.Begin(1, 129, 1));
    REQUIRE(peers.Begin(2, 100, 1));
    REQUIRE(peers.Complete(2));
    REQUIRE(peers.Begin(1, 130, 1));
}

TEST_CASE("Sync pending queries include retired streams until their old response tail drains",
    "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    const LLP::SyncPeerState& state = peers;
    REQUIRE_FALSE(state.Pending(0));
    REQUIRE_FALSE(state.Pending(1));
    REQUIRE(peers.Begin(1, 100, 1));
    REQUIRE(state.Pending(1));
    REQUIRE_FALSE(state.Pending(0));
    REQUIRE_FALSE(state.Pending(2));

    SECTION("Paused response")
    {
        peers.Pause(1, 110);
    }

    SECTION("Expired response")
    {
        REQUIRE(peers.Expire(1, 160));
    }

    REQUIRE_FALSE(state.Active(1));
    REQUIRE(state.Pending(1));
    REQUIRE_FALSE(peers.Begin(1, 1000, 1));
    REQUIRE(state.Pending(1));
    peers.Progress(1, 1000);
    REQUIRE(state.Pending(1));
    REQUIRE_FALSE(peers.Complete(1));
    REQUIRE_FALSE(state.Pending(1));
    REQUIRE(peers.Begin(1, 1000, 1));
    REQUIRE(state.Pending(1));
    REQUIRE(peers.Complete(1));
    REQUIRE_FALSE(state.Pending(1));
}

TEST_CASE("Sync pending queries forget disconnected and cleared streams", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE(peers.Begin(1, 100, 4));
    REQUIRE(peers.Begin(2, 100, 4));
    REQUIRE(peers.Begin(3, 100, 4));
    peers.Pause(1, 110);
    REQUIRE(peers.Pending(1));
    peers.Remove(1);
    REQUIRE_FALSE(peers.Pending(1));
    REQUIRE(peers.Pending(2));
    peers.Remove(2);
    REQUIRE_FALSE(peers.Pending(2));
    peers.Clear();
    REQUIRE_FALSE(peers.Pending(3));
}

TEST_CASE("Sync peers ignore unknown sessions without creating cooldown records", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    peers.Pause(1, 100);
    peers.Progress(1, 100);
    REQUIRE_FALSE(peers.Expire(1, 200));
    REQUIRE_FALSE(peers.Active(1));
    REQUIRE_FALSE(peers.Complete(1));
    peers.Remove(1);
    REQUIRE(peers.Size() == 0);
    REQUIRE(peers.Begin(1, 100, 1));
    REQUIRE_FALSE(peers.Complete(2));
    peers.Pause(2, 100);
    REQUIRE(peers.Size() == 1);
}

TEST_CASE("Sync peers do not enroll requests rejected by the capacity limit", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE(peers.Begin(1, 100, 1));
    REQUIRE_FALSE(peers.Begin(2, 100, 1));
    peers.Pause(2, 100);
    REQUIRE(peers.Complete(1));
    REQUIRE(peers.Begin(2, 100, 1));
}

TEST_CASE("Sync peers retire each active slot only once", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE(peers.Begin(1, 100, 4));
    REQUIRE(peers.Begin(2, 100, 4));
    peers.Pause(1, 110);
    peers.Pause(1, 110);
    REQUIRE_FALSE(peers.Expire(1, 200));
    REQUIRE_FALSE(peers.Complete(1));
    REQUIRE_FALSE(peers.Complete(1));
    peers.Remove(1);
    REQUIRE(peers.Size() == 1);
    REQUIRE(peers.Primary() == 2);
    for(uint64_t session = 3; session <= 5; ++session)
        REQUIRE(peers.Begin(session, 110, 4));

    REQUIRE(peers.Size() == 4);
    REQUIRE_FALSE(peers.Begin(6, 110, 4));
}

TEST_CASE("Sync peers remove all disconnected session state", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE(peers.Begin(1, 100, 1));
    peers.Remove(1);
    peers.Remove(1);
    REQUIRE(peers.Size() == 0);
    REQUIRE_FALSE(peers.Complete(1));
    REQUIRE(peers.Begin(1, 100, 1));
    peers.Pause(1, 110);
    peers.Remove(1);
    REQUIRE(peers.Begin(1, 110, 1));
    REQUIRE(peers.Complete(1));
    peers.Pause(1, 110);
    peers.Remove(1);
    REQUIRE(peers.Begin(1, 110, 1));
}

TEST_CASE("Sync peers clear active retired and completed records", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    for(uint64_t session = 1; session <= 3; ++session)
        REQUIRE(peers.Begin(session, 100, 4));

    peers.Pause(2, 110);
    REQUIRE(peers.Complete(3));
    peers.Clear();
    peers.Clear();
    REQUIRE(peers.Size() == 0);
    REQUIRE(peers.Primary() == 0);
    for(uint64_t session = 1; session <= 3; ++session)
    {
        REQUIRE_FALSE(peers.Active(session));
        REQUIRE_FALSE(peers.Complete(session));
        REQUIRE(peers.Begin(session, 110, 4));
    }
}

TEST_CASE("Sync peers retire all active streams without losing response tail ownership",
    "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    for(uint64_t session = 1; session <= 4; ++session)
        REQUIRE(peers.Begin(session, 100, 4));

    peers.RetireAll(110);
    REQUIRE(peers.Size() == 0);
    REQUIRE(peers.Primary() == 0);
    for(uint64_t session = 1; session <= 4; ++session)
    {
        REQUIRE_FALSE(peers.Active(session));
        REQUIRE(peers.Pending(session));
        peers.Progress(session, 200);
        REQUIRE_FALSE(peers.Expire(session, 200));
        REQUIRE_FALSE(peers.Begin(session, 200, 4));
        REQUIRE_FALSE(peers.Complete(session));
        REQUIRE_FALSE(peers.Pending(session));
    }

    REQUIRE(peers.Begin(1, 200, 4));
}

TEST_CASE("Sync peers retire all only once without changing completed or already retired cooldowns",
    "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    peers.RetireAll(100);
    REQUIRE(peers.Size() == 0);
    REQUIRE(peers.Primary() == 0);
    for(uint64_t session = 1; session <= 3; ++session)
        REQUIRE(peers.Begin(session, 100, 4));

    peers.Pause(1, 100);
    REQUIRE(peers.Complete(2));
    peers.RetireAll(110);
    peers.RetireAll(120);
    REQUIRE_FALSE(peers.Complete(1));
    REQUIRE_FALSE(peers.Complete(3));
    REQUIRE_FALSE(peers.Begin(1, 129, 4));
    REQUIRE(peers.Begin(1, 130, 4));
    REQUIRE(peers.Begin(2, 130, 4));
    REQUIRE_FALSE(peers.Begin(3, 139, 4));
    REQUIRE(peers.Begin(3, 140, 4));
}

TEST_CASE("Sync peers rebase timeout and cooldown when clocks move backwards", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    REQUIRE(peers.Begin(1, 100, 1));
    REQUIRE_FALSE(peers.Expire(1, 50));
    REQUIRE_FALSE(peers.Expire(1, 109));
    REQUIRE(peers.Expire(1, 110));
    REQUIRE_FALSE(peers.Complete(1));
    REQUIRE_FALSE(peers.Begin(1, 10, 1));
    REQUIRE_FALSE(peers.Begin(1, 39, 1));
    REQUIRE(peers.Begin(1, 40, 1));
    peers.Progress(1, 0);
    REQUIRE_FALSE(peers.Expire(1, 59));
    REQUIRE(peers.Expire(1, 60));
}

TEST_CASE("Sync peers compare elapsed times without overflowing timestamps", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    const uint64_t maximum = std::numeric_limits<uint64_t>::max();
    REQUIRE(peers.Begin(1, maximum - 100, 1));
    REQUIRE_FALSE(peers.Expire(1, maximum - 41));
    REQUIRE(peers.Expire(1, maximum - 40));
    REQUIRE_FALSE(peers.Complete(1));
    REQUIRE_FALSE(peers.Begin(1, maximum - 11, 1));
    REQUIRE(peers.Begin(1, maximum - 10, 1));
    REQUIRE_FALSE(peers.Expire(1, maximum));
    peers.Pause(1, maximum - 10);
    REQUIRE_FALSE(peers.Complete(1));
    REQUIRE_FALSE(peers.Begin(1, maximum, 1));
    REQUIRE_FALSE(peers.Begin(1, 0, 1));
    REQUIRE_FALSE(peers.Begin(1, 29, 1));
    REQUIRE(peers.Begin(1, 30, 1));
    REQUIRE_FALSE(peers.Expire(1, maximum, maximum));
}

TEST_CASE("Sync peers reserve concurrent capacity atomically", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    std::atomic<bool> start{false};
    std::atomic<unsigned int> successes{0};
    std::vector<std::thread> workers;
    for(uint64_t session = 1; session <= 16; ++session)
    {
        workers.emplace_back([&, session]()
        {
            while(!start.load())
                std::this_thread::yield();

            if(peers.Begin(session, 100, 4))
                ++successes;
        });
    }

    start.store(true);
    for(auto& worker : workers)
        worker.join();

    REQUIRE(successes.load() == 4);
    REQUIRE(peers.Size() == 4);
    REQUIRE(peers.Active(peers.Primary()));
    unsigned int completed = 0;
    for(uint64_t session = 1; session <= 16; ++session)
    {
        if(peers.Complete(session))
            ++completed;
    }

    REQUIRE(completed == 4);
    REQUIRE(peers.Size() == 0);
}

TEST_CASE("Sync peers reject racing duplicate requests and duplicate completions", "[llp][sync_peer_state]")
{
    LLP::SyncPeerState peers;
    std::atomic<bool> start{false};
    std::atomic<unsigned int> successes{0};
    std::vector<std::thread> workers;
    for(unsigned int i = 0; i < 16; ++i)
    {
        workers.emplace_back([&]()
        {
            while(!start.load())
                std::this_thread::yield();

            if(peers.Begin(1, 100, 4))
                ++successes;
        });
    }

    start.store(true);
    for(auto& worker : workers)
        worker.join();

    REQUIRE(successes.load() == 1);
    REQUIRE(peers.Size() == 1);
    workers.clear();
    successes.store(0);
    start.store(false);
    for(unsigned int i = 0; i < 16; ++i)
    {
        workers.emplace_back([&]()
        {
            while(!start.load())
                std::this_thread::yield();

            peers.Progress(1, 101);
            if(peers.Complete(1))
                ++successes;
        });
    }

    start.store(true);
    for(auto& worker : workers)
        worker.join();

    REQUIRE(successes.load() == 1);
    REQUIRE(peers.Size() == 0);
    REQUIRE(peers.Primary() == 0);
}
