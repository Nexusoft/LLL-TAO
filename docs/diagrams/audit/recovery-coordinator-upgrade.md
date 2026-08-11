# Diagram — Recovery Coordinator Upgrade (A1 / A1b → BESTCHAIN)

**Status:** Implemented on NODE via PR #690 + #691 + #694 (+ near-tip inventory/orphan gates)  
**Code:** `AttemptPeerBestChainRecovery`, `RequestMissingTxBranchRecovery`, `RequestBestChainBranchRecovery`, `PeerBestRecoveryResult`  
**Series map:** [process-upgrade-series.md](process-upgrade-series.md) · [key fixes](../../current/node/audit/PROCESS_UPGRADE_KEY_FIXES.md)  
**Audit TIPs remaining:** TIP-03 soak; TIP-04 ForceLocalChainResync (spec)

---

## Before (#689 and earlier)

```
                    peer advertises foreign best
                              │
              ┌───────────────┴───────────────┐
              ▼                               ▼
     BESTCHAIN notify                  missing-tx exhausted
              │                               │
              ▼                               ▼
     LIST+TRANSACTIONS              AttemptPeerBestChainRecovery
     (unthrottled)                            │
                                  ┌───────────┴───────────┐
                                  ▼                       ▼
                           on disk?                  not on disk
                              │                       │
                              ▼                       ▼
                        activate path         in orphan pool?
                                              │ yes        │ no
                                              ▼            ▼
                                         walkback    LOG ONLY
                                         / gap LIST  "block-not-yet-received"
                                                         │
                                                         ▼
                                                    ★ SILENT NO-OP ★
                                                    (A1 wedge hours)
                              │
                              ▼
                    ALWAYS second LIST from caller
                    (window replace / throttle defeat)
```

---

## After (#690 + #691 + #694 + near-tip gates)

```
                    foreign best / incomplete block / BESTCHAIN
                                    │
              ┌─────────────────────┴─────────────────────┐
              ▼                                           ▼
 RequestMissingTxBranchRecovery          RequestBestChainBranchRecovery
         « A1b »                              « TIP-01/02 »
              │                                           │
              │                              near-tip inventory race?
              │                              (not on disk, not orphan,
              │                               delta≤1, matching BLOCK GET)
              │                                   yes → SKIP (no LIST)
              │                                   no  ──┐
              └─────────────────────┬───────────────────┘
                                    ▼
                 AttemptPeerBestChainRecovery
                                    │
              ┌─────────────────────┼─────────────────────┐
              ▼                     ▼                     ▼
         on disk               orphan walk            far tip / gap
      heavier activate      connectable Process     LIST+TRANSACTIONS
              │                     │               + optional fanout
              ▼                     ▼                     │
          PROGRESS              PROGRESS /                │
                                SKIPPED                   │
                                                          ▼
                                    ┌─────────────────────┴──────────────┐
                                    │ ShouldSendBranchSyncRequest(anc)?  │
                                    └──────────────┬─────────────────────┘
                                           yes     │      no
                                            ▼      │       ▼
                                     FETCH_QUEUED  │  FETCH_THROTTLED
                                            │      │       │
                                            └──────┴───────┘
                                                   │
                         fallback LIST only if result == SKIPPED
                         (NOT on QUEUED or THROTTLED)     « A1b / TIP-01 »
```

---

## PeerBestRecoveryResult state card

```
┌──────────────────┬────────────────────────────────────────────┐
│ SKIPPED          │ disabled / same tip / no peer / no action  │
│ PROGRESS         │ local best advanced                        │
│ FETCH_QUEUED     │ primary locator LIST on the wire           │
│ FETCH_THROTTLED  │ would fetch; 3s ancestor throttle hit      │
└──────────────────┴────────────────────────────────────────────┘
         FETCH_QUEUED ──┐
         FETCH_THROTTLED┤── suppress caller fallback LIST
         PROGRESS ──────┘
         SKIPPED ────────── allow throttled fallback LIST
```

---

## Near-tip shortcut (post-#694, inventory + orphan gates)

```
   missing-tx path                     BESTCHAIN notify path
   ────────────────                    ─────────────────────
   coordinator ✅                      coordinator ✅ (#694)
   throttle on recovery ✅             throttle ✅
   fanout on far tip ✅                fanout ✅
   result enum ✅                      result enum ✅

   Near-tip SKIP only when ALL hold:
     !on_disk && !mapOrphans && peer_at/ahead && delta≤1 && matching BLOCK GET
```

---

## Acceptance (already on NODE)

- [x] Far tip issues LIST (no terminal silent no-op)
- [x] Missing-tx path does not double-LIST after FETCH_QUEUED/THROTTLED
- [x] On-wire unit coverage for LIST+TRANSACTIONS+BLOCK+LOCATOR
- [x] BESTCHAIN shares throttle + coordinator (TIP-01/02 / #694)
- [x] Near-tip skip requires matching BLOCK inventory GET
- [x] Near-tip skip excludes tips already in mapOrphans
- [ ] Production soak signatures observed (open TIP-03)
