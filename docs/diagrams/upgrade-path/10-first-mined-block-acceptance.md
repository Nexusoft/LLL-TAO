# Diagram 10 — First-Mined-Block Acceptance Harness

**Roadmap Item:** R-10  
**Priority:** 4 (Future Architecture)

---

## Context (Before)

There is no automated end-to-end integration test that proves a miner can connect, authenticate, bind a reward address, receive a template, solve it, submit it, and have the node accept and apply the reward — all in a single reproducible test run.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Manual Verification Only                                  ║
║                                                                      ║
║  To verify "first block accepted" today:                             ║
║  1. Start a real node in testnet mode                                ║
║  2. Start a real NexusMiner binary                                   ║
║  3. Manually inspect node logs for BLOCK_ACCEPTED                    ║
║  4. Manually check coinbase reward address                           ║
║                                                                      ║
║  Problems:                                                           ║
║  • PoW difficulty on testnet may require real mining time            ║
║  • Log inspection is manual and error-prone                          ║
║  • No regression detection — a refactor that breaks block acceptance ║
║    is not caught until someone tries to mine manually                ║
║  • Multi-step dependencies between node and miner code               ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

A self-contained in-process integration test (`first_block_acceptance_test`) runs the full miner–node protocol flow using a loopback connection and trivially-solved PoW, and asserts block acceptance with reward address verification.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — First-Mined-Block Acceptance Harness                      ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  Test Setup                                                  │   ║
║  │  • Node: in-process test instance, testnet, trivial PoW      │   ║
║  │  • Miner: test stub implementing stateless protocol client   │   ║
║  │  • Transport: loopback socket pair (no real network)         │   ║
║  │  • Falcon key: pre-generated test key pair                   │   ║
║  │  • Reward address: known, decodable test address             │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  Harness Flow                                                │   ║
║  │                                                              │   ║
║  │  [1] Miner sends MINER_AUTH                                  │   ║
║  │      Node: creates container, registers in NodeSessionRegistry│   ║
║  │      Assert: MINER_AUTH_RESPONSE received                    │   ║
║  │      Assert: container.ValidateConsistency() == true          │   ║
║  │                                                              │   ║
║  │  [2] Miner sends MINER_SET_REWARD (encrypted reward hash)    │   ║
║  │      Node: decrypts with container.vChacha20Key              │   ║
║  │      Assert: REWARD_SET_OK received                          │   ║
║  │      Assert: container.fRewardBound == true                  │   ║
║  │      Assert: no ChaCha20 tag mismatch in log                 │   ║
║  │                                                              │   ║
║  │  [3] Miner sends GET_BLOCK                                   │   ║
║  │      Assert: NEW_BLOCK template received                     │   ║
║  │      Assert: nChannel, nHeight, nBits parsed correctly       │   ║
║  │                                                              │   ║
║  │  [4] Test stub solves PoW (trivial difficulty)               │   ║
║  │                                                              │   ║
║  │  [5] Miner sends SUBMIT_BLOCK (solved block + encrypted sig) │   ║
║  │      Node: decrypts, validates, applies reward               │   ║
║  │      Assert: BLOCK_ACCEPTED received                         │   ║
║  │      Assert: coinbase output == container.vRewardHash        │   ║
║  │      Assert: no BLOCK_REJECTED in log                        │   ║
║  │                                                              │   ║
║  │  [6] Teardown                                                │   ║
║  │      Assert: NodeSessionRegistry.size() == 0 after disconnect│   ║
║  │      Assert: no leaked recovery snapshots                    │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  GAIN: Any regression in the miner-to-node path is caught           ║
║        automatically in CI before manual testing.                    ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Preconditions (All Must Be Complete Before This Test Can Pass)

| Condition | Roadmap Item |
|---|---|
| Authoritative container at `MINER_AUTH` | PR #361 |
| `ValidateConsistency` at `SUBMIT_BLOCK` | R-02 |
| Canonical `CryptoContext` accessor | R-09 |
| Reward hash applied to coinbase | PR #363 |
| Packet-ingress preflight gate | R-14 |

---

## Acceptance Criteria

- [ ] `tests/unit/LLP/first_block_acceptance_test.cpp` created
- [ ] Test runs in CI without real PoW (trivial difficulty or mock solver)
- [ ] All six harness assertions pass
- [ ] Test fails with meaningful message if any precondition is missing
- [ ] Test runtime < 10 seconds on CI hardware
