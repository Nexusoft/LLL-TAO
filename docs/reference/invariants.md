# Hard-Won Invariants

This page collects invariants that should remain stable across future refactors. Treat these as current reference rules, not change history.

## 1. Sigchain oracle consistency

Submit-time `vtx` consistency must resolve predecessors the same way `Connect()` will see them: earlier same-genesis transactions in the same block first, then committed disk state. Mempool-only predecessors are not authoritative submit-time anchors.

Reference: [Sigchain Last Resolution](sigchain-last-resolution.md).

## 2. Merkle-root immutability after signing

After a block template is signed, the transaction set and producer transaction contents must not be mutated. Any change to `vtx`, producer contracts, reward outputs, or extra nonce semantics requires rebuilding and re-signing the template.

## 3. Sigchain ordering inside a block

For multiple transactions from the same genesis in one block, each later transaction must point to the hash produced by the earlier same-genesis entry. Validation should track this in block order rather than re-reading stale external state.

## 4. Disk-only `Connect()` anchor

`Connect()` is the final ledger authority for block acceptance. Pre-checks may reject templates that cannot match `Connect()` semantics, but they must not bless anchors that `Connect()` cannot reproduce from disk plus prior in-block writes.

## 5. Shared `pMemory` ownership

`FLAGS::MEMPOOL` and physical `FLAGS::BLOCK` both use the process-wide `pMemory` overlay. `TxnBegin` must reject the second of those begins, and any duplicate begin of an already-owned slot, before `MemoryBegin`. MINER and SANITIZE stay on `pMiner` and `pSanitize` and may remain open beside MEMPOOL. A foreign owner rejects MINER and SANITIZE commit without releasing that owner. A mismatched or unowned memory abort or MEMPOOL commit must not release another thread's overlay. Commit and abort must pass an instance mask that covers every instance opened by the matching begin. A narrower mask is rejected and does not release the owner, so omitted journals or overlays cannot be left open and unowned. A wider mask still covers the opened set. `TritiumBlock::Accept()` commits and releases the physical slot before `mempool.ProcessOrphans(hashProducer)`. That call runs only when disk `ReadLast` for the producer genesis is the producer, so a stored but unconnected side chain does not record a false conflict. A throw from `MemoryBegin` or a selected journal `TxnBegin` after the slot is claimed must release only instances that finished starting, clear that slot, and then propagate. Callers must not treat that exception as a begin they still own. Disposable SANITIZE and MINER transactions must abort on exceptional exit so a later begin of that slot is not rejected.

Reference: [LLD global transaction ownership](../diagrams/architecture/lld-global-transactions.md).
