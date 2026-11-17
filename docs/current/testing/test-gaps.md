# Mining Infrastructure — Test Gaps

**Last Updated:** 2026-04-11  
**Status:** Living document — update as gaps are filled

---

## Overview

This document identifies areas of the mining infrastructure that lack direct
unit or integration test coverage.  These gaps exist for various reasons:
kernel dependency, private method access, or complexity of mocking the
required environment.

---

## 1. Epoll / DataThread I/O Path

### Gap: No unit tests for `ThreadEpoll()`

**Why:** `ThreadEpoll()` requires a real Linux `epoll` file descriptor, real
sockets, and real `DataThread` lifecycle management.  Mocking `epoll_create1`,
`epoll_ctl`, and `epoll_wait` at the unit test level would require either:

- A Linux-specific test environment (not portable)
- A virtual file descriptor layer (significant engineering)
- Integration tests with real sockets (slow, flaky)

**Current coverage:** `ThreadEpoll()` is only exercised in live node operation.

**Recommendation:** Create a Linux-only integration test that:
1. Creates a `DataThread<Miner>` instance
2. Connects a test socket
3. Verifies epoll registration via `/proc/self/fdinfo/<epoll_fd>`
4. Sends a packet and verifies `ProcessPacket()` is called
5. Disconnects and verifies epoll deregistration

### Gap: No test for epoll → poll fallback

**Why:** The fallback path (`epoll_create1` returns -1) requires simulating
a kernel failure, which is difficult without `LD_PRELOAD` or `seccomp`.

**Recommendation:** Test at the code path level: verify that when
`m_nEpollFd == -1`, `Thread()` does not call `ThreadEpoll()` and instead
enters the poll loop.  This could be a compile-time test with a mock
`epoll_create1` that always fails.

### Gap: No test for `check_connection_health()` (private method)

**Why:** `check_connection_health()` is a private method of `DataThread`.
Testing it requires either:

- `friend class` declaration in `data.h`
- Extraction to a free function (would require refactoring)
- Testing indirectly through `Thread()` / `ThreadEpoll()` behavior

**Current coverage:** Indirectly tested by connection timeout, buffer overflow,
and partial-stall scenarios in integration tests.

**Recommendation:** Extract the five health check conditions into a testable
free function or public static method that operates on connection state
without requiring a full `DataThread` instance.

---

## 2. Concurrency & Thread Safety

### Gap: No stress test for concurrent `epoll_ctl ADD/DEL`

**Why:** `EPOLL_CTL_ADD` (from `ListeningThread`) and `EPOLL_CTL_DEL` (from
`DataThread`) execute concurrently.  While the Linux kernel guarantees this
is safe, edge cases with rapid connect/disconnect cycles could reveal timing
issues.

**Recommendation:** Create a stress test that rapidly connects and disconnects
100+ sockets while the DataThread is processing packets, and verify no crashes
or FD leaks occur.

### Gap: No test for `shared_mutex` read contention under mining load

**Why:** The `std::shared_mutex` change in `SectorDatabase` (PR #540) enables
concurrent readers, but there are no tests that verify performance improvement
or correctness under mining-specific read patterns (multiple DataThreads
calling `Get()` simultaneously).

**Recommendation:** Benchmark test that creates N reader threads performing
`Get()` operations and 1 writer thread performing `Update()` operations,
measuring throughput compared to the old `std::mutex` behavior.

### Gap: No test for `CONNECTIONS` vector spinlock contention

**Why:** The `atomic_lock_unique_ptr` used for connection slot management
could become a bottleneck under high connection churn.  No test exercises
this path at scale.

**Recommendation:** Stress test with 1000+ rapid connect/disconnect cycles
measuring slot allocation latency.

---

## 3. Security & FD Hardening

### Gap: No test for `SOCK_CLOEXEC` inheritance after `std::system()`

**Why:** The `SOCK_CLOEXEC` flag prevents FD leaks into child processes
spawned by `std::system()` (e.g., `-blocknotify` handlers).  Testing this
requires spawning a child process and verifying it does not inherit the
parent's socket FDs.

**Recommendation:** Test that:
1. Creates a socket with `SOCK_CLOEXEC`
2. Calls `std::system("ls /proc/self/fd")`
3. Verifies the socket FD does not appear in the child's FD list

### Gap: No test for `accept4(SOCK_CLOEXEC)` ENOSYS fallback

**Why:** The `ENOSYS` fallback (for kernels that don't support `accept4`)
is tested only on kernels that actually lack `accept4` — which is
essentially none in production.

**Recommendation:** Low priority.  The fallback code path is straightforward
(call `accept()` + `fcntl(FD_CLOEXEC)`) and unlikely to contain bugs.

---

## 4. Protocol & Session

### Gap: No test for protocol auto-detection in `Miner::ReadPacket()`

**Why:** `Miner::ReadPacket()` (virtual override) detects the protocol lane
based on the first byte: `0xD0` → 16-bit stateless, else → 8-bit legacy.
There are no unit tests that feed raw bytes and verify lane selection.

**Recommendation:** Test that creates a mock connection, writes bytes with
`0xD0` prefix, and verifies stateless framing is selected; then writes bytes
with non-`0xD0` prefix and verifies legacy framing.

### Gap: No test for `AutoCooldownManager` expiry timing

**Why:** `AutoCooldownManager` replaces permanent DDOS bans with auto-expiring
cooldowns.  No test verifies that cooldowns actually expire after the
configured duration.

**Recommendation:** Unit test that sets a cooldown, advances the clock past
expiry, and verifies the connection is allowed again.

---

## 5. Stale Test Audit

### Test files with no stale references found

A search for removed components (`SessionRecoveryManager`,
`ActiveSessionBoard`, `GlobalTemplateCache`, `STATELESS_MINER_SERVER`) in
test files returned **zero matches**.  All test files appear to reference
current APIs only.

### Test files that may need review

| File | Reason |
|------|--------|
| `session_management_comprehensive.cpp` | May reference old session fields |
| `dual_connection_manager_tests.cpp` | May reference old failover patterns |

---

## Priority Matrix

| Gap | Priority | Effort | Impact |
|-----|----------|--------|--------|
| `ThreadEpoll()` integration test | Medium | High | High — covers critical I/O path |
| `check_connection_health()` extraction | Medium | Medium | Medium — improves testability |
| Protocol auto-detection test | Medium | Low | Medium — simple but untested |
| `SOCK_CLOEXEC` child process test | Low | Medium | Low — security hardening |
| `shared_mutex` concurrency bench | Low | Medium | Low — performance validation |
| `epoll_ctl` stress test | Low | High | Low — kernel guarantees safety |
| `AutoCooldownManager` expiry test | Low | Low | Low — straightforward logic |

---

## Related Documentation

- [Mining Test Inventory](mining-test-inventory.md)
- [Linux Epoll Mining Architecture](../mining/linux-epoll-mining-architecture.md)
- [Known Risks and Limitations](../mining/known-risks-and-limitations.md)
