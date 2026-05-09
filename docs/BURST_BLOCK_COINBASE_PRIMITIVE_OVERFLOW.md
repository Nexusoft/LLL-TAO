# BURST_BLOCK_COINBASE_PRIMITIVE_OVERFLOW

## Summary

This document describes a **TOCTOU (time-of-check/time-of-use) race condition** observed in production that produces the following error sequence:

```
ERROR: Verify : can not verify PRIMITIVE per contract
ERROR: SetBest : failed to connect <block_hash>
ERROR: Index : failed to set best chain
ERROR: handle_submit_block_stateless : SUBMIT BLOCK ledger write failed: block rejected
```

The block passes **every other validation check** (PoW, hashPrevBlock, height, difficulty, nSequence, Falcon signature, nNonce) and only fails at the final ledger-commit step inside `TAO::Register::Verify()`.

---

## 1. Root Cause: Coinbase Contract Stream Corruption

### Where the Error Fires

The error fires from [`src/TAO/Register/verify.cpp` lines 799–800](../src/TAO/Register/verify.cpp#L799):

```cpp
/* Check for end of stream. */
if(!contract.End())
    return debug::error(FUNCTION, "can only have one PRIMITIVE per contract ");
```

This check fires when the producer's coinbase contract operation byte-stream has **residual bytes** after the `OP::COINBASE` primitive is fully consumed. The COINBASE case in `verify.cpp` reads exactly 48 bytes past the opcode:

```cpp
case TAO::Operation::OP::COINBASE:
{
    /* Seek through coinbase data. */
    contract.Seek(48);
    break;
}
```

If the stream is not exactly exhausted after those 48 bytes, `contract.End()` is `false` → error.

### Expected Coinbase Contract Layout

A single miner-reward coinbase contract in `CreateProducer` writes exactly:

| Field | Size | Code |
|---|---|---|
| `OP::COINBASE` opcode | 1 byte | `rProducer[0] << uint8_t(TAO::Operation::OP::COINBASE)` |
| `hashRewardRecipient` (uint256_t) | 32 bytes | `rProducer[0] << hashRewardRecipient` |
| `nCredit` (uint64_t) | 8 bytes | `rProducer[0] << nCredit` |
| `nExtraNonce` (uint64_t) | 8 bytes | `rProducer[0] << nExtraNonce` |
| **Total** | **49 bytes** | |

Ambassador/developer payout contracts use the same layout: `OP::COINBASE` + `it->first` (uint256_t, 32 bytes) + `nCredit` (uint64_t, 8 bytes) + `uint64_t(0)` (8 bytes) = **49 bytes**.

---

## 2. The Race Condition Mechanism

### Two Concurrent Paths

The corruption occurs due to a **TOCTOU race** between two concurrent paths in `CreateBlock()`:

- **Submit path** (`handle_submit_block_stateless` → `sign_block` → `AcceptMinedBlock`): reads the cached producer to assemble the solved block for ledger commit.
- **Template rebuild path** (`CreateBlock` → `CreateProducer`): writes a new coinbase contract stream to `rBlockRet.producer` when a burst-block or mempool change triggers cache invalidation.

### Burst-Block Timeline (from the Production Incident)

| Timestamp | Event |
|---|---|
| `12:02:55.151` | `CreateBlock` → Template 1 queued (height=6703531) |
| `12:02:55.152` | Miner receives Template 1, validates it |
| `12:02:55.152` | `[Solo FEED]` suppressed — a second feed attempted at same height |
| `12:02:57.588` | **Fermat chain of length 7 found** (solution for Template 1) |
| `12:02:57.803` | **BLOCK FOUND CALLBACK** fires while second template processing is active |
| `12:02:57.803–.817` | Miner builds and submits the block |
| `12:02:57.817` | Node receives `SUBMIT_BLOCK` (1841 bytes), begins validation |
| `12:02:58.280–.386` | PoW ✅, hashPrevBlock ✅, sequence ✅, difficulty ✅ |
| `12:02:58.386` | **✅ PRIME BLOCK VALID** |
| `12:02:58.387` | `AcceptMinedBlock` → `SetBest` → `Verify` → **❌ PRIMITIVE overflow** |

### Supporting Evidence

1. **`central_tracker_height = 6703530` vs `template_height = 6703531`** — the miner's height tracker had not caught up at submit time, confirming the burst window.
2. **`[Solo FEED] Feed suppressed: same unified height 6703530 within 10s cooldown`** — the second template feed was attempted and suppressed, meaning template machinery was re-entering for the same height.
3. **`CreateBlock : Rebuilding stale producer`** immediately after rejection — the node itself confirms the producer was in the process of being rebuilt concurrently with the submit path.
4. **All PoW checks pass** — the block data itself is valid; only the serialized producer contract content is wrong.

### Why Other Checks Pass

- **PoW** (`nNonce`, `hashPrevBlock`): baked into the ProofHash computed from header fields, not from the producer's coinbase stream bytes.
- **Merkle root**: computed from the serialized `vtx` and `producer` transaction hashes. The merkle root was correct at template-issue time; the race mutates the stream content but the `hashMerkleRoot` field in the block header is not recalculated by the submit path.
- **Falcon signature**: covers the merkle root, not the raw producer contract bytes.
- **nSequence / hashPrevTx**: these are fields on the producer `Transaction` object, not the contract stream. They survive the race unaffected.

Only `TAO::Register::Verify()` reads the raw coinbase contract byte-stream and checks for stream exhaustion, which is why this is the only gate that fails.

---

## 3. Distinction from the NSEQ_DIAG Bug

This failure is **completely different** from the `NSEQ_DIAG_MEMPOOL_HASHLAST_BUG`:

| Characteristic | NSEQ_DIAG Bug | This Bug |
|---|---|---|
| Error message | `"prev transaction incorrect sequence"` | `"can not verify PRIMITIVE per contract"` |
| `hashLast_mismatch` | `yes` | `no` |
| Root cause | Mempool's `ReadLast` returned stale hash for sigchain | Coinbase contract stream corrupted by TOCTOU race |
| Producer content | Valid stream, wrong sequence number | Corrupted stream (wrong byte count) |
| PoW valid? | Yes | Yes |
| Fix | Snapshot the correct chain tip before producer creation | Pre-commit stream size guard (this PR) |

---

## 4. Defensive Fix: Pre-Commit Guard

### What This PR Adds

**File:** `src/LLP/stateless_miner_connection.cpp` — in the stateless SUBMIT_BLOCK handler.
**File:** `src/LLP/miner.cpp` — in the legacy SUBMIT_BLOCK handler.

After assembling the solved block candidate (after `sign_block()` / `BuildAndSignSolvedTritiumCandidate`) and before `ValidateMinedBlock()`, both handlers now validate that every `OP::COINBASE` contract in the producer has exactly **49 bytes** in its operation stream.

If any contract deviates from 49 bytes:
1. A `[warning]` is logged including `stream_size`, `expected`, `nHeight`, `nChannel`, `nNonce`.
2. The block is rejected with `BLOCK_REJECTED` and reason code `MALFORMED_PRODUCER` (= 11 in `OpcodeUtility::RejectionReason`).
3. The miner's next `GET_BLOCK` request returns a fresh template.

The guard does **not** crash or assert — it is a clean, recoverable error.

### Guard Logic

```cpp
static constexpr uint64_t COINBASE_STREAM_SIZE = 49;
const uint32_t nProducerContracts = pTritium->producer.Size();
for(uint32_t nContract = 0; nContract < nProducerContracts; ++nContract)
{
    const TAO::Operation::Contract& rContract = pTritium->producer[nContract];
    if(rContract.Empty())
        continue;

    if(rContract.Primitive() == TAO::Operation::OP::COINBASE &&
       rContract.Operations().size() != COINBASE_STREAM_SIZE)
    {
        debug::warning(FUNCTION,
            "[BURST_BLOCK_GUARD] Malformed coinbase contract detected"
            " — stream_size=", rContract.Operations().size(), ...);
        // return BLOCK_REJECTED / MALFORMED_PRODUCER
    }
}
```

### Diagnostic Logging in `CreateProducer`

**File:** `src/TAO/Ledger/create.cpp`

After each coinbase contract is fully written (i.e., after `<< nExtraNonce` or `<< uint64_t(0)` for ambassador/developer slots), a `debug::log(2, ...)` line emits `[COINBASE_STREAM]` with the slot index and stream size. At `-verbose=2` or higher this makes burst-block races visible at the moment of creation, before any submit occurs.

---

## 5. How to Reproduce in Testing

A rapid double-template scenario can be reproduced with:

1. Start a Prime-channel miner against a local testnet node.
2. Inject a artificial `GetLastState` delay (e.g., a `std::this_thread::sleep_for` in `CreateProducer` between the `rProducer[0] << nCredit` and `rProducer[0] << nExtraNonce` writes).
3. Submit a block during that window (via a second miner thread calling `SUBMIT_BLOCK` simultaneously).
4. Observe `[BURST_BLOCK_GUARD]` in the logs, confirming the guard fires before `ValidateMinedBlock`.

Without the guard, the same scenario would reach `TAO::Register::Verify` and produce:
```
ERROR: Verify : can not verify PRIMITIVE per contract
```

---

## 6. Limitations and Follow-on Work

This PR implements a **defensive pre-commit layer**. It prevents the deep-stack `Verify` failure and gives the miner a clean `MALFORMED_PRODUCER` rejection code and an immediate path to a fresh template.

The **underlying race** (Path A reading the producer while Path B writes it) is not closed at the concurrency level by this PR. A deeper fix — mutex protection or snapshot isolation on the producer copy passed to the submit path — is a follow-on item. The `memory::atomic<MiningTemplateCacheEntry>` in `create.cpp` correctly serializes the cache store/load pair; the race described here happens in the submit path's local copy of the block, not in the cache itself.

---

## 7. Files Changed

| File | Change |
|---|---|
| `src/LLP/include/opcode_utility.h` | Added `MALFORMED_PRODUCER = 11` to `RejectionReason` enum |
| `src/LLP/stateless_miner_connection.cpp` | Added coinbase stream size guard before `ValidateMinedBlock` |
| `src/LLP/miner.cpp` | Added coinbase stream size guard before `ValidateMinedBlock`; added `#include <TAO/Operation/include/enum.h>` |
| `src/TAO/Ledger/create.cpp` | Added `debug::log(2, ...)` after each `nExtraNonce` write in `CreateProducer` |
| `tests/unit/LLP/coinbase_contract_size_tests.cpp` | New unit tests for coinbase contract stream size invariant |
| `docs/BURST_BLOCK_COINBASE_PRIMITIVE_OVERFLOW.md` | This document |
