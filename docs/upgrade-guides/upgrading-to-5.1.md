# Upgrading to LLL-TAO 5.1.0 (Stateless Mining)

## Overview

This guide covers upgrading existing Nexus nodes to LLL-TAO 5.1.0+ to enable the new stateless mining protocol with Falcon-1024 authentication and push notifications.

**Target Version:** LLL-TAO 5.1.0+  
**Protocol:** Stateless Mining 1.0  
**Release Date:** 2026-01-12

---

## What's New in 5.1.0

### Major Features

1. **Stateless Mining Protocol**
   - Push-based notifications (no polling)
   - <10ms template delivery latency
   - Channel-isolated updates (eliminates ~40% false staleness)
   - 216-byte compact templates

2. **Falcon-1024 Authentication**
   - Post-quantum security (NIST Level 5)
   - Dual signature system (Disposable + Physical)
   - Automatic version detection (Falcon-512 and Falcon-1024)
   - Key whitelisting support

3. **Performance Improvements**
   - Template generation: <1ms
   - Block validation: 20-30ms typical
   - Support for 100-500+ concurrent miners
   - Efficient session caching (24h default)

4. **Enhanced Security**
   - ChaCha20-Poly1305 encryption
   - Optional SSL/TLS support
   - DDoS protection
   - Miner key whitelisting

### Breaking Changes

**None for existing node operators.** Mining is disabled by default, so upgrading doesn't change node behavior unless you explicitly enable mining.

**Miner compatibility:** Stateless mining protocol is backward compatible with legacy GET_ROUND protocol. Miners auto-negotiate protocol version.

---

## Before You Upgrade

### Prerequisites

1. **Backup Data**
   ```bash
   # Stop node
   ./nexus stop
   
   # Backup data directory
   cp -r ~/.Nexus ~/.Nexus.backup-$(date +%Y%m%d)
   
   # Backup configuration
   cp ~/.Nexus/nexus.conf ~/.Nexus/nexus.conf.backup
   ```

2. **Check System Requirements**
   - **OS:** Linux (Ubuntu 20.04+ or CentOS 8+), macOS 10.15+, Windows 10+
   - **CPU:** 2+ cores (4+ recommended for mining)
   - **RAM:** 4 GB minimum (8 GB+ recommended for mining)
   - **Disk:** 50 GB+ free space (SSD recommended)

3. **Review Current Configuration**
   ```bash
   # Check if mining was enabled
   grep "mining" ~/.Nexus/nexus.conf
   
   # If mining=1, prepare for migration
   # If mining not set or =0, no action needed
   ```

---

## Upgrade Process

### Step 1: Download New Version

#### From Source (Recommended)

```bash
# Clone or update repository
cd ~/nexus-src
git fetch --all
git checkout v5.1.0  # Or latest release tag

# Or clone fresh:
git clone https://github.com/Nexusoft/LLL-TAO.git
cd LLL-TAO
git checkout v5.1.0
```

#### From Binary Release

```bash
# Download from GitHub releases
wget https://github.com/Nexusoft/LLL-TAO/releases/download/v5.1.0/nexus-5.1.0-linux-x64.tar.gz

# Extract
tar -xzf nexus-5.1.0-linux-x64.tar.gz
cd nexus-5.1.0
```

### Step 2: Build (if from source)

```bash
# Build with mining support
cd LLL-TAO
make -f makefile.cli clean
make -f makefile.cli -j$(nproc)

# Or with specific optimizations
make -f makefile.cli -j$(nproc) CXXFLAGS="-O3 -march=native"
```

**Build time:** 10-30 minutes depending on hardware

### Step 3: Stop Old Node

```bash
# Graceful shutdown
./nexus stop

# Wait for shutdown (check with)
ps aux | grep nexus

# Force stop if needed (last resort)
killall nexus
```

### Step 4: Install New Binary

```bash
# Backup old binary
mv /usr/local/bin/nexus /usr/local/bin/nexus.5.0.backup

# Install new binary
sudo cp nexus /usr/local/bin/nexus
sudo chmod +x /usr/local/bin/nexus

# Or if not using system install:
cp nexus ~/nexus-5.1
```

### Step 5: Update Configuration

If enabling mining for the first time:

```bash
# Edit nexus.conf
nano ~/.Nexus/nexus.conf

# Add mining configuration
mining=1
miningport=9325  # Default, can customize
miningthreads=4  # Adjust to your CPU
```

If mining was already enabled:

```bash
# No changes required!
# Existing mining config will work with stateless protocol
# Miners will auto-negotiate protocol version
```

Optional enhancements:

```bash
# Enable SSL for remote miners
miningssl=1
miningsslrequired=1
sslcert=/path/to/cert.crt
sslkey=/path/to/cert.key

# Enable DDoS protection
miningddos=1
miningcscore=1
miningrscore=50

# Whitelist specific miner keys (optional)
#minerallowkey=<hex_pubkey_1>
#minerallowkey=<hex_pubkey_2>
```

### Step 6: Start New Node

```bash
# Start node
./nexus

# Or if using systemd
sudo systemctl start nexus
```

### Step 7: Verify Upgrade

```bash
# Check version
./nexus --version
# Should show: Nexus Core version v5.1.0

# Check mining status
./nexus system/get/info | grep mining
# Should show: "mining": true (if enabled)

# Check logs
tail -f ~/.Nexus/debug.log
# Look for: "Mining LLP Server started on port 9325"
```

---

## Post-Upgrade Validation

### 1. Test Mining Connection

From miner machine:

```bash
# Test port connectivity
telnet <node_ip> 9325

# Or use nc
nc -zv <node_ip> 9325
```

### 2. Connect Test Miner

```bash
# On miner machine
nexusminer --host=<node_ip> --port=9325 --verbose=2

# Should see:
# [INFO] Authenticating with Falcon-1024...
# [INFO] Session established: <session_id>
# [INFO] Subscribed to Prime/Hash notifications
# [INFO] Received GET_BLOCK (216 bytes)
```

### 3. Monitor Performance

```bash
# Enable verbose logging temporarily
./nexus -verbose=3

# Watch for template delivery times
tail -f ~/.Nexus/debug.log | grep "GET_BLOCK\|NEW_BLOCK"

# Should see latencies < 10ms
```

### 4. Verify Push Notifications

Trigger a test by mining a block or waiting for blockchain advance:

```bash
# Watch logs on node
tail -f ~/.Nexus/debug.log | grep "NEW_BLOCK"

# Should see broadcast to all miners: "Broadcasted NEW_BLOCK to N miners in X ms"
```

---

## Troubleshooting Upgrade Issues

### "Mining server failed to start"

**Cause:** Port already in use or permission denied

**Solution:**
```bash
# Check if port is available
netstat -tuln | grep 9325

# Kill old process if needed
killall nexus

# Check firewall
sudo ufw allow 9325/tcp

# Restart
./nexus
```

### "No miners connecting after upgrade"

**Cause:** Configuration issue or network problem

**Solution:**
```bash
# Verify mining enabled
grep "mining" ~/.Nexus/nexus.conf

# Check logs
tail -100 ~/.Nexus/debug.log | grep -i mining

# Test locally
telnet localhost 9325
```

### "High memory usage after upgrade"

**Cause:** Default cache sizes may be larger

**Solution:**
```bash
# Adjust cache in nexus.conf
dbcache=256  # Reduce from default if needed

# Restart node
./nexus stop && ./nexus
```

### "Miners can't authenticate"

**Cause:** Miner needs Falcon keys

**Solution:**

On miner:
```bash
# Generate Falcon-1024 keys
nexusminer --generate-keys --falcon=1024

# Verify keys exist
ls ~/.nexusminer/*.fl  # Should show key files
```

---

## Migrating from Legacy Protocol

If you were running the legacy GET_ROUND mining protocol:

### Node Side (Automatic)

No action required! The node automatically supports both protocols. Miners will detect and use stateless protocol if available, fall back to legacy if needed.

### Miner Side

Update miners to latest NexusMiner version:

```bash
# On each miner machine
cd ~/nexusminer-src
git pull origin main
make clean && make

# Generate Falcon keys (one-time)
./nexusminer --generate-keys --falcon=1024

# Start mining (auto-negotiates protocol)
./nexusminer --host=<node_ip> --port=9325
```

**See:** [NexusMiner Upgrade Guide](https://github.com/Nexusoft/NexusMiner/blob/main/docs/upgrade-guides/legacy-to-stateless.md)

---

## Rolling Back (If Needed)

If you encounter critical issues:

```bash
# Stop new version
./nexus stop

# Restore old binary
sudo cp /usr/local/bin/nexus.5.0.backup /usr/local/bin/nexus

# Restore old configuration (if changed)
cp ~/.Nexus/nexus.conf.backup ~/.Nexus/nexus.conf

# Restore data (only if corrupted)
rm -rf ~/.Nexus
mv ~/.Nexus.backup-YYYYMMDD ~/.Nexus

# Start old version
./nexus
```

**Note:** Rolling back should rarely be necessary. Contact support if you need to roll back.

---

## Performance Optimization After Upgrade

### For Solo Miners

```ini
# In nexus.conf
miningthreads=2      # Minimal threads
dbcache=512          # Standard cache
```

### For Small Pools (<10 miners)

```ini
miningthreads=4
dbcache=1024
threads=4
```

### For Large Pools (100+ miners)

```ini
miningthreads=16
dbcache=4096
threads=8
maxconnections=32
```

**See:** [High-Performance Configuration](../reference/config-examples/high-performance-mining.conf)

---

## Configuration Migration

### Old Config (5.0.x)

```ini
# Old mining config (still works!)
mining=1
rpcuser=user
rpcpassword=pass
```

### New Config (5.1.0+)

```ini
# Enhanced with new features
mining=1
rpcuser=user
rpcpassword=pass

# New features (optional)
miningthreads=4       # Optimize for your CPU
miningssl=1           # Enable SSL
miningddos=1          # Enable DDoS protection
```

All old parameters still work. New parameters are optional enhancements.

---

## FAQ

### Q: Will my miners need updates?

**A:** For stateless protocol, yes. Update miners to NexusMiner 5.1.0+. But the node remains backward compatible with legacy miners.

### Q: Can I run both old and new miners simultaneously?

**A:** Yes! The node supports both protocols concurrently. Mixed deployment is supported during migration.

### Q: Do I need to reindex the blockchain?

**A:** No. The upgrade doesn't change blockchain format.

### Q: Will my mining rewards be lost during upgrade?

**A:** No. All pending rewards remain queued. Downtime only affects mining during the upgrade window.

### Q: How long does the upgrade take?

**A:** 10-30 minutes for building from source, 5-10 minutes for binary installation, plus your node's restart time (1-5 minutes).

---

## Getting Help

If you encounter issues during upgrade:

1. **Check logs:** `tail -100 ~/.Nexus/debug.log`
2. **Review troubleshooting:** [Mining Server Issues](../current/troubleshooting/mining-server-issues.md)
3. **Community support:**
   - Discord: https://nexus.io/discord
   - Forum: https://nexus.io/forum
4. **File bug report:** https://github.com/Nexusoft/LLL-TAO/issues

---

## Cross-References

**Related Documentation:**
- [Configuration Reference](../reference/nexus.conf.md)
- [Stateless Protocol](../current/mining/stateless-protocol.md)
- [Troubleshooting](../current/troubleshooting/mining-server-issues.md)
- [Migration from GET_ROUND](../archive/migration-guides/MIGRATION_GET_ROUND_TO_GETBLOCK.md)

**Miner Documentation:**
- [NexusMiner Upgrade Guide](https://github.com/Nexusoft/NexusMiner/blob/main/docs/upgrade-guides/legacy-to-stateless.md)
- [NexusMiner Setup](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/getting-started/setup.md)

---

## Version Information

**Document Version:** 1.0  
**Target LLL-TAO Version:** 5.1.0+  
**Last Updated:** 2026-01-13
