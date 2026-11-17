# Stateless Mining Architecture

## Overview

NexusMiner connects to LLL-TAO node via TCP mining protocol (port 8323).
This is a stateless mining architecture - no API access, no SIGCHAIN integration, no session required.

## Components

### Genesis Hash (Tritium Account)

- **Purpose**: Shared secret for ChaCha20 key derivation only
- **Validation**: Must exist on blockchain (`LLD::Ledger->HasFirst()`)
- **Usage**: Encryption key = `SHA256("nexus-mining-chacha20-v1" || genesis_bytes)`
- **NOT used for**: Account resolution, SIGCHAIN lookup, reward routing

### Reward Address (NXS Account)

- **Purpose**: Where mining rewards are paid
- **Validation**: Basic format check only (non-zero, 256-bit)
- **Consensus**: Invalid addresses cause block rejection (miner's loss)
- **Same as**: Old block creation method (direct address, no resolution)

### Falcon Keys (Authentication)

- **Purpose**: Authenticate miner identity
- **Validation**: Signature verification only
- **Session ID**: Derived from `SHA256(falcon_pubkey)`
- **NOT used for**: SIGCHAIN binding, account resolution

## Architecture Flow

### Before (Broken - Required API Session)

```
Genesis → SIGCHAIN Lookup → Default Account → Reward Address
  ↑ Requires API session (stateless miners don't have this)
```

### After (Working - Stateless)

```
Genesis → ChaCha20 Key (blockchain validation only)
Reward Address → Direct usage (consensus validates)
```

## ChaCha20 Key Derivation

The ChaCha20 session key is derived as follows:

```cpp
std::string DOMAIN = "nexus-mining-chacha20-v1";
std::vector<uint8_t> preimage;
preimage.insert(preimage.end(), DOMAIN.begin(), DOMAIN.end());
preimage.insert(preimage.end(), genesis_bytes.begin(), genesis_bytes.end());
uint256_t hashKey = LLC::SK256(preimage);  // SHA-256
std::vector<uint8_t> vKey = hashKey.GetBytes();  // 32 bytes
```

**Key components:**
- Domain: `"nexus-mining-chacha20-v1"` (ASCII bytes)
- Genesis: 32 bytes from `hashGenesis.GetBytes()` (network byte order)
- Hash function: SK256 (SHA-256)
- Output: 32-byte symmetric key

## Reward Address Validation

Reward addresses undergo **basic format validation only**:

```cpp
bool ValidateRewardAddress(const uint256_t& hashReward)
{
    // Check for zero address
    if(hashReward == 0)
        return false;
    
    // Trust miner to provide correct address
    // Invalid addresses → block rejected by consensus (miner's loss)
    return true;
}
```

**Why not validate on-chain?**
1. `LLD::Register->ReadObject()` requires authenticated API session
2. Stateless miners run outside Nexus Interface
3. Consensus validation catches invalid addresses anyway
4. Same approach as old block creation (direct address usage)

## What Was Removed

- ❌ SIGCHAIN resolution (Genesis → Default Account)
- ❌ API-based reward address validation (`LLD::Register->ReadObject()`)
- ❌ FalconAuth SIGCHAIN binding
- ❌ Session-dependent account lookups
- ❌ Account type verification (requires object parsing)
- ❌ Token ID checking (requires SIGCHAIN access)

## What Remains

- ✅ Genesis blockchain validation (security)
- ✅ ChaCha20-Poly1305 encryption (transport security)
- ✅ Falcon signature verification (authentication)
- ✅ Direct reward address usage (same as old method)
- ✅ Basic format validation (non-zero address)

## Diagnostic Logging

### ChaCha20 Decryption Diagnostics

When debugging key mismatches, look for logs like:

```
╔═══════════════════════════════════════════════════════════╗
║  ChaCha20 DECRYPTION DIAGNOSTIC (Node Side)              ║
╠═══════════════════════════════════════════════════════════╣
║ Genesis (uint256_t): a174011c...
║ Genesis (GetHex):    a174011c...
║ Genesis (bytes hex): ...
║ 
║ Domain: nexus-mining-chacha20-v1
║ Key derivation: SHA256(domain || genesis_bytes)
║ 
║ Derived Key (32 bytes): 3f7a2b1c...
║ 
║ Wrapped Pubkey Components:
║   Nonce (12 bytes):    ab12cd34...
║   Ciphertext (bytes):  897
║   Tag (16 bytes):      ef56gh78...
║   AAD:                 FALCON_PUBKEY
╚═══════════════════════════════════════════════════════════╝
```

### Decryption Failure Diagnostics

```
╔═══════════════════════════════════════════════════════════╗
║  ChaCha20 DECRYPTION FAILED                               ║
╠═══════════════════════════════════════════════════════════╣
║ Possible causes:
║  1. Genesis bytes mismatch (check GetBytes() order)
║  2. AAD mismatch (check exact string: FALCON_PUBKEY)
║  3. Nonce extraction error (check packet parsing)
║  4. Tag mismatch (authentication failure)
║ 
║ Compare node logs with NexusMiner logs byte-by-byte
╚═══════════════════════════════════════════════════════════╝
```

### Key Derivation Diagnostics (verbosity >= 2)

```
ChaCha20 Key Derivation Details:
  Genesis (input):  a174011c...
  Genesis (bytes):  1c0174a1...
  Domain:           nexus-mining-chacha20-v1
  Preimage (hex):   6e657875732d...
  Derived key:      3f7a2b1c...
```

## ChaCha20 Key Derivation Troubleshooting

### Key Derivation Formula
Both miner and node must derive the same ChaCha20 session key:
```
key = SHA256("nexus-mining-chacha20-v1" || genesis_bytes)
```

Where `genesis_bytes` is the 32-byte representation of the Tritium GenesisHash.

### Common Issues

1. **Genesis Byte Order Mismatch**
   - The miner's `tritium_genesis` must match the node's `hashGenesis.GetHex()` output
   - Check node logs for "Genesis (GetHex):" and copy that exact value

2. **Comparing Keys**
   - Node logs: "Derived Key (32 bytes): <hex>"
   - Miner logs: "Derived Key (hex): <hex>"
   - These MUST match for authentication to succeed

3. **Localhost Workaround**
   - For localhost mining (127.0.0.1), ChaCha20 is optional
   - Set `enable_chacha20_wrapping = false` in miner.conf

## Testing

### Test: Simplified Reward Address Validation

```bash
# Setup: NexusMiner with any valid NXS address format
# Expected: Node accepts address without blockchain lookup
# Logs:
✓ Reward address format validated: 8BMe6Gc...
  Note: Address existence deferred to consensus validation
```

### Test: ChaCha20 Diagnostics

```bash
# Setup: NexusMiner with wrapped Falcon pubkey
# Expected: Detailed diagnostic logs showing key derivation steps
# Compare with NexusMiner logs to find exact mismatch
```

### Test: No SIGCHAIN Dependencies

```bash
# Verify removed patterns (run from repository root)
grep -r GetDefaultAccount src/LLP/
grep -r ReadTrust src/LLP/
grep -r ResolveGenesisToAccount src/LLP/

# Expected: No matches (all removed)
```

## Security Notes

1. **Genesis validation is mandatory**: Even though reward address validation is simplified, genesis MUST be validated on blockchain before deriving ChaCha20 keys (prevents attacker-chosen key derivation).

2. **ChaCha20 proves genesis knowledge**: If miner can decrypt wrapped pubkey, they possess correct genesis (or guessed the 256-bit key).

3. **Invalid addresses rejected by consensus**: Consensus rejects blocks with invalid reward addresses. This follows the same design pattern as legacy block creation.

4. **No API session requirements**: Everything works without Nexus Interface running.

## Block Signing Architecture

### Critical Distinction: Authentication vs. Signing

**Falcon Authentication (Miner Sessions)**
- Miners authenticate using disposable Falcon keys
- Provides secure, lightweight session management
- No wallet credentials exposed to miners
- Allows 500+ miners per node

**Wallet Signing (Block Consensus)**
- **ALL blocks MUST be wallet-signed per Nexus consensus**
- Node operator's wallet signs blocks via Autologin
- Rewards can be routed to miner addresses
- Network validates wallet signatures

### Important Clarification

"Stateless" refers to **MINER state** (no blockchain required), NOT block signing:
- **Miner side**: Stateless (no blockchain required)
- **Block signing**: Wallet-signed (consensus requirement)

### Node Configuration Requirements

**For Pool Operators:**
```bash
# nexus.conf - REQUIRED for block signing
-autologin=username:password    # Wallet auto-login for block signing
# OR
-unlock=mining                  # Alternative to autologin

# Optional: Whitelist miner Falcon keys
-minerallowkey=<falcon_pubkey_hash>
```

**Architecture Flow:**
1. Miner authenticates with Falcon keys (session security)
2. Miner requests block template
3. **Node creates block signed with its wallet** (consensus requirement)
4. Block rewards routed to miner's specified address
5. Miner performs PoW and submits signed block
6. Network validates wallet signature before accepting block

### Error Messages

If wallet is not unlocked, you'll see:
```
Mining not unlocked - use -unlock=mining or -autologin=username:password
CRITICAL: Nexus consensus requires wallet-signed blocks
Falcon authentication is for miner sessions, NOT block signing
```

If a block lacks wallet signature, network rejects it:
```
ProcessPacket : MinerLLP: SUBMIT_BLOCK sign block failed
Block Rejected by Nexus Network.
```

