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
