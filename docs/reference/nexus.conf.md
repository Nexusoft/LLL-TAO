# Nexus Node Configuration Reference (nexus.conf)

## Table of Contents

1. [Configuration Formats](#configuration-formats)
2. [Mining Server Settings](#mining-server-settings)
3. [Network Configuration](#network-configuration)
4. [Authentication & Security](#authentication--security)
5. [Performance Tuning](#performance-tuning)
6. [Database Settings](#database-settings)
7. [Logging Configuration](#logging-configuration)
8. [API/RPC Settings](#apirpc-settings)
9. [Advanced Settings](#advanced-settings)
10. [Examples](#examples)

---

## Configuration Formats

### File Location

**Default Paths:**
- Linux: `~/.Nexus/nexus.conf`
- macOS: `~/Library/Application Support/Nexus/nexus.conf`
- Windows: `%APPDATA%\Nexus\nexus.conf`

### Syntax

```ini
# Comment line
parameter=value
flag=1  # Enable boolean flag
flag=0  # Disable boolean flag
```

### Multiple Values

Some parameters accept multiple values by repeating the parameter:

```ini
llpallowip=192.168.1.0/24
llpallowip=10.0.0.0/8
minerallowkey=0123456789abcdef...
minerallowkey=fedcba9876543210...
```

---

## Mining Server Settings

### `mining`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Enable the mining server to allow miners to connect

The mining server implements the stateless mining protocol (LLL-TAO 5.1.0+) with push notifications, Falcon-1024 authentication, and automatic session management.

**Example:**
```ini
# Enable mining server
mining=1
```

**Cross-Reference:**
- [NexusMiner Configuration](https://github.com/Nexusoft/NexusMiner/blob/main/docs/reference/nexus.conf.md)
- [Stateless Protocol Documentation](../current/mining/stateless-protocol.md)

---

### `miningport`

**Type:** Integer  
**Default:** `9325` (mainnet), `8323` (testnet)  
**Description:** Port for the stateless mining protocol (LLP Mining Server)

This is the port where miners connect to receive block templates and submit solutions. The protocol uses push notifications (GET_BLOCK/NEW_BLOCK) rather than polling.

**Example:**
```ini
# Custom mining port
miningport=9325
```

**Related Ports:**
- Mainnet mining: `9325`
- Testnet mining: `8323`

**Security Note:** This port should be accessible to miners but protected by firewalls from untrusted sources. Consider using `llpallowip` to whitelist specific networks.

---

### `minerallowkey`

**Type:** String (Hexadecimal)  
**Default:** None (all keys allowed)  
**Description:** Whitelist specific miner Falcon public keys for enhanced security

When set, only miners with the specified Falcon public keys can authenticate to the mining server. This is useful for private pools or restricted mining operations.

**Format:** 
- Falcon-512: 897 bytes (1794 hex characters)
- Falcon-1024: 1793 bytes (3586 hex characters)

**Example:**
```ini
# Whitelist specific miners (optional)
minerallowkey=0123456789abcdef0123456789abcdef0123456789abcdef...
minerallowkey=fedcba9876543210fedcba9876543210fedcba9876543210...
```

**Use Cases:**
- Private mining pools
- Corporate mining operations
- Restricted testnet environments

**Cross-Reference:**
- [Key Whitelisting Guide](../current/security/key-whitelisting.md)
- [Authentication Documentation](../current/authentication/miner-authentication.md)

---

### `miningthreads`

**Type:** Integer  
**Default:** Number of CPU cores  
**Description:** Number of threads for block template generation and validation

Controls the parallelism of the mining server's template generation and block validation operations. Higher values can improve throughput for pools with many connected miners.

**Example:**
```ini
# Use 8 threads for template generation
miningthreads=8
```

**Recommendations:**
- Solo mining: 2-4 threads sufficient
- Small pool (<10 miners): 4-8 threads
- Large pool (>10 miners): 8-16 threads
- Do not exceed physical CPU core count

**Performance Impact:**
- Template generation: <1ms per template
- Block validation: 20-30ms per submission
- Higher thread count = lower latency for concurrent operations

---

### `miningssl`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Enable SSL/TLS encryption for mining connections

**Example:**
```ini
# Enable SSL for mining
miningssl=1
```

**Related Settings:**
- `miningsslrequired`: Force all connections to use SSL
- See [SSL Configuration](#ssl-configuration) for certificate setup

---

### `miningsslrequired`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Require all mining connections to use SSL/TLS

When enabled, non-SSL mining connections will be rejected.

**Example:**
```ini
# Require SSL for all miners
miningssl=1
miningsslrequired=1
```

**Security Considerations:**
- Recommended for public pools
- Not required for localhost mining
- Falcon authentication provides additional session security

---

### `miningddos`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Enable DDOS protection for mining server

Enables rate limiting and connection scoring to protect against denial-of-service attacks.

**Example:**
```ini
# Enable DDOS protection
miningddos=1
miningcscore=1
miningrscore=50
miningtimespan=60
```

**Related Settings:**
- `miningcscore`: Connection score threshold
- `miningrscore`: Request score threshold
- `miningtimespan`: Time window for scoring (seconds)

**Cross-Reference:**
- [DDOS Protection Guide](../current/security/ddos-protection.md)

---

### `miningcscore`

**Type:** Integer  
**Default:** `1`  
**Description:** Connection score threshold for DDOS protection

Connections exceeding this score within the timespan will be temporarily banned.

**Example:**
```ini
miningcscore=1
```

---

### `miningrscore`

**Type:** Integer  
**Default:** `50`  
**Description:** Request score threshold for DDOS protection

Requests exceeding this score within the timespan will be rate-limited.

**Example:**
```ini
miningrscore=50
```

---

### `miningtimespan`

**Type:** Integer  
**Default:** `60`  
**Description:** Time window (in seconds) for DDOS scoring

**Example:**
```ini
miningtimespan=60
```

---

### `miningtimeout`

**Type:** Integer  
**Default:** `30`  
**Description:** Socket timeout (in seconds) for mining connections

Idle mining connections will be closed after this duration.

**Example:**
```ini
miningtimeout=30
```

**Recommendations:**
- Increase for miners with poor connectivity
- Decrease to free up resources faster

---

## Network Configuration

### `testnet`

**Type:** Integer  
**Default:** `0`  
**Description:** Run on testnet instead of mainnet

**Values:**
- `0`: Mainnet
- `1`: Primary testnet
- `2`: Secondary testnet (port + 1)
- `3`: Tertiary testnet (port + 2)

**Example:**
```ini
testnet=1
```

**Port Changes:**
- Mining: 9325 → 8323
- API: 8080 → 7080
- RPC: 9336 → 8336
- P2P: 9326 → 8326

---

### `port`

**Type:** Integer  
**Default:** `9888` (mainnet), `8888` (testnet)  
**Description:** Main P2P network port for Tritium protocol

**Example:**
```ini
port=9888
```

---

### `p2pport`

**Type:** Integer  
**Default:** `9326` (mainnet), `8326` (testnet)  
**Description:** Legacy P2P network port

**Example:**
```ini
p2pport=9326
```

---

### `llpallowip`

**Type:** String (IP address or CIDR notation)  
**Default:** `127.0.0.1` (automatically added for mining)  
**Description:** Allow LLP connections from specific IP addresses or networks

Can be specified multiple times to whitelist multiple addresses/networks.

**Example:**
```ini
# Allow local connections
llpallowip=127.0.0.1

# Allow specific subnet
llpallowip=192.168.1.0/24

# Allow specific IP
llpallowip=10.0.0.50
```

**Security:** Always restrict to trusted networks for mining operations.

---

### `maxconnections`

**Type:** Integer  
**Default:** `16`  
**Description:** Maximum number of P2P connections

**Example:**
```ini
maxconnections=32
```

---

### `maxincoming`

**Type:** Integer  
**Default:** `84`  
**Description:** Maximum number of incoming P2P connections

**Example:**
```ini
maxincoming=100
```

---

### `maxoutgoing`

**Type:** Integer  
**Default:** `8`  
**Description:** Maximum number of outgoing P2P connections

**Example:**
```ini
maxoutgoing=16
```

---

## Authentication & Security

### `rpcuser`

**Type:** String  
**Default:** None  
**Description:** Username for RPC authentication

**Required:** Yes (if using RPC)

**Example:**
```ini
rpcuser=nexususer
```

---

### `rpcpassword`

**Type:** String  
**Default:** None  
**Description:** Password for RPC authentication

**Required:** Yes (if using RPC)

**Example:**
```ini
rpcpassword=secure_password_here
```

**Security:** Use strong, unique passwords. Consider using environment variables or secure configuration management.

---

### `apiuser`

**Type:** String  
**Default:** None  
**Description:** Username for API authentication

**Required:** Yes (if using API with authentication)

**Example:**
```ini
apiuser=nexusapiuser
```

---

### `apipassword`

**Type:** String  
**Default:** None  
**Description:** Password for API authentication

**Required:** Yes (if using API with authentication)

**Example:**
```ini
apipassword=secure_api_password_here
```

---

### `apiauth`

**Type:** Integer (0 or 1)  
**Default:** `1`  
**Description:** Enable API authentication

When disabled, API endpoints are accessible without credentials (not recommended for public nodes).

**Example:**
```ini
# Disable API auth (not recommended)
apiauth=0
```

---

### `ssl`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Enable SSL/TLS for various services

**Example:**
```ini
ssl=1
```

---

### `sslrequired`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Require SSL/TLS for all connections

**Example:**
```ini
ssl=1
sslrequired=1
```

---

### `sslcert`

**Type:** String (file path)  
**Default:** None  
**Description:** Path to SSL certificate file

**Example:**
```ini
sslcert=/path/to/server.crt
```

**Certificate Requirements:**
- PEM format
- Valid X.509 certificate
- Matching private key

---

### `sslkey`

**Type:** String (file path)  
**Default:** None  
**Description:** Path to SSL private key file

**Example:**
```ini
sslkey=/path/to/server.key
```

**Security:** Protect private key file with appropriate permissions (chmod 600).

---

## Performance Tuning

### `dbcache`

**Type:** Integer (MB)  
**Default:** `256`  
**Description:** Database cache size in megabytes

Larger cache = better performance, higher memory usage.

**Example:**
```ini
# Use 512 MB cache
dbcache=512
```

**Recommendations:**
- Minimum: 128 MB
- Standard: 256-512 MB
- High-performance: 1024-2048 MB
- Mining pool: 2048+ MB

---

### `threads`

**Type:** Integer  
**Default:** Number of CPU cores  
**Description:** Number of threads for general node operations

**Example:**
```ini
threads=8
```

---

### `mempool.max`

**Type:** Integer  
**Default:** `100000`  
**Description:** Maximum number of transactions in mempool

**Example:**
```ini
mempool.max=200000
```

---

## Database Settings

### `datadir`

**Type:** String (directory path)  
**Default:** Platform-specific (see [Configuration Formats](#configuration-formats))  
**Description:** Directory for blockchain and database files

**Example:**
```ini
datadir=/var/nexus/data
```

---

### `reindex`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Rebuild blockchain index on startup

**Example:**
```ini
# Reindex blockchain
reindex=1
```

**Warning:** This is a one-time operation that can take several hours.

---

### `rescan`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Rescan blockchain for wallet transactions

**Example:**
```ini
rescan=1
```

---

## Logging Configuration

### `verbose`

**Type:** Integer (0-5)  
**Default:** `0`  
**Description:** Logging verbosity level

**Levels:**
- `0`: Minimal logging (errors only)
- `1`: Basic logging (warnings and errors)
- `2`: Standard logging (info, warnings, errors)
- `3`: Verbose logging (debug information)
- `4`: Very verbose (detailed debug)
- `5`: Maximum verbosity (trace level)

**Example:**
```ini
# Enable verbose logging
verbose=3
```

---

### `log`

**Type:** Integer (0 or 1)  
**Default:** `1`  
**Description:** Enable logging to file

**Example:**
```ini
log=1
```

---

### `logfile`

**Type:** String (file path)  
**Default:** `debug.log` (in data directory)  
**Description:** Path to log file

**Example:**
```ini
logfile=/var/log/nexus/debug.log
```

---

### `logtimestamps`

**Type:** Integer (0 or 1)  
**Default:** `1`  
**Description:** Include timestamps in log entries

**Example:**
```ini
logtimestamps=1
```

---

## API/RPC Settings

### `server`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Enable RPC server

**Example:**
```ini
server=1
```

---

### `rpcport`

**Type:** Integer  
**Default:** `9336` (mainnet), `8336` (testnet)  
**Description:** RPC server port

**Example:**
```ini
rpcport=9336
```

---

### `rpcthreads`

**Type:** Integer  
**Default:** `4`  
**Description:** Number of threads for RPC request handling

**Example:**
```ini
rpcthreads=8
```

---

### `api`

**Type:** Integer (0 or 1)  
**Default:** `1`  
**Description:** Enable API server

**Example:**
```ini
api=1
```

---

### `apiport`

**Type:** Integer  
**Default:** `8080` (mainnet), `7080` (testnet)  
**Description:** API server port

**Example:**
```ini
apiport=8080
```

---

### `apisslport`

**Type:** Integer  
**Default:** `8443` (mainnet), `7443` (testnet)  
**Description:** API SSL port

**Example:**
```ini
apisslport=8443
```

---

### `apithreads`

**Type:** Integer  
**Default:** `4`  
**Description:** Number of threads for API request handling

**Example:**
```ini
apithreads=8
```

---

### `apiremote`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Allow remote API connections

When disabled, API only accepts localhost connections.

**Example:**
```ini
# Allow remote API access
apiremote=1
```

**Security Warning:** Only enable for trusted networks. Use with `apiauth=1` and strong credentials.

---

### `apiddos`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Enable DDOS protection for API

**Example:**
```ini
apiddos=1
apicscore=1
apirscore=100
apitimespan=60
```

---

### `apicscore`

**Type:** Integer  
**Default:** `1`  
**Description:** Connection score threshold for API DDOS protection

**Example:**
```ini
apicscore=1
```

---

### `apirscore`

**Type:** Integer  
**Default:** `100`  
**Description:** Request score threshold for API DDOS protection

**Example:**
```ini
apirscore=100
```

---

### `apitimespan`

**Type:** Integer  
**Default:** `60`  
**Description:** Time window (seconds) for API DDOS scoring

**Example:**
```ini
apitimespan=60
```

---

### `apitimeout`

**Type:** Integer  
**Default:** `30`  
**Description:** API connection timeout (seconds)

**Example:**
```ini
apitimeout=60
```

---

## Advanced Settings

### `daemon`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Run as a background daemon

**Example:**
```ini
daemon=1
```

---

### `stake`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Enable staking on this node

**Example:**
```ini
stake=1
```

**Note:** Staking requires a funded Tritium account and unlocked wallet.

---

### `client`

**Type:** Integer (0 or 1)  
**Default:** `0`  
**Description:** Run as a lite client (no mining, limited features)

**Example:**
```ini
client=1
```

---

### `llpsleep`

**Type:** Integer  
**Default:** `0`  
**Description:** Sleep time (ms) between LLP data reads

Can reduce CPU usage at the cost of slightly higher latency.

**Example:**
```ini
llpsleep=1
```

---

### `llpwait`

**Type:** Integer  
**Default:** `1`  
**Description:** Wait time (ms) for LLP data availability

**Example:**
```ini
llpwait=1
```

---

## Examples

### Example 1: Private Solo Mining Node (Localhost)

```ini
#
# Private solo mining configuration
# Node and miner on same machine
#

# Basic authentication
rpcuser=miner
rpcpassword=secure_password_123
apiuser=miner
apipassword=secure_api_password_456
daemon=1

# Enable mining server
mining=1
miningport=9325

# Optional: Extended logging for debugging
verbose=2

# Performance tuning
dbcache=512
miningthreads=4
```

---

### Example 2: Public Mining Pool (Internet-Accessible)

```ini
#
# Public mining pool configuration
# Accessible over the internet
#

# Authentication
rpcuser=pool_operator
rpcpassword=ultra_secure_password_xyz
apiuser=pool_api
apipassword=ultra_secure_api_password_abc
daemon=1

# Mining server with security
mining=1
miningport=9325
miningssl=1
miningsslrequired=1

# SSL certificates
sslcert=/etc/ssl/certs/nexus-pool.crt
sslkey=/etc/ssl/private/nexus-pool.key

# DDOS protection
miningddos=1
miningcscore=1
miningrscore=50
miningtimespan=60
miningtimeout=30

# Network restrictions
llpallowip=0.0.0.0/0  # Allow all (use firewall for restrictions)

# Performance for many miners
dbcache=2048
miningthreads=16
threads=8

# API access
api=1
apiremote=1
apissl=1
apiddos=1
apicscore=1
apirscore=100

# Logging
verbose=2
log=1
```

---

### Example 3: Testnet Mining Node

```ini
#
# Testnet mining configuration
# For testing new features
#

# Testnet mode
testnet=1

# Authentication
rpcuser=testuser
rpcpassword=testpass
apiuser=testapi
apipassword=testapipass
daemon=1

# Mining on testnet
mining=1
# Port automatically becomes 8323 on testnet

# Verbose logging for testing
verbose=3
log=1

# Standard cache
dbcache=256
```

---

### Example 4: High-Performance Mining Pool with Whitelisting

```ini
#
# High-performance pool with miner key whitelisting
# Maximum security and performance
#

# Authentication
rpcuser=pool_admin
rpcpassword=complex_password_here
apiuser=pool_api
apipassword=complex_api_password
daemon=1

# Mining with SSL
mining=1
miningport=9325
miningssl=1
miningsslrequired=1

# Whitelist specific miner keys (Falcon-1024)
# Note: Real keys are 3586 hex characters. Export from miner: nexusminer --export-pubkey
minerallowkey=a1b2c3d4e5f6...[3586_hex_chars]...f6e5d4c3b2a1
minerallowkey=f6e5d4c3b2a1...[3586_hex_chars]...a1b2c3d4e5f6

# SSL configuration
sslcert=/etc/ssl/certs/nexus-hiperf-pool.crt
sslkey=/etc/ssl/private/nexus-hiperf-pool.key

# DDOS protection
miningddos=1
miningcscore=1
miningrscore=50
miningtimespan=60

# Maximum performance
dbcache=4096
miningthreads=32
threads=16
mempool.max=200000

# Network
llpallowip=10.0.0.0/8
llpallowip=192.168.0.0/16

# API with protection
api=1
apiremote=1
apissl=1
apisslrequired=1
apiddos=1
apicscore=1
apirscore=100
apithreads=8

# Verbose logging
verbose=2
log=1
```

---

## Cross-References

**Miner Perspective:**
- [NexusMiner Configuration Reference](https://github.com/Nexusoft/NexusMiner/blob/main/docs/reference/nexus.conf.md)
- [NexusMiner Setup Guide](https://github.com/Nexusoft/NexusMiner/blob/main/docs/current/getting-started/setup.md)

**Node Documentation:**
- [Stateless Mining Protocol](../current/mining/stateless-protocol.md)
- [Mining Server Architecture](../current/mining/mining-server.md)
- [Authentication Guide](../current/authentication/miner-authentication.md)
- [Security Best Practices](../current/security/network-security.md)
- [Troubleshooting Guide](../current/troubleshooting/mining-server-issues.md)

**Configuration Examples:**
- [Mining Node Config](config-examples/mining-node.conf)
- [Mining Node Testnet Config](config-examples/mining-node-testnet.conf)
- [Whitelisted Mining Config](config-examples/mining-node-whitelisted.conf)
- [High-Performance Config](config-examples/high-performance-mining.conf)

---

## Version Information

**Document Version:** 1.0  
**LLL-TAO Version:** 5.1.0+  
**Protocol:** Stateless Mining (PR #170)  
**Last Updated:** 2026-01-13

---

*This reference covers all major configuration parameters for the Nexus node. For additional parameters and advanced configurations, consult the source code or community documentation.*
