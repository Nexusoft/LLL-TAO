# Known Risks and Limitations — Mining Infrastructure

**Version:** LLL-TAO 5.1.0+  
**Last Updated:** 2026-11-06

**Status:** Living document — update as risks are mitigated or new ones identified

---

## Chain Connectivity and Mining Readiness

For the incremental scope and recovery/admission diagram covering PRs
#728–#730, see the [recent changes summary](recent-changes-summary.md#peer-recovery-and-mining-readiness-prs-728730).

**Deployment status:** These changes require controlled multi-node testing
before live deployment. Unit regressions cannot demonstrate behavior during
real partitions, peer churn, or prolonged recovery.

**Build requirement:** Use a full clean rebuild. `TritiumBlock` and
`TritiumNode` gained memory-only fields; mixing old and new object layouts is
unsafe.

Verification covers a forced unit rebuild, production-mode syntax checks,
and 143 selected mining/ledger regression cases (12,572 assertions), including
the formerly crashing mempool suite. The prewarmer and solo-stake fixtures
now use the current reward-type, worker-lifecycle, and trust-read semantics.
The related prewarmer, staking, template, readiness, and mempool selection
passed 57 cases; a clean full-suite result is still outstanding.

Prewarmer requests coalesce while the same channel, reward, and tip are queued
or being built. Failed, stale, and completed requests release their in-flight
key, allowing a later notification to retry; a notification coalesced during a
failed build does not itself schedule another attempt. The `warmed` counter and
log now count only non-null template results. Warming remains best-effort and
does not replace the mining-readiness or stale-work checks.

Mining now has a separate readiness gate; the existing synchronization
predicate and its consensus proof-verification fast paths are unchanged.
Both external mining protocols and staking pause when the gate is closed.
Private/hybrid channel-3 production is unchanged.

On public networks, readiness requires initial synchronization to have
completed, a consistent committed local tip, and fresh supporting observations
from **two distinct peer IP addresses** by default. A supporting report must
identify a locally stored, validated main-chain block no more than two heights
behind the current tip, with both the returned block hash and advertised height
matching the report.
Fresh unknown tips, mismatched hash/height reports, and locally known heavier
branches veto readiness even if other peers support the local chain. Known
non-heavier branches do not veto readiness, but do not count as support.

For an intentional single-peer deployment, set `-miningminpeers=1`; this is an
explicit reduction in network evidence, not authentication of that peer.
Values 1 through 16 are accepted; other values fail closed. Local-development
testnets (`-testnet` with `-dns=0`, or hybrid testnet mode) bypass the peer and
initial-sync requirements, but not shutdown/reorganization checks. Neither a
recent tip nor `-sync=0` bypasses public-network evidence requirements.

Best-chain evidence expires after **90 seconds**. Core requests current
`BESTCHAIN`/`BESTHEIGHT` reports every **30 seconds**, including from idle peers,
using the existing subscription protocol. Disconnect/destruction removes that
connection's observations. A height report alone does not refresh the age of
its best-chain evidence. Older peers send the two fields in separate
notifications; NODE5 pairs those fields only within 10 seconds and consumes
each pair once. A contradictory unpaired field immediately makes the peer's
existing evidence unresolved and revokes work until a complete pair arrives.
Split reports are not an atomic peer snapshot, so the resulting hash and
height still undergo the same local block and chain checks. A peer that stops
answering either field will still lose freshness.

An in-memory work epoch revokes existing work when observations change in
potentially unsafe ways, disconnect, or expire, or when a readiness check
detects a ready-to-paused transition. Identical timely reports do not revoke
work. A new or updated report at the locally committed tip does not revoke
work if the hash and height are verified against the stored block and the
local tip is stable; an address change still revokes. This avoids cancelling
same-tip work as peers catch up after a block. Other changes, including unknown
or mismatched tips, still revoke immediately even if repaired before the next
readiness check. Both mining lanes
check the epoch before issuing templates and validating submissions, and
local block admission checks again under the processing mutex immediately
before acceptance. Staking checks while hashing and before admission.
Readiness restoration does not revalidate revoked work.

Template cache publication checks both the build tip and the work epoch.
Its diagnostics distinguish a tip change from an epoch revocation; neither
condition permits publishing the old build.

Shared and connection-local caches reject revoked epochs; in-flight builds
cannot publish them as current. On public networks the producer extra nonce
uses the epoch in its upper 32 bits and the caller's nonce in its lower 32 bits
(larger caller nonces are rejected, not silently truncated),
so rebuilding at the same tip after an interruption cannot resurrect an old
merkle root. Epoch exhaustion pauses mining. Epochs are process-local and are
not serialized, hashed, or trusted from network blocks; restarting also drops
the server's issued-template maps. The 216-byte mining header and consensus
block format are unchanged.

Paused GET_BLOCK requests use the existing retryable `TEMPLATE_NOT_READY`
control with a 1000 ms hint. Template responses and deferred pushes are checked
again at the write boundary, after waiting for the flush mutex. A revocation
prevents queued templates from updating delivery markers; those markers are
published only after a successful guarded write. Guarded writes and marker
publication are serialized per connection, including synchronous GET_BLOCK
responses, so concurrent delivery callbacks cannot overwrite newer markers
with older ones. Stateless recovery templates
use the same guarded queue. A revocation
after that point can still race with a write, and bytes already sent or buffered
cannot be recalled. There
is no miner-side cancellation message, so a template issued while current may
later be rejected if its epoch is revoked. `system/get/info` includes `mining_readiness` with `ready`,
`reason`, `epoch`, `fresh_peers` (supporting distinct IPs), `required_peers`, and
`local_bypass`. Existing `synchronized`/`syncing` fields are unchanged and need
not agree with mining readiness; no Interface display change is included.

Distinct IPs are not proof of independent peers or the global best chain.
An unresolved minority report can deliberately pause mining, an availability
tradeoff. Admission uses a final point-in-time check, not an atomic guarantee
against connectivity changing immediately afterward.

Sync completion uses the greatest recent `BESTHEIGHT` advertised by connected
peers. Observations are removed on disconnect and
expire 30 minutes after their last height notification. Periodic report
requests allow responsive idle peers to refresh their observations. Sync
completion is re-evaluated by generic events, not just block notifications,
so removal/expiry of an obstructing height does not require another block.
Completion requires the selected session's recently observed best hash to
match the active tip and a matching height report no older than 90 seconds.
Selection and completion coordinate session ownership so a stale attempt cannot reopen
completed sync or clear another selected session. A connected peer can
still advertise an erroneous height; repeated excessive advertisements can
still delay sync completion. No peer height or peer agreement proves that a
chain is best. Fork activation remains subject to local validation and chain
weight.

When `-peerbestchainrecovery` is enabled, a connected peer's unresolved best
tip is retried at most once per 30 seconds, including an unknown shorter tip.
Each changed advertised hash gets a fresh per-tip grace period, capped by the
30-second deadline for a continuously unresolved mismatch. Hash changes do not
reset the five-second body retry or this cap; repeated reports preserve all
deadlines. Local resolution enables a fresh grace period for the next mismatch.
Orphan and larger-gap recovery remain immediate.
Near-tip unknowns are requested directly as blocks, including when a refresh
contains no block inventory. Orphans and larger gaps still enter branch recovery.
Retries stop 30 minutes after the last best-chain advertisement. A successful
inventory request is not assumed to have delivered the block. Unknown shorter
branches are fetched for local validation and channel-weight comparison,
not activated by height or peer agreement.

Coinbase credits that appear immature at the next local height, using the
current tip's maturity schedule, are retained in the bounded conflict pool for
re-evaluation as the local chain advances. The next block's version can change
the schedule at an upgrade; block validation remains authoritative.
Peer-advertised heights do not make a credit mature or change block validation.
Deferred roots and their dependent sigchain tails remain queued for bounded
reconciliation; eviction still applies. An orphan request with no available
connected peer is logged and left queued rather than dereferencing a null
connection.
Orphan admission distinguishes an available live/confirmed predecessor from
an entry that is merely known in a parked queue. When the predecessor arrives,
queued children are actually revalidated and admitted; their orphan indexes
are removed only on successful admission. Conflict-root deferral remains
separate from ordinary missing-predecessor handling.

---

## 1. Platform Limitation: Linux-Only Epoll

**Risk Level:** Medium (production) / Low (development)

The dedicated epoll I/O path for mining DataThreads is **Linux-only**.  Non-Linux
platforms (macOS, Windows) fall back to `poll()` with a reduced 10 ms timeout.

| Platform | I/O Mechanism | Timeout | Complexity |
|----------|--------------|---------|------------|
| Linux | `epoll_wait()` | 1 ms | O(ready) |
| macOS | `poll()` | 10 ms | O(n) |
| Windows | `WSAPoll()` | 10 ms | O(n) |

**Impact:** Development and testing on macOS/Windows will have higher mining
I/O latency.  Production mining nodes **must run on Linux** for optimal
performance.

**Mitigation:** The poll fallback is functional — miners will work on any
platform, just with higher latency.

---

## 2. CONNECTIONS Vector Contention

**Risk Level:** Low–Medium

Mining connections share the LLP's `CONNECTIONS` vector with slot management via
`atomic_lock_unique_ptr`.  Two concurrent access patterns exist:

- **ListeningThread** → `AddConnection()` → `epoll_register()` → writes to slot
- **DataThread** → `remove_connection()` → `epoll_deregister()` → clears slot

Under high connection churn (rapid connect/disconnect cycles at >100 events/sec),
the spinlock-based atomic pointer could cause contention between these threads.

**Impact:** Latency spikes during connection storms (pool restart, miner
reconnect waves).

**Mitigation:** Not yet observed in production.  If it occurs, consider:
1. Per-DataThread connection lists (eliminate shared vector)
2. Lock-free slot allocation with CAS
3. Escalate to full MiningSocket route (see ADR)

---

## 3. Shared FLUSH_THREAD

**Risk Level:** Low

The outgoing packet drain thread (`DrainOutgoingQueue()` via `FLUSH_CONDITION`)
is shared across all connection types.  Mining push notifications enqueued via
`QueuePacket()` are drained by the same thread that drains P2P and API packets.

**Impact:** If P2P traffic has large queued packets (e.g., block relay, inventory
batches), mining push notifications may experience millisecond-level delays.

**Mitigation:** PR #536 decoupled template building from socket writes.  The
`fHasOutgoing` atomic flag provides lock-free predicate evaluation.  If delays
persist, a dedicated mining FLUSH_THREAD could be added.

---

## 4. Health Sweep O(n) Cost

**Risk Level:** Low

Every 250 ms, `ThreadEpoll()` performs a health sweep across **all** mining
connections (not just those with events).  At N connections, this is O(N) work
per sweep.

| Connections | Sweep Cost (estimated) |
|-------------|----------------------|
| 50 | ~0.05 ms |
| 500 | ~0.5 ms |
| 5000 | ~5 ms |

**Impact:** At very high connection counts (5000+), the health sweep could
add noticeable latency to the epoll loop.

**Mitigation:** Current mining scale is <500 concurrent connections.  If scaling
beyond 1000, consider:
1. Staggered health sweeps (check 1/N connections per cycle)
2. Separate health-check thread
3. Event-driven health checks (triggered by timer FD in epoll)

---

## 5. epoll_ctl Thread Safety Edge Cases

**Risk Level:** Low

`EPOLL_CTL_ADD` (from ListeningThread) and `EPOLL_CTL_DEL` (from DataThread)
can execute concurrently on the same epoll instance.  The Linux kernel guarantees
this is safe — `epoll_ctl` acquires an internal mutex per epoll instance.

**Impact:** Under rapid connect/disconnect cycles, the kernel-internal mutex
could become a bottleneck.  Additionally, a race between `EPOLL_CTL_DEL` and
`close(fd)` could theoretically cause the epoll instance to reference a
recycled FD.

**Mitigation:** Current production traffic patterns do not trigger this.  The
`epoll_deregister()` call always precedes `close()` in the disconnect path.

---

## 6. Session Recovery Removed — Full Re-Auth Required

**Risk Level:** Low (design decision)

`SessionRecoveryManager` was removed in PR #532.  On disconnect, miners must
perform a full Falcon re-authentication handshake.  There is no session
resumption across reconnects.

**Impact:** Miners that disconnect and reconnect (e.g., network glitch) must
perform a full Falcon handshake (~2–5 ms for Falcon-512, ~10–20 ms for
Falcon-1024) before receiving new block templates.

**Mitigation:** This is an intentional design decision — simpler and more secure.
The handshake time is negligible compared to block find times (minutes to hours).

---

## 7. Template Storm on Rapid Chain Tip Changes

**Risk Level:** Low (mitigated by PR #538)

When multiple blocks are found in rapid succession (e.g., Hash blocks at 50-second
intervals), each tip change triggers template generation for all connected miners.

**Impact:** Brief CPU spikes during rapid tip changes.  Mostly mitigated.

**Mitigation:** PR #538 added a global rate limiter and live context reads to
prevent redundant template generation.  PR #539 added rate-limit-before-read
for GET_BLOCK requests.

---

## 8. Dual Server Process — Not Yet Unified at Socket Level

**Risk Level:** Informational

The mining infrastructure runs **two separate server instances**:
- `STATELESS_MINER_SERVER` → `Server<StatelessMinerConnection>` (port 9323)
- `MINING_SERVER` → `Server<Miner>` (port 8323)

Both share configuration via `MiningServerFactory` but maintain separate
DataThread pools, connection storage, and FLUSH_THREADs.

**Impact:** Double the resource usage (threads, memory) compared to a single
unified server.  However, this also provides natural isolation between protocol
lanes.

**Future:** The `unified-mining-server-architecture.md` design document outlines
a future protocol adapter pattern that would consolidate to a single server.

---

## Risk Escalation Triggers

Monitor the following metrics in production.  If thresholds are exceeded,
consider escalating to the full MiningSocket route:

| Metric | Threshold | Action |
|--------|-----------|--------|
| Template delivery p99 latency | >10 ms | Investigate CONNECTIONS contention |
| FLUSH_THREAD drain latency | >5 ms | Add dedicated mining FLUSH_THREAD |
| Health sweep duration | >2 ms | Implement staggered sweeps |
| Connection churn rate | >100/sec | Investigate spinlock contention |
| Miner-reported stale blocks | >1% | Full diagnostic investigation |

---

## Related Documentation

- [Linux Epoll Mining Architecture](linux-epoll-mining-architecture.md)
- [Dedicated DataThread Decision Record](dedicated-datathread-decision.md)
- [Recent Changes Summary](recent-changes-summary.md)
