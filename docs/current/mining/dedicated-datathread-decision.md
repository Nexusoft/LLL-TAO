# ADR: Dedicated Mining DataThread with Epoll

**Status:** Accepted  
**Date:** 2026-04 (PR #543)  
**Deciders:** Core development team  
**Category:** Architecture Decision Record (ADR)

---

## Context

Mining connections on the Nexus NODE server compete with Tritium P2P traffic for
I/O processing time.  Both share the same `DataThread` pool, which uses `poll()`
with a 100 ms timeout and iterates over all file descriptors on every cycle.

When P2P synchronization traffic is heavy (block sync, inventory exchange,
`LIST_BLOCKS` batches of 2500), the `poll()` cycle can saturate CPU for 100 ms+
before mining connections get serviced.  This causes:

- Template delivery delays of 100–500 ms
- Silent connection drops (POLL_EMPTY, TIMEOUT_WRITE)
- Shadow bans from read-idle timeout violations
- Missed block submissions on short-proof channels (Hash)

The production deployment target is **Linux** (Ubuntu 22.04+).

---

## Options Considered

### Option 1: Separate Process

Run the mining server as a standalone binary that communicates with the main
NODE process via IPC (shared memory / Unix sockets) for block templates.

- ✅ Complete I/O isolation
- ❌ Requires IPC protocol for templates, chain state, and block submission
- ❌ Doubles memory footprint (two processes)
- ❌ Complex deployment and lifecycle management
- ❌ Latency from serialization/deserialization

**Rejected:** Engineering cost disproportionate to benefit.

### Option 2: Full MiningSocket

Create a custom `MiningSocket` subclass with its own accept loop, connection
storage, read/write buffers, and platform-specific event loop
(epoll/kqueue/IOCP).

- ✅ Complete control over I/O path
- ✅ Cross-platform potential
- ❌ ~2000–3000 lines of new code
- ❌ Duplicates SSL termination, DDOS management, connection lifecycle
- ❌ Loses LLP's battle-tested `BaseConnection` outgoing queue, health checks,
  graceful shutdown, and FLUSH_THREAD integration
- ❌ Testing matrix explosion: 3 platforms × 2 protocols × SSL/non-SSL

**Rejected:** Too much code duplication; may be revisited if epoll proves insufficient.

### Option 3: Separate Thread Pool (poll only)

Give mining DataThreads their own thread pool (separate from P2P/API), but
continue using `poll()` with a reduced timeout (10 ms).

- ✅ Eliminates competition with P2P traffic
- ✅ Minimal code change
- ⚠️ Still O(n) per poll cycle
- ⚠️ 10 ms minimum latency floor

**Partially accepted:** Thread pool separation is a prerequisite regardless of
I/O mechanism.  Implemented in PR #541.

### Option 4: Epoll on Mining DataThread ← CHOSEN

Use Linux's `epoll` for mining DataThreads, with compile-time trait gating
(`is_mining_data_thread_v`) and runtime fallback to `poll()` on non-Linux
platforms or `epoll_create1` failure.

- ✅ **O(ready)** complexity — only processes connections with events
- ✅ **1 ms timeout** — sub-millisecond responsiveness
- ✅ **~300 lines of code** — minimal change surface
- ✅ Zero impact on P2P/API traffic (different DataThread pool + different I/O)
- ✅ Graceful fallback on non-Linux (poll with 10 ms timeout)
- ⚠️ Linux-only optimization (macOS/Windows get poll fallback)
- ⚠️ If contention moves above DataThread layer, need Option 2

### Option 5: io_uring

Use Linux's `io_uring` for fully asynchronous I/O with kernel-side completion.

- ✅ Even lower latency than epoll (kernel-side I/O)
- ✅ Batch submission of I/O operations
- ❌ Requires kernel 5.1+ (2019), many production servers on older kernels
- ❌ Complex API (submission queue + completion queue + ring buffer management)
- ❌ Overkill for connection counts <1000
- ❌ Security concerns (historically many io_uring CVEs)

**Rejected:** Complexity and kernel version requirements exceed the benefit for
our connection scale.

---

## Decision

**Option 4 — Epoll on mining DataThreads.**

The implementation:

1. Adds `is_mining_data_thread_v<ProtocolType>` compile-time trait in `data.h`
2. Creates epoll fd in DataThread constructor for mining protocols
3. Dispatches to `ThreadEpoll()` instead of `Thread()` when epoll is available
4. Falls back to `poll()` with 10 ms timeout on non-Linux or epoll failure
5. Shares `check_connection_health()` between both I/O paths
6. Uses `EPOLL_CLOEXEC` to prevent FD leaks into child processes

---

## Consequences

### Positive

- **Sub-millisecond mining I/O** on Linux — 1 ms `epoll_wait` timeout with
  O(ready) complexity
- **Zero impact on P2P/API** — separate thread pool with separate I/O mechanism
- **Minimal code surface** — ~300 lines added, no existing code restructured
- **Graceful degradation** — non-Linux platforms automatically fall back to poll

### Negative

- **Linux-only** — macOS and Windows developers get poll fallback with higher
  latency (10 ms); production mining must run on Linux
- **Shared FLUSH_THREAD** — outgoing packet drain is still shared across protocols;
  push notifications may be delayed if P2P traffic has large queued packets
- **CONNECTIONS vector contention** — epoll registration/deregistration and slot
  management still use the LLP's shared connection storage
- **Health sweep O(n)** — every 250 ms, ThreadEpoll scans all connections (not
  just ready ones) for timeout/stall/overflow checks

### If Epoll Proves Insufficient

Escalate to **Option 2 (Full MiningSocket)** if production miners report:
- Latency spikes >10 ms at the 99th percentile
- CONNECTIONS vector spinlock contention under high connection churn
- FLUSH_THREAD stalls delaying mining push notifications

---

## Related

- [Linux Epoll Mining Architecture](linux-epoll-mining-architecture.md)
- [Epoll vs Poll Architecture Diagrams](../../diagrams/mining/epoll-vs-poll-architecture.md)
- [Known Risks and Limitations](known-risks-and-limitations.md)
