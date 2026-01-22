# Mining Lanes Cheat Sheet (Legacy vs Stateless)

**Purpose:** fast navigation + keyword/entrypoint map for **legacy vs stateless protocol lanes**, including **Auth/Session/Reward** and **mining/blockchain lifecycle**.

---

## A0) Quick Keywords (Auth/Session/Reward)
- `MINER_AUTH_INIT`
- `MINER_AUTH_CHALLENGE`
- `MINER_AUTH_RESPONSE`
- `MINER_AUTH_RESULT`
- `SESSION_START`
- `SESSION_KEEPALIVE`
- `MINER_SET_REWARD`
- `MINER_REWARD_RESULT`
- `MINER_READY`
- `STATELESS_MINER_READY`
- `DisposableFalcon`
- `FalconConstants`
- `ChaCha20`
- `DeriveChaCha20Key`
- `MiningContext`
- `SessionMetadata`
- `NodeSyncManager::RegisterSession`
- `fAuthenticated`
- `fEncryptionReady`
- `hashGenesis`
- `hashKeyID`
- `nSessionId`

---

## A1) Lane-agnostic protocol concepts (what stays consistent)

### A1.1 Authentication (Falcon challenge-response)
**Intent:** Miner identity/session auth (NOT block signing).  
**Who uses it:** Stateless lane mandatory; legacy lane may still support for compatibility depending on policy.

**Typical flow (8-bit naming, conceptually same for stateless lane):**
1. Miner → Node: `MINER_AUTH_INIT (207)`
2. Node → Miner: `MINER_AUTH_CHALLENGE (208)`
3. Miner → Node: `MINER_AUTH_RESPONSE (209)`
4. Node → Miner: `MINER_AUTH_RESULT (210)` (includes success + session id)

**Core state fields to look for:**
- `fMinerAuthenticated` / `context.fAuthenticated`
- `vAuthNonce`
- `vMinerPubKey`
- `hashGenesis`
- `nSessionId`

**Cross references:**
- `src/LLP/miner.cpp` routes some packets to `StatelessMiner::ProcessPacket(...)` (migration scaffold)
- `src/LLP/stateless_miner_connection.cpp` handles stateless lane opcodes and uses `MiningContext`

### A1.2 Encryption (ChaCha20 session wrapping)
**Intent:** Encrypt selected payloads after auth (e.g., reward binding, submit wrappers depending on protocol stage).

**Search anchors:**
- `fEncryptionReady`
- `vChaChaKey`
- `MiningSessionKeys::DeriveChaCha20Key`
- `EncryptPacket` / `DecryptPacket` (if implemented)
- `FalconConstants::SUBMIT_BLOCK_*` (wrapper sizes)

### A1.3 Reward binding (separate from genesis)
**Intent:** Allow miner to specify reward address (encrypted post-auth) so block creation routes reward correctly.

**Packets:**
- Miner → Node: `MINER_SET_REWARD (213)` (encrypted)
- Node → Miner: `MINER_REWARD_RESULT (214)` (encrypted result)

**State:**
- `hashRewardAddress`
- `fRewardBound`

**Key ledger tie:**
- `TAO::Ledger::CreateBlockForStatelessMining(..., hashRewardAddress)` routes rewards.

---

## A2) Legacy lane (8323 / 8-bit) — Auth/Session/Reward touchpoints

**Port:** `MAINNET_LEGACY_MINING_LLP_PORT` / `TESTNET_LEGACY_MINING_LLP_PORT` = 8323 (`src/LLP/include/port.h`)

**Files:**
- `src/LLP/miner.cpp`
- `src/LLP/types/miner.h`
- `src/LLP/stateless_miner.cpp` + `src/LLP/include/stateless_miner.h` (processor used as migration helper)

**Important observation:**
`Miner.cpp` currently acts like a “utility arsenal / bridge” for stateless migration:
- It tries `StatelessMiner::ProcessPacket(...)` for a subset (auth/session/config/reward)
- On "Unknown packet type", it falls back to legacy handling

**Search anchors inside miner.cpp:**
- `StatelessMiner::ProcessPacket`
- `"Unknown packet type"`
- `ProcessPacketStateless(PACKET)`

**Strict lane target direction:**
- On 8323, keep opcode width 8-bit only.
- Consider limiting “StatelessMiner bridging” to auth-only (or removing entirely if 8323 becomes pure legacy).

---

## A3) Stateless lane (9323+ / 16-bit) — Auth/Session/Reward touchpoints

**Port:** `MAINNET_MINING_LLP_PORT` / `TESTNET_MINING_LLP_PORT` = 9323 (`src/LLP/include/port.h`)

**Files:**
- `src/LLP/stateless_miner_connection.cpp`
- `src/LLP/types/stateless_miner_connection.h`
- `src/LLP/include/stateless_opcodes.h`
- `src/LLP/stateless_miner.cpp` (functional processor for auth/session/reward logic)
- `src/LLP/include/stateless_miner.h`

**Entry:**
- `bool StatelessMinerConnection::ProcessPacket()`

**Required property:**
- Reject non-0xD0xx opcodes on stateless port (`StatelessOpcodes::IsStateless` check).

**Stateless “subscribe” / mining readiness:**
- `STATELESS_MINER_READY` (mirror of 216; `0xD0D8`)

**Session persistence:**
- `MiningContext` is the immutable snapshot updated per packet
- `StatelessMinerManager` stores per-connection contexts

---

## A4) Full opcode list (Auth/Session/Reward + Mining) + mirror mapping concept

### A4.1 8-bit (legacy) opcodes of interest
**Mining/data**
- `BLOCK_DATA = 0`
- `SUBMIT_BLOCK = 1`
- `BLOCK_HEIGHT = 2`
- `SET_CHANNEL = 3`
- `BLOCK_REWARD = 4`

**Requests**
- `GET_BLOCK = 129`
- `GET_HEIGHT = 130`
- `GET_REWARD = 131`
- `GET_ROUND = 133`

**Responses**
- `BLOCK_ACCEPTED = 200`
- `BLOCK_REJECTED = 201`
- `NEW_ROUND = 204`
- `OLD_ROUND = 205`
- `CHANNEL_ACK = 206`

**Auth/session/reward**
- `MINER_AUTH_INIT = 207`
- `MINER_AUTH_CHALLENGE = 208`
- `MINER_AUTH_RESPONSE = 209`
- `MINER_AUTH_RESULT = 210`
- `SESSION_START = 211`
- `SESSION_KEEPALIVE = 212`
- `MINER_SET_REWARD = 213`
- `MINER_REWARD_RESULT = 214`

**Notifications**
- `MINER_READY = 216`
- `PRIME_BLOCK_AVAILABLE = 217`
- `HASH_BLOCK_AVAILABLE = 218`

### A4.2 16-bit (stateless) mirror opcode concept
**Concept (from `stateless_opcodes.h`):**
- `Mirror(x)` maps legacy 8-bit opcode `x` into stateless 16-bit opcode `0xD0xx` family.
- Example references:
  - `STATELESS_GET_BLOCK = Mirror(129) = 0xD081`
  - `STATELESS_SUBMIT_BLOCK = Mirror(1) = 0xD001`
  - `STATELESS_BLOCK_ACCEPTED = Mirror(200) = 0xD0C8`
  - `STATELESS_BLOCK_REJECTED = Mirror(201) = 0xD0C9`
  - `STATELESS_GET_ROUND = Mirror(133) = 0xD085`

**Search anchors:**
- `src/LLP/include/stateless_opcodes.h`
- `Mirror(129)`
- `STATELESS_*`

---

## B0) Quick Keywords (Mining / Chain acceptance)
- `CreateBlockForStatelessMining`
- `CreateBlock(`
- `TritiumBlock::Check`
- `TritiumBlock::Accept`
- `AcceptBlock(`
- `TAO::Ledger::Process(`
- `GetNextTargetRequired`
- `GetLastState`
- `nChannelHeight`
- `mapBlocks`
- `TemplateMetadata`
- `CleanupStaleTemplates`
- `nBlockIterator`
- `hashMerkleRoot`
- `hashPrevBlock`
- `nBits`
- `nNonce`
- `vOffsets` (Prime channel requirement)

---

## B1) Ledger canonical: template creation (single source of truth)
**File:** `src/TAO/Ledger/stateless_block_utility.cpp`  
**Function:** `TAO::Ledger::CreateBlockForStatelessMining(uint32_t nChannel, uint64_t nExtraNonce, const uint256_t& hashRewardAddress)`

**Core behavior (high confidence from code):**
- Validates input channel (must be 1 or 2; rejects channel 0). Channel 1 = Prime, channel 2 = Hash; channel 0 is stake-only and not mined.
- Require wallet unlock for mining (consensus signing requirement)
- Anchors template to `ChainState::tStateBest`:
  - `hashPrevBlock = statePrev.GetHash()`
  - `nBits = GetNextTargetRequired(statePrev, nChannel, false)`
  - `nTime = max(prevTime+1, unifiedtimestamp())`
- Uses `GetLastState(stateChannel, nChannel)` to set:
  - `nHeight = stateChannel.nChannelHeight + 1`
- Calls `CreateBlock(pCredentials, strPIN, nChannel, *pBlock, nExtraNonce, ..., hashRewardAddress)`
- Defers PoW validation until miner submits solution

**Why this matters:**
- Channel-height correctness prevents “template staleness false positives”
- This file should also host the **canonical mined-block acceptance helper** (future refactor)

---

## B2) Legacy lane mining server (8323): `LLP::Miner`

**Files:**
- `src/LLP/miner.cpp`
- `src/LLP/types/miner.h`

**Storage:**
- `std::map<uint512_t, TAO::Ledger::Block*> mapBlocks;`

**Template creation flow:**
- `GET_BLOCK (129)` → `new_block()` → store in mapBlocks → respond `BLOCK_DATA (0)` with serialized block

**Submit flow:**
- `SUBMIT_BLOCK (1)`:
  - parse merkle root + nonce at minimum
  - locate template by merkle root in mapBlocks
  - apply nonce / finalize
  - validate + accept to chain
  - respond `BLOCK_ACCEPTED (200)` or `BLOCK_REJECTED (201)`

**Prime rule reminder:**
- Prime channel (`nChannel==1`) requires `vOffsets` non-empty during `TritiumBlock::Check()`. `vOffsets` contains the prime sieve offsets used to validate Prime PoW solutions.
  - If miner doesn’t submit offsets, offsets must already exist in template or be derived.
  - If offsets are missing, `Check()` will reject.

---

## B3) Stateless lane mining server (9323+): `LLP::StatelessMinerConnection`

**Files:**
- `src/LLP/stateless_miner_connection.cpp`
- `src/LLP/types/stateless_miner_connection.h`
- `src/LLP/include/stateless_opcodes.h`

**Storage:**
- `std::map<uint512_t, TemplateMetadata> mapBlocks;`

**Template push flow (stateless):**
- `SendStatelessTemplate()` sends `STATELESS_GET_BLOCK (0xD081)` payload:
  - 12 bytes metadata (big endian):
    - unified height
    - channel height
    - difficulty
  - + 216 bytes Tritium template

**Submit flow (stateless):**
- `STATELESS_SUBMIT_BLOCK (0xD001)`:
  - decrypt/unwrap
  - locate template in mapBlocks
  - apply nonce/solution
  - call `TritiumBlock::Check()`
  - accept into chain
  - respond `STATELESS_BLOCK_ACCEPTED (0xD0C8)` or `STATELESS_BLOCK_REJECTED (0xD0C9)`

**Uniqueness / extra nonce:**
- `static std::atomic<uint32_t> nBlockIterator;`
- Must increment in `new_block()` to avoid template collisions.

---

## B4) NodeSyncManager (not template store)
**File:** `src/LLP/node_sync.cpp`

Stores:
- Session metadata (`SessionMetadata`)
- Block metadata (`BlockMetadata`)

This is not currently wired as the authoritative template store for mining validation.

---

## B5) Golden Refactor Target (future)
### Centralize acceptance next to template creation
Template creation is already centralized in `TAO::Ledger::CreateBlockForStatelessMining`.
The next refactor should add:

- `TAO::Ledger::SubmitMinedBlockForStatelessMining(TAO::Ledger::TritiumBlock&) -> SubmitResult` (success/failure + rejection reason)

Then both lanes can standardize:
1. parse submission payload
2. locate template (mapBlocks)
3. apply nonce/solution
4. call ledger helper
5. respond with lane-correct opcodes
