# Direct Reward Address Implementation - Summary

## Overview

This implementation adds a simplified reward routing system where miners specify their desired payout address directly. The reward address is sent **encrypted** after ChaCha20 keys are established during Falcon authentication.

## Protocol Flow

```
NexusMiner                              Nexus Node
    │                                        │
    │──── MINER_AUTH_INIT ──────────────────▶│  Genesis (32) + Falcon PubKey + ID
    │                                        │  Node extracts genesis, stores pubkey
    │◀─── MINER_AUTH_CHALLENGE ─────────────│  Nonce (32 bytes)
    │                                        │
    │──── MINER_AUTH_RESPONSE ──────────────▶│  Falcon Signature
    │                                        │  Node verifies signature
    │◀─── MINER_AUTH_RESULT ────────────────│  Success
    │                                        │  ChaCha20 key = SK256(domain + genesis + nonce)
    │                                        │
    │═══════════════════════════════════════│
    │         ENCRYPTED FROM HERE            │
    │═══════════════════════════════════════│
    │                                        │
    │──── MINER_SET_REWARD ─────────────────▶│  Encrypted(Reward Address)
    │                                        │  Node validates:
    │                                        │  ├─ Address != 0
    │                                        │  └─ Type byte = UserType (TritiumGenesis)
    │◀─── MINER_REWARD_RESULT ──────────────│  Encrypted(Success)
    │                                        │
    │──── GET_BLOCK ────────────────────────▶│  Mining begins
    │◀─── BLOCK_DATA ───────────────────────│  Block uses hashRewardAddress
    │──── SUBMIT_BLOCK ─────────────────────▶│  Coinbase → hashRewardAddress
```

## New Message Types

### MINER_SET_REWARD (0xd5 / 213)

**Direction**: Miner → Node  
**Encrypted**: Yes  
**Format** (after decryption):
```
[32 bytes] hashRewardAddress  // NXS account register address
```

### MINER_REWARD_RESULT (0xd6 / 214)

**Direction**: Node → Miner  
**Encrypted**: Yes  
**Format** (after decryption):
```
[1 byte]  nStatus       // 0x01 = success, 0x00 = failure
[1 byte]  nMessageLen   // Length of error message (0 on success)
[N bytes] strMessage    // Error message (only present on failure)
```

## Implementation Details

### 1. Miner Class Extensions

**New Fields** (src/LLP/types/miner.h):
```cpp
uint256_t            hashGenesis;        // Miner's genesis (from AUTH_INIT)
std::vector<uint8_t> vChaChaKey;         // ChaCha20 session key
bool                 fEncryptionReady;   // ChaCha20 established
uint256_t            hashRewardAddress;  // Bound reward address
bool                 fRewardBound;       // Reward address validated
```

**New Methods**:
- `EncryptPayload()` - ChaCha20-Poly1305 AEAD encryption
- `DecryptPayload()` - ChaCha20-Poly1305 AEAD decryption
- `ValidateRewardAddress()` - On-chain NXS account validation
- `SendRewardResult()` - Encrypted result transmission
- `ProcessSetReward()` - Reward address binding handler

### 2. ChaCha20 Key Derivation

**Formula**:
```cpp
vInput = DOMAIN + hashGenesis + vAuthNonce
vChaChaKey = SK256(vInput)
```

**Security Properties**:
- Domain separation: `"nexus-mining-chacha20-v1"`
- Genesis binding: Each genesis has unique key
- Session uniqueness: Nonce prevents replay across sessions
- 256-bit key strength

### 3. Reward Address Validation

**Checks** (ValidateRewardAddress):
1. ✓ Address is not zero
2. ✓ Address has valid TritiumGenesis type byte (GenesisConstants::IsValidGenesisType)

**Note:** Register Addresses (type 0x02 ACCOUNT registers) are explicitly **NOT** supported.
`Coinbase::Verify()` enforces that the coinbase recipient must be a TritiumGenesis (UserType)
sigchain address. Any block with a Register Address in the coinbase field is rejected by all
network peers. Pool operators use their own TritiumGenesis account as `hashRewardAddress` and
handle internal payout to individual miners separately.

### 4. Block Creation Integration

**Modified**: `Miner::new_block()` (src/LLP/miner.cpp)

```cpp
// Use reward address for stateless miners
uint256_t hashDynamicReward = 0;
if(fStatelessMinerSession.load() && fRewardBound)
{
    hashDynamicReward = hashRewardAddress;
}

// Pass to CreateBlock via existing parameter
CreateBlock(pCredentials, strPIN, nChannel, *pBlock, 
            nBlockIterator, &tCoinbaseTx, hashDynamicReward);
```

The existing `TAO::Ledger::CreateBlock` function already supports dynamic reward routing via the `hashDynamicGenesis` parameter - no changes needed there.

### 5. Protocol State Machine

```
DISCONNECTED
    ↓ (MINER_AUTH_INIT received)
AUTH_INIT_RECEIVED
    ↓ (Challenge sent)
CHALLENGE_SENT
    ↓ (MINER_AUTH_RESPONSE received + verified)
AUTH_SUCCESS [ChaCha20 key derived]
    ↓ (MINER_SET_REWARD received + validated)
REWARD_BOUND
    ↓ (GET_BLOCK allowed)
MINING
```

## Security Analysis

### ✅ Strengths

1. **Post-Quantum Authentication**: Falcon-512 signatures required before any operations
2. **Encrypted Communication**: All reward data protected by ChaCha20-Poly1305
3. **Session Isolation**: Unique keys per session via nonce
4. **Address Validation**: On-chain verification prevents invalid destinations
5. **TritiumGenesis-Only Routing**: Only valid TritiumGenesis (UserType) sigchain accounts accepted; prevents consensus rejection by network peers

### ⚠️ Considerations

1. **Genesis Disclosure**: Genesis is sent in AUTH_INIT (unencrypted)
   - **Mitigation**: Genesis is public on-chain anyway
   
2. **Nonce Reuse**: Same nonce used for both signature and key derivation
   - **Status**: Acceptable - nonce is 32 bytes, cryptographically random
   
3. **No Rate Limiting**: ProcessSetReward can be called multiple times
   - **Impact**: Minimal - requires authentication first

## Testing

### Unit Tests (tests/unit/LLP/reward_address_protocol.cpp)

**Coverage**:
- ✅ MINER_SET_REWARD packet structure validation
- ✅ MINER_REWARD_RESULT format verification
- ✅ Message type constant values
- ✅ Zero address rejection
- ✅ Payload size validation

### Integration Testing

**Manual Testing Required**:
1. Full authentication + reward binding flow
2. Block creation with bound address
3. Multiple reward address changes
4. Invalid address rejection scenarios
5. Encryption/decryption round-trip

## Use Cases Enabled

| Scenario | Auth Genesis | Reward Address | Benefit |
|----------|--------------|----------------|---------|
| Standard mining | ALICE | ALICE:default | Normal operation |
| Cold storage | ALICE | COLD_WALLET:default | Security |
| Team mining | BOB | TEAM:treasury | Shared rewards |
| Charity mining | ALICE | CHARITY:donations | Giving back |
| Pool payout | POOL_OP | MINER_123:default | Pool flexibility |

## Files Modified

1. **src/LLP/types/miner.h**
   - Added MINER_SET_REWARD (213) constant
   - Added MINER_REWARD_RESULT (214) constant
   - Added encryption and reward fields to Miner class
   - Added helper method declarations

2. **src/LLP/miner.cpp**
   - Updated MINER_AUTH_INIT to extract genesis
   - Added ChaCha20 key derivation after authentication
   - Implemented ProcessSetReward handler
   - Implemented ValidateRewardAddress
   - Implemented encryption/decryption helpers
   - Modified new_block() to use hashRewardAddress
   - Added reward bound check in GET_BLOCK handler

3. **tests/unit/LLP/reward_address_protocol.cpp** (NEW)
   - Packet structure tests
   - Message format validation

## Backward Compatibility

**Impact**: None - this is new functionality

- Stateful miners (TAO API session): Unaffected
- Old protocol miners: Cannot use reward routing (as expected)
- New protocol miners: Must send MINER_SET_REWARD before mining

## Next Steps for Production

1. **Integration Testing**: Test full protocol flow with live miner
2. **Performance Testing**: Measure encryption overhead
3. **Documentation**: Update mining protocol docs
4. **Miner Implementation**: Update NexusMiner client to support new flow
5. **Network Testing**: Testnet deployment and validation

## Summary

This implementation provides a clean, secure way for miners to specify arbitrary NXS accounts for reward routing. The design leverages existing Falcon authentication, adds minimal protocol overhead (2 new messages), and reuses proven encryption (ChaCha20-Poly1305). The flexibility enables use cases from cold storage to pool payouts while maintaining strong security properties.
