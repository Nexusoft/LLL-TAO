# Recovery Merge Model

**Section:** Node Architecture  
**Version:** LLL-TAO 5.1.0+ (post-PR #361)  
**Last Updated:** 2026-03-10

---

## Overview

When a miner TCP connection drops and the miner reconnects, the node must decide whether existing session state can be safely restored.  The **recovery merge model** describes how the node finds, validates, and merges a previously-stored session snapshot into the new live container.

---

## Recovery Pipeline

```
  Miner reconnects (new TCP connection)
          │
          ▼
  ┌────────────────────────────────────────────────────────────┐
  │  MINER_AUTH handler                                        │
  │                                                            │
  │  1. Verify Falcon signature on new connection              │
  │     → produces strFalconKeyID, hashGenesis                 │
  │                                                            │
  │  2. Query SessionRecoveryManager                           │
  │     → primary key:   strFalconKeyID                        │
  │     → secondary key: strRemoteAddress (only if no hit)     │
  │                                                            │
  │  3. If snapshot found: ValidateRecovery()                  │
  │     → checks snapshot.strFalconKeyID == new strFalconKeyID │
  │     → checks snapshot.hashGenesis    == new hashGenesis    │
  │     → calls snapshot.ValidateConsistency()                 │
  │                                                            │
  │  4a. Validation PASS → MergeContext(snapshot)              │
  │      → copies vChacha20Key, vRewardHash, fRewardBound,     │
  │        fAuthenticated (sticky), eLane                      │
  │      → new nSessionID replaces old (new session epoch)     │
  │                                                            │
  │  4b. Validation FAIL → log + discard snapshot              │
  │      → fresh container, miner must re-bind reward          │
  │                                                            │
  │  5. Register new container in NodeSessionRegistry          │
  │  6. Send MINER_AUTH_RESPONSE                               │
  └────────────────────────────────────────────────────────────┘
```

---

## Identity-First Recovery Key Hierarchy

The node uses a strict key priority for recovery lookups:

```
  Priority 1 (strongest): Falcon key fingerprint
  ─────────────────────────────────────────────────
  Derived from verified Falcon-1024 public key.
  Unique per owner identity.
  Cannot be forged without the private key.

  Priority 2 (weaker): Remote address
  ─────────────────────────────────────────────────
  Used only when no Falcon-key hit is found.
  Subject to NAT/proxy collision for multi-miner setups.
  When used, result is still cross-validated against
  snapshot.strFalconKeyID before merge proceeds.
```

This ordering means a miner's identity always drives recovery, not its network location.

---

## MergeContext Rules

`MinerSessionContainer::MergeContext(const MinerSessionContainer& incoming)` applies the following rules:

| Field | Rule |
|---|---|
| `nSessionID` | **New connection wins** — always use the freshly-negotiated session ID |
| `hashGenesis` | **Must match** — abort merge if different |
| `strFalconKeyID` | **Must match** — abort merge if different |
| `vChacha20Key` | **Recovered wins** — restore from snapshot so existing reward binding remains valid |
| `vRewardHash` | **Recovered wins** — restored unconditionally if `fRewardBound` is true in snapshot |
| `fRewardBound` | **Sticky true** — once bound, cannot be un-bound by a merge |
| `fAuthenticated` | **Sticky true** — once authenticated, a merge from an authenticated snapshot keeps it true |
| `eLane` | **Recovered wins** — preserve original lane designation |
| `nLastActivity` | **New wins** — set to current time on merge |

---

## Reconnect Accounting

After every successful recovery merge, the node logs a structured reconnect accounting entry:

```
[RECOVERY] miner reconnected
  falcon_key_id   = <fingerprint>
  genesis_hash    = <hex>
  old_session_id  = <u32>
  new_session_id  = <u32>
  reward_restored = YES/NO
  lane            = STATELESS/LEGACY
  recovery_key    = FALCON/ADDRESS
```

This log line is the operator's primary signal that a miner recovered cleanly rather than starting fresh.

---

## Failure Modes and Safe Fallback

| Failure condition | Node action |
|---|---|
| No snapshot found | Fresh container; miner must send `MINER_SET_REWARD` again |
| Snapshot found but Falcon mismatch | Discard snapshot; treat as fresh |
| Snapshot found but `ValidateConsistency()` fails | Discard snapshot; treat as fresh |
| Snapshot found but `hashGenesis` mismatch | Discard snapshot; treat as fresh |
| `MergeContext` aborts mid-merge | Roll back to empty container; treat as fresh |

In every failure path the node does **not** terminate the connection.  It continues with an empty container, which forces the miner to re-authenticate reward binding.

---

## Planned: SessionConflictResolver (Roadmap Item 11)

Currently two reconnecting miners with the same Falcon key ID but different `hashGenesis` values will both fail recovery independently.  A future `SessionConflictResolver` component will:

1. Detect the conflict before any merge attempt.
2. Log the full set of conflicting snapshots.
3. Apply a deterministic resolution rule (newer `nLastActivity` wins, or block mining for both until the conflict is resolved out-of-band).

See [Diagram 11 — SessionConflictResolver](../../diagrams/upgrade-path/11-session-conflict-resolver.md) and the [Roadmap](roadmap-upgrade-path.md) for details.

---

## Related Pages

- [Session Container Architecture](session-container-architecture.md)
- [Roadmap & Upgrade Path](roadmap-upgrade-path.md)
- [Diagrams: Live Container vs Recovery Snapshot (Diagram 4)](../../diagrams/upgrade-path/04-live-container-vs-recovery-snapshot.md)
- [Diagrams: Identity-First Recovery Keys (Diagram 5)](../../diagrams/upgrade-path/05-identity-first-recovery-keys.md)
- [Diagrams: Scoped Update Guard (Diagram 6)](../../diagrams/upgrade-path/06-scoped-update-guard.md)
- [Diagrams: SessionConflictResolver (Diagram 11)](../../diagrams/upgrade-path/11-session-conflict-resolver.md)
