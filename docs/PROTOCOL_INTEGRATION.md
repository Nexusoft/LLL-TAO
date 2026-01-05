# Falcon Protocol Integration - PR #122

## Overview

This document describes the complete protocol-level integration of Falcon-512/1024 dual version support for Nexus stateless mining, implemented in PR #122.

## Table of Contents

- [Executive Summary](#executive-summary)
- [Session Management](#session-management)
- [Packet Formats](#packet-formats)
- [Version Detection](#version-detection)
- [Physical Falcon Signatures](#physical-falcon-signatures)
- [Key Bonding](#key-bonding)
- [Backward Compatibility](#backward-compatibility)
- [Security Considerations](#security-considerations)
- [Testing](#testing)

---

## Executive Summary

PR #122 implements dual Falcon version support at the protocol level, allowing miners to use either Falcon-512 (NIST Level 1) or Falcon-1024 (NIST Level 5) signatures transparently. The implementation includes:

- ✅ **Automatic version detection** from public key size
- ✅ **Session-based version tracking** (version persists per mining session)
- ✅ **Optional Physical Falcon signatures** (blockchain-stored proof of authorship)
- ✅ **Key bonding enforcement** (prevents mixing Falcon-512 and Falcon-1024)
- ✅ **Full backward compatibility** (accepts blocks without Physical Falcon)

---

## Session Management

### Falcon Version Tracking

Each mining session tracks:

```cpp
struct MiningContext {
    // ... existing fields ...
    
    /* Falcon version tracking (PR #122) */
    LLC::FalconVersion nFalconVersion;      // Detected version (512 or 1024)
    bool fFalconVersionDetected;            // Whether version has been detected
    std::vector<uint8_t> vchPhysicalSignature; // Optional Physical Falcon sig
    bool fPhysicalFalconPresent;            // Whether Physical Falcon is present
};
```

### Session Lifecycle

1. **MINER_AUTH_INIT**: Miner sends public key
2. **MINER_AUTH_RESPONSE**: Miner signs challenge
3. **Auto-detection**: Node detects Falcon version from public key size
4. **Storage**: Detected version stored in `MiningContext`
5. **SUBMIT_BLOCK**: Node uses stored version for verification
6. **Session End**: Version cleared on session reset

---

## Packet Formats

### MINER_AUTH_RESPONSE Packet

**Plaintext format** (before ChaCha20 encryption):
```
[version(1)] [pubkey(var)] [challenge(var)] [signature(var)] [timestamp(8)]
```

**Public Key Sizes:**
- Falcon-512: 897 bytes
- Falcon-1024: 1793 bytes

**Signature Sizes (Constant-Time):**
- Falcon-512: 809 bytes
- Falcon-1024: 1577 bytes

### SUBMIT_BLOCK Packet

**Format with Disposable signature only:**
```
[block(var)] [timestamp(8)] [siglen(2)] [disposable_signature(var)]
```

**Format with Disposable + Physical signatures:**
```
[block(var)] [timestamp(8)] 
[siglen(2)] [disposable_signature(var)] 
[physiglen(2)] [physical_signature(var)]
```

**Notes:**
- Physical signature is **OPTIONAL**
- Both signatures must use the **SAME** Falcon version (key bonding)
- Block size can vary from 216 bytes (empty) to 2MB (with transactions)

---

## Version Detection

### Auto-Detection Process

**Step 1: Extract Public Key**
```cpp
std::vector<uint8_t> vchPubKey;
// Extract from MINER_AUTH packet
```

**Step 2: Detect Version from Size**
```cpp
LLC::FalconVersion detectedVersion;
if (!LLP::FalconVerify::VerifyPublicKey(vchPubKey, detectedVersion))
    return error("Invalid Falcon public key");
```

**Step 3: Store in Session**
```cpp
context = context.WithFalconVersion(detectedVersion);
```

### Detection Rules

| Public Key Size | Detected Version | Signature CT Size |
|----------------|------------------|-------------------|
| 897 bytes      | Falcon-512       | 809 bytes         |
| 1793 bytes     | Falcon-1024      | 1577 bytes        |
| Other          | INVALID          | N/A               |

---

## Physical Falcon Signatures

### Purpose

Physical Falcon signatures provide **permanent blockchain-stored proof of block authorship**. Unlike disposable signatures (which are verified and discarded), Physical Falcon signatures are:

- Stored on the blockchain
- Optional (backward compatible)
- Use the same key pair as disposable signatures (key bonding)

### When to Use Physical Falcon

Physical Falcon signatures are recommended for:

- **High-security mining operations**
- **Regulatory compliance** (permanent audit trail)
- **Dispute resolution** (proof of authorship)
- **Emergency backup** (when disposable signature is insufficient)

### Implementation

**Miner Side:**
```cpp
// Sign block with disposable key (mandatory)
std::vector<uint8_t> disposableSig = SignDisposable(block);

// OPTIONALLY sign with physical key (same key pair)
std::vector<uint8_t> physicalSig = SignPhysical(block);

// Send both signatures
SendSubmitBlock(block, disposableSig, physicalSig);
```

**Node Side:**
```cpp
// Verify disposable signature (mandatory)
if (!VerifyDisposable(disposableSig))
    return REJECT;

// Verify physical signature IF PRESENT (optional)
if (physicalSigPresent) {
    if (!VerifyPhysical(physicalSig))
        return REJECT;
    
    // Enforce key bonding
    if (physicalSig.size() != disposableSig.size())
        return REJECT; // Version mismatch
}
```

---

## Key Bonding

### Definition

**Key bonding** ensures that a miner uses the **SAME Falcon version** for both disposable and physical signatures. This prevents:

- Version downgrade attacks
- Signature forgery attempts
- Protocol confusion attacks

### Enforcement

**Rule:** If both disposable and physical signatures are present, they MUST have the same signature size.

```cpp
// Validate signature sizes match
size_t expectedSize = LLC::FalconConstants::GetSignatureCTSize(context.nFalconVersion);

if (vchPhysicalSignature.size() != expectedSize)
    return error("Key bonding violation: signature size mismatch");
```

### Example

**Valid:**
- Disposable: Falcon-512 (809 bytes)
- Physical: Falcon-512 (809 bytes)
- ✅ ACCEPTED

**Invalid:**
- Disposable: Falcon-512 (809 bytes)
- Physical: Falcon-1024 (1577 bytes)
- ❌ REJECTED (key bonding violation)

---

## Backward Compatibility

### Design Principles

1. **Physical Falcon is OPTIONAL**
   - Miners can submit blocks WITHOUT Physical Falcon
   - Existing miners continue working without changes

2. **Version Detection is TRANSPARENT**
   - Miners don't need to specify their Falcon version
   - Node auto-detects from public key size

3. **Graceful Degradation**
   - If Physical Falcon verification fails, only reject the block
   - Don't disconnect the miner (allow retry)

### Migration Path

**Phase 1:** Node deploys PR #122 (accepts both versions)
```
Falcon-512 miners: ✅ Continue working
Falcon-1024 miners: ✅ Start working
Physical Falcon: ⚠️ Optional
```

**Phase 2:** Miners upgrade to support Physical Falcon
```
Falcon-512 miners: ✅ Can add Physical Falcon
Falcon-1024 miners: ✅ Can add Physical Falcon
Physical Falcon: ✅ Recommended
```

**Phase 3:** Physical Falcon becomes standard
```
All miners: ✅ Use Physical Falcon for audit trail
Backward compat: ✅ Still accept without Physical
```

---

## Security Considerations

### Threat Model

**Threats Mitigated:**
1. ✅ Quantum attacks (post-quantum signatures)
2. ✅ Key compromise (disposable keys)
3. ✅ Version downgrade attacks (key bonding)
4. ✅ Replay attacks (timestamp validation)
5. ✅ Signature forgery (constant-time signatures)

**Threats NOT Mitigated:**
1. ⚠️ Miner collusion (separate issue)
2. ⚠️ 51% attacks (consensus-level issue)

### Best Practices

1. **Always use ChaCha20 encryption** for packet transport
2. **Validate signature sizes** match expected version
3. **Enforce timestamp freshness** (30-second window)
4. **Store session keys securely** (memory-only, no persistence)
5. **Clear session data** on disconnect

### Cryptographic Parameters

| Parameter | Falcon-512 | Falcon-1024 |
|-----------|------------|-------------|
| Public Key | 897 bytes | 1793 bytes |
| Private Key | 1281 bytes | 2305 bytes |
| Signature (CT) | 809 bytes | 1577 bytes |
| Security Level | NIST Level 1 | NIST Level 5 |
| Quantum Security | ~128-bit | ~256-bit |

---

## Testing

### Test Coverage

**Unit Tests:**
- ✅ `falcon_protocol_integration.cpp` (20+ test sections)
- ✅ Version detection from public keys
- ✅ Signature size validation
- ✅ Key bonding enforcement
- ✅ Backward compatibility scenarios

**Integration Tests:**
- ✅ MINER_AUTH with Falcon-512
- ✅ MINER_AUTH with Falcon-1024
- ✅ SUBMIT_BLOCK disposable-only
- ✅ SUBMIT_BLOCK with Physical Falcon
- ✅ Cross-version scenarios
- ✅ Error handling

### Test Scenarios

**Success Paths:**
1. Falcon-512 authentication → disposable-only submission ✅
2. Falcon-512 authentication → disposable + physical submission ✅
3. Falcon-1024 authentication → disposable-only submission ✅
4. Falcon-1024 authentication → disposable + physical submission ✅

**Error Paths:**
1. Invalid public key size → REJECT ✅
2. Signature size mismatch → REJECT ✅
3. Key bonding violation → REJECT ✅
4. Unauthenticated submission → REJECT ✅

---

## API Reference

### MiningContext Methods

```cpp
// Set detected Falcon version
MiningContext WithFalconVersion(LLC::FalconVersion version) const;

// Store Physical Falcon signature
MiningContext WithPhysicalSignature(const std::vector<uint8_t>& vSig) const;
```

### FalconVerify Functions

```cpp
// Auto-detect version from public key
bool VerifyPublicKey(const std::vector<uint8_t>& pubkey, 
                    LLC::FalconVersion& detected);

// Verify signature with specific version
bool VerifySignature(const std::vector<uint8_t>& pubkey,
                    const std::vector<uint8_t>& message,
                    const std::vector<uint8_t>& signature,
                    LLC::FalconVersion version);

// Verify Physical Falcon signature (auto-detects version)
bool VerifyPhysicalFalconSignature(const std::vector<uint8_t>& pubkey,
                                   const std::vector<uint8_t>& message,
                                   const std::vector<uint8_t>& signature);
```

### FalconConstants Functions

```cpp
// Get signature size for version
size_t GetSignatureCTSize(FalconVersion version);
// Returns: 809 for Falcon-512, 1577 for Falcon-1024

// Get public key size for version
size_t GetPublicKeySize(FalconVersion version);
// Returns: 897 for Falcon-512, 1793 for Falcon-1024
```

---

## Performance Impact

### Computational Overhead

| Operation | Falcon-512 | Falcon-1024 | Delta |
|-----------|------------|-------------|-------|
| Key Generation | ~10ms | ~20ms | +100% |
| Signing | ~1ms | ~2ms | +100% |
| Verification | ~1ms | ~2ms | +100% |
| Network Transfer | +809 bytes | +1577 bytes | +768 bytes |

### Memory Overhead

| Component | Per Session | Notes |
|-----------|-------------|-------|
| Version tracking | 4 bytes | `LLC::FalconVersion` |
| Detection flag | 1 byte | `bool` |
| Physical signature | 0-1577 bytes | Optional |
| Total | ~5-1582 bytes | Minimal impact |

---

## Future Enhancements

### Potential Improvements

1. **Adaptive Version Selection**
   - Allow miners to switch versions dynamically
   - Based on network conditions or security requirements

2. **Signature Aggregation**
   - Combine multiple physical signatures
   - Reduce blockchain storage overhead

3. **Zero-Knowledge Proofs**
   - Prove signature validity without revealing signature
   - Enhanced privacy for mining operations

4. **Hardware Acceleration**
   - GPU/FPGA support for Falcon operations
   - Reduce signing/verification latency

---

## References

- **PR #121:** Core cryptographic layer (merged)
- **PR #122:** Protocol integration (this PR)
- **NIST PQC:** https://csrc.nist.gov/Projects/post-quantum-cryptography
- **Falcon Spec:** https://falcon-sign.info/

---

## Glossary

- **Disposable Falcon:** Session-specific signature (not stored on blockchain)
- **Physical Falcon:** Blockchain-stored signature (permanent proof of authorship)
- **Key Bonding:** Enforcement that both signatures use the same Falcon version
- **CT Signature:** Constant-Time signature (fixed size, timing-attack resistant)
- **Stealth Mode:** Node accepts both Falcon-512 and Falcon-1024 transparently

---

## Support

For questions or issues, please:
- Open an issue on GitHub
- Contact the development team
- Review test cases for examples

---

**Last Updated:** 2026-01-05  
**PR Number:** #122  
**Status:** Ready for review
