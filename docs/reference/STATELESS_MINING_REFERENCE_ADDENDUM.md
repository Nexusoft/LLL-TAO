---

## 10. ⚠️ CRITICAL TRAP: The `nChannelHeight` / `nHeight` Confusion

> **This section documents the exact confusion that caused the compile error at `stateless_miner_connection.cpp:2818`.**  
> **This is the single most dangerous pitfall in the mining infrastructure.**

---

### 10.1 The Three Types and What They Carry

```
TAO::Ledger::Block (base class — the minimal wire template)
  ├── nVersion         uint32_t
  ├── hashPrevBlock    uint1024_t
  ├── hashMerkleRoot   uint512_t
  ├── nChannel         uint32_t   ← WHICH channel (1=Prime, 2=Hash)
  ├── nHeight          uint32_t   ← Context-dependent! (see 10.2 below)
  ├── nBits            uint32_t
  ├── nNonce           uint64_t
  └── vOffsets         vector<uint8_t>
       ↑ NO nChannelHeight here!

TAO::Ledger::TritiumBlock : public Block
  ├── (all Block fields above)
  ├── nTime            uint64_t
  ├── producer         Transaction
  ├── ssSystem         Stream
  └── vtx              vector<pair<uint8_t, uint512_t>>
       ↑ NO nChannelHeight here either!

TAO::Ledger::BlockState : public Block      ← The ONLY type with nChannelHeight
  ├── (all Block fields above)
  ├── nTime            uint64_t
  ├── ssSystem         Stream
  ├── vtx              vector<pair<...>>
  ├── nChainTrust      uint64_t
  ├── nMoneySupply     uint64_t
  ├── nChannelHeight   uint32_t   ← ✅ THE ONLY PLACE THIS EXISTS
  ├── nChannelWeight   uint128_t[3]
  ├── nReleasedReserve int64_t[3]
  └── hashNextBlock    uint1024_t
```

### 10.2 What Does `Block::nHeight` Actually Mean?

**It depends on WHEN it was set:**

```
When CreateBlockForStatelessMining() creates a mining template:

    block.nHeight = stateChannel.nChannelHeight + 1
                    ↑
                    This is the CHANNEL height of the NEXT block to mine,
                    NOT the unified blockchain height!

    block.nChannel = context.nChannel (1=Prime or 2=Hash)

So inside new_block():

    pBlock->nHeight    == next channel height   (e.g., 2165444)
    pBlock->nChannel   == 1 or 2
    pBlock->nChannelHeight == ❌ DOES NOT EXIST
```

**The rule is:**

| Code Location | `Block::nHeight` means |
|---------------|----------------------|
| After `CreateBlockForStatelessMining()` | **Channel height** of next block to mine |
| After `BlockState` is built (via `Accept()`) | **Unified blockchain height** |
| In `TemplateMetadata` | `nHeight` = unified; `nChannelHeight` = channel-specific |

### 10.3 The Compile Error Decoded

```cpp
// ❌ WRONG — This compiles against BlockState::nChannelHeight
// But pBlock is TritiumBlock* which inherits Block, NOT BlockState!

TAO::Ledger::TritiumBlock* pBlock = ...;
if(pBlock->nChannelHeight != stateChannel.nChannelHeight + 1)  // ← ERROR!
    ...
debug::log(0, "   Template validated at channel height ", pBlock->nChannelHeight);  // ← ERROR!
```

The compiler correctly rejects this because `TritiumBlock` inherits from `Block`, not `BlockState`. `nChannelHeight` lives only on `BlockState`.

### 10.4 The Fix

```cpp
// ✅ CORRECT — Block::nHeight IS the channel height after CreateBlock()

TAO::Ledger::TritiumBlock* pBlock = ...;
if(pBlock->nHeight != stateChannel.nChannelHeight + 1)   // nHeight = channel height here
{
    debug::error(FUNCTION, "Template stale: channel height mismatch");
    debug::error(FUNCTION, "  Template nHeight (=channel height): ", pBlock->nHeight);
    debug::error(FUNCTION, "  Expected (current channel + 1):    ", stateChannel.nChannelHeight + 1);
    delete pBlock;
    return nullptr;
}

debug::log(0, "   Template validated at channel height ", pBlock->nHeight);
```

### 10.5 Why Is This So Confusing?

The confusion arises from three simultaneous facts that seem contradictory:

1. **`BlockState::nChannelHeight`** — The authoritative, immutable channel height of a confirmed block. Read-only after `Accept()`.

2. **`Block::nHeight` after `CreateBlock()`** — Set to `channelHeight + 1` *because that's the next block to mine*. So temporarily, `Block::nHeight` IS the channel height for the purpose of template staleness.

3. **`TemplateMetadata::nChannelHeight`** — Our wrapper that stores the channel height *separately* from the block, precisely because the raw `Block` type doesn't carry it in a field named `nChannelHeight`. `TemplateMetadata` is our "extension" of `Block` with explicit labeling.

```
The Relationship:

  pBlock->nHeight                  ←→   TemplateMetadata::nChannelHeight
  (implicitly channel height            (explicitly labeled for clarity)
   after CreateBlock())

  stateChannel.nChannelHeight      ←→   BlockState::nChannelHeight
  (the confirmed channel height          (the ONLY place this name is
   from the chain)                        officially defined)
```

### 10.6 Quick Decision Guide: Which Field To Use?

```
Do you have a TAO::Ledger::BlockState?
  YES → Use .nChannelHeight (the authoritative field, clearly labeled)
  NO  → You have a Block* or TritiumBlock*

Do you need the channel height of a mining template (Block*)?
  → Use .nHeight (set to channel height by CreateBlock())
  → Compare against stateChannel.nChannelHeight + 1

Do you need the UNIFIED blockchain height?
  → Use TAO::Ledger::ChainState::nBestHeight.load() or stateBest.nHeight
  → Do NOT use the mining template's nHeight for this — they're different!

Do you need channel height for staleness detection?
  → Use TemplateMetadata::nChannelHeight (our labeled storage)
  → It mirrors what pBlock->nHeight contained at creation time
```

### 10.7 Summary Table

| Expression | Valid? | Value |
|------------|--------|-------|
| `blockState.nChannelHeight` | ✅ | Channel-specific height of confirmed block |
| `pTritiumBlock->nChannelHeight` | ❌ COMPILE ERROR | Field doesn't exist on TritiumBlock |
| `pBlock->nChannelHeight` | ❌ COMPILE ERROR | Field doesn't exist on Block |
| `pBlock->nHeight` (after CreateBlock) | ✅ | Next channel height to mine |
| `templateMeta.nChannelHeight` | ✅ | Stored channel height (our extension) |
| `ChainState::nBestHeight.load()` | ✅ | Unified blockchain height (atomic) |
| `stateBest.nHeight` | ✅ | Unified blockchain height from state snapshot |

---