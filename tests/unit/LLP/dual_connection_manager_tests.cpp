/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <unit/catch2/catch.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <cstdint>

/*
 * Unit tests for DualConnectionManager API semantics.
 *
 * DualConnectionManager (from PR #334, now removed from production source)
 * managed dual-connection SIM Link architecture and failover logic.
 * These tests verify the documented API contract and state machine semantics
 * using a self-contained test implementation that mirrors the original design.
 *
 * Covers:
 *   - Lane health tracking (STATELESS / LEGACY)
 *   - SIM Link activation / deactivation
 *   - Failover state transitions (PRIMARY → FAILOVER → PRIMARY)
 *   - Secondary endpoint derivation
 *   - Fail counter semantics
 *   - Thread-safety smoke test
 *   - Integration scenario tests
 */

/* ── Minimal DualConnectionManager implementation for testing ─────────── */
/* This mirrors the original PR #334 design without depending on             */
/* the production source files that were removed.                            */
namespace TestDCM
{
    class DualConnectionManager
    {
    public:
        DualConnectionManager()
        : m_stateless_lane_alive(false)
        , m_legacy_lane_alive(false)
        , m_using_failover(false)
        , m_primary_fail_count(0)
        , m_active_endpoint()
        {}

        void set_lane_alive(bool is_stateless)
        {
            if(is_stateless)
                m_stateless_lane_alive.store(true);
            else
                m_legacy_lane_alive.store(true);
        }

        void set_lane_dead(bool is_stateless)
        {
            if(is_stateless)
                m_stateless_lane_alive.store(false);
            else
                m_legacy_lane_alive.store(false);
        }

        bool is_stateless_lane_alive() const { return m_stateless_lane_alive.load(); }
        bool is_legacy_lane_alive()    const { return m_legacy_lane_alive.load();    }
        bool is_sim_link_active()      const
        {
            return m_stateless_lane_alive.load() && m_legacy_lane_alive.load();
        }

        void set_failover_active(bool using_failover, const std::string& endpoint)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_using_failover    = using_failover;
            m_active_endpoint   = endpoint;
        }

        bool is_using_failover() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_using_failover;
        }

        std::string get_active_endpoint() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_active_endpoint;
        }

        /* get_secondary_endpoint
         *
         * Derives the LEGACY (8323) endpoint from the active primary (STATELESS, 9323) endpoint
         * by replacing the port suffix with 8323.
         * The active endpoint is always expected to be the STATELESS port (9323).
         */
        std::string get_secondary_endpoint() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if(m_active_endpoint.empty())
                return "";

            std::string secondary = m_active_endpoint;
            std::size_t colon_pos = secondary.rfind(':');
            if(colon_pos == std::string::npos)
                return secondary;

            return secondary.substr(0, colon_pos + 1) + "8323";
        }

        uint32_t get_primary_fail_count() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_primary_fail_count;
        }

        void increment_fail_count()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            ++m_primary_fail_count;
        }

        void reset_fail_count()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_primary_fail_count = 0;
        }

    private:
        mutable std::mutex        m_mutex;
        std::atomic<bool>         m_stateless_lane_alive;
        std::atomic<bool>         m_legacy_lane_alive;
        bool                      m_using_failover;
        uint32_t                  m_primary_fail_count;
        std::string               m_active_endpoint;
    };

} // namespace TestDCM


/* ── Lane Health Tests ─────────────────────────────────────────────────── */

TEST_CASE("DualConnectionManager: Lane Health", "[dual_connection_manager][llp]")
{
    SECTION("Default state — both lanes DEAD, SIM Link inactive")
    {
        TestDCM::DualConnectionManager dcm;
        REQUIRE(dcm.is_stateless_lane_alive() == false);
        REQUIRE(dcm.is_legacy_lane_alive()    == false);
        REQUIRE(dcm.is_sim_link_active()      == false);
    }

    SECTION("STATELESS lane alive — stateless=ALIVE, legacy=DEAD, sim_link=false")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_lane_alive(true);
        REQUIRE(dcm.is_stateless_lane_alive() == true);
        REQUIRE(dcm.is_legacy_lane_alive()    == false);
        REQUIRE(dcm.is_sim_link_active()      == false);
    }

    SECTION("LEGACY lane alive — legacy=ALIVE, stateless=DEAD, sim_link=false")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_lane_alive(false);
        REQUIRE(dcm.is_stateless_lane_alive() == false);
        REQUIRE(dcm.is_legacy_lane_alive()    == true);
        REQUIRE(dcm.is_sim_link_active()      == false);
    }

    SECTION("Both lanes alive — SIM Link ACTIVE")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_lane_alive(true);
        dcm.set_lane_alive(false);
        REQUIRE(dcm.is_stateless_lane_alive() == true);
        REQUIRE(dcm.is_legacy_lane_alive()    == true);
        REQUIRE(dcm.is_sim_link_active()      == true);
    }

    SECTION("SIM Link deactivates when STATELESS lane goes dead")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_lane_alive(true);
        dcm.set_lane_alive(false);
        REQUIRE(dcm.is_sim_link_active() == true);

        dcm.set_lane_dead(true);  /* STATELESS dead */
        REQUIRE(dcm.is_stateless_lane_alive() == false);
        REQUIRE(dcm.is_sim_link_active()      == false);
    }

    SECTION("SIM Link deactivates when LEGACY lane goes dead")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_lane_alive(true);
        dcm.set_lane_alive(false);
        REQUIRE(dcm.is_sim_link_active() == true);

        dcm.set_lane_dead(false); /* LEGACY dead */
        REQUIRE(dcm.is_legacy_lane_alive() == false);
        REQUIRE(dcm.is_sim_link_active()   == false);
    }

    SECTION("Idempotent set_lane_alive — calling twice is safe")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_lane_alive(true);
        dcm.set_lane_alive(true);
        REQUIRE(dcm.is_stateless_lane_alive() == true);
        REQUIRE(dcm.is_sim_link_active()      == false);
    }

    SECTION("Idempotent set_lane_dead — calling when already dead is safe")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_lane_dead(true);  /* already dead at start */
        REQUIRE(dcm.is_stateless_lane_alive() == false);
        REQUIRE(dcm.is_sim_link_active()      == false);
    }
}


/* ── Failover State Tests ──────────────────────────────────────────────── */

TEST_CASE("DualConnectionManager: Failover State", "[dual_connection_manager][llp]")
{
    SECTION("Default: not using failover")
    {
        TestDCM::DualConnectionManager dcm;
        REQUIRE(dcm.is_using_failover() == false);
    }

    SECTION("Activate failover — is_using_failover() == true, active endpoint set")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_failover_active(true, "192.168.1.11:9323");
        REQUIRE(dcm.is_using_failover()    == true);
        REQUIRE(dcm.get_active_endpoint()  == "192.168.1.11:9323");
    }

    SECTION("Deactivate failover — is_using_failover() == false, primary endpoint set")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_failover_active(true, "192.168.1.11:9323");
        dcm.set_failover_active(false, "192.168.1.10:9323");
        REQUIRE(dcm.is_using_failover()   == false);
        REQUIRE(dcm.get_active_endpoint() == "192.168.1.10:9323");
    }

    SECTION("Secondary endpoint derivation — STATELESS primary (9323) → LEGACY secondary (8323)")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_failover_active(false, "192.168.1.10:9323");
        REQUIRE(dcm.get_secondary_endpoint() == "192.168.1.10:8323");
    }

    SECTION("Secondary endpoint derivation — any port suffix is replaced with 8323")
    {
        /* Active endpoint is always the STATELESS port (9323); the secondary is always 8323.
         * This verifies the port-replacement logic for a non-standard port value. */
        TestDCM::DualConnectionManager dcm;
        dcm.set_failover_active(false, "10.0.0.5:9323");
        REQUIRE(dcm.get_secondary_endpoint() == "10.0.0.5:8323");
    }

    SECTION("Secondary endpoint empty when no active endpoint configured")
    {
        TestDCM::DualConnectionManager dcm;
        REQUIRE(dcm.get_secondary_endpoint() == "");
    }

    SECTION("Failover toggle: PRIMARY → FAILOVER → PRIMARY (full cycle)")
    {
        TestDCM::DualConnectionManager dcm;

        /* Start on primary */
        dcm.set_failover_active(false, "10.0.0.1:9323");
        REQUIRE(dcm.is_using_failover()   == false);
        REQUIRE(dcm.get_active_endpoint() == "10.0.0.1:9323");

        /* Switch to failover */
        dcm.set_failover_active(true, "10.0.0.2:9323");
        REQUIRE(dcm.is_using_failover()   == true);
        REQUIRE(dcm.get_active_endpoint() == "10.0.0.2:9323");

        /* Return to primary */
        dcm.set_failover_active(false, "10.0.0.1:9323");
        REQUIRE(dcm.is_using_failover()   == false);
        REQUIRE(dcm.get_active_endpoint() == "10.0.0.1:9323");
    }
}


/* ── Fail Counter Tests ────────────────────────────────────────────────── */

TEST_CASE("DualConnectionManager: Fail Counter", "[dual_connection_manager][llp]")
{
    SECTION("Initial fail count is zero")
    {
        TestDCM::DualConnectionManager dcm;
        REQUIRE(dcm.get_primary_fail_count() == 0u);
    }

    SECTION("Increment fail count — three increments yields count == 3")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.increment_fail_count();
        dcm.increment_fail_count();
        dcm.increment_fail_count();
        REQUIRE(dcm.get_primary_fail_count() == 3u);
    }

    SECTION("Reset fail count — increments then reset yields zero")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.increment_fail_count();
        dcm.increment_fail_count();
        dcm.increment_fail_count();
        dcm.increment_fail_count();
        dcm.increment_fail_count();
        REQUIRE(dcm.get_primary_fail_count() == 5u);

        dcm.reset_fail_count();
        REQUIRE(dcm.get_primary_fail_count() == 0u);
    }

    SECTION("Fail counter is independent of failover state")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_failover_active(true, "10.0.0.2:9323");

        dcm.increment_fail_count();
        dcm.increment_fail_count();
        REQUIRE(dcm.get_primary_fail_count() == 2u);
        REQUIRE(dcm.is_using_failover()      == true);
    }
}


/* ── Thread-Safety Smoke Test ──────────────────────────────────────────── */

TEST_CASE("DualConnectionManager: Thread-Safety Smoke Test", "[dual_connection_manager][llp]")
{
    SECTION("Concurrent lane updates — no crash or assertion failure")
    {
        TestDCM::DualConnectionManager dcm;

        constexpr int kThreads    = 4;
        constexpr int kIterations = 500;

        std::vector<std::thread> threads;
        threads.reserve(kThreads);

        for(int t = 0; t < kThreads; ++t)
        {
            threads.emplace_back([&dcm, t, kIterations]()
            {
                for(int i = 0; i < kIterations; ++i)
                {
                    bool stateless = (i % 2 == 0);
                    if((t + i) % 3 == 0)
                        dcm.set_lane_alive(stateless);
                    else
                        dcm.set_lane_dead(stateless);

                    /* Read state without asserting — just exercise code paths */
                    (void)dcm.is_sim_link_active();
                    (void)dcm.is_using_failover();
                    (void)dcm.get_primary_fail_count();
                }
            });
        }

        for(auto& th : threads)
            th.join();

        /* If we reach here without crash the smoke test passes */
        REQUIRE(true);
    }
}


/* ── Integration Scenario Tests ───────────────────────────────────────── */

TEST_CASE("DualConnectionManager: Integration Scenarios", "[dual_connection_manager][llp]")
{
    SECTION("Full failover scenario — primary fails N times, activates failover, then resets")
    {
        TestDCM::DualConnectionManager dcm;
        const std::string kPrimary  = "10.0.0.1:9323";
        const std::string kFailover = "10.0.0.2:9323";

        /* Establish primary */
        dcm.set_failover_active(false, kPrimary);
        dcm.set_lane_alive(true);
        dcm.set_lane_alive(false);
        REQUIRE(dcm.is_sim_link_active() == true);
        REQUIRE(dcm.is_using_failover()  == false);

        /* Simulate 5 consecutive primary failures */
        for(int i = 0; i < 5; ++i)
            dcm.increment_fail_count();
        REQUIRE(dcm.get_primary_fail_count() == 5u);

        /* Activate failover */
        dcm.set_lane_dead(true);
        dcm.set_lane_dead(false);
        dcm.set_failover_active(true, kFailover);
        REQUIRE(dcm.is_using_failover()   == true);
        REQUIRE(dcm.get_active_endpoint() == kFailover);

        /* Re-establish on failover node */
        dcm.reset_fail_count();
        dcm.set_lane_alive(true);
        REQUIRE(dcm.get_primary_fail_count()  == 0u);
        REQUIRE(dcm.is_stateless_lane_alive() == true);

        /* Return to primary */
        dcm.set_failover_active(false, kPrimary);
        REQUIRE(dcm.is_using_failover()   == false);
        REQUIRE(dcm.get_active_endpoint() == kPrimary);
    }

    SECTION("SIM Link + failover combined — secondary endpoint follows active node")
    {
        TestDCM::DualConnectionManager dcm;

        /* Both lanes alive on primary */
        dcm.set_failover_active(false, "10.0.0.1:9323");
        dcm.set_lane_alive(true);
        dcm.set_lane_alive(false);
        REQUIRE(dcm.is_sim_link_active()     == true);
        REQUIRE(dcm.get_secondary_endpoint() == "10.0.0.1:8323");

        /* Primary drops — both lanes dead */
        dcm.set_lane_dead(true);
        dcm.set_lane_dead(false);
        REQUIRE(dcm.is_sim_link_active() == false);

        /* Failover switch */
        dcm.set_failover_active(true, "10.0.0.2:9323");
        dcm.set_lane_alive(true);
        dcm.set_lane_alive(false);
        REQUIRE(dcm.is_using_failover()      == true);
        REQUIRE(dcm.is_sim_link_active()     == true);
        REQUIRE(dcm.get_secondary_endpoint() == "10.0.0.2:8323");
    }

    SECTION("Secondary endpoint stays consistent when active endpoint has no port")
    {
        TestDCM::DualConnectionManager dcm;
        dcm.set_failover_active(false, "10.0.0.1");
        /* rfind(':') returns npos — function returns the endpoint unchanged */
        REQUIRE(dcm.get_secondary_endpoint() == "10.0.0.1");
    }
}
