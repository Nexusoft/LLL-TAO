# Consensus Debug Cheat Sheet

Validation failure diagnostics and AI-assisted resolution for consensus-related issues.

---

## Block Validation Failures

### Header Validation Failed
**Symptoms:** Block rejected at header check stage

**Common Causes:**
- Block size exceeds maximum limit
- Timestamp outside acceptable range
- Version doesn't match current timelock

**AI Prompt:** "What are the block size and timestamp limits in TritiumBlock::Check()?"

### Producer Validation Failed
**Symptoms:** Block rejected - producer transaction invalid

**Diagnostic Steps:**
1. Verify producer transaction exists in block
2. Check if coinstake transaction is properly formed
3. Validate trust timestamp for stake blocks

**AI Prompt:** "How is the producer transaction validated during block acceptance?"

---

## Channel-Specific Failures

### Proof of Stake Rejection
**Symptoms:** PoS block (Channel 0) rejected

**Diagnostic Steps:**
1. Verify stake amount meets minimum requirement
2. Check trust score calculation
3. Validate coinstake transaction rules

**AI Prompt:** "What are the validation rules for Proof of Stake blocks on Channel 0?"

### Prime Mining Rejection
**Symptoms:** Prime block (Channel 1) rejected

**Diagnostic Steps:**
1. Verify prime chain meets cluster size requirement
2. Check difficulty target is met
3. Validate proof format

**AI Prompt:** "How is a prime chain validated for Channel 1 blocks?"

### Hash Mining Rejection
**Symptoms:** Hash block (Channel 2) rejected

**Diagnostic Steps:**
1. Verify hash meets difficulty target
2. Check nonce is valid
3. Validate block hash computation

**AI Prompt:** "What is the hash validation process for Channel 2 blocks?"

---

## Merkle Root Failures

### Merkle Mismatch
**Symptoms:** Block rejected - merkle root doesn't match

**Diagnostic Steps:**
1. Verify all transactions are included in merkle tree
2. Check transaction ordering matches expected order
3. Rebuild merkle tree manually to find discrepancy

**AI Prompt:** "How is the merkle root computed and validated in block acceptance?"

---

## Transaction Verification Failures

### Invalid Signature
**Symptoms:** Transaction rejected - signature verification failed

**Diagnostic Steps:**
1. Check signing key matches expected public key
2. Verify transaction data wasn't modified after signing
3. Check for key rotation issues

**AI Prompt:** "How are transaction signatures verified in the Tritium protocol?"

### Duplicate Transaction
**Symptoms:** Block rejected - duplicate transaction detected

**Diagnostic Steps:**
1. Check if transaction exists in another pending block
2. Verify mempool state for conflicts
3. Check for double-spend attempts

**AI Prompt:** "How are duplicate and conflicting transactions detected during block validation?"

---

## State Consistency Issues

### Register State Mismatch
**Symptoms:** POSTSTATE checksum doesn't match expected value

**Diagnostic Steps:**
1. Verify PRESTATE was captured correctly
2. Check operation execution order
3. Validate checksum computation

**AI Prompt:** "Explain the PRESTATE/POSTSTATE checksum mechanism in TAO register operations"

---

## Validation Flow Quick Reference

```
Block Received
  → Size Check         (reject if too large)
  → Timestamp Check    (reject if out of range)
  → Version Check      (reject if wrong version)
  → Producer Check     (reject if invalid producer)
  → Channel Check      (PoS/Prime/Hash specific)
  → Merkle Check       (reject if root mismatch)
  → Transaction Check  (reject if any tx invalid)
  → Accept             (commit to chain)
```

---

## Cross-References

- [Consensus Validation Flow](../../diagrams/architecture/consensus-validation-flow.md)
- [Ledger State Machine](../../diagrams/architecture/ledger-state-machine.md)
- [Mining Debug](mining-debug.md)
