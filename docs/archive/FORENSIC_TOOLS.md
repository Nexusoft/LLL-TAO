# Forensic Tools Documentation

> **Note — Scope of this document:**
> The forensic tools described here measure the **sum-of-channel-heights vs unified height**
> discrepancy. This diagnostic is separate from block-template anchoring.
>
> - The `unified_height` field reported by these tools is the sum-based cross-check value.
>   It is **not** the value placed inside block templates; `pBlock->nHeight` is the
>   **channel target height** (`stateChannel.nChannelHeight + 1`).
> - A discrepancy within tolerance (≤ 5 blocks) does not affect block acceptance. Block
>   acceptance is gated on `hashPrevBlock == hashBestChain`, not on height arithmetic.
>
> For template anchoring semantics, see:
> **[Unified Tip and Channel Heights](../current/mining/unified-tip-and-channel-heights.md)**

## Overview

This document describes the comprehensive forensic analysis tools available in Nexus for blockchain health monitoring, fork analysis, and anomaly detection. These tools help operators diagnose channel height discrepancies, identify orphaned blocks, and assess blockchain integrity.

## Available Tools

### 1. `-forensicforks` / `-analyzeforks`

**Purpose:** Run comprehensive forensic analysis on channel height discrepancies and orphaned blocks.

**Usage:**
```bash
./nexus -forensicforks
```

**Output:**
```
═══════════════════════════════════════════════════════
       CHANNEL HEIGHT DISCREPANCY FORENSIC ANALYSIS      
═══════════════════════════════════════════════════════

CURRENT BLOCKCHAIN STATE:
   Stake channel:      2,068,487
   Prime channel:      2,302,664
   Hash channel:       2,166,272
   ───────────────────────────────────
   Calculated sum:     6,537,423
   Actual unified:     6,537,420
   Discrepancy:        +3

TOLERANCE CHECK: ✓ PASS
   Discrepancy (3) within tolerance (5)
   This is consistent with historical fork resolution.

ORPHANED BLOCK SCAN:
   Scanning for orphaned blocks from height 6527420 to 6537420...
   Found orphaned block at height 1234567 hash: 00abc...
   Found orphaned block at height 1234890 hash: 00def...
   Found orphaned block at height 1235001 hash: 00ghi...
   Found 3 orphaned blocks

ANALYSIS:
   Channel heights are AHEAD of unified height by 3 blocks
   This suggests orphaned blocks were counted in channels
   but excluded from unified height (expected behavior).

RECOMMENDATIONS:
   ✓ No action required - blockchain state is consistent
   ✓ Discrepancy within expected historical tolerance
   See docs/CHANNEL_HEIGHT_DISCREPANCY.md for background

═══════════════════════════════════════════════════════
```

**What it does:**
1. Scans blockchain for orphaned blocks (blocks with `hashNextBlock == 0`)
2. Compares sum of channel heights against unified height
3. Checks if discrepancy is within historical tolerance (5 blocks)
4. Provides diagnostic recommendations

**When to use:**
- After node sync to verify blockchain integrity
- When suspecting fork or database corruption
- For routine health monitoring
- When investigating unexpected behavior

**Exit behavior:** Exits immediately after analysis (does not start full node).

---

### 2. `-channelstats`

**Purpose:** Output channel height statistics in JSON format for programmatic access.

**Usage:**
```bash
./nexus -channelstats
```

**Output:**
```json
{
  "stake_height": 2068487,
  "prime_height": 2302664,
  "hash_height": 2166272,
  "calculated_sum": 6537423,
  "unified_height": 6537420,
  "discrepancy": 3,
  "abs_discrepancy": 3,
  "tolerance": 5,
  "within_tolerance": true,
  "orphaned_blocks": 3
}
```

**What it does:**
1. Retrieves current height for all three channels (Stake, Prime, Hash)
2. Calculates sum and compares to unified height
3. Scans for orphaned blocks
4. Outputs structured JSON data

**When to use:**
- For automated monitoring scripts
- Integration with external monitoring tools
- Programmatic blockchain health checks
- Building custom dashboards

**Exit behavior:** Exits immediately after output (does not start full node).

**Example usage in scripts:**
```bash
#!/bin/bash
# Monitor blockchain health
STATS=$(./nexus -channelstats 2>/dev/null)
WITHIN_TOLERANCE=$(echo $STATS | jq -r '.within_tolerance')

if [ "$WITHIN_TOLERANCE" != "true" ]; then
    echo "ALERT: Channel height discrepancy exceeds tolerance!"
    echo $STATS | jq .
    # Send alert notification
fi
```

---

### 3. `-verifyunified=N`

**Purpose:** Enable periodic unified height verification during normal operation.

**Usage:**
```bash
./nexus -verifyunified=10  # Verify every 10 blocks (default)
./nexus -verifyunified=0   # Verify every block (high overhead)
./nexus -verifyunified=100 # Verify every 100 blocks (low overhead)
```

**Default:** `10` (verify every 10 blocks)

**What it does:**
1. Integrates into `BlockState::SetBest()` after block processing
2. Checks unified height consistency every N blocks
3. Accepts discrepancies within historical tolerance (5 blocks)
4. Logs warnings but does NOT reject blocks (non-blocking)
5. Triggers fork callbacks on all channel managers if verification fails

**Logging output (normal):**
```
[SetBest] ═══ UNIFIED HEIGHT VERIFICATION (Height 6537420) ═══
[SetBest]    Stake:      2068487
[SetBest]    Prime:      2302664
[SetBest]    Hash:       2166272
[SetBest]    Calculated: 6537423
[SetBest]    Actual:     6537420
[SetBest]    Tolerance:  5
[SetBest] ✓ Unified height consistent (perfect match)
```

**Logging output (within tolerance):**
```
═══════════════════════════════════════════
⚠ UNIFIED HEIGHT WITHIN TOLERANCE
═══════════════════════════════════════════
   Stake channel:      2068487
   Prime channel:      2302664
   Hash channel:       2166272
   Calculated sum:     6537423
   Actual unified:     6537420
   Difference:         3
   Tolerance:          5

   NOTE: This is expected from pre-Tritium fork resolution
   where orphaned blocks were preserved for network resilience.
   See docs/CHANNEL_HEIGHT_DISCREPANCY.md for details.

✓ Unified height verification passed (within tolerance)
═══════════════════════════════════════════
```

**Logging output (CRITICAL ERROR):**
```
═══════════════════════════════════════════
❌ CRITICAL: Unified height mismatch exceeds tolerance!
═══════════════════════════════════════════
   Stake channel:      2068487
   Prime channel:      2302664
   Hash channel:       2166272
   Calculated sum:     6537430
   Actual unified:     6537420
   Difference:         10
   Tolerance:          5

   This indicates a NEW fork or database corruption!

   DIAGNOSIS: Channel heights are AHEAD of unified
   Possible causes:
     - Channel heights incorrectly incremented
     - Unified height not updated during block processing
     - Database corruption

   RECOMMENDED ACTIONS:
     1. Stop the node immediately
     2. Run: ./nexus -forensicforks (analyze the issue)
     3. Run: ./nexus -reindex (rebuild database)
     4. If problem persists, run: ./nexus -rescan
     5. Check for disk/hardware errors
═══════════════════════════════════════════
```

**When to use:**
- Default: Always enabled (verify every 10 blocks)
- Set to `0` for maximum safety (verify every block, slight performance impact)
- Set to `100+` for minimal overhead on low-power nodes
- Disable with very large value if verification is not desired

**Performance impact:**
- Negligible: O(1) operation (simple arithmetic and atomic state access)
- No disk I/O (uses cached blockchain state)
- Typical overhead: < 0.1ms per verification

---

## Forensic Data Structures

### ForensicForkInfo

**Structure:**
```cpp
struct ForensicForkInfo
{
    uint32_t nStakeHeight;      // Stake channel height
    uint32_t nPrimeHeight;      // Prime channel height
    uint32_t nHashHeight;       // Hash channel height
    uint64_t nCalculatedSum;    // Sum of all channel heights
    uint32_t nUnifiedHeight;    // Actual unified blockchain height
    int64_t nDiscrepancy;       // Difference between calculated and actual
    uint32_t nOrphanedBlocks;   // Number of orphaned blocks found
    bool fWithinTolerance;      // Is discrepancy within historical tolerance
};
```

**Usage:**
```cpp
LLP::ForensicForkInfo info = LLP::ChannelStateManager::AnalyzeChannelHeightDiscrepancy();

if(!info.fWithinTolerance)
{
    debug::error("Blockchain health check FAILED!");
    debug::error("Discrepancy: ", info.nDiscrepancy);
    debug::error("Orphaned blocks: ", info.nOrphanedBlocks);
    // Take corrective action
}
```

---

## Forensic Methods

### FindOrphanedBlocks()

**Signature:**
```cpp
static uint32_t FindOrphanedBlocks(uint32_t nMaxDepth = 10000);
```

**Purpose:** Scan blockchain backward to find orphaned blocks.

**Parameters:**
- `nMaxDepth`: Maximum number of blocks to scan backward (default: 10,000)

**Returns:** Number of orphaned blocks found

**Algorithm:**
1. Start from current best block
2. Scan backward up to `nMaxDepth` blocks
3. For each block, check if `hashNextBlock == 0` (orphaned marker)
4. Exclude chain tip (always has `hashNextBlock == 0`)
5. Count and log orphaned blocks

**Orphaned block definition:**
- Block exists on disk (preserved by Colin's design)
- Not part of best chain (`hashNextBlock == 0`)
- Not the current chain tip

**Usage:**
```cpp
uint32_t nOrphaned = LLP::ChannelStateManager::FindOrphanedBlocks(10000);
debug::log(0, "Found ", nOrphaned, " orphaned blocks in last 10,000 blocks");
```

**Performance:** O(n) where n = nMaxDepth. Typically completes in < 1 second for 10,000 blocks.

---

### AnalyzeChannelHeightDiscrepancy()

**Signature:**
```cpp
static ForensicForkInfo AnalyzeChannelHeightDiscrepancy();
```

**Purpose:** Generate comprehensive forensic report on channel height discrepancies.

**Returns:** `ForensicForkInfo` structure with complete analysis

**What it does:**
1. Retrieves current heights for all channels (Stake, Prime, Hash)
2. Calculates sum of channel heights
3. Compares to actual unified height
4. Determines if within tolerance (5 blocks)
5. Scans for orphaned blocks
6. Provides diagnostic analysis
7. Generates recommendations

**Diagnostic logic:**
```cpp
if(discrepancy > 0)
    // Channel heights AHEAD of unified (expected for historical +3)
else if(discrepancy < 0)
    // Unified AHEAD of channels (UNEXPECTED - corruption!)
else
    // Perfect match
```

**Usage:**
```cpp
LLP::ForensicForkInfo info = LLP::ChannelStateManager::AnalyzeChannelHeightDiscrepancy();

if(info.fWithinTolerance)
    debug::log(0, "Blockchain healthy (discrepancy: ", info.nDiscrepancy, ")");
else
    debug::error("Blockchain corruption detected!");
```

---

### GetChannelHeightStatistics()

**Signature:**
```cpp
static std::string GetChannelHeightStatistics();
```

**Purpose:** Get channel height statistics in JSON format for programmatic access.

**Returns:** JSON string with statistics

**JSON Schema:**
```json
{
  "stake_height": <uint32_t>,
  "prime_height": <uint32_t>,
  "hash_height": <uint32_t>,
  "calculated_sum": <uint64_t>,
  "unified_height": <uint32_t>,
  "discrepancy": <int64_t>,
  "abs_discrepancy": <uint32_t>,
  "tolerance": <uint32_t>,
  "within_tolerance": <bool>,
  "orphaned_blocks": <uint32_t>
}
```

**Usage:**
```cpp
std::string strStats = LLP::ChannelStateManager::GetChannelHeightStatistics();
debug::log(0, strStats);

// Or parse JSON
encoding::json jStats = encoding::json::parse(strStats);
if(!jStats["within_tolerance"].get<bool>())
{
    // Handle error
}
```

---

### VerifyUnifiedHeightWithTolerance()

**Signature:**
```cpp
static bool VerifyUnifiedHeightWithTolerance(
    uint32_t nStake,
    uint32_t nPrime,
    uint32_t nHash,
    uint32_t nUnified,
    uint32_t nTolerance = HISTORICAL_FORK_TOLERANCE
);
```

**Purpose:** Verify unified height with tolerance for historical anomalies.

**Parameters:**
- `nStake`: Stake channel height
- `nPrime`: Prime channel height
- `nHash`: Hash channel height
- `nUnified`: Actual unified blockchain height
- `nTolerance`: Maximum acceptable discrepancy (default: 5 blocks)

**Returns:** `true` if within tolerance, `false` if exceeds tolerance

**Logic:**
```cpp
uint32_t nCalculated = nStake + nPrime + nHash;
uint32_t nDifference = abs(nCalculated - nUnified);

if(nDifference == 0)
    return true; // Perfect match

if(nDifference <= nTolerance)
{
    log("Historical anomaly within tolerance");
    return true; // Accept with warning
}

error("CRITICAL: Unified height mismatch exceeds tolerance!");
return false;
```

**Tolerance rationale:**
- `HISTORICAL_FORK_TOLERANCE = 5 blocks`
- Accepts known +3 historical anomaly (with warning)
- Detects NEW forks/corruption when discrepancy > 5

**Usage:**
```cpp
uint32_t nStake = 2068487;
uint32_t nPrime = 2302664;
uint32_t nHash = 2166272;
uint32_t nUnified = 6537420;

bool fValid = LLP::ChannelStateManager::VerifyUnifiedHeightWithTolerance(
    nStake, nPrime, nHash, nUnified);

if(!fValid)
{
    debug::error("Blockchain integrity check FAILED!");
    // Take corrective action
}
```

---

## Integration Examples

### 1. Automated Health Monitoring Script

```bash
#!/bin/bash
# health_monitor.sh - Monitor blockchain health every hour

while true; do
    echo "[$(date)] Running blockchain health check..."
    
    # Get statistics
    STATS=$(./nexus -channelstats 2>/dev/null)
    
    # Parse JSON
    WITHIN_TOLERANCE=$(echo $STATS | jq -r '.within_tolerance')
    DISCREPANCY=$(echo $STATS | jq -r '.discrepancy')
    ORPHANED=$(echo $STATS | jq -r '.orphaned_blocks')
    
    # Check health
    if [ "$WITHIN_TOLERANCE" = "true" ]; then
        echo "✓ Blockchain healthy (discrepancy: $DISCREPANCY, orphaned: $ORPHANED)"
    else
        echo "❌ Blockchain health check FAILED!"
        echo "Discrepancy: $DISCREPANCY blocks"
        echo "Running forensic analysis..."
        ./nexus -forensicforks
        
        # Send alert
        mail -s "Nexus Health Alert" admin@example.com <<EOF
Blockchain health check failed!
Discrepancy: $DISCREPANCY blocks
Orphaned blocks: $ORPHANED

Full stats:
$STATS
EOF
    fi
    
    # Wait 1 hour
    sleep 3600
done
```

### 2. Prometheus Metrics Exporter

```bash
#!/bin/bash
# nexus_exporter.sh - Export metrics for Prometheus

while true; do
    STATS=$(./nexus -channelstats 2>/dev/null)
    
    # Extract metrics
    STAKE=$(echo $STATS | jq -r '.stake_height')
    PRIME=$(echo $STATS | jq -r '.prime_height')
    HASH=$(echo $STATS | jq -r '.hash_height')
    UNIFIED=$(echo $STATS | jq -r '.unified_height')
    DISCREPANCY=$(echo $STATS | jq -r '.abs_discrepancy')
    ORPHANED=$(echo $STATS | jq -r '.orphaned_blocks')
    TOLERANCE=$(echo $STATS | jq -r '.within_tolerance' | sed 's/true/1/; s/false/0/')
    
    # Write Prometheus format
    cat > /var/lib/prometheus/node-exporter/nexus.prom <<EOF
# HELP nexus_stake_height Stake channel height
# TYPE nexus_stake_height gauge
nexus_stake_height $STAKE

# HELP nexus_prime_height Prime channel height
# TYPE nexus_prime_height gauge
nexus_prime_height $PRIME

# HELP nexus_hash_height Hash channel height
# TYPE nexus_hash_height gauge
nexus_hash_height $HASH

# HELP nexus_unified_height Unified blockchain height
# TYPE nexus_unified_height gauge
nexus_unified_height $UNIFIED

# HELP nexus_channel_discrepancy Channel height discrepancy
# TYPE nexus_channel_discrepancy gauge
nexus_channel_discrepancy $DISCREPANCY

# HELP nexus_orphaned_blocks Number of orphaned blocks
# TYPE nexus_orphaned_blocks gauge
nexus_orphaned_blocks $ORPHANED

# HELP nexus_within_tolerance Whether discrepancy is within tolerance
# TYPE nexus_within_tolerance gauge
nexus_within_tolerance $TOLERANCE
EOF
    
    sleep 60
done
```

### 3. Pre-upgrade Health Check

```bash
#!/bin/bash
# pre_upgrade_check.sh - Verify blockchain health before upgrade

echo "Running pre-upgrade blockchain health check..."

# Run forensic analysis
./nexus -forensicforks > /tmp/forensic_report.txt

# Check if within tolerance
if grep -q "TOLERANCE CHECK: ✓ PASS" /tmp/forensic_report.txt; then
    echo "✓ Blockchain health check PASSED"
    echo "Safe to proceed with upgrade"
    exit 0
else
    echo "❌ Blockchain health check FAILED"
    echo "DO NOT UPGRADE - fix blockchain issues first!"
    cat /tmp/forensic_report.txt
    exit 1
fi
```

---

## Troubleshooting Guide

### Discrepancy Exceeds Tolerance

**Symptom:**
```
❌ CRITICAL: Unified height mismatch exceeds tolerance!
   Difference: 10
   Tolerance: 5
```

**Diagnosis:**
1. Run forensic analysis: `./nexus -forensicforks`
2. Check for orphaned blocks
3. Verify disk integrity

**Solutions:**
```bash
# Solution 1: Reindex blockchain
./nexus -reindex

# Solution 2: Rescan blockchain
./nexus -rescan

# Solution 3: Full resync (nuclear option)
rm -rf ~/.Nexus/blocks
./nexus
```

### High Number of Orphaned Blocks

**Symptom:**
```
Found 100 orphaned blocks
```

**Diagnosis:**
- More than 3-5 orphaned blocks is unusual
- May indicate fork resolution issues
- May indicate network partition

**Investigation:**
```bash
# Get orphaned block hashes
./nexus -forensicforks | grep "Found orphaned block"

# Check if orphaned blocks form a pattern
# (e.g., all at same height range → fork resolution event)
```

**Action:**
- If orphaned blocks are clustered: Historical fork resolution (normal)
- If orphaned blocks are scattered: Ongoing fork issues (investigate)

### Discrepancy Direction Unexpected

**Symptom:**
```
ANALYSIS:
   Unified height is AHEAD of channel heights by 5 blocks
```

**This is UNEXPECTED** - unified should NEVER be ahead of sum of channels!

**Diagnosis:**
- Database corruption
- Incomplete block indexing
- Bug in channel height tracking

**Solutions:**
```bash
# Reindex blockchain (recommended)
./nexus -reindex

# If reindex fails, full resync
rm -rf ~/.Nexus/blocks
./nexus
```

---

## Best Practices

### 1. Regular Health Monitoring

**Recommendation:** Run `-forensicforks` after every major event:
- After node restart
- After sync completion
- After software upgrade
- After network partition
- Monthly for long-running nodes

### 2. Automated Monitoring

**Recommendation:** Set up automated health monitoring:
```bash
# Add to crontab
0 */6 * * * /path/to/health_monitor.sh
```

### 3. Tolerance Configuration

**Recommendation:** Use default `-verifyunified=10` for most nodes.

**Adjust if:**
- High-security node: `-verifyunified=1` (every block)
- Low-power node: `-verifyunified=100` (reduce overhead)
- Development node: `-verifyunified=0` (disabled, not recommended)

### 4. Pre-upgrade Checks

**Recommendation:** Always verify blockchain health before upgrades:
```bash
./nexus -forensicforks
# Only proceed with upgrade if PASS
```

### 5. Backup Before Repairs

**Recommendation:** Always backup before `-reindex` or `-rescan`:
```bash
# Backup blockchain data
tar -czf nexus_backup_$(date +%Y%m%d).tar.gz ~/.Nexus/

# Then perform repair
./nexus -reindex
```

---

## References

- **Channel Height Discrepancy**: `docs/CHANNEL_HEIGHT_DISCREPANCY.md`
- **Channel State Manager**: `src/LLP/include/channel_state_manager.h`
- **Implementation**: `src/LLP/channel_state_manager.cpp`
- **Integration**: `src/TAO/Ledger/state.cpp` (BlockState::SetBest)

---

## Conclusion

The forensic tools provide comprehensive blockchain health monitoring and diagnostic capabilities. Use these tools regularly to maintain blockchain integrity and quickly diagnose any issues.

**Key Takeaways:**
- `-forensicforks`: Comprehensive analysis (run after major events)
- `-channelstats`: JSON output (for automation)
- `-verifyunified=N`: Continuous monitoring (always enabled)
- Tolerance-based verification accepts historical +3 anomaly
- Discrepancies > 5 blocks indicate NEW issues requiring action

**This toolset enables proactive blockchain health management for production operations.**
