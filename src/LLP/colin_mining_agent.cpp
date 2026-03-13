/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2025

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/colin_mining_agent.h>
#include <LLP/include/stateless_manager.h>
#include <LLP/include/session_recovery.h>
#include <LLP/include/failover_connection_tracker.h>

#include <TAO/Ledger/include/chainstate.h>
#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <iomanip>
#include <sstream>

namespace LLP
{

    /* ── Private helpers ─────────────────────────────────────────────────── */

    namespace
    {
        /** Format a steady_clock duration as "Xh Ym Zs" */
        std::string FormatUptime(std::chrono::steady_clock::time_point since)
        {
            using namespace std::chrono;
            auto elapsed = duration_cast<seconds>(steady_clock::now() - since).count();
            if(elapsed < 0) elapsed = 0;
            uint64_t h = elapsed / 3600;
            uint64_t m = (elapsed % 3600) / 60;
            uint64_t s = elapsed % 60;

            std::ostringstream oss;
            if(h > 0) oss << h << "h ";
            if(h > 0 || m > 0) oss << m << "m ";
            oss << s << "s";
            return oss.str();
        }

        /** Return channel name string */
        std::string ChannelName(uint32_t channel)
        {
            if(channel == 1) return "PRIME";
            if(channel == 2) return "HASH";
            return "UNKNOWN";
        }

        /** Box width for the Colin report */
        constexpr int BOX_WIDTH = 68;

        /** Print a padded report line: "║  <text>  ║" */
        std::string BoxLine(const std::string& text)
        {
            int pad = BOX_WIDTH - 2 - static_cast<int>(text.size());
            if(pad < 0) pad = 0;
            return std::string("║  ") + text + std::string(pad - 2, ' ') + std::string("║");
        }
    } // anonymous namespace


    /* ── PingFrame implementation ────────────────────────────────────────── */

    std::vector<uint8_t> PingFrame::Serialize() const
    {
        std::vector<uint8_t> v;
        v.reserve(64);

        auto push16 = [&](uint16_t x){
            v.push_back((x >> 8) & 0xFF);
            v.push_back( x       & 0xFF);
        };
        auto push32 = [&](uint32_t x){
            v.push_back((x >> 24) & 0xFF);
            v.push_back((x >> 16) & 0xFF);
            v.push_back((x >>  8) & 0xFF);
            v.push_back( x        & 0xFF);
        };
        auto push64 = [&](uint64_t x){
            push32(static_cast<uint32_t>(x >> 32));
            push32(static_cast<uint32_t>(x & 0xFFFFFFFF));
        };

        push16(opcode);
        push16(version);
        push32(sequence);
        push64(timestamp_send_us);
        push32(unified_height);
        push32(channel_height);
        push32(prime_pushes_30s);
        push32(hash_pushes_30s);
        push32(blocks_submitted);
        push32(blocks_accepted);
        push32(blocks_rejected);
        push32(get_block_count);
        v.push_back(health_flags);
        v.push_back(channel_id);
        push16(reserved);
        for(int i = 0; i < 12; ++i) v.push_back(0x00);  // padding

        assert(v.size() == 64);
        return v;
    }


    /* ── Singleton ───────────────────────────────────────────────────────── */

    ColinMiningAgent& ColinMiningAgent::Get()
    {
        static ColinMiningAgent instance;
        return instance;
    }


    /* ── Constructor / Destructor ─────────────────────────────────────────── */

    ColinMiningAgent::ColinMiningAgent()
    : m_mutex()
    , m_miners()
    , m_global()
    , m_interval_s(60)
    , m_report_thread()
    , m_stop(false)
    , m_running(false)
    {
    }


    ColinMiningAgent::~ColinMiningAgent()
    {
        stop();
    }


    /* ── Public API ──────────────────────────────────────────────────────── */

    void ColinMiningAgent::start()
    {
        /* Respect colin=0 config flag */
        if(!config::GetBoolArg("-colin", true))
        {
            debug::log(1, FUNCTION, "Colin mining agent disabled (colin=0)");
            return;
        }

        /* Already running guard */
        if(m_running.load())
            return;

        /* Read interval from config (default 60 s) */
        m_interval_s = static_cast<uint32_t>(config::GetArg("-colin_interval", 60));
        if(m_interval_s < 5)
            m_interval_s = 5;   // clamp minimum

        m_stop.store(false);
        m_running.store(true);

        m_report_thread = std::thread([this]{ run_report(); });

        debug::log(0, FUNCTION, "Colin mining agent started (interval=", m_interval_s, "s)");
    }


    void ColinMiningAgent::stop()
    {
        if(!m_running.load())
            return;

        m_stop.store(true);

        if(m_report_thread.joinable())
            m_report_thread.join();

        m_running.store(false);
        debug::log(1, FUNCTION, "Colin mining agent stopped");
    }


    void ColinMiningAgent::on_node_shutdown()
    {
        debug::log(0, FUNCTION, "Node shutdown — emitting final Colin report before miner disconnect");
        emit_report();
    }


    void ColinMiningAgent::on_block_submitted(const std::string& genesis_prefix,
                                              uint32_t channel,
                                              bool accepted,
                                              const std::string& rejection_reason)
    {
        (void)channel;  // reserved for future per-channel breakdown

        std::lock_guard<std::mutex> lock(m_mutex);

        auto& stats = m_miners[genesis_prefix];
        stats.genesis_prefix = genesis_prefix;
        stats.blocks_submitted++;
        if(accepted)
        {
            stats.blocks_accepted++;
        }
        else
        {
            stats.blocks_rejected++;
            stats.last_rejection_reason = rejection_reason;
        }
        stats.last_submit = std::chrono::steady_clock::now();
    }


    void ColinMiningAgent::on_template_pushed(uint32_t channel, uint32_t /*height*/)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if(channel == 1)
            m_global.prime_pushes++;
        else if(channel == 2)
            m_global.hash_pushes++;
    }


    void ColinMiningAgent::on_miner_connected(const std::string& genesis_prefix,
                                              const std::string& remote_endpoint)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto& stats = m_miners[genesis_prefix];
        stats.genesis_prefix   = genesis_prefix;
        stats.remote_endpoint  = remote_endpoint;
        stats.connection_count++;

        if(stats.connection_count == 1)
        {
            stats.connected_at = std::chrono::steady_clock::now();
            debug::log(0, FUNCTION, "[Mining LLP] Miner connected: genesis ", genesis_prefix,
                       " @ ", remote_endpoint);
        }
        else
        {
            /* SIM Link detected — same genesis on a second connection */
            debug::log(0, FUNCTION, "[Mining LLP] Dual-connection SIM Link detected: genesis ",
                       genesis_prefix, "... already has active session");
            debug::log(0, FUNCTION, "[Mining LLP] Allowing second connection (SIM Link / failover mode)");
            debug::log(0, FUNCTION, "[Mining LLP] Both connections will receive identical template pushes");
        }
    }


    void ColinMiningAgent::on_miner_disconnected(const std::string& genesis_prefix)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_miners.find(genesis_prefix);
        if(it == m_miners.end())
            return;

        if(it->second.connection_count > 0)
            it->second.connection_count--;

        debug::log(1, FUNCTION, "[Mining LLP] Miner disconnected: genesis ", genesis_prefix,
                   " (remaining connections: ", it->second.connection_count, ")");

        /* Remove from active miners once all connections drop */
        if(it->second.connection_count == 0)
            m_miners.erase(it);
    }


    void ColinMiningAgent::on_get_block_received(const std::string& genesis_prefix,
                                                  bool rate_limited)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto& stats = m_miners[genesis_prefix];
        stats.genesis_prefix = genesis_prefix;
        stats.get_block_count++;
        if(rate_limited)
            stats.get_block_rate_limited++;
    }


    void ColinMiningAgent::on_keepalive_ack(const std::string& genesis_prefix,
                                             uint32_t unified_height,
                                             uint32_t prime_height,
                                             uint32_t hash_height,
                                             uint32_t stake_height,
                                             uint32_t fork_score)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto& stats = m_miners[genesis_prefix];
        stats.genesis_prefix         = genesis_prefix;
        stats.keepalive_ack_count++;
        stats.last_unified_height    = unified_height;
        stats.last_prime_height      = prime_height;
        stats.last_hash_height       = hash_height;
        stats.last_stake_height      = stake_height;
        stats.last_fork_score        = fork_score;
        if(fork_score > stats.peak_fork_score)
            stats.peak_fork_score    = fork_score;
        stats.last_keepalive_ack_at  = std::chrono::steady_clock::now();
    }


    void ColinMiningAgent::on_pong_received(const std::string& genesis_prefix,
                                             const std::vector<uint8_t>& payload)
    {
        if(payload.size() < 64)
            return;  // Malformed — silently discard

        /* Parse echoed sequence from bytes [4-7] */
        uint32_t seq = (static_cast<uint32_t>(payload[4]) << 24)
                     | (static_cast<uint32_t>(payload[5]) << 16)
                     | (static_cast<uint32_t>(payload[6]) <<  8)
                     |  static_cast<uint32_t>(payload[7]);

        /* Parse echoed send timestamp from bytes [8-15] */
        uint64_t ts_send = 0;
        for(int i = 0; i < 8; ++i)
            ts_send = (ts_send << 8) | payload[8 + i];

        /* Compute RTT: now_us - ts_send */
        using namespace std::chrono;
        uint64_t now_us = static_cast<uint64_t>(
            duration_cast<microseconds>(
                system_clock::now().time_since_epoch()).count());
        uint64_t rtt_us = (now_us > ts_send) ? (now_us - ts_send) : 0;

        /* Parse miner fields from bytes [24-40] */
        auto rd32 = [&](int off) -> uint32_t {
            return (static_cast<uint32_t>(payload[off])   << 24)
                 | (static_cast<uint32_t>(payload[off+1]) << 16)
                 | (static_cast<uint32_t>(payload[off+2]) <<  8)
                 |  static_cast<uint32_t>(payload[off+3]);
        };

        uint32_t hash_rate   = rd32(24);
        uint32_t temp_cdeg   = rd32(28);
        uint32_t threads     = rd32(32);
        uint32_t queue_depth = rd32(36);
        uint8_t  mflags      = payload[40];

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_miners.find(genesis_prefix);
        if(it == m_miners.end())
            return;

        PongRecord& pr = it->second.last_pong;
        pr.rtt_prev_us         = pr.rtt_us;
        pr.rtt_us              = rtt_us;
        pr.sequence            = seq;
        pr.miner_hash_rate     = hash_rate;
        pr.miner_temp_cdeg     = temp_cdeg;
        pr.miner_threads       = threads;
        pr.miner_queue_depth   = queue_depth;
        pr.miner_health_flags  = mflags;
        pr.pong_received       = true;

        /* Update rolling RTT history */
        it->second.rtt_history.push_back(rtt_us);
        if(it->second.rtt_history.size() > RTT_HISTORY_SIZE)
            it->second.rtt_history.pop_front();

        /* React to miner health flags — log under lock since we have the miner id */
        const std::string& display_id = it->second.remote_endpoint.empty()
            ? genesis_prefix : it->second.remote_endpoint;

        if(mflags & PongRecord::MFLAG_HASH_RATE_ZERO)
            debug::log(0, FUNCTION, "[Colin] Miner ", display_id,
                       " reports ZERO hash rate \xe2\x80\x94 worker stalled");
        if(mflags & PongRecord::MFLAG_TEMP_CRITICAL)
            debug::log(0, FUNCTION, "[Colin] Miner ", display_id,
                       " CRITICAL TEMPERATURE: ", format_temp(temp_cdeg), " \xe2\x86\x90 CRITICAL");
        if(mflags & PongRecord::MFLAG_QUEUE_OVERFLOW)
            debug::log(0, FUNCTION, "[Colin] Miner ", display_id,
                       " queue overflow \xe2\x80\x94 depth: ", queue_depth);
        if(mflags & PongRecord::MFLAG_THREAD_ZERO)
            debug::log(0, FUNCTION, "[Colin] Miner ", display_id,
                       " reports zero active threads");
        if(mflags & PongRecord::MFLAG_LOW_ACCEPT_RATE)
            debug::log(0, FUNCTION, "[Colin] Miner ", display_id,
                       " low accept rate flagged");
    }


    void ColinMiningAgent::on_canonical_snap_updated(const std::string& genesis_prefix,
                                                     uint64_t snap_age_ms,
                                                     bool is_stale)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_miners.find(genesis_prefix);
        if(it == m_miners.end())
            return;

        it->second.last_canonical_snap_age_ms = snap_age_ms;
        it->second.last_canonical_snap_stale  = is_stale;

        if(is_stale)
        {
            const std::string& display_id = it->second.remote_endpoint.empty()
                ? genesis_prefix : it->second.remote_endpoint;
            debug::warning(FUNCTION, "[Colin] Miner ", display_id,
                           " canonical snapshot stale (", snap_age_ms, " ms) — template may be outdated");
        }
    }


    bool ColinMiningAgent::check_and_record_submission(uint32_t nHeight, uint64_t nNonce,
                                                        const std::string& merkleHex)
    {
        /* Compute a cheap dedup key from the three identity fields */
        size_t nKey = std::hash<uint32_t>{}(nHeight);
        nKey ^= std::hash<uint64_t>{}(nNonce)         << 1;
        nKey ^= std::hash<std::string>{}(merkleHex)   << 2;

        std::lock_guard<std::mutex> lock(m_dedup_mutex);
        auto tNow = std::chrono::steady_clock::now();

        /* Purge expired entries to keep the cache bounded */
        for(auto it = m_dedup_cache.begin(); it != m_dedup_cache.end(); )
        {
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                tNow - it->second).count();
            if(elapsedMs > static_cast<int64_t>(DEDUP_TTL_MS))
                it = m_dedup_cache.erase(it);
            else
                ++it;
        }

        /* Check for duplicate */
        if(m_dedup_cache.count(nKey))
            return true;   // duplicate

        /* Record first submission */
        m_dedup_cache[nKey] = tNow;
        return false;
    }


    void ColinMiningAgent::clear_dedup_cache()
    {
        std::lock_guard<std::mutex> lock(m_dedup_mutex);
        m_dedup_cache.clear();
    }


    /* ── Private implementation ──────────────────────────────────────────── */

    PingFrame ColinMiningAgent::build_ping_frame(const MinerStats& stats,
                                                  const GlobalStats& global,
                                                  uint32_t u_height,
                                                  uint32_t c_height,
                                                  uint8_t  channel_id) const
    {
        using namespace std::chrono;
        uint64_t now_us = static_cast<uint64_t>(
            duration_cast<microseconds>(
                system_clock::now().time_since_epoch()).count());

        PingFrame f;
        /* Use stateless opcode by default; connection layer may override for legacy */
        f.opcode             = 0xD0E0;
        f.version            = 0x0001;
        f.sequence           = stats.ping_sequence;
        f.timestamp_send_us  = now_us;
        f.unified_height     = u_height;
        f.channel_height     = c_height;
        f.prime_pushes_30s   = global.prime_pushes;
        f.hash_pushes_30s    = global.hash_pushes;
        f.blocks_submitted   = stats.blocks_submitted;
        f.blocks_accepted    = stats.blocks_accepted;
        f.blocks_rejected    = stats.blocks_rejected;
        f.get_block_count    = stats.get_block_count;
        f.channel_id         = channel_id;

        /* Build node-side health flags */
        uint8_t hf = 0;
        if(TAO::Ledger::ChainState::Synchronizing())
            hf |= PingFrame::FLAG_NODE_SYNCING;
        if(stats.connection_count > 1)
            hf |= PingFrame::FLAG_SIM_LINK_ACTIVE;
        if(stats.get_block_rate_limited > 0)
            hf |= PingFrame::FLAG_RATE_LIMITED;
        if(stats.blocks_submitted > 0)
        {
            uint32_t rej_pct = static_cast<uint32_t>(
                (static_cast<uint64_t>(stats.blocks_rejected) * 100) / stats.blocks_submitted);
            if(rej_pct > 20)
                hf |= PingFrame::FLAG_HIGH_REJECT_RATE;
        }
        if(stats.connection_count == 1 &&
           duration_cast<seconds>(steady_clock::now() - stats.connected_at).count() < 120)
            hf |= PingFrame::FLAG_FIRST_CONNECT;

        f.health_flags = hf;

        /* Self-validate: log node-side template drought if reporting zero pushes */
        if(f.prime_pushes_30s == 0)
            debug::log(0, FUNCTION, "[Colin] Node-side PRIME template drought "
                       "\xe2\x80\x94 zero pushes in 30s window");
        if(f.hash_pushes_30s == 0)
            debug::log(0, FUNCTION, "[Colin] Node-side HASH template drought "
                       "\xe2\x80\x94 zero pushes in 30s window");

        return f;
    }


    std::string ColinMiningAgent::format_rtt(uint64_t rtt_us)
    {
        std::ostringstream oss;
        if(rtt_us < 1000)
            oss << rtt_us << "\xC2\xB5s";
        else
            oss << std::fixed << std::setprecision(1)
                << (static_cast<double>(rtt_us) / 1000.0) << "ms";
        return oss.str();
    }


    std::string ColinMiningAgent::format_temp(uint32_t cdeg)
    {
        if(cdeg == 0) return "n/a";
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1)
            << (static_cast<double>(cdeg) / 10.0) << "\xC2\xB0" "C";
        return oss.str();
    }


    std::string ColinMiningAgent::format_miner_health(uint8_t flags)
    {
        if(flags == 0) return "\xE2\x9C\x93 OK";
        std::string out = "\xE2\x9A\xA0 ";
        bool first = true;
        auto add = [&](const char* s){ if(!first) out += " | "; out += s; first = false; };
        if(flags & PongRecord::MFLAG_HASH_RATE_ZERO)   add("HASH_RATE_ZERO");
        if(flags & PongRecord::MFLAG_TEMP_CRITICAL)    add("TEMP_CRITICAL");
        if(flags & PongRecord::MFLAG_QUEUE_OVERFLOW)   add("QUEUE_OVERFLOW");
        if(flags & PongRecord::MFLAG_THREAD_ZERO)      add("THREAD_ZERO");
        if(flags & PongRecord::MFLAG_HIGH_STALE_RATE)  add("HIGH_STALE_RATE");
        if(flags & PongRecord::MFLAG_LOW_ACCEPT_RATE)  add("LOW_ACCEPT_RATE");
        if(flags & PongRecord::MFLAG_RECONNECT_WINDOW) add("RECONNECT_WINDOW");
        if(flags & PongRecord::MFLAG_FIRST_PONG)       add("FIRST_PONG");
        return out;
    }


    std::string ColinMiningAgent::format_rtt_trend(const std::deque<uint64_t>& history)
    {
        if(history.size() < 2)
            return "";  // Insufficient data — caller should omit trend display

        /* Average the first half vs. second half to smooth noise */
        size_t mid = history.size() / 2;
        double avg_old = 0.0, avg_new = 0.0;
        for(size_t i = 0; i < mid; ++i)
            avg_old += static_cast<double>(history[i]);
        avg_old /= static_cast<double>(mid);
        for(size_t i = mid; i < history.size(); ++i)
            avg_new += static_cast<double>(history[i]);
        avg_new /= static_cast<double>(history.size() - mid);

        double delta_us = avg_new - avg_old;
        constexpr double STABLE_THRESHOLD_US = 500.0; // ±0.5ms = stable

        if(std::abs(delta_us) < STABLE_THRESHOLD_US)
            return "\xe2\x94\x80 stable";

        std::ostringstream oss;
        if(delta_us < 0)
        {
            oss << "\xe2\x96\xbc " << format_rtt(static_cast<uint64_t>(std::abs(delta_us)))
                << " improving";
        }
        else
        {
            oss << "\xe2\x96\xb2 +" << format_rtt(static_cast<uint64_t>(delta_us))
                << " degrading";
        }
        return oss.str();
    }


    void ColinMiningAgent::run_report()
    {
        while(!m_stop.load())
        {
            /* Sleep in small increments so we can respond to stop() promptly */
            for(uint32_t i = 0; i < m_interval_s * 10 && !m_stop.load(); ++i)
                runtime::sleep(100);

            if(!m_stop.load())
                emit_report();
        }

        /* Final report on shutdown */
        emit_report();
    }


    void ColinMiningAgent::emit_report()
    {
        std::map<std::string, MinerStats> miners_copy;
        GlobalStats global_copy;

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            /* Bump ping sequence for each miner before snapshot */
            for(auto& kv : m_miners)
                kv.second.ping_sequence++;

            miners_copy = m_miners;
            global_copy = m_global;

            /* Reset periodic counters */
            m_global.prime_pushes = 0;
            m_global.hash_pushes  = 0;
        }

        /* Timestamp — use localtime_r for thread safety on POSIX */
        std::time_t now_t = std::time(nullptr);
        char tbuf[32] = {};
        struct tm tm_buf = {};
        ::localtime_r(&now_t, &tm_buf);
        std::strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", &tm_buf);

        /* Border strings */
        const std::string top    = "╔" + std::string(BOX_WIDTH - 2, '═') + "╗";
        const std::string sep    = "╠" + std::string(BOX_WIDTH - 2, '═') + "╣";
        const std::string bottom = "╚" + std::string(BOX_WIDTH - 2, '═') + "╝";

        debug::log(0, top);
        debug::log(0, BoxLine(std::string("COLIN NODE MINING REPORT  [") + tbuf + "]"));
        debug::log(0, sep);

        debug::log(0, BoxLine(std::string("CONNECTED MINERS: ")
                              + std::to_string(miners_copy.size())));
        debug::log(0, sep);

        /* ── Miner Session State (StatelessMinerManager + SessionRecoveryManager) ─── */
        {
            std::vector<MiningContext> vMiners = StatelessMinerManager::Get().ListMiners();

            debug::log(0, BoxLine("MINER SESSION STATE:"));

            if(vMiners.empty())
            {
                debug::log(0, BoxLine("  No active miner sessions"));
            }
            else
            {
                for(const auto& ctx : vMiners)
                {
                    /* Use PeekSession (read-only) — NOT RecoverSessionByIdentity (consuming).
                     * Calling the consuming variant here would exhaust DEFAULT_MAX_RECONNECTS (10)
                     * in 10 report cycles, silently evicting the session and blocking real recovery.
                     * Bug documented in PR #375 Technical Error Report. */
                    /* Check fFreshAuth flag from SessionRecoveryData (set by MarkFreshAuth()
                     * after a confirmed failover Falcon handshake on this node). */
                    bool fFreshAuth = false;
                    auto optSession = SessionRecoveryManager::Get().PeekSession(ctx.hashKeyID);
                    if(optSession.has_value())
                        fFreshAuth = optSession->fFreshAuth;

                    std::string channel_name = (ctx.nChannel == 1) ? "Prime" :
                                               (ctx.nChannel == 2) ? "Hash"  : "Unknown";

                    std::ostringstream line;
                    line << "  Session 0x" << std::hex << std::setw(8) << std::setfill('0')
                         << ctx.nSessionId << std::dec
                         << " | addr=" << ctx.strAddress
                         << " | " << (ctx.fAuthenticated ? "Falcon auth OK" : "unauthenticated")
                         << " | channel=" << channel_name << " (" << ctx.nChannel << ")"
                         << " | fresh_auth=" << (fFreshAuth ? "true" : "false");

                    debug::log(0, BoxLine(line.str()));
                }
            }

            /* Failover count from FailoverConnectionTracker */
            uint32_t nFailoverCount = FailoverConnectionTracker::Get().GetTotalFailoverCount();
            debug::log(0, BoxLine(std::string("  Failover connections since boot: ")
                                  + std::to_string(nFailoverCount)));
        }
        debug::log(0, sep);

        for(const auto& kv : miners_copy)
        {
            const MinerStats& s = kv.second;

            std::string simTag = (s.connection_count > 1)
                ? " [SIM LINK: " + std::to_string(s.connection_count) + " connections]"
                : "";

            debug::log(0, BoxLine("Miner: " + s.genesis_prefix + "... @ "
                                  + s.remote_endpoint + simTag));

            if(s.connection_count > 0)
                debug::log(0, BoxLine("  Uptime: " + FormatUptime(s.connected_at)));

            debug::log(0, BoxLine(std::string("  Submitted: ")
                                  + std::to_string(s.blocks_submitted)
                                  + "  Accepted: " + std::to_string(s.blocks_accepted)
                                  + "  Rejected: " + std::to_string(s.blocks_rejected)));

            debug::log(0, BoxLine(std::string("  GET_BLOCK: ")
                                  + std::to_string(s.get_block_count)
                                  + " requests  Rate-limited: "
                                  + std::to_string(s.get_block_rate_limited)));

            /* ── Colin PING Diagnostics ──────────────────────────────── */
            {
                debug::log(0, BoxLine("  \xE2\x94\x80 COLIN DIAGNOSTICS (Miner: "
                    + (s.remote_endpoint.empty() ? s.genesis_prefix : s.remote_endpoint)
                    + ") \xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"));

                if(s.last_pong.pong_received)
                {
                    /* RTT with rolling-history trend */
                    std::string trend = format_rtt_trend(s.rtt_history);
                    std::string trend_part = trend.empty() ? "" : ("  (" + trend + ")");
                    debug::log(0, BoxLine("  PING #" + std::to_string(s.last_pong.sequence)
                        + "   RTT: " + format_rtt(s.last_pong.rtt_us)
                        + trend_part));

                    /* Submission stats with rejection percentage */
                    std::string rej_pct_str;
                    if(s.blocks_submitted > 0)
                    {
                        uint32_t pct = static_cast<uint32_t>(
                            (static_cast<uint64_t>(s.blocks_rejected) * 100) / s.blocks_submitted);
                        rej_pct_str = " (" + std::to_string(pct) + "%)";
                    }
                    debug::log(0, BoxLine("  Submissions: "
                        + std::to_string(s.blocks_submitted) + " sent / "
                        + std::to_string(s.blocks_accepted) + " accepted / "
                        + std::to_string(s.blocks_rejected) + " rejected" + rej_pct_str));

                    /* Miner health with critical temperature annotation */
                    std::string health_str = format_miner_health(s.last_pong.miner_health_flags);
                    std::string flags_hex;
                    {
                        std::ostringstream fss;
                        fss << "0x" << std::hex << std::setw(2) << std::setfill('0')
                            << static_cast<unsigned>(s.last_pong.miner_health_flags);
                        flags_hex = fss.str();
                    }
                    debug::log(0, BoxLine("  Miner Health: " + health_str
                        + "  (flags: " + flags_hex + ")"));

                    /* Per-miner hardware telemetry */
                    std::string temp_str = format_temp(s.last_pong.miner_temp_cdeg);
                    if(s.last_pong.miner_health_flags & PongRecord::MFLAG_TEMP_CRITICAL)
                        temp_str += " \xe2\x86\x90 CRITICAL";
                    debug::log(0, BoxLine("    HashRate: "
                        + std::to_string(s.last_pong.miner_hash_rate) + " kH/s"
                        + "  Temp: " + temp_str));
                    debug::log(0, BoxLine("    Threads: "
                        + std::to_string(s.last_pong.miner_threads)
                        + "  Queue: " + std::to_string(s.last_pong.miner_queue_depth)));
                }
                else
                {
                    debug::log(0, BoxLine("  PING #" + std::to_string(s.ping_sequence)
                        + "  (awaiting first PONG response...)"));
                }
            }

            if(!s.last_rejection_reason.empty() || s.last_canonical_snap_stale)
            {
                debug::log(0, sep);
                debug::log(0, BoxLine(std::string("  DIAGNOSTICS & WARNINGS:")));
                if(!s.last_rejection_reason.empty())
                    debug::log(0, BoxLine("  * Last rejection: " + s.last_rejection_reason));
                if(s.last_canonical_snap_stale)
                    debug::log(0, BoxLine("  * [WARN] Canonical snapshot stale ("
                        + std::to_string(s.last_canonical_snap_age_ms) + " ms) — miner may hold outdated template"));
            }

            debug::log(0, sep);
        }

        /* Template push stats */
        uint32_t total_pushes = global_copy.prime_pushes + global_copy.hash_pushes;
        debug::log(0, BoxLine(std::string("TEMPLATE PUSH STATS (last ")
                              + std::to_string(m_interval_s) + "s):"));
        debug::log(0, BoxLine(std::string("  PRIME pushes: ")
                              + std::to_string(global_copy.prime_pushes)
                              + "  HASH pushes: " + std::to_string(global_copy.hash_pushes)
                              + "  Total: " + std::to_string(total_pushes)));
        debug::log(0, bottom);
    }

} // namespace LLP
