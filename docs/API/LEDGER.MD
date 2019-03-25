The Ledger API Interface Definition
-----------------------------------

### GetBlockHash

Retrieves the blockhash for the given height.

```
<method>:
    /ledger/getblockhash

<key>=<value> parameter pairs:
    height=<height>
    
Input parameter description:
<height>:
    The block height to retrieve the hash for.
```

Return value JSON object:
```    
{
    "hash"    : "<transaction-id>"
}    

Output parameter description:
<hash>:
    The hash of the block
```


### GetBlock

Retrieves block data for the given block hash or height.

```
<method>:
    /ledger/getblock

<key>=<value> parameter pairs:
    hash=<hash>
    height=<height>
    txverbose<txverbose-level>
    
Input parameter description:
<hash>:
    The block hash to retrieve the block data for.
<height>:
    The block height to retrieve the block data for.
<txverbose-level>
    Determines how much transaction data to include in the response 
    0: hash
    1: type, version, sequence, timestamp, and operation.
    2: genesis, nexthash, and prevhash.
    3: pubkey and signature.
```

Note: Either the hash or the height needs to be supplied, but not both.  
Retrieving block data by height is only allowed if the daemon has been configured with indexheight=1

Return value JSON object:
```    
{
    "hash": "677444ec303ad3a0f408725e287529348f21f8b94c15e742469c30d6b9cd8eb17e3c2f059de8c83a663fe753704c0ababf5599f319b2ee63c1ed243b3abc4820ae9960254ed095e09a7bcd11b10f225865e6da8f7ca696e0445dac822e2dcc9d3de15b1759daf084447abcb0cc8b04a1e06d27d876c0c85b53fe3428a4095621",
    "proofhash": "39e2015948c026622b9abfc2cfe5dc2f9784965fd9eca511d8792db37c272802a864e2319fdbfe845ec2b65a7b0dee1f6bf0e4a9da5e76875022c39f6c8252746875be31bbadbfffca7cf9297101313c50d064fec9443b3f3545cc3ff7813898da7f7f845dda937875e603b125fbd085b5b4a81ddf05e1fcd0cbdbac13788d85",
    "size": 727,
    "height": 7,
    "channel": 1,
    "version": 6,
    "merkleroot": "1c667a87798a57e4432774e746204f589cfd35877a38863b3e8230dde2852af6b14110c619b7e225371038751043d38b555da2f0c91e6351ed0286dbdcd520b5",
    "time": "2019-03-22 03:58:55 UTC",
    "nonce": 118903990760413130,
    "bits": "01871a07",
    "difficulty": 2.5631239,
    "mint": 76.499339,
    "previousblockhash": "c5aa84cde4a771e42dbbda542a34e82c12c7bde05d63ae7f5b1684f1ff8c2cf8f173f82a73c5c68c3d00d953bdcf74c205fb853d00584ec9f970e8cc35f43049757846f3833873eaceb2d7a76bc91187a2d6289c11ff3072b7bffed4f38c163cb82a1dd037c30ff8169c37ca1af23fe0f1e6022ce6b35508a1de38c108292cee",
    "nextblockhash": "d4078f2b2041aa8bd21f10c640fa57faee59c1545298ffe7360095721d8007a487bea0749b579cbee1cba24c5bbedd643c722d6f4e19b0a08eccc933399fb9950a5e689bd7886f40490a88480bd1b9272993ff522f12e78bf92b0947399f4cc57727dbfeb8d8a8f8e8695b05ef2aba94b39280f222c3e2e6a9904fba1a7626fb",
    "tx": [
        {
            "type": "tritium base",
            "version": 1,
            "sequence": 6,
            "timestamp": 1553227128,
            "genesis": "2aa5f479a8f121332eca4bb28d9d1beb13ea066f5efb90e8b6e6490fecb6b5ea",
            "nexthash": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9",
            "prevhash": "ef2526856b28b04121c5cf586423167521e1a6f50b464e321105e34f48af6adf6cd8bc72e3fd7ac14bfcf94b458db4745cba257bacf7d05da48a4f1cd5f8b205",
            "pubkey": "02323a1213c0dd330d459176fa493137aa5691d541537ee41e8101d794039d015e6ea8e4c8926134701614e4085b7964ab4070fa14acf577cc5b759a7df06c9c4b",
            "signature": "30818402402f2a5cf5c2d6c21c70ea4a0d25ac5eae006536823a74b0ca1c772aa9aa68afe7eef7512780b23feed43ae970daee8e22491e570980b5f0a766010b209026aaa6024008a5246199fd62d75a5b0a7bab49aa8a4ad10095bb09d8a46b307363940423a0b018ae5b5d9314e5b5f4a583d99f7343db97e2ae3b8f70a9322c4a27b98fd2a6",
            "hash": "1c667a87798a57e4432774e746204f589cfd35877a38863b3e8230dde2852af6b14110c619b7e225371038751043d38b555da2f0c91e6351ed0286dbdcd520b5",
            "operation": "Coinbase(nonce=3, amount=76499339)"
        }
    ]
}
    

Output parameter description:
<hash>:
    The hash of the block
<proofhash>
    The proof hash of the block
<size>
    Block size in bytes
<height>
    Block height
<channel>
    Channel (1=Prime, 2=Hash, 3=Stake)
<version>
    Serialization version of this block
<merkleroot>
    Merkle root of the block transactions
<time>
    Unified time the block was created
<nonce>
    The solution nonce 
<bits>
<difficulty>
    The channel specific difficulty 
<mint>
    The value minted in this block
<previousblockhash>
    Hash of the previous block in the chain
<nextblockhash>
    Hash of the next block in the chain, unless this is the last block in the chain. 
<tx>
    Array of transactions included in this block 
    <type>
        Description of the transaction (legacy|tritium base|trust|genesis|user)
    <version>
        Serialization version of the transaction
    <sequence>
        Sequence number of this transaction within the signature chain 
    <timestamp>
        Unix timestamp when the transaction was created
    <genesis>
        Signature chain genesis hash
    <nexthash>
        Hash of the next transaction in the sequence
    <prevhash>
        Hash of the previous transaction in the sequence
    <pubkey>
        Public key
    <signature>
        Signature hash
    <hash>
        Transaction hash
    <operation>
        Description of the opcodes included in the transaction

```

