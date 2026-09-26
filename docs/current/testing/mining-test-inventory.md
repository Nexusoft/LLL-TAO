# Mining Test Inventory

**Last Updated:** 2026-04-11  
**Framework:** Catch2 (header-only, `tests/unit/catch2/catch.hpp`)  
**Total Mining-Related Tests:** 72 files across LLP and LLC

---

## LLP Mining Tests (61 files)

### Authentication & Crypto (11 files)

| File | Purpose |
|------|---------|
| `falcon_1024_auth_test.cpp` | Falcon-1024 signature generation and verification |
| `falcon_1024_detection_test.cpp` | Auto-detection of Falcon-512 vs Falcon-1024 key format |
| `falcon_auth_stress.cpp` | Stress test: rapid auth/deauth cycles |
| `falcon_endianness.cpp` | Byte-order correctness in Falcon key serialization |
| `falcon_full_block_format.cpp` | Falcon signatures in full block context |
| `falcon_handshake.cpp` | Complete MINER_AUTH_INIT → RESULT handshake flow |
| `falcon_protocol_integration.cpp` | Falcon integrated with full protocol lifecycle |
| `falcon_verify_test.cpp` | Isolated Falcon signature verification |
| `encryption_context_test.cpp` | ChaCha20 encryption context setup and key derivation |
| `test_stateless_miner_crypto.cpp` | Session key derivation and crypto primitives |
| `genesis_validation.cpp` | Genesis hash validation in authentication |

### Mining Protocol Core (16 files)

| File | Purpose |
|------|---------|
| `stateless_miner.cpp` | Comprehensive stateless mining protocol tests (largest test file) |
| `miner_protocol.cpp` | Legacy miner protocol tests |
| `mining_config.cpp` | Mining configuration parsing and defaults |
| `mining_server_factory_test.cpp` | MiningServerFactory::BuildConfig() lane tests |
| `mining_template_integration.cpp` | Template creation ↔ mining protocol integration |
| `packet_data_payload.cpp` | Packet payload serialization/deserialization |
| `packet_length_validation_test.cpp` | Packet length boundary validation |
| `packet_processing_comprehensive.cpp` | Full packet processing lifecycle |
| `packet_round_response.cpp` | GET_ROUND response format and height encoding |
| `submit_block_format_test.cpp` | SUBMIT_BLOCK wire format validation |
| `template_metadata_channel_height.cpp` | TemplateMetadata channel height tracking |
| `test_get_block_response_contract.cpp` | GET_BLOCK response contract validation |
| `test_get_round_schema.cpp` | GET_ROUND schema compliance |
| `test_respond_auto.cpp` | Auto-response mechanism tests |
| `test_template_staleness.cpp` | Template staleness detection logic |
| `test_cleanup_stale_templates.cpp` | Stale template cleanup and purging |

### Session Management (7 files)

| File | Purpose |
|------|---------|
| `session_management_comprehensive.cpp` | Comprehensive session lifecycle tests |
| `session_status_tests.cpp` | SESSION_STATUS handler and MINER_DEGRADED recovery |
| `session_store_tests.cpp` | SessionStore + CanonicalSession CRUD operations |
| `node_session_registry_tests.cpp` | NodeSessionRegistry identity tracking |
| `node_cache.cpp` | Node-side session cache operations |
| `test_heartbeat_refresh.cpp` | Keepalive heartbeat and session refresh |
| `test_simlink_session_rate_limiter.cpp` | SIM-Link session rate limiting |

### Concurrency & Reliability (7 files)

| File | Purpose |
|------|---------|
| `concurrency_testing.cpp` | Multi-threaded mining operation tests |
| `context_immutability_comprehensive.cpp` | MiningContext immutability guarantees |
| `dual_identity_comprehensive.cpp` | Dual-identity (stateless + legacy) scenarios |
| `error_handling_comprehensive.cpp` | Error path coverage |
| `integration_comprehensive.cpp` | End-to-end integration scenarios |
| `node_integration_hardening.cpp` | Node stability under mining load |
| `socket_buffer_tests.cpp` | Socket buffer Write/Flush/fBufferFull lifecycle |

### Notifications & Push (5 files)

| File | Purpose |
|------|---------|
| `burst_mode_push_notification_test.cpp` | Burst-mode push notification delivery |
| `push_notification_tests.cpp` | Push notification subscription and delivery |
| `push_throttle_test.cpp` | Push notification rate limiting |
| `canonical_chain_state_tests.cpp` | CanonicalChainState snapshot correctness |
| `channel_state_manager_tests.cpp` | Channel state transition management |

### Protocol Features (10 files)

| File | Purpose |
|------|---------|
| `keepalive_v2_tests.cpp` | KeepaliveV2 format and session refresh |
| `llp_lane_enforcement_test.cpp` | Protocol lane enforcement (8-bit vs 16-bit) |
| `test_protocol_lane.cpp` | Lane selection and switching logic |
| `rate_limiting_tests.cpp` | Per-connection and global rate limiting |
| `reward_address_protocol.cpp` | MINER_SET_REWARD protocol tests |
| `reward_routing.cpp` | Reward address routing and binding |
| `rewards_payout_verification.cpp` | Reward payout correctness |
| `opcode_alias.cpp` | Opcode alias resolution |
| `hash_sign_finalization_tests.cpp` | Hash channel sign and finalization |
| `prime_sign_finalization_tests.cpp` | Prime channel sign and finalization |

### Failover & Shutdown (5 files)

| File | Purpose |
|------|---------|
| `dual_connection_manager_tests.cpp` | DualConnectionManager failover state |
| `graceful_shutdown_tests.cpp` | NODE_SHUTDOWN (0xD0FF) packet delivery |
| `test_tolerance_verification.cpp` | Tolerance verification examples |
| `trust_address.cpp` | Trust-based address operations |
| `base_address.cpp` | Base address handling |

---

## LLC Crypto Tests (11 files)

| File | Purpose |
|------|---------|
| `aes.cpp` | AES encryption/decryption |
| `argon2.cpp` | Argon2 key derivation |
| `chacha20_helpers.cpp` | ChaCha20 helper functions |
| `chacha20_poly1305.cpp` | ChaCha20-Poly1305 AEAD cipher |
| `falcon_qtv_parity_test.cpp` | Falcon QTV parity checks |
| `falcon_version_test.cpp` | Falcon version detection |
| `fermat.cpp` | Fermat primality testing (Prime channel) |
| `mining_session_keys.cpp` | Mining session key generation and derivation |
| `qtv_hook_bridge_test.cpp` | QTV hook bridge integration |
| `test_genesis_endianness.cpp` | Genesis hash endianness correctness |
| `uint1024.cpp` | 1024-bit unsigned integer operations |

---

## Infrastructure

| File | Purpose |
|------|---------|
| `TESTING_PLAYGROUND_README.cpp` | Test playground documentation |
| `tests/unit/main.cpp` | Catch2 test runner entry point |
| `tests/live/main.cpp` | Live network test runner |
| `tests/bench/main.cpp` | Benchmark suite runner |

---

## Test Categories by PR

| PR | Tests Added/Modified |
|----|---------------------|
| #503 | `test_get_round_schema.cpp` (unified height staleness) |
| #507 | `socket_buffer_tests.cpp` (15 MB buffer) |
| #512 | `socket_buffer_tests.cpp` (fBufferFull lifecycle) |
| #530 | `session_store_tests.cpp` (SessionStore CRUD) |
| #535 | `mining_server_factory_test.cpp` (factory lane tests) |
| Graceful shutdown | `graceful_shutdown_tests.cpp` |
| Keepalive V2 | `keepalive_v2_tests.cpp` |
| Falcon 1024 | `falcon_1024_auth_test.cpp`, `falcon_1024_detection_test.cpp` |

---

## Running Tests

```bash
# Build all tests
make -f makefile.cli test-lll-tao

# Run specific test suites (if supported by makefile)
make -f makefile.cli test-llp          # LLP mining protocol tests
make -f makefile.cli test-crypto       # Cryptography tests

# Run benchmarks
make -f makefile.cli bench
```

---

## Related Documentation

- [Test Gaps](test-gaps.md)
- [LLP Comprehensive Test Summary](LLP-comprehensive-test-summary.md)
- [LLP Test Playground Guide](LLP-test-playground-guide.md)
