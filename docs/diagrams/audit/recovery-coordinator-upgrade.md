# Diagram — Recovery Coordinator Upgrade (A1 / A1b)

**Status:** Implemented on NODE via PR #690 + #691  
**Code:** `AttemptPeerBestChainRecovery`, `RequestMissingTxBranchRecovery`, `PeerBestRecoveryResult`  
**Audit TIPs remaining:** TIP-01 (BESTCHAIN throttle), TIP-02 (BESTCHAIN unify)

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

## After (#690 + #691)

```
                         foreign best / incomplete block
                                    │
                                    ▼
                 ┌──────────────────────────────────────┐
                 │   RequestMissingTxBranchRecovery     │  « coordinator »
                 │   (missing-tx path; BESTCHAIN TBD)   │
                 └──────────────────┬───────────────────┘
                                    │
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
                         (NOT on QUEUED or THROTTLED)     « A1b »
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

## Residual asymmetry (TIP-01 / TIP-02)

```
   missing-tx path                     BESTCHAIN notify path
   ────────────────                    ─────────────────────
   coordinator ✅                      partial ❌
   throttle on recovery ✅             LIST unthrottled ❌
   fanout on far tip ✅                single notifying peer only
   result enum ✅                      bool fRecovered only

   Target: both arrows enter the same coordinator box.
```

---

## Acceptance (already on NODE)

- [x] Far tip issues LIST (no terminal silent no-op)
- [x] Missing-tx path does not double-LIST after FETCH_QUEUED/THROTTLED
- [x] On-wire unit coverage for LIST+TRANSACTIONS+BLOCK+LOCATOR
- [ ] BESTCHAIN shares throttle + coordinator (open TIP-01/02)
- [ ] Production soak signatures observed (open TIP-03)
