# Network API

The Network API provides methods for managing network connections and nodes, including listing connected peers and network nodes.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `node` | A network node |
| `peer` | A connected peer |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`list`](#list) | List network nodes or peers | `node`, `peer` |

## Commands

### <a name="list"></a> List Network Nodes

List all known network nodes.

```bash
network/list/node
```

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

#### Optional Parameters

##### Filtering
The following parameters can be used to filter the results:

- `limit`: The number of records to return for the current page. The default is 100
- `offset`: The number of records to skip before starting to return results
- `order`: The sort order to apply (default is "desc")
- `column`: The column to sort by (default is "lastseen")

#### Results

##### Return Value
```json
[
    {
        "address": "192.168.1.1:8080",
        "lastseen": 1581389015,
        "latency": 100
    }
]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `address` | The IP address and port of the node |
| `lastseen` | The UNIX timestamp when the node was last seen |
| `latency` | The network latency to the node in milliseconds |

### <a name="list"></a> List Network Peers

List all connected network peers.

```bash
network/list/peer
```

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

#### Optional Parameters

##### Filtering
The following parameters can be used to filter the results:

- `limit`: The number of records to return for the current page. The default is 100
- `offset`: The number of records to skip before starting to return results
- `order`: The sort order to apply (default is "desc")
- `column`: The column to sort by (default is "lastseen")

#### Results

##### Return Value
```json
[
    {
        "address": "192.168.1.1:8080",
        "type": "Nexus/1.0.0",
        "version": 1,
        "session": 12345,
        "outgoing": true,
        "height": 1000,
        "best": "0000000000000000000000000000000000000000000000000000000000000000",
        "genesis": "0000000000000000000000000000000000000000000000000000000000000000",
        "latency": 100,
        "lastseen": 1581389015
    }
]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `address` | The IP address and port of the peer |
| `type` | The version string of the connected peer |
| `version` | The protocol version being used to communicate |
| `session` | Session ID for the current connection |
| `outgoing` | Flag indicating whether this was an outgoing connection or incoming |
| `height` | The current height of the peer |
| `best` | Block hash of the peer's best chain |
| `genesis` | Block hash of the peer's genesis block |
| `latency` | The calculated network latency between this node and the peer |
| `lastseen` | Unix timestamp of the last time this node had any communications with the peer | 