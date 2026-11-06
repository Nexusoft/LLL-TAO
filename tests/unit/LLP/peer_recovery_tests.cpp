#include <unit/catch2/catch.hpp>

#include <LLD/types/address.h>
#include <LLP/include/manager.h>
#include <LLP/include/peer_recovery.h>
#include <LLP/include/network.h>
#include <LLP/templates/server.h>
#include <LLP/types/miner.h>
#include <LLP/types/time.h>
#include <Util/include/args.h>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <vector>

#include <openssl/ssl.h>

TEST_CASE("Peer recovery retries are bounded and suspendable", "[llp][peer-recovery]")
{
    LLP::PeerRecovery recovery;
    REQUIRE_FALSE(recovery.Due(0, false, false, true, true, true, false));
    REQUIRE_FALSE(recovery.Due(0, true, true, true, true, true, false));
    REQUIRE(recovery.Due(0, true, false, true, true, true, false));

    recovery.Completed(0, 500, false, false);
    REQUIRE(recovery.NextAttempt() == 15);
    REQUIRE_FALSE(recovery.Due(14, true, false, true, true, true, false));
    REQUIRE(recovery.Due(15, true, false, true, true, true, false));
    recovery.Completed(15, 500, false, false);
    REQUIRE(recovery.NextAttempt() == 45);

    for(unsigned int i = 0; i < 10; ++i)
    {
        const auto now = recovery.NextAttempt();
        recovery.Completed(now, 1000, false, false);
        REQUIRE(recovery.NextAttempt() - now <= 300);
    }

    const auto now = recovery.NextAttempt();
    recovery.Completed(now, 500, true, true);
    REQUIRE(recovery.NextAttempt() - now == 15);
}

TEST_CASE("Peer recovery jitter has bounded retry deadlines", "[llp][peer-recovery]")
{
    LLP::PeerRecovery minimum;
    LLP::PeerRecovery maximum;
    uint64_t minNow = 0;
    uint64_t maxNow = 0;
    for(unsigned int attempt = 0; attempt < 10; ++attempt)
    {
        minimum.Completed(minNow, 0, false, false);
        maximum.Completed(maxNow, 1000, false, false);
        const uint64_t base = std::min(uint64_t(300), uint64_t(15) << std::min(attempt, 5u));
        REQUIRE(minimum.NextAttempt() - minNow == base * 800 / 1000);
        REQUIRE(maximum.NextAttempt() - maxNow == std::min(uint64_t(300), base * 1200 / 1000));
        minNow = minimum.NextAttempt();
        maxNow = maximum.NextAttempt();
    }

    minimum.Completed(minNow, 0, true, true);
    maximum.Completed(maxNow, 1000, true, true);
    REQUIRE(minimum.NextAttempt() - minNow == 12);
    REQUIRE(maximum.NextAttempt() - maxNow == 18);
}

TEST_CASE("A recovered peer shortens the next recovery backoff", "[llp][peer-recovery]")
{
    LLP::PeerRecovery recovery;
    for(unsigned int i = 0; i < 6; ++i)
        recovery.Completed(recovery.NextAttempt(), 500, false, false);
    const uint64_t recoveredAt = recovery.NextAttempt() - 200;
    recovery.Observe(recoveredAt, true);
    REQUIRE(recovery.NextAttempt() == recoveredAt + 15);
    recovery.Completed(recoveredAt + 15, 500, true, false);
    const auto retryAfterFailure = recovery.NextAttempt();
    recovery.Observe(recoveredAt + 16, true);
    REQUIRE(recovery.NextAttempt() == retryAfterFailure);
}

TEST_CASE("Peer recovery detects prolonged degradation and respects capacity", "[llp][peer-recovery]")
{
    LLP::PeerRecovery recovery;
    REQUIRE_FALSE(recovery.Due(100, true, false, true, false, true, false));
    REQUIRE_FALSE(recovery.Due(129, true, false, true, false, true, false));
    REQUIRE_FALSE(recovery.Due(130, true, false, false, false, true, false));
    REQUIRE(recovery.Due(130, true, false, true, false, true, false));
    REQUIRE_FALSE(recovery.Due(131, true, true, true, false, true, true));
    REQUIRE_FALSE(recovery.Due(132, false, false, true, false, true, true));
    REQUIRE(recovery.Due(133, true, false, true, false, false, true));
}

TEST_CASE("Peer admission and selection ignore incoming limits and disconnected peers", "[llp][peer-recovery]")
{
    REQUIRE(LLP::CanAttemptOutgoing(0, 8));
    REQUIRE(LLP::CanAttemptOutgoing(7, 8));
    REQUIRE_FALSE(LLP::CanAttemptOutgoing(8, 8));
    REQUIRE_FALSE(LLP::CanAttemptOutgoing(0, 0));

    struct Peer
    {
        bool connected;
        bool Connected() const { return connected; }
    };
    auto active = std::make_shared<Peer>(Peer{true});
    auto other = std::make_shared<Peer>(Peer{true});
    auto disconnected = std::make_shared<Peer>(Peer{false});
    const std::vector<std::shared_ptr<Peer>> peers{nullptr, disconnected, active};
    REQUIRE(LLP::SelectConnectedPeer(peers, [](size_t) { return 0; }) == active);
    REQUIRE(LLP::SelectConnectedPeer(std::vector<std::shared_ptr<Peer>>{active, other},
                                    [](size_t) { return 1; }) == other);
    active->connected = false;
    REQUIRE_FALSE(LLP::SelectConnectedPeer(peers, [](size_t) { return 0; }));

    active->connected = true;
    REQUIRE_FALSE(LLP::SelectConnectedPeer(peers, [&](size_t) {
        active->connected = false;
        return 0;
    }));
}

TEST_CASE("Seed refresh retries failures and preserves live address state", "[llp][peer-recovery]")
{
    REQUIRE_FALSE(config::fSuspended.load());
    const uint16_t port = 54327;
    const LLP::BaseAddress first("198.51.100.10", port, false);
    const LLP::BaseAddress second("198.51.100.11", port, false);
    {
        LLP::AddressManager manager(port);
        LLP::PeerRecovery recovery;
        uint64_t updated = 0;
        REQUIRE_FALSE(manager.ReadLastDNSUpdate(updated));
        REQUIRE(recovery.Due(0, true, false, true, true, true, false));
        REQUIRE_FALSE(manager.RefreshSeedAddresses(true, [](const std::string&, std::vector<LLP::BaseAddress>&) {
            return false;
        }));
        REQUIRE_FALSE(manager.RefreshSeedAddresses(true, [](const std::string&, std::vector<LLP::BaseAddress>& out) {
            out.emplace_back();
            return true;
        }));
        recovery.Completed(0, 500, false, false);
        REQUIRE_FALSE(recovery.Due(14, true, false, true, true, true, false));
        REQUIRE(recovery.Due(15, true, false, true, true, true, false));
        REQUIRE_FALSE(manager.ReadLastDNSUpdate(updated));
        bool stopped = false;
        REQUIRE_FALSE(manager.RefreshSeedAddresses(true, [&](const std::string&, std::vector<LLP::BaseAddress>& out) {
            out.push_back(first);
            stopped = true;
            return true;
        }, [&] { return stopped; }));
        REQUIRE_FALSE(manager.ReadLastDNSUpdate(updated));
        REQUIRE_FALSE(manager.Has(first));

        {
            struct RestoreSuspended
            {
                ~RestoreSuspended() { config::fSuspended.store(false); }
            } restore;
            const bool result = manager.RefreshSeedAddresses(true, [&](const std::string&, std::vector<LLP::BaseAddress>& out) {
                out.push_back(first);
                config::fSuspended.store(true);
                return true;
            });
            REQUIRE_FALSE(result);
            REQUIRE_FALSE(manager.Has(first));
            REQUIRE_FALSE(manager.ReadLastDNSUpdate(updated));
        }

        manager.AddAddress(first, LLP::ConnectState::CONNECTED);
        manager.ReadDatabase();
        REQUIRE((manager.GetState(first) & LLP::ConnectState::CONNECTED) != 0);

        unsigned calls = 0;
        REQUIRE(manager.RefreshSeedAddresses(true, [&](const std::string&, std::vector<LLP::BaseAddress>& out) {
            if(calls++ != 0)
                return false;
            out.push_back(first);
            return true;
        }));
        REQUIRE(manager.ReadLastDNSUpdate(updated));
        REQUIRE(updated > 0);
        REQUIRE((manager.GetState(first) & LLP::ConnectState::CONNECTED) != 0);

        REQUIRE(manager.RefreshSeedAddresses(true, [&](const std::string&, std::vector<LLP::BaseAddress>& out) {
            out.push_back(second);
            return true;
        }));
        REQUIRE(manager.Has(first));
        REQUIRE(manager.Has(second));
        LLP::BaseAddress selected;
        REQUIRE(manager.StochasticSelect(selected));
        REQUIRE(selected == second);
    }
    {
        LLP::AddressManager manager(port);
        manager.ReadDatabase();
        REQUIRE(manager.Has(first));
        REQUIRE(manager.Has(second));
        REQUIRE((manager.GetState(first) & LLP::ConnectState::CONNECTED) == 0);
        REQUIRE(manager.EligibleCount() >= 2);
        manager.RemoveAddress(first);
        manager.RemoveAddress(second);
    }
    LLD::AddressDB database(port, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    REQUIRE(database.Erase(std::string("updated")));
}

#ifdef __linux__
TEST_CASE("A disabled connection manager does not resolve seeds", "[llp][peer-recovery]")
{
    std::atomic<unsigned> lookups{0};
    LLP::Config config(0);
    config.ENABLE_LISTEN = false;
    config.ENABLE_MANAGER = false;
    config.ENABLE_METERS = false;
    config.MAX_THREADS = 1;
    LLP::Server<LLP::TimeNode> server(config, [&](const std::string&, std::vector<LLP::BaseAddress>&) {
        ++lookups;
        return false;
    });
    server.RequestPeerRecovery();
    REQUIRE_FALSE(server.GetAddressManager());
    REQUIRE_FALSE(server.GetPeerRecoveryStatus().enabled);
    REQUIRE(lookups.load() == 0);
}

TEST_CASE("Idle server workers shut down reliably", "[llp][peer-recovery]")
{
    LLP::Config config(0);
    config.ENABLE_LISTEN = false;
    config.ENABLE_MANAGER = false;
    config.ENABLE_METERS = false;
    config.MAX_THREADS = 1;
    for(unsigned int i = 0; i < 64; ++i)
    {
        LLP::Server<LLP::TimeNode> server(config);
        REQUIRE_FALSE(server.GetAddressManager());
    }
    for(unsigned int i = 0; i < 16; ++i)
    {
        LLP::DataThread<LLP::Miner> worker(0, false, 1, 1, 10);
        REQUIRE_FALSE(worker.fDestruct.load());
    }
}

TEST_CASE("Stopping a manager discards an in-flight seed result", "[llp][peer-recovery]")
{
    const uint16_t port = 54328;
    const LLP::BaseAddress address("198.51.100.12", port, false);
    std::atomic<bool> entered{false};
    std::atomic<bool> release{false};
    std::atomic<unsigned> calls{0};
    LLP::Config config(port);
    config.ENABLE_LISTEN = false;
    config.ENABLE_MANAGER = true;
    config.ENABLE_METERS = false;
    config.MAX_THREADS = 1;
    auto server = std::make_unique<LLP::Server<LLP::TimeNode>>(config, [&](const std::string&, std::vector<LLP::BaseAddress>& out) {
        ++calls;
        entered.store(true);
        while(!release.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        out.push_back(address);
        return true;
    });
    struct ReleaseResolver
    {
        std::atomic<bool>& flag;
        ~ReleaseResolver() { flag.store(true); }
    } releaseResolver{release};

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while(!entered.load() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(entered.load());
    server->RequestManagerStop();
    release.store(true);
    server.reset();
    REQUIRE(calls.load() == 1);
    LLD::AddressDB database(port, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    LLP::TrustAddress stored;
    uint64_t updated = 0;
    REQUIRE_FALSE(database.ReadTrustAddress(address.GetHash(), stored));
    REQUIRE_FALSE(database.ReadLastUpdate(updated));
}

TEST_CASE("A running manager recovers after seed connectivity returns", "[llp][peer-recovery]")
{
    REQUIRE_FALSE(config::fSuspended.load());
    REQUIRE_FALSE(config::GetBoolArg("-nodns"));
    struct Listener
    {
        int fd = -1;
        ~Listener() { if(fd >= 0) closesocket(fd); }
    } listener;
    listener.fd = ::socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(listener.fd >= 0);
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = 0;
    REQUIRE(::bind(listener.fd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == 0);
    socklen_t length = sizeof(local);
    REQUIRE(::getsockname(listener.fd, reinterpret_cast<sockaddr*>(&local), &length) == 0);
    REQUIRE(::listen(listener.fd, 4) == 0);
    const uint16_t port = ntohs(local.sin_port);

    std::atomic<bool> online{false};
    std::atomic<unsigned> lookups{0};
    LLP::Config config(port);
    config.ENABLE_LISTEN = false;
    config.ENABLE_MANAGER = true;
    config.ENABLE_METERS = false;
    config.MAX_INCOMING = 0;
    config.MAX_CONNECTIONS = 1;
    config.MAX_THREADS = 1;
    config.MANAGER_SLEEP = 1000;
    auto server = std::make_unique<LLP::Server<LLP::TimeNode>>(config, [&](const std::string&, std::vector<LLP::BaseAddress>& out) {
        const bool available = online.load();
        ++lookups;
        if(!available)
            return false;
        out.emplace_back("127.0.0.1", port, false);
        out.emplace_back("127.0.0.2", port, false);
        return true;
    });
    server->RequestPeerRecovery();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while(lookups.load() < 2 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(lookups.load() >= 2);
    REQUIRE(server->GetConnectionCount() == 0);
    uint64_t updated = 0;
    REQUIRE_FALSE(server->GetAddressManager()->ReadLastDNSUpdate(updated));

    online.store(true);
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(25);
    while(!server->RandomConnection() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(server->GetConnectionCount() == 1);
    REQUIRE(server->RandomConnection());
    REQUIRE(server->GetAddressManager()->ReadLastDNSUpdate(updated));
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while(server->GetAddressManager()->EligibleCount() != 1 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(server->GetAddressManager()->EligibleCount() == 1);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    REQUIRE(server->GetConnectionCount() == 1);
    const auto status = server->GetPeerRecoveryStatus();
    REQUIRE(status.enabled);
    REQUIRE(status.lastSucceeded);
    REQUIRE(status.lastSuccess > 0);
    REQUIRE(status.outgoing == 1);
    REQUIRE(status.state == "at_capacity");
    server->GetAddressManager()->RemoveAddress(LLP::BaseAddress("127.0.0.1", port, false));
    server->GetAddressManager()->RemoveAddress(LLP::BaseAddress("127.0.0.2", port, false));
    server.reset();
    LLD::AddressDB database(port, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    REQUIRE(database.Erase(std::string("updated")));
}

TEST_CASE("A running manager recovers from a changed network path", "[llp][peer-recovery]")
{
    struct Socket
    {
        int fd = -1;
        ~Socket() { if(fd >= 0) closesocket(fd); }
        void Close() { if(fd >= 0) { closesocket(fd); fd = -1; } }
    } oldListener, newListener, accepted;

    oldListener.fd = ::socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(oldListener.fd >= 0);
    int reuse = 1;
    REQUIRE(::setsockopt(oldListener.fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == 0);
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    REQUIRE(::bind(oldListener.fd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == 0);
    socklen_t length = sizeof(local);
    REQUIRE(::getsockname(oldListener.fd, reinterpret_cast<sockaddr*>(&local), &length) == 0);
    REQUIRE(::listen(oldListener.fd, 4) == 0);
    const uint16_t port = ntohs(local.sin_port);

    std::atomic<bool> changed{false};
    std::atomic<unsigned> lookups{0};
    LLP::Config config(port);
    config.ENABLE_LISTEN = false;
    config.ENABLE_MANAGER = true;
    config.ENABLE_METERS = false;
    config.MAX_INCOMING = 0;
    config.MAX_CONNECTIONS = 1;
    config.MAX_THREADS = 1;
    config.MANAGER_SLEEP = 1000;
    auto server = std::make_unique<LLP::Server<LLP::TimeNode>>(config, [&](const std::string&, std::vector<LLP::BaseAddress>& out) {
        ++lookups;
        out.emplace_back(changed.load() ? "127.0.0.2" : "127.0.0.1", port, false);
        return true;
    });

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    while(!server->RandomConnection() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto peer = server->RandomConnection();
    REQUIRE(peer);
    REQUIRE(peer->GetAddress().ToStringIP() == "127.0.0.1");
    accepted.fd = ::accept(oldListener.fd, nullptr, nullptr);
    REQUIRE(accepted.fd >= 0);
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while(server->GetPeerRecoveryStatus().lastSuccess == 0 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(server->GetPeerRecoveryStatus().lastSuccess > 0);
    const unsigned beforeChange = lookups.load();

    changed.store(true);
    oldListener.Close();
    REQUIRE(::shutdown(accepted.fd, SHUT_RDWR) == 0);
    accepted.Close();
    peer.reset();
    newListener.fd = ::socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(newListener.fd >= 0);
    REQUIRE(::setsockopt(newListener.fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == 0);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    REQUIRE(::bind(newListener.fd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == 0);
    REQUIRE(::listen(newListener.fd, 4) == 0);
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(50);
    bool recovered = false;
    while(std::chrono::steady_clock::now() < deadline)
    {
        peer = server->RandomConnection();
        if(peer && peer->GetAddress().ToStringIP() == "127.0.0.2")
        {
            recovered = true;
            break;
        }
        peer.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    const auto status = server->GetPeerRecoveryStatus();
    INFO("state=" << status.state << " in=" << status.incoming << " out=" << status.outgoing
         << " eligible=" << status.eligible << " retry=" << status.retryIn
         << " addresses=" << server->GetAddressManager()->ToString());
    REQUIRE(recovered);
    REQUIRE(lookups.load() > beforeChange);
    REQUIRE(server->GetConnectionCount() == 1);
    peer.reset();
    server->GetAddressManager()->RemoveAddress(LLP::BaseAddress("127.0.0.1", port, false));
    server->GetAddressManager()->RemoveAddress(LLP::BaseAddress("127.0.0.2", port, false));
    server.reset();
    LLD::AddressDB database(port, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    REQUIRE(database.Erase(std::string("updated")));
}

TEST_CASE("Connection flush producers share the wait mutex", "[llp][peer-recovery]")
{
    std::mutex mutex;
    std::condition_variable condition;
    LLP::TimeNode peer;
    peer.FLUSH_CONDITION = &condition;
    peer.FLUSH_CONDITION_MUTEX = &mutex;
    LLP::Packet packet;
    packet.HEADER = 0;
    packet.DATA.resize(256 * 1024, 1);
    packet.LENGTH = packet.DATA.size();

    bool queued = true;
    SECTION("Deferred packets") { queued = true; }
    SECTION("Buffered writes") { queued = false; }

    struct Socket
    {
        int fd = -1;
        ~Socket() { if(fd >= 0) closesocket(fd); }
    } reader;
    int sockets[2];
    REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    reader.fd = sockets[0];
    peer.fd = sockets[1];
    const int size = 4096;
    REQUIRE(::setsockopt(peer.fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) == 0);
    REQUIRE(::fcntl(peer.fd, F_SETFL, O_NONBLOCK) == 0);

    std::unique_lock<std::mutex> lock(mutex);
    std::promise<void> started;
    auto ready = started.get_future();
    auto producer = std::async(std::launch::async, [&] {
        started.set_value();
        if(queued)
            peer.QueuePacket(packet);
        else
            peer.WritePacket(packet);
    });
    ready.wait();
    const bool blocked = producer.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout;
    const bool pendingBeforeUnlock = peer.HasQueuedPackets() || peer.Buffered();
    lock.unlock();
    producer.get();
    REQUIRE(blocked);
    REQUIRE_FALSE(pendingBeforeUnlock);
    REQUIRE((peer.HasQueuedPackets() || peer.Buffered()));
}

TEST_CASE("Outgoing TLS requires a completed handshake", "[llp][peer-recovery][ssl]")
{
    bool complete = false;
    bool invalid = false;
    SECTION("A silent peer times out") {}
    SECTION("A plaintext peer fails") { invalid = true; }
    SECTION("A TLS peer completes") { complete = true; }

    struct Socket
    {
        int fd = -1;
        ~Socket() { if(fd >= 0) closesocket(fd); }
    } listener;
    listener.fd = ::socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(listener.fd >= 0);
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    REQUIRE(::bind(listener.fd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == 0);
    socklen_t length = sizeof(local);
    REQUIRE(::getsockname(listener.fd, reinterpret_cast<sockaddr*>(&local), &length) == 0);
    REQUIRE(::listen(listener.fd, 1) == 0);
    REQUIRE(LLP::pSSL_CTX != nullptr);

    std::atomic<bool> release{false};
    auto responder = std::async(std::launch::async, [&] {
        pollfd waiting{};
        waiting.fd = listener.fd;
        waiting.events = POLLIN;
        if(::poll(&waiting, 1, 5000) <= 0)
            return false;
        Socket accepted;
        accepted.fd = ::accept(listener.fd, nullptr, nullptr);
        if(accepted.fd < 0)
            return false;
        const timeval timeout{5, 0};
        if(::setsockopt(accepted.fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0
        || ::setsockopt(accepted.fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0)
            return false;

        bool success = true;
        if(complete)
        {
            std::unique_ptr<SSL, decltype(&SSL_free)> ssl(SSL_new(LLP::pSSL_CTX), SSL_free);
            if(!ssl)
                return false;
            SSL_set_fd(ssl.get(), accepted.fd);
            success = SSL_accept(ssl.get()) == 1;
        }
        else if(invalid)
        {
            const char response[] = "not a TLS handshake";
            success = ::send(accepted.fd, response, sizeof(response), 0) > 0;
        }
        while(!release.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return success;
    });
    struct ReleasePeer
    {
        std::atomic<bool>& flag;
        ~ReleasePeer() { flag.store(true); }
    } releasePeer{release};

    LLP::Socket peer;
    peer.SetSSL(true);
    const bool connected = peer.Attempt(LLP::BaseAddress("127.0.0.1", ntohs(local.sin_port), false), 1000);
    const bool encrypted = peer.IsSSL();
    release.store(true);
    REQUIRE(responder.get());
    REQUIRE(connected == complete);
    REQUIRE(encrypted == complete);
    if(!complete)
        REQUIRE(peer.IsNull());
}

TEST_CASE("A slow resolver does not block known-peer connections", "[llp][peer-recovery]")
{
    struct Listener
    {
        int fd = -1;
        ~Listener() { if(fd >= 0) closesocket(fd); }
    } listener;
    listener.fd = ::socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(listener.fd >= 0);
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    REQUIRE(::bind(listener.fd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == 0);
    socklen_t length = sizeof(local);
    REQUIRE(::getsockname(listener.fd, reinterpret_cast<sockaddr*>(&local), &length) == 0);
    REQUIRE(::listen(listener.fd, 4) == 0);
    const uint16_t port = ntohs(local.sin_port);

    std::atomic<bool> entered{false};
    std::atomic<bool> release{false};
    LLP::Config config(port);
    config.ENABLE_LISTEN = false;
    config.ENABLE_MANAGER = true;
    config.ENABLE_METERS = false;
    config.MAX_INCOMING = 0;
    config.MAX_CONNECTIONS = 1;
    config.MAX_THREADS = 1;
    LLP::Server<LLP::TimeNode> server(config, [&](const std::string&, std::vector<LLP::BaseAddress>&) {
        entered.store(true);
        while(!release.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return false;
    });
    struct ReleaseResolver
    {
        std::atomic<bool>& flag;
        ~ReleaseResolver() { flag.store(true); }
    } releaseResolver{release};
    server.RequestPeerRecovery();
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while(!entered.load() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(entered.load());

    const LLP::BaseAddress address("127.0.0.1", port, false);
    server.GetAddressManager()->AddAddress(address);
    std::shared_ptr<LLP::TimeNode> peer;
    REQUIRE(server.ConnectNode(address.ToString(), peer));
    peer = server.RandomConnection();
    REQUIRE(peer);
    REQUIRE(peer->Connected());
    REQUIRE(server.GetPeerRecoveryStatus().resolving);
    REQUIRE((server.GetAddressManager()->GetState(address) & LLP::ConnectState::CONNECTED) != 0);
    server.GetAddressManager()->RemoveAddress(address);
}
#endif
