# Stake Block Burst Cooldown — Upstream Issue Draft

**Status:** Draft for upstream discussion  
**Scope:** Mainnet-first diagnostics and local-node mitigation; do not change consensus timing here.  
**Related local work:** SUBMIT_BLOCK stale-template diagnostics and shared staleness rejection handling.

## Issue title

Investigate 3-5 second cooldown for stake block bursts to reduce orphan/stale-tip stalls

## Problem statement

Mainnet nodes can receive or produce bursts of stake blocks close together, including repeated patterns around five blocks in ten seconds. During these bursts, local nodes may not clear stale/orphan state quickly enough before the next stake block arrives.

The observed symptom is not a clear fatal error. The node can keep running while the blockchain tip refuses to advance from the stuck point, even after multiple `RevertBlock` attempts and restarts.

The most visible clue in available logs was tied to orphan handling. GUI logs are harder to use for this failure mode than command-line node logs, so the local codebase should improve diagnostics around stale-template and orphan-related rejection paths while preserving the existing ability to process burst stake blocks every second.

## Proposed upstream question

Should Nexus add a universal short cooldown for stake-block processing or stake-block acceptance fan-out, around 3 to 5 seconds, so nodes have enough time to clear orphan/stale state during stake bursts?

This should be considered upstream because it may affect network-wide timing, propagation, and consensus-adjacent behavior. The local repository should not independently introduce a broad universal cooldown as a consensus-policy change.

## Local repository stance

- Mainnet behavior matters most; testnet coverage is still useful to prevent regressions.
- Do not block current one-second stake-burst processing locally yet.
- Implement local node-side diagnostics and safe stale-template recovery only.
- Keep local changes usable as a tested example for upstream maintainers.
- Avoid broad network behavior changes until an upstream issue is opened and reviewed.

## Candidate requirement for upstream

A universal stake-block cooldown, if adopted, should:

1. Apply to stake-block burst handling, not Prime/Hash mining work submission.
2. Be short enough to avoid slowing normal mainnet behavior; initial candidate range is 3 to 5 seconds, derived from the observed five-blocks-in-ten-seconds burst pattern while leaving extra time for orphan/stale cleanup.
3. Help nodes drain orphan/stale-tip state before processing the next burst item.
4. Preserve safety during real reorgs and not hide invalid blocks.
5. Emit clear command-line diagnostics when cooldown engages, including current tip, candidate block hash, channel, height, orphan status, and cooldown remaining time.
6. Be covered by testnet/regression tests that simulate burst stake blocks and orphan recovery.

## Local diagnostics requested before upstreaming

The local node should make it easier to identify whether a stuck tip is caused by stale templates, orphan handling, or a post-validation tip race:

- Log a stable reason code for each SUBMIT_BLOCK staleness rejection.
- Include both `hashPrevBlock` and current `hashBestChain` in stale diagnostics.
- Include channel, unified height, merkle root, and lane where applicable.
- Keep the stateless and legacy mining lanes in parity by sharing staleness decision logic.
- Continue forcing a fresh template push after staleness-rooted rejection so local miners do not wait for poll/backoff recovery.

## Non-goals for local code

- Do not add the universal stake cooldown locally as a network policy change.
- Do not change consensus validation rules.
- Do not suppress or ignore orphan/reject states.
- Do not rely only on GUI logs for diagnosis.

## Suggested upstream acceptance criteria

- Reproduce burst stake behavior with a controlled testnet/regression scenario.
- Demonstrate that a short cooldown reduces stuck-tip/orphan-loop incidents.
- Demonstrate that normal block propagation and valid stake acceptance remain safe.
- Provide operator-visible diagnostics for cooldown activation and orphan cleanup.
- Document whether the cooldown is consensus, policy, relay, or local-processing behavior.
