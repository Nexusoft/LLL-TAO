# RPC Mining Endpoints Documentation

## Overview

This document describes the RPC endpoints for mining pool management and discovery in the Nexus Core daemon.

**Base URL:** `http://localhost:8080/` (default API port)

## Authentication

Most mining endpoints do not require authentication. Endpoints that modify pool configuration may require appropriate credentials in a production environment.

## Endpoints

### 1. List Mining Pools

**Endpoint:** `mining/list/pools`

**Method:** `POST`

**Description:** Returns a list of available mining pools discovered on the network.

**Parameters:**

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| maxfee | uint8 | No | 5 | Maximum pool fee percentage (0-5) |
| minreputation | uint32 | No | 0 | Minimum reputation score (0-100) |
| onlineonly | boolean | No | true | Only return pools that are currently reachable |

**Request Example:**
```json
{
  "maxfee": 3,
  "minreputation": 80,
  "onlineonly": true
}
```

**Response Example:**
```json
[
  {
    "genesis": "8abc123def456789abc123def456789abc123def456789abc123def456789abc",
    "endpoint": "123.45.67.89:9549",
    "fee": 2,
    "currentminers": 150,
    "maxminers": 500,
    "hashrate": 1000000000,
    "blocksfound": 1234,
    "reputation": 98,
    "uptime": 0.995,
    "name": "Example Mining Pool",
    "lastseen": 1734529175,
    "online": true
  },
  {
    "genesis": "9def456abc789def456abc789def456abc789def456abc789def456abc789def",
    "endpoint": "98.76.54.32:9549",
    "fee": 1,
    "currentminers": 200,
    "maxminers": 1000,
    "hashrate": 2000000000,
    "blocksfound": 2456,
    "reputation": 95,
    "uptime": 0.98,
    "name": "Another Pool",
    "lastseen": 1734529180,
    "online": true
  }
]
```

**Response Fields:**

| Field | Type | Description |
|-------|------|-------------|
| genesis | string | Pool operator's genesis hash (identity) |
| endpoint | string | Pool's IP:PORT address |
| fee | integer | Pool fee percentage (0-5) |
| currentminers | integer | Number of currently connected miners |
| maxminers | integer | Maximum concurrent miners allowed |
| hashrate | uint64 | Total pool hashrate |
| blocksfound | integer | Lifetime blocks found by pool |
| reputation | integer | Calculated reputation score (0-100) |
| uptime | number | Uptime percentage (0.0-1.0) |
| name | string | Human-readable pool name |
| lastseen | uint64 | Unix timestamp of last announcement |
| online | boolean | Whether pool is currently reachable |

**Error Responses:**

| Code | Message | Description |
|------|---------|-------------|
| -1 | Invalid parameters | Parameter validation failed |

---

### 2. Get Pool Statistics

**Endpoint:** `mining/get/poolstats`

**Method:** `POST`

**Description:** Returns current pool statistics for this node (if running as a pool).

**Parameters:** None

**Request Example:**
```json
{}
```

**Response Example:**
```json
{
  "enabled": true,
  "genesis": "8abc123def456789abc123def456789abc123def456789abc123def456789abc",
  "endpoint": "123.45.67.89:9549",
  "fee": 2,
  "maxminers": 500,
  "connectedminers": 150,
  "authenticatedminers": 145,
  "rewardboundminers": 140,
  "totalhashrate": 0,
  "blocksfound": 0,
  "uptime": 1.0,
  "name": "My Nexus Pool"
}
```

**Response Fields:**

| Field | Type | Description |
|-------|------|-------------|
| enabled | boolean | Whether pool service is enabled |
| genesis | string | Pool operator's genesis hash |
| endpoint | string | Pool's public endpoint |
| fee | integer | Pool fee percentage |
| maxminers | integer | Maximum concurrent miners |
| connectedminers | integer | Currently connected miners |
| authenticatedminers | integer | Authenticated miners (passed Falcon auth) |
| rewardboundminers | integer | Miners with reward address set |
| totalhashrate | uint64 | Total pool hashrate (future) |
| blocksfound | integer | Blocks found by pool (future) |
| uptime | number | Pool uptime percentage |
| name | string | Pool name |

---

### 3. Start Pool Service

**Endpoint:** `mining/start/pool`

**Method:** `POST`

**Description:** Enable pool service and begin broadcasting announcements.

**Parameters:**

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| fee | uint8 | Yes | - | Pool fee percentage (0-5) |
| genesis | string | Yes | - | Pool operator's genesis hash |
| maxminers | uint16 | No | 500 | Maximum concurrent miners |
| name | string | No | "Nexus Mining Pool" | Human-readable pool name |

**Request Example:**
```json
{
  "fee": 2,
  "maxminers": 500,
  "name": "My Nexus Pool",
  "genesis": "8abc123def456789abc123def456789abc123def456789abc123def456789abc"
}
```

**Response Example:**
```json
{
  "success": true,
  "message": "Pool service started",
  "genesis": "8abc123def456789abc123def456789abc123def456789abc123def456789abc",
  "endpoint": "123.45.67.89:9549",
  "fee": 2,
  "maxminers": 500,
  "name": "My Nexus Pool"
}
```

**Error Responses:**

| Code | Message | Description |
|------|---------|-------------|
| -1 | Missing fee parameter | Required parameter not provided |
| -2 | Fee exceeds maximum: 5% | Fee is greater than 5% |
| -3 | Missing genesis parameter | Genesis hash not provided |
| -4 | Failed to broadcast pool announcement | Broadcast failed (check logs) |

---

### 4. Stop Pool Service

**Endpoint:** `mining/stop/pool`

**Method:** `POST`

**Description:** Disable pool service and stop broadcasting announcements.

**Parameters:** None

**Request Example:**
```json
{}
```

**Response Example:**
```json
{
  "success": true,
  "message": "Pool service stopped"
}
```

**Error Responses:**

| Code | Message | Description |
|------|---------|-------------|
| -1 | Pool service is not currently running | Pool was not enabled |

---

### 5. Get Connected Miners

**Endpoint:** `mining/get/connectedminers`

**Method:** `POST`

**Description:** Returns a list of currently connected miners (if running as a pool).

**Parameters:** None

**Request Example:**
```json
{}
```

**Response Example:**
```json
[
  {
    "genesis": "1abc456def789abc456def789abc456def789abc456def789abc456def789abc",
    "rewardaddress": "2def789abc123def789abc123def789abc123def789abc123def789abc123def",
    "authenticated": true,
    "rewardbound": true,
    "sessionid": 12345,
    "hashrate": 0,
    "sharessubmitted": 0,
    "sharesaccepted": 0,
    "blocksfound": 0,
    "connectedtime": 0,
    "lastactivity": 0
  },
  {
    "genesis": "3def789abc456def789abc456def789abc456def789abc456def789abc456def",
    "rewardaddress": "4abc123def456abc123def456abc123def456abc123def456abc123def456abc",
    "authenticated": true,
    "rewardbound": true,
    "sessionid": 12346,
    "hashrate": 0,
    "sharessubmitted": 0,
    "sharesaccepted": 0,
    "blocksfound": 0,
    "connectedtime": 0,
    "lastactivity": 0
  }
]
```

**Response Fields:**

| Field | Type | Description |
|-------|------|-------------|
| genesis | string | Miner's genesis hash (authentication) |
| rewardaddress | string | Miner's reward payout address |
| authenticated | boolean | Whether Falcon auth succeeded |
| rewardbound | boolean | Whether reward address has been set |
| sessionid | integer | Unique session identifier |
| hashrate | uint64 | Miner's current hashrate (future) |
| sharessubmitted | integer | Shares submitted (future) |
| sharesaccepted | integer | Shares accepted (future) |
| blocksfound | integer | Blocks found by miner (future) |
| connectedtime | integer | Connection duration in seconds (future) |
| lastactivity | uint64 | Unix timestamp of last activity (future) |

**Error Responses:**

| Code | Message | Description |
|------|---------|-------------|
| -1 | Pool service is not currently running | Pool is not enabled |

---

## Usage Examples

### Using curl

**List all pools with fee ≤ 2%:**
```bash
curl -X POST http://localhost:8080/mining/list/pools \
  -H "Content-Type: application/json" \
  -d '{"maxfee": 2, "minreputation": 0, "onlineonly": true}'
```

**Start a pool:**
```bash
curl -X POST http://localhost:8080/mining/start/pool \
  -H "Content-Type: application/json" \
  -d '{
    "fee": 2,
    "maxminers": 500,
    "name": "My Pool",
    "genesis": "YOUR_GENESIS_HASH"
  }'
```

**Get pool statistics:**
```bash
curl -X POST http://localhost:8080/mining/get/poolstats
```

**Stop the pool:**
```bash
curl -X POST http://localhost:8080/mining/stop/pool
```

### Using Python

```python
import requests
import json

# List pools
response = requests.post(
    'http://localhost:8080/mining/list/pools',
    json={'maxfee': 3, 'minreputation': 80, 'onlineonly': True}
)
pools = response.json()
print(json.dumps(pools, indent=2))

# Start pool
response = requests.post(
    'http://localhost:8080/mining/start/pool',
    json={
        'fee': 2,
        'maxminers': 500,
        'name': 'Python Pool',
        'genesis': 'YOUR_GENESIS_HASH'
    }
)
result = response.json()
print(result)
```

### Using JavaScript (Node.js)

```javascript
const axios = require('axios');

// List pools
async function listPools() {
  const response = await axios.post('http://localhost:8080/mining/list/pools', {
    maxfee: 3,
    minreputation: 80,
    onlineonly: true
  });
  console.log(JSON.stringify(response.data, null, 2));
}

// Start pool
async function startPool() {
  const response = await axios.post('http://localhost:8080/mining/start/pool', {
    fee: 2,
    maxminers: 500,
    name: 'JavaScript Pool',
    genesis: 'YOUR_GENESIS_HASH'
  });
  console.log(response.data);
}

listPools();
```

## Rate Limiting

Currently, no rate limiting is enforced on RPC endpoints. However, pool announcements are rate-limited to 1 per hour per genesis to prevent spam.

## CORS Support

The API supports Cross-Origin Resource Sharing (CORS) for web-based applications. Configure CORS settings in your daemon configuration if needed.

## WebSocket Support

WebSocket support for real-time pool statistics updates may be added in future versions.

## Versioning

Current API Version: **1.0**

Future versions will maintain backward compatibility where possible. Breaking changes will be announced and documented.

## Support

For questions or issues with the Mining API:

- **Documentation:** See [POOL_OPERATOR_GUIDE.md](POOL_OPERATOR_GUIDE.md)
- **Protocol:** See [POOL_DISCOVERY_PROTOCOL.md](POOL_DISCOVERY_PROTOCOL.md)
- **Issues:** Report on GitHub
- **Community:** Discord and forums
