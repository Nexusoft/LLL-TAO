# Ledger API

The Ledger API provides users with access to data held by the ledger such as blocks, transactions and network-wide staking and mining information.

> **Note**: Retrieving block data by height is only possible if the core has been configured with `--indexheight=1`.

## Direct Endpoints

The following commands are direct endpoints and thus do not support the verb and noun structure:

| Command | Description |
|:--------|:------------|
| [`get/blockhash`](#getblockhash) | Get block hash by height |
| [`get/block`](#getblock) | Get block data |
| [`list/blocks`](#listblocks) | List blocks in range |
| [`recent/blocks`](#recentblocks) | Get recent blocks |
| [`get/transaction`](#gettransaction) | Get transaction data |
| [`list/transactions`](#listtransactions) | List transactions |
| [`submit/transaction`](#submittransaction) | Submit a transaction |
| [`get/info`](#getinfo) | Get ledger info |
| [`get/metrics`](#getmetrics) | Get ledger metrics |
| [`sync/status`](#syncstatus) | Get sync status |

> **Note**: Direct endpoints support `filters` and `operators`.

## Deprecated Endpoints

The following endpoints are deprecated and will be removed in a future version:

- [`get/mininginfo`](#getmininginfo) - Deprecated in favor of `get/info`

## Commands

### <a name="getblockhash"></a> Get Block Hash

Retrieves the hash of the block for the given height.

```bash
ledger/get/blockhash
```

#### Required Parameters

- `height` (required): The block height to retrieve the hash for

#### Results

##### Return Value
```json
{
    "hash": "289b76fbe40ca5e0d0df68016677a8c9be6389f3225cc9faf65cb92bbc98a3580b57cb7ba3d7f7ab36ce890d47a4be90254d407270ad4c2e69c99c998ef8547ca45f054b081959d4d59feb871360bce888f82b4cdc1476502bd1b8e95dffe811e55592d3d4768b56e0510e02b5e32171858de097cd811e7c8d09920c24960ae8"
}
[Completed in 0.083791 ms]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `hash` | The hash of the block |

### <a name="getblock"></a> Get Block

Retrieves block data for the given block hash or height.

```bash
ledger/get/block
```

#### Required Parameters

One of the following parameters is required to identify the block:

- `hash` (required if `height` not provided): The block hash to retrieve the block data for
- `height` (required if `hash` not provided): The block height to retrieve the block data for

#### Optional Parameters

- `verbose`: Determines how much transaction data to include in the response. Supported values are:
  - `default`: hash
  - `summary`: type, version, sequence, timestamp, and contracts
  - `detail`: genesis, nexthash, prevhash, pubkey and signature

See also:
- [`Sorting`](//docs/API/SORTING.MD)
- [`Filtering`](/docs/API/FILTERING.MD)

#### Results

##### Return Value
```json
{
    "hash": "69336bc69ac2b9d06d7b9f9ede253cbeadba227751125cbb220654499c59a7b5138ea06ff62670fae614e5fed26aec8d3a0f346e03f10ea6c83dcb20ba8d93d17d92fffd6604666034e7002f421e39e64cef43325dfdf2c0334e51e018a31710c8fac17214d96d309b8dddb8f9bf9b3a27d28c71016b93a0e89f588c3604420a",
    "proofhash": "0000000000036b5c62766b5781f78734cf336f6fc6bce0363b93092551390fa7bd4d942d20316758def777b770db0500c3910f43b147c006ad11246ae62ed28284a4af309aebdc31ed6df23ebc7218b5a638b4da42cb496661fb12cace6890b920233e7db70a3daa8b439e0973a0dd01be67db652eaf8e7281027228f47a89cf",
    "size": 797,
    "height": 4555100,
    "channel": 2,
    "version": 8,
    "merkleroot": "017ff70158049886f8bad93cfe15285bae34d0e643991088e00769228161868a60230551724ac0ad5c7a98fabfdbebca88dffedf1a9e5ff095524b7565bccee1",
    "timestamp": 1658817542,
    "date": "2022-07-26 06:39:02 UTC",
    "nonce": 396216179366,
    "bits": "7b06e198",
    "difficulty": 2380.934877,
    "mint": 1.921131,
    "previousblockhash": "7dff690671fb7af3413f59315757a2fad8fb8acb7ea323269cc94152e9cbc696ed5064ec581c0709972a584f159ec75f8597559f93ec5f13d1bc608c0e7799ec891a5f26b455c97f0ac9dd1d826cb396aae1bcef8c0b74639e034161bb1d22a7cd4b29580d73ecba882a91273cc10121233096a5bb12c45ad774975c1bef925e",
    "nextblockhash": "5c985f5402f0bcb339ebef0e2edcad30427bf79803994772cd3a1458012748cd1d9d2ee8d6f1466f0c8ca867e1d8a2d9cfe6bc8cfdd885421fb56c55bfc11328f19c61bdc87193831851526a835e457fb46fcd0088879d564e88bb39524aa40489329ecc2cbc6a0453b8dd69a90b82232df6ee6d39ddec80e16149c99b1ddeab",
    "tx": [
        {
            "txid": "017ff70158049886f8bad93cfe15285bae34d0e643991088e00769228161868a60230551724ac0ad5c7a98fabfdbebca88dffedf1a9e5ff095524b7565bccee1",
            "type": "tritium base",
            "version": 4,
            "sequence": 11340,
            "timestamp": 1658817487,
            "blockhash": "69336bc69ac2b9d06d7b9f9ede253cbeadba227751125cbb220654499c59a7b5138ea06ff62670fae614e5fed26aec8d3a0f346e03f10ea6c83dcb20ba8d93d17d92fffd6604666034e7002f421e39e64cef43325dfdf2c0334e51e018a31710c8fac17214d96d309b8dddb8f9bf9b3a27d28c71016b93a0e89f588c3604420a",
            "confirmations": 262,
            "genesis": "a1e91c3b5d2856427508823fea815a37a11f5745d78e79091022d55e5bdb224d",
            "nexthash": "6adfd58123aa4ea622d808bbcf5b11d80d41921358c50dea71d25cf5791df106",
            "prevhash": "018f90d5ed4c2744e1d319c3b77e16f3a06187ee79e4a2bc982bc53dedfb2298aa0df30622df9cf080324aa103623db21648043cb1bb8f5a186764151fa9b242",
            "pubkey": "025d8d82339653aa9256bec27119cee55a9535e07b55aedcf7ed44a1f775a6afde6db2f76df2f337dd941ec337a98ad196c6840c040130ddc033cf779680735cd4",
            "signature": "30818402404da5e71d74b981fc7027cf30d0e7e56ea79367e6f97c76d3380bd3b56ea6aa608f6c370553c17c66fa022571412ab6cf2ebd41d9cc80295e37d6b59ed3a82fe602407a09ed3861823b1ec0ddd0d45a327b7615408b7d116032f6a806d2f286a420ac52275b1540085048491667173429c72113654255da72b12189a22eea89b8fa90",
            "contracts": [
                {
                    "id": 0,
                    "OP": "COINBASE",
                    "genesis": "a1e91c3b5d2856427508823fea815a37a11f5745d78e79091022d55e5bdb224d",
                    "nonce": 180,
                    "amount": 1.921131,
                    "token": "0000000000000000000000000000000000000000000000000000000000000000",
                    "ticker": "NXS"
                }
            ]
        }
    ]
}
[Completed in 0.422600 ms]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `hash` | The hash of the block |
| `proofhash` | The proof hash of the block |
| `size` | The block size in bytes |
| `height` | The block height |
| `channel` | The block channel (0=Stake, 1=Prime, 2=Hash) |
| `version` | Serialization version of this block |
| `merkleroot` | The hash merkle root of block transactions |
| `time` | The unified time the block was created |
| `nonce` | The solution nonce |
| `bits` | The compact representation for the block difficulty |
| `difficulty` | The channel specific difficulty |
| `mint` | The value minted in this block |
| `previousblockhash` | The hash of the previous block in the chain |
| `nextblockhash` | The hash of the next block in the chain, unless this is the last block in the chain |
| `tx` | An array of transactions included in this block |
| `tx[].type` | The description of the transaction (legacy | tritium base | trust | genesis | user) |
| `tx[].version` | The serialization version of the transaction |
| `tx[].sequence` | The sequence number of this transaction within the signature chain |
| `tx[].timestamp` | The Unix timestamp of when the transaction was created |
| `tx[].blockhash` | The hash of the block that this transaction is included in. Blank if not yet included in a block |
| `tx[].confirmations` | The number of confirmations that this transaction has obtained by the network |
| `tx[].genesis` | The profile username hash |
| `tx[].nexthash` | The hash of the next transaction in the sequence |
| `tx[].prevhash` | The hash of the previous transaction in the sequence |
| `tx[].pubkey` | The public key |
| `tx[].signature` | The signature hash |
| `tx[].contracts` | The array of contracts bound to this transaction and their details with opcodes |
| `tx[].contracts[].id` | The sequential ID of this contract within the transaction |
| `tx[].contracts[].OP` | The contract operation. Can be APPEND, CLAIM, COINBASE, CREATE, CREDIT, DEBIT, FEE, GENESIS, LEGACY, TRANSFER, TRUST, STAKE, UNSTAKE, WRITE |
| `tx[].contracts[].for` | For CREDIT transactions, the contract that this credit was created for. Can be COINBASE, DEBIT or LEGACY |
| `tx[].contracts[].txid` | The transaction that was credited / claimed |
| `tx[].contracts[].contract` | The ID of the contract within the transaction that was credited / claimed |
| `tx[].contracts[].proof` | The register address proving the credit |
| `tx[].contracts[].from` | For DEBIT, CREDIT, FEE transactions, the register address of the account that the debit is being made from |
| `tx[].contracts[].from_name` | For DEBIT, CREDIT, FEE transactions, the name of the account that the debit is being made from. Only included if the name can be resolved |
| `tx[].contracts[].to` | For DEBIT and CREDIT transactions, the register address of the recipient account |
| `tx[].contracts[].to_name` | For DEBIT and CREDIT transactions, the name of the recipient account. Only included if the name can be resolved |
| `tx[].contracts[].amount` | The token amount of the transaction |
| `tx[].contracts[].token` | The register address of the token that the transaction relates to. Set to 0 for NXS transactions |
| `tx[].contracts[].ticker` | The name of the token that the transaction relates to |
| `tx[].contracts[].reference` | For DEBIT and CREDIT transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number |

### <a name="listblocks"></a> List Blocks

Retrieves an array of block data for a sequential range of blocks from a given hash or height.

```bash
ledger/list/blocks
```

#### Required Parameters

One of the following parameters is required to identify the block:

- `hash` (required if `height` not provided): The block hash to retrieve the block data for
- `height` (required if `hash` not provided): The block height to retrieve the block data for

#### Optional Parameters

- `verbose`: Determines how much transaction data to include in the response. Supported values are:
  - `summary`: type, version, sequence, timestamp, and operation
  - `detail`: genesis, nexthash, prevhash, pubkey and signature
- `where`: An array of clauses to filter the JSON results. More information on filtering the results from /list/xxx API methods can be found at [`Queries`](/docs/API/QUERIES.md)

See also:
- [`Sorting`](//docs/API/SORTING.MD)

### <a name="recentblocks"></a> Recent Blocks

Retrieves an array of the most recent blocks in the chain.

```bash
ledger/recent/blocks
```

#### Parameters

- `verbose`: Optional, determines how much transaction data to include in the response. Supported values are:
  - `summary`: type, version, sequence, timestamp, and operation
  - `detail`: genesis, nexthash, prevhash, pubkey and signature

See also:
- [`Sorting`](//docs/API/SORTING.MD)
- [`Filtering`](/docs/API/FILTERING.MD)
- [`Operators`](//docs/API/OPERATORS.MD)

#### Results

##### Return Value
```json
[
    {
        "hash": "69336bc69ac2b9d06d7b9f9ede253cbeadba227751125cbb220654499c59a7b5138ea06ff62670fae614e5fed26aec8d3a0f346e03f10ea6c83dcb20ba8d93d17d92fffd6604666034e7002f421e39e64cef43325dfdf2c0334e51e018a31710c8fac17214d96d309b8dddb8f9bf9b3a27d28c71016b93a0e89f588c3604420a",
        "height": 4555100,
        "channel": 2,
        "version": 8,
        "timestamp": 1658817542,
        "date": "2022-07-26 06:39:02 UTC",
        "mint": 1.921131
    }
]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `hash` | The hash of the block |
| `height` | The block height |
| `channel` | The block channel (0=Stake, 1=Prime, 2=Hash) |
| `version` | Serialization version of this block |
| `timestamp` | The unified time the block was created |
| `date` | The formatted date and time the block was created |
| `mint` | The value minted in this block |

### <a name="gettransaction"></a> Get Transaction

Retrieves transaction data for a given transaction hash.

```bash
ledger/get/transaction
```

#### Parameters

- `format`: Determines the format of the return value. Parameter value can be JSON (the default) or raw. If raw is specified then the method returns a serialized, hex-encoded transaction that can subsequently be broadcast to the network via /ledger/submit/transaction.
- `txid`: Required to identify the txid to retrieve the transaction data for
- `verbose`: Optional, determines how much transaction data to include in the response. This is ignored if raw format is requested. Supported values are:
  - summary: hash, type, version, sequence, timestamp, and contracts
  - detail: genesis, nexthash, prevhash, pubkey and signature

See also:
- [`Sorting`](//docs/API/SORTING.MD)
- [`Filtering`](/docs/API/FILTERING.MD)

#### Returns

##### Return Value
```json
{
    "txid": "017ff70158049886f8bad93cfe15285bae34d0e643991088e00769228161868a60230551724ac0ad5c7a98fabfdbebca88dffedf1a9e5ff095524b7565bccee1",
    "type": "tritium base",
    "version": 4,
    "sequence": 11340,
    "timestamp": 1658817487,
    "blockhash": "69336bc69ac2b9d06d7b9f9ede253cbeadba227751125cbb220654499c59a7b5138ea06ff62670fae614e5fed26aec8d3a0f346e03f10ea6c83dcb20ba8d93d17d92fffd6604666034e7002f421e39e64cef43325dfdf2c0334e51e018a31710c8fac17214d96d309b8dddb8f9bf9b3a27d28c71016b93a0e89f588c3604420a",
    "confirmations": 267,
    "genesis": "a1e91c3b5d2856427508823fea815a37a11f5745d78e79091022d55e5bdb224d",
    "nexthash": "6adfd58123aa4ea622d808bbcf5b11d80d41921358c50dea71d25cf5791df106",
    "prevhash": "018f90d5ed4c2744e1d319c3b77e16f3a06187ee79e4a2bc982bc53dedfb2298aa0df30622df9cf080324aa103623db21648043cb1bb8f5a186764151fa9b242",
    "pubkey": "025d8d82339653aa9256bec27119cee55a9535e07b55aedcf7ed44a1f775a6afde6db2f76df2f337dd941ec337a98ad196c6840c040130ddc033cf779680735cd4",
    "signature": "30818402404da5e71d74b981fc7027cf30d0e7e56ea79367e6f97c76d3380bd3b56ea6aa608f6c370553c17c66fa022571412ab6cf2ebd41d9cc80295e37d6b59ed3a82fe602407a09ed3861823b1ec0ddd0d45a327b7615408b7d116032f6a806d2f286a420ac52275b1540085048491667173429c72113654255da72b12189a22eea89b8fa90",
    "contracts": [
        {
            "id": 0,
            "OP": "COINBASE",
            "genesis": "a1e91c3b5d2856427508823fea815a37a11f5745d78e79091022d55e5bdb224d",
            "nonce": 180,
            "amount": 1.921131,
            "token": "0000000000000000000000000000000000000000000000000000000000000000",
            "ticker": "NXS"
        }
    ]
}
[Completed in 0.290130 ms]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `txid` | The transaction hash |
| `type` | The description of the transaction (legacy | tritium base | trust | genesis | user) |
| `version` | The serialization version of the transaction |
| `sequence` | The sequence number of this transaction within the signature chain |
| `timestamp` | The Unix timestamp of when the transaction was created |
| `blockhash` | The hash of the block that this transaction is included in. Blank if not yet included in a block |
| `confirmations` | The number of confirmations that this transaction obtained by the network |
| `genesis` | The profile username hash |
| `nexthash` | The hash of the next transaction in the sequence |
| `prevhash` | The hash of the previous transaction in the sequence |
| `pubkey` | The public key |
| `signature` | The signature hash |
| `contracts` | The array of contracts bound to this transaction and their details with opcodes |
| `contracts[].id` | The sequential ID of this contract within the transaction |
| `contracts[].OP` | The contract operation. Can be APPEND, CLAIM, COINBASE, CREATE, CREDIT, DEBIT, FEE, GENESIS, LEGACY, TRANSFER, TRUST, STAKE, UNSTAKE, WRITE |
| `contracts[].for` | For CREDIT transactions, the contract that this credit was created for. Can be COINBASE, DEBIT or LEGACY |
| `contracts[].txid` | The transaction that was credited / claimed |
| `contracts[].contract` | The ID of the contract within the transaction that was credited / claimed |
| `contracts[].proof` | The register address proving the credit |
| `contracts[].from` | For DEBIT, CREDIT, FEE transactions, the register address of the account that the debit is being made from |
| `contracts[].from_name` | For DEBIT, CREDIT, FEE transactions, the name of the account that the debit is being made from. Only included if the name can be resolved |
| `contracts[].to` | For DEBIT and CREDIT transactions, the register address of the recipient account |
| `contracts[].to_name` | For DEBIT and CREDIT transactions, the name of the recipient account. Only included if the name can be resolved |
| `contracts[].amount` | The token amount of the transaction |
| `contracts[].token` | The register address of the token that the transaction relates to. Set to 0 for NXS transactions |
| `contracts[].ticker` | The name of the token that the transaction relates to |
| `contracts[].reference` | For DEBIT and CREDIT transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number |

### <a name="listtransactions"></a> List Transactions

Retrieves transaction data for a given transaction hash.

```bash
ledger/get/transaction
```

#### Parameters

- `session`: Required by argument `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.
- `verbose`: Optional, determines how much transaction data to include in the response. Supported values are:
  - summary: hash, type, version, sequence, timestamp, and contracts
  - detail: genesis, nexthash, prevhash, pubkey and signature
- `where`: An array of clauses to filter the JSON results. More information on filtering the results from /list/xxx API methods can be found at [`Queries`](/docs/API/QUERIES.md)

See also:
- [`Sorting`](//docs/API/SORTING.MD)
- [`Filtering`](/docs/API/FILTERING.MD)
- [`Operators`](//docs/API/OPERATORS.MD)

#### Returns

##### Return Value
```json
{
    "txid": "017ff70158049886f8bad93cfe15285bae34d0e643991088e00769228161868a60230551724ac0ad5c7a98fabfdbebca88dffedf1a9e5ff095524b7565bccee1",
    "type": "tritium base",
    "version": 4,
    "sequence": 11340,
    "timestamp": 1658817487,
    "blockhash": "69336bc69ac2b9d06d7b9f9ede253cbeadba227751125cbb220654499c59a7b5138ea06ff62670fae614e5fed26aec8d3a0f346e03f10ea6c83dcb20ba8d93d17d92fffd6604666034e7002f421e39e64cef43325dfdf2c0334e51e018a31710c8fac17214d96d309b8dddb8f9bf9b3a27d28c71016b93a0e89f588c3604420a",
    "confirmations": 267,
    "genesis": "a1e91c3b5d2856427508823fea815a37a11f5745d78e79091022d55e5bdb224d",
    "nexthash": "6adfd58123aa4ea622d808bbcf5b11d80d41921358c50dea71d25cf5791df106",
    "prevhash": "018f90d5ed4c2744e1d319c3b77e16f3a06187ee79e4a2bc982bc53dedfb2298aa0df30622df9cf080324aa103623db21648043cb1bb8f5a186764151fa9b242",
    "pubkey": "025d8d82339653aa9256bec27119cee55a9535e07b55aedcf7ed44a1f775a6afde6db2f76df2f337dd941ec337a98ad196c6840c040130ddc033cf779680735cd4",
    "signature": "30818402404da5e71d74b981fc7027cf30d0e7e56ea79367e6f97c76d3380bd3b56ea6aa608f6c370553c17c66fa022571412ab6cf2ebd41d9cc80295e37d6b59ed3a82fe602407a09ed3861823b1ec0ddd0d45a327b7615408b7d116032f6a806d2f286a420ac52275b1540085048491667173429c72113654255da72b12189a22eea89b8fa90",
    "contracts": [
        {
            "id": 0,
            "OP": "COINBASE",
            "genesis": "a1e91c3b5d2856427508823fea815a37a11f5745d78e79091022d55e5bdb224d",
            "nonce": 180,
            "amount": 1.921131,
            "token": "0000000000000000000000000000000000000000000000000000000000000000",
            "ticker": "NXS"
        }
    ]
}
[Completed in 0.290130 ms]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `txid` | The transaction hash |
| `type` | The description of the transaction (legacy | tritium base | trust | genesis | user) |
| `version` | The serialization version of the transaction |
| `sequence` | The sequence number of this transaction within the signature chain |
| `timestamp` | The Unix timestamp of when the transaction was created |
| `blockhash` | The hash of the block that this transaction is included in. Blank if not yet included in a block |
| `confirmations` | The number of confirmations that this transaction obtained by the network |
| `genesis` | The profile username hash |
| `nexthash` | The hash of the next transaction in the sequence |
| `prevhash` | The hash of the previous transaction in the sequence |
| `pubkey` | The public key |
| `signature` | The signature hash |
| `contracts` | The array of contracts bound to this transaction and their details with opcodes |
| `contracts[].id` | The sequential ID of this contract within the transaction |
| `contracts[].OP` | The contract operation. Can be APPEND, CLAIM, COINBASE, CREATE, CREDIT, DEBIT, FEE, GENESIS, LEGACY, TRANSFER, TRUST, STAKE, UNSTAKE, WRITE |
| `contracts[].for` | For CREDIT transactions, the contract that this credit was created for. Can be COINBASE, DEBIT or LEGACY |
| `contracts[].txid` | The transaction that was credited / claimed |
| `contracts[].contract` | The ID of the contract within the transaction that was credited / claimed |
| `contracts[].proof` | The register address proving the credit |
| `contracts[].from` | For DEBIT, CREDIT, FEE transactions, the register address of the account that the debit is being made from |
| `contracts[].from_name` | For DEBIT, CREDIT, FEE transactions, the name of the account that the debit is being made from. Only included if the name can be resolved |
| `contracts[].to` | For DEBIT and CREDIT transactions, the register address of the recipient account |
| `contracts[].to_name` | For DEBIT and CREDIT transactions, the name of the recipient account. Only included if the name can be resolved |
| `contracts[].amount` | The token amount of the transaction |
| `contracts[].token` | The register address of the token that the transaction relates to. Set to 0 for NXS transactions |
| `contracts[].ticker` | The name of the token that the transaction relates to |
| `contracts[].reference` | For DEBIT and CREDIT transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number |

### <a name="submittransaction"></a> Submit Transaction

Submits a transaction to be included in the mempool and broadcast to the network.

```bash
ledger/submit/transaction
```

#### Parameters

- `data`: The serialized, hex-encoded transaction data to be submitted

#### Results

##### Return Value
```json
{
    "hash": "47959e245f45aab773c0ce5320a5454f49ac15f63e15acb36855ac654d54d6314fe36b61dd64ec7a9a546bcc439a628e9badcdccb6e5f8072d04a0a3b67f8679"
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `hash` | The tranaction hash, if successfully committed to the mempool / broadcast |

### <a name="getinfo"></a> Get Info

Retrieves mining and NXS supply related data for the ledger.

```bash
ledger/get/info
```

#### Parameters

See also:
- [`Filtering`](/docs/API/FILTERING.MD)

#### Returns

##### Return Value
```json
{
    "stake": {
        "height": 1478452,
        "weight": "0000000000000000000136d9ce5eff4a",
        "timespan": 163,
        "fees": 17789.398,
        "difficulty": 528.770417
    },
    "prime": {
        "height": 1607544,
        "weight": "0000000000000000000acf8296a5dace",
        "timespan": 152,
        "fees": 40208.42,
        "difficulty": 7.5050107,
        "reserve": 275.540352,
        "reward": 1.89596,
        "primes": 721104210
    },
    "hash": {
        "height": 1508658,
        "weight": "00000000000000000153053d8ed4b372",
        "timespan": 162,
        "fees": 50891.389,
        "difficulty": 995.146668,
        "reserve": 390.83726,
        "reward": 1.895962,
        "hashes": 1700092903007
    },
    "supply": {
        "total": 74104442.485668,
        "target": 71313176.905702,
        "inflation": 0.4766,
        "minute": 3.651488,
        "hour": 219.087198,
        "day": 5256.922284,
        "week": 36747.303,
        "month": 146280.58871
    },
    "height": 4594651,
    "timestamp": 1660902585,
    "checkpoint": "20b50d243a585a6c9a3d420ebfa61aee8ac0a68c1d8c71ef415247fc3d677c44b6f0a2b497c72ca4d9f2271cfbe1090042522740e7671781c7548b275f84eebce7d4340b703abfe8544a3fd0ff1cd6132f6acb1b04948da81ae965a1828e307d1ab47b731d9cca7f28be830082b81c2a3d0b1fadd3e9fcc2ec9203d22a61ab12"
}
[Completed in 3827.013421 ms]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `Stake` | Retreives information for the stake channel |
| `Stake.height` | The current number of blocks for the stake channel |
| `Stake.weight` | The total work completed for the stake channel |
| `Stake.timespan` | It is the average block time for the stake channel |
| `Stake.fees` | It is the total NXS accumulated on the stake channel |
| `Stake.difficulty` | The current difficulty of the stake channel |
| `prime` | Retreives information for the prime channel |
| `prime.height` | The current number of blocks for the prime channel |
| `prime.weight` | The total work completed for prime channel |
| `prime.timespan` | It is the average block time for the prime channel |
| `prime.fees` | It is the total NXS accumulated on the prime channel |
| `prime.difficulty` | The current difficulty of the prime channel |
| `prime.reserve` | The amount of NXS in the reserve balance for the prime channel |
| `prime.reward` | The block reward for the next prime block to be found |
| `prime.primes` | It is total network hash rate for prime channel |
| `hash` | Retreives information for the hash channel |
| `hash.height` | The current number of blocks for the hash channel |
| `hash.weight` | The total work completed for hash channel |
| `hash.timespan` | It is the average block time for the hash channel |
| `hash.fees` | It is the total NXS accumulated on the hash channel |
| `hash.difficulty` | The current difficulty of the hash channel |
| `hash.reserve` | The amount of NXS in the reserve balance for the hash channel |
| `hash.reward` | The block reward for the next hash block to be found |
| `hash.hashes` | It is total network hash rate for the hash channel |
| `supply` | Metrics on NXS supply rates |
| `supply.total` | Total amount of NXS in existence |
| `supply.target` | The target supply rate for this point in time |
| `supply.inflation` | The current inflation rate percentage based on the last year of data |
| `supply.minute` | The increase in supply rate within the last minute |
| `supply.hour` | The increase in supply rate within the last hour |
| `supply.day` | The increase in supply rate within the last day |
| `supply.week` | The increase in supply rate within the last week |
| `supply.month` | The increase in supply rate within the last 30 days |
| `height` | The block height when the information is retrieved |
| `timestamp` | The Unix timestamp of the most recent block in the blockchain |
| `checkpoint` | It is the most recent checkpoint that blocks can't be reorganized before, so everything before that hash is finalized |

### <a name="getmetrics"></a> Get Metrics

Retrieves mining related data for the ledger.

```bash
ledger/get/metrics
```

#### Parameters

See also:
- [`Filtering`](/docs/API/FILTERING.MD)

#### Returns

##### Return Value
```json
{
    "volumes": {
        "transactions": {
            "daily": 1858,
            "weekly": 13729,
            "monthly": 55761
        },
        "contracts": {
            "daily": 2272,
            "weekly": 18588,
            "monthly": 82712
        },
        "accounts": {
            "daily": 272,
            "weekly": 1566,
            "monthly": 4128
        }
    },
    "exchanges": {
        "deposits": {
            "daily": 675.23,
            "weekly": 39999.196652,
            "monthly": 1151130.175639
        },
        "withdraws": {
            "daily": 65413.954116,
            "weekly": 839913.950865,
            "monthly": 4010148.232528
        }
    },
    "network": {
        "mint": {
            "staking": {
                "daily": 2064.475289,
                "weekly": 14508.834716,
                "monthly": 56739.446981
            },
            "mining": {
                "daily": 5235.841724,
                "weekly": 36701.701742,
                "monthly": 148204.639604
            }
        },
        "stake": {
            "daily": 11651.941826,
            "weekly": 533078.575505,
            "monthly": 1367705.4935
        }
    }
}
[Completed in 12378.496480 ms]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `volumes` | This retrieves the transactions, contracts and accounts volume |
| `volumes.transactions` | The total number of transactions in the ledger |
| `volumes.contracts` | The total number of contracts in the ledger |
| `volumes.accounts` | The total number of accounts in the ledger |
| `blocks` | This retrieves the blocks volume |
| `blocks.total` | The total number of blocks in the ledger |
| `blocks.stake` | The total number of stake blocks in the ledger |
| `blocks.prime` | The total number of prime blocks in the ledger |
| `blocks.hash` | The total number of hash blocks in the ledger |
| `blocks.timespan` | The average block time in seconds |
| `blocks.timestamp` | The Unix timestamp of the most recent block |
| `blocks.checkpoint` | The most recent checkpoint that blocks can't be reorganized before |
| `blocks.difficulty` | The current network difficulty |
| `blocks.weight` | The total work completed for the network |
| `blocks.fees` | The total NXS accumulated in fees |
| `blocks.reward` | The block reward for the next block to be found |
| `blocks.hashes` | The total network hash rate |
| `blocks.reserve` | The amount of NXS in the reserve balance |
| `blocks.supply` | The total amount of NXS in existence |
| `blocks.target` | The target supply rate for this point in time |
| `blocks.inflationrate` | The current inflation rate percentage based on the last year of data |
| `blocks.minute` | The increase in supply rate within the last minute |
| `blocks.hour` | The increase in supply rate within the last hour |
| `blocks.day` | The increase in supply rate within the last day |
| `blocks.week` | The increase in supply rate within the last week |
| `blocks.month` | The increase in supply rate within the last 30 days |

### <a name="syncstatus"></a> Sync Status

Retrieves the current synchronization status of the ledger.

```bash
ledger/sync/status
```

#### Parameters

See also:
- [`Filtering`](/docs/API/FILTERING.MD)

#### Returns

##### Return Value
```json
{
    "status": "synced",
    "height": 4555100,
    "timestamp": 1658817542,
    "checkpoint": "69336bc69ac2b9d06d7b9f9ede253cbeadba227751125cbb220654499c59a7b5138ea06ff62670fae614e5fed26aec8d3a0f346e03f10ea6c83dcb20ba8d93d17d92fffd6604666034e7002f421e39e64cef43325dfdf2c0334e51e018a31710c8fac17214d96d309b8dddb8f9bf9b3a27d28c71016b93a0e89f588c3604420a"
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `status` | The current synchronization status (synced, syncing, or error) |
| `height` | The current block height |
| `timestamp` | The Unix timestamp of the most recent block |
| `checkpoint` | The most recent checkpoint that blocks can't be reorganized before |