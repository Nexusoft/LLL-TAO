# LLP Test Playground Guide

**Component:** Stateless Mining Protocol — LLP Test Suite  
**Source:** Originally `tests/unit/LLP/TESTING_PLAYGROUND_README.cpp`  
**Last Updated:** 2026-03-13

---

## Overview

This guide documents the **Comprehensive Testing Playground for Stateless Mining** — a
collection of unit and integration tests that validate the stateless mining architecture
with broad creative coverage, including happy paths, error conditions, concurrency, and
edge cases.

The original file (`TESTING_PLAYGROUND_README.cpp`) existed as a C++ file containing
only a large documentation comment block and a trivial `REQUIRE(true)` test case
(`TEST_CASE("Test Suite Documentation")`).  The trivial test has been removed — it added
nothing but build noise.  All substantive testing lives in the numbered suites below.

---

## Test Philosophy

- Test **everything** that can be tested
- Embrace redundancy — multiple tests for the same feature = confidence
- Document intent clearly in every `SECTION`
- Think like an attacker — validate adversarial inputs
- Validate both happy paths **and** error conditions
- Test edge cases exhaustively

---

## Test Infrastructure

### `helpers/test_fixtures.h`

| Helper | Purpose |
|--------|---------|
| `CreateTestGenesis()` | Deterministic genesis hash for tests |
| `CreateTestRegisterAddress()` | Base58 NXS register address |
| `CreateRandomHash()` | Random 256-bit hash |
| `CreateTestNonce()` | Test challenge nonce |
| `CreateTestFalconPubKey()` | Minimal Falcon-1024 public key bytes |
| `CreateTestChaChaKey()` | 32-byte ChaCha20 session key |
| `CreateBasicMiningContext()` | Unauthenticated context |
| `CreateAuthenticatedContext()` | Falcon-authenticated context |
| `CreateRewardBoundContext()` | Reward-address-bound context |
| `CreateFullMiningContext()` | Fully configured context |
| `MiningContextBuilder` | Fluent builder: `WithRandomGenesis()`, `WithAuthenticated()`, `WithChannel()`, etc. |

### `helpers/packet_builder.h`

| Helper | Purpose |
|--------|---------|
| `PacketBuilder` | Fluent packet construction (header, length, data) |
| `CreateSetChannelPacket()` | SET\_CHANNEL packet factory |
| `CreateGetBlockPacket()` | GET\_BLOCK packet factory |
| `CreateSubmitBlockPacket()` | SUBMIT\_BLOCK packet factory |
| `CreateAuthInitPacket()` | MINER\_AUTH\_INIT packet factory |
| `CreateAuthResponsePacket()` | MINER\_AUTH\_RESPONSE packet factory |
| `CreateSetRewardPacket()` | MINER\_SET\_REWARD packet factory |
| `CreateMalformedPacket()` | Truncated / corrupted packet |
| `CreateOversizedPacket()` | Oversized payload packet |

---

## Test Suites

### 1. `context_immutability_comprehensive.cpp` (150+ assertions)

**Validates:** `MiningContext` immutable architecture

- Default construction (all fields zero/empty/false)
- Immutability: every `With*` method returns a **new** context — original unchanged
- Method chaining (2, 5, 10 methods deep)
- State transitions: Disconnected → Mining Ready
- Copy semantics
- Edge cases: max values, empty vectors, large vectors
- Helper methods: `GetPayoutAddress()`, `HasValidPayout()`
- Builder pattern usage

**Why it matters:**  
Immutability prevents accidental state corruption in concurrent scenarios.  Each context
is a snapshot of miner state at a specific moment in time.

---

### 2. `dual_identity_comprehensive.cpp` (70+ assertions)

**Validates:** Authentication vs Reward separation (THE CORE FIX)

- Authentication identity (`hashGenesis` from Falcon)
- Reward identity (`hashRewardAddress` from `MINER_SET_REWARD`)
- Register address support (base58 decoded addresses)
- Genesis hash support (backward compatibility)
- Mixed-miner scenarios
- Zero/invalid address handling
- Manager integration
- Payout address logic
- Complete protocol flow simulation

**Why it matters:**  
This validates the critical fix allowing miners to use register addresses for rewards
while authenticating with their genesis hash.  `CreateProducer()` must trust
`hashDynamicGenesis` directly **without** `HasFirst()` lookup.

---

### 3. `session_management_comprehensive.cpp` (100+ assertions)

**Validates:** Session lifecycle and stability

- Session initialisation
- Timeout configuration
- Expiration detection
- Keepalive handling
- Session recovery
- Manager integration
- Concurrent access safety
- Complete lifecycle

**Why it matters:**  
Sessions provide state continuity.  DEFAULT session validation ensures graceful error
handling when the node is not started with `-unlock=mining`.

---

### 4. `packet_processing_comprehensive.cpp` (90+ assertions)

**Validates:** Packet-based protocol communication

- Packet construction and serialisation
- Authentication packets (`INIT`, `CHALLENGE`, `RESPONSE`)
- Reward binding packets (`SET_REWARD`)
- Mining operation packets (`SET_CHANNEL`, `GET_BLOCK`, `SUBMIT`)
- Malformed packet handling
- Oversized packet handling
- Builder utility functions
- Edge cases
- Valid packet sequences
- Data integrity

**Why it matters:**  
Packet protocol is the foundation of miner–node communication.  Robust packet handling
prevents crashes and security vulnerabilities.

---

### 5. `integration_comprehensive.cpp` (80+ assertions)

**Validates:** End-to-end mining flows

- Complete mining cycle (connection → block submission)
- Multi-miner concurrent mining
- Miner disconnection and reconnection
- Channel switching during mining
- Height updates and new rounds
- Session expiration and cleanup
- Stress test with 100 miners
- Complete protocol flow with packets

**Why it matters:**  
Integration tests catch bugs that unit tests miss by validating that all components work
together correctly in realistic scenarios.

---

### 6. `error_handling_comprehensive.cpp` (100+ assertions)

**Validates:** Defensive programming and error recovery

- Authentication errors
- Reward binding errors
- Session errors
- Channel errors
- Packet validation errors
- State validation
- Manager error conditions
- Resource exhaustion
- Edge case scenarios
- Concurrent access safety
- Recovery from errors

**Why it matters:**  
Graceful error handling prevents crashes, provides clear error messages, and keeps
connections alive when possible.  Critical for production systems.

---

## Coverage Summary

| Suite | Assertions | Test Cases |
|-------|-----------|-----------|
| `context_immutability_comprehensive.cpp` | 150+ | ~25 |
| `dual_identity_comprehensive.cpp` | 70+ | ~15 |
| `session_management_comprehensive.cpp` | 100+ | ~20 |
| `packet_processing_comprehensive.cpp` | 90+ | ~18 |
| `integration_comprehensive.cpp` | 80+ | ~16 |
| `error_handling_comprehensive.cpp` | 100+ | ~20 |
| **Total** | **590+** | **~114** |

---

## Critical Paths Validated

- ✅ **Register Address Reward Routing** — base58 address decoded → `hashDynamicGenesis` → coinbase
- ✅ **DEFAULT Session Validation** — node without `-unlock=mining` → clean error, no crash
- ✅ **Dual-Identity Architecture** — Falcon auth genesis ≠ reward address
- ✅ **Immutability Pattern** — all `With*` methods return new contexts
- ✅ **Session Management** — init, timeout, expiry, keepalive, recovery
- ✅ **Packet Protocol** — all types validated, malformed data handled
- ✅ **Error Handling** — all categories covered, graceful failure modes

---

## Building and Running

```bash
# Build
make -f makefile.cli UNIT_TESTS=1 -j 4

# Run all tests
./nexus

# Run specific suite
./nexus "[context]"
./nexus "[dual-identity]"
./nexus "[session]"
./nexus "[packet]"
./nexus "[integration]"
./nexus "[error-handling]"

# Run specific test
./nexus "MiningContext Default Construction"
```

---

## See Also

- [LLP Comprehensive Test Summary](LLP-comprehensive-test-summary.md)
- [Stateless Mining Protocol](../mining/stateless-protocol.md)
- [Session Container Architecture](../node/session-container-architecture.md)
