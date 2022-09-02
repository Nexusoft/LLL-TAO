# Assets API
----------------------------------
An asset is a user-defined data structure that is stored in an object register, owned by a profile. Assets can hold one or more pieces of data and users can define the fields (name, data, type, mutability) that data is stored in. The Assets API supports the `readonly`, `raw`, `basic` and `JSON` formats and the user is required to specify the format.

The `readonly` and `raw` formats are useful when developers wish to store arbitrary data, without incurring the overhead of defining an object. The value will be provided with the `data` parameter. The `readonly` format cannot be updated and is stored in a readonly register. The `raw` format is stored in a raw register and allows the data to be updated.

The `basic` format allows callers to define an asset in terms of simple 'key=value' pairs. It assumes all values are stored using the string data type. The 'key=value' pairs can be updated.

The `JSON` format allows callers to provide a detailed definition of the supply data structure, with each field defined with a specific data type and mutability. Items defined with one of the complex formats can be updated after their initial creation.  

The assets API also provides a history of changes to the values, as well as the history of ownership of the item.

The full supported endpoint of the finance URI is as follows:

```
assets/verb/noun/filter/operator
```

The minimum required components of the URI are:

```
assets/verb/noun
```

## `Supported Verbs`

The following verbs are currently supported by this API command-set:

[`create`](#create) - Generate a new object of supported type.  
[`get`](#get) - Get object of supported type.  
[`list`](#list) - List all objects owned by given user.  
[`update`](#update) - Update a specified object.  
[`transfer`](#transfer) - Transfer a specified object register.  
[`claim`](#claim) - Claim ownership of an object register.  
[`tokenize`](#tokenize) - To represent ownership of an asset object with a token object.  
[`transactions`](#transactions) - List all transactions that modified specified object.  
[`history`](#history) - Generate the history of all last states.  

## `Supported Nouns`

The following nouns are supported for this API command-set:

[`asset`] - An object register containing an asset object.  
[`raw`] - An object register containing a raw asset object.  
[`readonly`] - An object register containing a read-only asset object.  
[`any`] - An object selection noun allowing mixed assets.  
[`schema`] - An object register containing a user-defined asset object. 

## `Direct Endpoints`

The following commands are direct endpoints and thus do not support the above `verb` and `noun` structure available above.

[`list/partial`](#list/partial) - List users partial ownership details for tokenized assets.      

-----------------------------------

# <a name="create"></a> `create`

Create a new object register specified by given noun.

```
assets/create/noun
```

This command supports the `asset`, `readonly`, `raw`,  and `any` nouns.

#### create/asset

Creates a new asset in an object register.  

#### create/readonly

Creates a new asset in an readonly register.  

#### create/raw

Creates a new asset in a raw register.  

#### create/any

Creates a new asset specified by the format parameter.  

### Parameters:

`pin` : Required if **locked**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Optional for **noun** `name` as a *UTF-8* encoded string that will generate a name object register that points to new object.

`format` : Required to **identify** the format used to define the asset. Values can be `readonly`, `raw`, `basic` and `JSON`.

`data` : Required if **format** is `readonly` or `raw`, allows caller to add arbitrary data to object. `readonly` assets cannot be updated  All other preceding fields are ignored.

`<fieldname>=<value>` : Optional for **format** `basic`, The caller can provide 'key=value' pairs for each piece of data to store in the asset.

`json` : If **format** is `JSON`, then this field will hold the json definition of the asset as a JSON array of objects representing each field in the object. It uses the following format:

- `name` : The name of the data field.

- `type` : The data type to use for this field. Values can be uint8, uint16, uint32, uint64, uint256, uint512, uint1024, string, or bytes.

- `value` : The default value of the field.

- `mutable` : The boolean field to indicate whether the field is writable (true) or 
read-only (false).

- `maxlength`: Only applicable to string or bytes fields where mutable=true, this is the maximum number of characters (bytes) that can be stored in the field. If no maxlength parameter is provided then we will default the field size to the length of the default value rounded up to the nearest 64 bytes.

**NOTE:** 
- There is a limit of 1KB for asset data to be saved in the register, excluding the asset name.
- There is a fee of 1 NXS for creating and an asset and an additional 1 NXS for the optional name object.

### Results:

#### Return value JSON object:

```
{
    "success": true,
    "address": "87VmNhitFJv3WA3Yrovt9A3hts2MoXcfExyy9LiXyhK1sdThwYM",
    "txid": "01230bbc8f0d72aaaff13471e34520d17902290de9a1e5ce1f320f0883024e2d96e21b59a3e48b8d2b5ba2874e93d5d3b4f412e06dc92440c2e12682c958fe34"
}
[Completed in 4998.584942 ms]
```

#### Return Values:

`success` : Boolean flag indicating that the asset was saved successfully.

`address` : The register address for this account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.

`txid` : The hash of the transaction that was generated for this tx. If using -autotx this field will be ommitted.

-----------------------------------

# <a name="get"></a> `get`

Retrieves information for an asset specified by the noun.

```
assets/get/noun
```

This command supports the `readonly`, `raw`, and `asset`  nouns.

#### get/asset
Get asset for register type object.

#### get/readonly
Get asset for register type readonly.

#### get/raw
Get asset for register type raw.

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the name of the asset. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the asset. This is optional if the `name` is provided.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD). 

### Results:

#### Return value JSON object:

```
[
    {
        "owner": "b7392196b83aca438567558462cd0c5d982569c7cefa668500c4bf3e61a03b7a",
        "version": 1,
        "created": 1655279431,
        "modified": 1655279431,
        "type": "OBJECT",
        "Location": "Meta World ",
        "Owner Name": "John Doe",
        "Registration Details": "MRG/05/478564",
        "address": "87Wai2JoS4hNAEVXZVmejLS6pK21XQWKoLAkaep5aXFdrYnJJyk",
        "name": "local:Meta_World"
    }
]
[Completed in 2.838543 ms]
```

#### Return Values:

`owner` : The username hash of the owners profile.

`version` : The serialization version of the transaction.

`created` : The UNIX timestamp when the asset was created.

`modified` : The UNIX timestamp when the asset was last modified.

`type` : Asset register type. Can be OBJECT, RAW or READONLY.

`data` : The data stored in a raw or readonly asset object.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the asset.

`address` : The register address of the asset.

`name` : The name identifying the asset.

-----------------------------------

# <a name="list"></a> `list`

Retrieves a list of all the asset objects owned by the profile specified by the noun.

```
assets/list/noun
```

This command supports the `asset`, `readonly`, `raw`, and `any` nouns.

#### list/assets

Lists all the assets for object register type.

#### list/readonly

Lists all assets for readonly register type. 

#### list/raw

Lists all assets for raw register type.

#### list/any

Lists all assets for all supported register types.

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

`where` : An array of clauses to **filter** the JSON results. More information on filtering the results from /list/xxx API methods can be found at [`Queries`](/docs/API/QUERIES.md).

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 

### Results:

#### Return value JSON object:

```
[
    {
        "owner": "b7392196b83aca438567558462cd0c5d982569c7cefa668500c4bf3e61a03b7a",
        "version": 1,
        "created": 1655279431,
        "modified": 1655279431,
        "type": "OBJECT",
        "Location": "Margoa",
        "Owner Name": "Ageon",
        "Registration Details": "MRG/05/478564",
        "address": "87Wai2JoS4hNAEVXZVmejLS6pK21XQWKoLAkaep5aXFdrYnJJyk",
        "name": "local:Asset2"
    }
]
[Completed in 2.838543 ms]
```

#### Return Values:

`owner` : The username hash of the owners profile.

`version` : The serialization version of the transaction.

`created` : The UNIX timestamp when the asset was created.

`modified` : The UNIX timestamp when the asset was last modified.

`type` : Asset register type. Can be OBJECT, RAW or READONLY.

`data` : The data stored in a raw or readonly asset object.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the asset.

`address` : The register address of the asset.

`name` : The name identifying the asset.
-----------------------------------

# <a name="update"></a> `update`

This method provides the user with the ability to update the object data specified by the noun.

```
assets/update/noun
```

This command only supports the `asset` and `raw` nouns.

#### update/asset

This updates the data values for asset with the format basic and JSON.

#### update/raw

This updates the data values for the raw item register.

### Parameters:

`pin` : Required if **locked**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the name of the asset. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided.

`address` : Required to **identify** the register address of the asset. This is optional if the `name` is provided.

`format` : Required to **specify** the format of the asset to update. Values can be `readonly`, `raw`, `basic` and `JSON`. 

`data` : Required if **format** is `readonly` or `raw`, allows caller to update the data object.  

`<fieldname>=<value>` : Optional for **format** `basic`, The caller can provide 'key=value' pairs to update each piece of data for the asset.

### Results:

#### Return value JSON object:

```
{
    "success": true,
    "txid": "01947f824e9b117d618ed49a7dd84f0e7c4bb0896e40d0a95e04e27917e6ecb6b9a5ccfba7d0d5c308b684b95e98ada4f39bbac84db75e7300a09befd1ac0999"
}
[Completed in 18533.182336 ms]
```

#### Return values:

`success` : Boolean flag indicating that the asset was saved successfully.

`txid` : The hash of the transaction that was generated for this tx. If using -autotx this field will be ommitted.

-----------------------------------

# <a name="transfer"></a> `transfer`

This will initiate ownership transfer of the specified noun.

```
assets/transfer/noun
```

This command supports the `asset`,  `raw` and `readonly` nouns.

#### transfer/asset

This initiates transfer for an asset to a recipient.

#### transfer/raw

This initiates transfer for a raw asset to a recipient.

#### transfer/readonly

This initiates transfer for a readonly asset to a recipient.


### Parameters:

`pin` : Required if **locked**. The `PIN`for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the name of the asset. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the asset. This is optional if the `name` is provided.

`recipient` : Required to **identify** the recipient account. This can be the profile username or genesis hash.

`expires` : Optional field allows callers to specify an **expiration** for the transfer transaction. The expires value is the number of seconds from the transaction creation time after which the transaction can no longer be claimed by the recipient. Conversely, when you apply an expiration to a transaction, you are unable to void the transaction until after the expiration time. If expires is set to 0, the transaction will never expire, making the sender unable to ever void the transaction. If omitted, a default expiration of 7 days (604800 seconds) is applied.

### Results:

#### Return value JSON object:

```
{
    "success": true,
    "address": "87VmNhitFJv3WA3Yrovt9A3hts2MoXcfExyy9LiXyhK1sdThwYM",
    "txid": "01bc48f80792fd4b97d43555d5993f0edfb0998ab14bcf159404f52e34a64abd6769f61014f006703100aa040a27aca65227133b2b5a71617efac3bc5640e361"
}
[Completed in 4998.999748 ms]
```

#### Return values:

`success` : Boolean flag indicating that the asset transfer was successful.

`address` : The register address for this asset.

`txid` : The ID (hash) of the transaction that includes the asset transfer.

-----------------------------------

# <a name="claim"></a> `claim`

This method will claim ownership of the asset to complete the corresponding transfer transaction.

```
assets/claim/noun
```

This command supports the `asset`,  `raw` and `readonly` nouns.

#### claim/asset

Assets that have been transferred need to be claimed by the recipient. This method creates the claim transaction.

#### claim/raw

Raw assets that have been transferred need to be claimed by the recipient. This method creates the claim transaction.

#### claim/readonly

Readonly assets that have been transferred need to be claimed by the recipient. This method creates the claim transaction.

### Parameters:

`pin` : Required if **locked**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`txid` : Required the **transaction ID** (hash) of the asset transfer transaction for which is being claimed.

`name` : Optional field allows the user to **rename** an item when it is claimed. By default the name is copied from the previous owner and a Name record is created for the item in your user namespace. If you already have an object for this name then you will need to provide a new name in order for the claim to succeed.

### Results:

#### Return value JSON object:

```
{
    "success": true,
    "txid": "01f35304d41d00b002ca02d3bd9cf6cfeb134f5c454d4b6f5a355e35ffc557cfb3756834f3cf5cbeaf98da6773adcaaeca80154d15e449d4876ddf35c0b895cf"
}
[Completed in 4978.949420 ms]
```

#### Return values:

`success` : Boolean flag indicating that the asset claim was successful.

`txid` : The ID (hash) of the transaction that includes the claimed asset.

-----------------------------------

# <a name="tokenize"></a> `tokenize`

This method will tokenize an asset into fungible tokens that represent ownership

```
assets/tokenize/noun
```

This command only supports the `asset` noun.

### Parameters:

`pin` : Required if **locked**. The `PIN `for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the name of the asset to be tokenised. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the asset to be tokenised. This is optional if the `name` is provided.

`token` : Required to **identify** the token to tokenize the asset. It can be the name (for global names) username:tokenname ( for local names) or the register address of the token.

**NOTE:** Create the token before using tokenize API.


### Results:

#### Return value JSON object:

```
{
    "success": true,
    "txid": "01f35304d41d00b002ca02d3bd9cf6cfeb134f5c454d4b6f5a355e35ffc557cfb3756834f3cf5cbeaf98da6773adcaaeca80154d15e449d4876ddf35c0b895cf"
}
[Completed in 4978.949420 ms]
```

#### Return values:

`success` : Boolean flag indicating that the asset tokenization  was successful.

`txid` : The ID (hash) of the transaction that includes the claimed asset.

-----------------------------------

# <a name="history"></a> `history`

This will get the history of changes to an asset, including both the data and it's ownership.

```
assets/history/noun
```

This command supports the `asset`,  `raw`, `readonly` and `any` nouns.

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the name of the asset. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the asset. This is optional if the `name` is provided.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 

### Results:

#### Return value JSON object:

```
[
    {
        "owner": "b7a57ddfb001d5d83ab5b25c0eaa0521e6b367784a30025114d07c444aa455c0",
        "version": 1,
        "created": 1656614448,
        "modified": 1656614448,
        "type": "OBJECT",
        "Assetname": "Bugatti Veyron",
        "Chassis_No": "BV45648784634546",
        "Purchased Date": "22/06/2022",
        "Engine_No": "BVE54864660",
        "Owner": "John Doe",
        "Registration_No": "DL01EK0001",
        "address": "88NcYcKtMTRwtwDgfXFkZ4TbrHvkRGzsQkqVZco77Hqx1WRgCyi",
        "name": "local:Asset0001",
        "action": "CREATE"
    }
]
[Completed in 0.870644 ms]
```

#### Return values:

The return value is a JSON array of objects for each entry in the names history:

`owner` : The username hash of the owners profile.

`version` : The serialization version of the transaction.

`created` : The UNIX timestamp when the account was created.

`modified` : The UNIX timestamp of when the name was updated.

`type` : The type of register. Can be OBJECT, RAW or READONLY.

`<fieldname>=<value>` : If format is basic, then the caller can provide additional = pairs for each piece of data to store in the asset.

`data` : The data stored in a raw or readonly asset object.

`address` : The register address of the name object.

`name` : The name of the object.

`action` : The action that occurred - CREATE | MODIFY | TRANSFER | CLAIM.

-----------------------------------

# <a name="transactions"></a> `transactions`

This will list off all of the transactions for the specified noun.

```
assets/transactions/noun
```

This command supports all nouns.

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the asset. This is optional if the `address` is provided.

`address` : Required to **identify** the register address of the asset. This is optional if the `name` is provided.

`verbose` : Optional, determines how much transaction data to include in the response. Supported values are :
- `default` : summary.

- `summary` : type, version, sequence, timestamp, blockhash, confirmations and contracts.

- `detail` : All in summary + genesis, nexthash, prevhash, pubkey and signature.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 


### Results:

#### Return value JSON object:

```
[
    {
        "txid": "0167d11ebea68f68e2c6591a9d94c236a60c312cf70d596492002a3e4d9a95a4f4e859c498fa6ac17c56cb4db1043abacdfd44f669112641fc4add15bb3efefa",
        "type": "tritium user",
        "version": 4,
        "sequence": 4,
        "timestamp": 1656614805,
        "blockhash": "9b2c063b2e8766ce9b55c5b75524b13c12c7006e0c67423dbfe1539ed4f62cd0c764f63fbbc6cb41ab9fc785ce6f88ae92e6a82e3151c4c956fa65eac524db3e1922bdced7bdd4cd97a4563dd21f011a310d062440300b2a7532fe21f6df55c7e3ba9046a982b99af97acddd768569d3cf5368cb91dcb0138cf5af7f1a6f391b",
        "confirmations": 95,
        "contracts": [
            {
                "id": 0,
                "OP": "TRANSFER",
                "address": "88NcYcKtMTRwtwDgfXFkZ4TbrHvkRGzsQkqVZco77Hqx1WRgCyi",
                "recipient": "8Dnn5LUeffcRCqZLpboCHnzw5AkfwzkM5hjVScjSDHVKhFpVzCB",
                "tokenized": true
            }
        ]
    },
    {
        "txid": "01e4eff49cadbd887511c6302d5fe8e89ead4caa87bcec2dfd55e18cb201a8fc8db9726571d3bebe554a81f579b22e2794a0e86756be326f362964e95ab28f23",
        "type": "tritium user",
        "version": 4,
        "sequence": 2,
        "timestamp": 1656614448,
        "blockhash": "e9b46f5dcf3c321f8a2dc672a6c47a5c6b763bc9d1f001722bc446b186afda3566f04cc739253696a3f232e036bc273ca68c18d84213186a1317d246ec86a165ba152838a2eca4425c19472dc8fb1b762f4413c5a633c4fef5494378b9858da34129dcd5bc556110c1e79aa42f01f8bae6f4938a6e87a63b6d30e2ee6025d507",
        "confirmations": 97,
        "contracts": [
            {
                "id": 0,
                "OP": "CREATE",
                "address": "88NcYcKtMTRwtwDgfXFkZ4TbrHvkRGzsQkqVZco77Hqx1WRgCyi",
                "type": "OBJECT",
                "standard": "NONSTANDARD",
                "object": {
                    "Assetname": " Bugatti Veyron",
                    "Chassis_No": "BV45648784634546",
                    "Purchased Date": "22/06/2022",
                    "Engine_No": "BVE54864660",
                    "Owner": "John Doe",
                    "Registration_No": "DL01EK0001"
                }
            }
        ]
    }
]
[Completed in 3.128134 ms]
```

#### Return values:

`txid` : The transaction hash.

`type` : The description of the transaction (legacy | tritium base | trust | genesis | user).

`version` : The serialization version of the transaction.

`sequence` : The sequence number of this transaction within the signature chain.

`timestamp` : The Unix timestamp of when the transaction was created.

`blockhash` : The hash of the block that this transaction is included in. Blank if not yet included in a block.

`confirmations` : The number of confirmations that this transaction obtained by the network.

`genesis` : The signature chain genesis hash.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : the hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature` : The signature hash.

`contracts` : The array of contracts bound to this transaction and their details with opcodes.
{
`id` : The sequential ID of this contract within the transaction.

`OP` : The contract operation. Can be CREATE | MODIFY | TRANSFER | CLAIM.

`address` : The register address of the asset object.

`txid` : The transaction ID hash of the transaction for the credit / claim.

`contract` : The ID of the contract within the transaction for the credit / claim.

`type`: The type of register. Can be OBJECT, RAW or READONLY.

`standard` : The type of object. Can be NONSTANDARD or JSON.

`recipient` : The transfer recipients profile username hash.

`tokenized` : Boolean flag indicating if the asset is tokenized or not.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the asset.

`object` : The data stored in the asset.
}

-----------------------------------

# <a name="list/partial"></a> `list/partial`

Retrieves a list of all the partial ownership details of tokenized assets for the user. 

```
assets/list/partial
```

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.


### Results:

#### Return value JSON object:

```
[
    {
        "owner": "d751c1ddbd2a50a0bee9883f4f8e029272551e8d09b336aeca876e1270e91422",
        "version": 1,
        "created": 1656614448,
        "modified": 1656614805,
        "type": "OBJECT",
        "Assetname": "Block Incorporated",
        "Registration_No": "NY_CA_45648784634546",
        "Owner": "John Doe ",
        "address": "88NcYcKtMTRwtwDgfXFkZ4TbrHvkRGzsQkqVZco77Hqx1WRgCyi",
        "ownership": 12.0
    }
]
[Completed in 0.542454 ms]
```

#### Return Values:

`owner` : The username hash of the profile that owns this asset.

`version` : The serialization version of the transaction.

`created` : The UNIX timestamp when the asset was created.

`modified` : The UNIX timestamp when the asset was last modified.

`type` : Asset register type. Can be OBJECT, RAW or READONLY.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the asset.

`data` : The data stored in a raw or readonly asset object.

`address` : The register address of the asset.

`name` : The name identifying the asset.

`ownership` : This is the percentage of the asset owned by the caller, based on the number of tokens owned.

-----------------------------------