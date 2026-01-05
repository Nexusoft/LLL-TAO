# Falcon-1024 Migration Guide

## Executive Summary

Nexus now supports **Falcon-1024** (NIST Level 5) alongside Falcon-512 (NIST Level 1) for post-quantum signature security in stateless mining. This guide explains the upgrade, configuration, and migration path.

**Important Architectural Note:**
- **Disposable Falcon** (session-based, NOT on blockchain): Can use Falcon-512 OR Falcon-1024
- **Physical Falcon** (permanent, ON blockchain): ALWAYS uses Falcon-512, NEVER Falcon-1024

This design minimizes permanent blockchain overhead while allowing maximum security for real-time mining sessions.

---

## Why Falcon-1024?

### Security Benefits
- **256-bit quantum security** (vs. 128-bit for Falcon-512)
- **2^64 × stronger** against quantum attacks
- **RSA-4096 equivalent** classical security
- **NIST Level 5** - Maximum standardized security

### Practical Costs (Disposable Falcon Only)
- **95% signature overhead**: 809 → 1577 bytes (768 bytes increase)
- **Zero blockchain overhead**: Disposable signatures not stored on blockchain
- **<1.5ms signing time**: Still sub-second for mining operations
- **Full backward compatibility**: Falcon-512 continues to work

**Note:** Physical Falcon signatures (when implemented) will ALWAYS use Falcon-512 to minimize permanent blockchain bloat.

---

## Dual Signature Architecture

Nexus uses **two distinct types** of Falcon signatures:

### 1. Disposable Falcon (Session-Based)
- **Purpose:** Real-time mining protocol authentication
- **Storage:** NOT stored on blockchain (zero blockchain overhead)
- **Lifetime:** Single mining session only
- **Version Support:** Falcon-512 OR Falcon-1024 (miner's choice)
- **Default:** Falcon-512 (proven, smaller)
- **Opt-in:** Falcon-1024 (maximum quantum security)
- **This PR enables:** Support for both versions in Disposable Falcon

### 2. Physical Falcon (Permanent) - **Now Implemented**
- **Purpose:** Emergency backup block authorship proof
- **Storage:** STORED on blockchain permanently
- **Lifetime:** Forever (immutable blockchain record)
- **Version Support:** Falcon-512 OR Falcon-1024 (SAME as Disposable)
- **Key Bonding:** Must use SAME key pair as Disposable Falcon
- **Variable Size:** 809 bytes (Falcon-512) or 1577 bytes (Falcon-1024)
- **Default:** Always enabled on node (auto-detects version)
- **Miner Choice:** Security (1024) vs Blockchain Overhead (512)
- **Implementation:** Auto-detects and validates both versions

### Key Bonding Architecture ⭐ NEW
Miners use a **single Falcon key pair** for BOTH signature types:
- **Cannot mix versions:** If using Falcon-512, both Disposable AND Physical use 512
- **Cannot mix versions:** If using Falcon-1024, both Disposable AND Physical use 1024
- **Reason:** Miner can only configure ONE Falcon key pair (bonded to alias/session)
- **Node behavior:** Auto-detects version from public key size
- **Verification:** Both signatures validated with same key pair

### Design Rationale
- **Key Bonding:** Single key pair for both Disposable and Physical signatures
- **Miner Choice:** Choose Falcon-512 (smaller) OR Falcon-1024 (more secure)
- **Disposable:** Zero blockchain overhead regardless of version
- **Physical:** Variable blockchain overhead (809 or 1577 bytes, miner's choice)
- **Trade-off:** Security level vs permanent blockchain storage cost

---

## Deployment Architecture

### **Hardcoded Acceptance** (Default) - No Configuration
Node automatically accepts BOTH Falcon versions for BOTH signature types:
The node operates in "stealth mode" by default for Disposable Falcon signatures:
- ✅ Accepts **both** Falcon-512 and Falcon-1024 disposable signatures
- ✅ Auto-detects version from miner's public key size
- ✅ Zero configuration required for node operators
- ✅ Seamless protocol upgrade without network fork

### **Miner Choice** - For Disposable Falcon
Miners choose their Disposable Falcon version:
- 🔧 Default: Falcon-512 (proven, faster, smaller)
- 🔧 Opt-in: Falcon-1024 (maximum quantum resistance)
- 🔧 Configuration: Single flag in `miner.conf`
- 🔧 Future: Default will change to Falcon-1024 ON in NexusMiner

**Note:** Physical Falcon (when implemented) will always use Falcon-512 regardless of miner config.

---

## Configuration

### Node Configuration (`nexus.conf`)

**NO CONFIGURATION REQUIRED**

The node behavior is hardcoded:
- Always accepts Disposable Falcon-512 OR Falcon-1024 (auto-detected)
- Always accepts Physical Falcon-512 ONLY (enforced)
- Both can be present in the SAME Submit Block Structure

```ini
# Falcon signature acceptance is hardcoded - no parameters needed
# Node ALWAYS accepts:
#   - Disposable Falcon: 512 OR 1024 (miner's choice)
#   - Physical Falcon: 512 ONLY (enforced)
```

**Node operators: No action required.** Acceptance is automatic and cannot be disabled.

---

### Miner Configuration (`miner.conf` in NexusMiner)

```ini
# Disposable Falcon Signature Mode
# Default: Falcon-512 (proven, faster, smaller signatures)
# Opt-in: Falcon-1024 (maximum quantum resistance)
# Future: Will default to Falcon-1024 ON in upcoming NexusMiner release
falcon1024=0  # Currently OFF by default (use Falcon-512 for disposable signatures)

# To enable Falcon-1024 for disposable signatures:
# falcon1024=1

# Physical Falcon signature (now implemented)
# ALWAYS uses Falcon-512, never Falcon-1024
# Default OFF to minimize permanent blockchain bloat
physicalsigner=0  # OFF by default (enable only when quantum threat requires)
```

**Miner operators:**
- **No action required** to keep using Falcon-512 for disposable signatures
- **Set `falcon1024=1`** to upgrade disposable signatures to maximum quantum security
- **Note:** Physical signatures (when enabled) always use Falcon-512 regardless of falcon1024 setting
- **Physical Falcon:** Keep OFF (default) unless quantum computing requires permanent blockchain proof 
- **No action required** to keep using Falcon-512
- **Set `falcon1024=1`** to upgrade to maximum quantum security

---

## Key Generation

### Falcon-512 (Default)
```bash
# Generate Falcon-512 keys (default)
./falcon_keygen -o miner.conf
```

Output in `miner.conf`:
```ini
[falcon]
version = 512
pubkey = "89700..."  # 897 bytes
privkey = "128100..." # 1281 bytes
```

### Falcon-1024 (Opt-in)
```bash
# Generate Falcon-1024 keys
./falcon_keygen --falcon1024 -o miner.conf
```

Output in `miner.conf`:
```ini
[falcon]
version = 1024
pubkey = "179300..." # 1793 bytes
privkey = "230500..." # 2305 bytes
```

---

## Migration Timeline

### **Phase 1: Stealth Deployment** (Current)
- ✅ Nodes accept both Falcon-512 and Falcon-1024
- ✅ Miners continue using Falcon-512 by default
- ✅ Early adopters can opt-in to Falcon-1024
- ✅ Zero disruption to existing operations

### **Phase 2: Testnet Validation** (Q2 2025)
- 🧪 Extended Falcon-1024 testing on testnet
- 🧪 Performance benchmarking under load
- 🧪 Security audit completion

### **Phase 3: Early Adopter Opt-in** (Q3 2025)
- 📢 Announce Falcon-1024 availability
- 📢 Provide migration tools and guides
- 📢 Monitor adoption metrics

### **Phase 4: Long-term Evolution** (2026+)
- 🔮 Evaluate making Falcon-1024 the default
- 🔮 Based on adoption, performance, and quantum threat landscape

---

## Security Analysis

### Quantum Threat Model

| Algorithm | Classical Bits | Quantum Bits | RSA Equivalent | NIST Level |
|-----------|---------------|--------------|----------------|------------|
| Falcon-512 | 128 | 128 | RSA-2048 | Level 1 |
| **Falcon-1024** | **256** | **256** | **RSA-4096** | **Level 5** |

**Falcon-1024 provides 2^64 times more resistance against quantum attacks.**

### Attack Resistance

```
Falcon-512:  2^128 operations to break (current standard)
Falcon-1024: 2^256 operations to break (maximum security)

Quantum speedup (Grover's algorithm):
Falcon-512:  ~2^64 quantum operations
Falcon-1024: ~2^128 quantum operations

Difference: 2^64 × more secure
```

### Recommendation
- **Use Falcon-512** if you prioritize:
  - Smaller signatures (809 vs 1577 bytes)
  - Slightly faster signing (<1ms vs <1.5ms)
  - Proven track record in production

- **Use Falcon-1024** if you prioritize:
  - Maximum quantum attack resistance
  - Long-term future-proofing (10+ years)
  - Compliance with highest security standards

---

## Performance Comparison

### Key Generation
| Metric | Falcon-512 | Falcon-1024 |
|--------|-----------|------------|
| Public Key | 897 bytes | 1793 bytes |
| Private Key | 1281 bytes | 2305 bytes |
| Generation Time | ~5ms | ~8ms |

### Signing
| Metric | Falcon-512 | Falcon-1024 |
|--------|-----------|------------|
| Signature Size | 809 bytes | 1577 bytes |
| Signing Time | <1ms | <1.5ms |
| Overhead per Block | 809 bytes | 1577 bytes |

### Network Impact
| Metric | Falcon-512 | Falcon-1024 |
|--------|-----------|------------|
| Block Overhead (2MB) | 0.040% | 0.077% |
| Yearly Bandwidth (1000 blocks/day) | 295 MB | 575 MB |
| **Difference** | - | **+280 MB/year** |

**Conclusion:** Network impact is negligible for both versions.

---

## Troubleshooting

### "Invalid public key size" Error
**Symptom:** Miner authentication fails with size error.

**Cause:** Mismatch between key version and miner configuration.

**Solution:**
```bash
# Check your miner.conf falcon version
grep "version" miner.conf

# Regenerate keys matching your falcon1024 setting
./falcon_keygen --falcon1024 -o miner.conf  # For 1024
./falcon_keygen -o miner.conf              # For 512
```

### "Signature verification failed" Error
**Symptom:** Block submissions rejected.

**Cause:** Using wrong key version or corrupted keys.

**Solution:**
1. Verify key integrity in `miner.conf`
2. Regenerate keys if corrupted
3. Ensure `falcon1024` flag matches key version

### Node Doesn't Accept Falcon-1024
**Symptom:** Miner with Falcon-1024 keys gets rejected.

**Cause:** Node running outdated software or `falcon1024=0` set.

**Solution:**
```ini
# In nexus.conf, ensure:
falcon1024=1  # Enable stealth mode (default)
```
Restart node after configuration change.

---

## FAQ

### Q: Should I upgrade to Falcon-1024?
**A:** Not required. Falcon-512 remains secure and is the default. Upgrade if you want maximum quantum resistance.

### Q: Can old and new versions coexist?
**A:** Yes! Nodes in stealth mode accept both. No network fork required.

### Q: What's the network bandwidth impact?
**A:** +215 MB/year per miner (negligible). Less than 0.1% block overhead.

### Q: Is there a performance penalty?
**A:** Minimal. Signing: +0.5ms. Verification: +0.3ms. Not noticeable in mining.

### Q: Can I switch between versions?
**A:** Yes. Regenerate keys with `falcon_keygen` and update `miner.conf`.

### Q: When will Falcon-1024 become the default?
**A:** TBD. Depends on adoption, testing, and quantum threat timeline. Earliest: 2026.

---

## Best Practices

### For Node Operators
1. ✅ Leave `falcon1024=1` (default) - enables stealth mode
2. ✅ Monitor logs for version distribution
3. ✅ Keep node software up-to-date

### For Miner Operators
1. ✅ Start with Falcon-512 (default, proven)
2. ✅ Test Falcon-1024 on testnet first
3. ✅ Backup keys securely (both versions)
4. ✅ Use hardware wallets for private key storage (future)

### For Developers
1. ✅ Always use `FalconVerify::DetectVersionFromPubkey()` for auto-detection
2. ✅ Store session version after authentication
3. ✅ Use version-aware size helpers from `falcon_constants_v2.h`
4. ✅ Test with both versions in integration tests

---

## Technical References

- **Falcon Specification:** https://falcon-sign.info/
- **NIST PQC:** https://csrc.nist.gov/projects/post-quantum-cryptography
- **Nexus Stateless Mining:** See `docs/stateless_mining.md`
- **ChaCha20-Poly1305:** RFC 8439

---

## Support

- **GitHub Issues:** https://github.com/Nexusoft/LLL-TAO/issues
- **Discord:** https://discord.gg/nexus
- **Forum:** https://nexus.io/forum

---

**Last Updated:** January 2026  
**Status:** Production Ready  
**Security Review:** Pending (Phase 2)
