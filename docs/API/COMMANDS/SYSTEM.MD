# System API
-----------------------------------

The System API provides public access to information about this node. This includes data such as the version of software the node is running, ledger and mempool state, node IP address, and number of connected peers.

-----------------------------------
## `Direct Endpoints`

The following commands are direct endpoints and thus do not support the above verb and noun structure available above.

[`stop`](#stop)  
[`get/info`](#get/info)  
[`get/metrics`](#get/metrics)  
[`list/peers`](#list/peers)  
[`list/lisp-eids`](#list/lisp-eids)  
[`validate/address`](#validate/address)  

Direct endpoints support filters and operators.  

-----------------------------------

# <a name="stop"></a> `stop`

Initiate node shutdown sequence.

```
system/stop
```

### Parameters:

`password` : Optional to **authenticate** before initiating core shutdown. This password is the one provided in the configuration file which will help prevent accidental remote shutdown.

**Note:** The shutdown authentication password has to be provided with the flag `-system/stop=<password>`

### Results:

#### Return value JSON object:

```
"Nexus server stopping"
[Completed in 0.039270 ms]
```

-----------------------------------

# <a name="get/info"></a> `get/info`

Returns a summary of information about this node

```
system/get/info
```

### Parameters:

[`Filtering`](/docs/API/FILTERING.MD).

### Results:

#### Return value JSON object:

```
{
    "version": "5.1.0-rc2 Tritium++ CLI [LLD][x64]",
    "protocolversion": 3010000,
    "walletversion": 10001,
    "timestamp": 1658759654,
    "hostname": "localhost",
    "directory": "/home/nexus/.Nexus/",
    "address": "11.79.25.105",
    "private": false,
    "hybrid": false,
    "multiuser": false,
    "litemode": false,
    "blocks": 4553990,
    "status": "Synchronized",
    "synchronized": true,
    "syncing": false,
    "txtotal": 0,
    "conflicts": 0,
    "connections": 41
}
[Completed in 0.070190 ms]
```

#### Return values:

`version` : The daemon software version.

`protocolversion` : The Lower Level Protocol (LLP) version.

`walletversion` : Legacy wallet version.

`timestamp` : Current unified time as reported by this node.

`hostname` : The local machine host name.

`directory` : The current data directory.

`address` : The public IP address of the local machine, as seen by peers that you are connected to. This will be blank until you have at least one peer connection.

`testnet` : If this node is running on the testnet then this shows the testnet number.

`private` : Whether this node is running in private mode.

`hybrid` : Whether this node is running in hybrid mode.

`latency` : Only returned in hybrid or private mode. The latency parameter value in milliseconds.

`multiuser` : Whether this node is running in multiuser mode.

`litemode` : Flag indicating whether this node is running in lite (client) mode.

`nolegacy` : Flag indicating whether this node has been compiled with support for the legacy UTxO wallet. If it has not then all of the legacy wallet functions (available via the legacy RPC) are not available. This flag is currently omitted from the response if it has been compiled with legacy support, hence it will only be included if it is true.

`blocks` : The current block height of this node.

`status` : The current status message for this node (e.g., "Synchronized", "Synchronizing", etc.).

`synchronized` : Flag indicating whether this node is currently synchronized with the network.

`syncing` : Contains sync status information when the node is synchronizing. Returns `false` when not syncing, or an object with sync details when syncing.

`txtotal` : Number of transactions in the node's mempool.

`conflicts` : Number of conflicting transactions in the node's mempool.

`connections` : Number of peer connections.

`eids` : Array of LISP Endpoint Idendifiers (EID's) for this node. Not included unless this node is using LISP.

-----------------------------------

# <a name="get/metrics"></a> `get/metrics`

Returns metrics and statistics for the ledger, registers, sigchains, trust and reserves.

```
system/get/metrics
```

### Parameters:

[`Filtering`](/docs/API/FILTERING.MD).

#### Return value JSON object:

```
{
    "registers": {
        "total": 183908,
        "names": {
            "global": 38,
            "local": 60668,
            "namespaced": 29
        },
        "namespaces": 14,
        "objects": {
            "accounts": 91400,
            "assets": 123,
            "crypto": 30362,
            "tokenized": 51,
            "tokens": 134
        },
        "state": {
            "raw": 0,
            "readonly": 706
        }
    },
    "sigchains": 30351,
    "trust": {
        "total": 501,
        "stake": 30155872.897004,
        "trust": 8360292851
    },
    "reserves": {
        "ambassador": 332.180905,
        "developer": 222.343689,
        "fee": 108889.217,
        "hash": 91.32012,
        "prime": 32.78941
    }
}
[Completed in 1020.804545 ms]
```

#### Return values:

`registers` : Statistics on the register database.  
{  
`names` : Total number of name registers, including user local, global, and namespaced.  
{  
`global` : Number of names that have been created in the global namespace.    

`local` : Number of names that have been created in the local namespace.  
       
`namespaced` : Number of names that have been created in a namespace.  
}    

`namespaces` : Number of namespaces that have been created.

`objects` : Statistics of non-standard object registers. These include accounts, tokens, crypto, assets, items and any other derived object register.  
{  
`accounts` : Number of account registers. This includes both NXS and other token accounts.

`assets` : Number of asset registers.
   
`crypto` : Number of crypto registers.

`tokenized` : Number of object registers that have been tokenized (where the object owner is a token rather than a profile).
   
`tokens` : Number of token registers.  
}

`state` : Statistics of raw and readonly registers.  
{  
`raw` : Number of raw registers.  
   
`readonly` : Number of read only registers.  
}  
}     

`sig_chains` : Total number of signature chains in the ledger.

`trust` : Statistics on active trust accounts.  
{  
`total` : Total number trust accounts that are active (have a non-zero stake balance).
   
`stake` : Total amount of NXS currently being staked network-wide).
   
`trust` : Total trust score of all trust accounts being staked.   
}    

`reserves` : Statistics on the amount of NXS currently in the various reserves.  
{  
`ambassador` : Amount of NXS in the ambassador reserves, waiting to be paid to the ambassador sig chains.
   
`developer` : Amount of NXS in the developer reserves, waiting to be paid to the developer sig chains.
   
`fee` : Amount of NXS in the fee reserves.
   
`hash` : Amount of NXS in the hash channel reserves.
   
`prime` : Amount of NXS in the prime channel reserves.  
}

-----------------------------------

# <a name="list/peers"></a> `list/peers`

Returns a summary of information about the peers currently connected to this node. The return array is sorted by the peer score value.

```
system/list/peers
```

### Parameters:

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 

### Results:

#### Return value JSON object:

```
[
    {
        "address": "45.180.14.231:9888",
        "type": "5.0.5 Tritium CLI [LLD][x64]",
        "version": 30000,
        "session": 8412734217501590516,
        "outgoing": true,
        "height": 0,
        "best": "00000000000000000000",
        "latency": 4294967295,
        "lastseen": 1658762086,
        "connects": 3,
        "drops": 0,
        "fails": 0,
        "score": 8850.0
    },
    {
        "address": "29.19.192.168:9888",
        "type": "5.1.0-rc2 Tritium++ CLI [LLD][x64]",
        "version": 3010000,
        "session": 10774729209155580028,
        "outgoing": true,
        "height": 0,
        "best": "00000000000000000000",
        "latency": 4294967295,
        "lastseen": 1658762098,
        "connects": 3,
        "drops": 1,
        "fails": 0,
        "score": 10195.0
    }
]
[Completed in 0.409770 ms]
```

#### Return values:

`address` : The IP address and port of the peer. This could be a LISP EID using the LISP overlay or an IP address using the native underlay.

`type` : The daemon software version string of the connected peer.

`version` : The protocol version being used to communicate.

`session` : Session ID for the current connection

`outgoing` : Flag indicating whether this was an outgoing connection or incoming

`height` : The current height of the peer.

`best` : Block hash of the peer's best chain

`genesis` : The genesis hash of the peer's logged in profile (if available).

`latency` : The calculated network latency between this node and the peer.

`lastseen` : UNIX timestamp of the last time this node had any communications with the peer.

`connects` : The number of connections successfully established with this peer since this node started.

`drops` : The number of connections dropped with this peer since this node started.

`fails` : The number of failed connection attempts to this peer since this node started.

`score` : The score value assigned to this peer based on latency and other connection statistics.

-----------------------------------

# <a name="list/lisp-eids"></a> `list/lisp-eids`

This will return the LISP Endpoint Identifiers (EID's) currently configured for this node. If the lispers.net API is not running / available then this will return an empty array.

```
system/list/lisp-eids
```

### Parameters:

-none-

### Results:

#### Return value JSON object:

```
[
    {
        "instance-id": "200",
        "eid": "240.139.76.244",
        "rlocs": [
            {
                "interface": "wlp59s0",
                "rloc-name": "mymachine",
                "rloc": "102.162.64.46"
            }
        ]
    },
    {
        "instance-id": "200",
        "eid": "fe::139:76:244",
        "rlocs": [
            {
                "interface": "wlp59s0",
                "rloc-name": "mymachine",
                "rloc": "102.162.64.46"
            }
        ]
    }
]
[Completed in 1004.256167 ms]
```

#### Return values:

`instance-id` : The LISP instance group that this EID belongs to.

`eid` : The endpoint identifier. This can be either IPv4 or IPv6 format.

`rlocs` : The array of RLOC's (routing locators) associtated with the EID.

`interface` : Name of the device for the RLOC.

`rloc-name` : Hostname associated with the device.

`rloc` : The IP address associated with the device.

-----------------------------------

# <a name="validate/address"></a> `validate/address`

Validates a register / legacy address.

```
system/validate/address
```

### Parameters:

`address` : Required to **validate** the base58 encoded register or legacy address to check.

### Results:

#### Return value JSON object:

```
{
    "address": "8BuhE1y5oard3cEWqCSYK35bC6LdPRvcPR8N7GXYNatTw6De8eK",
    "is_valid": true,
    "type": "OBJECT",
    "object_type": "ACCOUNT"
}
```
   
#### Return values:

`address` : The address that was validated.

`is_valid` : Boolean flag indicating if the address is valid or not. For legacy addresses this indicates that the address if simply formatted correctly. For register addresses this flag checks that a register exists in the blockchain at the specified address.

`type` : For register addresses, the type of register at the address. Values can be APPEND, LEGACY, OBJECT, RAW, READONLY, RESERVED, SYSTEM, or UNKNOWN.

`object_type` : If the type is OBJECT this field contains the standard object type of the object register. Values can be ACCOUNT, CRYPTO, NAME, NAMESPACE, REGISTER, TOKEN, TRUST, or UNKNOWN.

`is_mine` : If the type is LEGACY this boolean flag indicates if the private key for the address is held in the local wallet.

-----------------------------------