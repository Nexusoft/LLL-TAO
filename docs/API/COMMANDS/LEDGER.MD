# LEDGER API
-----------------------------------
The Ledger API provides users with access to data held by the ledger such as blocks, transactions and network-wide staking and mining information.

**Info:** Retrieving block data by height is only possible if the core has been configured with --indexheight=1.

## `Direct Endpoints`

The following commands are direct endpoints and thus do not support the above verb and noun structure available above.

[`get/blockhash`](#get/blockhash)  
[`get/block`](#get/block)  
[`list/blocks`](#list/blocks)  
[`get/transaction`](#get/transaction)  
[`list/transactions`](#list/transactions)  
[`submit/transaction`](#submit/transaction)  
[`get/info`](#get/info)  
[`get/metrics`](#get/metrics)  
[`recent/blocks`](#recent/blocks)  
[`sync/status`](#sync/status)  

Direct endpoints support `filters` and `operators`.

-----------------------------------

# <a name="get/blockhash"></a> `get/blockhash`

Retrieves the hash of the block for the given height.

```
ledger/get/blockhash
```

### Parameters:

`height` : The **block height** to retrieve the hash for.

### Results:

#### Return value JSON object:

```
{
    "hash": "289b76fbe40ca5e0d0df68016677a8c9be6389f3225cc9faf65cb92bbc98a3580b57cb7ba3d7f7ab36ce890d47a4be90254d407270ad4c2e69c99c998ef8547ca45f054b081959d4d59feb871360bce888f82b4cdc1476502bd1b8e95dffe811e55592d3d4768b56e0510e02b5e32171858de097cd811e7c8d09920c24960ae8"
}
[Completed in 0.083791 ms]  
```

#### Return values:

`hash` : The hash of the block.

-----------------------------------

# <a name="get/block"></a> `get/block`

Retrieves block data for the given block hash or height.

```
ledger/get/block
```

### Parameters:

`hash` : Required to **identify** the block to retrieve the block data for. This is optional if the height is provided.

`height` : Required to **identify** the block to retrieve the block data for. This is optional if the hash is provided.

`verbose` : Optional, determines how much **transaction data** to include in the response. Supported values are :
-  `default` : hash.

- `summary` : type, version, sequence, timestamp, and contracts.

- `detail` : genesis, nexthash, prevhash, pubkey and signature.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD). 

### Results:

#### Return value JSON object:

```
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

#### Return values:


`hash` : The hash of the block.

`proofhash` : The proof hash of the block.

`size` : The block size in bytes.

`height` : The block height.

`channel` : The block channel (0=Stake, 1=Prime, 2=Hash).

`version` : Serialization version of this block.

`merkleroot` : The hash merkle root of block transactions.

`time` : The unified time the block was created.

`nonce` : The solution nonce.

`bits` : The compact representation for the block difficulty.

`difficulty` : The channel specific difficulty.

`mint` : The value minted in this block.

`previousblockhash` : The hash of the previous block in the chain.

`nextblockhash` : The hash of the next block in the chain, unless this is the last block in the chain.

`tx` : An array of transactions included in this block.

`type` : The description of the transaction (legacy | tritium base | trust | genesis | user).

`version` : The serialization version of the transaction.

`sequence` : The sequence number of this transaction within the signature chain.

`timestamp` : The Unix timestamp of when the transaction was created.

`blockhash` : The hash of the block that this transaction is included in. Blank if not yet included in a block.

`confirmations` : The number of confirmations that this transaction has obtained by the network.

`genesis` : The profile username hash.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : The hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature` : The signature hash.

`hash` : The transaction hash.

`contracts` : The array of contracts bound to this transaction and their details with opcodes.  
{  
`id` : The sequential ID of this contract within the transaction.

`OP` : The contract operation. Can be APPEND, CLAIM, COINBASE, CREATE, CREDIT, DEBIT, FEE, GENESIS, LEGACY, TRANSFER, TRUST, STAKE, UNSTAKE, WRITE.

`for` : For CREDIT transactions, the contract that this credit was created for . Can be COINBASE, DEBIT or LEGACY.

`txid` : The transaction that was credited / claimed.

`contract` : The ID of the contract within the transaction that was credited / claimed.

`proof` : The register address proving the credit.

`from` : For DEBIT, CREDIT, FEE transactions, the register address of the account that the debit is being made from.

`from_name` : For DEBIT, CREDIT, FEE transactions, the name of the account that the debit is being made from. Only included if the name can be resolved.

`to` : For DEBIT and CREDIT transactions, the register address of the recipient account.

`to_name` : For DEBIT and CREDIT transactions, the name of the recipient account. Only included if the name can be resolved.

`amount` : The token amount of the transaction.

`token` : the register address of the token that the transaction relates to. Set to 0 for NXS transactions.

`ticker` : The name of the token that the transaction relates to.

`reference` : For DEBIT and CREDIT transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number.

}

-----------------------------------

# <a name="list/blocks"></a> `list/blocks`

Retrieves an array of block data for a sequential range of blocks from a given hash or height.

```
ledger/list/blocks
```

### Parameters:

`hash` : Required to **identify** the block to retrieve the block data for. This is optional if the `height` is provided

`height` : Required to **identify** the block to retrieve the block data for. This is optional if the `hash` is provided

`verbose` : This is optional, determines how much **transaction data** to include in the response. Supported values are:
- `summary` : type, version, sequence, timestamp, and operation.

- `detail` : genesis, nexthash, prevhash, pubkey and signature.

`where` : An array of clauses to **filter** the JSON results. More information on filtering the results from /list/xxx API methods can be found at [`Queries`](/docs/API/QUERIES.md).

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 

**NOTE :** 
- Either the hash or the height needs to be supplied, but not both.
- Retrieving block data by height is only allowed if the daemon has been configured with indexheight=1.

### Returns:

#### Return value JSON object:

```
[
    {
        "hash": "5c985f5402f0bcb339ebef0e2edcad30427bf79803994772cd3a1458012748cd1d9d2ee8d6f1466f0c8ca867e1d8a2d9cfe6bc8cfdd885421fb56c55bfc11328f19c61bdc87193831851526a835e457fb46fcd0088879d564e88bb39524aa40489329ecc2cbc6a0453b8dd69a90b82232df6ee6d39ddec80e16149c99b1ddeab",
        "proofhash": "3f10d762af5952f7fdda2f84a27f9afe121e54be25eb0a41ee5074219919b9a2832d535d2c7c995a91e21ee6000017b2e7cc2099537cda035c09ebcaa94112384e88cdeec9d1ba6eb662b1acdf4f41d9b964f3da5a578d757bd2653acd8ed7364e8bd807777fbaabb4cda19cad6506b84431940e6a1e43a32d92eff9d9ace635",
        "size": 807,
        "height": 4555101,
        "channel": 1,
        "version": 8,
        "merkleroot": "0161d4629a2f385be81bda414359aaafb6baec7522f017c6fd06183c975c6c4a9baf4b89e5890327ddd79853eb415bfb054f84da719ac653520097bbc01b6e6c",
        "timestamp": 1658817563,
        "date": "2022-07-26 06:39:23 UTC",
        "nonce": 15731843013639865474,
        "bits": "04c23531",
        "difficulty": 7.9836465,
        "mint": 1.92113,
        "previousblockhash": "69336bc69ac2b9d06d7b9f9ede253cbeadba227751125cbb220654499c59a7b5138ea06ff62670fae614e5fed26aec8d3a0f346e03f10ea6c83dcb20ba8d93d17d92fffd6604666034e7002f421e39e64cef43325dfdf2c0334e51e018a31710c8fac17214d96d309b8dddb8f9bf9b3a27d28c71016b93a0e89f588c3604420a",
        "nextblockhash": "6e4b01acefa38b2f591ba7060c165daafe4cdaac8f147f7b8a02986b8f8b3c82d051601746ac53ff57167a650d75d501eab859b4a52d48444f036b9df1571648a22d777dd71896860b6faa7236bcd54d2f2de2445dcdf2e5a34f64b5c8918d3a8a8fce357024d74ca3c8b76c5a1457d4182ce18e015a782316390fc0dfb8eb34",
        "tx": [
            {
                "txid": "0161d4629a2f385be81bda414359aaafb6baec7522f017c6fd06183c975c6c4a9baf4b89e5890327ddd79853eb415bfb054f84da719ac653520097bbc01b6e6c",
                "type": "tritium base",
                "version": 4,
                "sequence": 176177,
                "timestamp": 1658817543,
                "blockhash": "5c985f5402f0bcb339ebef0e2edcad30427bf79803994772cd3a1458012748cd1d9d2ee8d6f1466f0c8ca867e1d8a2d9cfe6bc8cfdd885421fb56c55bfc11328f19c61bdc87193831851526a835e457fb46fcd0088879d564e88bb39524aa40489329ecc2cbc6a0453b8dd69a90b82232df6ee6d39ddec80e16149c99b1ddeab",
                "confirmations": 249,
                "contracts": [
                    {
                        "id": 0,
                        "OP": "COINBASE",
                        "genesis": "a165c6bc1b1e0dd9444f3ec01f07686991a8b4bdc1ceb4143dbb3eed16339a74",
                        "nonce": 308,
                        "amount": 1.92113,
                        "token": "0000000000000000000000000000000000000000000000000000000000000000",
                        "ticker": "NXS"
                    }
                ]
            }
        ]
    }
]
[Completed in 8.883924 ms]
```

#### Return values:

`hash` : The hash of the block.

`proofhash` : The proof hash of the block.

`size` : The block size in bytes.

`height` : The block height.

`channel` : The block channel (0=Stake, 1=Prime, 2=Hash).

`version` : The serialization version of this block.

`merkleroot` : The hash merkle root of the block transactions.

`time` : The unified time the block was created.

`nonce` : The solution nonce.

`bits` : The compact representation for the block difficulty.

`difficulty` : The channel specific difficulty.

`mint` : The value minted in this block.

`previousblockhash` : The hash of the previous block in the chain.

`nextblockhash` : The hash of the next block in the chain, unless this is the last block in the chain.

`tx` : An array of transactions included in this block.

`type` : The description of the transaction (legacy | tritium base | trust | genesis | user).

`version` : The serialization version of the transaction.

`sequence` : The sequence number of this transaction within the signature chain.

`timestamp` : The Unix timestamp of when the transaction was created.

`blockhash` : The hash of the block that this transaction is included in. Blank if not yet included in a block.

`confirmations` : The number of confirmations that this transaction has obtained by the network.

`genesis` : The signature chain genesis hash.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : The hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature` : The signature hash.

`hash` : The transaction hash.

`contracts` : The array of contracts bound to this transaction and their details with opcodes.  
{  
`id` : The sequential ID of this contract within the transaction.

`OP` : The contract operation. Can be APPEND, CLAIM, COINBASE, CREATE, CREDIT, DEBIT, FEE, GENESIS, LEGACY, TRANSFER, TRUST, STAKE, UNSTAKE, WRITE.

`for` : For CREDIT transactions, the contract that this credit was created for . Can be COINBASE, DEBIT or LEGACY.

`txid` : The transaction that was credited / claimed.

`contract` : The ID of this contract within the transaction that was credited / claimed.

`proof` : The register address proving the credit.

`from` : For DEBIT, CREDIT, FEE transactions, the register address of the account that the debit is being made from.

`from_name` : For DEBIT, CREDIT, FEE transactions, the name of the account that the debit is being made from. Only included if the name can be resolved.

`to` : For DEBIT and CREDIT transactions, the register address of the recipient account.

`to_name` : For DEBIT and CREDIT transactions, the name of the recipient account. Only included if the name can be resolved.

`amount` : the token amount of the transaction.

`token` : the register address of the token that the transaction relates to. Set to 0 for NXS transactions.

`ticker` : The name of the token that the transaction relates to.

`reference` : For DEBIT and CREDIT transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number.

}

-----------------------------------

# <a name="get/transaction"></a> `get/transaction`

Retrieves transaction data for a given transaction hash.

```
ledger/get/transaction
```

### Parameters:

`format` : Determines the **format** of the return value. Parameter value can be JSON (the default) or raw. If raw is specified then the method returns a serialized, hex-encoded transaction that can subsequently be broadcast to the network via /ledger/submit/transaction.

`txid` : Required to **identify** the txid to retrieve the transaction data for. 

`verbose` : Optional, determines how much **transaction data** to include in the response. This is ignored if raw format is requested. Supported values are:
- summary : hash, type, version, sequence, timestamp, and contracts. 

- detail : genesis, nexthash, prevhash, pubkey and signature.  

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).  

### Returns:

#### Return value JSON object:

```
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

#### Return values:

`txid` : The transaction hash.

`type` : The description of the transaction (legacy | tritium base | trust | genesis | user).

`version` : The serialization version of the transaction.

`sequence` : The sequence number of this transaction within the signature chain.

`timestamp` : The Unix timestamp of when the transaction was created.

`blockhash` : The hash of the block that this transaction is included in. Blank if not yet included in a block.

`confirmations` : The number of confirmations that this transaction obtained by the network.

`genesis` : The profile username hash.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : the hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature`: The signature hash.

`contracts` : The array of contracts bound to this transaction and their details with opcodes.  
{  
`id` : The sequential ID of this contract within the transaction.

`OP` : The contract operation. Can be APPEND, CLAIM, COINBASE, CREATE, CREDIT, DEBIT, FEE, GENESIS, LEGACY, TRANSFER, TRUST, STAKE, UNSTAKE, WRITE.

`for` : For CREDIT transactions, the contract that this credit was created for . Can be COINBASE, DEBIT or LEGACY.

`txid` : The transaction that was credited / claimed.

`contract` : The ID of the contract within the transaction that was credited / claimed.

`proof` : The register address proving the credit.

`from` : For DEBIT, CREDIT, FEE transactions, the register address of the account that the debit is being made from.

`from_name` : For DEBIT, CREDIT, FEE transactions, the name of the account that the debit is being made from. Only included if the name can be resolved.

`to` : For DEBIT and CREDIT transactions, the register address of the recipient account.

`to_name` : For DEBIT and CREDIT transactions, the name of the recipient account. Only included if the name can be resolved.

`amount` : the token amount of the transaction.

`token` : the register address of the token that the transaction relates to. Set to 0 for NXS transactions.

`ticker` : The name of the token that the transaction relates to.

`reference` : For DEBIT and CREDIT transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number.  
}

-----------------------------------

# <a name="list/transactions"></a> `list/transactions`

Retrieves a list of transactions from the mempool.

```
ledger/list/transactions
```

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

`verbose` : Optional, determines how much **transaction data** to include in the response. Supported values are:

- summary : hash, type, version, sequence, timestamp, and contracts.

- detail : genesis, nexthash, prevhash, pubkey and signature.

`where` : An array of clauses to **filter** the JSON results. More information on filtering the results from /list/xxx API methods can be found at [`Queries`](/docs/API/QUERIES.md).

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 


### Returns:

#### Return value JSON object:

```
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

#### Return values:

`txid` : The transaction hash.

`type` : The description of the transaction (legacy | tritium base | trust | genesis | user).

`version` : The serialization version of the transaction.

`sequence` : The sequence number of this transaction within the signature chain.

`timestamp` : The Unix timestamp of when the transaction was created.

`blockhash` : The hash of the block that this transaction is included in. Blank if not yet included in a block.

`confirmations` : The number of confirmations that this transaction obtained by the network.

`genesis` : The profile username hash.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : the hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature`: The signature hash.

`contracts` : The array of contracts bound to this transaction and their details with opcodes.  
{  
`id` : The sequential ID of this contract within the transaction.

`OP` : The contract operation. Can be APPEND, CLAIM, COINBASE, CREATE, CREDIT, DEBIT, FEE, GENESIS, LEGACY, TRANSFER, TRUST, STAKE, UNSTAKE, WRITE.

`for` : For CREDIT transactions, the contract that this credit was created for . Can be COINBASE, DEBIT or LEGACY.

`txid` : The transaction that was credited / claimed.

`contract` : The ID of the contract within the transaction that was credited / claimed.

`proof` : The register address proving the credit.

`from` : For DEBIT, CREDIT, FEE transactions, the register address of the account that the debit is being made from.

`from_name` : For DEBIT, CREDIT, FEE transactions, the name of the account that the debit is being made from. Only included if the name can be resolved.

`to` : For DEBIT and CREDIT transactions, the register address of the recipient account.

`to_name` : For DEBIT and CREDIT transactions, the name of the recipient account. Only included if the name can be resolved.

`amount` : the token amount of the transaction.

`token` : the register address of the token that the transaction relates to. Set to 0 for NXS transactions.

`ticker` : The name of the token that the transaction relates to.

`reference` : For DEBIT and CREDIT transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number.  
}

-----------------------------------

# <a name="submit/transaction"></a> `submit/transaction`

Submits a transaction to be included in the mempool and broadcast to the network.

```
ledger/submit/transaction
```

### Parameters:

`data` : The serialized, hex-encoded transaction data to be submitted.

### Results:

#### Return value JSON object:

{
    "hash": "47959e245f45aab773c0ce5320a5454f49ac15f63e15acb36855ac654d54d6314fe36b61dd64ec7a9a546bcc439a628e9badcdccb6e5f8072d04a0a3b67f8679"
}

#### Return values:

`hash` : The tranaction hash, if successfully committed to the mempool / broadcast.

-----------------------------------

# <a name="get/info"></a> `get/info`

Retrieves mining and NXS supply related data for the ledger.

```
ledger/get/info
```

### Parameters:

[`Filtering`](/docs/API/FILTERING.MD). 

### Returns:

#### Return value JSON object:

```
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

#### Return values:

`Stake` :  Retreives information for the stake channel.  
{  
`height` : The current number of blocks for the stake channel.

`weight` : The total work completed for the stake channel.

`timespan` : It is the average block time for the stake channel.

`fees` : It is the total NXS accumulated on the stake channel.

`difficulty` : The current difficulty of the stake channel.   
}  

`prime` :  Retreives information for the prime channel.  
{  
`height` : The current number of blocks for the prime channel.

`weight` : The total work completed for prime channel.

`timespan` : It is the average block time for the prime channel.

`fees` : It is the total NXS accumulated on the prime channel.

`difficulty` : The current difficulty of the prime channel.

`reserve` : The amount of NXS in the reserve balance for the prime channel.

`reward` : The block reward for the next prime block to be found.

`hashes` : It is total network hash rate for prime channel.  
}  

`hash` :  Retreives information for the hash channel.    
{    
`height` : The current number of blocks for the hash channel.

`weight` : The total work completed for hash channel.

`timespan` : It is the average block time for the hash channel.

`fees` : It is the total NXS accumulated on the hash channel.

`difficulty` : The current difficulty of the hash channel.

`reserve` : The amount of NXS in the reserve balance for the hash channel.

`reward` : The block reward for the next hash block to be found.

`hashes` : It is total network hash rate for the hash channel.  
}  

`supply` : Metrics on NXS supply rates.

{  
`total` : Total amount of NXS in existence.
   
`target` : The target supply rate for this point in time.
   
`inflationrate` : The current inflation rate percentage based on the last year of data.
   
`minute` : The increase in supply rate within the last minute.
   
`hour` : The increase in supply rate within the last hour.
   
`day` : The increase in supply rate within the last day.
   
`week` : The increase in supply rate within the last week.
   
`month` : The increase in supply rate within the last 30 days.  
}  

`height` : The block height when the information is retrieved.

`timestamp` :  The Unix timestamp of the most recent block in the blockchain.

`checkpoint` : It is the most recent checkpoint that blocks can’t be reorganized before, so everything before that hash is finalized.

-----------------------------------

# <a name="get/metrics"></a> `get/metrics`

Retrieves mining related data for the ledger.

```
ledger/get/metrics
```

### Parameters:

[`Filtering`](/docs/API/FILTERING.MD). 

### Returns:

#### Return value JSON object:

```
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
#### Return Values:

`volumes` : This retrieves the transactions, contracts and accounts volume.    
{    
`transactions` :  Statistics for transactions for a specified period.    
{  
`daily` : Total number of transactions in the last 24 hours.  

`weekly` : Total number of transactions in the last 7 days.   
    
`monthly` : Total number of transactions in the last month.  
}  

`contracts` :  Statistics for contracts for a specified period.    
{  
`daily` : Total number of contracts in the last 24 hours.  

`weekly` : Total number of contracts in the last 7 days.   
    
`monthly` : Total number of contracts in the last month.  
}  

`accounts` :  Statistics for accounts (profiles) created in a specified period.    
{
`daily` : Total number of accounts created in the last 24 hours.  

`weekly` : Total number of accounts created in the last 7 days.   
    
`monthly` : Total number of accounts created in the last month.  
}  
}  

`exchanges` : Statistics of NXS deposits and withdrawals from exchanges.      
{      
`deposits` :  Statistics for NXS deposited to exchanges (moved to legacy from tritium).    
{  
`daily` : Total number of NXS deposited in the last 24 hours.  

`weekly` : Total number of NXS deposited in the last 7 days.   
    
`monthly` : Total number of NXS deposited in the last month.  
}  

`withdraws` :  Statistics for NXS withdraws from exchanges (moved to tritium from legacy).    
{  
`daily` : Total number of NXS withdraws in the last 24 hours.  

`weekly` : Total number of NXS withdraws in the last 7 days.   
    
`monthly` : Total number of NXS withdraws in the last month.  
}  
}  

`Network` : Statistics of NXS minted by the staking, mining channels and NXS moved to /f rom stake.      
{      
`mint` : This retreives the statistics for the NXS minted on staking and mining channels.  
{  

`staking` : Statistics for NXS minted from staking for a specified period.  
{  
`daily` : Total number of NXS minted from staking in the last 24 hours.  

`weekly` : Total number of NXS minted from staking in the last 7 days.   
    
`monthly` : Total number of NXS minted from staking in the last month.  
}  

`mining` :  Statistics for NXS minted from mining for a specified period.  
{  
`daily` : Total number of NXS minted from mining in the last 24 hours.  

`weekly` : Total number of NXS minted from mining in the last 7 days.   
    
`monthly` : Total number of NXS minted from mining in the last month.  
}  
}  

`stake` :  Statistics for NXS locked or unlocked from stake (trust accounts) for a specified period.  
{  
`daily` : Total number of NXS locked or unlocked from stake in the last 24 hours.  

`weekly` : Total number of NXS locked or unlocked from stake in the last 7 days.   
    
`monthly` : Total number of NXS locked or unlocked from stake in the last month.    
}    
}    

-----------------------------------

# <a name="recent/blocks"></a> `recent/blocks`

Retrieves an array of recent block data, starting from the most recent block and working backwards.

```
ledger/recent/blocks
```

### Parameters:

`limit` : Optional, the number of blocks to return. Default is 10, maximum is 1440. Only returns blocks from the last 28 days.

`verbose` : Optional, determines how much **transaction data** to include in the response. Default is 1 (hash only). Supported values are:
- `default` : hash.

- `summary` : type, version, sequence, timestamp, and operation.

- `detail` : genesis, nexthash, prevhash, pubkey and signature.

[`Filtering`](/docs/API/FILTERING.MD). 

### Returns:

#### Return value JSON object:

```
[
    {
        "hash": "5c985f5402f0bcb339ebef0e2edcad30427bf79803994772cd3a1458012748cd1d9d2ee8d6f1466f0c8ca867e1d8a2d9cfe6bc8cfdd885421fb56c55bfc11328f19c61bdc87193831851526a835e457fb46fcd0088879d564e88bb39524aa40489329ecc2cbc6a0453b8dd69a90b82232df6ee6d39ddec80e16149c99b1ddeab",
        "proofhash": "...",
        "size": 807,
        "height": 4555101,
        "channel": 1,
        "version": 8,
        "merkleroot": "...",
        "timestamp": 1658817563,
        "date": "2022-07-26 06:39:23 UTC",
        "nonce": 15731843013639865474,
        "bits": "04c23531",
        "difficulty": 7.9836465,
        "mint": 1.92113,
        "previousblockhash": "...",
        "nextblockhash": "...",
        "tx": [...]
    }
]
```

#### Return values:

`hash` : The hash of the block.

`proofhash` : The proof hash of the block.

`size` : The block size in bytes.

`height` : The block height.

`channel` : The block channel (0=Stake, 1=Prime, 2=Hash).

`version` : The serialization version of this block.

`merkleroot` : The hash merkle root of the block transactions.

`timestamp` : The Unix timestamp of when the block was created.

`date` : The UTC date/time when the block was created.

`nonce` : The solution nonce.

`bits` : The compact representation for the block difficulty.

`difficulty` : The channel specific difficulty.

`mint` : The value minted in this block.

`previousblockhash` : The hash of the previous block in the chain.

`nextblockhash` : The hash of the next block in the chain, unless this is the last block in the chain.

`tx` : An array of transactions included in this block (detail depends on verbose level).

-----------------------------------

# <a name="sync/status"></a> `sync/status`

Returns the current synchronization status of the node.

```
ledger/sync/status
```

### Parameters:

None.

### Returns:

#### Return value JSON object:

```
{
    "networkBlock": 4595000,
    "downloadRate": 125,
    "completed": 99.85,
    "progress": 95.5,
    "secondsRemaining": 3600,
    "timeRemaining": "01:00:00"
}
```

#### Return values:

`networkBlock` : The highest block height known from connected peers.

`downloadRate` : The number of blocks downloaded per second.

`completed` : The overall percentage of the blockchain synchronized (0-100).

`progress` : The percentage progress of the current sync session (0-100).

`secondsRemaining` : The estimated number of seconds remaining until sync is complete.

`timeRemaining` : The estimated time remaining in HH:MM:SS format.

-----------------------------------
