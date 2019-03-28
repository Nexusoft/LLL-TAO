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
    Determines how much transaction data to include in the response, Optional (default 1). 
    0: no transaction data
    1: hash 
    2: type, version, sequence, timestamp, and operation.
    3: genesis, nexthash, and prevhash.
    4: pubkey and signature.
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

### GetBlocks

Retrieves an array of block data for a sequential range of blocks from a given hash or height.

```
<method>:
    /ledger/getblocks

<key>=<value> parameter pairs:
    hash=<hash>
    height=<height>
    count=<count>
    txverbose<txverbose-level>
    
Input parameter description:
<hash>:
    The block hash to retrieve the block data for.
<height>:
    The block height to retrieve the block data for.
<count>
    The number of blocks to return data for.  Optional (default 10), must be between 1 and 1000
<txverbose-level>
    Determines how much transaction data to include in the response, Optional (default 1). 
    0: no transaction data
    1: hash 
    2: type, version, sequence, timestamp, and operation.
    3: genesis, nexthash, and prevhash.
    4: pubkey and signature.
```

Note: Either the hash or the height needs to be supplied, but not both.  
Retrieving block data by height is only allowed if the daemon has been configured with indexheight=1

Return value JSON object:
```    
[
    {
        "hash": "5534a161f52a2b4904c752fc487fca2ce18f27a25b2b7de9b894ac18a41a3b312a4a9b55204cf18b7b0ce0c55bc1d7a6decb0a4f3bad98f8274ae9e31d19a4862cf40daa0bc8bf567f14bcce43555c4a7ac9a7445647e9cb9f95ef759a7737917ece8d61306da38fd7d9ae45909de7ed96e82caa8af3f7ab6758fe539ac4d12f",
        "proofhash": "500344c3287f8082802cbf21dbd31c4ef7f1f626046ab2b1e5f4db7dbf344cfbf4681c757f9e4c67ce5efde5bfa34d5b58b3d0c9456e9166c733cc37b4a2f5d5764ba122252f5e6efa9a233bb37f7b1528b46c56b7f17e24516a86a378738ea8dbc7d85b41e48705c827538f86dad226fda5257077542f2a9fe343a986b52e1f",
        "size": 727,
        "height": 1,
        "channel": 1,
        "version": 6,
        "merkleroot": "3328b8e25d45904bb26ad1298260eda220417859bca8b7b1e123817518b8aecc8ea89404900c0ac6941139b5403579e705677bf9e2bd2f131891c6348f667d1b",
        "time": "2019-03-22 03:58:09 UTC",
        "nonce": 11636693071151760,
        "bits": "017d7840",
        "difficulty": 2.5,
        "mint": 0.0,
        "previousblockhash": "00002a0ccd35f2e1e9e1c08f5a2109a82834606b121aca891d5862ba12c6987d55d1e789024fcd5b9adaf07f5445d24e78604ea136a0654497ed3db0958d63e72f146fae2794e86323126b8c3d8037b193ce531c909e5222d090099b4d574782d15c135ddd99d183ec14288437563e8a6392f70259e761e62d2ea228977dd2f7",
        "nextblockhash": "c4603451343ea95bfd543074751200bab6b55c7d27495eab25286d441edb112ea7fc6883a86f538218bba8fabf20eb30557fc6a73e9929059d20e7f74676ce1f700420b951832116e7420b4f5b8863b1ffc09810b2b123e465637d136d130033c6bc3b2fee701f25e980e4dee2711a2aca248ad529368dacb220968951cbd2a3",
        "tx": [
            {
                "hash": "3328b8e25d45904bb26ad1298260eda220417859bca8b7b1e123817518b8aecc8ea89404900c0ac6941139b5403579e705677bf9e2bd2f131891c6348f667d1b"
            }
        ]
    },
    {
        "hash": "c4603451343ea95bfd543074751200bab6b55c7d27495eab25286d441edb112ea7fc6883a86f538218bba8fabf20eb30557fc6a73e9929059d20e7f74676ce1f700420b951832116e7420b4f5b8863b1ffc09810b2b123e465637d136d130033c6bc3b2fee701f25e980e4dee2711a2aca248ad529368dacb220968951cbd2a3",
        "proofhash": "61090468e7369af7013a46c4532013d21dc0e80401da19b723ceb7ef2f331c0864fba8c0dff2907d732b9f867af8fead3aabdf8ea8072a512e4a208c63f6b08c5d83a7855d3d6cf3f5ec6aad080f246f1c140710de2ecf614afe8bcd0b973f8e0d89830c4d9d66d4409b6e46819d337a2402d6eed2c664d38475d8a09993773b",
        "size": 727,
        "height": 2,
        "channel": 1,
        "version": 6,
        "merkleroot": "b3e587c3299b8f4b9c65cd221e7ad3afe579a1d01b0acd935c9bbd2e35d102f9f2023bfc7bcba1f840a3b9725e651c26dc516e53c63eff7e37206ed94b8f746f",
        "time": "2019-03-22 03:58:17 UTC",
        "nonce": 143375589399502436,
        "bits": "017d7840",
        "difficulty": 2.5,
        "mint": 0.0,
        "previousblockhash": "5534a161f52a2b4904c752fc487fca2ce18f27a25b2b7de9b894ac18a41a3b312a4a9b55204cf18b7b0ce0c55bc1d7a6decb0a4f3bad98f8274ae9e31d19a4862cf40daa0bc8bf567f14bcce43555c4a7ac9a7445647e9cb9f95ef759a7737917ece8d61306da38fd7d9ae45909de7ed96e82caa8af3f7ab6758fe539ac4d12f",
        "nextblockhash": "2e9176b4a8462244fafca6fa91d7549257d2513207272890b3c6875443abaa24cca19ddb5acaa234b0b70e3d226b37599e1a2a6950236dcb81871e4b685559e7f977e413cb2fbcf8589f62ebcd97471d377e22e4ad301ce7e9fd0049459accebf97813e669eda5e0fc33ad2ec1a77c104cd713112de09fc2f2c19b9b14306ae8",
        "tx": [
            {
                "hash": "b3e587c3299b8f4b9c65cd221e7ad3afe579a1d01b0acd935c9bbd2e35d102f9f2023bfc7bcba1f840a3b9725e651c26dc516e53c63eff7e37206ed94b8f746f"
            }
        ]
    }
]

    

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