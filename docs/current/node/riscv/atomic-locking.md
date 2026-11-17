# Atomic Operations & Locking Considerations

**Section:** Node Architecture → RISC-V  
**Version:** LLL-TAO 5.1.0+  
**Last Updated:** 2026-03-10

---

## Overview

RISC-V uses a **relaxed memory model** (RVWMO — RISC-V Weak Memory Ordering).  This is weaker than the Total Store Order (TSO) model used by x86.  Code that works correctly on x86 through accidental sequencing may behave incorrectly on RISC-V if memory-order annotations are missing.

This page summarises the locking and atomic patterns used in the stateless mining node and confirms that they are safe on RISC-V.

---

## Memory Model Summary

| Architecture | Model | Implication |
|---|---|---|
| x86 / x86-64 | TSO (strong) | Loads not reordered with loads; stores not reordered with stores |
| ARM64 | Weak (similar to RVWMO) | Explicit acquire/release needed for synchronisation |
| RISC-V RV64GC | RVWMO (weak) | All reorderings permitted unless fenced; `fence` instruction required |

**Key consequence:** On RISC-V, a plain `bool` flag set by one thread and read by another is a data race unless protected by a `std::atomic` or a mutex.

---

## Node Atomic Usage Audit

### `MinerSessionContainer` Flags

| Field | C++ type | Safe on RISC-V? |
|---|---|---|
| `fAuthenticated` | `bool` (protected by container mutex) | ✅ Yes — mutex provides sequential consistency |
| `fRewardBound` | `bool` (protected by container mutex) | ✅ Yes — mutex provides sequential consistency |
| `nLastActivity` | `int64_t` (protected by container mutex) | ✅ Yes |

> **Rule:** All flag accesses on `MinerSessionContainer` must be performed while holding the container's `std::mutex`.  Do not read flags without the lock, even for "quick peek" optimisations — the RISC-V memory model does not guarantee visibility.

### `NodeSessionRegistry` Indexes

| Operation | Locking | Safe on RISC-V? |
|---|---|---|
| Lookup by session ID | `std::shared_mutex` read lock | ✅ Yes |
| Lookup by Falcon key ID | `std::shared_mutex` read lock | ✅ Yes |
| Insert new entry | `std::shared_mutex` write lock | ✅ Yes |
| Erase on disconnect | `std::shared_mutex` write lock | ✅ Yes |

### `SessionRecoveryManager` Snapshot Store

| Operation | Locking | Safe on RISC-V? |
|---|---|---|
| Store snapshot | `std::mutex` | ✅ Yes |
| Lookup snapshot | `std::mutex` | ✅ Yes |
| Evict expired snapshots | `std::mutex` | ✅ Yes |

---

## Lock-Scoped Validation Pattern

The recommended pattern for any multi-field read from `MinerSessionContainer` is:

```cpp
// CORRECT — hold lock for the entire validation block
{
    std::lock_guard<std::mutex> lock(container.mutex);
    std::string reason;
    if (!container.ValidateConsistency(reason)) {
        LOG(2, "container invalid: ", reason);
        return false;
    }
    // All reads here are coherent
    auto key = container.vChacha20Key;  // copy under lock
}
// Use 'key' outside the lock — safe because it's a local copy
```

```cpp
// WRONG — reading fields without lock on RISC-V
if (container.fAuthenticated) {                // RACE
    auto key = container.vChacha20Key;         // RACE
}
```

---

## Planned: `ScopedContainerUpdate` (R-06)

Roadmap item R-06 ([Diagram 6](../../../diagrams/upgrade-path/06-scoped-update-guard.md)) introduces a `ScopedContainerUpdate` RAII guard that:

1. Acquires the container write lock on construction.
2. Stages all field changes into a temporary copy.
3. On explicit `Commit()`: validates consistency then atomically swaps the staging copy into the live container.
4. On scope exit without `Commit()`: silently discards the staging copy (rollback).

This eliminates the class of bug where a partial write leaves the container in an inconsistent state if an exception or early return occurs mid-update.

```
  ┌──────────────────────────────────────────────────────────────┐
  │  ScopedContainerUpdate guard                                  │
  │                                                              │
  │  Construction         → acquires write lock                  │
  │  staging.vRewardHash = decoded;  (staged, not live yet)      │
  │  staging.fRewardBound = true;                                │
  │  guard.Commit()       → ValidateConsistency(staging)         │
  │                          OK → swap staging → live            │
  │                          FAIL → throw / return false         │
  │  Destructor           → releases write lock                  │
  │                          (rollback if no Commit was called)  │
  └──────────────────────────────────────────────────────────────┘
```

---

## Fence Requirements for Custom Atomics

If any future code introduces a hand-rolled atomic (not using `std::atomic`), it must insert explicit `fence` instructions on RISC-V:

```cpp
// Example: signal a producer/consumer flag safely on RISC-V
// Use std::atomic<bool> instead — this is illustrative only
asm volatile ("fence w, r" ::: "memory");   // store-release
asm volatile ("fence r, r" ::: "memory");   // load-acquire
```

**Strong recommendation:** Do not introduce hand-rolled atomics.  Always use `std::atomic<T>` with appropriate `std::memory_order` annotations.

---

## Deadlock Prevention Rules

1. **Lock ordering** — If two mutexes must be held simultaneously, always acquire them in the same order (e.g. container lock → registry lock, never the reverse).
2. **No blocking under lock** — Never perform I/O, logging, or any operation that could block while holding a container or registry lock.
3. **No nested container locks** — A thread holding one container's lock must never attempt to acquire another container's lock.
4. **Short critical sections** — All field reads/writes that need the lock should be batched into a single locked block; do not hold the lock across packet processing logic.

---

## Related Pages

- [Endianness & Serialisation](endianness-serialization.md)
- [Diagnostics & Testing Notes](diagnostics.md)
- [RISC-V Overview](index.md)
- [Scoped Update Guard Diagram (Diagram 6)](../../../diagrams/upgrade-path/06-scoped-update-guard.md)
