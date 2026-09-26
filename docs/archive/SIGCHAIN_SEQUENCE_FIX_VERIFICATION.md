# Sigchain Sequence (nSequence) Fix — Full Verification Across All Block Production Paths

## Summary

This document verifies that the `CreateTransaction()` nSequence/hashPrevTx fix
(three-source resolution + `else if` → unconditional disk check) correctly
applies to **all** Tritium block production paths:

| Block Type | Channel | Entry Point | Calls `CreateTransaction()`? | Fixed? |
|-----------|---------|-------------|------------------------------|--------|
| **PoW — Prime** | 1 | `CreateBlock()` → `CreateProducer()` | ✅ Yes (create.cpp:705) | ✅ |
| **PoW — Hash** | 2 | `CreateBlock()` → `CreateProducer()` | ✅ Yes (create.cpp:705) | ✅ |
| **PoS — Stake** | 0 | `CreateStakeBlock()` | ✅ Yes (create.cpp:893) | ✅ |
| **Private/Hybrid** | 3 | `CreateBlock()` → `CreateProducer()` | ✅ Yes (create.cpp:705) | ✅ |

All four paths converge on a **single** `CreateTransaction()` function
(create.cpp:73–239), which means the fix is applied uniformly across
every block type that the Nexus node can produce.

---

## The Three Bugs That Were Fixed

### Bug 1 (Critical): Mempool Genesis Ignored for seq=0 Sigchains

**Location:** `src/TAO/Ledger/create.cpp`, lines 108–127

When the mempool contained the genesis profile transaction (seq=0) and Sessions
had no entry, `txPrev` was default-constructed with `hashGenesis==0` and
`nSequence==0`.  The old comparison `txMem.nSequence > txPrev.nSequence` evaluated
to `0 > 0 → false`, silently ignoring the mempool genesis.

**Fix:** Added `|| (txPrev.hashGenesis == 0 && txMem.hashGenesis != 0)` to accept
any valid mempool entry when `txPrev` is still unset.

### Bug 2 (Critical): `else if` Prevented Disk Fallback

**Location:** `src/TAO/Ledger/create.cpp`, lines 129–155

When `mempool.Get()` returned `true` (even if the sequence comparison failed), the
`else if` on the disk path prevented `ReadLast()` from ever running.  A newer
on-disk entry was never consulted.

**Fix:** Changed `else if` to unconditional block with separate `hashDiskLast`
variable.  The disk entry only wins if its sequence is strictly greater than
whatever `txPrev` already holds.

### Bug 3: RefreshProducerIfStale Missed Mempool Genesis

**Location:** `src/TAO/Ledger/stateless_block_utility.cpp`, RefreshProducerIfStale()

At submit time, `RefreshProducerIfStale()` only checked disk (`ReadLast`), missing
genesis profiles still in mempool.

**Fix:** Added `mempool.Get()` fallback when disk `ReadLast` returns false.

---

## Proof-of-Stake (Staking) Path — Detailed Verification

The Tritium Stake Minter is a background thread that produces Proof-of-Stake
(channel 0) blocks.  It requires a logged-in sigchain with staking unlocked.

### Call Chain

```
TritiumMinter::StakeMinterThread()         [tritium_minter.cpp:356]
  │
  ├─ StakeMinter::CheckUser()              [stake_minter.cpp:152]
  │    └─ Authentication::Unlocked(PinUnlock::STAKING)
  │
  ├─ StakeMinter::CreateCandidateBlock()   [stake_minter.cpp:351]
  │    ├─ Authentication::Unlock(strPIN, PinUnlock::STAKING)
  │    ├─ Authentication::Credentials()
  │    │
  │    └─ CreateStakeBlock(pCredentials, strPIN, block, fGenesis)
  │       [create.cpp:865]
  │       │
  │       ├─ AddTransactions(block)        [create.cpp:889] (if !fGenesis)
  │       │
  │       └─ CreateTransaction(user, pin, rProducer)
  │          [create.cpp:893]              ← *** SAME FIXED FUNCTION ***
  │          │
  │          ├─ Source 1: Sessions->ReadLast()
  │          ├─ Source 2: mempool.Get()    ← FIX: hashLast synced + genesis override
  │          └─ Source 3: Ledger->ReadLast()  ← FIX: unconditional (no else if)
  │
  ├─ StakeMinter::CalculateWeights()       [stake_minter.cpp:418]
  │
  └─ TritiumMinter::MintBlock()            [tritium_minter.cpp:422]
       └─ StakeMinter::HashBlock()         [stake_minter.cpp:522]
            └─ StakeMinter::ProcessBlock() [stake_minter.cpp:610]
                 │
                 ├─ block.producer.Build()
                 ├─ block.producer.Sign(pCredentials->Generate(nSequence, strPIN))
                 ├─ BuildMerkleTree()
                 ├─ SignBlock()
                 └─ TAO::Ledger::Process(block, nStatus)
```

### Key Facts

1. **`CreateStakeBlock()` calls `CreateTransaction()` at line 893** — the exact same
   function that was fixed.  There is no separate code path for stake producers.

2. **The producer's `nSequence` and `hashPrevTx`** are set by `CreateTransaction()`
   (lines 179–181).  The stake minter does NOT override these values.

3. **Signing uses the correct sequence:** `ProcessBlock()` signs with
   `pCredentials->Generate(block.producer.nSequence, strPIN)` (line 640),
   which uses the sequence already set by `CreateTransaction()`.

4. **The stake minter requires `PinUnlock::STAKING`** (not `PinUnlock::MINING`).
   Both flags follow the same credential pattern but are distinct unlock actions.

---

## Proof-of-Work (Mining) Path — Detailed Verification

### Call Chain

```
CreateBlock(user, pin, nChannel, rBlockRet, ...)   [create.cpp:437]
  │
  ├─ AddTransactions(rBlockRet)                     [create.cpp:619]
  │
  └─ CreateProducer(user, pin, rProducer, ...)      [create.cpp:695]
       │
       └─ CreateTransaction(user, pin, rProducer)
          [create.cpp:705]                          ← *** SAME FIXED FUNCTION ***
          │
          ├─ Source 1: Sessions->ReadLast()
          ├─ Source 2: mempool.Get()    ← FIX applied
          └─ Source 3: Ledger->ReadLast()  ← FIX applied
```

### Key Facts

1. Both **Prime (channel 1)** and **Hash (channel 2)** use this path.
2. The PoW producer adds `OP::COINBASE` operations after `CreateTransaction()`.
3. The nSequence/hashPrevTx are set identically to staking.

---

## Authentication Requirements Comparison

| Component | Unlock Flag | Credential Source | When Configured |
|-----------|------------|-------------------|-----------------|
| **Stake Minter** | `PinUnlock::STAKING` | `Authentication::Credentials()` | User login + unlock for staking |
| **PoW Mining (Legacy port 8323)** | `PinUnlock::MINING` | `Authentication::Credentials()` | `-autologin` + unlock for mining |
| **PoW Mining (Stateless port 9323)** | `PinUnlock::MINING` | `Authentication::Credentials(SESSION::DEFAULT)` | `-autologin` + unlock for mining |

Both paths ultimately call `CreateTransaction()` with the same
`memory::encrypted_ptr<Credentials>`, ensuring the sigchain resolution logic is
identical.

---

## Regression Test Coverage

### Existing Test: `tests/unit/TAO/Ledger/create_transaction.cpp`

Tests the three-source resolution in isolation:
- Test 1: Mempool winner — hashLast tracks mempool tx hash
- Test 2: No mempool — hashLast uses ledger
- Test 3: Mempool does not override higher-sequence sessions tx
- Test 4: Genesis transaction produces zero hashLast

### New Test: `tests/unit/TAO/Ledger/stake_sequence.cpp`

Tests specific to the stake minter path:
- Test 1: Stake producer sequence when mempool has genesis (seq=0)
- Test 2: Stake producer sequence with disk-only prior transactions
- Test 3: Disk fallback always consulted (no `else if` skip)
- Test 4: Mempool genesis override for first stake block
- Test 5: VTX-same-genesis interaction with stake producer

---

## How to Verify Manually

### With `-nseqdiag` flag

Start the node with `-nseqdiag` to enable sequence diagnostics:

```bash
./nexus -testnet -autologin=1 -nseqdiag
```

Look for `[NSEQ_DIAG][CreateTransaction]` log entries.  Verify:
- `hashLast_mismatch=no` (hashLast matches txPrev.hash)
- `source=` shows the correct winning source
- `chosen.nSequence=` is exactly `txPrev.nSequence + 1`

### With `-trainingwheels=1` flag

For detailed PoW validation logging:

```bash
./nexus -testnet -autologin=1 -trainingwheels=1
```

### Check stake minter specifically

```bash
./nexus -testnet -autologin=1 -stake -verbose=3
```

Monitor for:
- `CreateStakeBlock` → `CreateTransaction` sequence in logs
- `ProcessBlock` → `Sign` using correct nSequence
- `BLOCK ACCEPTED` after successful stake

---

## Conclusion

The nSequence fix in `CreateTransaction()` is the **single point of truth** for all
sigchain sequencing in the Nexus node.  Because both PoW (`CreateBlock` →
`CreateProducer`) and PoS (`CreateStakeBlock`) converge on this function, the fix
is automatically applied to all block production paths.  No separate stake-specific
fix is needed.

The fix addresses three distinct failure modes:
1. Mempool genesis (seq=0) being ignored when txPrev is unset
2. Disk fallback being skipped when mempool.Get() returned true
3. RefreshProducerIfStale not consulting mempool for genesis profiles

All three fixes work together to ensure that the producer transaction in any mined
or staked block has the correct `nSequence` and `hashPrevTx`, preventing
"duplicate genesis-id" and "prev transaction incorrect sequence" errors.
