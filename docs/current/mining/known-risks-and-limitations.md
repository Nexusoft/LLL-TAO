# Known Risks and Limitations — Mining Infrastructure

**Version:** LLL-TAO 5.1.0+  
**Last Updated:** 2026-04-11  
**Status:** Living document — update as risks are mitigated or new ones identified

---

## 1. Platform Limitation: Linux-Only Epoll

**Risk Level:** Medium (production) / Low (development)

The dedicated epoll I/O path for mining DataThreads is **Linux-only**.  Non-Linux
platforms (macOS, Windows) fall back to `poll()` with a reduced 10 ms timeout.

| Platform | I/O Mechanism | Timeout | Complexity |
|----------|--------------|---------|------------|
| Linux | `epoll_wait()` | 1 ms | O(ready) |
| macOS | `poll()` | 10 ms | O(n) |
| Windows | `WSAPoll()` | 10 ms | O(n) |

**Impact:** Development and testing on macOS/Windows will have higher mining
I/O latency.  Production mining nodes **must run on Linux** for optimal
performance.

**Mitigation:** The poll fallback is functional — miners will work on any
platform, just with higher latency.

---

## 2. CONNECTIONS Vector Contention

**Risk Level:** Low–Medium

Mining connections share the LLP's `CONNECTIONS` vector with slot management via
`atomic_lock_unique_ptr`.  Two concurrent access patterns exist:

- **ListeningThread** → `AddConnection()` → `epoll_register()` → writes to slot
- **DataThread** → `remove_connection()` → `epoll_deregister()` → clears slot

Under high connection churn (rapid connect/disconnect cycles at >100 events/sec),
the spinlock-based atomic pointer could cause contention between these threads.

**Impact:** Latency spikes during connection storms (pool restart, miner
reconnect waves).

**Mitigation:** Not yet observed in production.  If it occurs, consider:
1. Per-DataThread connection lists (eliminate shared vector)
2. Lock-free slot allocation with CAS
3. Escalate to full MiningSocket route (see ADR)

---

## 3. Shared FLUSH_THREAD

**Risk Level:** Low

The outgoing packet drain thread (`DrainOutgoingQueue()` via `FLUSH_CONDITION`)
is shared across all connection types.  Mining push notifications enqueued via
`QueuePacket()` are drained by the same thread that drains P2P and API packets.

**Impact:** If P2P traffic has large queued packets (e.g., block relay, inventory
batches), mining push notifications may experience millisecond-level delays.

**Mitigation:** PR #536 decoupled template building from socket writes.  The
`fHasOutgoing` atomic flag provides lock-free predicate evaluation.  If delays
persist, a dedicated mining FLUSH_THREAD could be added.

---

## 4. Health Sweep O(n) Cost

**Risk Level:** Low

Every 250 ms, `ThreadEpoll()` performs a health sweep across **all** mining
connections (not just those with events).  At N connections, this is O(N) work
per sweep.

| Connections | Sweep Cost (estimated) |
|-------------|----------------------|
| 50 | ~0.05 ms |
| 500 | ~0.5 ms |
| 5000 | ~5 ms |

**Impact:** At very high connection counts (5000+), the health sweep could
add noticeable latency to the epoll loop.

**Mitigation:** Current mining scale is <500 concurrent connections.  If scaling
beyond 1000, consider:
1. Staggered health sweeps (check 1/N connections per cycle)
2. Separate health-check thread
3. Event-driven health checks (triggered by timer FD in epoll)

---

## 5. epoll_ctl Thread Safety Edge Cases

**Risk Level:** Low

`EPOLL_CTL_ADD` (from ListeningThread) and `EPOLL_CTL_DEL` (from DataThread)
can execute concurrently on the same epoll instance.  The Linux kernel guarantees
this is safe — `epoll_ctl` acquires an internal mutex per epoll instance.

**Impact:** Under rapid connect/disconnect cycles, the kernel-internal mutex
could become a bottleneck.  Additionally, a race between `EPOLL_CTL_DEL` and
`close(fd)` could theoretically cause the epoll instance to reference a
recycled FD.

**Mitigation:** Current production traffic patterns do not trigger this.  The
`epoll_deregister()` call always precedes `close()` in the disconnect path.

---

## 6. Session Recovery Removed — Full Re-Auth Required

**Risk Level:** Low (design decision)

`SessionRecoveryManager` was removed in PR #532.  On disconnect, miners must
perform a full Falcon re-authentication handshake.  There is no session
resumption across reconnects.

**Impact:** Miners that disconnect and reconnect (e.g., network glitch) must
perform a full Falcon handshake (~2–5 ms for Falcon-512, ~10–20 ms for
Falcon-1024) before receiving new block templates.

**Mitigation:** This is an intentional design decision — simpler and more secure.
The handshake time is negligible compared to block find times (minutes to hours).

---

## 7. Template Storm on Rapid Chain Tip Changes

**Risk Level:** Low (mitigated by PR #538)

When multiple blocks are found in rapid succession (e.g., Hash blocks at 50-second
intervals), each tip change triggers template generation for all connected miners.

**Impact:** Brief CPU spikes during rapid tip changes.  Mostly mitigated.

**Mitigation:** PR #538 added a global rate limiter and live context reads to
prevent redundant template generation.  PR #539 added rate-limit-before-read
for GET_BLOCK requests.

---

## 8. Dual Server Process — Not Yet Unified at Socket Level

**Risk Level:** Informational

The mining infrastructure runs **two separate server instances**:
- `STATELESS_MINER_SERVER` → `Server<StatelessMinerConnection>` (port 9323)
- `MINING_SERVER` → `Server<Miner>` (port 8323)

Both share configuration via `MiningServerFactory` but maintain separate
DataThread pools, connection storage, and FLUSH_THREADs.

**Impact:** Double the resource usage (threads, memory) compared to a single
unified server.  However, this also provides natural isolation between protocol
lanes.

**Future:** The `unified-mining-server-architecture.md` design document outlines
a future protocol adapter pattern that would consolidate to a single server.

---

## Risk Escalation Triggers

Monitor the following metrics in production.  If thresholds are exceeded,
consider escalating to the full MiningSocket route:

| Metric | Threshold | Action |
|--------|-----------|--------|
| Template delivery p99 latency | >10 ms | Investigate CONNECTIONS contention |
| FLUSH_THREAD drain latency | >5 ms | Add dedicated mining FLUSH_THREAD |
| Health sweep duration | >2 ms | Implement staggered sweeps |
| Connection churn rate | >100/sec | Investigate spinlock contention |
| Miner-reported stale blocks | >1% | Full diagnostic investigation |

---

## Related Documentation

- [Linux Epoll Mining Architecture](linux-epoll-mining-architecture.md)
- [Dedicated DataThread Decision Record](dedicated-datathread-decision.md)
- [Recent Changes Summary](recent-changes-summary.md)
