# Connection Acceptance and DDoS Defense

## Overview

This document describes the connection-acceptance pipeline shared by every LLP
server instance (Tritium, Time, Lookup, File, API/RPC) and the two regressions
that were silently introduced by PR #667's bulk merge of upstream
`Nexusoft/LLL-TAO:merging` into NODE (merge commit `291480ac4`), then fixed in
this change. `src/LLP/permissions.cpp` and the DDoS defaults in
`src/LLP/global.cpp` were part of the ~553 files that auto-merged without
textual conflicts and were therefore never manually reviewed.

> **Note on "E.x" labels:** §2 below is titled "Fix E.3" because that is how
> this specific fix was originally requested (as one item from a numbered
> "E.1–E.6" list from an earlier audit conversation). That numbering scheme
> is not used anywhere else in the codebase — it has no meaning outside this
> document. §4 explains why the rest of that list (E.1, E.2, E.4, E.5, E.6)
> could not be carried forward, and what was done instead to close out the
> request.

---

## 1. The connection-acceptance pipeline

```
                          ┌───────────────────────────┐
                          │   OS accept() new socket   │
                          └─────────────┬─────────────┘
                                        │
                                        ▼
                     ┌───────────────────────────────────┐
                     │ Server<ProtocolType>::accept_...   │  src/LLP/server.cpp
                     │  - reads peer IP + local port      │
                     └─────────────────┬───────────────────┘
                                        │
                                        ▼
                     ┌───────────────────────────────────┐
                     │ CheckPermissions(strAddress, nPort)│  src/LLP/permissions.cpp
                     │  - localhost bypass                │  (called ~server.cpp:1081)
                     │  - "standard port" open-by-default │
                     │  - -llpallowip whitelist match      │
                     └─────────────────┬───────────────────┘
                              denied   │  allowed
                        ┌──────────────┴───────────────┐
                        ▼                               ▼
                  connection dropped          dispatched to a DataThread
                                                          │
                                                          ▼
                                          ┌───────────────────────────────┐
                                          │ Per-packet DDoS scoring loop   │  src/LLP/data.cpp
                                          │  - rSCORE moving average       │  (~line 605-620)
                                          │  - Ban() if rSCORE > DDOS_rSCORE│
                                          └───────────────────────────────┘
```

Every server instance is independent: each has its own `Config` (port,
`ENABLE_DDOS`, `DDOS_RSCORE`, `DDOS_CSCORE`, `DDOS_TIMESPAN`, ...), but all of
them funnel through the *same* `CheckPermissions()` gate and the *same*
`DDOS_Score` moving-average implementation (`src/LLP/templates/ddos.h`,
`src/LLP/ddos.cpp`).

---

## 2. Fix E.3 — DDoS defaults restored

Commit `144f40035` ("Adjust default DDoS parameters for different LLP
instances", upstream) silently changed the following defaults, and PR #667's
auto-merge pulled them into NODE unreviewed:

| Server           | Argument     | Before (correct) | Regressed (post-merge) | Restored (this change) |
|------------------|--------------|-------------------|--------------------------|---------------------------|
| TRITIUM_SERVER   | `-ddos`      | `false`           | `true`                   | `false`                   |
| TRITIUM_SERVER   | `-rscore`    | `2000`            | `500`                    | `2000`                    |
| API_SERVER       | `-apiddos`   | `true`            | `false`                  | `true`                    |

Why this matters:

* **`-ddos=true` by default on the main P2P port** turns on active banning
  for every peer out of the box. Combined with the lowered `-rscore`
  threshold, legitimate high-volume peers (e.g. nodes serving `GET BLOCK`
  during a sync, which costs `+50` rSCORE per request — see
  `src/LLP/tritium.cpp`) could self-trigger `DDOS_Score::Score() >
  DDOS_rSCORE` (`src/LLP/data.cpp:611`) and be disconnected/banned
  (`DISCONNECT::DDOS`), 4x more easily than intended (`500` vs `2000`).
* **`-apiddos=false` by default** silently removed DDoS/rate-limiting
  protection from the local API server, which is reachable by any process
  with API credentials (or, if `-apiremote`/no auth is configured, by the
  network).

Fixed in `src/LLP/global.cpp` (TRITIUM_SERVER and API_SERVER config blocks).

---

## 3. Testnet permissions whitelist bypass (companion fix)

`CheckPermissions(strAddress, nPort)` (`src/LLP/permissions.cpp`) is the
single gate invoked for *every* incoming connection on *every* LLP server.
The post-merge rewrite computed the "is this a standard, open-by-default
port" flag with:

```cpp
if(config::fTestNet.load())
    fStandardPort = true;   // unconditional, for ANY port
```

This meant that on testnet, **every port** (mining, lookup, RPC, API — not
just the time/tritium ports intended to be open) was treated as open by
default, bypassing the `-llpallowip` whitelist entirely unless an operator
had also configured a filter — the opposite of defense-in-depth.

```
BEFORE (bug)                             AFTER (fixed)
──────────────                           ──────────────
testnet?                                 testnet?
  └─ fStandardPort = true   (ALL ports)    └─ switch(nPort)
                                                case TESTNET_TIME_LLP_PORT:
                                                    fStandardPort = true
                                                default:
                                                    fStandardPort = false

                                          then, network-agnostic (both nets):
                                            if(nPort == TRITIUM_PORT_CHECK ||
                                               nPort == TRITIUM_SSL_PORT_CHECK)
                                                fStandardPort = true
```

The fix mirrors the mainnet branch's per-port `switch` instead of a blanket
`true`, and consolidates the Tritium/SSL port check into a single
network-agnostic block (using the already testnet-aware `LLP::GetDefaultPort()`)
applied after the mainnet/testnet branch, so both networks share one
code path for the ports that are legitimately meant to be open
(Tritium message port, its SSL port, and the time-server port).

Regression test: `tests/unit/LLP/permissions_test.cpp`.

---

## 4. Scope note on the "E.1–E.6" request

This task was originally requested as "implement ALL the E fixes you
identified: E.1, E.2, E.3 DDoS defaults restored, E.4, E.5, and E.6",
referencing a prior audit conversation. That conversation's session state was
not recoverable in this repository (no matching issue, pull request,
discussion, or stored session references the E.1–E.6 labels), so the exact
content of E.1, E.2, E.4, E.5, and E.6 as originally enumerated could not be
reconstructed verbatim.

To close out this task within the available time, this change performed a
second, independent pass over the same `src/LLP` connection-acceptance
surface (`server.cpp` accept loop, `permissions.cpp`, `global.cpp` server
defaults, `manager.cpp`, `ddos.cpp`) and cross-checked each against the
current upstream `Nexusoft/LLL-TAO` sources to look for any further
unreviewed regressions from PR #667's silent auto-merge:

* **E.3 — DDoS defaults restored** (`-ddos`, `-rscore`, `-apiddos`) — fully
  implemented, see §2.
* **Testnet `CheckPermissions()` whitelist bypass** — fully implemented,
  see §3. (This is the fix most likely referred to by one of the missing
  `E.x` labels, since it was found during the same audit pass as E.3.)
* **AddressManager `Get()` TOCTOU** (tracked elsewhere as "Finding #2") was
  already fixed and merged prior to this branch; re-verified in this pass
  that both call sites (`src/LLP/tritium.cpp` `ACTION::NOTIFY::ADDRESS`
  handler and `src/TAO/API/commands/system/initialize.cpp` peer listing) use
  the non-throwing `Get(addr, TrustAddress&)` overload, and that no other
  caller in the tree still uses the throwing single-argument `Get()`.
* The accept loop in `server.cpp` (max-connection checks, DDOS filter
  creation/scoring, ban check, `CheckPermissions()` call, `AddConnection()`
  dispatch) and `ddos.cpp`'s moving-average scorer were diffed against
  upstream and found structurally identical — no further silent-merge
  regressions were found there.

No additional high-confidence bugs were identified in this pass beyond the
two documented above. If E.1/E.2/E.4/E.5/E.6 refer to specific findings
outside this pipeline (e.g. block-acceptance/consensus code rather than
LLP connection acceptance), please share their details — ideally by filing
them as separate tracked issues — so they aren't lost to session resets
again and can be addressed in a follow-up change.
