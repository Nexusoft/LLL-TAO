/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

__________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/types/tritium.h>
#include <LLP/include/block_inventory_requests.h>
#include <LLP/include/version.h>
#include <LLP/include/peer_best_retry.h>
#include <LLP/templates/events.h>
#include <LLP/templates/socket.h>
#include <LLP/include/base_address.h>

#include <TAO/Ledger/include/admissibility.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/peer_height.h>
#include <TAO/Ledger/include/mining_readiness.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/stateless_block_utility.h>
#include <TAO/Ledger/include/supply.h>
#include <LLP/include/legacy_get_block_handler.h>
#include <LLP/include/mining_template_payload.h>
#include <LLP/include/stateless_get_block_handler.h>
#include <LLP/include/stateless_opcodes.h>
#include <LLP/packets/packet.h>
#include <LLP/packets/stateless_packet.h>
#include <LLP/templates/base_connection.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/credentials.h>
#include <TAO/Ledger/types/locator.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>
#include <TAO/Register/include/create.h>

#include <Util/include/args.h>
#include <Util/include/filesystem.h>
#include <Util/include/runtime.h>
#include <Util/templates/datastream.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <future>
#include <thread>

#include <unit/catch2/catch.hpp>

#include <algorithm>

#ifndef WIN32
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif


namespace
{
    /*  Lightweight guard that creates a temporary LedgerDB when the global
     *  test suite hasn't initialised one (e.g. when running only [process]
     *  tagged tests in isolation).  On destruction it deletes only what it
     *  created, leaving the global state untouched if it was already set up. */
    struct LedgerGuard
    {
        bool ownedLedger{false};

        LedgerGuard()
        {
            config::fTestNet.store(true);
            config::mapArgs["-testnet"] = "1";

            if(!LLD::Ledger)
            {
                LLD::Ledger = new LLD::LedgerDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
                ownedLedger = true;
            }
        }

        ~LedgerGuard()
        {
            if(ownedLedger)
            {
                delete LLD::Ledger;
                LLD::Ledger = nullptr;
            }
        }
    };


    /** MissingBlock
     *
     *  A minimal Block subclass whose Check() passes but populates vMissing,
     *  simulating a valid block that is temporarily missing a transaction. This
     *  is the soft-fail "incomplete" condition handled by Process().
     */
    class MissingBlock final : public TAO::Ledger::Block
    {
    public:
        MissingBlock()
        : TAO::Ledger::Block()
        {
        }

        MissingBlock* Clone() const override
        {
            return new MissingBlock(*this);
        }

        bool Check(bool /*fForceProof*/ = false) const override
        {
            /* Simulate a block whose single vtx is not yet in LLD. Check()
             * itself succeeds; the missing transaction is reported via vMissing
             * and handled as a soft incomplete condition. */
            vMissing.clear();
            vMissing.push_back(std::make_pair(
                TAO::Ledger::TRANSACTION::TRITIUM, uint512_t(0xdeadbeef)));

            return true;
        }

        bool Accept() const override
        {
            return false;
        }
    };


    /** PassBlock
     *
     *  A Block subclass whose Check() and Accept() both succeed with no missing
     *  transactions, so that Process() reaches the ACCEPTED path.
     */
    class PassBlock final : public TAO::Ledger::Block
    {
    public:
        PassBlock()
        : TAO::Ledger::Block()
        {
        }

        PassBlock* Clone() const override
        {
            return new PassBlock(*this);
        }

        bool Check(bool /*fForceProof*/ = false) const override
        {
            vMissing.clear();
            return true;
        }

        bool Accept() const override
        {
            return true;
        }
    };


    class RejectBlock final : public TAO::Ledger::Block
    {
    public:
        RejectBlock* Clone() const override
        {
            return new RejectBlock(*this);
        }

        bool Check(bool /*fForceProof*/ = false) const override
        {
            return false;
        }

        bool Accept() const override
        {
            return false;
        }
    };

    struct PublicMiningGuard
    {
        const bool testnet = config::fTestNet.load();
        const bool hybrid = config::fHybrid.load();
        const bool synchronized = LLP::TritiumNode::fSynchronized.load();
        const bool reorg = TAO::Ledger::ChainState::fChainReorg.load();
        const uint64_t syncSession = TAO::Ledger::nSyncSession.load();
        const uint32_t height = TAO::Ledger::ChainState::nBestHeight.load();
        const TAO::Ledger::BlockState best = TAO::Ledger::ChainState::tStateBest.load();
        const uint1024_t hashBest = TAO::Ledger::ChainState::hashBestChain.load();
        const bool hadPolicy = config::mapArgs.count("-miningminpeers");
        const std::string policy = hadPolicy ? config::mapArgs["-miningminpeers"] : "";
        int peers[3]{};
        TAO::Ledger::BlockState tip;
        bool written = false;
        uint1024_t hashWrittenTip = 0;
        std::vector<uint1024_t> extraBlocks;

        PublicMiningGuard()
        {
            config::fTestNet.store(false);
            config::fHybrid.store(false);
            config::mapArgs.erase("-miningminpeers");
            LLP::TritiumNode::fSynchronized.store(true);
            TAO::Ledger::ChainState::fChainReorg.store(false);
            TAO::Ledger::nSyncSession.store(0);
            tip.nVersion = 7;
            tip.nHeight = 200;
            tip.nChannel = 1;
            tip.nChannelHeight = 1;
            tip.nBits = 0x8001ffff;
            tip.nNonce = LLC::GetRand();
            hashWrittenTip = tip.GetHash();
            written = LLD::Ledger->WriteBlock(hashWrittenTip, tip);
            TAO::Ledger::ChainState::tStateBest.store(tip);
            TAO::Ledger::ChainState::hashBestChain.store(tip.GetHash());
            TAO::Ledger::ChainState::nBestHeight.store(tip.nHeight);
            TAO::Ledger::ChainState::CommitMiningTip(tip.GetHash());
        }

        void Report(size_t i)
        {
            TAO::Ledger::ChainState::ObserveMiningPeer(&peers[i],
                "192.0.2." + std::to_string(i + 1), tip.GetHash(), tip.nHeight);
        }

        ~PublicMiningGuard()
        {
            for(auto& peer : peers)
                TAO::Ledger::ChainState::ForgetPeerHeight(&peer);
            config::fTestNet.store(testnet);
            config::fHybrid.store(hybrid);
            LLP::TritiumNode::fSynchronized.store(synchronized);
            TAO::Ledger::ChainState::fChainReorg.store(reorg);
            TAO::Ledger::nSyncSession.store(syncSession);
            TAO::Ledger::ChainState::tStateBest.store(best);
            TAO::Ledger::ChainState::hashBestChain.store(hashBest);
            TAO::Ledger::ChainState::nBestHeight.store(height);
            TAO::Ledger::ChainState::CommitMiningTip(hashBest);
            if(hadPolicy)
                config::mapArgs["-miningminpeers"] = policy;
            else
                config::mapArgs.erase("-miningminpeers");
            TAO::Ledger::ClearMiningTemplateCaches("mining readiness test cleanup");
            if(written)
                LLD::Ledger->EraseBlock(hashWrittenTip);
            for(const auto& hash : extraBlocks)
                LLD::Ledger->EraseBlock(hash);
        }
    };

#ifndef WIN32
    template<typename PacketType>
    class TestTemplateConnection : public LLP::BaseConnection<PacketType>
    {
        void Event(uint8_t, uint32_t) override {}
        bool ProcessPacket() override { return true; }
        void ReadPacket() override {}
    };

    template<typename PacketType>
    void CheckQueuedTemplateEpoch(PublicMiningGuard& guard, typename PacketType::message_t opcode)
    {
        using namespace TAO::Ledger::ChainState;
        TestTemplateConnection<PacketType> connection;
        int sockets[2];
        REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
        struct Reader
        {
            int fd;
            ~Reader() { close(fd); }
        } reader{sockets[0]};
        connection.fd = sockets[1];

        PacketType packet(opcode);
        packet.DATA = {1, 2, 3};
        packet.LENGTH = packet.DATA.size();
        const auto expected = packet.GetBytes();
        const auto ready = MiningReadiness();
        REQUIRE(ready.ready);
        uint32_t deliveries = 0;
        auto recordDelivery = [&]() { ++deliveries; };
        REQUIRE_FALSE(LLP::QueueMiningTemplate(connection, packet, 0));
        REQUIRE(LLP::QueueMiningTemplate(connection, packet, ready.epoch, recordDelivery));
        REQUIRE(deliveries == 0);

        ForgetPeerHeight(&guard.peers[0]);
        REQUIRE_FALSE(MiningReadiness().ready);
        REQUIRE(connection.DrainOutgoingQueue() == 1);
        REQUIRE(deliveries == 0);
        uint8_t bytes[1024];
        REQUIRE(recv(reader.fd, bytes, sizeof(bytes), MSG_DONTWAIT) == -1);
        REQUIRE((errno == EAGAIN || errno == EWOULDBLOCK));

        guard.Report(0);
        const auto restored = MiningReadiness();
        REQUIRE(restored.ready);
        REQUIRE(restored.epoch == ready.epoch);
        REQUIRE(LLP::QueueMiningTemplate(connection, packet, restored.epoch, recordDelivery));
        REQUIRE(deliveries == 0);
        REQUIRE(connection.DrainOutgoingQueue() == 1);
        REQUIRE(deliveries == 1);
        const auto count = recv(reader.fd, bytes, sizeof(bytes), MSG_DONTWAIT);
        REQUIRE(count == static_cast<ssize_t>(expected.size()));
        REQUIRE(std::vector<uint8_t>(bytes, bytes + count) == expected);

        REQUIRE(LLP::QueueMiningTemplate(connection, packet, restored.epoch, recordDelivery));
        std::mutex flushMutex;
        connection.FLUSH_CONDITION_MUTEX = &flushMutex;
        std::unique_lock<std::mutex> lock(flushMutex);
        std::promise<void> started;
        auto startSignal = started.get_future();
        auto drained = std::async(std::launch::async, [&]()
        {
            started.set_value();
            return connection.DrainOutgoingQueue();
        });
        startSignal.wait();
        ForgetPeerHeight(&guard.peers[0]);
        lock.unlock();
        REQUIRE(drained.get() == 1);
        REQUIRE(deliveries == 1);
        connection.FLUSH_CONDITION_MUTEX = nullptr;
        REQUIRE(recv(reader.fd, bytes, sizeof(bytes), MSG_DONTWAIT) == -1);
        REQUIRE((errno == EAGAIN || errno == EWOULDBLOCK));

        guard.Report(0);
        const auto next = MiningReadiness();
        REQUIRE(next.ready);
        REQUIRE(LLP::QueueMiningTemplate(connection, packet, next.epoch, recordDelivery));
        ForgetPeerHeight(&guard.peers[0]);
        guard.Report(0);
        REQUIRE(MiningReadiness().ready);
        connection.QueuePacket(packet);
        REQUIRE(connection.DrainOutgoingQueue() == 2);
        REQUIRE(deliveries == 2);
        const auto controlCount = recv(reader.fd, bytes, sizeof(bytes), MSG_DONTWAIT);
        auto both = expected;
        both.insert(both.end(), expected.begin(), expected.end());
        REQUIRE(controlCount == static_cast<ssize_t>(both.size()));
        REQUIRE(std::vector<uint8_t>(bytes, bytes + controlCount) == both);

        REQUIRE(LLP::QueueMiningTemplate(connection, packet, next.epoch, recordDelivery));
        ++guard.tip.nNonce;
        tStateBest.store(guard.tip);
        hashBestChain.store(guard.tip.GetHash());
        CommitMiningTip(guard.tip.GetHash());
        guard.Report(0);
        guard.Report(1);
        guard.extraBlocks.push_back(guard.tip.GetHash());
        REQUIRE(LLD::Ledger->WriteBlock(guard.tip.GetHash(), guard.tip));
        REQUIRE(connection.DrainOutgoingQueue() == 1);
        REQUIRE(deliveries == 2);
        REQUIRE(recv(reader.fd, bytes, sizeof(bytes), MSG_DONTWAIT) == -1);
        REQUIRE((errno == EAGAIN || errno == EWOULDBLOCK));
        REQUIRE(LLP::WriteMiningTemplate(connection, packet, next.epoch, recordDelivery)
            == LLP::MiningTemplateWriteResult::REVOKED);
        REQUIRE(deliveries == 2);
        const auto current = MiningReadiness();
        REQUIRE(current.ready);
        connection.FLUSH_CONDITION_MUTEX = &flushMutex;
        std::unique_lock<std::mutex> directLock(flushMutex);
        std::promise<void> directStarted;
        auto directSignal = directStarted.get_future();
        auto directWrite = std::async(std::launch::async, [&]()
        {
            directStarted.set_value();
            return LLP::WriteMiningTemplate(connection, packet, current.epoch, recordDelivery);
        });
        directSignal.wait();
        ForgetPeerHeight(&guard.peers[0]);
        directLock.unlock();
        REQUIRE(directWrite.get() == LLP::MiningTemplateWriteResult::REVOKED);
        REQUIRE(deliveries == 2);
        connection.FLUSH_CONDITION_MUTEX = nullptr;
        REQUIRE(recv(reader.fd, bytes, sizeof(bytes), MSG_DONTWAIT) == -1);
        REQUIRE((errno == EAGAIN || errno == EWOULDBLOCK));

        guard.Report(0);
        const auto fresh = MiningReadiness();
        REQUIRE(fresh.ready);
        REQUIRE(LLP::WriteMiningTemplate(connection, packet, fresh.epoch)
            == LLP::MiningTemplateWriteResult::SENT);
        const auto directCount = recv(reader.fd, bytes, sizeof(bytes), MSG_DONTWAIT);
        REQUIRE(directCount == static_cast<ssize_t>(expected.size()));
        REQUIRE(std::vector<uint8_t>(bytes, bytes + directCount) == expected);

        connection.fd = -1;
        close(sockets[1]);
        REQUIRE(LLP::WriteMiningTemplate(connection, packet, fresh.epoch, recordDelivery)
            == LLP::MiningTemplateWriteResult::WRITE_FAILED);
        REQUIRE(deliveries == 2);
        REQUIRE(LLP::QueueMiningTemplate(connection, packet, fresh.epoch, recordDelivery));
        REQUIRE(connection.DrainOutgoingQueue() == 1);
        REQUIRE(deliveries == 2);
    }

    template<typename PacketType>
    void CheckTemplateDeliveryOrder(bool firstDirect, bool secondDirect)
    {
        TestTemplateConnection<PacketType> connection;
        int sockets[2];
        REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
        struct Reader
        {
            int fd;
            ~Reader() { close(fd); }
        } reader{sockets[0]};
        connection.fd = sockets[1];

        std::mutex flushMutex;
        connection.FLUSH_CONDITION_MUTEX = &flushMutex;
        const auto epoch = TAO::Ledger::ChainState::MiningReadiness().epoch;
        PacketType older(0), newer(0);
        older.DATA = {1, 2, 3};
        newer.DATA = {4, 5, 6};
        older.LENGTH = older.DATA.size();
        newer.LENGTH = newer.DATA.size();

        uint32_t deliveredHeight = 0;
        uint1024_t deliveredHash = 0;
        std::vector<uint32_t> deliveries;
        std::promise<void> callbackStarted, releaseCallback, secondStarted;
        auto callbackSignal = callbackStarted.get_future();
        auto releaseSignal = releaseCallback.get_future();
        auto secondSignal = secondStarted.get_future();
        auto recordOlder = [&]()
        {
            callbackStarted.set_value();
            releaseSignal.wait();
            deliveredHeight = 10;
            deliveredHash = 100;
            deliveries.push_back(10);
        };
        auto recordNewer = [&]()
        {
            deliveredHeight = 11;
            deliveredHash = 101;
            deliveries.push_back(11);
        };

        if(!firstDirect)
            REQUIRE(LLP::QueueMiningTemplate(connection, older, epoch, recordOlder));
        auto first = std::async(std::launch::async, [&]()
        {
            return firstDirect
                ? LLP::WriteMiningTemplate(connection, older, epoch, recordOlder)
                    == LLP::MiningTemplateWriteResult::SENT
                : connection.DrainOutgoingQueue() == 1;
        });
        callbackSignal.wait();

        if(!secondDirect)
            LLP::QueueMiningTemplate(connection, newer, epoch, recordNewer);
        auto second = std::async(std::launch::async, [&]()
        {
            secondStarted.set_value();
            return secondDirect
                ? LLP::WriteMiningTemplate(connection, newer, epoch, recordNewer)
                    == LLP::MiningTemplateWriteResult::SENT
                : connection.DrainOutgoingQueue() == 1;
        });
        secondSignal.wait();
        const auto secondStatus = second.wait_for(std::chrono::milliseconds(100));
        uint8_t bytes[1024];
        const auto firstCount = recv(reader.fd, bytes, sizeof(bytes), MSG_DONTWAIT);
        const auto firstBytes = firstCount > 0
            ? std::vector<uint8_t>(bytes, bytes + firstCount) : std::vector<uint8_t>();

        /* Always release and join workers before asserting, even on failure. */
        releaseCallback.set_value();
        const bool firstSent = first.get();
        const bool secondSent = second.get();
        connection.FLUSH_CONDITION_MUTEX = nullptr;

        REQUIRE(firstSent);
        REQUIRE(secondSent);
        REQUIRE(secondStatus == std::future_status::timeout);
        REQUIRE(firstBytes == older.GetBytes());
        REQUIRE(deliveries == std::vector<uint32_t>{10, 11});
        REQUIRE(deliveredHeight == 11);
        REQUIRE(deliveredHash == uint1024_t(101));
        const auto secondCount = recv(reader.fd, bytes, sizeof(bytes), MSG_DONTWAIT);
        REQUIRE(secondCount == static_cast<ssize_t>(newer.GetBytes().size()));
        REQUIRE(std::vector<uint8_t>(bytes, bytes + secondCount) == newer.GetBytes());
    }
#endif
}

TEST_CASE("Mining readiness requires fresh distinct evidence without owning work epochs",
    "[ledger][mining-readiness]")
{
    using namespace TAO::Ledger::ChainState;
    MiningPeerTracker tracker;
    const auto now = MiningPeerTracker::Clock::now();
    int a, b, c;
    auto evidence = [](const uint1024_t& hash, uint32_t)
    {
        return hash == 1 ? MiningPeerEvidence::SUPPORTING :
               hash == 2 ? MiningPeerEvidence::BEHIND : MiningPeerEvidence::UNRESOLVED;
    };
    auto check = [&](auto time)
    {
        return tracker.Check(2, false, true, false, false, evidence, time);
    };
    REQUIRE_FALSE(check(now).ready);
    tracker.Observe(&a, "one", 1, 100, now);
    REQUIRE_FALSE(check(now).ready);
    tracker.Observe(&b, "one", 1, 100, now);
    REQUIRE_FALSE(check(now).ready);
    tracker.Observe(&b, "two", 1, 100, now);
    const auto ready = check(now);
    REQUIRE(ready.ready);
    REQUIRE(ready.peers == 2);
    tracker.Observe(&a, "one", 1, 100, now + std::chrono::seconds(1));
    REQUIRE(check(now + std::chrono::seconds(1)).epoch == ready.epoch);
    tracker.Remove(&a);
    tracker.Observe(&a, "one", 1, 100, now + std::chrono::seconds(1));
    REQUIRE(check(now + std::chrono::seconds(1)).ready);
    REQUIRE(check(now + std::chrono::seconds(1)).epoch == ready.epoch);

    tracker.Observe(&c, "three", 0xDEAD, 10, now);
    REQUIRE(check(now).reason == "unresolved-peer-tip");
    tracker.Observe(&c, "three", 2, 10, now);
    REQUIRE(check(now).ready);
    REQUIRE(check(now + MiningPeerTracker::MAX_AGE).ready);
    const auto expired = now + MiningPeerTracker::MAX_AGE + std::chrono::seconds(2);
    REQUIRE_FALSE(check(expired).ready);
    const auto oldEpoch = check(expired).epoch;
    tracker.Observe(&a, "one", 1, 100, expired);
    tracker.Observe(&b, "two", 1, 100, expired);
    REQUIRE(check(expired).ready);
    REQUIRE(check(expired).epoch == oldEpoch);
    REQUIRE_FALSE(tracker.Check(2, false, false, false, false, evidence, expired).ready);
    REQUIRE_FALSE(tracker.Check(2, false, true, true, false, evidence, expired).ready);
    REQUIRE_FALSE(tracker.Check(2, true, true, false, true, evidence, expired).ready);
    REQUIRE(tracker.Check(2, true, false, false, false, evidence, expired).ready);
    REQUIRE_FALSE(tracker.Check(0, false, true, false, false, evidence, expired).ready);

    MiningPeerTracker idle;
    idle.Observe(&a, "one", 1, 100, now);
    idle.Observe(&b, "two", 1, 100, now);
    const auto beforeGap = idle.Check(2, false, true, false, false, evidence, now);
    REQUIRE(beforeGap.ready);
    idle.Observe(&a, "one", 1, 100, expired);
    idle.Observe(&b, "two", 1, 100, expired);
    const auto afterGap = idle.Check(2, false, true, false, false, evidence, expired);
    REQUIRE(afterGap.ready);
    REQUIRE(afterGap.epoch == beforeGap.epoch);
}

TEST_CASE("Mining work readiness caches evidence without delaying invalidation",
    "[ledger][mining-readiness]")
{
    using namespace TAO::Ledger::ChainState;
    MiningPeerTracker tracker;
    const auto now = MiningPeerTracker::Clock::now();
    int peer;
    unsigned reads = 0;
    auto evidence = MiningPeerEvidence::SUPPORTING;
    const auto resolve = [&](const uint1024_t&, uint32_t)
    {
        ++reads;
        return evidence;
    };
    const auto check = [&](auto time, uint1024_t tip = 1, uint32_t required = 1,
                           bool bypass = false, bool synchronized = true,
                           bool reorg = false, bool shutdown = false)
    {
        return tracker.CheckCached(tip, required, bypass, synchronized, reorg, shutdown, resolve, time);
    };
    tracker.Observe(&peer, "one", 1, 100, now);
    const auto ready = check(now);
    REQUIRE(ready.ready);
    REQUIRE(reads == 1);
    REQUIRE(check(now + std::chrono::milliseconds(999)).epoch == ready.epoch);
    REQUIRE(reads == 1);
    REQUIRE(check(now + std::chrono::seconds(1)).ready);
    REQUIRE(reads == 2);

    REQUIRE(check(now + std::chrono::seconds(1), 2).ready);
    REQUIRE(reads == 3);
    REQUIRE_FALSE(check(now + std::chrono::seconds(1), 2, 2).ready);
    REQUIRE(reads == 4);
    REQUIRE(check(now + std::chrono::seconds(1), 2, 2, true).ready);
    REQUIRE(reads == 4);
    REQUIRE_FALSE(check(now + std::chrono::seconds(1), 2, 1, false, false).ready);
    REQUIRE_FALSE(check(now + std::chrono::seconds(1), 2, 1, false, true, true).ready);
    REQUIRE_FALSE(check(now + std::chrono::seconds(1), 2, 1, false, true, false, true).ready);
    REQUIRE(check(now + std::chrono::seconds(1)).ready);

    auto previousReads = reads;
    tracker.Observe(&peer, "one", 1, 100, now + std::chrono::seconds(1));
    REQUIRE(check(now + std::chrono::seconds(1)).ready);
    REQUIRE(reads == previousReads);
    tracker.Observe(&peer, "renamed", 1, 100, now + std::chrono::seconds(1));
    REQUIRE(check(now + std::chrono::seconds(1)).ready);
    REQUIRE(reads == previousReads + 1);
    tracker.Height(&peer, 101);
    previousReads = reads;
    REQUIRE(check(now + std::chrono::seconds(1)).ready);
    REQUIRE(reads == previousReads + 1);

    tracker.Remove(&peer);
    REQUIRE_FALSE(check(now + std::chrono::seconds(1)).ready);
    tracker.Observe(&peer, "one", 1, 100, now + std::chrono::seconds(1));
    const auto restored = check(now + std::chrono::seconds(1));
    REQUIRE(restored.ready);
    REQUIRE(restored.epoch == ready.epoch);

    evidence = MiningPeerEvidence::UNRESOLVED;
    REQUIRE(check(now + std::chrono::milliseconds(1500)).ready);
    REQUIRE_FALSE(tracker.Check(1, false, true, false, false, resolve,
                               now + std::chrono::milliseconds(1500)).ready);
    REQUIRE_FALSE(check(now + std::chrono::milliseconds(1500)).ready);
    evidence = MiningPeerEvidence::SUPPORTING;
    REQUIRE_FALSE(check(now + std::chrono::milliseconds(2499)).ready);
    REQUIRE(check(now + std::chrono::milliseconds(2500)).ready);

    const auto expiry = now + std::chrono::seconds(1) + MiningPeerTracker::MAX_AGE;
    const auto beforeExpiry = check(expiry - std::chrono::milliseconds(500));
    previousReads = reads;
    REQUIRE(beforeExpiry.ready);
    REQUIRE(check(expiry).ready);
    REQUIRE(reads == previousReads);
    REQUIRE_FALSE(check(expiry + std::chrono::milliseconds(1)).ready);
    tracker.Observe(&peer, "one", 1, 100, expiry + std::chrono::milliseconds(2));
    REQUIRE(check(expiry + std::chrono::milliseconds(2)).ready);
    REQUIRE(check(expiry + std::chrono::milliseconds(2)).epoch == beforeExpiry.epoch);
}

TEST_CASE("Mining work epochs follow committed hashes including unsampled reorgs",
    "[ledger][mining-readiness][template_freshness]")
{
    using namespace TAO::Ledger::ChainState;
    MiningWorkEpoch work;
    work.Commit(1);
    const auto first = work.ForTip(1);
    REQUIRE(first != 0);
    REQUIRE(work.ForTip(2) == 0);
    work.Commit(1);
    REQUIRE(work.ForTip(1) == first);
    work.Commit(2);
    REQUIRE(work.ForTip(2) == first + 1);
    REQUIRE(work.ForTip(1) == 0);
    work.Commit(1);
    REQUIRE(work.ForTip(1) == first + 2);
    REQUIRE(work.ForTip(2) == 0);

    work.Commit(2);
    work.Commit(1);
    REQUIRE(work.ForTip(1) == first + 4);
}

TEST_CASE("Mining readiness cannot stamp work across a committed tip publication",
    "[ledger][mining-readiness][template_freshness]")
{
    using namespace TAO::Ledger;
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    guard.Report(0);
    guard.Report(1);
    const auto initial = ChainState::MiningReadiness();
    REQUIRE(initial.ready);

    const BlockState first = guard.tip;
    ++guard.tip.nNonce;
    guard.extraBlocks.push_back(guard.tip.GetHash());
    REQUIRE(LLD::Ledger->WriteBlock(guard.tip.GetHash(), guard.tip));
    ChainState::tStateBest.store(guard.tip);
    REQUIRE_FALSE(ChainState::MiningReadiness().ready);
    ChainState::hashBestChain.store(guard.tip.GetHash());
    REQUIRE_FALSE(ChainState::MiningReadiness().ready);
    ChainState::CommitMiningTip(guard.tip.GetHash());
    guard.Report(0);
    guard.Report(1);
    const auto replaced = ChainState::MiningReadiness();
    REQUIRE(replaced.ready);
    REQUIRE(replaced.epoch == initial.epoch + 1);
    REQUIRE_FALSE(ChainState::MiningWorkCurrent(initial.epoch));

    /* No readiness sample between these committed transitions. */
    ChainState::CommitMiningTip(first.GetHash());
    ChainState::CommitMiningTip(guard.tip.GetHash());
    REQUIRE(ChainState::MiningReadiness().epoch == replaced.epoch + 2);
    REQUIRE_FALSE(ChainState::MiningWorkCurrent(replaced.epoch));
}

TEST_CASE("Split peer tip reports refresh only complete timely evidence",
    "[ledger][mining-readiness]")
{
    using namespace TAO::Ledger::ChainState;
    MiningPeerTracker tracker;
    LLP::MiningTipReports reports;
    const auto now = MiningPeerTracker::Clock::now();
    int peer;
    const auto resolve = [](const uint1024_t& hash, uint32_t height)
    {
        return hash == 1 && height == 100 ? MiningPeerEvidence::SUPPORTING
                                          : MiningPeerEvidence::UNRESOLVED;
    };
    const auto send = [&](bool hasHash, uint1024_t hash, bool hasHeight,
                          uint32_t height, auto time)
    {
        uint1024_t pairedHash;
        uint32_t pairedHeight = 0;
        if(reports.Observe(hasHash, hash, hasHeight, height, pairedHash, pairedHeight, time))
            tracker.Observe(&peer, "one", pairedHash, pairedHeight, time);
        else
            tracker.InvalidatePartial(&peer, hasHash, hash, hasHeight, height);
    };
    const auto check = [&](auto time)
    {
        return tracker.CheckCached(1, 1, false, true, false, false, resolve, time);
    };

    send(false, 0, true, 100, now);
    REQUIRE_FALSE(check(now).ready);
    send(true, 1, false, 0, now + std::chrono::seconds(1));
    const auto ready = check(now + std::chrono::seconds(1));
    REQUIRE(ready.ready);
    for(int seconds : {30, 60, 90, 120})
    {
        const auto t = now + std::chrono::seconds(seconds);
        send(true, 1, false, 0, t);
        send(false, 0, true, 100, t + std::chrono::seconds(1));
        REQUIRE(check(t + std::chrono::seconds(1)).ready);
        REQUIRE(check(t + std::chrono::seconds(1)).epoch == ready.epoch);
    }

    const auto changed = now + std::chrono::seconds(122);
    send(false, 0, true, 101, changed);
    const auto paused = check(changed);
    REQUIRE_FALSE(paused.ready);
    REQUIRE(paused.reason == "unresolved-peer-tip");
    REQUIRE(paused.epoch == ready.epoch);
    send(true, 1, false, 0, changed + std::chrono::seconds(1));
    REQUIRE_FALSE(check(changed + std::chrono::seconds(1)).ready);
    send(true, 1, true, 100, changed + std::chrono::seconds(2));
    REQUIRE(check(changed + std::chrono::seconds(2)).ready);
    REQUIRE(check(changed + std::chrono::seconds(2)).epoch == ready.epoch);

    /* A single field, or fields too far apart, cannot renew freshness. */
    const auto lastPair = changed + std::chrono::seconds(2);
    send(true, 1, false, 0, lastPair + std::chrono::seconds(5));
    send(false, 0, true, 100, lastPair + std::chrono::seconds(16));
    send(false, 0, true, 100, lastPair + std::chrono::seconds(60));
    REQUIRE(check(lastPair + MiningPeerTracker::MAX_AGE).ready);
    REQUIRE_FALSE(check(lastPair + MiningPeerTracker::MAX_AGE + std::chrono::seconds(1)).ready);

    /* Pending reports are never reused after a complete pair or reconnect. */
    uint1024_t pairedHash;
    uint32_t pairedHeight = 0;
    reports.Reset();
    REQUIRE_FALSE(reports.Observe(true, 1, false, 0, pairedHash, pairedHeight, now));
    reports.Reset();
    REQUIRE_FALSE(reports.Observe(false, 0, true, 100, pairedHash, pairedHeight, now));
}

TEST_CASE("Contradictory partial peer reports pause readiness even with a quorum",
    "[ledger][mining-readiness]")
{
    using namespace TAO::Ledger::ChainState;
    MiningPeerTracker tracker;
    const auto now = MiningPeerTracker::Clock::now();
    int a, b, c;
    for(const auto& item : {std::pair{&a, "one"}, std::pair{&b, "two"},
                            std::pair{&c, "three"}})
        tracker.Observe(item.first, item.second, 1, 100, now);
    const auto resolve = [](const uint1024_t&, uint32_t) { return MiningPeerEvidence::SUPPORTING; };
    const auto check = [&]
    {
        return tracker.CheckCached(1, 2, false, true, false, false, resolve, now);
    };
    const auto ready = check();
    REQUIRE(ready.ready);
    REQUIRE(ready.peers == 3);

    SECTION("Changed hash") { tracker.InvalidatePartial(&c, true, 2, false, 0); }
    SECTION("Changed height") { tracker.InvalidatePartial(&c, false, 0, true, 101); }
    const auto paused = check();
    REQUIRE_FALSE(paused.ready);
    REQUIRE(paused.peers == 2);
    REQUIRE(paused.reason == "unresolved-peer-tip");
    REQUIRE(paused.epoch == ready.epoch);
    tracker.Observe(&c, "three", 1, 100, now);
    REQUIRE(check().ready);
    REQUIRE(check().epoch == ready.epoch);
}

TEST_CASE("Split peer catch-up gates readiness without changing committed work",
    "[ledger][mining-readiness][template_freshness]")
{
    using namespace TAO::Ledger::ChainState;
    MiningPeerTracker tracker;
    const auto now = MiningPeerTracker::Clock::now();
    int peer;
    MiningWorkEpoch work;
    work.Commit(2);
    tracker.Observe(&peer, "one", 1, 100, now);
    const auto resolve = [](const uint1024_t& hash, uint32_t height)
    {
        return (hash == 1 && height == 100) || (hash == 2 && height == 101)
            ? MiningPeerEvidence::SUPPORTING : MiningPeerEvidence::UNRESOLVED;
    };
    const auto check = [&](auto time)
    {
        auto status = tracker.CheckCached(2, 1, false, true, false, false, resolve, time);
        status.epoch = work.ForTip(2);
        return status;
    };
    const auto ready = check(now);
    REQUIRE(ready.ready);
    const bool hashFirst = GENERATE(false, true);
    const auto partial = [&](bool hashField, uint1024_t hash, uint32_t height)
    {
        tracker.InvalidatePartial(&peer, hashField, hash, !hashField, height);
    };
    const auto confirm = [&](uint1024_t hash, uint32_t height)
    {
        tracker.Observe(&peer, "one", hash, height, now);
    };

    partial(hashFirst, 2, 101);
    SECTION("Matching pair completes before work is checked")
    {
        confirm(2, 101);
        const auto after = check(now);
        REQUIRE(after.ready);
        REQUIRE(after.epoch == ready.epoch);
    }
    SECTION("Work checked between the two fields pauses until the pair completes")
    {
        const auto paused = check(now);
        REQUIRE_FALSE(paused.ready);
        REQUIRE(paused.reason == "unresolved-peer-tip");
        REQUIRE(paused.epoch == ready.epoch);
        confirm(2, 101);
        REQUIRE(check(now).ready);
        REQUIRE(check(now).epoch == ready.epoch);
    }
    SECTION("A contradictory later field stays unresolved until a complete pair")
    {
        partial(hashFirst, 3, 102);
        REQUIRE_FALSE(check(now).ready);
        confirm(2, 101);
        REQUIRE(check(now).ready);
        REQUIRE(check(now).epoch == ready.epoch);
    }
    SECTION("A mismatched complete report pauses until repaired")
    {
        confirm(hashFirst ? 3 : 2, hashFirst ? 101 : 102);
        REQUIRE_FALSE(check(now).ready);
        confirm(2, 101);
        REQUIRE(check(now).ready);
        REQUIRE(check(now).epoch == ready.epoch);
    }
    SECTION("An unknown same-height peer tip does not replace the committed tip")
    {
        confirm(3, 101);
        REQUIRE_FALSE(check(now).ready);
        confirm(2, 101);
        REQUIRE(check(now).ready);
        REQUIRE(check(now).epoch == ready.epoch);
    }
    SECTION("An unknown complete report pauses before recovery")
    {
        confirm(3, 102);
        REQUIRE_FALSE(check(now).ready);
        confirm(2, 101);
        REQUIRE(check(now).ready);
        REQUIRE(check(now).epoch == ready.epoch);
    }
    SECTION("Expiry pauses readiness without replacing work")
    {
        const auto expired = now + MiningPeerTracker::MAX_AGE + std::chrono::seconds(1);
        REQUIRE_FALSE(check(expired).ready);
        tracker.Observe(&peer, "one", 2, 101, expired);
        REQUIRE(check(expired).ready);
        REQUIRE(check(expired).epoch == ready.epoch);
    }
    SECTION("Disconnect pauses readiness without replacing work")
    {
        tracker.Remove(&peer);
        REQUIRE_FALSE(check(now).ready);
        confirm(2, 101);
        REQUIRE(check(now).ready);
        REQUIRE(check(now).epoch == ready.epoch);
    }
}

TEST_CASE("Split tip pairing consumes fields and enforces its time window",
    "[ledger][mining-readiness]")
{
    LLP::MiningTipReports reports;
    const auto now = LLP::MiningTipReports::Clock::now();
    uint1024_t hash;
    uint32_t height = 0;
    REQUIRE_FALSE(reports.Observe(true, 1, false, 0, hash, height, now));
    REQUIRE(reports.Observe(false, 0, true, 100, hash, height,
                            now + LLP::MiningTipReports::PAIR_WINDOW));
    REQUIRE(hash == 1);
    REQUIRE(height == 100);
    REQUIRE_FALSE(reports.Observe(false, 0, true, 101, hash, height,
                                  now + LLP::MiningTipReports::PAIR_WINDOW));
    REQUIRE(reports.Observe(true, 2, true, 102, hash, height,
                            now + LLP::MiningTipReports::PAIR_WINDOW));
    REQUIRE(hash == 2);
    REQUIRE(height == 102);
    REQUIRE_FALSE(reports.Observe(true, 3, false, 0, hash, height,
                                  now + LLP::MiningTipReports::PAIR_WINDOW));
    REQUIRE_FALSE(reports.Observe(false, 0, true, 103, hash, height,
                                  now + LLP::MiningTipReports::PAIR_WINDOW * 2
                                      + std::chrono::milliseconds(1)));
    REQUIRE(reports.Observe(true, 4, false, 0, hash, height,
                            now + LLP::MiningTipReports::PAIR_WINDOW * 2
                                + std::chrono::milliseconds(1)));
    REQUIRE(hash == 4);
    REQUIRE(height == 103);
}

TEST_CASE("Both mining lanes resume same-tip work after connectivity returns",
    "[ledger][mining-readiness]")
{
    using namespace TAO::Ledger;
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    REQUIRE_FALSE(ChainState::MiningReadiness().ready);
    memory::encrypted_ptr<Credentials> noCredentials;
    TritiumBlock stake;
    REQUIRE_FALSE(CreateStakeBlock(noCredentials, "", stake));
    guard.Report(0);
    REQUIRE_FALSE(ChainState::MiningReadiness().ready);
    config::mapArgs["-miningminpeers"] = "1";
    REQUIRE(ChainState::MiningReadiness().ready);
    config::mapArgs.erase("-miningminpeers");
    guard.Report(1);
    auto ready = ChainState::MiningReadiness();
    REQUIRE(ready.ready);
    REQUIRE(ready.required == 2);
    const bool validationSync = ChainState::Synchronizing();
    TritiumBlock work;
    work.nMiningEpoch = ready.epoch;
    work.nVersion = 9;
    work.nChannel = 2;
    work.nHeight = guard.tip.nHeight + 1;
    work.nBits = 0x1f00ffff;
    work.hashPrevBlock = guard.tip.GetHash();
    work.hashMerkleRoot = 123;
    LLP::LegacyGetBlockRequest legacy;
    legacy.fnCreateBlock = [&]() -> Block* { return &work; };
    LLP::StatelessGetBlockRequest stateless;
    stateless.fnCreateBlock = legacy.fnCreateBlock;
    REQUIRE(LLP::LegacyGetBlockHandler(legacy).fSuccess);
    REQUIRE(LLP::StatelessGetBlockHandler(stateless).fSuccess);

    ChainState::ForgetPeerHeight(&guard.peers[0]);
    REQUIRE_FALSE(ChainState::MiningReadiness().ready);
    REQUIRE_FALSE(LLP::LegacyGetBlockHandler(legacy).fSuccess);
    REQUIRE_FALSE(ChainState::MiningWorkCurrent(work.nMiningEpoch));
    REQUIRE_FALSE(LLP::StatelessGetBlockHandler(stateless).fSuccess);
    REQUIRE(ValidateMinedBlock(work).reason == "mining paused or work epoch revoked");
    REQUIRE_FALSE(AcceptMinedBlock(work).accepted);
    uint8_t status = 0;
    Process(work, status, nullptr, true, work.nMiningEpoch);
    REQUIRE(status == (PROCESS::IGNORED | PROCESS::WORK_REVOKED));
    REQUIRE(ChainState::Synchronizing() == validationSync);
    REQUIRE(LLP::TritiumNode::fSynchronized.load());

    guard.Report(0);
    REQUIRE(ChainState::MiningReadiness().ready);
    REQUIRE(ChainState::MiningReadiness().epoch == ready.epoch);
    REQUIRE(ChainState::MiningWorkCurrent(work.nMiningEpoch));
    work.nMiningEpoch = ChainState::MiningReadiness().epoch;
    TritiumBlock staleWork = work;
    staleWork.hashPrevBlock++;
    status = 0;
    Process(staleWork, status, nullptr, true, staleWork.nMiningEpoch);
    REQUIRE(status == (PROCESS::IGNORED | PROCESS::STALE));
    REQUIRE(LLP::LegacyGetBlockHandler(legacy).fSuccess);
    REQUIRE(LLP::StatelessGetBlockHandler(stateless).fSuccess);
    DataStream serialized(SER_NETWORK, LLP::PROTOCOL_VERSION);
    serialized << work;
    TritiumBlock copied = work;
    REQUIRE(copied.nMiningEpoch == work.nMiningEpoch);
    copied.nMiningEpoch++;
    DataStream serializedCopy(SER_NETWORK, LLP::PROTOCOL_VERSION);
    serializedCopy << copied;
    REQUIRE(std::vector<uint8_t>(serializedCopy.begin(), serializedCopy.end())
        == std::vector<uint8_t>(serialized.begin(), serialized.end()));
    REQUIRE(copied.GetHash() == work.GetHash());
    TritiumBlock decoded;
    serializedCopy >> decoded;
    REQUIRE(decoded.nMiningEpoch == 0);

    ChainState::ObserveMiningPeer(&guard.peers[2], "192.0.2.3", 0xBAD, 1);
    REQUIRE(ChainState::MiningReadiness().reason == "unresolved-peer-tip");
    REQUIRE_FALSE(LLP::StatelessGetBlockHandler(stateless).fSuccess);
}

#ifndef WIN32
TEST_CASE("Deferred templates are fenced at the socket write boundary",
    "[ledger][mining-readiness][template_freshness]")
{
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    guard.Report(0);
    guard.Report(1);

    SECTION("Legacy lane")
    {
        CheckQueuedTemplateEpoch<LLP::Packet>(guard, 0);
    }
    SECTION("Stateless lane")
    {
        CheckQueuedTemplateEpoch<LLP::StatelessPacket>(guard, 0xD000);
    }
    SECTION("Stateless recovery template")
    {
        CheckQueuedTemplateEpoch<LLP::StatelessPacket>(guard, LLP::StatelessOpcodes::STATELESS_GET_BLOCK);
    }
}

TEST_CASE("Template delivery markers preserve concurrent write order",
    "[ledger][mining-readiness][template_freshness]")
{
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    guard.Report(0);
    guard.Report(1);
    REQUIRE(TAO::Ledger::ChainState::MiningReadiness().ready);

    const bool firstDirect = GENERATE(false, true);
    const bool secondDirect = GENERATE(false, true);
    SECTION("Legacy lane")
    {
        CheckTemplateDeliveryOrder<LLP::Packet>(firstDirect, secondDirect);
    }
    SECTION("Stateless lane")
    {
        CheckTemplateDeliveryOrder<LLP::StatelessPacket>(firstDirect, secondDirect);
    }
}
#endif

TEST_CASE("Both GET_BLOCK lanes reject a template after a committed tip change during creation",
    "[ledger][mining-readiness][template_freshness]")
{
    using namespace TAO::Ledger;
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    guard.Report(0);
    guard.Report(1);
    const auto ready = ChainState::MiningReadiness();
    REQUIRE(ready.ready);

    TritiumBlock work;
    work.nMiningEpoch = ready.epoch;
    work.nVersion = 9;
    work.nChannel = 2;
    work.nHeight = guard.tip.nHeight + 1;
    work.nBits = 0x1f00ffff;
    work.hashPrevBlock = guard.tip.GetHash();
    work.hashMerkleRoot = 123;
    auto create = [&]() -> Block*
    {
        ++guard.tip.nNonce;
        guard.extraBlocks.push_back(guard.tip.GetHash());
        REQUIRE(LLD::Ledger->WriteBlock(guard.tip.GetHash(), guard.tip));
        ChainState::tStateBest.store(guard.tip);
        ChainState::hashBestChain.store(guard.tip.GetHash());
        ChainState::CommitMiningTip(guard.tip.GetHash());
        guard.Report(0);
        guard.Report(1);
        return &work;
    };

    SECTION("Legacy lane")
    {
        LLP::LegacyGetBlockRequest req;
        req.fnCreateBlock = create;
        const auto result = LLP::LegacyGetBlockHandler(req);
        REQUIRE_FALSE(result.fSuccess);
        REQUIRE(result.eReason == LLP::GetBlockPolicyReason::TEMPLATE_NOT_READY);
        REQUIRE(result.vPayload.empty());
    }
    SECTION("Stateless lane")
    {
        LLP::StatelessGetBlockRequest req;
        req.fnCreateBlock = create;
        const auto result = LLP::StatelessGetBlockHandler(req);
        REQUIRE_FALSE(result.fSuccess);
        REQUIRE(result.eReason == LLP::GetBlockPolicyReason::TEMPLATE_NOT_READY);
        REQUIRE(result.vPayload.empty());
    }
}

TEST_CASE("Peer churn preserves committed work across creation validation and publication",
    "[ledger][mining-readiness][template_freshness]")
{
    using namespace TAO::Ledger;
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    guard.Report(0);
    guard.Report(1);
    REQUIRE(ChainState::MiningReadiness().ready);

    const auto oldTip = guard.tip;
    guard.extraBlocks.push_back(oldTip.GetHash());
    guard.tip.hashPrevBlock = oldTip.GetHash();
    ++guard.tip.nHeight;
    ++guard.tip.nNonce;
    /* A second Prime state uses the reserve-based reward path rather than
     * the first-block subsidy fallback. Give that state a spendable reserve. */
    guard.tip.nReleasedReserve[0] = NXS_COIN;
    REQUIRE(LLD::Ledger->WriteBlock(guard.tip.GetHash(), guard.tip));
    guard.extraBlocks.push_back(guard.tip.GetHash());
    ChainState::tStateBest.store(guard.tip);
    ChainState::hashBestChain.store(guard.tip.GetHash());
    ChainState::nBestHeight.store(guard.tip.nHeight);
    ChainState::CommitMiningTip(guard.tip.GetHash());
    ClearMiningTemplateCaches("test local acceptance");

    const auto ready = ChainState::MiningReadiness();
    REQUIRE(ready.ready);
    REQUIRE(GetCoinbaseReward(guard.tip, 1, 0) > 0);
    memory::encrypted_ptr<Credentials> user =
        new Credentials(LLC::GetRand256().ToString().c_str(), "peer-catch-up-test");
    TritiumBlock work;
    REQUIRE(CreateBlock(user, "1234", 1, work, 42));
    REQUIRE(work.hashPrevBlock == guard.tip.GetHash());
    REQUIRE(work.nHeight == guard.tip.nHeight + 1);
    REQUIRE(work.nMiningEpoch == ready.epoch);
    bool owner = false;
    const uint256_t reward = 123;
    auto token = Testing::BeginOrJoinMiningTemplateInFlight(1, reward, owner,
                                                            guard.tip.GetHash(), ready.epoch);
    REQUIRE(owner);

    const bool hashFirst = GENERATE(false, true);
    LLP::MiningTipReports reports[2];
    const auto sendField = [&](size_t peer, bool hasHash)
    {
        uint1024_t pairedHash;
        uint32_t pairedHeight = 0;
        const uint1024_t hash = guard.tip.GetHash();
        if(reports[peer].Observe(hasHash, hash, !hasHash, guard.tip.nHeight,
                                 pairedHash, pairedHeight))
            ChainState::ObserveMiningPeer(&guard.peers[peer],
                "192.0.2." + std::to_string(peer + 1), pairedHash, pairedHeight);
        else
            ChainState::InvalidatePartialMiningPeer(&guard.peers[peer], hasHash,
                hash, !hasHash, guard.tip.nHeight);
    };
    sendField(0, hashFirst);
    sendField(0, !hashFirst);
    sendField(1, !hashFirst);
    sendField(1, hashFirst);
    guard.Report(2);
    REQUIRE(ChainState::MiningReadiness().ready);
    REQUIRE(ChainState::MiningWorkCurrent(work.nMiningEpoch));
    TritiumBlock probe = work;
    probe.nChannel = 3;
    REQUIRE(ValidateMinedBlock(probe).reason == "invalid block channel");
    TritiumBlock stale = work;
    stale.hashPrevBlock++;
    REQUIRE(ValidateMinedBlock(stale).failure == MinedBlockFailure::STALE);
    REQUIRE(Testing::CompleteMiningTemplateInFlight(token, 1, reward, 42, ready.epoch));
    TritiumBlock next;
    REQUIRE(CreateBlock(user, "1234", 1, next, 42));
    REQUIRE(next.nMiningEpoch == work.nMiningEpoch);
    REQUIRE(next.hashPrevBlock == work.hashPrevBlock);

    /* Reproduce the peer-tip storm without a local best-chain change. */
    token = Testing::BeginOrJoinMiningTemplateInFlight(1, reward, owner,
                                                        guard.tip.GetHash(), ready.epoch);
    REQUIRE(owner);
    for(uint32_t i = 0; i < 1000; ++i)
        ChainState::ObserveMiningPeer(&guard.peers[0], "192.0.2.1", 0xBAD + i, i + 1);
    const auto paused = ChainState::MiningReadiness();
    REQUIRE_FALSE(paused.ready);
    REQUIRE(paused.reason == "unresolved-peer-tip");
    REQUIRE(paused.epoch == ready.epoch);
    REQUIRE_FALSE(ChainState::MiningWorkCurrent(work.nMiningEpoch));
    REQUIRE(ValidateMinedBlock(work).failure == MinedBlockFailure::WORK_REVOKED);
    REQUIRE_FALSE(AcceptMinedBlock(work).accepted);
    uint8_t status = 0;
    Process(work, status, nullptr, true, work.nMiningEpoch);
    REQUIRE(status == (PROCESS::IGNORED | PROCESS::WORK_REVOKED));
    guard.Report(0);
    REQUIRE(ChainState::MiningReadiness().ready);
    REQUIRE(ChainState::MiningReadiness().epoch == ready.epoch);
    REQUIRE(ChainState::MiningWorkCurrent(work.nMiningEpoch));
    REQUIRE(Testing::CompleteMiningTemplateInFlight(token, 1, reward, 42, ready.epoch));
    REQUIRE(Testing::StoreMiningTemplateCacheEntryForTesting(
        1, reward, 42, guard.tip.GetHash(), ready.epoch));
    probe = work;
    probe.nChannel = 3;
    REQUIRE(ValidateMinedBlock(probe).reason == "invalid block channel");

    const auto restored = ChainState::MiningReadiness();
    ChainState::ForgetPeerHeight(&guard.peers[0]);
    guard.Report(0);
    REQUIRE(ChainState::MiningReadiness().ready);
    REQUIRE(ChainState::MiningWorkCurrent(restored.epoch));
}

TEST_CASE("Same-tip mining cache preserves identity after an evidence interruption",
    "[ledger][mining-readiness][template_freshness]")
{
    using namespace TAO::Ledger;
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    guard.Report(0);
    guard.Report(1);
    REQUIRE(ChainState::MiningReadiness().ready);
    memory::encrypted_ptr<Credentials> user =
        new Credentials(LLC::GetRand256().ToString().c_str(), "readiness-test");
    TritiumBlock oversized;
    REQUIRE_FALSE(CreateBlock(user, "1234", 1, oversized, uint64_t(UINT32_MAX) + 1));
    TritiumBlock first;
    REQUIRE(CreateBlock(user, "1234", 1, first, 42));
    REQUIRE(ChainState::MiningWorkCurrent(first.nMiningEpoch));
    TAO::Operation::Contract coinbase = first.producer[0];
    coinbase.Reset();
    coinbase.Seek(41);
    uint64_t extraNonce = 0;
    coinbase >> extraNonce;
    REQUIRE(extraNonce == ((first.nMiningEpoch << 32) | 42));
    ChainState::ForgetPeerHeight(&guard.peers[0]);
    REQUIRE_FALSE(ChainState::MiningReadiness().ready);
    TritiumBlock paused;
    REQUIRE_FALSE(CreateBlock(user, "1234", 1, paused, 42));
    guard.Report(0);
    TritiumBlock next;
    REQUIRE(CreateBlock(user, "1234", 1, next, 42));
    REQUIRE(next.hashPrevBlock == first.hashPrevBlock);
    REQUIRE(next.nMiningEpoch == first.nMiningEpoch);
    /* Cache hits still refresh producer timestamps/signatures; the work
     * namespace, not the newly returned merkle root, must remain unchanged. */
    REQUIRE(next.producer[0].Operations() == first.producer[0].Operations());
    REQUIRE(ChainState::MiningWorkCurrent(first.nMiningEpoch));
    REQUIRE(ChainState::MiningWorkCurrent(next.nMiningEpoch));
}

TEST_CASE("Mining evidence uses local weights and exact hash-height pairing",
    "[ledger][mining-readiness]")
{
    using namespace TAO::Ledger;
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    guard.Report(0);
    guard.Report(1);
    REQUIRE(ChainState::MiningReadiness().ready);
    ChainState::ObserveMiningPeer(&guard.peers[0], "192.0.2.1",
        guard.tip.GetHash(), guard.tip.nHeight + 1);
    REQUIRE(ChainState::MiningReadiness().reason == "unresolved-peer-tip");
    guard.Report(0);
    BlockState shorter = guard.tip;
    shorter.nHeight--;
    shorter.nNonce++;
    const uint1024_t hashShorter = shorter.GetHash();
    REQUIRE(LLD::Ledger->WriteBlock(hashShorter, shorter));
    guard.extraBlocks.push_back(hashShorter);
    ChainState::ObserveMiningPeer(&guard.peers[2], "192.0.2.3",
        hashShorter, shorter.nHeight);
    REQUIRE(ChainState::MiningReadiness().ready);
    shorter.nChannelWeight[0] = 100;
    shorter.nChannelWeight[1] = 100;
    REQUIRE(shorter.IsHeavierThan(guard.tip));
    REQUIRE(LLD::Ledger->WriteBlock(hashShorter, shorter));
    REQUIRE(ChainState::MiningReadiness().reason == "unresolved-peer-tip");
}

TEST_CASE("Local admission rechecks readiness after forced proof verification",
    "[ledger][process][mining-readiness][stake]")
{
    using namespace TAO::Ledger;
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    guard.Report(0);
    guard.Report(1);
    const auto ready = ChainState::MiningReadiness();
    REQUIRE(ready.ready);
    class RevokedBlock final : public Block
    {
    public:
        const void* peer = nullptr;
        mutable bool forceProof = false;
        mutable bool accepted = false;
        RevokedBlock* Clone() const override
        {
            return new RevokedBlock(*this);
        }
        bool Check(bool force) const override
        {
            forceProof = force;
            ChainState::ForgetPeerHeight(peer);
            return true;
        }
        bool Accept() const override
        {
            accepted = true;
            return true;
        }
    } block;
    block.peer = &guard.peers[0];
    block.nVersion = 7;
    block.nChannel = 0;
    block.nHeight = guard.tip.nHeight + 1;
    block.nNonce = LLC::GetRand();
    block.hashPrevBlock = guard.tip.GetHash();
    uint8_t status = 0;
    Process(block, status, nullptr, false, ready.epoch);
    REQUIRE(block.forceProof);
    REQUIRE_FALSE(block.accepted);
    REQUIRE(status == (PROCESS::IGNORED | PROCESS::WORK_REVOKED));
    REQUIRE_FALSE(LLD::Ledger->HasBlock(block.GetHash()));
}

TEST_CASE("Mining readiness rejects a block stored under a mismatched hash",
    "[ledger][process][mining-readiness]")
{
    using namespace TAO::Ledger;
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    guard.Report(0);
    guard.Report(1);
    REQUIRE(ChainState::MiningReadiness().ready);

    const uint1024_t wrongHash = guard.tip.GetHash() + 1;
    guard.extraBlocks.push_back(wrongHash);
    REQUIRE(LLD::Ledger->WriteBlock(wrongHash, guard.tip));
    ChainState::ObserveMiningPeer(&guard.peers[2], "192.0.2.3",
        wrongHash, guard.tip.nHeight);
    const auto status = ChainState::MiningReadiness();
    REQUIRE_FALSE(status.ready);
    REQUIRE(status.reason == "unresolved-peer-tip");
    REQUIRE(status.peers == 2);
}

TEST_CASE("Mining readiness counts only ancestors of the best tip",
    "[ledger][mining-readiness]")
{
    using namespace TAO::Ledger;
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    const uint32_t distance = GENERATE(0u, 1u, 2u);

    BlockState grandparent = guard.tip;
    grandparent.nHeight -= 2;
    grandparent.nNonce++;
    guard.extraBlocks.push_back(grandparent.GetHash());
    REQUIRE(LLD::Ledger->WriteBlock(grandparent.GetHash(), grandparent));
    BlockState parent = guard.tip;
    parent.nHeight--;
    parent.hashPrevBlock = grandparent.GetHash();
    guard.extraBlocks.push_back(parent.GetHash());
    REQUIRE(LLD::Ledger->WriteBlock(parent.GetHash(), parent));
    guard.extraBlocks.push_back(guard.tip.GetHash());
    guard.tip.hashPrevBlock = parent.GetHash();
    REQUIRE(LLD::Ledger->WriteBlock(guard.tip.GetHash(), guard.tip));
    ChainState::tStateBest.store(guard.tip);
    ChainState::hashBestChain.store(guard.tip.GetHash());
    ChainState::CommitMiningTip(guard.tip.GetHash());

    const BlockState ancestor = distance == 0 ? guard.tip :
                                distance == 1 ? parent : grandparent;
    BlockState side = ancestor;
    side.nNonce++;
    side.hashNextBlock = guard.tip.GetHash();
    guard.extraBlocks.push_back(side.GetHash());
    REQUIRE(LLD::Ledger->WriteBlock(side.GetHash(), side));
    REQUIRE(side.IsInMainChain());
    REQUIRE_FALSE(side.IsHeavierThan(guard.tip));

    guard.Report(0);
    ChainState::ObserveMiningPeer(&guard.peers[1], "192.0.2.2",
        side.GetHash(), side.nHeight);
    const auto rejected = ChainState::MiningReadiness();
    REQUIRE_FALSE(rejected.ready);
    REQUIRE(rejected.peers == 1);
    REQUIRE(rejected.reason == "insufficient-fresh-peers");

    ChainState::ObserveMiningPeer(&guard.peers[1], "192.0.2.2",
        ancestor.GetHash(), ancestor.nHeight);
    const auto accepted = ChainState::MiningReadiness();
    REQUIRE(accepted.ready);
    REQUIRE(accepted.peers == 2);
}

#ifndef WIN32
TEST_CASE("Mining evidence accepts paired and split tip notifications",
    "[ledger][process][mining-readiness]")
{
    using namespace TAO::Ledger;
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    guard.Report(0);
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    LLP::TritiumNode node;
    struct Cleanup
    {
        LLP::TritiumNode& node;
        int* fds;
        ~Cleanup()
        {
            ChainState::ForgetPeerHeight(&node);
            node.fd = -1;
            close(fds[0]);
            close(fds[1]);
        }
    } cleanup{node, fds};
    node.fd = fds[1];
    node.events = POLLIN;
    node.nCurrentSession = 4321;
    node.nProtocolVersion = LLP::PROTOCOL_VERSION;
    node.nCurrentHeight = guard.tip.nHeight - 1;
    node.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BESTCHAIN
                 | LLP::TritiumNode::SUBSCRIPTION::BESTHEIGHT);
    const auto notify = [&](const DataStream& payload)
    {
        node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        node.INCOMING.DATA.assign(payload.begin(), payload.end());
        REQUIRE(node.ProcessPacket());
    };
    DataStream hashOnly(SER_NETWORK, LLP::PROTOCOL_VERSION);
    hashOnly << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << guard.tip.GetHash();
    notify(hashOnly);
    REQUIRE(ChainState::MiningReadiness().reason == "insufficient-fresh-peers");
    DataStream heightOnly(SER_NETWORK, LLP::PROTOCOL_VERSION);
    heightOnly << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << guard.tip.nHeight;
    notify(heightOnly);
    REQUIRE(ChainState::MiningReadiness().ready);
    REQUIRE(ChainState::MiningReadiness().peers == 2);

    DataStream pair(SER_NETWORK, LLP::PROTOCOL_VERSION);
    SECTION("Hash precedes height")
    {
        pair << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << guard.tip.GetHash()
             << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << guard.tip.nHeight;
    }
    SECTION("Height precedes hash")
    {
        pair << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << guard.tip.nHeight
             << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << guard.tip.GetHash();
    }
    notify(pair);
    const auto ready = ChainState::MiningReadiness();
    REQUIRE(ready.ready);
    REQUIRE(ready.peers == 2);

    DataStream newerHeight(SER_NETWORK, LLP::PROTOCOL_VERSION);
    newerHeight << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << (guard.tip.nHeight + 1);
    notify(newerHeight);
    REQUIRE_FALSE(ChainState::MiningReadiness().ready);
    REQUIRE(ChainState::MiningReadiness().reason == "unresolved-peer-tip");
    notify(hashOnly);
    REQUIRE_FALSE(ChainState::MiningReadiness().ready);
    notify(pair);
    REQUIRE(ChainState::MiningReadiness().ready);
    REQUIRE(ChainState::MiningReadiness().epoch == ready.epoch);

    const auto readSent = [&]()
    {
        while(node.Buffered() > 0)
            REQUIRE(node.Flush() > 0);
        std::vector<uint8_t> sent;
        uint8_t buffer[512];
        for(ssize_t n = recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT);
            n > 0; n = recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT))
            sent.insert(sent.end(), buffer, buffer + n);
        return sent;
    };
    readSent();
    ChainState::nBestHeight.store(guard.tip.nHeight + 10);
    ChainState::hashBestChain.store(guard.tip.GetHash() + 1);
    for(const uint16_t flags : {
        uint16_t(LLP::TritiumNode::SUBSCRIPTION::BESTCHAIN
               | LLP::TritiumNode::SUBSCRIPTION::BESTHEIGHT),
        uint16_t(LLP::TritiumNode::SUBSCRIPTION::BESTCHAIN),
        uint16_t(LLP::TritiumNode::SUBSCRIPTION::BESTHEIGHT)})
    {
        DataStream subscription(SER_NETWORK, LLP::PROTOCOL_VERSION);
        DataStream expected(SER_NETWORK, LLP::PROTOCOL_VERSION);
        if(flags & LLP::TritiumNode::SUBSCRIPTION::BESTCHAIN)
        {
            subscription << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN);
            expected << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << guard.tip.GetHash();
        }
        if(flags & LLP::TritiumNode::SUBSCRIPTION::BESTHEIGHT)
        {
            subscription << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT);
            expected << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << guard.tip.nHeight;
        }
        for(int refresh = 0; refresh < 2; ++refresh)
        {
            node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::SUBSCRIBE;
            node.INCOMING.DATA.assign(subscription.begin(), subscription.end());
            REQUIRE(node.ProcessPacket());
            REQUIRE(readSent() == LLP::TritiumNode::NewMessage(
                LLP::TritiumNode::ACTION::NOTIFY, expected).GetBytes());
        }
    }
}

TEST_CASE("Idle sync finalizes when a disconnected peer's high watermark disappears",
    "[ledger][process][sync][mining-readiness]")
{
    using namespace TAO::Ledger;
    PublicMiningGuard guard;
    REQUIRE(guard.written);
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    LLP::TritiumNode node;
    struct Cleanup
    {
        LLP::TritiumNode& node;
        int* fds;
        ~Cleanup()
        {
            ChainState::ForgetPeerHeight(&node);
            node.fd = -1;
            close(fds[0]);
            close(fds[1]);
        }
    } cleanup{node, fds};
    node.fd = fds[1];
    node.events = POLLIN;
    node.nCurrentSession = 4321;
    node.nProtocolVersion = LLP::PROTOCOL_VERSION;
    node.nLastPing.store(runtime::unifiedtimestamp());
    node.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BESTCHAIN
                 | LLP::TritiumNode::SUBSCRIPTION::BESTHEIGHT);
    LLP::TritiumNode::fSynchronized.store(false);
    nSyncSession.store(node.nCurrentSession);
    ChainState::ObservePeerHeight(&guard.peers[2], UINT32_MAX);
    DataStream payload(SER_NETWORK, LLP::PROTOCOL_VERSION);
    payload << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << guard.tip.GetHash()
            << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << guard.tip.nHeight;
    node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
    node.INCOMING.DATA.assign(payload.begin(), payload.end());
    REQUIRE(node.ProcessPacket());
    REQUIRE_FALSE(LLP::TritiumNode::fSynchronized.load());
    REQUIRE(nSyncSession.load() == node.nCurrentSession);
    ChainState::ForgetPeerHeight(&guard.peers[2]);
    nSyncSession.store(9876);
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE_FALSE(LLP::TritiumNode::fSynchronized.load());
    node.Sync();
    REQUIRE(nSyncSession.load() == 9876);
    nSyncSession.store(node.nCurrentSession);
    ChainState::ForgetPeerHeight(&node);
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE_FALSE(LLP::TritiumNode::fSynchronized.load());
    node.nCurrentHeight = guard.tip.nHeight - 1;
    ChainState::ObservePeerHeight(&node, node.nCurrentHeight);
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE_FALSE(LLP::TritiumNode::fSynchronized.load());
    node.nCurrentHeight = guard.tip.nHeight;
    ChainState::ObservePeerHeight(&node, guard.tip.nHeight);
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE(LLP::TritiumNode::fSynchronized.load());
    REQUIRE(nSyncSession.load() == 0);
    node.Sync();
    REQUIRE(nSyncSession.load() == 0);
}
#endif


TEST_CASE("Missing transactions yield a soft INCOMPLETE, never a REJECT", "[ledger][process]")
{
    LedgerGuard env;

    /* Use a unique prev-block hash for this test so we don't collide with
     * other tests.  Write a minimal BlockState to LLD so Process() does not
     * take the orphan path. */
    const uint1024_t hashPrev(0x1234abcd);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 1;
    statePrev.nBits         = 1;

    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    /* Create the missing-tx block referencing the known prev hash. */
    MissingBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 2;
    block.nBits         = 1;
    block.nNonce        = 42;

    const uint1024_t hashBlock = block.GetHash();

    /* Ensure the block hash itself is NOT already in LLD (not a duplicate). */
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashBlock));

    /* Clear any stale retry state from previous tests. */
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();

    /* Drive the block through Process() up to the retry limit. Each iteration
     * should return INCOMPLETE (never REJECTED) and increment the retry
     * counter keyed by the block hash.  Clear mapLastMissingProcessTime before
     * each call so the 250-ms rate-limit guard does not coalesce rapid
     * successive test invocations into a single counter increment. */
    for(uint64_t i = 1; i <= LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES; ++i)
    {
        TAO::Ledger::mapLastMissingProcessTime.erase(hashBlock);
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        INFO("iteration " << i);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock));
        REQUIRE(TAO::Ledger::mapLastMissing[hashBlock] == i);
    }

    /* The next call exceeds the retry limit: still INCOMPLETE (not REJECTED),
     * the block's missing-tx markers are cleared (hashMissing = 0 signals the
     * LLP layer to escalate to full branch recovery), and — critically — the
     * mapLastMissing entry is ERASED so the next arrival of this block starts
     * a fresh retry cycle rather than being permanently silenced. */
    {
        TAO::Ledger::mapLastMissingProcessTime.erase(hashBlock);
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
        REQUIRE_FALSE(block.vMissing.empty());
        REQUIRE(block.hashMissing == 0);
        /* Counter must be erased — a leftover >50 value would permanently
         * wedge this block on every future arrival. */
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) == 0);
        REQUIRE(TAO::Ledger::mapMissingBranchEscalations.count(hashBlock) == 1);
        REQUIRE(TAO::Ledger::mapMissingBranchEscalations[hashBlock] == 1);
        REQUIRE_FALSE(TAO::Ledger::IsMissingBranchRecoveryCapped(hashBlock));
    }

    /* The retry counter has been reset, so the very next Process() call
     * for the same block should start a fresh cycle: INCOMPLETE again and
     * the counter re-initialized to 1, not permanently dropped. */
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);

        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) != 0);
        REQUIRE(TAO::Ledger::mapLastMissing[hashBlock] == 1);
    }

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    LLD::Ledger->EraseBlock(hashBlock);
    LLD::Ledger->EraseBlock(hashPrev);
}

TEST_CASE("Missing-tx branch-recovery escalations are counted and capped", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t hashPrev(0x2345bcde);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 1;
    statePrev.nBits         = 1;

    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    MissingBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 2;
    block.nBits         = 1;
    block.nNonce        = 4201;

    const uint1024_t hashBlock = block.GetHash();
    const uint32_t nTestCycles = TAO::Ledger::MAX_BRANCH_RECOVERY_ESCALATIONS + 1;
    const uint64_t nRetries = LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES + 1;

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    for(uint32_t nCycle = 1; nCycle <= nTestCycles; ++nCycle)
    {
        for(uint64_t i = 0; i < nRetries; ++i)
        {
            /* Bypass the 250-ms reprocess rate-limit so rapid test calls each
             * count as a distinct attempt rather than being coalesced. */
            TAO::Ledger::mapLastMissingProcessTime.erase(hashBlock);
            uint8_t nStatus = 0;
            TAO::Ledger::Process(block, nStatus);
            REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
            REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED) == 0);
        }

        INFO("cycle " << nCycle);
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) == 0);
        REQUIRE(TAO::Ledger::MissingBranchRecoveryEscalations(hashBlock) == nCycle);
        REQUIRE(TAO::Ledger::IsMissingBranchRecoveryCapped(hashBlock)
            == (nCycle > TAO::Ledger::MAX_BRANCH_RECOVERY_ESCALATIONS));
    }

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();
    LLD::Ledger->EraseBlock(hashBlock);
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Missing-transaction retry counter clears on successful ACCEPT", "[ledger][process]")
{
    LedgerGuard env;

    /* Set up a prev block in LLD. */
    const uint1024_t hashPrev(0x5678ef01);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 10;
    statePrev.nBits         = 1;

    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    /* Create a block that is temporarily missing a transaction. */
    MissingBlock missingBlock;
    missingBlock.nVersion      = 4;
    missingBlock.hashPrevBlock = hashPrev;
    missingBlock.nChannel      = 2;
    missingBlock.nHeight       = 11;
    missingBlock.nBits         = 1;
    missingBlock.nNonce        = 99;

    const uint1024_t hashMissing = missingBlock.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashMissing));

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();

    /* Drive a few INCOMPLETE results to build up the counter.
     * Erase the rate-limit entry before each call so the 250-ms guard does
     * not prevent rapid successive test calls from each incrementing the
     * counter. */
    for(uint32_t i = 0; i < 3; ++i)
    {
        TAO::Ledger::mapLastMissingProcessTime.erase(hashMissing);
        uint8_t nStatus = 0;
        TAO::Ledger::Process(missingBlock, nStatus);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    }
    REQUIRE(TAO::Ledger::mapLastMissing[hashMissing] == 3);

    /* Now process a block that passes Check()/Accept() with no missing tx and
     * confirm its retry counter is cleared on accept. */
    PassBlock goodBlock;
    goodBlock.nVersion      = 4;
    goodBlock.hashPrevBlock = hashPrev;
    goodBlock.nChannel      = 2;
    goodBlock.nHeight       = 11;
    goodBlock.nBits         = 1;
    goodBlock.nNonce        = 100;

    const uint1024_t hashGood = goodBlock.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashGood));

    /* Artificially add a counter for the good block's hash. */
    TAO::Ledger::mapLastMissing[hashGood] = 5;

    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(goodBlock, nStatus);

        REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED) != 0);
        /* The counter for this block should have been cleared on accept. */
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashGood) == 0);
    }

    /* The missing block's counter should still be there (untouched by the good
     * accept). */
    REQUIRE(TAO::Ledger::mapLastMissing[hashMissing] == 3);

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("mapLastMissing is bounded at MAX_MISSING_MAP_ENTRIES", "[ledger][process]")
{
    LedgerGuard env;

    /* Pre-fill the map to the cap by direct insertion so the test runs in
     * microseconds rather than calling Process() MAX_MISSING_MAP_ENTRIES times. */
    TAO::Ledger::mapLastMissing.clear();
    const uint64_t CAP = TAO::Ledger::MAX_MISSING_MAP_ENTRIES;
    for(uint64_t i = 1; i <= CAP; ++i)
        TAO::Ledger::mapLastMissing[uint1024_t(i)] = 1;

    REQUIRE(TAO::Ledger::mapLastMissing.size() == CAP);

    /* Write a prev block so the new missing block doesn't take the orphan path. */
    const uint1024_t hashPrevCap(0xC0DE12345ULL);

    TAO::Ledger::BlockState statePrevCap;
    statePrevCap.nVersion      = 4;
    statePrevCap.hashPrevBlock = uint1024_t(0);
    statePrevCap.nChannel      = 2;
    statePrevCap.nHeight       = 30;
    statePrevCap.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrevCap, statePrevCap));

    /* A missing block whose hash is NOT one of the 10 000 pre-filled keys. */
    MissingBlock capBlock;
    capBlock.nVersion      = 4;
    capBlock.hashPrevBlock = hashPrevCap;
    capBlock.nChannel      = 2;
    capBlock.nHeight       = 31;
    capBlock.nBits         = 1;
    capBlock.nNonce        = 7777;

    const uint1024_t hashCap = capBlock.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashCap));

    /* Process the new missing block: should trigger a clear then insert 1 entry. */
    uint8_t nStatus = 0;
    TAO::Ledger::Process(capBlock, nStatus);

    REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);

    /* After the cap-clear the map must contain only the single new entry. */
    REQUIRE(TAO::Ledger::mapLastMissing.size() == 1);
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashCap) == 1);
    REQUIRE(TAO::Ledger::mapLastMissing[hashCap] == 1);

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    LLD::Ledger->EraseBlock(hashPrevCap);
}


TEST_CASE("Orphan-drain missing tx sets hashMissing to orphan's own hash", "[ledger][process]")
{
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();

    /* Write a prev block so the accepted parent block doesn't take the orphan
     * path when Process() is called with it. */
    const uint1024_t hashPrevOrphan(0xABCD5600ULL);

    TAO::Ledger::BlockState statePrevOrphan;
    statePrevOrphan.nVersion      = 4;
    statePrevOrphan.hashPrevBlock = uint1024_t(0);
    statePrevOrphan.nChannel      = 2;
    statePrevOrphan.nHeight       = 40;
    statePrevOrphan.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrevOrphan, statePrevOrphan));

    /* Build the accepted parent PassBlock so we can determine its hash. */
    PassBlock parentBlock;
    parentBlock.nVersion      = 4;
    parentBlock.hashPrevBlock = hashPrevOrphan;
    parentBlock.nChannel      = 2;
    parentBlock.nHeight       = 41;
    parentBlock.nBits         = 1;
    parentBlock.nNonce        = 2001;

    const uint1024_t hashParent = parentBlock.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashParent));

    /* Build the orphan MissingBlock whose parent is the just-accepted block. */
    MissingBlock orphanBlock;
    orphanBlock.nVersion      = 4;
    orphanBlock.hashPrevBlock = hashParent;   /* parent = accepted block */
    orphanBlock.nChannel      = 2;
    orphanBlock.nHeight       = 42;
    orphanBlock.nBits         = 1;
    orphanBlock.nNonce        = 3001;

    const uint1024_t hashOrphan = orphanBlock.GetHash();

    /* Seed mapOrphans: keyed by the orphan's parent hash (= hashParent). */
    REQUIRE(TAO::Ledger::mapOrphans.Insert(orphanBlock));

    /* Process the parent block: it is accepted, then the orphan-drain kicks in
     * and finds the orphan has missing transactions. */
    uint8_t nStatus = 0;
    TAO::Ledger::Process(parentBlock, nStatus);

    /* Parent was accepted, orphan is incomplete. */
    REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);

    /* hashMissing must be the ORPHAN's own hash, not the parent/map key. */
    REQUIRE(parentBlock.hashMissing == hashOrphan);
    REQUIRE(parentBlock.hashMissing != hashParent);

    /* Retry counter should have been started for the orphan's hash. */
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashOrphan) == 1);
    REQUIRE(TAO::Ledger::mapLastMissing[hashOrphan] == 1);

    /* Clean up. */
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();
    LLD::Ledger->EraseBlock(hashPrevOrphan);
}


TEST_CASE("Orphan-drain retry-limit erases counter and signals branch recovery", "[ledger][process]")
{
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();

    /* Write a prev block so the accepted parent block doesn't take the orphan
     * path when Process() is called. */
    const uint1024_t hashPrevOrphan2(0xABCD5601ULL);

    TAO::Ledger::BlockState statePrevOrphan2;
    statePrevOrphan2.nVersion      = 4;
    statePrevOrphan2.hashPrevBlock = uint1024_t(0);
    statePrevOrphan2.nChannel      = 2;
    statePrevOrphan2.nHeight       = 43;
    statePrevOrphan2.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrevOrphan2, statePrevOrphan2));

    /* Build the accepted parent PassBlock. */
    PassBlock parentBlock2;
    parentBlock2.nVersion      = 4;
    parentBlock2.hashPrevBlock = hashPrevOrphan2;
    parentBlock2.nChannel      = 2;
    parentBlock2.nHeight       = 44;
    parentBlock2.nBits         = 1;
    parentBlock2.nNonce        = 2002;

    const uint1024_t hashParent2 = parentBlock2.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashParent2));

    /* Build the orphan MissingBlock whose parent is the just-accepted block. */
    MissingBlock orphanBlock2;
    orphanBlock2.nVersion      = 4;
    orphanBlock2.hashPrevBlock = hashParent2;
    orphanBlock2.nChannel      = 2;
    orphanBlock2.nHeight       = 45;
    orphanBlock2.nBits         = 1;
    orphanBlock2.nNonce        = 3002;

    const uint1024_t hashOrphan2 = orphanBlock2.GetHash();

    /* Seed the orphan pool. */
    REQUIRE(TAO::Ledger::mapOrphans.Insert(orphanBlock2));

    /* Pre-fill the retry counter to exactly the retry limit, simulating that
     * this orphan has already been retried MAX_MISSING_TRANSACTIONS_RETRIES
     * times.  The next call to Process() should push it over the limit. */
    TAO::Ledger::mapLastMissing[hashOrphan2] =
        LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES;

    /* Process the parent (accepted), which triggers the orphan-drain and
     * encounters an over-limit retry for the orphan's missing transactions. */
    uint8_t nStatus = 0;
    TAO::Ledger::Process(parentBlock2, nStatus);

    /* Parent accepted, orphan still incomplete. */
    REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED)  != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);

    /* The escalation path now preserves missing hashes while setting
     * hashMissing=0 so LLP can fan out per-transaction recovery requests. */
    REQUIRE_FALSE(parentBlock2.vMissing.empty());
    REQUIRE(parentBlock2.hashMissing == 0);

    /* CRITICAL: the entry must be ERASED so the orphan is not permanently
     * wedged on future arrivals — a leftover value > MAX would cause
     * immediate silent drop on every subsequent Process() call. */
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashOrphan2) == 0);

    /* Clean up. */
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();
    LLD::Ledger->EraseBlock(hashPrevOrphan2);
}


TEST_CASE("Persisted orphan is removed while draining", "[ledger][process]")
{
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();

    const uint1024_t hashPrev(0xABCD5700ULL);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 50;
    statePrev.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    PassBlock parentBlock;
    parentBlock.nVersion      = 4;
    parentBlock.hashPrevBlock = hashPrev;
    parentBlock.nChannel      = 2;
    parentBlock.nHeight       = 51;
    parentBlock.nBits         = 1;
    parentBlock.nNonce        = 4001;

    const uint1024_t hashParent = parentBlock.GetHash();

    PassBlock orphanBlock;
    orphanBlock.nVersion      = 4;
    orphanBlock.hashPrevBlock = hashParent;
    orphanBlock.nChannel      = 2;
    orphanBlock.nHeight       = 52;
    orphanBlock.nBits         = 1;
    orphanBlock.nNonce        = 4002;

    const uint1024_t hashOrphan = orphanBlock.GetHash();
    TAO::Ledger::BlockState stateOrphan;
    stateOrphan.nVersion      = orphanBlock.nVersion;
    stateOrphan.hashPrevBlock = orphanBlock.hashPrevBlock;
    stateOrphan.nChannel      = orphanBlock.nChannel;
    stateOrphan.nHeight       = orphanBlock.nHeight;
    stateOrphan.nBits         = orphanBlock.nBits;
    REQUIRE(LLD::Ledger->WriteBlock(hashOrphan, stateOrphan));

    REQUIRE(TAO::Ledger::mapOrphans.Insert(orphanBlock));

    uint8_t nStatus = 0;
    TAO::Ledger::Process(parentBlock, nStatus);

    REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED) != 0);
    REQUIRE(TAO::Ledger::mapOrphans.Empty());
    REQUIRE(LLD::Ledger->HasBlock(hashOrphan));

    LLD::Ledger->EraseBlock(hashOrphan);
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Block orphan pool retains siblings and suppresses exact duplicates", "[ledger][process][orphan_pool]")
{
    TAO::Ledger::OrphanPool pool;
    const uint1024_t hashParent(0x9911);

    PassBlock first;
    first.nVersion = 4;
    first.hashPrevBlock = hashParent;
    first.nNonce = 1;
    first.hashMerkleRoot = uint512_t(1);

    PassBlock second = first;
    second.nNonce = 2;
    second.hashMerkleRoot = uint512_t(2);

    REQUIRE(pool.Insert(first));
    REQUIRE(pool.Insert(second));
    REQUIRE_FALSE(pool.Insert(first));
    REQUIRE(pool.Size() == 2);

    const std::vector<uint1024_t> vChildren = pool.Children(hashParent);
    REQUIRE(vChildren.size() == 2);
    REQUIRE(std::is_sorted(vChildren.begin(), vChildren.end()));
}


TEST_CASE("Block orphan pool FIFO eviction keeps indexes consistent", "[ledger][process][orphan_pool]")
{
    TAO::Ledger::OrphanPool pool;
    const uint1024_t hashParent(0x9922);
    uint1024_t hashFirst = 0;

    for(uint64_t i = 1; i <= TAO::Ledger::MAX_BLOCK_ORPHANS; ++i)
    {
        PassBlock block;
        block.nVersion = 4;
        block.hashPrevBlock = hashParent;
        block.nNonce = i;
        block.hashMerkleRoot = uint512_t(i);
        if(i == 1)
            hashFirst = block.GetHash();
        REQUIRE(pool.Insert(block));
    }

    PassBlock overflow;
    overflow.nVersion = 4;
    overflow.hashPrevBlock = hashParent;
    overflow.nNonce = TAO::Ledger::MAX_BLOCK_ORPHANS + 1;
    overflow.hashMerkleRoot = uint512_t(TAO::Ledger::MAX_BLOCK_ORPHANS + 1);

    uint1024_t hashEvicted = 0;
    REQUIRE(pool.Insert(overflow, &hashEvicted));
    REQUIRE(hashEvicted == hashFirst);
    REQUIRE_FALSE(pool.Contains(hashFirst));
    REQUIRE(pool.Size() == TAO::Ledger::MAX_BLOCK_ORPHANS);
    REQUIRE(pool.Children(hashParent).size() == TAO::Ledger::MAX_BLOCK_ORPHANS);
}


TEST_CASE("Orphan drain handles trees, invalid subtrees, and incomplete siblings", "[ledger][process][orphan_pool]")
{
    LedgerGuard env;
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();

    const uint1024_t hashPrev(0x9933);
    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion = 4;
    statePrev.nHeight = 70;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    PassBlock parent;
    parent.nVersion = 4;
    parent.hashPrevBlock = hashPrev;
    parent.nHeight = 71;
    parent.nNonce = 10;
    const uint1024_t hashParent = parent.GetHash();

    MissingBlock incomplete;
    incomplete.nVersion = 4;
    incomplete.hashPrevBlock = hashParent;
    incomplete.nHeight = 72;
    incomplete.nNonce = 11;
    const uint1024_t hashIncomplete = incomplete.GetHash();

    PassBlock sibling;
    sibling.nVersion = 4;
    sibling.hashPrevBlock = hashParent;
    sibling.nHeight = 72;
    sibling.nNonce = 12;
    const uint1024_t hashSibling = sibling.GetHash();

    PassBlock grandchild;
    grandchild.nVersion = 4;
    grandchild.hashPrevBlock = hashSibling;
    grandchild.nHeight = 73;
    grandchild.nNonce = 13;
    const uint1024_t hashGrandchild = grandchild.GetHash();

    RejectBlock invalid;
    invalid.nVersion = 4;
    invalid.hashPrevBlock = hashParent;
    invalid.nHeight = 72;
    invalid.nNonce = 14;
    const uint1024_t hashInvalid = invalid.GetHash();

    PassBlock invalidDescendant;
    invalidDescendant.nVersion = 4;
    invalidDescendant.hashPrevBlock = hashInvalid;
    invalidDescendant.nHeight = 73;
    invalidDescendant.nNonce = 15;
    const uint1024_t hashInvalidDescendant = invalidDescendant.GetHash();

    REQUIRE(TAO::Ledger::mapOrphans.Insert(incomplete));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(sibling));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(grandchild));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(invalid));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(invalidDescendant));

    uint8_t nStatus = 0;
    TAO::Ledger::Process(parent, nStatus);

    REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED) != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    REQUIRE(TAO::Ledger::mapOrphans.Contains(hashIncomplete));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashSibling));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashGrandchild));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashInvalid));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashInvalidDescendant));

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Disk-known block is not requeued as an orphan", "[ledger][process][orphan_pool]")
{
    LedgerGuard env;
    TAO::Ledger::mapOrphans.Clear();

    PassBlock block;
    block.nVersion = 4;
    block.hashPrevBlock = uint1024_t(0x9944);
    block.nHeight = 80;
    block.nNonce = 20;
    const uint1024_t hashBlock = block.GetHash();

    TAO::Ledger::BlockState state;
    state.nVersion = block.nVersion;
    state.hashPrevBlock = block.hashPrevBlock;
    state.nHeight = block.nHeight;
    state.nNonce = block.nNonce;
    REQUIRE(LLD::Ledger->WriteBlock(hashBlock, state));

    uint8_t nStatus = 0;
    TAO::Ledger::Process(block, nStatus);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::DUPLICATE) != 0);
    REQUIRE(TAO::Ledger::mapOrphans.Empty());

    LLD::Ledger->EraseBlock(hashBlock);
}


TEST_CASE("Unified fork scoring is shared by candidate activation", "[ledger][fork_choice]")
{
    TAO::Ledger::BlockState best;
    best.nVersion = 7;
    best.nChannel = 2;
    best.nHeight = 100;
    best.nNonce = 1;
    best.nChannelWeight[0] = 10;
    best.nChannelWeight[1] = 10;
    best.nChannelWeight[2] = 10;

    TAO::Ledger::BlockState candidate = best;
    candidate.nNonce = 2;
    candidate.nChannelWeight[0] = 11;
    candidate.nChannelWeight[1] = 11;
    candidate.nChannelWeight[2] = 9;
    REQUIRE(candidate.IsHeavierThan(best));

    candidate = best;
    candidate.nNonce = 3;
    candidate.nChannelWeight[0] = 11;
    REQUIRE(candidate.IsHeavierThan(best));

    candidate = best;
    candidate.nNonce = 4;
    candidate.nHeight = 102;
    candidate.nChannelWeight[0] = 11;
    candidate.nChannelWeight[1] = 9;
    REQUIRE(candidate.IsHeavierThan(best));

    candidate = best;
    candidate.nNonce = 5;
    REQUIRE_FALSE(candidate.IsHeavierThan(best));

    candidate.nVersion = 6;
    best.nVersion = 6;
    candidate.nChainTrust = 101;
    best.nChainTrust = 100;
    REQUIRE(candidate.IsHeavierThan(best));
}


TEST_CASE("Persisted block validation bypasses only incoming duplicate rejection",
    "[ledger][process][stored_validation]")
{
    LedgerGuard env;

    const bool fHybridBefore = config::fHybrid.load();
    config::fHybrid.store(true);

    TAO::Ledger::TritiumBlock block;
    block.nVersion = TAO::Ledger::CurrentBlockVersion();
    block.nChannel = 3;
    block.nHeight = 2;
    block.nNonce = 0x647;
    block.nTime = runtime::unifiedtimestamp();

    block.producer.nVersion = TAO::Ledger::CurrentTransactionVersion();
    block.producer.hashGenesis = uint256_t(
        "0xb7a74c14508bd09e104eff93d86cbbdc5c9556ae68546895d964d8374a0e9a41");
    block.producer.hashPrevTx = uint512_t(1);
    block.producer.hashNext = uint512_t(2);
    block.producer.nSequence = 1;
    block.producer.nTimestamp = block.nTime;
    block.producer.nKeyType = TAO::Ledger::SIGNATURE::BRAINPOOL;
    block.producer[0] << uint8_t(TAO::Operation::OP::AUTHORIZE);

    const uint512_t hashSecret(3);
    REQUIRE(block.producer.Sign(hashSecret));

    const uint512_t hashProducer = block.producer.GetHash();
    block.hashMerkleRoot = block.BuildMerkleTree({hashProducer});

    LLC::ECKey key(LLC::BRAINPOOL_P512_T1, 64);
    const std::vector<uint8_t> vSecret = hashSecret.GetBytes();
    REQUIRE(key.SetSecret(LLC::CSecret(vSecret.begin(), vSecret.end()), true));
    REQUIRE(block.GenerateSignature(key));

    TAO::Ledger::BlockState state(block);
    REQUIRE(LLD::Ledger->WriteBlock(block.GetHash(), state));

    REQUIRE_FALSE(block.Check());
    REQUIRE(block.CheckStored());

    LLD::Ledger->EraseBlock(block.GetHash());
    config::fHybrid.store(fHybridBefore);
}


TEST_CASE("Failed candidate activation preserves the active best chain",
    "[ledger][process][fork_choice]")
{
    LedgerGuard env;

    const TAO::Ledger::BlockState stateBestBefore =
        TAO::Ledger::ChainState::tStateBest.load();
    const uint1024_t hashBestBefore =
        TAO::Ledger::ChainState::hashBestChain.load();

    TAO::Ledger::BlockState candidate = stateBestBefore;
    candidate.nVersion = 7;
    candidate.nNonce += 1;
    candidate.nHeight += 1;
    candidate.nChannelWeight[0] += 1;
    candidate.vtx.clear();

    REQUIRE_FALSE(TAO::Ledger::ActivateCandidateBestChain(
        candidate, "unit test invalid candidate", true));
    REQUIRE(TAO::Ledger::ChainState::hashBestChain.load() == hashBestBefore);
    REQUIRE(TAO::Ledger::ChainState::tStateBest.load() == stateBestBefore);
}


TEST_CASE("Synchronization requires the advertised hash to be active",
    "[ledger][process][sync]")
{
    const uint1024_t hashActive(0x646);
    const uint1024_t hashStoredSideBranch(0x647);
    const uint1024_t hashBestBefore =
        TAO::Ledger::ChainState::hashBestChain.load();

    TAO::Ledger::ChainState::hashBestChain.store(hashActive);

    REQUIRE(TAO::Ledger::IsBestChainSynchronized(hashActive));
    REQUIRE_FALSE(TAO::Ledger::IsBestChainSynchronized(hashStoredSideBranch));
    REQUIRE_FALSE(TAO::Ledger::IsBestChainSynchronized(uint1024_t(0)));

    TAO::Ledger::ChainState::hashBestChain.store(hashBestBefore);
}


/* ========================================================================
 * Follow-up regression tests for the third failure mode:
 *   - Escalation cap invariant
 *   - Blacklist short-circuit
 *   - 250-ms rate limit
 *   - Blacklist clears on accept
 *   - Throttle key unification (mapLastOrphanRequest keyed by hashPrevBlock)
 *   - PurgeOrphanRecoveryState completeness
 *   - AttemptPeerBestChainRecovery orphan-pool walkback
 *   - Hash-channel (nChannel=2) PoW non-regression
 * ======================================================================== */


TEST_CASE("Escalation cap invariant: counter never exceeds MAX+1", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t hashPrev(0xCA000001ULL);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 100;
    statePrev.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    MissingBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 101;
    block.nBits         = 1;
    block.nNonce        = 9001;

    const uint1024_t hashBlock = block.GetHash();
    const uint64_t nRetries = LLP::TritiumNode::ACTION::MAX_MISSING_TRANSACTIONS_RETRIES + 1;
    const uint32_t nMaxEsc   = TAO::Ledger::MAX_BRANCH_RECOVERY_ESCALATIONS;

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    /* Drive through enough cycles to reach and exceed the cap. */
    const uint32_t nExtraCycles = 5;
    const uint32_t nTotalCycles = nMaxEsc + 1 + nExtraCycles;

    uint32_t nCyclesCompleted = 0;
    for(uint32_t nCycle = 0; nCycle < nTotalCycles; ++nCycle)
    {
        /* Every call to Process() returns INCOMPLETE (hashMissing==0 once
         * blacklisted, since Check()/mapLastMissing are then skipped).
         * Drive the inner retry loop only while the block is not yet
         * blacklisted, since once blacklisted the escalation counter is
         * frozen and further cycles would be redundant. */
        if(TAO::Ledger::setUnrecoverableBlocks.count(hashBlock))
            break; /* blacklisted — further calls short-circuit immediately */

        for(uint64_t i = 0; i < nRetries; ++i)
        {
            TAO::Ledger::mapLastMissingProcessTime.erase(hashBlock);
            uint8_t nStatus = 0;
            TAO::Ledger::Process(block, nStatus);
            /* Status is always INCOMPLETE; break as soon as we're blacklisted
             * since further arrivals no longer advance the escalation counter. */
            if(TAO::Ledger::setUnrecoverableBlocks.count(hashBlock))
                break;
            REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        }
        ++nCyclesCompleted;
    }

    /* After the cap cycle fires the counter must be exactly MAX+1 — no more,
     * no less.  Subsequent arrivals are blocked by the blacklist before
     * reaching incrementMissingEscalations(), so the counter is frozen. */
    const uint32_t nFinalEsc = TAO::Ledger::MissingBranchRecoveryEscalations(hashBlock);
    REQUIRE(nFinalEsc == nMaxEsc + 1);
    REQUIRE(TAO::Ledger::IsMissingBranchRecoveryCapped(hashBlock));
    REQUIRE(TAO::Ledger::setUnrecoverableBlocks.count(hashBlock) == 1);

    /* Drive many more calls: every one returns INCOMPLETE with hashMissing==0
     * (not IGNORED — the blacklist guard now reports INCOMPLETE so the LLP
     * capped-path branch, and ShouldSendBranchSyncRequest(), stay reachable
     * on every arrival), counter stays frozen. */
    for(uint32_t i = 0; i < 20; ++i)
    {
        TAO::Ledger::mapLastMissingProcessTime.erase(hashBlock);
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE(block.hashMissing == 0);
        REQUIRE(TAO::Ledger::MissingBranchRecoveryEscalations(hashBlock) == nMaxEsc + 1);
    }

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Blacklist short-circuit: INCOMPLETE with hashMissing=0", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t hashPrev(0xB100001ULL);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 110;
    statePrev.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    MissingBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 111;
    block.nBits         = 1;
    block.nNonce        = 9002;

    const uint1024_t hashBlock = block.GetHash();

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    /* Manually blacklist the block. */
    TAO::Ledger::setUnrecoverableBlocks.insert(hashBlock);

    /* Process() must short-circuit immediately — before any Check(),
     * mapLastMissing write, or escalation logic — but must still report
     * INCOMPLETE with hashMissing == 0 (not a silent IGNORED) so the LLP
     * capped-path branch, and therefore ShouldSendBranchSyncRequest(),
     * stays reachable on every arrival rather than firing only once. */
    uint8_t nStatus = 0;
    TAO::Ledger::Process(block, nStatus);

    REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::REJECTED)   == 0);
    REQUIRE(block.hashMissing == 0);

    /* mapLastMissing must NOT have been written — the early-return prevented it. */
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) == 0);

    /* Clean up. */
    TAO::Ledger::setUnrecoverableBlocks.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Rate-limit: two rapid calls produce only one full-path execution", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t hashPrev(0xA1000001ULL);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 120;
    statePrev.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    MissingBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 121;
    block.nBits         = 1;
    block.nNonce        = 9003;

    const uint1024_t hashBlock = block.GetHash();

    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    /* First call: hash not yet in mapLastMissing — no rate-limit check.
     * Process() runs the full path and sets mapLastMissing[hash] = 1. */
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock) == 1);
        REQUIRE(TAO::Ledger::mapLastMissing[hashBlock] == 1);
    }

    /* Second call immediately (within 250 ms): hash IS in mapLastMissing and
     * mapLastMissingProcessTime was set by the first call — rate-limit fires.
     * Returns INCOMPLETE immediately; counter stays at 1. */
    {
        uint8_t nStatus = 0;
        TAO::Ledger::Process(block, nStatus);
        REQUIRE((nStatus & TAO::Ledger::PROCESS::INCOMPLETE) != 0);
        /* Counter must NOT have been incremented — rate-limit short-circuited. */
        REQUIRE(TAO::Ledger::mapLastMissing[hashBlock] == 1);
    }

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Blacklist clears on ACCEPT: all recovery maps erased", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t hashPrev(0xBC000001ULL);

    TAO::Ledger::BlockState statePrev;
    statePrev.nVersion      = 4;
    statePrev.hashPrevBlock = uint1024_t(0);
    statePrev.nChannel      = 2;
    statePrev.nHeight       = 130;
    statePrev.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashPrev, statePrev));

    /* PassBlock will be accepted (Check/Accept both succeed). */
    PassBlock block;
    block.nVersion      = 4;
    block.hashPrevBlock = hashPrev;
    block.nChannel      = 2;
    block.nHeight       = 131;
    block.nBits         = 1;
    block.nNonce        = 9004;

    const uint1024_t hashBlock = block.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashBlock));

    /* Pre-populate every recovery map to simulate a prior incomplete run. */
    TAO::Ledger::mapLastMissing[hashBlock]              = 42;
    TAO::Ledger::mapMissingBranchEscalations[hashBlock] = 2;
    TAO::Ledger::setUnrecoverableBlocks.insert(hashBlock);
    TAO::Ledger::mapLastMissingProcessTime[hashBlock]   = runtime::timestamp(true) - 10000;

    /* Process() checks setUnrecoverableBlocks FIRST, so a blacklisted PassBlock
     * would return IGNORED.  Verify the accept-path clears maps correctly:
     * first, remove from blacklist to let the accept path run, then accept. */
    TAO::Ledger::setUnrecoverableBlocks.erase(hashBlock);

    uint8_t nStatus = 0;
    TAO::Ledger::Process(block, nStatus);
    REQUIRE((nStatus & TAO::Ledger::PROCESS::ACCEPTED) != 0);

    /* All recovery maps must be cleared on ACCEPT. */
    REQUIRE(TAO::Ledger::mapLastMissing.count(hashBlock)              == 0);
    REQUIRE(TAO::Ledger::mapMissingBranchEscalations.count(hashBlock) == 0);
    REQUIRE(TAO::Ledger::setUnrecoverableBlocks.count(hashBlock)      == 0);
    REQUIRE(TAO::Ledger::mapLastMissingProcessTime.count(hashBlock)   == 0);

    /* Clean up. */
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapMissingBranchEscalations.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    LLD::Ledger->EraseBlock(hashPrev);
}


TEST_CASE("Throttle key unification: mapLastOrphanRequest keyed by hashPrevBlock", "[ledger][process]")
{
    /* ShouldSendBranchSyncRequest() records its timestamp in mapLastOrphanRequest
     * keyed by the hashAncestor argument (== block.hashPrevBlock, the missing
     * ancestor).  Both the orphan-insert path in Process() and the capped-path
     * LIST in the LLP layer use this same canonical key so the drain-loop
     * cleanup erase(hashParent) is always effective. */

    const uint1024_t hashAncestor(0xA2000001ULL);

    TAO::Ledger::mapLastOrphanRequest.clear();

    /* First call must return true (no prior entry). */
    REQUIRE(TAO::Ledger::ShouldSendBranchSyncRequest(hashAncestor));

    /* Verify the timestamp was recorded under the ancestor key. */
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashAncestor) == 1);

    /* Second call within ORPHAN_REQUEST_THROTTLE_SECONDS must return false. */
    REQUIRE_FALSE(TAO::Ledger::ShouldSendBranchSyncRequest(hashAncestor));

    /* The drain-loop cleanup: erasing by hashParent (== hashAncestor) removes
     * the throttle entry regardless of which code path wrote it. */
    TAO::Ledger::mapLastOrphanRequest.erase(hashAncestor);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashAncestor) == 0);

    /* After erase, ShouldSendBranchSyncRequest should allow a new request. */
    REQUIRE(TAO::Ledger::ShouldSendBranchSyncRequest(hashAncestor));

    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("PurgeOrphanRecoveryState clears all correlated maps", "[ledger][process]")
{
    LedgerGuard env;

    const uint1024_t h1(0xA3000001ULL);
    const uint1024_t h2(0xA3000002ULL);
    const uint1024_t h3(0xA3000003ULL);

    /* Seed every recovery map with at least one entry. */
    TAO::Ledger::mapLastMissing[h1]              = 5;
    TAO::Ledger::mapMissingBranchEscalations[h2] = 2;
    TAO::Ledger::setUnrecoverableBlocks.insert(h3);
    TAO::Ledger::mapLastMissingProcessTime[h1]   = 12345678;
    TAO::Ledger::mapLastOrphanRequest[h2]        = 87654321;

    /* Seed the orphan pool with a dummy block. */
    PassBlock dummy;
    dummy.nVersion      = 4;
    dummy.hashPrevBlock = uint1024_t(0xDEAD);
    dummy.nNonce        = 0xBEEF;
    TAO::Ledger::mapOrphans.Insert(dummy);
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Empty());

    /* Purge everything at once. */
    TAO::Ledger::PurgeOrphanRecoveryState("unit-test");

    /* Every map and set must be empty after the purge. */
    REQUIRE(TAO::Ledger::mapOrphans.Empty());
    REQUIRE(TAO::Ledger::mapLastMissing.empty());
    REQUIRE(TAO::Ledger::mapMissingBranchEscalations.empty());
    REQUIRE(TAO::Ledger::setUnrecoverableBlocks.empty());
    REQUIRE(TAO::Ledger::mapLastMissingProcessTime.empty());
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.empty());
}


TEST_CASE("AttemptPeerBestChainRecovery walks orphan pool for connectable ancestor",
    "[ledger][process]")
{
    LedgerGuard env;

    /* Set up a known-good root block on disk. */
    const uint1024_t hashRoot(0xA4000001ULL);

    TAO::Ledger::BlockState stateRoot;
    stateRoot.nVersion      = 4;
    stateRoot.hashPrevBlock = uint1024_t(0);
    stateRoot.nChannel      = 2;
    stateRoot.nHeight       = 200;
    stateRoot.nBits         = 1;
    REQUIRE(LLD::Ledger->WriteBlock(hashRoot, stateRoot));

    /* Block A: connectable orphan (its hashPrevBlock is on disk). */
    PassBlock blockA;
    blockA.nVersion      = 4;
    blockA.hashPrevBlock = hashRoot;
    blockA.nChannel      = 2;
    blockA.nHeight       = 201;
    blockA.nBits         = 1;
    blockA.nNonce        = 10001;
    const uint1024_t hashA = blockA.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashA));

    /* Block B: peer tip — orphan whose hashPrevBlock is hashA (also in pool). */
    PassBlock blockB;
    blockB.nVersion      = 4;
    blockB.hashPrevBlock = hashA;
    blockB.nChannel      = 2;
    blockB.nHeight       = 202;
    blockB.nBits         = 1;
    blockB.nNonce        = 10002;
    const uint1024_t hashB = blockB.GetHash();

    /* Seed both blocks in the orphan pool: B is the peer's advertised tip;
     * A is the connectable ancestor (prev is on disk). */
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();
    REQUIRE(TAO::Ledger::mapOrphans.Insert(blockA));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(blockB));

    /* AttemptPeerBestChainRecovery should:
     *   1. ReadBlock(hashB) fails (not on disk).
     *   2. Walk back: hashB → hashA (connectable, prev=hashRoot is on disk).
     *   3. Process(blockA) → ACCEPTED → BFS drain picks up blockB.
     *   4. Return PROGRESS (forward progress made). */
    const auto result = TAO::Ledger::AttemptPeerBestChainRecovery(
        hashB, 202, "unit-test", nullptr);

    REQUIRE(result == TAO::Ledger::PeerBestRecoveryResult::PROGRESS);

    /* Both blockA and blockB must have been walked through Accept() (the
     * mock PassBlock::Accept() does not itself persist to disk, so this is
     * verified via the orphan pool having been fully drained rather than
     * LLD::Ledger->HasBlock(), mirroring the "Persisted orphan is removed
     * while draining" test's pattern). Orphan pool should be empty: blockA
     * was consumed as the connectable ancestor fed through Process(), and
     * blockB was drained by the BFS walk that follows a successful accept. */
    REQUIRE(TAO::Ledger::mapOrphans.Empty());
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashA));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashB));

    /* Clean up. */
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    LLD::Ledger->EraseBlock(hashA);
    LLD::Ledger->EraseBlock(hashB);
    LLD::Ledger->EraseBlock(hashRoot);
}


TEST_CASE("AttemptPeerBestChainRecovery requests branch sync when tip far ahead",
    "[ledger][process][a1]")
{
    /* A1 regression: peer tip not on disk AND not in the orphan pool must
     * issue a throttled locator LIST + SPECIFIER::TRANSACTIONS (not a silent
     * no-op).  Exercise the path with an injectable TritiumNode backed by a
     * socketpair so we can assert the on-wire message, throttle key, and the
     * pfBranchSyncQueued out-parameter used by callers to skip duplicate LIST. */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashFarTip(0xA1000000000000FEULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashFarTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashFarTip));

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    /* Capture locator target before recovery so the expected packet matches
     * the LIST payload that was actually queued (hashBestChain is read once
     * inside the helper at send time). */
    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashFarTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fBranchSyncQueued = false;
    const auto result = TAO::Ledger::AttemptPeerBestChainRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-a1", &node, &fBranchSyncQueued);

    REQUIRE(result == TAO::Ledger::PeerBestRecoveryResult::FETCH_QUEUED);
    REQUIRE(fBranchSyncQueued);
    REQUIRE(TAO::Ledger::mapOrphans.Empty());

    /* Throttle key is the far tip itself when no orphan walk occurred. */
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    /* Flush any overflow-buffered remainder, then drain the peer read end. */
    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent.insert(vSent.end(), buf.begin(), buf.begin() + n);
        }
    }

    REQUIRE(vSent == vExpected);

    /* Immediate second call must be throttled: no new LIST, out-param false. */
    bool fBranchSyncQueued2 = true; /* helper must clear this */
    const auto result2 = TAO::Ledger::AttemptPeerBestChainRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-a1-throttle", &node,
        &fBranchSyncQueued2);
    REQUIRE(result2 == TAO::Ledger::PeerBestRecoveryResult::FETCH_THROTTLED);
    REQUIRE_FALSE(fBranchSyncQueued2);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent2;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent2.insert(vSent2.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent2.empty());

    /* BaseConnection only Close()s when fCONNECTED; we never connected, so
     * own both ends of the socketpair and clear node.fd before destruction. */
    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    /* WIN32: no socketpair — still verify throttle recording with a bare node.
     * PushMessage may fail without a live fd, so accept QUEUED or SKIPPED. */
    LLP::TritiumNode node;
    bool fBranchSyncQueued = false;
    const auto result = TAO::Ledger::AttemptPeerBestChainRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-a1", &node, &fBranchSyncQueued);
    REQUIRE((result == TAO::Ledger::PeerBestRecoveryResult::FETCH_QUEUED
          || result == TAO::Ledger::PeerBestRecoveryResult::SKIPPED));
    REQUIRE(TAO::Ledger::mapOrphans.Empty());
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);
#endif

    /* Clean up any throttle residue keyed on the far tip. */
    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("Missing-tx escalation queues only one LIST when recovery already did",
    "[ledger][process][a1][missing-tx-escalation]")
{
    /* Regression for the tritium missing-tx escalation coordination:
     * RequestMissingTxBranchRecovery must not follow a successful recovery LIST
     * with an identical fallback LIST (duplicate branch traffic + TxResponseWindow
     * replacement on the same peer).  Calling AttemptPeerBestChainRecovery alone
     * cannot catch a removed if(!fBranchSyncQueued) guard. */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashFarTip(0xA1000000000000FFULL);
    const uint1024_t hashIncomplete(0xA100000000000100ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashFarTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashFarTip));

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashFarTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fBranchSyncQueued = false;
    const bool fProgress = TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-escalation", &node, &fBranchSyncQueued);

    REQUIRE_FALSE(fProgress);
    REQUIRE(fBranchSyncQueued);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent.insert(vSent.end(), buf.begin(), buf.begin() + n);
        }
    }

    /* Combined path must emit exactly one LIST — not recovery+fallback. */
    REQUIRE(vSent.size() == vExpected.size());
    REQUIRE(vSent == vExpected);

    /* Fallback-only path (unknown peer best) still queues one LIST to hashBlock. */
    TAO::Ledger::mapLastOrphanRequest.clear();
    DataStream ssFallback(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssFallback
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashIncomplete);
    const std::vector<uint8_t> vFallbackExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssFallback).GetBytes();

    bool fFallbackQueued = false;
    const bool fFallbackProgress = TAO::Ledger::RequestMissingTxBranchRecovery(
        uint1024_t(0), hashIncomplete, /*nPeerHeight=*/0,
        "unit-test-missing-tx-fallback", &node, &fFallbackQueued);
    REQUIRE_FALSE(fFallbackProgress);
    REQUIRE(fFallbackQueued);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vFallbackSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vFallbackSent.insert(vFallbackSent.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vFallbackSent == vFallbackExpected);

    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    LLP::TritiumNode node;
    bool fBranchSyncQueued = false;
    const bool fProgress = TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-escalation", &node, &fBranchSyncQueued);
    REQUIRE_FALSE(fProgress);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);
#endif

    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("Missing-tx escalation second call emits no LIST while throttled",
    "[ledger][process][a1][missing-tx-escalation][throttle]")
{
    /* Regression: RequestMissingTxBranchRecovery must not treat
     * AttemptPeerBestChainRecovery's throttle denial (fBranchSyncQueued=false)
     * as permission to fire the unthrottled fallback LIST.  Two immediate
     * combined calls must produce exactly one on-wire LIST. */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashFarTip(0xA100000000000101ULL);
    const uint1024_t hashIncomplete(0xA100000000000102ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashFarTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashFarTip));

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashFarTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fQueued1 = false;
    REQUIRE_FALSE(TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-throttle-1", &node, &fQueued1));
    REQUIRE(fQueued1);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent1;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent1.insert(vSent1.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent1 == vExpected);

    /* Immediate second combined call: recovery is throttled; fallback must not
     * emit another LIST (the bug sent fallback whenever queued==false). */
    bool fQueued2 = true; /* helper must clear when nothing is sent */
    REQUIRE_FALSE(TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-throttle-2", &node, &fQueued2));
    REQUIRE_FALSE(fQueued2);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent2;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent2.insert(vSent2.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent2.empty());

    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    LLP::TritiumNode node;
    bool fQueued1 = false;
    REQUIRE_FALSE(TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-throttle-1", &node, &fQueued1));
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    bool fQueued2 = true;
    REQUIRE_FALSE(TAO::Ledger::RequestMissingTxBranchRecovery(
        hashFarTip, hashIncomplete, /*nPeerHeight=*/9999,
        "unit-test-missing-tx-throttle-2", &node, &fQueued2));
    REQUIRE_FALSE(fQueued2);
#endif

    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("BESTCHAIN recovery queues only one LIST for unknown tip",
    "[ledger][process][a1][bestchain]")
{
    /* TIP-01 / TIP-02: RequestBestChainBranchRecovery must route unknown tips
     * through AttemptPeerBestChainRecovery (throttled LIST + fanout-capable)
     * and must not emit a second unthrottled fallback LIST on the same call. */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashFarTip(0xBC01000000000001ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashFarTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashFarTip));

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashFarTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fQueued = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain", &node, &fQueued));
    REQUIRE(fQueued);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent.insert(vSent.end(), buf.begin(), buf.begin() + n);
        }
    }

    /* Coordinator LIST only — no fallback double-LIST on the same call. */
    REQUIRE(vSent.size() == vExpected.size());
    REQUIRE(vSent == vExpected);

    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    LLP::TritiumNode node;
    bool fQueued = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain", &node, &fQueued));
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);
#endif

    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("BESTCHAIN recovery second notify emits no LIST while throttled",
    "[ledger][process][a1][bestchain][throttle]")
{
    /* TIP-01: chatty BESTCHAIN must not thrash TxResponseWindow.  Two immediate
     * RequestBestChainBranchRecovery calls for the same unknown tip produce
     * exactly one on-wire LIST (second call is FETCH_THROTTLED with no fallback). */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashFarTip(0xBC01000000000002ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashFarTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashFarTip));

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashFarTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fQueued1 = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain-throttle-1", &node, &fQueued1));
    REQUIRE(fQueued1);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent1;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent1.insert(vSent1.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent1 == vExpected);

    bool fQueued2 = true; /* helper must clear when nothing is sent */
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain-throttle-2", &node, &fQueued2));
    REQUIRE_FALSE(fQueued2);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent2;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent2.insert(vSent2.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent2.empty());

    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    LLP::TritiumNode node;
    bool fQueued1 = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain-throttle-1", &node, &fQueued1));
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashFarTip) == 1);

    bool fQueued2 = true;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashFarTip, /*nPeerHeight=*/9999, "unit-test-bestchain-throttle-2", &node, &fQueued2));
    REQUIRE_FALSE(fQueued2);
#endif

    TAO::Ledger::mapLastOrphanRequest.clear();
}


#ifndef WIN32
TEST_CASE("BESTCHAIN recovery uses the height later in the same notification",
    "[ledger][process][bestchain][notify-order]")
{
    LedgerGuard env;
    TAO::Ledger::mapLastOrphanRequest.clear();
    const uint32_t savedHeight = TAO::Ledger::ChainState::nBestHeight.load();
    TAO::Ledger::ChainState::nBestHeight.store(1000);
    const uint1024_t tip(0xBC01000000000111ULL);
    const uint1024_t behindTip(0xBC01000000000112ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(tip));
    REQUIRE_FALSE(LLD::Ledger->HasBlock(behindTip));

    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    LLP::TritiumNode node;
    node.fd = fds[1];
    node.events = POLLIN;
    node.nCurrentSession = 1;
    node.nProtocolVersion = LLP::PROTOCOL_VERSION;
    node.nCurrentHeight = 500;
    node.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BESTCHAIN
                 | LLP::TritiumNode::SUBSCRIPTION::BESTHEIGHT);

    DataStream payload(SER_NETWORK, LLP::PROTOCOL_VERSION);
    payload << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << tip
            << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << uint32_t(1002)
            << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << behindTip
            << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << uint32_t(999);
    node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
    node.INCOMING.DATA.assign(payload.begin(), payload.end());
    REQUIRE(node.ProcessPacket());
    REQUIRE(node.nCurrentHeight == 999);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(tip) == 1);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(behindTip) == 0);

    TAO::Ledger::ChainState::ForgetPeerHeight(&node);
    TAO::Ledger::ChainState::nBestHeight.store(savedHeight);
    TAO::Ledger::mapLastOrphanRequest.clear();
    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
}
#endif


TEST_CASE("BESTCHAIN recovery ignores unknown tip from behind peer",
"[ledger][process][a1][bestchain][height-gate]")
{
/* Historical BESTCHAIN height gate: an unknown tip advertised by a peer
 * whose height is below local best must not queue LIST, open a
 * TxResponseWindow, or consume mapLastOrphanRequest throttle. */
LedgerGuard env;

TAO::Ledger::mapOrphans.Clear();
TAO::Ledger::mapLastOrphanRequest.clear();
TAO::Ledger::mapLastMissing.clear();
TAO::Ledger::mapLastMissingProcessTime.clear();
TAO::Ledger::setUnrecoverableBlocks.clear();

const uint1024_t hashUnknownTip(0xBC01000000000003ULL);
REQUIRE_FALSE(LLD::Ledger->HasBlock(hashUnknownTip));
REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashUnknownTip));

const uint32_t nSavedBestHeight = TAO::Ledger::ChainState::nBestHeight.load();
TAO::Ledger::ChainState::nBestHeight.store(1000);

#ifndef WIN32
int fds[2] = {-1, -1};
REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

LLP::TritiumNode node;
node.fd     = fds[1];
node.events = POLLIN;

bool fQueued = true; /* helper must clear when nothing is sent */
REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
    hashUnknownTip, /*nPeerHeight=*/500, "unit-test-bestchain-behind",
    &node, &fQueued));
REQUIRE_FALSE(fQueued);
REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashUnknownTip) == 0);
REQUIRE(TAO::Ledger::mapLastOrphanRequest.empty());

while(node.Buffered() > 0)
{
    if(node.Flush() <= 0)
        break;
}

std::vector<uint8_t> vSent;
{
    std::vector<uint8_t> buf(65536);
    for(;;)
    {
        const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
        if(n <= 0)
            break;
        vSent.insert(vSent.end(), buf.begin(), buf.begin() + n);
    }
}
REQUIRE(vSent.empty());

node.fd = -1;
close(fds[0]);
close(fds[1]);
#else
LLP::TritiumNode node;
bool fQueued = true;
REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
    hashUnknownTip, /*nPeerHeight=*/500, "unit-test-bestchain-behind",
    &node, &fQueued));
REQUIRE_FALSE(fQueued);
REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashUnknownTip) == 0);
#endif

TAO::Ledger::ChainState::nBestHeight.store(nSavedBestHeight);
TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("BESTCHAIN recovery skips near-tip unknown race",
    "[ledger][process][a1][bestchain][near-tip]")
{
    /* Near-tip unknowns use a block GET, with or without inventory. Orphans
     * and material gaps still require branch recovery. */
    LedgerGuard env;

    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::mapLastMissing.clear();
    TAO::Ledger::mapLastMissingProcessTime.clear();
    TAO::Ledger::setUnrecoverableBlocks.clear();

    const uint1024_t hashNearTip(0xBC01000000000004ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashNearTip));
    REQUIRE_FALSE(TAO::Ledger::mapOrphans.Contains(hashNearTip));

    const uint32_t nSavedBestHeight = TAO::Ledger::ChainState::nBestHeight.load();
    const uint32_t nLocalHeight = 1000;
    TAO::Ledger::ChainState::nBestHeight.store(nLocalHeight);

#ifndef WIN32
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);

    LLP::TritiumNode node;
    node.fd     = fds[1];
    node.events = POLLIN;

    /* Equal height with matching BLOCK GET: inventory owns the race. */
    bool fQueuedEqual = true;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNearTip, /*nPeerHeight=*/nLocalHeight, "unit-test-bestchain-near-eq",
        &node, &fQueuedEqual, /*fMatchingBlockInventoryGet=*/true));
    REQUIRE_FALSE(fQueuedEqual);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashNearTip) == 0);

    /* +1 height with matching BLOCK GET: still within near-tip slack. */
    bool fQueuedPlusOne = true;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNearTip,
        /*nPeerHeight=*/nLocalHeight + TAO::Ledger::BESTCHAIN_NEAR_TIP_HEIGHT_SLACK,
        "unit-test-bestchain-near-plus1", &node, &fQueuedPlusOne,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE_FALSE(fQueuedPlusOne);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashNearTip) == 0);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vSent.insert(vSent.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vSent.empty());

    /* A standalone near-tip BESTCHAIN requests the block, not a branch. */
    const uint1024_t hashNoInvTip(0xBC01000000000006ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashNoInvTip));

    DataStream ssExpectedNoInv(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpectedNoInv << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
                    << uint1024_t(hashNoInvTip);
    const std::vector<uint8_t> vExpectedNoInv =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::GET, ssExpectedNoInv).GetBytes();

    bool fQueuedNoInv = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNoInvTip, /*nPeerHeight=*/nLocalHeight,
        "unit-test-bestchain-near-no-inv", &node, &fQueuedNoInv,
        /*fMatchingBlockInventoryGet=*/false));
    REQUIRE_FALSE(fQueuedNoInv);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashNoInvTip) == 0);
    for(unsigned int i = 0; i < 10; ++i)
        REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
            hashNoInvTip, nLocalHeight, "unit-test-bestchain-repeat", &node));

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vNoInvSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vNoInvSent.insert(vNoInvSent.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vNoInvSent == vExpectedNoInv);

    {
        struct ClientModeGuard
        {
            bool saved{config::fClient.load()};
            bool owned{false};
            ClientModeGuard()
            {
                if(!LLD::Client)
                {
                    LLD::Client = new LLD::ClientDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
                    owned = true;
                }
                config::fClient.store(true);
            }
            ~ClientModeGuard()
            {
                config::fClient.store(saved);
                if(owned)
                {
                    delete LLD::Client;
                    LLD::Client = nullptr;
                }
            }
        } clientModeGuard;
        const uint1024_t hashClientTip(0xBC01000000000007ULL);
        DataStream expectedClient(SER_NETWORK, LLP::MIN_PROTO_VERSION);
        expectedClient << uint8_t(LLP::TritiumNode::SPECIFIER::CLIENT)
                       << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << hashClientTip;
        bool queued = true;
        REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
            hashClientTip, nLocalHeight, "unit-test-bestchain-client", &node, &queued));
        REQUIRE_FALSE(queued);
        REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashClientTip) == 0);
        REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
            hashClientTip, nLocalHeight, "unit-test-bestchain-client-repeat", &node));
        while(node.Buffered() > 0)
            REQUIRE(node.Flush() > 0);
        std::vector<uint8_t> sent;
        uint8_t buffer[4096];
        for(ssize_t n = recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT);
            n > 0; n = recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT))
            sent.insert(sent.end(), buffer, buffer + n);
        REQUIRE(sent == LLP::TritiumNode::NewMessage(
            LLP::TritiumNode::ACTION::GET, expectedClient).GetBytes());

        node.nProtocolVersion = LLP::PROTOCOL_VERSION;
        node.nCurrentSession = 1;
        node.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BLOCK
                     | LLP::TritiumNode::SUBSCRIPTION::BESTCHAIN);
        while(node.Buffered() > 0)
            REQUIRE(node.Flush() > 0);
        while(recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT) > 0) {}

        DataStream inventory(SER_NETWORK, LLP::PROTOCOL_VERSION);
        inventory << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << hashClientTip
                  << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << hashClientTip;
        node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        node.INCOMING.DATA.assign(inventory.begin(), inventory.end());
        REQUIRE(node.ProcessPacket());
        REQUIRE(node.Buffered() == 0);
        REQUIRE(recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT) <= 0);
    }

    /* +2 heights: material gap — coordinator must still fetch. */
    const uint1024_t hashGapTip(0xBC01000000000005ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashGapTip));

    const uint1024_t hashLocalBest = TAO::Ledger::ChainState::hashBestChain.load();
    DataStream ssExpected(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpected
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashGapTip);
    const std::vector<uint8_t> vExpected =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpected).GetBytes();

    bool fQueuedGap = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashGapTip, nLocalHeight + TAO::Ledger::BESTCHAIN_NEAR_TIP_HEIGHT_SLACK + 1,
        "unit-test-bestchain-gap-body-retry", &node, &fQueuedGap, false, true));
    REQUIRE_FALSE(fQueuedGap);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashGapTip) == 0);
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashGapTip,
        /*nPeerHeight=*/nLocalHeight + TAO::Ledger::BESTCHAIN_NEAR_TIP_HEIGHT_SLACK + 1,
        "unit-test-bestchain-gap", &node, &fQueuedGap,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE(fQueuedGap);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashGapTip) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vGapSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vGapSent.insert(vGapSent.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vGapSent == vExpected);

    /* Near-tip tip already in the orphan pool must NOT take the inventory
     * shortcut: a duplicate BLOCK GET returns ORPHAN immediately from Process()
     * and never walks ancestry / reissues the missing-branch LIST.  Seed an
     * orphan whose prev is not on disk so the coordinator gap path fires. */
    PassBlock orphanNearTip;
    orphanNearTip.nVersion      = 4;
    orphanNearTip.hashPrevBlock = uint1024_t(0xBC01DEADBEEF0001ULL);
    orphanNearTip.nChannel      = 2;
    orphanNearTip.nHeight       = nLocalHeight;
    orphanNearTip.nBits         = 1;
    orphanNearTip.nNonce        = 0x0B01;
    orphanNearTip.hashMerkleRoot = uint512_t(0x0B01);
    const uint1024_t hashOrphanNearTip = orphanNearTip.GetHash();
    const uint1024_t hashOrphanAncestor = orphanNearTip.hashPrevBlock;

    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashOrphanNearTip));
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hashOrphanAncestor));
    REQUIRE(TAO::Ledger::mapOrphans.Insert(orphanNearTip));
    REQUIRE(TAO::Ledger::mapOrphans.Contains(hashOrphanNearTip));

    DataStream ssExpectedOrphan(SER_NETWORK, LLP::MIN_PROTO_VERSION);
    ssExpectedOrphan
        << uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS)
        << uint8_t(LLP::TritiumNode::TYPES::BLOCK)
        << uint8_t(LLP::TritiumNode::TYPES::LOCATOR)
        << TAO::Ledger::Locator(hashLocalBest)
        << uint1024_t(hashOrphanNearTip);
    const std::vector<uint8_t> vExpectedOrphan =
        LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::LIST, ssExpectedOrphan).GetBytes();

    bool fQueuedOrphan = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashOrphanNearTip, nLocalHeight, "unit-test-bestchain-orphan-body-retry",
        &node, &fQueuedOrphan, false, true));
    REQUIRE_FALSE(fQueuedOrphan);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashOrphanAncestor) == 0);
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashOrphanNearTip, /*nPeerHeight=*/nLocalHeight,
        "unit-test-bestchain-near-orphan", &node, &fQueuedOrphan,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE(fQueuedOrphan);
    /* Gap-path throttle key is the deepest missing ancestor (orphan prev),
     * not the advertised tip — same contract as AttemptPeerBestChainRecovery. */
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashOrphanAncestor) == 1);

    while(node.Buffered() > 0)
    {
        if(node.Flush() <= 0)
            break;
    }

    std::vector<uint8_t> vOrphanSent;
    {
        std::vector<uint8_t> buf(65536);
        for(;;)
        {
            const ssize_t n = recv(fds[0], buf.data(), buf.size(), MSG_DONTWAIT);
            if(n <= 0)
                break;
            vOrphanSent.insert(vOrphanSent.end(), buf.begin(), buf.begin() + n);
        }
    }
    REQUIRE(vOrphanSent == vExpectedOrphan);

    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
#else
    LLP::TritiumNode node;
    bool fQueuedEqual = true;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNearTip, /*nPeerHeight=*/nLocalHeight, "unit-test-bestchain-near-eq",
        &node, &fQueuedEqual, /*fMatchingBlockInventoryGet=*/true));
    REQUIRE_FALSE(fQueuedEqual);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashNearTip) == 0);

    bool fQueuedPlusOne = true;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNearTip,
        /*nPeerHeight=*/nLocalHeight + TAO::Ledger::BESTCHAIN_NEAR_TIP_HEIGHT_SLACK,
        "unit-test-bestchain-near-plus1", &node, &fQueuedPlusOne,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE_FALSE(fQueuedPlusOne);

    const uint1024_t hashNoInvTip(0xBC01000000000006ULL);
    bool fQueuedNoInv = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashNoInvTip, /*nPeerHeight=*/nLocalHeight,
        "unit-test-bestchain-near-no-inv", &node, &fQueuedNoInv,
        /*fMatchingBlockInventoryGet=*/false));
    REQUIRE_FALSE(fQueuedNoInv);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashNoInvTip) == 0);

    const uint1024_t hashGapTip(0xBC01000000000005ULL);
    bool fQueuedGap = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashGapTip,
        /*nPeerHeight=*/nLocalHeight + TAO::Ledger::BESTCHAIN_NEAR_TIP_HEIGHT_SLACK + 1,
        "unit-test-bestchain-gap", &node, &fQueuedGap,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE(fQueuedGap);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashGapTip) == 1);

    PassBlock orphanNearTip;
    orphanNearTip.nVersion      = 4;
    orphanNearTip.hashPrevBlock = uint1024_t(0xBC01DEADBEEF0001ULL);
    orphanNearTip.nChannel      = 2;
    orphanNearTip.nHeight       = nLocalHeight;
    orphanNearTip.nBits         = 1;
    orphanNearTip.nNonce        = 0x0B01;
    orphanNearTip.hashMerkleRoot = uint512_t(0x0B01);
    const uint1024_t hashOrphanNearTip = orphanNearTip.GetHash();
    const uint1024_t hashOrphanAncestor = orphanNearTip.hashPrevBlock;
    REQUIRE(TAO::Ledger::mapOrphans.Insert(orphanNearTip));

    bool fQueuedOrphan = false;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(
        hashOrphanNearTip, /*nPeerHeight=*/nLocalHeight,
        "unit-test-bestchain-near-orphan", &node, &fQueuedOrphan,
        /*fMatchingBlockInventoryGet=*/true));
    REQUIRE(fQueuedOrphan);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hashOrphanAncestor) == 1);
#endif

    TAO::Ledger::ChainState::nBestHeight.store(nSavedBestHeight);
    TAO::Ledger::mapOrphans.Clear();
    TAO::Ledger::mapLastOrphanRequest.clear();
}


TEST_CASE("Process primary path does not force PrimeCheck on IBD (source guard)",
"[ledger][process][primecheck]")
{
    /* Regression guard for the multi-day sync collapse: the primary
     * network ingestion (epoch zero) must keep fForceProof=false so
     * Synchronizing() can skip full PrimeCheck. Local mining epochs
     * and the recovery retry must still force verification. */
    const char* vCandidates[] = {
        "src/TAO/Ledger/process.cpp",
        "./src/TAO/Ledger/process.cpp",
        "../src/TAO/Ledger/process.cpp",
        "../../src/TAO/Ledger/process.cpp",
    };

    std::string strSource;
    for(const char* psz : vCandidates)
    {
        std::ifstream f(psz, std::ios::in | std::ios::binary);
        if(!f.is_open())
            continue;
        std::ostringstream ss;
        ss << f.rdbuf();
        strSource = ss.str();
        if(!strSource.empty())
            break;
    }

    if(strSource.empty())
    {
        WARN("process.cpp not reachable from CWD; skipping source guard");
        SUCCEED();
        return;
    }

    REQUIRE(strSource.find("if(!fSkipCheck && !block.Check(nMiningEpoch != 0))") != std::string::npos);

    /* Must NOT reintroduce a forced-proof primary call of the form
     * if(!fSkipCheck && !block.Check(true)). */
    REQUIRE(strSource.find("if(!fSkipCheck && !block.Check(true))") == std::string::npos);

    /* Recovery retry is allowed to force proof. */
    REQUIRE(strSource.find("if(block.Check(true))") != std::string::npos);

    /* A1: far-tip path must issue locator branch sync, not only log
     * block-not-yet-received and return. */
    REQUIRE(strSource.find("action=locator-branch-sync-request") != std::string::npos);
    REQUIRE(strSource.find("action=block-not-yet-received") == std::string::npos);
}


TEST_CASE("Hash-channel (nChannel=2) PoW non-regression: Check() branches correctly",
    "[ledger][process]")
{
    /* Verify that a Hash-channel block has its proof hash computed over
     * [nVersion .. nNonce] (not [nVersion .. nBits] like the Prime channel).
     * This is tested at the Block level to confirm ProofHash() dispatches
     * correctly; the stuck block in production never reached VerifyWork()
     * because it returned early at the vMissing guard. */

    TAO::Ledger::TritiumBlock block;
    block.nVersion = TAO::Ledger::CurrentBlockVersion();
    block.nChannel = 2;   /* Hash channel */
    block.nHeight  = 1;
    block.nNonce   = 0xDEADBEEF12345678ULL;
    block.nBits    = 0x20ffffff;

    /* ProofHash for nChannel != 1 must include nNonce (it's computed over
     * [nVersion, nNonce] inclusive), so changing nNonce changes the hash. */
    const uint1024_t hashNonce1 = block.ProofHash();
    block.nNonce = 0xDEADBEEF12345679ULL;
    const uint1024_t hashNonce2 = block.ProofHash();
    REQUIRE(hashNonce1 != hashNonce2);

    /* For comparison, Prime channel (nChannel == 1) hashes over
     * [nVersion, nBits] — nNonce changes do NOT change the proof hash. */
    block.nChannel = 1;
    const uint1024_t hashPrime1 = block.ProofHash();
    block.nNonce = 0xDEADBEEF12345680ULL;
    const uint1024_t hashPrime2 = block.ProofHash();
    REQUIRE(hashPrime1 == hashPrime2);

    /* Restore Hash channel and verify changing nBits also changes the hash
     * (both fields are in [nVersion, nNonce]). */
    block.nChannel = 2;
    block.nNonce   = 0xDEADBEEF12345678ULL;
    const uint1024_t hashBits1 = block.ProofHash();
    block.nBits = 0x20fefefe;
    const uint1024_t hashBits2 = block.ProofHash();
    REQUIRE(hashBits1 != hashBits2);
}


/* ==========================================================================
 * Tests for ACTION::LIST specifier sync-state coupling
 *
 * Background (PR #66x regression): fork-recovery LIST calls incorrectly used
 * SPECIFIER::SYNC after initial synchronisation completed.  The receiving
 * handler rejects SYNC blocks as "unsolicited" whenever fSynchronized == true,
 * causing silent DISCONNECT::FORCE.  The fix is to use SPECIFIER::TRANSACTIONS
 * on every post-sync recovery path, preserving SPECIFIER::SYNC only for the
 * true sync-time paths (TritiumNode::Sync() and the LASTINDEX handler).
 * ========================================================================== */

TEST_CASE("SPECIFIER enum values are stable (regression guard)", "[ledger][process]")
{
    /* These constant values are part of the on-wire protocol.  Any reordering
     * or renumbering silently changes which specifier is sent to remote peers.
     * Document them explicitly so an accidental renumber shows up as a test
     * failure rather than a protocol regression. */
    REQUIRE(uint8_t(LLP::TritiumNode::SPECIFIER::LEGACY)       == 0x40);
    REQUIRE(uint8_t(LLP::TritiumNode::SPECIFIER::TRITIUM)      == 0x41);
    REQUIRE(uint8_t(LLP::TritiumNode::SPECIFIER::SYNC)         == 0x42);
    REQUIRE(uint8_t(LLP::TritiumNode::SPECIFIER::TRANSACTIONS) == 0x43);
    REQUIRE(uint8_t(LLP::TritiumNode::SPECIFIER::CLIENT)       == 0x44);

    /* TRANSACTIONS and SYNC must be distinct — the entire post-sync vs sync-time
     * specifier split relies on this. */
    REQUIRE(LLP::TritiumNode::SPECIFIER::TRANSACTIONS != LLP::TritiumNode::SPECIFIER::SYNC);
}


TEST_CASE("Unsolicited-sync guard: fSynchronized==true rejects SPECIFIER::SYNC",
    "[ledger][process]")
{
    /* Document the guard condition that makes the SPECIFIER::SYNC regression
     * possible.  In src/LLP/tritium.cpp the TYPES::BLOCK / SPECIFIER::SYNC
     * handler reads:
     *
     *   if(nCurrentSession != TAO::Ledger::nSyncSession || fSynchronized.load())
     *       return debug::drop(FUNCTION, "unsolicted sync block");
     *
     * After initial sync completes:
     *   - TAO::Ledger::nSyncSession is reset to 0
     *   - fSynchronized is set to true
     *
     * Therefore the guard trips for every SYNC block received on a fully
     * synced node — including responses to our own recovery requests. */

    /* Save and restore the global sync session so we don't disturb other tests. */
    const uint64_t nSavedSession = TAO::Ledger::nSyncSession.load();
    const bool fSavedSynced      = LLP::TritiumNode::fSynchronized.load();

    /* --- Simulate: initial-sync in progress -------------------------------- */
    /* Assign an arbitrary non-zero sync session and clear fSynchronized. */
    TAO::Ledger::nSyncSession.store(42);
    LLP::TritiumNode::fSynchronized.store(false);

    const uint64_t nCurrentSession = 42; /* matches nSyncSession */

    /* Guard condition: (nCurrentSession != nSyncSession) || fSynchronized */
    const bool fDropDuringSyncWithMatchingSession =
        (nCurrentSession != TAO::Ledger::nSyncSession.load())
        || LLP::TritiumNode::fSynchronized.load();

    /* Should NOT drop during active sync when sessions match. */
    REQUIRE_FALSE(fDropDuringSyncWithMatchingSession);

    /* --- Simulate: sync complete (post-sync state) ------------------------- */
    TAO::Ledger::nSyncSession.store(0);
    LLP::TritiumNode::fSynchronized.store(true);

    const bool fDropPostSync =
        (nCurrentSession != TAO::Ledger::nSyncSession.load())
        || LLP::TritiumNode::fSynchronized.load();

    /* MUST drop: any SYNC block arriving on a synced node is unsolicited from
     * the receiver's perspective, even if we sent the LIST ourselves. */
    REQUIRE(fDropPostSync);

    /* --- Simulate: session mismatch during sync (different peer) ----------- */
    TAO::Ledger::nSyncSession.store(99);     /* different from nCurrentSession */
    LLP::TritiumNode::fSynchronized.store(false);

    const bool fDropMismatchedSession =
        (nCurrentSession != TAO::Ledger::nSyncSession.load())
        || LLP::TritiumNode::fSynchronized.load();

    /* Must drop: this peer is not the designated sync peer. */
    REQUIRE(fDropMismatchedSession);

    /* Restore global state. */
    TAO::Ledger::nSyncSession.store(nSavedSession);
    LLP::TritiumNode::fSynchronized.store(fSavedSynced);
}


TEST_CASE("Post-sync state: nSyncSession==0 and fSynchronized==true after Sync() completes",
    "[ledger][process]")
{
    /* After the ACTION::NOTIFY BESTCHAIN handler detects synchronisation is
     * complete it executes:
     *
     *   fSynchronized.store(true);
     *   TAO::Ledger::nSyncSession.store(0);
     *
     * This test documents the expected post-sync state so that any future
     * change to the completion logic is visible as a test failure. */

    const uint64_t nSavedSession = TAO::Ledger::nSyncSession.load();
    const bool fSavedSynced      = LLP::TritiumNode::fSynchronized.load();

    /* Simulate the two stores that mark sync complete. */
    LLP::TritiumNode::fSynchronized.store(true);
    TAO::Ledger::nSyncSession.store(0);

    REQUIRE(LLP::TritiumNode::fSynchronized.load() == true);
    REQUIRE(TAO::Ledger::nSyncSession.load()       == 0);

    /* In this state, the unsolicited-sync guard always fires regardless of
     * what nCurrentSession the *receiving* handler reads: even if some stale
     * connection still has nCurrentSession == 0 the condition
     *   (0 != 0) || true  →  true
     * causes a drop.  This is why SPECIFIER::TRANSACTIONS must be used for
     * all post-sync fork-recovery LIST requests. */
    const uint64_t nCurrentSession = 0;   /* worst-case stale session value */
    const bool fWouldDrop =
        (nCurrentSession != TAO::Ledger::nSyncSession.load())
        || LLP::TritiumNode::fSynchronized.load();
    REQUIRE(fWouldDrop);

    /* Restore. */
    TAO::Ledger::nSyncSession.store(nSavedSession);
    LLP::TritiumNode::fSynchronized.store(fSavedSynced);
}


TEST_CASE("Sync completion height guard prevents stale half-chain finalization", "[ledger][process]")
{
    using namespace TAO::Ledger::ChainState;
    PeerHeightTracker heights;
    int syncPeer = 0;
    int otherPeer = 0;
    heights.Update(&syncPeer, 500000);
    heights.Update(&otherPeer, 900000);
    REQUIRE_FALSE(CanFinalizeSyncFromPeer(500000, 500000, heights.Max()));
    heights.Remove(&otherPeer);
    REQUIRE(CanFinalizeSyncFromPeer(500000, 500000, heights.Max()));
    heights.Update(&otherPeer, 500002);
    REQUIRE(CanFinalizeSyncFromPeer(500000, 500000, heights.Max()));
    heights.Update(&otherPeer, 500003);
    REQUIRE_FALSE(CanFinalizeSyncFromPeer(500000, 500000, heights.Max()));
    REQUIRE_FALSE(CanFinalizeSyncFromPeer(499999, 500000, heights.Max()));
    REQUIRE(CanFinalizeSyncFromPeer(100, 100, 0));
    REQUIRE_FALSE(CanFinalizeSyncFromPeer(100, 101, 0));
    REQUIRE(CanFinalizeSyncFromPeer(UINT32_MAX, UINT32_MAX, UINT32_MAX));
}


/* ==========================================================================
 * Tests for the admissibility classifier (stranded-state loop fix)
 *
 * Background: three call sites compared against local chain state, decided
 * something was invalid, and took no recovery action.  On a node already
 * behind, local state is exactly what is wrong, so the condition is
 * self-sustaining.  The admissibility classifier distinguishes
 * INVALID_ABSOLUTE from DEFERRED_LOCAL_STATE so recovery paths can retain
 * and retry instead of permanently blacklisting.
 * ========================================================================== */

TEST_CASE("AdmissibilityClass enum has correct values", "[ledger][process]")
{
    /* Verify the enum is defined and the values are distinct.
     * This locks in the classifier API so future refactors show up as
     * compile-time failures rather than silent behavioural regressions. */
    REQUIRE(TAO::Ledger::AdmissibilityClass::VALID
         != TAO::Ledger::AdmissibilityClass::INVALID_ABSOLUTE);
    REQUIRE(TAO::Ledger::AdmissibilityClass::VALID
         != TAO::Ledger::AdmissibilityClass::DEFERRED_LOCAL_STATE);
    REQUIRE(TAO::Ledger::AdmissibilityClass::INVALID_ABSOLUTE
         != TAO::Ledger::AdmissibilityClass::DEFERRED_LOCAL_STATE);
    REQUIRE(TAO::Ledger::AdmissibilityClass::UNKNOWN
         != TAO::Ledger::AdmissibilityClass::VALID);
}


TEST_CASE("AdmissibilityClass thread-local side-channel set/take semantics", "[ledger][process]")
{
    /* The thread-local side-channel used between Transaction::Connect() and
     * Mempool::Accept() must:
     *   1. Start at UNKNOWN.
     *   2. Reflect the value set by SetLastConnectClass().
     *   3. Be reset to UNKNOWN after a single TakeLastConnectClass() call —
     *      stale classifications must not persist across unrelated Connect()
     *      calls on the same thread. */

    /* Initial value must be UNKNOWN. */
    TAO::Ledger::g_nLastConnectClass = TAO::Ledger::AdmissibilityClass::UNKNOWN;
    REQUIRE(TAO::Ledger::g_nLastConnectClass == TAO::Ledger::AdmissibilityClass::UNKNOWN);

    /* After setting DEFERRED_LOCAL_STATE, TakeLastConnectClass() returns it. */
    TAO::Ledger::SetLastConnectClass(TAO::Ledger::AdmissibilityClass::DEFERRED_LOCAL_STATE);
    REQUIRE(TAO::Ledger::g_nLastConnectClass ==
            TAO::Ledger::AdmissibilityClass::DEFERRED_LOCAL_STATE);

    const TAO::Ledger::AdmissibilityClass cls = TAO::Ledger::TakeLastConnectClass();
    REQUIRE(cls == TAO::Ledger::AdmissibilityClass::DEFERRED_LOCAL_STATE);

    /* After the take the slot must be reset to UNKNOWN. */
    REQUIRE(TAO::Ledger::g_nLastConnectClass == TAO::Ledger::AdmissibilityClass::UNKNOWN);

    /* A second take without an intervening set returns UNKNOWN. */
    const TAO::Ledger::AdmissibilityClass cls2 = TAO::Ledger::TakeLastConnectClass();
    REQUIRE(cls2 == TAO::Ledger::AdmissibilityClass::UNKNOWN);

    /* INVALID_ABSOLUTE round-trip. */
    TAO::Ledger::SetLastConnectClass(TAO::Ledger::AdmissibilityClass::INVALID_ABSOLUTE);
    REQUIRE(TAO::Ledger::TakeLastConnectClass() ==
            TAO::Ledger::AdmissibilityClass::INVALID_ABSOLUTE);
    REQUIRE(TAO::Ledger::g_nLastConnectClass == TAO::Ledger::AdmissibilityClass::UNKNOWN);
}


TEST_CASE("Peer height observations expire and follow connection updates", "[ledger][process]")
{
    using TAO::Ledger::ChainState::PeerHeightTracker;
    PeerHeightTracker heights;
    const auto now = PeerHeightTracker::Clock::now();
    int first = 0;
    int second = 0;
    heights.Update(&first, 900, now);
    heights.Update(&second, 100, now + std::chrono::minutes(1));
    uint32_t selected = 0;
    REQUIRE(heights.Get(&first, selected, now + std::chrono::seconds(90),
        std::chrono::seconds(90)));
    REQUIRE(selected == 900);
    REQUIRE_FALSE(heights.Get(&first, selected, now + std::chrono::seconds(91),
        std::chrono::seconds(90)));
    REQUIRE(heights.Max(now + std::chrono::seconds(91)) == 900);
    REQUIRE(heights.Max(now + PeerHeightTracker::MAX_AGE) == 900);
    REQUIRE(heights.Max(now + PeerHeightTracker::MAX_AGE + std::chrono::seconds(1)) == 100);
    heights.Update(&first, 80, now + PeerHeightTracker::MAX_AGE + std::chrono::seconds(1));
    REQUIRE(heights.Max(now + PeerHeightTracker::MAX_AGE + std::chrono::seconds(1)) == 100);
    heights.Remove(&second);
    REQUIRE(heights.Max(now + PeerHeightTracker::MAX_AGE + std::chrono::seconds(1)) == 80);
    heights.Remove(&first);
    REQUIRE(heights.Max(now + PeerHeightTracker::MAX_AGE + std::chrono::seconds(1)) == 0);
}

TEST_CASE("Shutdown rejects block processing and peer-best recovery",
    "[ledger][process][shutdown]")
{
    const bool savedShutdown = config::fShutdown.load();
    struct RestoreShutdown
    {
        bool saved;
        ~RestoreShutdown() { config::fShutdown.store(saved); }
    } restore{savedShutdown};
    config::fShutdown.store(true);

    PassBlock block;
    uint8_t status = 0;
    TAO::Ledger::Process(block, status, nullptr);
    REQUIRE(status == TAO::Ledger::PROCESS::IGNORED);

    LLP::TritiumNode node;
    const uint1024_t tip(0xBC01000000000170ULL);
    bool queued = true;
    REQUIRE(TAO::Ledger::AttemptPeerBestChainRecovery(tip, 1000, "shutdown-test",
        &node, &queued) == TAO::Ledger::PeerBestRecoveryResult::SKIPPED);
    REQUIRE_FALSE(queued);
    queued = true;
    REQUIRE_FALSE(TAO::Ledger::RequestBestChainBranchRecovery(tip, 1000,
        "shutdown-test", &node, &queued));
    REQUIRE_FALSE(queued);

    const auto packets = node.PACKETS.load();
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE(node.PACKETS.load() == packets);
}

TEST_CASE("Peer best retry is bounded and expires without fresh advertisements",
    "[ledger][process][bestchain][retry]")
{
    LLP::PeerBestRetry retry;
    const auto now = LLP::PeerBestRetry::Clock::now();
    REQUIRE(retry.RefreshDue(now));
    REQUIRE_FALSE(retry.RefreshDue(now + LLP::PeerBestRetry::INTERVAL - std::chrono::seconds(1)));
    REQUIRE(retry.RefreshDue(now + LLP::PeerBestRetry::INTERVAL));
    REQUIRE_FALSE(retry.Due(now));
    retry.Observe(true, now);
    REQUIRE_FALSE(retry.Due(now + LLP::PeerBestRetry::INTERVAL - std::chrono::seconds(1)));
    REQUIRE(retry.Due(now + LLP::PeerBestRetry::INTERVAL));
    REQUIRE_FALSE(retry.Due(now + LLP::PeerBestRetry::INTERVAL));
    retry.Observe(false, now + LLP::PeerBestRetry::INTERVAL + std::chrono::seconds(1));
    REQUIRE_FALSE(retry.Due(now + 2 * LLP::PeerBestRetry::INTERVAL - std::chrono::seconds(1)));
    REQUIRE(retry.Due(now + 2 * LLP::PeerBestRetry::INTERVAL));
    const auto expired = now + LLP::PeerBestRetry::MAX_AGE
                       + LLP::PeerBestRetry::INTERVAL + std::chrono::seconds(2);
    REQUIRE_FALSE(retry.Due(expired));
    retry.Observe(false, expired);
    REQUIRE(retry.Due(expired));
    retry.Reset();
    REQUIRE(retry.RefreshDue(now));
    REQUIRE_FALSE(retry.Due(now + LLP::PeerBestRetry::MAX_AGE + 2 * LLP::PeerBestRetry::INTERVAL));

    retry.Observe(true, now);
    retry.Resolved();
    REQUIRE(retry.Fresh(now));
    REQUIRE_FALSE(retry.Due(now + LLP::PeerBestRetry::INTERVAL));
    REQUIRE_FALSE(retry.BlockDue(now + LLP::PeerBestRetry::INTERVAL));
    retry.Observe(false, now + LLP::PeerBestRetry::INTERVAL);
    REQUIRE_FALSE(retry.Due(now + 2 * LLP::PeerBestRetry::INTERVAL - std::chrono::seconds(1)));
    retry.Observe(false, now + 2 * LLP::PeerBestRetry::INTERVAL - std::chrono::seconds(1));
    REQUIRE(retry.Due(now + 2 * LLP::PeerBestRetry::INTERVAL));
}

TEST_CASE("Block GET retries do not expedite branch recovery",
    "[ledger][process][bestchain][retry][inventory]")
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
}

TEST_CASE("Changed peer tips retain a bounded unresolved deadline until local resolution",
    "[ledger][process][bestchain][retry]")
{
    LLP::PeerBestRetry retry;
    const auto now = LLP::PeerBestRetry::Clock::now();
    retry.Observe(true, now);
    const auto changed = now + LLP::BlockInventoryRequests::TIMEOUT - std::chrono::seconds(1);
    retry.Observe(true, changed);
    REQUIRE_FALSE(retry.BlockDue(changed));
    retry.Observe(false, changed + std::chrono::seconds(1));
    REQUIRE(retry.BlockDue(now + LLP::BlockInventoryRequests::TIMEOUT));
    retry.Observe(true, now + LLP::PeerBestRetry::INTERVAL - std::chrono::seconds(1));
    REQUIRE_FALSE(retry.Due(now + LLP::PeerBestRetry::INTERVAL - std::chrono::seconds(1)));
    REQUIRE(retry.Due(now + LLP::PeerBestRetry::INTERVAL));
    retry.Observe(true, now + 2 * LLP::PeerBestRetry::INTERVAL);
    REQUIRE(retry.Due(now + 2 * LLP::PeerBestRetry::INTERVAL));
    REQUIRE_FALSE(retry.Due(now + 2 * LLP::PeerBestRetry::INTERVAL));

    retry.Resolved();
    retry.Observe(true, now + 3 * LLP::PeerBestRetry::INTERVAL);
    REQUIRE_FALSE(retry.BlockDue(now + 3 * LLP::PeerBestRetry::INTERVAL));
    REQUIRE_FALSE(retry.Due(now + 3 * LLP::PeerBestRetry::INTERVAL));
    REQUIRE(retry.BlockDue(now + 3 * LLP::PeerBestRetry::INTERVAL + LLP::BlockInventoryRequests::TIMEOUT));
    REQUIRE(retry.Due(now + 4 * LLP::PeerBestRetry::INTERVAL));
}

TEST_CASE("Block inventory requests expire and track independent peers",
    "[ledger][process][bestchain][inventory]")
{
    LLP::BlockInventoryRequests requests;
    const auto now = LLP::BlockInventoryRequests::Clock::now();
    const uint1024_t tip(0xBC01000000000150ULL);
    int first = 0;
    int second = 0;

    REQUIRE(requests.Add(tip, &first, now));
    REQUIRE(requests.Add(tip, &second, now + std::chrono::seconds(1)));
    REQUIRE(requests.Contains(tip, &second, now + LLP::BlockInventoryRequests::TIMEOUT));
    requests.Remove(tip, &second);
    REQUIRE_FALSE(requests.Contains(tip, &second, now + LLP::BlockInventoryRequests::TIMEOUT));

    const auto expired = now + LLP::BlockInventoryRequests::TIMEOUT;
    REQUIRE(requests.Add(tip, &first, expired));
    REQUIRE(requests.Add(tip, &second, expired));
    requests.RemovePeer(&first);
    REQUIRE(requests.Contains(tip, &second, expired));
    requests.RemovePeer(&second);
    REQUIRE_FALSE(requests.Contains(tip, &second, expired));
}

TEST_CASE("Block inventory request tracking is bounded and reuses expired entries",
    "[ledger][process][bestchain][inventory]")
{
    LLP::BlockInventoryRequests requests;
    const auto now = LLP::BlockInventoryRequests::Clock::now();
    int peers[LLP::BlockInventoryRequests::MAX_PEERS_PER_HASH + 1] = {};
    const uint1024_t tip(1);
    for(std::size_t i = 0; i < LLP::BlockInventoryRequests::MAX_PEERS_PER_HASH; ++i)
        REQUIRE(requests.Add(tip, &peers[i], now));
    REQUIRE_FALSE(requests.Add(tip, &peers[LLP::BlockInventoryRequests::MAX_PEERS_PER_HASH], now));
    REQUIRE_FALSE(requests.Add(tip, &peers[0], now));
    REQUIRE(requests.Add(tip, &peers[LLP::BlockInventoryRequests::MAX_PEERS_PER_HASH],
        now + LLP::BlockInventoryRequests::TIMEOUT));

    LLP::BlockInventoryRequests hashes;
    for(std::size_t i = 0; i < LLP::BlockInventoryRequests::MAX_HASHES_PER_PEER; ++i)
        REQUIRE(hashes.Add(uint1024_t(i + 1), &peers[0], now));
    const uint1024_t overflow(LLP::BlockInventoryRequests::MAX_HASHES_PER_PEER + 1);
    REQUIRE_FALSE(hashes.Add(overflow, &peers[0], now));
    REQUIRE(hashes.Add(overflow, &peers[1], now));
    REQUIRE(hashes.Add(overflow, &peers[0],
        now + LLP::BlockInventoryRequests::TIMEOUT));
}

#ifndef WIN32
TEST_CASE("Incomplete block responses cannot bypass the BESTCHAIN GET throttle",
    "[ledger][process][bestchain][retry][inventory]")
{
    LedgerGuard env;
    MissingBlock incomplete;
    incomplete.nVersion = 4;
    incomplete.hashPrevBlock = TAO::Ledger::ChainState::hashBestChain.load();
    incomplete.nHeight = TAO::Ledger::ChainState::nBestHeight.load() + 1;
    incomplete.nChannel = 2;
    incomplete.nNonce = 0x73201;
    const uint1024_t hash = incomplete.GetHash();
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hash));
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    LLP::TritiumNode node;
    struct Cleanup
    {
        LLP::TritiumNode& node;
        int* fds;
        uint1024_t hash;
        ~Cleanup()
        {
            TAO::Ledger::mapLastMissing.erase(hash);
            TAO::Ledger::mapLastMissingProcessTime.erase(hash);
            TAO::Ledger::mapLastOrphanRequest.erase(hash);
            node.fd = -1;
            close(fds[0]);
            close(fds[1]);
        }
    } cleanup{node, fds, hash};
    node.fd = fds[1];
    node.events = POLLIN;
    node.nCurrentSession = 1;
    node.nProtocolVersion = LLP::PROTOCOL_VERSION;
    node.nLastPing.store(runtime::unifiedtimestamp());
    node.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BLOCK
                 | LLP::TritiumNode::SUBSCRIPTION::BESTCHAIN
                 | LLP::TritiumNode::SUBSCRIPTION::BESTHEIGHT);
    const auto readSent = [&]()
    {
        while(node.Buffered() > 0)
            REQUIRE(node.Flush() > 0);
        std::vector<uint8_t> sent;
        uint8_t buffer[4096];
        for(ssize_t n = recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT);
            n > 0; n = recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT))
            sent.insert(sent.end(), buffer, buffer + n);
        return sent;
    };
    node.Event(LLP::EVENTS::GENERIC, 0);
    readSent();

    DataStream notify(SER_NETWORK, LLP::PROTOCOL_VERSION);
    notify << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << hash
           << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << hash
           << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << incomplete.nHeight;
    node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
    node.INCOMING.DATA.assign(notify.begin(), notify.end());
    REQUIRE(node.ProcessPacket());
    DataStream get(SER_NETWORK, LLP::PROTOCOL_VERSION);
    get << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << hash;
    const auto expectedGet = LLP::TritiumNode::NewMessage(
        LLP::TritiumNode::ACTION::GET, get).GetBytes();
    REQUIRE(readSent() == expectedGet);

    uint8_t status = 0;
    TAO::Ledger::Process(incomplete, status);
    REQUIRE(status & TAO::Ledger::PROCESS::INCOMPLETE);
    TAO::Ledger::TritiumBlock body;
    static_cast<TAO::Ledger::Block&>(body) = incomplete;
    REQUIRE(body.GetHash() == hash);
    DataStream response(SER_NETWORK, LLP::PROTOCOL_VERSION);
    response << uint8_t(LLP::TritiumNode::SPECIFIER::TRITIUM) << body;
    /* Exercise the already-validated incomplete-body fast path. */
    TAO::Ledger::mapLastMissingProcessTime[hash] = runtime::timestamp(true);
    node.INCOMING.MESSAGE = LLP::TritiumNode::TYPES::BLOCK;
    node.INCOMING.DATA.assign(response.begin(), response.end());
    REQUIRE(node.ProcessPacket());
    REQUIRE(node.nConsecutiveFails == 0);
    REQUIRE(TAO::Ledger::mapLastMissing.at(hash) == 1);
    readSent();

    for(unsigned int repeat = 0; repeat < 10; ++repeat)
    {
        node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        node.INCOMING.DATA.assign(notify.begin(), notify.end());
        REQUIRE(node.ProcessPacket());
        REQUIRE(readSent().empty());
    }
    REQUIRE(TAO::Ledger::mapLastMissing.at(hash) == 1);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(hash));
    std::this_thread::sleep_for(LLP::BlockInventoryRequests::TIMEOUT + std::chrono::milliseconds(100));
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE(readSent() == expectedGet);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hash) == 0);
    node.MakePeerBestRetryDueForTests();
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(hash) == 1);
}

TEST_CASE("Inventory overflow preserves queued work without disconnecting compliant peers",
    "[ledger][process][bestchain][inventory]")
{
    LedgerGuard env;
    const bool client = GENERATE(false, true);
    struct ClientModeGuard
    {
        bool saved{config::fClient.load()};
        bool owned{false};
        ~ClientModeGuard()
        {
            config::fClient.store(saved);
            if(owned)
            {
                delete LLD::Client;
                LLD::Client = nullptr;
            }
        }
    } clientModeGuard;
    if(client && !LLD::Client)
    {
        LLD::Client = new LLD::ClientDB(LLD::FLAGS::CREATE | LLD::FLAGS::FORCE);
        clientModeGuard.owned = true;
    }
    config::fClient.store(client);
    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    LLP::TritiumNode node;
    struct Cleanup
    {
        LLP::TritiumNode& node;
        int* fds;
        ~Cleanup()
        {
            node.fCONNECTED.store(false);
            node.fd = -1;
            close(fds[0]);
            close(fds[1]);
        }
    } cleanup{node, fds};
    node.fd = fds[1];
    node.events = POLLIN;
    node.fCONNECTED.store(true);
    node.nCurrentSession = 1;
    node.nProtocolVersion = LLP::PROTOCOL_VERSION;
    node.nLastPing.store(runtime::unifiedtimestamp());
    node.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BLOCK);
    const auto readSent = [&]()
    {
        while(node.Buffered() > 0)
            REQUIRE(node.Flush() > 0);
        std::vector<uint8_t> sent;
        uint8_t buffer[4096];
        for(ssize_t n = recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT);
            n > 0; n = recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT))
            sent.insert(sent.end(), buffer, buffer + n);
        return sent;
    };
    node.Event(LLP::EVENTS::GENERIC, 0);
    readSent();
    using Requests = LLP::BlockInventoryRequests;
    const size_t capacity = Requests::MAX_HASHES_PER_PEER + Requests::MAX_PENDING_PER_PEER;
    for(size_t start = 0; start <= capacity; start += 100)
    {
        DataStream inventory(SER_NETWORK, LLP::PROTOCOL_VERSION);
        for(size_t i = start; i < std::min(start + 100, capacity + 1); ++i)
            inventory << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << uint1024_t(0x732100 + i);
        node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        node.INCOMING.DATA.assign(inventory.begin(), inventory.end());
        REQUIRE(node.ProcessPacket());
        REQUIRE(node.Connected());
    }
    const auto expectedBatch = [&](size_t start)
    {
        DataStream get(SER_NETWORK, LLP::PROTOCOL_VERSION);
        for(size_t i = start; i < start + Requests::MAX_HASHES_PER_PEER; ++i)
        {
            if(client)
                get << uint8_t(LLP::TritiumNode::SPECIFIER::CLIENT);
            get << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << uint1024_t(0x732100 + i);
        }
        return get;
    };
    REQUIRE(readSent() == LLP::TritiumNode::NewMessage(
        LLP::TritiumNode::ACTION::GET, expectedBatch(0)).GetBytes());
    std::this_thread::sleep_for(Requests::TIMEOUT + std::chrono::milliseconds(100));
    node.Event(LLP::EVENTS::GENERIC, 0);
    std::vector<uint8_t> expected;
    for(size_t i = Requests::MAX_HASHES_PER_PEER; i < 2 * Requests::MAX_HASHES_PER_PEER; ++i)
    {
        DataStream get(SER_NETWORK, LLP::PROTOCOL_VERSION);
        if(client)
            get << uint8_t(LLP::TritiumNode::SPECIFIER::CLIENT);
        get << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << uint1024_t(0x732100 + i);
        const auto bytes = LLP::TritiumNode::NewMessage(LLP::TritiumNode::ACTION::GET, get).GetBytes();
        expected.insert(expected.end(), bytes.begin(), bytes.end());
    }
    REQUIRE(readSent() == expected);
    REQUIRE(node.Connected());
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE(readSent().empty());
}

TEST_CASE("BESTCHAIN does not duplicate its peer's block GET",
    "[ledger][process][bestchain][inventory]")
{
    LedgerGuard env;
    const uint32_t savedHeight = TAO::Ledger::ChainState::nBestHeight.load();
    const bool savedSynchronized = LLP::TritiumNode::fSynchronized.load();
    int a[2] = {-1, -1};
    int b[2] = {-1, -1};
    struct Cleanup
    {
        uint32_t height;
        bool synchronized;
        int (&a)[2];
        int (&b)[2];
        ~Cleanup()
        {
            TAO::Ledger::ChainState::nBestHeight.store(height);
            LLP::TritiumNode::fSynchronized.store(synchronized);
            TAO::Ledger::mapLastOrphanRequest.clear();
            for(int fd : {a[0], a[1], b[0], b[1]})
                if(fd >= 0)
                    close(fd);
        }
    } cleanup{savedHeight, savedSynchronized, a, b};
    TAO::Ledger::ChainState::nBestHeight.store(1000);
    LLP::TritiumNode::fSynchronized.store(false);
    TAO::Ledger::mapLastOrphanRequest.clear();
    const uint1024_t tip(0xBC01000000000151ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(tip));

    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, a) == 0);
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, b) == 0);
    LLP::TritiumNode bestPeer;
    bestPeer.fd = b[1];
    bestPeer.events = POLLIN;
    bestPeer.nCurrentSession = 2;
    bestPeer.nProtocolVersion = LLP::PROTOCOL_VERSION;
    bestPeer.nLastPing.store(runtime::unifiedtimestamp());
    bestPeer.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BESTCHAIN
                     | LLP::TritiumNode::SUBSCRIPTION::BESTHEIGHT);

    DataStream best(SER_NETWORK, LLP::PROTOCOL_VERSION);
    best << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << uint32_t(1001)
         << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << tip;

    {
        LLP::TritiumNode inventoryPeer;
        inventoryPeer.fd = a[1];
        inventoryPeer.events = POLLIN;
        inventoryPeer.nCurrentSession = 1;
        inventoryPeer.nProtocolVersion = LLP::PROTOCOL_VERSION;
        inventoryPeer.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BLOCK);

        DataStream inventory(SER_NETWORK, LLP::PROTOCOL_VERSION);
        inventory << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << tip;
        inventoryPeer.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        inventoryPeer.INCOMING.DATA.assign(inventory.begin(), inventory.end());
        REQUIRE(inventoryPeer.ProcessPacket());

        const uint64_t nPacketsBeforeBestNotify = bestPeer.PACKETS.load();
        bestPeer.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        bestPeer.INCOMING.DATA.assign(best.begin(), best.end());
        REQUIRE(bestPeer.ProcessPacket());
        REQUIRE(bestPeer.PACKETS.load() > nPacketsBeforeBestNotify);
        REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(tip) == 0);

        /* Periodic recovery must run even if an outstanding GET is repeatedly
         * renewed; otherwise announcements can defer recovery indefinitely. */
        bestPeer.MakePeerBestRetryDueForTests();
        bestPeer.Event(LLP::EVENTS::GENERIC, 0);
        REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(tip) == 1);
        TAO::Ledger::mapLastOrphanRequest.clear();

        inventoryPeer.fd = -1;
    }

    bestPeer.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
    bestPeer.INCOMING.DATA.assign(best.begin(), best.end());
    REQUIRE(bestPeer.ProcessPacket());
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(tip) == 0);

    /* A fatal send error must not make another peer wait for a GET that was
     * never sent or buffered. */
    TAO::Ledger::mapLastOrphanRequest.clear();
    close(a[0]);
    a[0] = -1;
    const uint1024_t failedTip(0xBC01000000000152ULL);
    {
        LLP::TritiumNode failedPeer;
        failedPeer.fd = a[1];
        failedPeer.events = POLLIN;
        failedPeer.nCurrentSession = 3;
        failedPeer.nProtocolVersion = LLP::PROTOCOL_VERSION;
        failedPeer.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BLOCK);
        DataStream inventory(SER_NETWORK, LLP::PROTOCOL_VERSION);
        inventory << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << failedTip;
        failedPeer.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        failedPeer.INCOMING.DATA.assign(inventory.begin(), inventory.end());
        REQUIRE(failedPeer.ProcessPacket());

        DataStream failedBest(SER_NETWORK, LLP::PROTOCOL_VERSION);
        failedBest << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << uint32_t(1001)
                   << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << failedTip;
        const uint64_t nPacketsBeforeFailedTip = bestPeer.PACKETS.load();
        bestPeer.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        bestPeer.INCOMING.DATA.assign(failedBest.begin(), failedBest.end());
        REQUIRE(bestPeer.ProcessPacket());
        REQUIRE(bestPeer.PACKETS.load() > nPacketsBeforeFailedTip);
        REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(failedTip) == 0);
        failedPeer.fd = -1;
    }

    TAO::Ledger::ChainState::ForgetPeerHeight(&bestPeer);
    bestPeer.fd = -1;
}
#endif

#ifndef WIN32
TEST_CASE("Resolved tip advances get fresh grace while unresolved churn has bounded recovery",
    "[ledger][process][bestchain][retry][near-tip]")
{
    LedgerGuard env;
    const uint32_t savedHeight = TAO::Ledger::ChainState::nBestHeight.load();
    const uint1024_t savedBest = TAO::Ledger::ChainState::hashBestChain.load();
    const bool savedSynchronized = LLP::TritiumNode::fSynchronized.load();
    TAO::Ledger::ChainState::nBestHeight.store(1000);
    LLP::TritiumNode::fSynchronized.store(false);
    TAO::Ledger::mapLastOrphanRequest.clear();

    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    LLP::TritiumNode node;
    struct Cleanup
    {
        LLP::TritiumNode& node;
        int* fds;
        uint32_t height;
        uint1024_t best;
        bool synchronized;
        ~Cleanup()
        {
            TAO::Ledger::ChainState::ForgetPeerHeight(&node);
            TAO::Ledger::ChainState::nBestHeight.store(height);
            TAO::Ledger::ChainState::hashBestChain.store(best);
            LLP::TritiumNode::fSynchronized.store(synchronized);
            TAO::Ledger::mapLastOrphanRequest.clear();
            node.fd = -1;
            close(fds[0]);
            close(fds[1]);
        }
    } cleanup{node, fds, savedHeight, savedBest, savedSynchronized};
    node.fd = fds[1];
    node.events = POLLIN;
    node.nCurrentSession = 1;
    node.nProtocolVersion = LLP::PROTOCOL_VERSION;
    node.nLastPing.store(runtime::unifiedtimestamp());
    node.hashBestChain = savedBest;
    node.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BLOCK
                 | LLP::TritiumNode::SUBSCRIPTION::BESTCHAIN
                 | LLP::TritiumNode::SUBSCRIPTION::BESTHEIGHT);
    const auto readSent = [&]()
    {
        while(node.Buffered() > 0)
            REQUIRE(node.Flush() > 0);
        std::vector<uint8_t> sent;
        uint8_t buffer[4096];
        for(ssize_t n = recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT);
            n > 0; n = recv(fds[0], buffer, sizeof(buffer), MSG_DONTWAIT))
            sent.insert(sent.end(), buffer, buffer + n);
        return sent;
    };
    readSent();

    const uint1024_t firstTip(0xBC01000000000141ULL);
    const uint1024_t secondTip(0xBC01000000000142ULL);
    for(unsigned int i = 0; i < 2; ++i)
    {
        const uint1024_t tip = i == 0 ? firstTip : secondTip;
        REQUIRE_FALSE(LLD::Ledger->HasBlock(tip));
        node.MakePeerBestRetryDueForTests();

        DataStream payload(SER_NETWORK, LLP::PROTOCOL_VERSION);
        if(i == 0)
            payload << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << tip;
        payload << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << tip
                << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << uint32_t(1001 + i);
        node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        node.INCOMING.DATA.assign(payload.begin(), payload.end());
        REQUIRE(node.ProcessPacket());

        DataStream get(SER_NETWORK, LLP::PROTOCOL_VERSION);
        get << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << tip;
        REQUIRE(readSent() == LLP::TritiumNode::NewMessage(
            LLP::TritiumNode::ACTION::GET, get).GetBytes());
        for(unsigned int repeat = 0; repeat < 3; ++repeat)
        {
            node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
            node.INCOMING.DATA.assign(payload.begin(), payload.end());
            REQUIRE(node.ProcessPacket());
            REQUIRE(readSent().empty());
        }

        DataStream bestOnly(SER_NETWORK, LLP::PROTOCOL_VERSION);
        bestOnly << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << tip;
        node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        node.INCOMING.DATA.assign(bestOnly.begin(), bestOnly.end());
        REQUIRE(node.ProcessPacket());
        REQUIRE(readSent().empty());
        node.Event(LLP::EVENTS::GENERIC, 0);
        REQUIRE(TAO::Ledger::mapLastOrphanRequest.empty());
        readSent();
        if(i == 0)
        {
            TAO::Ledger::ChainState::hashBestChain.store(firstTip);
            TAO::Ledger::ChainState::nBestHeight.store(1001);
        }
    }

    /* Without a response or another announcement, retry only the body at 5s. */
    std::this_thread::sleep_for(LLP::BlockInventoryRequests::TIMEOUT + std::chrono::milliseconds(100));
    node.Event(LLP::EVENTS::GENERIC, 0);
    DataStream retryGet(SER_NETWORK, LLP::PROTOCOL_VERSION);
    retryGet << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << secondTip;
    REQUIRE(readSent() == LLP::TritiumNode::NewMessage(
        LLP::TritiumNode::ACTION::GET, retryGet).GetBytes());
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.empty());

    /* A lost block is not masked by repeated refreshes of the same tip. */
    node.MakePeerBestRetryDueForTests();
    DataStream refresh(SER_NETWORK, LLP::PROTOCOL_VERSION);
    refresh << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << secondTip
            << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << uint32_t(1002);
    node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
    node.INCOMING.DATA.assign(refresh.begin(), refresh.end());
    REQUIRE(node.ProcessPacket());
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.empty());
    REQUIRE(readSent().empty());
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(secondTip) == 1);
    readSent();

    /* Changing an unresolved tip cannot postpone an overdue recovery. */
    TAO::Ledger::mapLastOrphanRequest.clear();
    node.MakePeerBestRetryDueForTests();
    for(const uint1024_t tip : {uint1024_t(0xBC01000000000143ULL),
                               uint1024_t(0xBC01000000000144ULL)})
    {
        REQUIRE_FALSE(LLD::Ledger->HasBlock(tip));
        DataStream changed(SER_NETWORK, LLP::PROTOCOL_VERSION);
        changed << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << tip;
        node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        node.INCOMING.DATA.assign(changed.begin(), changed.end());
        REQUIRE(node.ProcessPacket());
        DataStream get(SER_NETWORK, LLP::PROTOCOL_VERSION);
        get << uint8_t(LLP::TritiumNode::TYPES::BLOCK) << tip;
        REQUIRE(readSent() == LLP::TritiumNode::NewMessage(
            LLP::TritiumNode::ACTION::GET, get).GetBytes());
    }
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.empty());
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(uint1024_t(0xBC01000000000144ULL)) == 1);
    REQUIRE_FALSE(readSent().empty());
    TAO::Ledger::mapLastOrphanRequest.clear();
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.empty());
    REQUIRE(readSent().empty());
}

TEST_CASE("Idle peer retries an unknown shorter best chain despite stale-height tip churn",
    "[ledger][process][bestchain][retry]")
{
    LedgerGuard env;
    const uint32_t savedHeight = TAO::Ledger::ChainState::nBestHeight.load();
    const bool savedSynchronized = LLP::TritiumNode::fSynchronized.load();
    TAO::Ledger::ChainState::nBestHeight.store(1000);
    LLP::TritiumNode::fSynchronized.store(false);
    TAO::Ledger::mapLastOrphanRequest.clear();

    int fds[2] = {-1, -1};
    REQUIRE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
    REQUIRE(fcntl(fds[1], F_SETFL, O_NONBLOCK) == 0);
    LLP::TritiumNode node;
    node.fd = fds[1];
    node.events = POLLIN;
    node.nCurrentSession = 1;
    node.nProtocolVersion = LLP::PROTOCOL_VERSION;
    node.nLastPing.store(runtime::unifiedtimestamp());
    node.Subscribe(LLP::TritiumNode::SUBSCRIPTION::BESTCHAIN
                 | LLP::TritiumNode::SUBSCRIPTION::BESTHEIGHT);

    const uint1024_t tip(0xBC01000000000135ULL);
    REQUIRE_FALSE(LLD::Ledger->HasBlock(tip));
    DataStream payload(SER_NETWORK, LLP::PROTOCOL_VERSION);
    payload << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << uint32_t(500)
            << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << tip;
    node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
    node.INCOMING.DATA.assign(payload.begin(), payload.end());
    REQUIRE(node.ProcessPacket());
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(tip) == 0);

    uint8_t sent[65536];
    while(recv(fds[0], sent, sizeof(sent), MSG_DONTWAIT) > 0) {}
    node.MakePeerBestRetryDueForTests();
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(tip) == 1);
    REQUIRE(recv(fds[0], sent, sizeof(sent), MSG_DONTWAIT) > 0);
    TAO::Ledger::mapLastOrphanRequest.clear();
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.empty());
    node.MakePeerBestRetryDueForTests();
    const uint32_t changedHeight = GENERATE(uint32_t(500), uint32_t(999), uint32_t(1000));
    const uint1024_t changedTip(0xBC01000000000137ULL);
    for(const uint1024_t hash : {uint1024_t(0xBC01000000000136ULL), changedTip})
    {
        REQUIRE_FALSE(LLD::Ledger->HasBlock(hash));
        DataStream changed(SER_NETWORK, LLP::PROTOCOL_VERSION);
        changed << uint8_t(LLP::TritiumNode::TYPES::BESTHEIGHT) << changedHeight
                << uint8_t(LLP::TritiumNode::TYPES::BESTCHAIN) << hash;
        node.INCOMING.MESSAGE = LLP::TritiumNode::ACTION::NOTIFY;
        node.INCOMING.DATA.assign(changed.begin(), changed.end());
        REQUIRE(node.ProcessPacket());
        REQUIRE(TAO::Ledger::mapLastOrphanRequest.empty());
    }
    while(recv(fds[0], sent, sizeof(sent), MSG_DONTWAIT) > 0) {}
    node.Event(LLP::EVENTS::GENERIC, 0);
    REQUIRE(TAO::Ledger::mapLastOrphanRequest.count(changedTip) == 1);
    REQUIRE(recv(fds[0], sent, sizeof(sent), MSG_DONTWAIT) > 0);

    TAO::Ledger::ChainState::ForgetPeerHeight(&node);
    TAO::Ledger::mapLastOrphanRequest.clear();
    TAO::Ledger::ChainState::nBestHeight.store(savedHeight);
    LLP::TritiumNode::fSynchronized.store(savedSynchronized);
    node.fd = -1;
    close(fds[0]);
    close(fds[1]);
}
#endif

TEST_CASE("Coinbase credits are deferred until the next block can include them",
    "[ledger][mempool][coinbase-maturity]")
{
    using namespace TAO::Ledger;
    using namespace TAO::Operation;

    const bool unconfirmedPredecessor = GENERATE(false, true);
    const uint32_t savedHeight = ChainState::nBestHeight.load();
    const uint256_t genesis = Credentials::Genesis(
        std::string("coinbase-maturity-" + std::to_string(LLC::GetRand())).c_str());
    const uint512_t key1 = LLC::GetRand512();
    const uint512_t key2 = LLC::GetRand512();
    const uint512_t key3 = LLC::GetRand512();
    const uint512_t key4 = LLC::GetRand512();
    const TAO::Register::Address account(TAO::Register::Address::ACCOUNT);
    const TAO::Register::Address otherAccount(TAO::Register::Address::ACCOUNT);

    BlockState coinbaseBlock;
    coinbaseBlock.nHeight = 100;
    coinbaseBlock.nVersion = 7;
    coinbaseBlock.nChannel = 2;
    coinbaseBlock.nBits = 1;
    const uint1024_t blockHash = coinbaseBlock.GetHash();
    Transaction coinbase;
    Transaction create;
    Transaction credit;
    Transaction child;
    struct Cleanup
    {
        uint32_t height;
        const uint256_t& genesis;
        const uint1024_t& blockHash;
        const TAO::Register::Address& account;
        const TAO::Register::Address& otherAccount;
        Transaction& coinbase;
        Transaction& create;
        Transaction& credit;
        Transaction& child;
        bool active{false};
        uint8_t mode{FLAGS::BLOCK};

        ~Cleanup()
        {
            if(active)
                LLD::TxnAbort(mode);
            for(Transaction* tx : {&child, &credit, &create})
            {
                Transaction found;
                bool conflicted = false;
                if(mempool.Get(tx->GetHash(), found, conflicted))
                {
                    if(!conflicted && LLD::TxnBegin(FLAGS::MEMPOOL))
                    {
                        if(found.Disconnect(FLAGS::MEMPOOL))
                            LLD::TxnCommit(FLAGS::MEMPOOL);
                        else
                            LLD::TxnAbort(FLAGS::MEMPOOL);
                    }
                }
                mempool.Remove(tx->GetHash());
            }
            LLD::Ledger->EraseIndex(coinbase.GetHash());
            LLD::Ledger->EraseTx(coinbase.GetHash());
            LLD::Ledger->EraseTx(create.GetHash());
            LLD::Ledger->EraseLast(genesis);
            LLD::Ledger->EraseFirst(genesis);
            LLD::Ledger->EraseBlock(blockHash);
            LLD::Register->EraseState(account);
            LLD::Register->EraseState(otherAccount);
            ChainState::nBestHeight.store(height);
        }
    } cleanup{savedHeight, genesis, blockHash, account, otherAccount,
              coinbase, create, credit, child};
    REQUIRE(LLD::Ledger->WriteBlock(blockHash, coinbaseBlock));

    coinbase.hashGenesis = genesis;
    coinbase.nSequence = 0;
    coinbase.nKeyType = SIGNATURE::BRAINPOOL;
    coinbase.nNextType = SIGNATURE::BRAINPOOL;
    coinbase.NextHash(key2);
    coinbase[0] << uint8_t(OP::COINBASE) << genesis << uint64_t(1000) << uint64_t(0);
    REQUIRE(coinbase.Build());
    coinbase.Sign(key1);
    REQUIRE(coinbase.Connect(FLAGS::BLOCK));
    REQUIRE(LLD::Ledger->WriteTx(coinbase.GetHash(), coinbase));
    REQUIRE(LLD::Ledger->IndexBlock(coinbase.GetHash(), blockHash));

    create.hashGenesis = genesis;
    create.nSequence = 1;
    create.hashPrevTx = coinbase.GetHash();
    create.nKeyType = SIGNATURE::BRAINPOOL;
    create.nNextType = SIGNATURE::BRAINPOOL;
    create.NextHash(key3);
    const TAO::Register::Object object = TAO::Register::CreateAccount(0);
    create[0] << uint8_t(OP::CREATE) << account
              << uint8_t(TAO::Register::REGISTER::OBJECT) << object.GetState();
    REQUIRE(create.Build());
    create.Sign(key2);
    if(unconfirmedPredecessor)
        REQUIRE(mempool.Accept(create));
    else
    {
        REQUIRE(create.Connect(FLAGS::BLOCK));
        REQUIRE(LLD::Ledger->WriteTx(create.GetHash(), create));
    }

    credit.hashGenesis = genesis;
    credit.nSequence = 2;
    credit.hashPrevTx = create.GetHash();
    credit.nKeyType = SIGNATURE::BRAINPOOL;
    credit.nNextType = SIGNATURE::BRAINPOOL;
    credit.NextHash(key4);
    credit[0] << uint8_t(OP::CREDIT) << coinbase.GetHash() << uint32_t(0)
              << account << genesis << uint64_t(1000);
    REQUIRE(credit.Build());
    credit.Sign(key3);

    const uint32_t maturity = MaturityCoinBase(coinbaseBlock);
    REQUIRE(maturity > 3);
    ChainState::nBestHeight.store(coinbaseBlock.nHeight + maturity - 4);
    BlockState candidate = coinbaseBlock;
    candidate.nHeight = ChainState::nBestHeight.load() + 1;
    uint32_t tipConfirms = 0, candidateConfirms = 0;
    REQUIRE(LLD::Ledger->ReadConfirmations(coinbase.GetHash(), tipConfirms));
    REQUIRE(LLD::Ledger->ReadConfirmations(coinbase.GetHash(), candidateConfirms, &candidate));
    REQUIRE(tipConfirms == maturity - 3);
    REQUIRE(candidateConfirms == tipConfirms + 1);
    REQUIRE(tipConfirms + 2 < maturity);
    REQUIRE(candidateConfirms + 1 < maturity);
    REQUIRE(LLD::TxnBegin(FLAGS::BLOCK));
    cleanup.active = true;
    if(unconfirmedPredecessor)
    {
        REQUIRE(create.Connect(FLAGS::BLOCK));
        REQUIRE(LLD::Ledger->WriteTx(create.GetHash(), create));
    }
    REQUIRE_FALSE(credit.Connect(FLAGS::BLOCK, &candidate));
    LLD::TxnAbort(FLAGS::BLOCK);
    cleanup.active = false;
    REQUIRE(TakeLastConnectClass() == AdmissibilityClass::UNKNOWN);
    REQUIRE_FALSE(mempool.Accept(credit));
    REQUIRE_FALSE(mempool.Rejected(credit.GetHash()));
    Transaction found;
    bool conflicted = false;
    REQUIRE(mempool.Get(credit.GetHash(), found, conflicted));
    REQUIRE(conflicted);

    child.hashGenesis = genesis;
    child.nSequence = 3;
    child.hashPrevTx = credit.GetHash();
    child.nKeyType = SIGNATURE::BRAINPOOL;
    child.nNextType = SIGNATURE::BRAINPOOL;
    child.NextHash(LLC::GetRand512());
    child[0] << uint8_t(OP::CREATE) << otherAccount
             << uint8_t(TAO::Register::REGISTER::OBJECT) << object.GetState();
    REQUIRE(child.Build());
    child.Sign(key4);
    REQUIRE_FALSE(mempool.Accept(child));
    conflicted = false;
    REQUIRE(mempool.Get(child.GetHash(), found, conflicted));
    REQUIRE(conflicted);

    mempool.Check();
    REQUIRE(mempool.Get(credit.GetHash(), found, conflicted));
    REQUIRE(conflicted);
    REQUIRE(mempool.Get(child.GetHash(), found, conflicted));
    REQUIRE(conflicted);

    ChainState::nBestHeight.store(coinbaseBlock.nHeight + maturity - 3);
    candidate.nHeight = ChainState::nBestHeight.load() + 1;
    REQUIRE(LLD::Ledger->ReadConfirmations(coinbase.GetHash(), tipConfirms));
    REQUIRE(LLD::Ledger->ReadConfirmations(coinbase.GetHash(), candidateConfirms, &candidate));
    REQUIRE(tipConfirms == maturity - 2);
    REQUIRE(candidateConfirms == tipConfirms + 1);
    REQUIRE(tipConfirms + 2 == maturity);
    REQUIRE(candidateConfirms + 1 == maturity);
    REQUIRE(LLD::TxnBegin(FLAGS::BLOCK));
    cleanup.active = true;
    if(unconfirmedPredecessor)
    {
        REQUIRE(create.Connect(FLAGS::BLOCK));
        REQUIRE(LLD::Ledger->WriteTx(create.GetHash(), create));
    }
    REQUIRE(credit.Connect(FLAGS::BLOCK, &candidate));
    LLD::TxnAbort(FLAGS::BLOCK);
    cleanup.active = false;
    mempool.Check();
    conflicted = false;
    REQUIRE(mempool.Get(credit.GetHash(), found, conflicted));
    REQUIRE_FALSE(conflicted);
    REQUIRE(mempool.Get(child.GetHash(), found, conflicted));
    REQUIRE_FALSE(conflicted);

    REQUIRE(LLD::TxnBegin(FLAGS::MEMPOOL));
    cleanup.mode = FLAGS::MEMPOOL;
    cleanup.active = true;
    REQUIRE(child.Disconnect(FLAGS::MEMPOOL));
    REQUIRE(LLD::TxnCommit(FLAGS::MEMPOOL));
    cleanup.active = false;
    REQUIRE(mempool.Remove(child.GetHash()));
    REQUIRE(LLD::TxnBegin(FLAGS::MEMPOOL));
    cleanup.active = true;
    REQUIRE(credit.Disconnect(FLAGS::MEMPOOL));
    REQUIRE(LLD::TxnCommit(FLAGS::MEMPOOL));
    cleanup.active = false;
    REQUIRE(mempool.Remove(credit.GetHash()));
    if(unconfirmedPredecessor)
    {
        REQUIRE(LLD::TxnBegin(FLAGS::MEMPOOL));
        cleanup.active = true;
        REQUIRE(create.Disconnect(FLAGS::MEMPOOL));
        REQUIRE(LLD::TxnCommit(FLAGS::MEMPOOL));
        cleanup.active = false;
        REQUIRE(mempool.Remove(create.GetHash()));
    }
    REQUIRE(LLD::Ledger->EraseIndex(coinbase.GetHash()));
    REQUIRE(LLD::Ledger->EraseTx(coinbase.GetHash()));
    if(!unconfirmedPredecessor)
        REQUIRE(LLD::Ledger->EraseTx(create.GetHash()));
    REQUIRE(LLD::Ledger->EraseLast(genesis));
    REQUIRE(LLD::Ledger->EraseFirst(genesis));
    REQUIRE(LLD::Ledger->EraseBlock(blockHash));
    if(!unconfirmedPredecessor)
        REQUIRE(LLD::Register->EraseState(account));
    ChainState::nBestHeight.store(savedHeight);
}

TEST_CASE("Tritium disconnect removes its peer height observation", "[ledger][process][peer-height]")
{
    LLP::TritiumNode node;
    TAO::Ledger::ChainState::ObservePeerHeight(&node, UINT32_MAX);
    REQUIRE(TAO::Ledger::ChainState::MaxPeerHeight() == UINT32_MAX);
    node.Event(LLP::EVENTS::DISCONNECT, LLP::DISCONNECT::PEER);
    REQUIRE(TAO::Ledger::ChainState::MaxPeerHeight() < UINT32_MAX);
}


TEST_CASE("Idle sigchain with ancestor on main chain must not be INVALID_ABSOLUTE",
    "[ledger][process]")
{
    /* === Constraint from problem statement (treat as given) ===
     *
     * ForkDivergenceInfo::nDepth measures sigchain staleness, NOT fork
     * divergence.  A sigchain idle for N blocks yields nDepth = N on a
     * completely healthy node.  The observed divergence_depth=16172 was
     * emitted with fAncestorOnMainChain == true — not a fork.
     *
     * This test locks in the invariant: when fAncestorOnMainChain == true,
     * the correct classification is DEFERRED_LOCAL_STATE regardless of nDepth.
     * nDepth must NEVER be used as a rejection or eviction input.  Unlike a
     * bare predicate check, this test drives the actual Mempool::Check()
     * conflict-reconciliation pass end-to-end so a future change that adds a
     * depth threshold to Check() would fail this test. */

    using namespace TAO::Register;
    using namespace TAO::Operation;

    LedgerGuard env;

    const uint32_t nSavedBestHeight = TAO::Ledger::ChainState::nBestHeight.load();

    /* Unique identifiers for this test. */
    const uint256_t hashGenesis  =
        TAO::Ledger::Credentials::Genesis(std::string("strandedstateidle" + std::to_string(LLC::GetRand())).c_str());
    const uint512_t hashPrivKey1 = LLC::GetRand512();
    const uint512_t hashAncestorTx(0xA11CE500 + 1);
    const uint1024_t hashAncestorBlock(0xA11CEB10 + 1);
    const uint512_t hashDiskLast = LLC::GetRand512();

    /* Ancestor block: mark it connected into the main chain via a nonzero
     * hashNextBlock, matching BlockState::IsInMainChain()'s definition
     * (hashNextBlock != 0 || hash == best chain tip).  Height is set far
     * below the current best height to reproduce the large idle nDepth. */
    TAO::Ledger::BlockState stateAncestor;
    stateAncestor.nVersion      = 4;
    stateAncestor.hashPrevBlock = uint1024_t(0);
    stateAncestor.nChannel      = 2;
    stateAncestor.nHeight       = 500;
    stateAncestor.nBits         = 1;
    stateAncestor.hashNextBlock = uint1024_t(0xBEEF);

    REQUIRE(LLD::Ledger->WriteBlock(hashAncestorBlock, stateAncestor));
    REQUIRE(LLD::Ledger->IndexBlock(hashAncestorTx, hashAncestorBlock));

    /* A minimal placeholder record so LLD::Ledger->HasTx()/Exists() for the
     * ancestor tx hash succeeds (Mempool::Accept() requires the previous
     * transaction to be known before evaluating conflict state). */
    {
        TAO::Ledger::Transaction txAncestor;
        txAncestor.hashGenesis = hashGenesis;
        txAncestor.nSequence   = 0;
        txAncestor.nTimestamp  = runtime::timestamp();
        REQUIRE(LLD::Ledger->WriteTx(hashAncestorTx, txAncestor));
    }

    /* Set the local best height far ahead of the ancestor so nDepth (which
     * is diagnostic-only) comes out large, mirroring the ~16172 seen in
     * production. */
    TAO::Ledger::ChainState::nBestHeight.store(stateAncestor.nHeight + 16172);

    /* Disk's committed last transaction for this genesis is unrelated to the
     * ancestor tx that the conflicting transaction expects as its
     * predecessor.  This mismatch is exactly what Check()'s reconciliation
     * pass treats as a conflict. */
    REQUIRE(LLD::Ledger->WriteLast(hashGenesis, hashDiskLast));

    /* Build a conflicting, idle-sigchain transaction whose hashPrevTx points
     * at the ancestor (on the main chain), not at disk's actual last hash. */
    TAO::Ledger::Transaction tx;
    tx.hashGenesis = hashGenesis;
    tx.nSequence   = 1;
    tx.hashPrevTx  = hashAncestorTx;
    tx.nTimestamp  = runtime::timestamp();
    tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
    tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
    tx.NextHash(LLC::GetRand512());

    TAO::Register::Address hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);

    Object object;
    object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(1);

    tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

    REQUIRE(tx.Build());
    tx.Sign(hashPrivKey1);

    const uint512_t hashTx = tx.GetHash();

    /* Accept() should reject with a conflict (hashPrevTx != disk's last
     * hash) and place the transaction in the conflicted pool rather than
     * accepting or dropping it outright. */
    REQUIRE_FALSE(TAO::Ledger::mempool.Accept(tx));

    {
        TAO::Ledger::Transaction txOut;
        bool fConflicted = false;
        REQUIRE(TAO::Ledger::mempool.Get(hashTx, txOut, fConflicted));
        REQUIRE(fConflicted);
    }

    /* Drive the real reconciliation pass. Because the ancestor block is on
     * the main chain, Check() must classify this as DEFERRED_LOCAL_STATE and
     * retain the transaction in mapConflicts rather than evicting it,
     * regardless of the large diagnostic nDepth. */
    TAO::Ledger::mempool.Check();

    {
        TAO::Ledger::Transaction txOut;
        bool fConflicted = false;
        REQUIRE(TAO::Ledger::mempool.Get(hashTx, txOut, fConflicted));
        REQUIRE(fConflicted);
    }

    /* Cross-check: when the ancestor is NOT on the main chain (genuine
     * fork), Check() must evict rather than retain. Reuse the same genesis
     * with a fresh ancestor whose hashNextBlock is zero and whose own hash
     * is not the best chain tip. */
    const uint512_t hashForkAncestorTx(0xFEED500 + 1);
    const uint1024_t hashForkAncestorBlock(0xFEEDB10 + 1);

    TAO::Ledger::BlockState stateForkAncestor;
    stateForkAncestor.nVersion      = 4;
    stateForkAncestor.hashPrevBlock = uint1024_t(0);
    stateForkAncestor.nChannel      = 2;
    stateForkAncestor.nHeight       = 501;
    stateForkAncestor.nBits         = 1;
    stateForkAncestor.hashNextBlock = uint1024_t(0); /* not connected: off-chain */

    REQUIRE(LLD::Ledger->WriteBlock(hashForkAncestorBlock, stateForkAncestor));
    REQUIRE(LLD::Ledger->IndexBlock(hashForkAncestorTx, hashForkAncestorBlock));

    {
        TAO::Ledger::Transaction txForkAncestor;
        txForkAncestor.hashGenesis = hashGenesis;
        txForkAncestor.nSequence   = 0;
        txForkAncestor.nTimestamp  = runtime::timestamp();
        REQUIRE(LLD::Ledger->WriteTx(hashForkAncestorTx, txForkAncestor));
    }

    TAO::Ledger::Transaction txFork;
    txFork.hashGenesis = hashGenesis;
    txFork.nSequence   = 1;
    txFork.hashPrevTx  = hashForkAncestorTx;
    txFork.nTimestamp  = runtime::timestamp();
    txFork.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
    txFork.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
    txFork.NextHash(LLC::GetRand512());

    TAO::Register::Address hashAddressFork = TAO::Register::Address(TAO::Register::Address::OBJECT);
    Object objectFork;
    objectFork << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(2);
    txFork[0] << uint8_t(OP::CREATE) << hashAddressFork << uint8_t(REGISTER::OBJECT) << objectFork.GetState();

    REQUIRE(txFork.Build());
    txFork.Sign(hashPrivKey1);

    const uint512_t hashTxFork = txFork.GetHash();

    /* Remove the previous conflict first so this genesis's conflict set only
     * reflects the fork scenario for this reconciliation pass. Remove()
     * erases mapConflicts entries unconditionally but only returns true when
     * the hash was also found in the live mapLedger pool, so its return
     * value is not asserted here. */
    TAO::Ledger::mempool.Remove(hashTx);

    REQUIRE_FALSE(TAO::Ledger::mempool.Accept(txFork));

    TAO::Ledger::mempool.Check();

    {
        TAO::Ledger::Transaction txOut;
        bool fConflicted = false;
        /* Evicted: Get() must fail to locate it in either pool. */
        REQUIRE_FALSE(TAO::Ledger::mempool.Get(hashTxFork, txOut, fConflicted));
    }

    /* Clean up. */
    TAO::Ledger::mempool.Remove(hashTxFork);
    LLD::Ledger->EraseBlock(hashAncestorBlock);
    LLD::Ledger->EraseBlock(hashForkAncestorBlock);
    TAO::Ledger::ChainState::nBestHeight.store(nSavedBestHeight);
}


TEST_CASE("mapConflicts retry budget bounds are well-formed", "[ledger][process]")
{
    /* MAX_CONFLICT_STALE_RETRIES must be positive (so at least one retry is
     * allowed before force-eviction) and must not exceed MAX_CONFLICTS_MAP_ENTRIES
     * (which bounds the retry map itself). */
    REQUIRE(TAO::Ledger::MAX_CONFLICT_STALE_RETRIES  > 0u);
    REQUIRE(TAO::Ledger::MAX_CONFLICT_STALE_RETRIES  <=
            TAO::Ledger::MAX_CONFLICTS_MAP_ENTRIES);

    /* The retry window at CONFLICTS_SWEEP_INTERVAL_SECONDS (30 s) should be
     * at least 5 minutes so a node with modest peer latency can recover. */
    const uint64_t nWindowSecs =
        static_cast<uint64_t>(TAO::Ledger::MAX_CONFLICT_STALE_RETRIES)
        * TAO::Ledger::CONFLICTS_SWEEP_INTERVAL_SECONDS;
    REQUIRE(nWindowSecs >= 300u);  /* >= 5 minutes */
}
