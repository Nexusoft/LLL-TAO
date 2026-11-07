#include <unit/catch2/catch.hpp>

#include <LLD/types/address.h>
#include <LLP/include/manager.h>
#include <LLP/include/peer_recovery.h>
#include <LLP/include/block_inventory_requests.h>
#include <LLP/include/peer_best_retry.h>
#include <LLP/include/network.h>
#include <LLP/include/global.h>
#include <LLP/templates/server.h>
#include <LLP/types/miner.h>
#include <LLP/types/time.h>
#include <LLP/types/tritium.h>
#include <Util/include/args.h>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include <openssl/ssl.h>

TEST_CASE("Block GET reservations expire without duplicate deadline extension",
    "[llp][peer-recovery][block-requests]")
{
    LLP::BlockInventoryRequests requests;
    const auto now = LLP::BlockInventoryRequests::Clock::now();
    int peer = 0, other = 0;
    const uint1024_t hash(1);
    REQUIRE(requests.Add(hash, &peer, now));
    REQUIRE_FALSE(requests.Add(hash, &peer, now + std::chrono::seconds(4)));
    REQUIRE(requests.Add(hash, &other, now + std::chrono::seconds(1)));
    REQUIRE(requests.Add(hash, &peer, now + LLP::BlockInventoryRequests::TIMEOUT));
    requests.Remove(hash, &peer);
    REQUIRE_FALSE(requests.Contains(hash, &peer, now + std::chrono::seconds(5)));
    REQUIRE(requests.Contains(hash, &other, now + std::chrono::seconds(5)));
    requests.RemovePeer(&other);
    REQUIRE_FALSE(requests.Contains(hash, &other, now + std::chrono::seconds(5)));
}

TEST_CASE("Block GET reservations bound hashes and peers and prune expired capacity",
    "[llp][peer-recovery][block-requests]")
{
    LLP::BlockInventoryRequests requests;
    const auto now = LLP::BlockInventoryRequests::Clock::now();
    int peers[LLP::BlockInventoryRequests::MAX_PEERS_PER_HASH + 1]{};
    for(size_t i = 0; i < LLP::BlockInventoryRequests::MAX_PEERS_PER_HASH; ++i)
        REQUIRE(requests.Add(uint1024_t(1), &peers[i], now));
    REQUIRE_FALSE(requests.Add(uint1024_t(1), &peers[LLP::BlockInventoryRequests::MAX_PEERS_PER_HASH], now));
    for(size_t i = 2; i <= LLP::BlockInventoryRequests::MAX_HASHES; ++i)
        REQUIRE(requests.Add(uint1024_t(i), &peers[0], now));
    REQUIRE_FALSE(requests.Add(uint1024_t(LLP::BlockInventoryRequests::MAX_HASHES + 1), &peers[0], now));
    REQUIRE(requests.Add(uint1024_t(LLP::BlockInventoryRequests::MAX_HASHES + 1),
        &peers[0], now + LLP::BlockInventoryRequests::TIMEOUT));
}

TEST_CASE("Concurrent block GET reservations deduplicate each peer independently",
    "[llp][peer-recovery][block-requests]")
{
    LLP::BlockInventoryRequests requests;
    int peers[2]{};
    const auto now = LLP::BlockInventoryRequests::Clock::now();
    std::atomic<unsigned int> added{0};
    std::vector<std::thread> workers;
    for(unsigned int i = 0; i < 16; ++i)
        workers.emplace_back([&, i]()
        {
            if(requests.Add(uint1024_t(1), &peers[i % 2], now))
                ++added;
        });
    for(auto& worker : workers)
        worker.join();
    REQUIRE(added.load() == 2);
    requests.RemovePeer(&peers[0]);
    REQUIRE(requests.Contains(uint1024_t(1), &peers[1], now));
}

TEST_CASE("Block retries do not expedite the branch recovery deadline",
    "[llp][peer-recovery][block-requests]")
{
    LLP::PeerBestRetry retry;
    const auto now = LLP::PeerBestRetry::Clock::now();
    REQUIRE_FALSE(retry.BlockDue(now));
    retry.Observe(true, now);
    for(unsigned int seconds = 1; seconds <= 30; ++seconds)
    {
        const auto tick = now + std::chrono::seconds(seconds);
        retry.Observe(false, tick);
        REQUIRE(retry.BlockDue(tick) == (seconds % 5 == 0));
        REQUIRE(retry.Due(tick) == (seconds == 30));
    }
    retry.Observe(true, now + std::chrono::seconds(31));
    REQUIRE_FALSE(retry.BlockDue(now + std::chrono::seconds(35)));
    REQUIRE(retry.BlockDue(now + std::chrono::seconds(36)));
    REQUIRE_FALSE(retry.Due(now + std::chrono::seconds(60)));
    REQUIRE_FALSE(retry.BlockDue(now + LLP::PeerBestRetry::MAX_AGE + std::chrono::seconds(32)));
    retry.Reset();
    REQUIRE_FALSE(retry.BlockDue(now + std::chrono::seconds(100)));
}

TEST_CASE("Failed and throwing direct block GETs release their reservations",
    "[llp][peer-recovery][block-requests]")
{
    struct FailingNode : LLP::TritiumNode
    {
        bool throwing{false};
        uint64_t GetMaxSendBuffer() const override
        {
            if(throwing)
                throw std::runtime_error("test send failure");
            return 0;
        }
    } node;
    const uint1024_t hash(0xBC01000000000161ULL);
    REQUIRE_FALSE(node.QueueBlockGet(hash));
    REQUIRE_FALSE(node.QueueBlockGet(hash));
    node.throwing = true;
    REQUIRE_THROWS(node.QueueBlockGet(hash));
    REQUIRE_THROWS(node.QueueBlockGet(hash));
    node.throwing = false;
    REQUIRE_FALSE(node.QueueBlockGet(hash));

    node.nProtocolVersion = LLP::PROTOCOL_VERSION;
    node.nCurrentSession = 1;
    node.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BLOCK);
    DataStream inventory(SER_NETWORK, LLP::PROTOCOL_VERSION);
    inventory << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << hash;
    node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
    node.INCOMING.DATA.assign(inventory.begin(), inventory.end());
    REQUIRE(node.ProcessPacket());
    REQUIRE_FALSE(node.QueueBlockGet(hash));

    inventory << uint8_t(0xff);
    node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
    node.INCOMING.DATA.assign(inventory.begin(), inventory.end());
    REQUIRE_FALSE(node.ProcessPacket());
    REQUIRE_FALSE(node.QueueBlockGet(hash));

    node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
    node.INCOMING.DATA.assign(inventory.begin(), inventory.end() - 1);
    node.throwing = true;
    REQUIRE_THROWS(node.ProcessPacket());
    node.throwing = false;
    REQUIRE_FALSE(node.QueueBlockGet(hash));

    LLP::TritiumNode invalidSocket;
    invalidSocket.fd = -1;
    const auto packets = invalidSocket.PACKETS.load();
    REQUIRE_FALSE(invalidSocket.QueueBlockGet(hash));
    REQUIRE_FALSE(invalidSocket.QueueBlockGet(hash));
    REQUIRE(invalidSocket.PACKETS.load() == packets);
}

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

TEST_CASE("Relay queues drain while producers enqueue concurrently", "[llp][peer-recovery]")
{
    LLP::DataThread<LLP::TimeNode> worker(0, false, 1, 1, 10);
    std::vector<std::thread> producers;
    for(unsigned int producer = 0; producer < 4; ++producer)
    {
        producers.emplace_back([&, producer] {
            for(uint32_t sequence = 0; sequence < 4096; ++sequence)
            {
                if(producer % 2 == 0)
                    worker.Relay(uint8_t(1), sequence);
                else
                {
                    DataStream data(SER_NETWORK, LLP::MIN_PROTO_VERSION);
                    data << sequence;
                    worker._Relay(uint8_t(1), data);
                }
            }
        });
    }
    for(auto& producer : producers)
        producer.join();

    bool drained = false;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while(!drained && std::chrono::steady_clock::now() < deadline)
    {
        {
            std::scoped_lock lock(worker.FLUSH_CONDITION_MUTEX);
            drained = worker.RELAY->empty();
        }
        if(!drained)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    REQUIRE(drained);
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
    const LLP::BaseAddress spare("127.0.0.2", port, false);
    server->GetAddressManager()->AddAddress(spare, LLP::ConnectState::NEW);
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
    server->GetAddressManager()->RemoveAddress(spare);
    server.reset();
    LLD::AddressDB database(port, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    REQUIRE(database.Erase(std::string("updated")));
}

TEST_CASE("A time peer disconnect updates its owning server's address manager",
          "[llp][peer-recovery]")
{
    const bool reconnect = GENERATE(false, true);
    struct Socket
    {
        int fd = -1;
        ~Socket() { if(fd >= 0) closesocket(fd); }
        void Close() { if(fd >= 0) { closesocket(fd); fd = -1; } }
    } listener, accepted;
    listener.fd = ::socket(AF_INET, SOCK_STREAM, 0);
    REQUIRE(listener.fd >= 0);
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    REQUIRE(::bind(listener.fd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == 0);
    socklen_t length = sizeof(local);
    REQUIRE(::getsockname(listener.fd, reinterpret_cast<sockaddr*>(&local), &length) == 0);
    REQUIRE(::listen(listener.fd, 4) == 0);
    const uint16_t port = ntohs(local.sin_port);
    const LLP::BaseAddress address("127.0.0.1", port, false);

    LLP::AddressManager* globalManager = LLP::TIME_SERVER
        ? LLP::TIME_SERVER->GetAddressManager() : nullptr;
    const bool globalHadAddress = globalManager && globalManager->Has(address);
    struct GlobalCleanup
    {
        LLP::AddressManager* manager;
        const LLP::BaseAddress& address;
        bool existed;
        ~GlobalCleanup() { if(manager && !existed) manager->RemoveAddress(address); }
    } globalCleanup{globalManager, address, globalHadAddress};

    LLP::Config config(port);
    config.ENABLE_LISTEN = false;
    config.ENABLE_MANAGER = true;
    config.ENABLE_METERS = false;
    config.MAX_INCOMING = 0;
    config.MAX_CONNECTIONS = 1;
    config.MAX_THREADS = 1;
    config.MANAGER_SLEEP = 1000;
    auto server = std::make_unique<LLP::Server<LLP::TimeNode>>(config,
        [&](const std::string&, std::vector<LLP::BaseAddress>& out) {
            out.push_back(address);
            return true;
        });

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    std::shared_ptr<LLP::TimeNode> peer;
    while(!(peer = server->RandomConnection()) && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(peer);
    accepted.fd = ::accept(listener.fd, nullptr, nullptr);
    REQUIRE(accepted.fd >= 0);

    LLP::TrustAddress state;
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while((!server->GetAddressManager()->Get(address, state) || state.nConnected == 0)
          && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(state.nConnected == 1);
    if(!reconnect)
    {
        server->RequestManagerStop();
        listener.Close();
    }
    REQUIRE(::shutdown(accepted.fd, SHUT_RDWR) == 0);
    accepted.Close();
    peer.reset();

    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while(server->GetAddressManager()->Get(address, state)
          && (state.nDropped == 0 || (reconnect && state.nConnected < 2))
          && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(state.nDropped == 1);
    if(reconnect)
    {
        REQUIRE(state.nConnected >= 2);
        REQUIRE((state.nState & LLP::ConnectState::CONNECTED) != 0);
    }
    else
        REQUIRE((state.nState & LLP::ConnectState::CONNECTED) == 0);
    if(globalManager && !globalHadAddress)
        REQUIRE_FALSE(globalManager->Has(address));

    server->RequestManagerStop();
    server.reset();
    LLD::AddressDB database(port, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    REQUIRE(database.EraseTrustAddress(address.GetHash()));
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

    std::vector<std::string> candidates{"127.0.0.2"};
    ifaddrs* rawInterfaces = nullptr;
    REQUIRE(::getifaddrs(&rawInterfaces) == 0);
    std::unique_ptr<ifaddrs, decltype(&freeifaddrs)> interfaces(rawInterfaces, freeifaddrs);
    for(const ifaddrs* interface = interfaces.get(); interface; interface = interface->ifa_next)
    {
        if(!interface->ifa_addr || interface->ifa_addr->sa_family != AF_INET
           || !(interface->ifa_flags & IFF_UP))
            continue;

        char buffer[INET_ADDRSTRLEN]{};
        const auto* address = reinterpret_cast<const sockaddr_in*>(interface->ifa_addr);
        REQUIRE(::inet_ntop(AF_INET, &address->sin_addr, buffer, sizeof(buffer)));
        const LLP::BaseAddress candidate(buffer, 1, false);
        if(candidate.IsValid() && !candidate.IsRoutable() && std::string(buffer) != "127.0.0.1")
            candidates.emplace_back(buffer);
    }

    std::string newAddress;
    for(const auto& candidate : candidates)
    {
        Socket listener, client;
        listener.fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if(listener.fd < 0)
            continue;
        sockaddr_in probe{};
        probe.sin_family = AF_INET;
        if(::inet_pton(AF_INET, candidate.c_str(), &probe.sin_addr) != 1
           || ::bind(listener.fd, reinterpret_cast<sockaddr*>(&probe), sizeof(probe)) != 0
           || ::listen(listener.fd, 1) != 0)
            continue;
        socklen_t probeLength = sizeof(probe);
        if(::getsockname(listener.fd, reinterpret_cast<sockaddr*>(&probe), &probeLength) != 0)
            continue;
        client.fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if(client.fd < 0 || ::fcntl(client.fd, F_SETFL, O_NONBLOCK) != 0)
            continue;
        if(::connect(client.fd, reinterpret_cast<sockaddr*>(&probe), sizeof(probe)) != 0)
        {
            if(errno != EINPROGRESS)
                continue;
            pollfd ready{client.fd, POLLOUT, 0};
            int error = 0;
            socklen_t errorLength = sizeof(error);
            if(::poll(&ready, 1, 1000) <= 0
               || ::getsockopt(client.fd, SOL_SOCKET, SO_ERROR, &error, &errorLength) != 0
               || error != 0)
                continue;
        }
        newAddress = candidate;
        break;
    }
    INFO("A second reachable local IPv4 address is required");
    REQUIRE_FALSE(newAddress.empty());

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
        out.emplace_back(changed.load() ? newAddress : "127.0.0.1", port, false);
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
    REQUIRE(::inet_pton(AF_INET, newAddress.c_str(), &local.sin_addr) == 1);
    REQUIRE(::bind(newListener.fd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == 0);
    REQUIRE(::listen(newListener.fd, 4) == 0);
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(50);
    bool recovered = false;
    while(std::chrono::steady_clock::now() < deadline)
    {
        peer = server->RandomConnection();
        if(peer && peer->GetAddress().ToStringIP() == newAddress)
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
    server->RequestManagerStop();
    server.reset();
    LLD::AddressDB database(port, LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
    REQUIRE(database.EraseTrustAddress(LLP::BaseAddress("127.0.0.1", port, false).GetHash()));
    REQUIRE(database.EraseTrustAddress(LLP::BaseAddress(newAddress, port, false).GetHash()));
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
    REQUIRE(server.ConnectNode(address.ToStringIP(), peer));
    peer = server.RandomConnection();
    REQUIRE(peer);
    REQUIRE(peer->Connected());
    REQUIRE(server.GetPeerRecoveryStatus().resolving);
    REQUIRE((server.GetAddressManager()->GetState(address) & LLP::ConnectState::CONNECTED) != 0);
    server.GetAddressManager()->RemoveAddress(address);
}
#endif
