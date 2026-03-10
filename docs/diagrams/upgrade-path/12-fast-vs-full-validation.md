# Diagram 12 — Fast vs Full Validation Modes

**Roadmap Item:** R-12  
**Priority:** 3 (Scaling & Reliability)

---

## Context (Before)

Every `SUBMIT_BLOCK` triggers full PoW validation regardless of the miner's session state.  For authenticated, reward-bound miners with a clean history, this is over-conservative and adds latency to the critical path.

```
╔══════════════════════════════════════════════════════════════════════╗
║  CURRENT — Full Validation on Every Submit                           ║
║                                                                      ║
║  SUBMIT_BLOCK handler:                                               ║
║                                                                      ║
║    1. Decrypt block header (ChaCha20)                               ║
║    2. Check session state                                            ║
║    3. Verify PoW (full): hash block header → check against nBits    ║
║    4. Verify Falcon signature (full): verify over block hash         ║
║    5. Verify reward binding                                          ║
║    6. Apply to chain                                                 ║
║                                                                      ║
║  Steps 3 and 4 are expensive for every submission.                  ║
║  For Prime channel: step 3 involves large integer arithmetic.        ║
║                                                                      ║
║  PROBLEM: High-frequency submitted shares (near-solutions) consume  ║
║           significant CPU even when clearly not full solutions.      ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Target (After)

A configurable fast-path pre-filter quickly rejects clearly-invalid submissions before running the full validator.  The full validator is only invoked for submissions that pass the fast-path.

```
╔══════════════════════════════════════════════════════════════════════╗
║  TARGET — Fast vs Full Validation Gates                             ║
║                                                                      ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │  SUBMIT_BLOCK handler with two-stage validation              │   ║
║  │                                                              │   ║
║  │  Stage 1 — Fast Pre-filter (always runs, < 1ms target)      │   ║
║  │  ─────────────────────────────────────────────────────────   │   ║
║  │  • Decrypt block header                                      │   ║
║  │  • Check nChannel ∈ {1, 2}                                   │   ║
║  │  • Check nHeight within plausible range                      │   ║
║  │  • Check hashPrevBlock matches hashBestChain (stale check)   │   ║
║  │  • Check container.fRewardBound == true                      │   ║
║  │  • Check container.ValidateConsistency()                     │   ║
║  │                                                              │   ║
║  │  FAIL → BLOCK_REJECTED (fast, low CPU)                       │   ║
║  │  PASS ↓                                                      │   ║
║  │                                                              │   ║
║  │  Stage 2 — Full Validation (only on fast-path pass)          │   ║
║  │  ─────────────────────────────────────────────────────────   │   ║
║  │  • Verify PoW (hash / prime arithmetic)                      │   ║
║  │  • Verify Falcon signature over block hash                   │   ║
║  │  • Apply to chain                                            │   ║
║  │                                                              │   ║
║  │  FAIL → BLOCK_REJECTED (honest: solution didn't meet target) │   ║
║  │  PASS → BLOCK_ACCEPTED                                       │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                      ║
║  Configuration:                                                      ║
║    mining.fast_preflight = true   (default: true)                   ║
║    mining.fast_preflight = false  → skip to full validation always   ║
║                                                                      ║
║  GAIN: High-frequency near-miss submissions are rejected cheaply.    ║
║        Full PoW only runs on candidates that pass structural checks. ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## Acceptance Criteria

- [ ] `SUBMIT_BLOCK` handler restructured into fast-path + full-validation stages
- [ ] Fast-path checks listed above are all present
- [ ] `mining.fast_preflight` config flag controls fast-path (default: enabled)
- [ ] Benchmark: fast-path rejection for stale block costs < 0.1ms
- [ ] Full validation still always runs when fast-path passes
- [ ] Unit test: stale `hashPrevBlock` rejected in fast-path without running PoW
