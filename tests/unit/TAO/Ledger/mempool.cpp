/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/rollback.h>
#include <TAO/Register/include/create.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/include/verify.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/credentials.h>

#include <unit/catch2/catch.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>

namespace TAO::Ledger
{
    struct MempoolTestAccess
    {
        static void Root(Mempool& pool, const Transaction& tx)
        {
            RECURSIVE(pool.MUTEX);
            pool.AddConflictRoot(tx);
        }

        static bool Park(Mempool& pool, const Transaction& tx)
        {
            RECURSIVE(pool.MUTEX);
            return pool.ParkConflictDependent(tx);
        }

        static void Drain(Mempool& pool, const uint512_t& hash)
        {
            pool.ProcessConflictDependents(hash);
        }

        static void Unroot(Mempool& pool, const uint512_t& hash)
        {
            RECURSIVE(pool.MUTEX);
            pool.EraseConflictRoot(hash);
        }

        static void Claim(Mempool& pool, const uint512_t& hash)
        {
            RECURSIVE(pool.MUTEX);
            pool.mapClaimed[hash] = uint512_t(1);
        }

        static bool Rejected(Mempool& pool, const uint512_t& hash)
        {
            RECURSIVE(pool.MUTEX);
            return pool.mapRejected.count(hash);
        }

        static void FillRoots(Mempool& pool, const Transaction& tx)
        {
            RECURSIVE(pool.MUTEX);
            for(uint32_t i = 1; pool.mapConflicts.size() < MAX_CONFLICTS_MAP_ENTRIES; ++i)
                pool.mapConflicts[uint512_t(i)] = tx;
        }

        static void SeedRetryState(Mempool& pool, const uint256_t& genesis)
        {
            RECURSIVE(pool.MUTEX);
            pool.mapConflictRetries[genesis];
            pool.setStrandedGeneses.insert(genesis);
            pool.mapUnknownAncestorRetries[genesis];
            pool.setUnknownAncestorGeneses.insert(genesis);
        }

        static bool HasRetryState(Mempool& pool, const uint256_t& genesis)
        {
            RECURSIVE(pool.MUTEX);
            return pool.mapConflictRetries.count(genesis) ||
                pool.setStrandedGeneses.count(genesis) ||
                pool.mapUnknownAncestorRetries.count(genesis) ||
                pool.setUnknownAncestorGeneses.count(genesis);
        }

        static bool PrepareRestore(Mempool& pool, const Transaction& tx)
        {
            std::vector<uint512_t> resolved;
            bool committed = false, confirmed = false, restore = false;
            pool.AcceptTransaction(tx, nullptr, resolved, committed, confirmed, restore);
            return restore && !committed && !confirmed;
        }

        static void Restore(Mempool& pool, const Transaction& tx, bool& confirmed)
        {
            pool.RestoreConflictRoot(tx, confirmed);
        }
    };
}

namespace
{
    TAO::Ledger::Transaction CoordinatorTestTransaction(const TAO::Register::Address& address,
                                                       const uint256_t& genesis = 0)
    {
        TAO::Ledger::Transaction tx;
        tx.hashGenesis = genesis != 0 ? genesis
            : TAO::Ledger::Credentials::Genesis(LLC::GetRand256().ToString().c_str());
        tx.nTimestamp = runtime::timestamp();
        tx.nKeyType = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.nNextType = TAO::Ledger::SIGNATURE::BRAINPOOL;
        tx.NextHash(LLC::GetRand512());
        const std::string strHybrid = config::GetArg("-hybrid", "");
        tx.hashPrevTx = LLC::SK512(strHybrid.begin(), strHybrid.end());
        const auto account = TAO::Register::CreateAccount(uint256_t(0));
        tx[0] << uint8_t(TAO::Operation::OP::CREATE) << address
              << uint8_t(TAO::Register::REGISTER::OBJECT) << account.GetState();
        REQUIRE(tx.Build());
        REQUIRE(tx.Sign(LLC::GetRand512()));
        return tx;
    }

    struct RetryChain
    {
        TAO::Ledger::Mempool& pool = TAO::Ledger::mempool;
        std::vector<TAO::Register::Address> addresses;
        std::vector<TAO::Ledger::Transaction> txs;

        RetryChain()
        {
            uint512_t key = LLC::GetRand512();
            for(uint32_t i = 0; i < 3; ++i)
            {
                addresses.emplace_back(TAO::Register::Address::ACCOUNT);
                auto tx = CoordinatorTestTransaction(addresses.back(), i ? txs.front().hashGenesis : uint256_t(0));
                if(i)
                {
                    tx.hashGenesis = txs.front().hashGenesis;
                    tx.nSequence = i;
                    tx.hashPrevTx = txs.back().GetHash();
                }
                const auto next = LLC::GetRand512();
                tx.NextHash(next);
                tx.hashCache = 0;
                REQUIRE(tx.Build());
                REQUIRE(tx.Sign(key));
                key = next;
                txs.push_back(tx);
            }
        }

        ~RetryChain()
        {
            for(auto it = txs.rbegin(); it != txs.rend(); ++it)
            {
                pool.Remove(it->GetHash());
                LLD::Ledger->EraseIndex(it->GetHash());
                LLD::Ledger->EraseTx(it->GetHash());
            }
            LLD::Ledger->EraseLast(txs.front().hashGenesis);
            for(const auto& address : addresses)
                LLD::Register->EraseState(address, TAO::Ledger::FLAGS::MEMPOOL);
        }

        void Park()
        {
            TAO::Ledger::MempoolTestAccess::Root(pool, txs[0]);
            REQUIRE(TAO::Ledger::MempoolTestAccess::Park(pool, txs[1]));
            REQUIRE(TAO::Ledger::MempoolTestAccess::Park(pool, txs[2]));
        }

        void DiskParent()
        {
            REQUIRE(LLD::Ledger->WriteTx(txs[0].GetHash(), txs[0]));
            REQUIRE(LLD::Ledger->IndexBlock(txs[0].GetHash(), TAO::Ledger::ChainState::hashBestChain.load()));
            REQUIRE(LLD::Ledger->WriteLast(txs[0].hashGenesis, txs[0].GetHash()));
        }
    };
}

TEST_CASE("Mempool conflict lookup excludes queued orphans and admitted transactions",
          "[mempool][mempool_retry]")
{
    RetryChain chain;
    auto& pool = chain.pool;
    REQUIRE_FALSE(pool.Accept(chain.txs[1]));
    REQUIRE(pool.Has(chain.txs[1].GetHash()));
    REQUIRE_FALSE(pool.IsConflictNode(chain.txs[1].GetHash()));
    REQUIRE_FALSE(pool.Accept(chain.txs[1]));
    REQUIRE_FALSE(pool.IsConflictNode(chain.txs[1].GetHash()));
    // Resurrection must not suppress the next orphan in the same sigchain.
    REQUIRE_FALSE(pool.Accept(chain.txs[2]));
    REQUIRE(pool.Has(chain.txs[2].GetHash()));
    REQUIRE_FALSE(pool.IsConflictNode(chain.txs[2].GetHash()));
    REQUIRE(pool.AddUnchecked(chain.txs[0]));
    REQUIRE_FALSE(pool.IsConflictNode(chain.txs[0].GetHash()));
    pool.Remove(chain.txs[0].GetHash());
    TAO::Ledger::MempoolTestAccess::Root(pool, chain.txs[0]);
    REQUIRE(pool.IsConflictNode(chain.txs[0].GetHash()));
}

TEST_CASE("Mempool dependent retries retain reclassified and transient tails",
          "[mempool][mempool_retry]")
{
    using Access = TAO::Ledger::MempoolTestAccess;
    const auto outcome = GENERATE(std::string("root"), std::string("dependent"),
                                 std::string("orphan"), std::string("transient"),
                                 std::string("live"), std::string("rejected"));
    CAPTURE(outcome);
    RetryChain chain;
    chain.Park();
    if(outcome != "dependent")
        Access::Unroot(chain.pool, chain.txs[0].GetHash());
    if(outcome == "root" || outcome == "transient" || outcome == "rejected")
    {
        chain.DiskParent();
        if(outcome == "root")
            REQUIRE(LLD::Ledger->WriteLast(chain.txs[0].hashGenesis, LLC::GetRand512()));
        else if(outcome == "transient")
            REQUIRE(LLD::Ledger->EraseLast(chain.txs[0].hashGenesis));
        else
        {
            // A changed register prestate is a definitive Verify rejection.
            TAO::Register::State state;
            state.nType = TAO::Register::REGISTER::OBJECT;
            REQUIRE(LLD::Register->WriteState(chain.addresses[1], state, TAO::Ledger::FLAGS::MEMPOOL));
        }
    }
    if(outcome == "live")
    {
        REQUIRE(chain.pool.AddUnchecked(chain.txs[0]));
        LLD::TransactionGuard transaction(TAO::Ledger::FLAGS::MEMPOOL);
        REQUIRE(bool(transaction));
        Access::Drain(chain.pool, chain.txs[0].GetHash());
    }
    else
        Access::Drain(chain.pool, chain.txs[0].GetHash());
    REQUIRE(chain.pool.Has(chain.txs[2].GetHash()) == (outcome != "rejected"));
    REQUIRE(Access::Rejected(chain.pool, chain.txs[1].GetHash()) == (outcome == "rejected"));
    if(outcome == "root" || outcome == "dependent" || outcome == "transient" || outcome == "live")
        REQUIRE(chain.pool.IsConflictNode(chain.txs[1].GetHash()));
    if(outcome == "orphan")
    {
        REQUIRE(chain.pool.Has(chain.txs[1].GetHash()));
        REQUIRE_FALSE(chain.pool.IsConflictNode(chain.txs[1].GetHash()));
    }
}

TEST_CASE("Mempool stale marker guard preserves retryable tails",
          "[mempool][mempool_retry]")
{
    const auto outcome = GENERATE(std::string("root"), std::string("transient"), std::string("rejected"));
    CAPTURE(outcome);
    RetryChain chain;
    chain.Park();
    chain.DiskParent();
    if(outcome == "root")
        REQUIRE(LLD::Ledger->WriteLast(chain.txs[0].hashGenesis, LLC::GetRand512()));
    else if(outcome == "transient")
        REQUIRE(LLD::Ledger->EraseLast(chain.txs[0].hashGenesis));
    else
    {
        TAO::Register::State state;
        state.nType = TAO::Register::REGISTER::OBJECT;
        REQUIRE(LLD::Register->WriteState(chain.addresses[1], state, TAO::Ledger::FLAGS::MEMPOOL));
    }
    REQUIRE_FALSE(chain.pool.Accept(chain.txs[1]));
    REQUIRE(chain.pool.Has(chain.txs[2].GetHash()) == (outcome != "rejected"));
    REQUIRE(chain.pool.IsConflictNode(chain.txs[1].GetHash()) == (outcome != "rejected"));
}

TEST_CASE("Mempool dependent retries do not restore tails after DAG eviction",
          "[mempool][mempool_retry]")
{
    using Access = TAO::Ledger::MempoolTestAccess;
    RetryChain chain;
    chain.Park();
    Access::FillRoots(chain.pool, chain.txs[0]);
    Access::Drain(chain.pool, chain.txs[0].GetHash());
    for(const auto& tx : chain.txs)
        REQUIRE_FALSE(chain.pool.Has(tx.GetHash()));
}

TEST_CASE("Mempool disconnected predecessors retain their conflict markers and tails",
          "[mempool][mempool_retry]")
{
    RetryChain chain;
    chain.Park();
    chain.DiskParent();
    REQUIRE(LLD::Ledger->EraseIndex(chain.txs[0].GetHash()));
    REQUIRE_FALSE(chain.pool.Accept(chain.txs[1]));
    REQUIRE(chain.pool.Conflicts() == 1);
    for(const auto& tx : chain.txs)
        REQUIRE(chain.pool.IsConflictNode(tx.GetHash()));

    REQUIRE(LLD::Ledger->IndexBlock(chain.txs[0].GetHash(), TAO::Ledger::ChainState::hashBestChain.load()));
    REQUIRE(chain.pool.Accept(chain.txs[1]));
    REQUIRE_FALSE(chain.pool.IsConflictNode(chain.txs[0].GetHash()));
    REQUIRE_FALSE(chain.pool.IsConflictNode(chain.txs[2].GetHash()));
    TAO::Register::State state;
    REQUIRE(LLD::Register->ReadState(chain.addresses[2], state, TAO::Ledger::FLAGS::MEMPOOL));
}

TEST_CASE("Mempool confirmation clears retry state only after the last genesis root",
          "[mempool][mempool_retry]")
{
    using Access = TAO::Ledger::MempoolTestAccess;
    RetryChain chain;
    TAO::Ledger::Mempool pool;
    Access::Root(pool, chain.txs[0]);
    Access::Root(pool, chain.txs[1]);
    Access::SeedRetryState(pool, chain.txs[0].hashGenesis);
    for(size_t i = 0; i < 2; ++i)
    {
        REQUIRE(LLD::Ledger->WriteTx(chain.txs[i].GetHash(), chain.txs[i]));
        REQUIRE(LLD::Ledger->IndexBlock(chain.txs[i].GetHash(), TAO::Ledger::ChainState::hashBestChain.load()));
        bool confirmed = false;
        REQUIRE_FALSE(pool.Accept(chain.txs[i], nullptr, nullptr, &confirmed));
        REQUIRE(confirmed);
        REQUIRE(Access::HasRetryState(pool, chain.txs[i].hashGenesis) == (i == 0));
    }
    Access::Root(pool, chain.txs[2]);
    REQUIRE_FALSE(Access::HasRetryState(pool, chain.txs[2].hashGenesis));
}

TEST_CASE("Mempool retry restoration rechecks confirmation under the coordinator",
          "[mempool][mempool_retry][mempool_coordinator]")
{
    using Access = TAO::Ledger::MempoolTestAccess;
    const bool indexed = GENERATE(false, true);
    RetryChain chain;
    chain.Park();
    chain.DiskParent();
    REQUIRE(LLD::Ledger->EraseLast(chain.txs[0].hashGenesis));
    REQUIRE(Access::PrepareRestore(chain.pool, chain.txs[1]));
    Access::SeedRetryState(chain.pool, chain.txs[1].hashGenesis);

    std::promise<void> waiting;
    auto waitFuture = waiting.get_future();
    std::atomic<bool> notified{false};
    bool confirmed = false;
    bool waitingObserved = false;
    bool wrote = false;
    std::future<void> contender;
    {
        LLD::TransactionCoordinatorGuard coordinator;
        LLD::SetTxnCoordinatorWaitHook([&]()
        {
            if(!notified.exchange(true))
                waiting.set_value();
        });
        contender = std::async(std::launch::async, [&]()
        {
            Access::Restore(chain.pool, chain.txs[1], confirmed);
        });
        waitingObserved = waitFuture.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
        wrote = LLD::Ledger->WriteTx(chain.txs[1].GetHash(), chain.txs[1]) &&
            LLD::Ledger->WriteLast(chain.txs[1].hashGenesis, chain.txs[1].GetHash());
        if(indexed)
            wrote = wrote && LLD::Ledger->IndexBlock(chain.txs[1].GetHash(),
                TAO::Ledger::ChainState::hashBestChain.load());
    }
    contender.get();
    LLD::SetTxnCoordinatorWaitHook({});
    REQUIRE(waitingObserved);
    REQUIRE(wrote);
    REQUIRE(confirmed == indexed);
    REQUIRE(chain.pool.IsConflictNode(chain.txs[1].GetHash()) == !indexed);
    REQUIRE(Access::HasRetryState(chain.pool, chain.txs[1].hashGenesis) == !indexed);
    REQUIRE(chain.pool.IsConflictNode(chain.txs[2].GetHash()));
    if(indexed)
    {
        Access::Drain(chain.pool, chain.txs[1].GetHash());
        REQUIRE_FALSE(chain.pool.IsConflictNode(chain.txs[2].GetHash()));
        TAO::Register::State state;
        REQUIRE_FALSE(LLD::Register->ReadState(chain.addresses[1], state, TAO::Ledger::FLAGS::MEMPOOL));
        REQUIRE(LLD::Register->ReadState(chain.addresses[2], state, TAO::Ledger::FLAGS::MEMPOOL));
    }
}

TEST_CASE("Mempool Check preserves reclassified roots and orphan tails",
          "[mempool][mempool_retry]")
{
    using Access = TAO::Ledger::MempoolTestAccess;
    const auto outcome = GENERATE(std::string("root"), std::string("orphan"),
                                 std::string("transient"), std::string("rejected"),
                                 std::string("confirmed"));
    CAPTURE(outcome);
    RetryChain chain;
    TAO::Ledger::Mempool pool;
    Access::Root(pool, chain.txs[1]);
    REQUIRE(Access::Park(pool, chain.txs[2]));
    chain.DiskParent();
    if(outcome == "root")
        Access::Claim(pool, chain.txs[0].GetHash());
    else if(outcome == "orphan")
        REQUIRE(LLD::Ledger->EraseTx(chain.txs[0].GetHash()));
    else if(outcome == "rejected")
    {
        TAO::Register::State state;
        state.nType = TAO::Register::REGISTER::OBJECT;
        REQUIRE(LLD::Register->WriteState(chain.addresses[1], state, TAO::Ledger::FLAGS::MEMPOOL));
    }
    else if(outcome == "confirmed")
    {
        REQUIRE(LLD::Ledger->WriteTx(chain.txs[1].GetHash(), chain.txs[1]));
        REQUIRE(LLD::Ledger->IndexBlock(chain.txs[1].GetHash(), TAO::Ledger::ChainState::hashBestChain.load()));
    }
    if(outcome == "transient")
    {
        LLD::TransactionGuard transaction(TAO::Ledger::FLAGS::MEMPOOL);
        REQUIRE(bool(transaction));
        pool.Check();
    }
    else
        pool.Check();
    const bool retryable = outcome != "rejected" && outcome != "confirmed";
    REQUIRE(pool.Has(chain.txs[2].GetHash()) == (outcome != "rejected"));
    REQUIRE(Access::Rejected(pool, chain.txs[1].GetHash()) == (outcome == "rejected"));
    REQUIRE(pool.Has(chain.txs[1].GetHash()) == retryable);
    if(outcome == "confirmed")
    {
        REQUIRE(pool.IsConflictNode(chain.txs[2].GetHash()));
        REQUIRE(LLD::Ledger->WriteLast(chain.txs[1].hashGenesis, chain.txs[1].GetHash()));
        pool.Check();
        TAO::Register::State state;
        REQUIRE(LLD::Register->ReadState(chain.addresses[2], state, TAO::Ledger::FLAGS::MEMPOOL));
    }
}

TEST_CASE("Mempool commit-time confirmation wins over admission and detached queues",
          "[mempool][mempool_coordinator][mempool_retry]")
{
    using Access = TAO::Ledger::MempoolTestAccess;
    const auto mode = GENERATE(std::string("accept"), std::string("orphan"), std::string("dependent"));
    CAPTURE(mode);
    RetryChain chain;
    auto& pool = chain.pool;
    if(mode == "orphan")
    {
        REQUIRE_FALSE(pool.Accept(chain.txs[1]));
        REQUIRE_FALSE(pool.Accept(chain.txs[2]));
    }
    else if(mode == "dependent")
        chain.Park();
    else
    {
        Access::Root(pool, chain.txs[1]);
        REQUIRE(Access::Park(pool, chain.txs[2]));
    }
    chain.DiskParent();

    std::promise<void> waiting;
    auto waitFuture = waiting.get_future();
    std::atomic<bool> notified{false};
    bool committed = true;
    bool confirmed = false;
    bool waitingObserved = false;
    bool wrote = false;
    std::future<bool> contender;
    {
        LLD::TransactionCoordinatorGuard coordinator;
        LLD::SetTxnCoordinatorWaitHook([&]()
        {
            if(!notified.exchange(true))
                waiting.set_value();
        });
        contender = std::async(std::launch::async, [&]()
        {
            if(mode == "orphan")
                pool.ProcessOrphans(chain.txs[0].GetHash());
            else if(mode == "dependent")
                Access::Drain(pool, chain.txs[0].GetHash());
            else
                return pool.Accept(chain.txs[1], nullptr, &committed, &confirmed);
            return false;
        });
        waitingObserved = waitFuture.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
        wrote = LLD::Ledger->WriteTx(chain.txs[1].GetHash(), chain.txs[1]) &&
            LLD::Ledger->IndexBlock(chain.txs[1].GetHash(), TAO::Ledger::ChainState::hashBestChain.load()) &&
            LLD::Ledger->WriteLast(chain.txs[1].hashGenesis, chain.txs[1].GetHash());
    }
    const bool accepted = contender.get();
    LLD::SetTxnCoordinatorWaitHook({});
    REQUIRE(waitingObserved);
    REQUIRE(wrote);
    REQUIRE_FALSE(accepted);
    REQUIRE_FALSE(pool.Has(chain.txs[1].GetHash()));
    REQUIRE(pool.Has(chain.txs[2].GetHash()));
    REQUIRE_FALSE(pool.IsConflictNode(chain.txs[2].GetHash()));
    if(mode == "accept")
    {
        REQUIRE_FALSE(committed);
        REQUIRE(confirmed);
    }
    TAO::Register::State state;
    REQUIRE_FALSE(LLD::Register->ReadState(chain.addresses[1], state, TAO::Ledger::FLAGS::MEMPOOL));
    REQUIRE(LLD::Register->ReadState(chain.addresses[2], state, TAO::Ledger::FLAGS::MEMPOOL));
}

TEST_CASE("Mempool resurrects disk transactions after their block index is disconnected",
          "[mempool][mempool_retry]")
{
    RetryChain chain;
    chain.DiskParent();
    REQUIRE(LLD::Ledger->WriteTx(chain.txs[1].GetHash(), chain.txs[1]));
    REQUIRE(LLD::Ledger->IndexBlock(chain.txs[1].GetHash(), TAO::Ledger::ChainState::hashBestChain.load()));
    REQUIRE(LLD::Ledger->EraseIndex(chain.txs[1].GetHash()));
    bool committed = false;
    bool confirmed = true;
    REQUIRE(chain.pool.Accept(chain.txs[1], nullptr, &committed, &confirmed));
    REQUIRE(committed);
    REQUIRE_FALSE(confirmed);
    REQUIRE(chain.pool.Has(chain.txs[1].GetHash()));
}

TEST_CASE("Mempool retains freshly resurrected conflict descendants until their root resolves",
          "[mempool][mempool_retry]")
{
    RetryChain chain;
    chain.DiskParent();
    REQUIRE(LLD::Ledger->WriteLast(chain.txs[0].hashGenesis, LLC::GetRand512()));
    for(size_t i = 1; i < chain.txs.size(); ++i)
    {
        REQUIRE(LLD::Ledger->WriteTx(chain.txs[i].GetHash(), chain.txs[i]));
        REQUIRE_FALSE(chain.pool.Accept(chain.txs[i]));
        REQUIRE(chain.pool.IsConflictNode(chain.txs[i].GetHash()));
    }
    REQUIRE(chain.pool.Conflicts() == 1);
    REQUIRE(chain.pool.ConflictDependents() == 1);

    REQUIRE(LLD::Ledger->WriteLast(chain.txs[0].hashGenesis, chain.txs[0].GetHash()));
    chain.pool.Check();
    for(size_t i = 1; i < chain.txs.size(); ++i)
    {
        REQUIRE(chain.pool.Has(chain.txs[i].GetHash()));
        REQUIRE_FALSE(chain.pool.IsConflictNode(chain.txs[i].GetHash()));
        TAO::Register::State state;
        REQUIRE(LLD::Register->ReadState(chain.addresses[i], state, TAO::Ledger::FLAGS::MEMPOOL));
    }
}

TEST_CASE("Mempool preflight and empty orphan drains do not reserve the coordinator",
          "[mempool][mempool_coordinator]")
{
    TAO::Ledger::Mempool pool;
    std::future<bool> contender;
    bool fCompleted = false;
    const bool fOrphans = GENERATE(false, true);
    {
        LLD::TransactionCoordinatorGuard coordinator;
        contender = std::async(std::launch::async, [&]()
        {
            if(fOrphans)
            {
                pool.ProcessOrphans(uint512_t(0xCAFF03));
                return true;
            }
            return !pool.Accept(TAO::Ledger::Transaction());
        });
        fCompleted = contender.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
    }
    const bool fResult = contender.get();
    REQUIRE(fCompleted);
    REQUIRE(fResult);
}

TEST_CASE("Mempool admission rechecks concurrent insertion after waiting without its mutex",
          "[mempool][mempool_coordinator]")
{
    TAO::Ledger::Mempool pool;
    const TAO::Register::Address address(TAO::Register::Address::ACCOUNT);
    const auto tx = CoordinatorTestTransaction(address);
    const bool fDuplicate = GENERATE(false, true);
    auto competing = tx;
    if(!fDuplicate)
    {
        competing.nTimestamp++;
        competing.hashCache = 0;
    }

    std::promise<void> waiting;
    auto waitFuture = waiting.get_future();
    std::atomic<bool> notified{false};
    std::future<bool> contender;
    bool fWaiting = false;
    bool fMutexAvailable = false;
    bool fInserted = false;
    {
        LLD::TransactionCoordinatorGuard coordinator;
        LLD::SetTxnCoordinatorWaitHook([&]()
        {
            if(!notified.exchange(true))
                waiting.set_value();
        });
        contender = std::async(std::launch::async, [&]() { return pool.Accept(tx); });
        fWaiting = waitFuture.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
        fMutexAvailable = pool.MUTEX.try_lock();
        if(fMutexAvailable)
        {
            fInserted = pool.AddUnchecked(competing);
            pool.MUTEX.unlock();
        }
    }
    const bool fAccepted = contender.get();
    LLD::SetTxnCoordinatorWaitHook({});

    REQUIRE(fWaiting);
    REQUIRE(fMutexAvailable);
    REQUIRE(fInserted);
    REQUIRE_FALSE(fAccepted);
    TAO::Register::State state;
    REQUIRE_FALSE(LLD::Register->ReadState(address, state, TAO::Ledger::FLAGS::MEMPOOL));
}

TEST_CASE("Mempool orphan drain validates detached entries without retaining its mutex",
          "[mempool][mempool_coordinator]")
{
    TAO::Ledger::Mempool pool;
    const TAO::Register::Address address(TAO::Register::Address::ACCOUNT);
    const uint512_t key = LLC::GetRand512();
    auto parent = CoordinatorTestTransaction(TAO::Register::Address(TAO::Register::Address::ACCOUNT));
    parent.NextHash(key);
    parent.hashCache = 0;
    REQUIRE(parent.Sign(LLC::GetRand512()));

    auto child = CoordinatorTestTransaction(address);
    child.hashGenesis = parent.hashGenesis;
    child.nSequence = 1;
    child.hashPrevTx = parent.GetHash();
    child.hashCache = 0;
    REQUIRE(child.Build());
    REQUIRE(child.Sign(key));
    REQUIRE_FALSE(pool.Accept(child));
    REQUIRE(pool.Has(child.GetHash()));
    REQUIRE(LLD::Ledger->WriteTx(parent.GetHash(), parent));
    REQUIRE(LLD::Ledger->WriteLast(parent.hashGenesis, parent.GetHash()));

    std::promise<void> waiting;
    auto waitFuture = waiting.get_future();
    std::atomic<bool> notified{false};
    std::future<void> contender;
    bool fWaiting = false;
    bool fMutexAvailable = false;
    {
        LLD::TransactionCoordinatorGuard coordinator;
        LLD::SetTxnCoordinatorWaitHook([&]()
        {
            if(!notified.exchange(true))
                waiting.set_value();
        });
        contender = std::async(std::launch::async, [&]() { pool.ProcessOrphans(parent.GetHash()); });
        fWaiting = waitFuture.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
        fMutexAvailable = pool.MUTEX.try_lock();
        if(fMutexAvailable)
            pool.MUTEX.unlock();
    }
    contender.get();
    LLD::SetTxnCoordinatorWaitHook({});
    TAO::Register::State state;
    const bool fState = LLD::Register->ReadState(address, state, TAO::Ledger::FLAGS::MEMPOOL);
    std::vector<uint512_t> hashes;
    pool.List(hashes);
    LLD::Ledger->EraseLast(parent.hashGenesis);
    LLD::Ledger->EraseTx(parent.GetHash());
    LLD::Register->EraseState(address, TAO::Ledger::FLAGS::MEMPOOL);

    REQUIRE(fWaiting);
    REQUIRE(fMutexAvailable);
    REQUIRE(fState);
    REQUIRE(std::find(hashes.begin(), hashes.end(), child.GetHash()) != hashes.end());
}

TEST_CASE("Mempool revalidates a disk tip changed while admission waits",
          "[mempool][mempool_coordinator]")
{
    TAO::Ledger::Mempool pool;
    const TAO::Register::Address address(TAO::Register::Address::ACCOUNT);
    const auto parent = CoordinatorTestTransaction(TAO::Register::Address(TAO::Register::Address::ACCOUNT));
    auto tx = CoordinatorTestTransaction(address);
    tx.hashGenesis = parent.hashGenesis;
    tx.nSequence = 1;
    tx.hashPrevTx = parent.GetHash();
    tx.hashCache = 0;
    REQUIRE(tx.Build());
    REQUIRE(tx.Sign(LLC::GetRand512()));
    REQUIRE(LLD::Ledger->WriteTx(parent.GetHash(), parent));
    REQUIRE(LLD::Ledger->WriteLast(parent.hashGenesis, parent.GetHash()));

    std::promise<void> waiting;
    auto waitFuture = waiting.get_future();
    std::atomic<bool> notified{false};
    std::future<bool> contender;
    bool fWaiting = false;
    bool fUpdated = false;
    {
        LLD::TransactionCoordinatorGuard coordinator;
        LLD::SetTxnCoordinatorWaitHook([&]()
        {
            if(!notified.exchange(true))
                waiting.set_value();
        });
        contender = std::async(std::launch::async, [&]() { return pool.Accept(tx); });
        fWaiting = waitFuture.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
        fUpdated = LLD::Ledger->WriteLast(parent.hashGenesis, LLC::GetRand512());
    }
    const bool fAccepted = contender.get();
    LLD::SetTxnCoordinatorWaitHook({});
    LLD::Ledger->EraseLast(parent.hashGenesis);
    LLD::Ledger->EraseTx(parent.GetHash());

    REQUIRE(fWaiting);
    REQUIRE(fUpdated);
    REQUIRE_FALSE(fAccepted);
    REQUIRE(pool.Conflicts() == 1);
    TAO::Register::State state;
    REQUIRE_FALSE(LLD::Register->ReadState(address, state, TAO::Ledger::FLAGS::MEMPOOL));
}

TEST_CASE("Mempool Check re-admits resolved roots and drains real conflict dependents",
          "[mempool][mempool_coordinator]")
{
    auto& pool = TAO::Ledger::mempool;
    const TAO::Register::Address childAddress(TAO::Register::Address::ACCOUNT);
    const TAO::Register::Address tailAddress(TAO::Register::Address::ACCOUNT);
    const uint512_t childKey = LLC::GetRand512();
    const uint512_t tailKey = LLC::GetRand512();
    auto parent = CoordinatorTestTransaction(TAO::Register::Address(TAO::Register::Address::ACCOUNT));
    parent.NextHash(childKey);
    parent.hashCache = 0;
    REQUIRE(parent.Sign(LLC::GetRand512()));
    auto child = CoordinatorTestTransaction(childAddress);
    child.hashGenesis = parent.hashGenesis;
    child.nSequence = 1;
    child.hashPrevTx = parent.GetHash();
    child.NextHash(tailKey);
    child.hashCache = 0;
    REQUIRE(child.Build());
    REQUIRE(child.Sign(childKey));
    auto tail = CoordinatorTestTransaction(tailAddress);
    tail.hashGenesis = parent.hashGenesis;
    tail.nSequence = 2;
    tail.hashPrevTx = child.GetHash();
    tail.hashCache = 0;
    REQUIRE(tail.Build());
    REQUIRE(tail.Sign(tailKey));

    REQUIRE(LLD::Ledger->WriteTx(parent.GetHash(), parent));
    REQUIRE(LLD::Ledger->WriteLast(parent.hashGenesis, LLC::GetRand512()));
    REQUIRE_FALSE(pool.Accept(child));
    REQUIRE_FALSE(pool.Accept(tail));
    REQUIRE(LLD::Ledger->WriteLast(parent.hashGenesis, parent.GetHash()));
    pool.Check();

    TAO::Register::State state;
    const bool fChildState = LLD::Register->ReadState(childAddress, state, TAO::Ledger::FLAGS::MEMPOOL);
    const bool fTailState = LLD::Register->ReadState(tailAddress, state, TAO::Ledger::FLAGS::MEMPOOL);
    std::vector<uint512_t> hashes;
    pool.List(hashes);
    pool.Remove(tail.GetHash());
    pool.Remove(child.GetHash());
    LLD::Register->EraseState(childAddress, TAO::Ledger::FLAGS::MEMPOOL);
    LLD::Register->EraseState(tailAddress, TAO::Ledger::FLAGS::MEMPOOL);
    LLD::Ledger->EraseLast(parent.hashGenesis);
    LLD::Ledger->EraseTx(parent.GetHash());

    REQUIRE(fChildState);
    REQUIRE(fTailState);
    REQUIRE(std::find(hashes.begin(), hashes.end(), child.GetHash()) != hashes.end());
    REQUIRE(std::find(hashes.begin(), hashes.end(), tail.GetHash()) != hashes.end());
}

TEST_CASE( "Mempool and memory sequencing tests", "[mempool]")
{
    using namespace TAO::Register;
    using namespace TAO::Operation;

    /* Need to clear mempool in case other unit tests have added transactions.  This allows us to test the sequencing without
       having our tests affected by other transactions outside of this test. */
    std::vector<uint512_t> vExistingHashes;
    TAO::Ledger::mempool.List(vExistingHashes);

    /* Iterate existing mempool tx list and remove them all */
    for(auto& hash : vExistingHashes)
    {
        REQUIRE(TAO::Ledger::mempool.Remove(hash));
    }

    //create a list of transactions
    {

        //create object
        uint256_t hashGenesis   = TAO::Ledger::Credentials::Genesis("testuser");
        uint512_t hashPrivKey1  = LLC::GetRand512();
        uint512_t hashPrivKey2  = LLC::GetRand512();

        uint512_t hashPrevTx;

        TAO::Register::Address hashToken     = TAO::Register::Address(TAO::Register::Address::TOKEN);
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //hybrid data
            const std::string strHybrid = config::GetArg("-hybrid", "");
            tx.hashPrevTx = LLC::SK512(strHybrid.begin(), strHybrid.end());

            //create object
            Object token = CreateToken(hashToken, 1000, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx = tx.GetHash();
        }

        //set address
        TAO::Register::Address hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        {
            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object account = CreateAccount(hashToken);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx = tx.GetHash();
        }

        //set address
        TAO::Register::Address hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);
        {


            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("token") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashAddress, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint8_t>("byte") == uint8_t(55));
            REQUIRE(object2.get<std::string>("test") == std::string("this string"));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 3;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create an operation stream to set values.
            TAO::Operation::Stream stream;
            stream << std::string("test") << uint8_t(OP::TYPES::STRING) << std::string("stRInGISNew");

            //payload
            tx[0] << uint8_t(OP::WRITE) << hashAddress << stream.Bytes();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashAddress, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint8_t>("byte") == uint8_t(55));
            REQUIRE(object2.get<std::string>("test") == std::string("stRInGISNew"));

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 4;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create an operation stream to set values.
            TAO::Operation::Stream stream;
            stream << std::string("byte") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(13);

            //payload
            tx[0] << uint8_t(OP::WRITE) << hashAddress << stream.Bytes();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashAddress, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint8_t>("byte") == uint8_t(13));
            REQUIRE(object2.get<std::string>("test") == std::string("stRInGISNew"));

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 5;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(100) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 900);

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 6;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(500) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 400);

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 7;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(300) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 100);

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        //test a failure
        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 8;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(300) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE_FALSE(tx.Build());
        }



        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 8;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(100) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 0);

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {
            //check mempool list sequencing
            std::vector<uint512_t> vHashes;
            REQUIRE(TAO::Ledger::mempool.List(vHashes));

            //commit memory pool transactions to disk
            for(auto& hash : vHashes)
            {
                TAO::Ledger::Transaction tx;
                REQUIRE(TAO::Ledger::mempool.Get(hash, tx));

                LLD::Ledger->WriteTx(hash, tx);

                REQUIRE(tx.Verify());
                REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
                REQUIRE(TAO::Ledger::mempool.Remove(tx.GetHash()));
            }

            //check token balance with mempool flag on
            {
                TAO::Register::Object object2;
                REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

                //parse
                REQUIRE(object2.Parse());

                //check values
                REQUIRE(object2.get<uint64_t>("balance") == 0);
            }


            //check token balance from disk only
            {
                TAO::Register::Object object2;
                REQUIRE(LLD::Register->ReadState(hashToken, object2));

                //parse
                REQUIRE(object2.Parse());

                //check values
                REQUIRE(object2.get<uint64_t>("balance") == 0);
            }
        }
    }








    //handle out of order transactions
    {

        //vector to shuffle
        std::vector<TAO::Ledger::Transaction> vTX;

        //create object
        uint256_t hashGenesis   = TAO::Ledger::Credentials::Genesis("testuser");
        uint512_t hashPrivKey1  = LLC::GetRand512();
        uint512_t hashPrivKey2  = LLC::GetRand512();

        uint512_t hashPrevTx;

        TAO::Register::Address hashToken = TAO::Register::Address(TAO::Register::Address::TOKEN);
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //hybrid data
            const std::string strHybrid = config::GetArg("-hybrid", "");
            tx.hashPrevTx = LLC::SK512(strHybrid.begin(), strHybrid.end());

            //create object
            Object token = CreateToken(hashToken, 1000, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }

        //set address
        TAO::Register::Address hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        {


            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object account = CreateAccount(hashToken);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }

        //set address
        TAO::Register::Address hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);
        {


            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("token") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 3;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("token") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 4;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("token") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 5;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("token") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 6;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("token") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 7;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("token") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }



        //set new address
        hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 8;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("token") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        //set new address
        hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 9;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("token") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //set previous
            hashPrevTx = tx.GetHash();

            //push to vector
            vTX.push_back(tx);
        }


        {
            //random shuffle the list for sequencing
            LLC::random_shuffle(vTX.begin(), vTX.end());

            //accept all transactions in random ordering
            for(auto& tx : vTX)
            {
                /* Has also includes queued orphans; only a live predecessor
                 * allows immediate admission rather than further queueing. */
                TAO::Ledger::Transaction txPrev;
                bool fConflicted = false;
                const bool fLivePrev = TAO::Ledger::mempool.Get(tx.hashPrevTx, txPrev, fConflicted)
                    && !fConflicted;
                if(!fLivePrev && tx.nSequence != 0)
                {
                    REQUIRE_FALSE(TAO::Ledger::mempool.Accept(tx));
                }
                else //the first one (will trigger orphan processing), and any other non-ORPHAN should always pass
                {
                    REQUIRE(TAO::Ledger::mempool.Accept(tx));
                }
            }

            /* Every queued orphan must be admitted once its predecessor arrives. */
            for(const auto& tx : vTX)
                REQUIRE(TAO::Ledger::mempool.Has(tx.GetHash()));

            //check mempool list sequencing
            std::vector<uint512_t> vHashes;
            REQUIRE(TAO::Ledger::mempool.List(vHashes));

            //commit memory pool transactions to disk
            for(auto& hash : vHashes)
            {
                TAO::Ledger::Transaction tx;
                REQUIRE(TAO::Ledger::mempool.Get(hash, tx));

                LLD::Ledger->WriteTx(hash, tx);
                REQUIRE(tx.Verify());
                REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
                REQUIRE(TAO::Ledger::mempool.Remove(tx.GetHash()));
            }
        }
    }



    //create a list of transactions
    {

        //create object
        uint256_t hashGenesis   = TAO::Ledger::Credentials::Genesis("testuser");
        uint512_t hashPrivKey1  = LLC::GetRand512();
        uint512_t hashPrivKey2  = LLC::GetRand512();

        uint512_t hashPrevTx;

        TAO::Register::Address hashToken     = TAO::Register::Address(TAO::Register::Address::TOKEN);
        {
            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 0;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //hybrid data
            const std::string strHybrid = config::GetArg("-hybrid", "");
            tx.hashPrevTx = LLC::SK512(strHybrid.begin(), strHybrid.end());

            //create object
            Object token = CreateToken(hashToken, 1000, 100);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashToken << uint8_t(REGISTER::OBJECT) << token.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx = tx.GetHash();
        }

        //set address
        TAO::Register::Address hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);
        {


            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 1;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object account = CreateAccount(hashToken);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAccount << uint8_t(REGISTER::OBJECT) << account.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx = tx.GetHash();
        }

        //set address
        TAO::Register::Address hashAddress = TAO::Register::Address(TAO::Register::Address::OBJECT);
        {


            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 2;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("token") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashAddress, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint8_t>("byte") == uint8_t(55));
            REQUIRE(object2.get<std::string>("test") == std::string("this string"));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 3;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create an operation stream to set values.
            TAO::Operation::Stream stream;
            stream << std::string("test") << uint8_t(OP::TYPES::STRING) << std::string("stRInGISNew");

            //payload
            tx[0] << uint8_t(OP::WRITE) << hashAddress << stream.Bytes();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashAddress, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint8_t>("byte") == uint8_t(55));
            REQUIRE(object2.get<std::string>("test") == std::string("stRInGISNew"));

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 4;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create an operation stream to set values.
            TAO::Operation::Stream stream;
            stream << std::string("byte") << uint8_t(OP::TYPES::UINT8_T) << uint8_t(13);

            //payload
            tx[0] << uint8_t(OP::WRITE) << hashAddress << stream.Bytes();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashAddress, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint8_t>("byte") == uint8_t(13));
            REQUIRE(object2.get<std::string>("test") == std::string("stRInGISNew"));

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        std::vector<uint512_t> vOrphans;
        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 5;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(100) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 900);

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();

            //add to queue
            vOrphans.push_back(hashPrevTx);
        }




        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 6;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(500) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 400);

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();

            //add to queue
            vOrphans.push_back(hashPrevTx);
        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 7;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(300) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 100);

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();

            //add to queue
            vOrphans.push_back(hashPrevTx);
        }


        //test a failure
        {

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 8;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(300) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE_FALSE(tx.Build());
        }



        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 8;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(100) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 0);

            //make sure not on disk
            REQUIRE_FALSE(LLD::Register->ReadState(hashAddress, object2));

            //set previous
            hashPrevTx = tx.GetHash();

            //add to queue
            vOrphans.push_back(hashPrevTx);
        }


        {
            //check mempool list sequencing
            std::vector<uint512_t> vHashes;
            REQUIRE(TAO::Ledger::mempool.List(vHashes));

            //commit memory pool transactions to disk
            for(uint32_t i = 0; i < 6; ++i)
            {
                uint512_t hash = vHashes[i];

                TAO::Ledger::Transaction tx;
                REQUIRE(TAO::Ledger::mempool.Get(hash, tx));

                REQUIRE(LLD::Ledger->WriteTx(hash, tx));
                REQUIRE(LLD::Ledger->WriteLast(hashGenesis, hash));

                REQUIRE(tx.Verify());
                REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));
                REQUIRE(TAO::Ledger::mempool.Remove(tx.GetHash()));
            }

            //check token balance with mempool flag on
            {
                TAO::Register::Object object2;
                REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

                //parse
                REQUIRE(object2.Parse());

                //check values
                REQUIRE(object2.get<uint64_t>("balance") == 0);
            }


            //check token balance from disk only
            {
                TAO::Register::Object object2;
                REQUIRE(LLD::Register->ReadState(hashToken, object2));

                //parse
                REQUIRE(object2.Parse());

                //check values
                REQUIRE(object2.get<uint64_t>("balance") == 900);
            }

        }


        {

            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            TAO::Register::Address hashAddress2 = TAO::Register::Address(TAO::Register::Address::OBJECT);

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 6;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //create object
            Object object;
            object << std::string("byte") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT8_T) << uint8_t(55)
                   << std::string("test") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::STRING) << std::string("this string")
                   << std::string("token") << uint8_t(TAO::Register::TYPES::UINT256_T) << uint256_t(0);

            //payload
            tx[0] << uint8_t(OP::CREATE) << hashAddress2 << uint8_t(REGISTER::OBJECT) << object.GetState();

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            uint512_t hash = tx.GetHash();

            REQUIRE(LLD::Ledger->WriteTx(hash, tx));
            REQUIRE(LLD::Ledger->WriteLast(hashGenesis, hash));

            REQUIRE(tx.Verify());
            REQUIRE(Execute(tx[0], TAO::Ledger::FLAGS::BLOCK));

            hashPrevTx = tx.GetHash();
        }


        {

            //run unit test for the check function
            TAO::Ledger::mempool.Check();

            //check token balance with mempool flag on
            {
                TAO::Register::Object object2;
                REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

                //parse
                REQUIRE(object2.Parse());

                //check values
                REQUIRE(object2.get<uint64_t>("balance") == 900);
            }

            //run unit test for the check function
            TAO::Ledger::mempool.Check();

            //check that the right transactions were removed
            for(const auto& orphan : vOrphans)
            {
                REQUIRE_FALSE(TAO::Ledger::mempool.Has(orphan));
            }

        }


        {
            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 7;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(300) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.Accept(tx));

            //check values all match
            TAO::Register::Object object2;
            REQUIRE(LLD::Register->ReadState(hashToken, object2, TAO::Ledger::FLAGS::MEMPOOL));

            //parse
            REQUIRE(object2.Parse());

            //check values
            REQUIRE(object2.get<uint64_t>("balance") == 600);

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {
            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 8;
            tx.hashPrevTx  = 1;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(100) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE(TAO::Ledger::mempool.AddUnchecked(tx));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        {
            //set private keys
            hashPrivKey1 = hashPrivKey2;
            hashPrivKey2 = LLC::GetRand512();

            //create the transaction object
            TAO::Ledger::Transaction tx;
            tx.hashGenesis = hashGenesis;
            tx.nSequence   = 9;
            tx.hashPrevTx  = hashPrevTx;
            tx.nTimestamp  = runtime::timestamp();
            tx.nKeyType    = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.nNextType   = TAO::Ledger::SIGNATURE::BRAINPOOL;
            tx.NextHash(hashPrivKey2);

            //payload
            tx[0] << uint8_t(OP::DEBIT) << hashToken << hashAccount << uint64_t(100) << uint64_t(0);

            //generate the prestates and poststates
            REQUIRE(tx.Build());

            //sign
            tx.Sign(hashPrivKey1);

            //commit to disk
            REQUIRE_FALSE(TAO::Ledger::mempool.Accept(tx));

            //set previous
            hashPrevTx = tx.GetHash();
        }


        //run unit test for the check function
        TAO::Ledger::mempool.Check();
    }
}
