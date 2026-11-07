PeerRecovery, PeerBestRetry and ChainState::MiningReadiness
=========================================================

This guide explains the integrated Core upgrades from PRs #728–#733 using the
C++ classes and functions that implement them. Read it in execution order:
discover peers, collect evidence, recover and commit the chain, then admit
current mining work. These are separate decisions: **connected does not mean
synchronized, and synchronized does not mean ready to mine**.

Runtime integration
-------------------

::

    Server / AddressManager
      PeerRecovery -> known addresses / fresh DNS -> connected peers
                                                        |
                                    TritiumNode::ProcessPacket
                                  BESTCHAIN + BESTHEIGHT reports
                                                        |
                       +--------------------------------+------------------+
                       |                                |                  |
                 PeerHeightTracker                PeerBestRetry     MiningPeerTracker
                 connected heights                unresolved tip    fresh tip evidence
                       |                                |                  |
              TritiumNode::TryFinalizeSync       block GET / branch LIST    |
                       |                                |                  |
                       |                     Process -> BlockState::SetBest|
                       |                                |                  |
                       |                        LLD::TxnCommit             |
                       |                                |                  |
                       |                    publish connected BlockState  |
                       |                                |                  |
                       +--------------------+-----------+------------------+
                                            |
                               ChainState::MiningReadiness
                                    ready + current epoch
                                            |
                        templates / prewarmer / mining lanes / staking
                                            |
                              MiningWorkCurrent + final admission check

1. PeerRecovery: restore transport, not consensus
------------------------------------------------

``PeerRecovery`` schedules bounded, jittered DNS recovery alongside ordinary
known-address connection attempts. Its backoff is capped at 300 seconds.
Fresh DNS lookup results preserve live address state, and only successful
refreshes update success reporting. ``CanAttemptOutgoing`` enforces the
manager's capacity check; ``SelectConnectedPeer`` excludes disconnected entries.
``Socket::Attempt`` handles immediate success and checks asynchronous completion
before accepting a connection.

``DataThread`` initializes its resources before starting workers and coordinates
shutdown with the condition-variable wait mutexes. ``TimeNode`` sends outgoing
connect/drop updates to its owning server's ``AddressManager``, rather than a
different server's peer table. Colin reporting uses ``ColinReportBoxLine`` for
long lines and joins reporting workers before teardown.

Operator signal: ``system/get/info.peer_recovery`` describes connection
recovery only. An OS DNS call can still delay shutdown, and independent direct
connection paths do not share an atomic total-admission limit with the manager.
These Core upgrades do not change the Interface wallet.

2. PeerHeightTracker and MiningPeerTracker: two freshness policies
----------------------------------------------------------------

``ChainState::ObservePeerHeight`` records each connection's height;
``ForgetPeerHeight`` removes disconnected observations. ``PeerHeightTracker``
expires height reports after 30 minutes, avoiding a permanently sticky maximum.
BESTCHAIN recovery deferred within a NOTIFY packet uses the accompanying
BESTHEIGHT when present.

``TritiumNode::TryFinalizeSync`` rechecks completion during generic events,
not only when blocks arrive. The selected sync session must have a recently
observed best hash matching the local tip and a matching height report no older
than 90 seconds. Selection/completion also coordinate session ownership.

Mining evidence has a stricter lifetime: ``MiningPeerTracker::MAX_AGE`` is
90 seconds. Height-only notifications do not refresh best-hash evidence.
``PeerBestRetry::RefreshDue`` requests BESTCHAIN/BESTHEIGHT reports at 30-second
intervals, including from idle peers. Neither heights nor peer agreement prove
which chain is best; local block validation and chain weight remain authoritative.

3. PeerBestRetry and BlockInventoryRequests: fetch before escalating
------------------------------------------------------------------

::

    BESTCHAIN(hash)
      |
      +-- first mismatch -- Observe(): initialize body and branch deadlines
      +-- still mismatched- Observe(): refresh age, preserve deadlines
      +-- local best ------ Resolved(): disable body and branch retries
      |
      +-- unknown, not orphan, peer at/ahead within one height
      |      matching inventory GET? -> retain existing reservation
      |      otherwise -------------> QueueBlockGet(hash)
      |
      +-- orphan / larger gap / eligible known branch
      |      -> RequestBestChainBranchRecovery / AttemptPeerBestChainRecovery
      |
      +-- generic tick, still unresolved and fresh
             BlockDue at 5s -> near-tip body GET only
             Due at 30s ---> branch recovery, including unknown shorter tips

The five-second body interval and 30-second branch grace are independent.
Changing unresolved tip A to B preserves A's deadlines, even if A was overdue.
Repeated reports and hash churn cannot postpone retries during a continuous
mismatch. Resolution as the local best enables a fresh grace period for the
next mismatch. Report-refresh scheduling is independent of both timers; retries
expire 30 minutes after the last BESTCHAIN observation. Orphan and larger-gap
notification recovery are not grace-delayed.

``QueueBlockGet`` shares ``BlockInventoryRequests`` with inventory handling.
Reservations are atomic per hash/peer and expire after five seconds. Each of at
most 512 tracked peers can reserve eight hashes, bounding active entries to
4096; a hash can be requested from at most 32 peers. One peer cannot consume
another's active quota. Different peers can each hold a reservation for the same
hash; this is not a global one-request-per-hash prohibition.

Capacity and duplicate results are distinct. Capacity-limited inventory enters
a bounded per-peer queue of 128 hashes and retries on generic events at
five-second intervals, without requiring a BESTCHAIN announcement. Failed
inventory sends remain queued for retry; queue/tracked-peer exhaustion is an
explicit connection failure, not a silently discarded notification. Block
responses and peer teardown remove tracking. Client GETs include
``SPECIFIER::CLIENT``.

Branch recovery uses the coordinator's throttle and result enum, preserving
``SPECIFIER::TRANSACTIONS`` on post-sync LIST requests. Queued/throttled recovery
suppresses duplicate fallback LISTs. A queued request is not proof of delivery,
and fetching a branch is not activation.

4. BlockState::SetBest: publish the staged result after commit
-------------------------------------------------------------

::

    PROCESSING_MUTEX + transaction ownership
      -> stage Disconnect / Connect / block links / indexes
      -> read the updated candidate as stateCommitted
      -> LLD::TxnCommit(FLAGS::BLOCK, INSTANCES::CONSENSUS)
           failure -> no new chain publication; databases may be inconsistent
           success -> publish stateCommitted, then deferred side effects

``Connect`` changes monetary totals; rewinds clear successor links.
Consequently ``SetBest`` publishes the updated staged ``BlockState``, not the
original candidate. Mempool mutations and related side effects are deferred
until the commit succeeds. Hash-keyed ``LedgerDB::ReadBlock`` checks the returned
block identity before publishing it, including the atomic overload.

Journal replay also preserves erase-then-write ordering: subsequent writes
remove their keys from ``setErasedData`` so the later write survives.
**This is not crash-safe cross-database atomicity.** ``LLD::TxnCommit`` commits
databases separately. Owned physical commits create a ``commit-incomplete``
directory in the data directory before applying changes. A checkpoint, commit,
release or marker-removal failure requests shutdown, rejects subsequent
transaction begins and best-chain transitions, and retains the marker and
remaining journals rather than publishing an unconfirmed tip. Restart refuses
the marker before journal recovery can discard evidence. Journal replay commit
failures also retain the fence and stop startup. Restore a known-good
backup or rebuild the chain databases; do not simply remove the marker and
assume the databases agree. Successful commits release journals and remove the
marker. This conservative fence does not add fsync guarantees or coordinated
durable recovery; a crash-safe cross-database protocol remains outstanding.

5. ChainState::MiningReadiness: fence work against the committed tip
------------------------------------------------------------------

Public-network readiness requires completed initial sync, a consistent committed
tip, no shutdown/reorganization, and fresh support from two distinct peer IPs by
default. ``-miningminpeers`` permits 1–16; invalid values fail closed.
``MiningReadiness`` resolves each reported hash/height locally and checks ancestry
against its best-state snapshot, accepting support at most two heights behind.
Unknown or mismatched tips and known heavier branches veto readiness.
Known non-heavier side branches are not supporting evidence.

``MiningWorkCurrent(epoch)`` requires readiness and the current nonzero epoch.
Changed/disconnected/expired evidence and ready-to-paused transitions revoke
work. Templates, caches, both external mining lanes, staking and local admission
recheck work; final local admission checks under the processing mutex.
Restored readiness does not resurrect revoked work. Epochs are process-local,
not consensus fields; the mining header and network block format are unchanged.
Already sent bytes cannot be recalled: paused GET_BLOCK uses
``TEMPLATE_NOT_READY``, and stale submissions are rejected.

Local-development testnets can bypass peer/initial-sync requirements, but not
shutdown/reorganization checks. Private/hybrid channel-3 production is unchanged.
``ChainState::Synchronizing`` remains separate from this mining policy.
Operator signal: ``system/get/info.mining_readiness`` reports readiness, reason,
epoch, fresh/required peers and local bypass.

``MiningTemplatePrewarmer`` coalesces channel/reward/tip requests while queued
or in flight, releases keys on completion/failure/staleness, and counts only
non-null builds as warmed. It is best-effort, not permission to mine.
Immature coinbase credits and dependent sigchain tails remain in the bounded
conflict pool for local-height re-evaluation; peer heights cannot mature them.
Missing-predecessor orphans revalidate on arrival, and requests with no connected
peer remain safely queued.

Source map and upgrade provenance
---------------------------------

* `PR #728 <https://github.com/NamecoinGithub/LLL-TAO/pull/728>`_: transport recovery;
  ``src/LLP/include/peer_recovery.h``, ``manager.cpp``, ``hosts.cpp``,
  ``socket.cpp`` and ``data.cpp`` under ``src/LLP/``.
* `PR #729 <https://github.com/NamecoinGithub/LLL-TAO/pull/729>`_: height freshness
  and Colin lifecycle; ``src/TAO/Ledger/include/peer_height.h``,
  ``src/LLP/tritium.cpp`` and ``src/LLP/colin_mining_agent.cpp``.
* `PR #730 <https://github.com/NamecoinGithub/LLL-TAO/pull/730>`_: readiness,
  epoch fencing and prewarming; ``src/TAO/Ledger/include/mining_readiness.h``,
  ``chainstate.cpp``, ``create.cpp`` under ``src/TAO/Ledger/``, and
  ``src/LLP/template_prewarmer.cpp``.
* `PR #731 <https://github.com/NamecoinGithub/LLL-TAO/pull/731>`_: development
  environment, not a runtime arrow. ``.github/workflows/copilot-code-review.yml``
  provisions an Ubuntu runner with read-only contents permission and invokes
  ``contrib/devtools/install-build-deps.sh``. Automatic review requests require
  separate settings; this workflow does not configure them.
* `PR #732 <https://github.com/NamecoinGithub/LLL-TAO/pull/732>`_: near-tip GET
  grace and retry coordination; ``src/LLP/include/peer_best_retry.h``,
  ``src/LLP/tritium.cpp`` and ``src/TAO/Ledger/process.cpp``.
* `PR #733 <https://github.com/NamecoinGithub/LLL-TAO/pull/733>`_: staged best-chain
  transitions, shared request tracking and time-peer ownership;
  ``src/TAO/Ledger/state.cpp``, ``src/LLD/sector.cpp``,
  ``src/LLP/include/block_inventory_requests.h`` and ``src/LLP/time.cpp``.

Verification and deployment boundaries
--------------------------------------

Use a full clean unit build after nested-header/layout changes: the makefile
generates ``.P`` dependencies but does not include them. Run ``nexus`` tests with
a fresh isolated HOME: the unit bootstrap recursively clears its data directory.
Relevant regression tags are ``[llp][peer-recovery]``,
``[ledger][process][bestchain]`` and ``[ledger][mining-readiness]``.

Focused regressions are not a deployment acceptance test. Controlled multi-node
partition, peer-churn, reorganization and prolonged-recovery testing remains
necessary. Distinct IPs do not prove independent peers; unresolved minority
reports can pause mining. Do not infer crash safety from staged publication or
live-deployment readiness from the earlier PRs' selected-suite results.
