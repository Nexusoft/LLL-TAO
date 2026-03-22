# Node Mining Server Troubleshooting

## Overview

This guide helps diagnose and resolve common issues with the Nexus node mining server. It covers startup problems, connection issues, performance problems, and block validation failures.

---

## "Mining server not starting"

### Symptoms

```
[Mining] Failed to start mining LLP server
[Error] Port 9323 already in use
[Error] Unable to bind to port
```

### Solutions

#### 1. Check Port Availability

```bash
# Check if port is in use
netstat -tuln | grep 9323
lsof -i :9323

# Check for zombie processes
ps aux | grep nexus
```

**If port is in use:**
```bash
# Kill existing process
killall nexus

# Or kill specific PID
kill <PID>
```

#### 2. Verify Configuration

Check `nexus.conf`:

```ini
# Must be enabled
mining=1

# Verify port (optional, defaults to 9323 mainnet / 8323 testnet)
miningport=9323
```

#### 3. Check Firewall

```bash
# Ubuntu/Debian
sudo ufw status
sudo ufw allow 9323/tcp

# CentOS/RHEL
sudo firewall-cmd --list-all
sudo firewall-cmd --permanent --add-port=9323/tcp
sudo firewall-cmd --reload

# Check iptables
sudo iptables -L -n | grep 9323
```

#### 4. Verify No Other Mining Servers Running

```bash
# Check for multiple nexus instances
ps aux | grep nexus

# Check listening ports
sudo netstat -tulpn | grep nexus
```

#### 5. Check Permissions

Ensure user has permission to bind to the port:

```bash
# Low ports (<1024) need root or CAP_NET_BIND_SERVICE
# Standard mining ports (9323, 8323) should work without root

# If using low port, grant capability
sudo setcap 'cap_net_bind_service=+ep' /path/to/nexus
```

#### 6. Check Log Files

```bash
# View recent logs
tail -f ~/.Nexus/debug.log | grep Mining

# Search for error messages
grep -i "error\|failed" ~/.Nexus/debug.log | grep -i mining
```

---

## "No miners connecting"

### Symptoms

- Mining server started successfully
- No incoming connections
- Miners report "Connection refused" or timeout

### Solutions

#### 1. Check Node Logs

```bash
# Monitor for connection attempts
tail -f ~/.Nexus/debug.log | grep Mining

# Look for miner connections
grep "Miner connected" ~/.Nexus/debug.log
```

#### 2. Verify Mining Enabled

```bash
# Check via RPC
./nexus system/get/info | grep mining

# Should show: "mining": true
```

Or check `nexus.conf`:

```ini
mining=1  # Must be uncommented and set to 1
```

#### 3. Test Port Connectivity

From the miner machine:

```bash
# Test TCP connection
telnet <node_ip> 9323

# Or use nc (netcat)
nc -zv <node_ip> 9323

# Or use nmap
nmap -p 9323 <node_ip>
```

**If connection refused:**
- Check firewall on node
- Check network firewall/router
- Verify correct IP address
- Confirm port forwarding (if behind NAT)

#### 4. Check Network Configuration

**For localhost mining:**
```ini
# In nexus.conf
# Default allows localhost automatically
```

**For remote mining:**
```ini
# Allow specific IP/network
llpallowip=192.168.1.0/24

# Or allow all (use with caution)
llpallowip=0.0.0.0/0
```

#### 5. Verify SSL Configuration

If using SSL:

```ini
miningssl=1
miningsslrequired=1
sslcert=/path/to/server.crt
sslkey=/path/to/server.key
```

**Check certificate:**
```bash
# Verify certificate exists and is valid
openssl x509 -in /path/to/server.crt -text -noout

# Check private key
openssl rsa -in /path/to/server.key -check
```

**Common SSL issues:**
- Certificate expired
- Certificate/key mismatch
- Wrong file permissions (should be readable by nexus user)
- Self-signed certificate not trusted by miner

#### 6. Check DDoS Protection

If DDoS protection is too aggressive:

```ini
# Relax DDoS settings
miningddos=1
miningcscore=10        # Increase from default 1
miningrscore=500       # Increase from default 50
miningtimespan=120     # Increase from default 60
```

#### 7. Verify Miner Configuration

On miner side:

```bash
# Check miner command
nexusminer --host=<node_ip> --port=9323

# Enable verbose logging
nexusminer --host=<node_ip> --port=9323 --verbose=3
```

---

## "High block rejection rate"

### Symptoms

```
[Mining] Block rejected from miner 12345678 (reason: Invalid proof-of-work)
[Mining] Block rejected from miner 87654321 (reason: Stale template)
```

High percentage of submitted blocks are rejected.

### Solutions

#### 1. Analyze Rejection Reasons

```bash
# Count rejection reasons
grep "Block rejected" ~/.Nexus/debug.log | \
  sed 's/.*reason: \([^)]*\).*/\1/' | \
  sort | uniq -c | sort -rn
```

**Common reasons and solutions:**

**"Invalid proof-of-work":**
- Miner software bug
- Incorrect difficulty target
- Hardware malfunction
- Update miner software

**"Stale template":**
- Network latency too high
- Miner taking too long to find solution
- Template staleness timeout too short
- Consider mining on lower difficulty channel

**"Invalid block structure":**
- Miner software incompatibility
- Corrupted packet transmission
- Update miner to latest version

**"Invalid Falcon signature":**
- Miner using wrong Falcon keys
- Signature verification failure
- ChaCha20 encryption issue
- Regenerate miner Falcon keys

#### 2. Check Network Latency

High latency causes stale blocks:

```bash
# From miner machine
ping <node_ip>

# Should be < 100ms for best results
# > 500ms likely to cause frequent stales
```

**Solutions for high latency:**
- Use geographically closer node
- Improve network connection
- Use wired connection instead of WiFi

#### 3. Monitor Template Age

```bash
# Enable verbose logging
verbose=3
```

Look for template age in logs:
```
[Mining] NEW_BLOCK pushed (age: 0.2s)
[Mining] Block submitted (template_age: 45s)
```

**If template age > 50s at submission:**
- Miner is too slow (hardware insufficient)
- Network latency too high
- Consider Prime channel (slower difficulty)

#### 4. Verify Miner Hardware

**For Hash channel:**
- Requires GPU or FPGA
- CPU mining Hash is ineffective
- Check GPU driver and configuration

**For Prime channel:**
- CPU mining is appropriate
- More cores = better performance
- Check CPU isn't thermal throttling

#### 5. Check Blockchain Synchronization

```bash
# Verify node is fully synced
./nexus system/get/info

# Check height vs. explorer
# Should match https://nexus.io/explorer
```

**If not synced:**
```bash
# Node will generate templates on wrong chain
# Wait for full synchronization before mining
```

#### 6. Validate Falcon Keys

On miner:

```bash
# Verify Falcon keys are valid
nexusminer --check-keys

# Regenerate if corrupted
nexusminer --generate-keys --falcon=1024
```

---

## "Miners disconnecting frequently"

### Symptoms

```
[Mining] Miner disconnected: session timeout
[Mining] Miner disconnected: socket error
[Mining] Miner disconnected: connection reset
```

### Solutions

#### 1. Check Session Timeout

Default session timeout is 24 hours. If miners disconnect before:

```bash
# Review disconnect logs
grep "Miner disconnected" ~/.Nexus/debug.log | \
  grep -o "session [0-9]*" | \
  sort | uniq -c
```

**If frequent timeouts:**
```ini
# Increase socket timeout
miningtimeout=60  # Default is 30 seconds
```

#### 2. Verify Network Stability

```bash
# Check for packet loss
ping -c 100 <node_ip>

# Monitor connection quality
mtr <node_ip>
```

**Solutions:**
- Use wired connection
- Fix network issues
- Use better quality network provider

#### 3. Enable Keepalives

On miner:

```bash
# Configure keepalive interval
nexusminer --keepalive=300  # 5 minutes
```

#### 4. Check Node Resource Exhaustion

```bash
# Monitor node resources
htop

# Check memory usage
free -h

# Check CPU usage
mpstat 1
```

**If resources exhausted:**
```ini
# Reduce concurrent miners or increase resources
# Consider limiting connections
miningthreads=8  # Reduce if CPU bound
```

#### 5. Review Socket Errors

```bash
# Check system socket limits
ulimit -n

# Increase if needed
ulimit -n 4096

# Make permanent in /etc/security/limits.conf
<user> soft nofile 4096
<user> hard nofile 4096
```

---

## "Slow template generation"

### Symptoms

```
[Mining] Slow template generation: 2500μs
[Mining] NEW_BLOCK broadcast took 15ms (target <10ms)
```

### Solutions

#### 1. Check Database Performance

```bash
# Monitor I/O wait
iostat -x 1

# Check disk performance
sudo hdparm -Tt /dev/sda
```

**Solutions:**
```ini
# Increase database cache
dbcache=1024  # Default is 256 MB

# Use SSD instead of HDD
# Consider RAID for better performance
```

#### 2. Optimize Thread Count

```ini
# Match physical CPU cores
miningthreads=8  # Adjust to your CPU

# Don't exceed physical cores
# Hyper-threading usually doesn't help
```

#### 3. Check CPU Frequency Scaling

```bash
# Check CPU governor
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Set to performance mode
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

#### 4. Monitor System Load

```bash
# Check load average
uptime

# Should be < number of CPU cores
# If higher, system is overloaded
```

#### 5. Profile Template Generation

Enable profiling:

```ini
verbose=4  # Detailed timing information
```

Review logs for bottlenecks:
```bash
grep "template generation" ~/.Nexus/debug.log | \
  awk '{print $NF}' | sort -n | tail -20
```

---

## "Memory usage growing"

### Symptoms

- Node memory usage increases over time
- Eventually causes OOM (out of memory) kills
- System becomes unresponsive

### Solutions

#### 1. Check Session Cache

```bash
# Monitor session count
grep "Session" ~/.Nexus/debug.log | \
  grep -o "count=[0-9]*" | \
  tail -1
```

**If growing excessively:**
- Miners not disconnecting properly
- Session cleanup not working
- Restart node to clear cache

#### 2. Optimize Database Cache

```ini
# Reduce if memory limited
dbcache=256  # Default, reduce if needed
```

#### 3. Monitor Mempool

```bash
# Check mempool size
./nexus system/get/info | grep mempool
```

**If too large:**
```ini
# Limit mempool size
mempool.max=50000  # Default is 100000
```

#### 4. Check for Memory Leaks

```bash
# Monitor with valgrind (development builds only)
valgrind --leak-check=full ./nexus -mining=1
```

#### 5. Restart Periodically

Temporary workaround:

```bash
# Create systemd service with restart policy
# Or use cron to restart daily
0 4 * * * systemctl restart nexus
```

---

## "SSL/TLS errors"

### Symptoms

```
[Mining] SSL handshake failed
[Mining] Certificate verification failed
[Mining] SSL: certificate has expired
```

### Solutions

#### 1. Verify Certificate Validity

```bash
# Check certificate
openssl x509 -in /path/to/server.crt -text -noout

# Check expiration
openssl x509 -in /path/to/server.crt -noout -dates
```

#### 2. Regenerate Certificate

```bash
# Generate new self-signed certificate (valid 365 days)
openssl req -x509 -newkey rsa:4096 \
  -keyout server.key \
  -out server.crt \
  -days 365 \
  -nodes \
  -subj "/CN=nexus-mining-pool"
```

#### 3. Check File Permissions

```bash
# Certificate should be readable
chmod 644 /path/to/server.crt

# Key should be readable only by nexus user
chmod 600 /path/to/server.key
chown nexus:nexus /path/to/server.key
```

#### 4. Verify Certificate/Key Match

```bash
# Check certificate modulus
openssl x509 -noout -modulus -in server.crt | openssl md5

# Check key modulus
openssl rsa -noout -modulus -in server.key | openssl md5

# Modulus hashes should match
```

---

## Getting Help

If problems persist after trying these solutions:

### 1. Enable Verbose Logging

```ini
# In nexus.conf
verbose=4
log=1
```

### 2. Collect Diagnostic Information

```bash
# Node version
./nexus --version

# System information
uname -a
cat /etc/os-release

# Configuration (sanitize passwords!)
grep -v password ~/.Nexus/nexus.conf

# Recent logs (last 100 lines)
tail -100 ~/.Nexus/debug.log

# Network connectivity
netstat -tuln | grep 9323
ss -tuln | grep 9323
```

### 3. Check Community Resources

- **Discord:** https://nexus.io/discord
- **Forum:** https://nexus.io/forum
- **GitHub Issues:** https://github.com/Nexusoft/LLL-TAO/issues

### 4. Submit Bug Report

Include:
- Node version
- Operating system
- Configuration (sanitized)
- Relevant log excerpts
- Steps to reproduce

---

## Cross-References

**Related Documentation:**
- [Mining Server Architecture](../mining/mining-server.md)
- [Stateless Protocol](../mining/stateless-protocol.md)
- [Configuration Reference](../../reference/nexus.conf.md)
- [NexusMiner Troubleshooting](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/troubleshooting/connection-issues.md)

---

## Version Information

**Document Version:** 1.0  
**LLL-TAO Version:** 5.1.0+  
**Last Updated:** 2026-01-13
