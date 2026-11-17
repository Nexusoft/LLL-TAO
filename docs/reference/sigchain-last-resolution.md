# Sigchain Last Resolution

This reference defines the authoritative resolution order for Tritium sigchain predecessors during block construction and submit-time validation.

## Oracles

| Mechanism | Scope | Answers |
|-----------|-------|---------|
| `mapLast` | In-flight transactions already accepted into the same block candidate or being checked earlier in `vtx` order | What will `WriteLast()` have written by the time `Connect()` reaches this transaction? |
| `ReadLast` with default block scope | Committed chain state on disk | What is the last transaction for this genesis that `Connect()` can anchor to now? |

## Precedence rule

Use the in-block oracle first. If no earlier same-genesis transaction exists in the candidate block, use disk `ReadLast()` only. Do not peek at mempool state for submit-time `vtx` consistency.

This keeps `ValidateVtxSigchainConsistency()` aligned with `BlockState::Connect()`, which advances `WriteLast()` as it processes block transactions and otherwise anchors to committed ledger state.

## Why mempool is excluded

A mempool-only predecessor can disappear, be replaced, or become stale after a miner receives a template and before the solved block is submitted. Accepting that predecessor as an authoritative submit-time anchor can create templates that fail during `Connect()` validation against disk state.

`AddTransactions()` filters non-first transactions whose predecessor is neither already in the candidate block nor on disk. That gate ensures submit-time validation only needs the two Connect-aligned oracles above.

## Producer predecessor during template construction

Block producers follow the same two oracles. Both `CreateBlock()` (via `CreateProducer()`) and `CreateStakeBlock()` resolve the producer's predecessor as follows:

1. If `vtx` already contains a same-genesis transaction, chain to the highest-sequence one (`FindProducerGenesisTxInVtx()`).
2. Otherwise, chain to the committed ledger tail from `ReadLast(FLAGS::BLOCK)`. If no committed tail exists, create a new sigchain. Do not use the Sessions index, which may still name a disconnected transaction after a reorg. Do not use a mempool tail that was not selected into the block.

`vtx` stores only hashes, so step 1 reads each selected transaction again from the mempool or ledger. If any selected Tritium entry can no longer be read, for example because the mempool removed it after selection, the scan cannot prove that no same-genesis predecessor was selected. Template construction fails instead of using the step 2 fallback, because that would chain the producer behind an older tail that disagrees with the selected transactions. `ValidateProducerAgainstVtx()` also fails closed in this case.

## Current implementation anchors

- `TAO::Ledger::ValidateVtxSigchainConsistency()` tracks in-flight `mapLast` and falls back to disk `ReadLast()`.
- `TAO::Ledger::AddTransactions()` rejects mempool-only predecessors before a template is signed.
- `TAO::Ledger::CreateProducer()` and `TAO::Ledger::CreateStakeBlock()` take the producer predecessor from the selected `vtx`, or from the committed `FLAGS::BLOCK` tail when `vtx` has none, and fail when a selected `vtx` entry is unreadable.
- `TAO::Ledger::Transaction::Connect()` writes the new last hash through `WriteLast()` when a block transaction is connected.

## Historical context

The archived [`NSEQ_DIAG_MEMPOOL_READLAST_FIX.md`](../archive/NSEQ_DIAG_MEMPOOL_READLAST_FIX.md) document records a reverted mempool-first approach. It is intentionally archived and bannered because PR #612 restored the Connect-aligned invariant documented here.
