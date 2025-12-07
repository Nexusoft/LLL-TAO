# Legacy Mining Configuration (REMOVED)

## Overview

This document provides historical reference for the legacy `miningpubkey` and `miningaddress` configuration options that were removed from `nexus.conf` in favor of the Stateless Miner architecture.

## What Was Removed

The following configuration options are **no longer supported** and should be removed from your `nexus.conf`:

- `miningpubkey=<pubkey>` - Legacy mining public key configuration
- `miningaddress=<address>` - Legacy mining address configuration

## Why They Were Removed

### Previous Architecture (Legacy)

In the legacy mining architecture, miners required credentials to be configured in **two places**:

1. **Node side** (`nexus.conf`):
   ```
   mining=1
   miningpubkey=<your_pubkey>
   ```

2. **Miner side** (NexusMiner config):
   ```
   {
       "miner_falcon_pubkey": "<pubkey>",
       "miner_falcon_privkey": "<privkey>"
   }
   ```

This created several problems:
- **Redundancy**: Same credentials configured in two places
- **Synchronization**: Had to keep both configs in sync
- **Security Risk**: Falcon keys potentially stored on node side
- **Confusion**: Users didn't understand why they needed to configure both

### New Architecture (Stateless Miner)

The Stateless Miner architecture implements **miner-driven authentication** where:

1. **Node side** (`nexus.conf`) - Just enable mining:
   ```
   mining=1
   ```

2. **Miner side** (NexusMiner `falconminer.conf`) - All credentials:
   ```json
   {
       "miner_falcon_pubkey": "<pubkey>",
       "miner_falcon_privkey": "<privkey>",
       "tritium_genesis": "<genesis_hash>"
   }
   ```

Benefits:
- **Single Source of Truth**: Miner credentials only in NexusMiner config
- **Cleaner Node Config**: Just `mining=1` needed
- **Better Security**: Falcon private keys never stored on node side
- **Simpler Setup**: One place to configure credentials

## Migration Guide

### Step 1: Remove Legacy Config

Edit your `nexus.conf` and **delete** these lines if present:
```
miningpubkey=...
miningaddress=...
```

### Step 2: Keep Mining Enabled

Ensure you still have:
```
mining=1
```

### Step 3: Configure NexusMiner

Ensure your NexusMiner `falconminer.conf` has all required credentials:
```json
{
    "miner_falcon_pubkey": "<your_falcon_public_key>",
    "miner_falcon_privkey": "<your_falcon_private_key>",
    "tritium_genesis": "<your_genesis_hash>"
}
```

### Step 4: Restart

1. Restart Nexus Core node
2. Restart NexusMiner

That's it! Mining will continue to work with the simplified configuration.

## Current Simplified Configuration

### Node Configuration (`nexus.conf`)

The **only** mining-related configuration needed on the node side:

```
# Enable mining server
mining=1

# Optional: Allow specific IPs to connect (defaults to 127.0.0.1)
llpallowip=127.0.0.1
```

### Miner Configuration (`falconminer.conf`)

All mining credentials are configured on the miner side:

```json
{
    "miner_falcon_pubkey": "<your_falcon_public_key>",
    "miner_falcon_privkey": "<your_falcon_private_key>",
    "tritium_genesis": "<your_tritium_genesis_hash>",
    "pool_url": "http://localhost:9336"
}
```

## Authentication Flow

The new Stateless Miner uses **Falcon post-quantum signatures** for authentication:

1. **MINER_AUTH_INIT**: Miner sends Falcon public key to node
2. **MINER_AUTH_CHALLENGE**: Node generates random challenge nonce
3. **MINER_AUTH_RESPONSE**: Miner signs challenge with Falcon private key
4. **MINER_AUTH_RESULT**: Node verifies signature and authorizes session

This happens automatically when NexusMiner connects to the node. No manual configuration required.

## Reward Routing

Mining rewards are automatically credited to the **default account** of the user associated with the Tritium genesis hash configured in NexusMiner.

No additional configuration needed - just ensure your `tritium_genesis` points to the correct user account.

## Troubleshooting

### Error: "Missing mining configuration"

This error indicates you're running an older version of Nexus Core that still requires legacy config options. Please upgrade to the latest version that supports Stateless Miner.

### Mining not working after removing miningpubkey

1. Verify `mining=1` is still in `nexus.conf`
2. Verify NexusMiner has correct Falcon keys configured
3. Check node logs for authentication messages
4. Ensure NexusMiner is connecting to correct node address

### How to generate Falcon keys for NexusMiner

Use the Nexus Core wallet or API to generate Falcon keys:

```bash
# Using the API (requires logged in session)
nexus -api=system/create/falcon
```

Or use the NexusMiner tools to generate a new key pair.

## Technical Details

For developers and advanced users:

- **Protocol**: LLP Miner Protocol with Falcon authentication packets
- **Security**: Post-quantum Falcon-512 signatures
- **Session Management**: Stateless per-connection authentication
- **Compatibility**: Fully backward compatible with existing blocks

## See Also

- [NexusMiner Documentation](../README.md)
- [Mining API Documentation](../API.md)
- [Falcon Signature Scheme](https://falcon-sign.info/)

---

**Document Version**: 1.0  
**Last Updated**: 2025-12-07  
**Related PR**: Clean Delete Legacy Mining Config
