# Supply API
-----------------------------------

The Supply API provides functionality to support the ownership transfer requirements typical of a supply chain process. Items in the supply chain can be given a value and this value can be updated over time. The supply API supports the `readonly`, `raw`, `basic` and `JSON` formats and the user is required to specify the format.

The `readonly` and `raw` formats are useful when developers wish to store arbitrary data, without incurring the overhead of defining an object. The value will be provided with the `data` parameter. The `readonly` format cannot be updated and is stored in a readonly register. The `raw` format is stored in a raw register and allows the data to be updated.

The `basic` format allows callers to define an asset in terms of simple key=value pairs. It assumes all values are stored using the string data type. The key=value pairs can be updated.
.
The `JSON` format allows callers to provide a detailed definition of the supply data structure, with each field defined with a specific data type and mutability. Items defined with one of the complex formats can be updated after their initial creation.

The supply API also provides a history of changes to the values, as well as the history of ownership of the item.

The full supported endpoint of the supply URI is as follows:

```
supply/verb/noun/filter/operator
```

The minimum required components of the URI are:

```
supply/verb/noun
```

## `Supported Verbs`

The following verbs are currently supported by this API command-set:

[`create`](#create) - Generate a new object of supported type.  
[`get`](#get) - Get object of supported type.  
[`list`](#list) - List all objects owned by given user.  
[`update`](#update) - Update a specified object.  
[`transfer`](#transfer) - Transfer a specified object register.  
[`claim`](#claim) - Claim ownership of an object register.  
[`history`](#history) - Generate the history of all last states.  
[`transactions`](#transactions) - List all transactions that modified specified object.  

## `Supported Nouns`

The following nouns are supported for this API command-set:

[item] - An object register containing an item object.  
[raw] - An object register containing raw object.  
[readonly] - An object register containing read-only object.  
[any] - An object selection noun allowing mixed items.  

-----------------------------------

# <a name="create"></a> `create`

Create a new object register specified by given noun.


```
supply/create/noun
```

This command supports the `item`, `raw`, `readonly` and `any` nouns.

#### create/item

Creates a new item on a object register.

#### create/raw

Creates a new item on a raw register.

#### create/readonly

Creates a new item on a readonly register.

#### create/any

Creates a new item specified by the format parameter.

### Parameters:

`pin` : Required if **locked**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Optional name to identify the asset. If provided a Name object will also be created in the users local namespace, allowing the asset to be accessed/retrieved by name. If no name is provided the asset will need to be accessed/retrieved by its 256-bit register address.

`format` : Required to **identify** the format used to define the asset. Values can be `readonly`, `raw`, `basic` and `JSON`.

`data` : Required if **format** is `readonly` or `raw`, then this field contains the hex-encoded data to be stored in this asset. `readonly` assets cannot be updated read-only. All other preceding fields are ignored.

`<fieldname>=<value>` : If format is `basic`, then the caller can provide additional = pairs for each piece of data to store in the asset.
json : If format is JSON, then this field will hold the json definition of the asset as a JSON array of objects representing each field in the object. It uses the following format:

- `name` : The name of the data field.

- `type` : The data type to use for this field. Values can be uint8, uint16, uint32, uint64, uint256, uint512, uint1024, string, or bytes.

- `value` : The default value of the field.

- `mutable` : The boolean field to indicate whether the field is writable (true) or read-only (false).

- `maxlength`: Only applicable to string or bytes fields where mutable=true, this is the maximum number of characters (bytes) that can be stored in the field. If no maxlength parameter is provided then we will default the field size to the length of the default value rounded up to the nearest 64 bytes.

**NOTE:** 
- There is a limit of 1KB for item data to be saved in the register, excluding the item name.
- There is a fee of 1 NXS for creating and an item and an additional 1 NXS for the optional name object.

### Results:

#### Return value JSON object:

```
{
    "success": true,
    "address": "87PUYqzvL73Ta81VfNMWuNyVjAqy3ZzE2iiX6FawibeNY56XC1u",
    "txid": "012d0190164584f6174c0c5b1cfa7c155a7faeb453b050ee4cc520681b47e52402d9aab5cdf327ee614752c4f40647d4bd3b32364db19da2bacb83c5f71a9144"
}
[Completed in 4970.165011 ms]
```

#### Return values:

`success` : Boolean flag indicating that the item was saved successfully.

`address` : The register address for this new created item. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.

`txid` : The ID (hash) of the transaction that includes the created object.

-----------------------------------

# <a name="get"></a> `get`

Retrieves information for a single item for a type specified by the noun.

```
supply/get/noun
```

This command supports the `item`, `raw`, `readonly` and `any` nouns.

### Parameters:


`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the name of the item. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the item. This is optional if the `name` is provided.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

### Results:

#### Return value JSON object:

```
[
    {
        "owner": "b7a57ddfb001d5d83ab5b25c0eaa0521e6b367784a30025114d07c444aa455c0",
        "version": 1,
        "created": 1656664801,
        "modified": 1656665317,
        "type": "OBJECT",
        "ABW": "DH0001222145565",
        "Address": "Marks street, PO Box:7887",
        "Item": "Samsung Watch",
        "address": "87wQAJ6nTWqVhGB423mKqtAJyF4B6S5WUXMqzYQWRcH9rJZxZup",
        "name": "local:Item0002"
    }
]
[Completed in 34.175112 ms]
```
#### Return Values:

`owner` : The username hash of the owners profile.

`version` : The serialization version of the transaction.

`created` : The UNIX timestamp when the item was created.

`modified` : The UNIX timestamp when the item was last modified.

`type` : The type of register. Can be OBJECT, RAW or READONLY.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the item.

`data` : The data stored in a raw or readonly item object.

`address` : The register address of the item.

`name` : The name identifying the item.

-----------------------------------

# <a name="list"></a> `list`

Retrieves a list of all the item objects owned by the profile specified by the noun.

```
supply/list/noun
```

This command supports the `item`, `raw`, `readonly` and `any` nouns.

#### list/item

Lists all the items for object register type.

#### list/readonly

Lists all items for readonly register type. 

#### list/raw

Lists all items for raw register type.

#### list/any

Lists all items for supported register types.

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
        "owner": "b7a57ddfb001d5d83ab5b25c0eaa0521e6b367784a30025114d07c444aa455c0",
        "version": 1,
        "created": 1656664801,
        "modified": 1656665317,
        "type": "OBJECT",
        "ABW": "DH0001222145565",
        "Address": "Marks street, PO Box:7887",
        "Item": "Samsung Watch",
        "address": "87wQAJ6nTWqVhGB423mKqtAJyF4B6S5WUXMqzYQWRcH9rJZxZup",
        "name": "local:Item0002"
    },
    {
        "owner": "b7a57ddfb001d5d83ab5b25c0eaa0521e6b367784a30025114d07c444aa455c0",
        "version": 1,
        "created": 1656662631,
        "modified": 1656663298,
        "type": "OBJECT",
        "ABW": "FED456315463135",
        "Address": "Avenue street, PO Box:4587",
        "Item": "Mobile Holder",
        "address": "87nyrW2TiKX9gZwRi61q3JNoVQK3GGTpZxBJBwR634au1A8Arc3",
        "name": "local:Item0001"
    }
]
[Completed in 34.175112 ms]
```

#### Return Values:

`owner` : The username hash of the owners profile.

`version` : The serialization version of the transaction.

`created` : The UNIX timestamp when the item was created.

`modified` : The UNIX timestamp when the item was last modified.

`type` : The type of register. Can be OBJECT, RAW or READONLY.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the item.

`data` : The data stored in a raw or readonly item object.

`address` : The register address of the item.

`name` : The name identifying the item.

-----------------------------------

# <a name="update"></a> `update`

This method provides the user with the ability to update the object data specified by the noun.

```
supply/update/noun
```

This command supports the `item`, `raw` and `any` nouns.

#### update/item

This updates the data values for item with the format basic and JSON.

#### update/raw

This updates the data value for the raw item register.

### Parameters:

`pin` : Required if authenticate. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to  **identify** the item to update by name. This is optional if the `address` is provided.

`address` : Required to **identify** the item to update by register address. This is optional if the `name` is provided.

`format` : Required to **specify** the format of the asset to update. Values can be `raw`, `basic` and `JSON`. 

`data` : Required if **format** is `raw`, allows caller to update the data object. 

`<fieldname>=<value>` : Required for **format** `basic`, The caller can provide 'key=value' pairs to update each piece of data for the item.

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

`success` : Boolean flag indicating that the session was saved successfully.

`txid` : The ID (hash) of the transaction for the updated object.

-----------------------------------

# <a name="transfer"></a> `transfer`

This will initiate ownership transfer of the specified noun.

```
supply/transfer/noun
```

This command supports the `item`, `raw`, `readonly` and `any` nouns.

#### transfer/item

This initiates transfer for an item to a recipient.

#### transfer/raw

This initiates transfer for a raw item to a recipient.

#### transfer/readonly

This initiates transfer for a readonly item to a recipient.

### Parameters:

`pin` : Required if authenticate. The PIN for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the name of the item. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the item. This is optional if the `name` is provided.

`recipient` : Required to **identify** the profile to transfer the item to. This can be the profile username or genesis hash.

`expires` : Optional field allows callers to specify an **expiration** for the transfer transaction. The expires value is the number of seconds from the transaction creation time after which the transaction can no longer be claimed by the recipient. Conversely, when you apply an expiration to a transaction, you are unable to void the transaction until after the expiration time. If expires is set to 0, the transaction will never expire, making the sender unable to ever void the transaction. If omitted, a default expiration of 7 days (604800 seconds) is applied.

### Results:

#### Return value JSON object:

```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1"
    "address": "8FJxzexVDUN5YiQYK4QjvfRNrAUym8FNu4B8yvYGXgKFJL8nBse"
}
```

#### Return values:

`txid` : The ID (hash) of the transaction that includes the name transfer.

`address` : The register address for this name.

-----------------------------------

# <a name="claim"></a> `claim`

This method will claim ownership of the item to complete the corresponding transfer transaction.

```
supply/claim/noun
```

This command supports the `item`, `raw`, `readonly` and `any` nouns.

#### claim/item

Items that have been transferred need to be claimed by the recipient. This method creates the claim transaction.

#### claim/raw

Raw items that have been transferred need to be claimed by the recipient. This method creates the claim transaction.

#### claim/readonly

Readonly items that have been transferred need to be claimed by the recipient. This method creates the claim transaction.

### Parameters:

`pin` : Required if **locked**. The PIN for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`txid` : Required the **transaction ID** (hash) of the item transfer transaction for which it is being claimed.

`name` : Optional field allows the user to **rename** an item when it is claimed. By default the name is copied from the previous owner and a Name record is created for the item in your user namespace. If you already have an object for this name then you will need to provide a new name in order for the claim to succeed.


### Results:

#### Return value JSON object:

```
{
    "claimed":
    [
        "25428293b6631d2ff55b3a931926fec920e407a56f7759495e36089914718d68",
        "1ff463e036cbde3595fbe2de9dff15721a89e99ef3e2e9bfa7ce48ed825e9ec2"
    ],
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1"
}
```

#### Return values:

`claimed`: Array of addresses for each name that was claimed by the transaction

`txid` : The ID (hash) of the transaction that includes the name transfer.

-----------------------------------

# <a name="history"></a> `history`

This will get the history of changes to an item, including both the data and it's ownership.

```
supply/history/noun
```

This command supports the `item`,  `raw`, `readonly` and `any` nouns.

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the name of the item. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the item. This is optional if the `name` is provided.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 

### Results:

#### Return value JSON object:

```
[
    {
        "owner": "00392196b83aca438567558462cd0c5d982569c7cefa668500c4bf3e61a03b7a",
        "version": 1,
        "created": 1656662631,
        "modified": 1656663027,
        "type": "OBJECT",
        "Item": "Amvette Mobile Holder",
        "Model": "SP-1010",
        "Location": "Aisle: 6, Rack: 7",
        "address": "87nyrW2TiKX9gZwRi61q3JNoVQK3GGTpZxBJBwR634au1A8Arc3",
        "action": "TRANSFER"
    },
    {
        "owner": "b7392196b83aca438567558462cd0c5d982569c7cefa668500c4bf3e61a03b7a",
        "version": 1,
        "created": 1656662631,
        "modified": 1656662631,
        "type": "OBJECT",
        "Item": "Amvette Mobile Holder",
        "Model": "SP-1010",
        "Location": "Aisle: 6, Rack: 7",
        "address": "87nyrW2TiKX9gZwRi61q3JNoVQK3GGTpZxBJBwR634au1A8Arc3",
        "action": "CREATE"
    }
]
[Completed in 2.431636 ms]
```

#### Return Values:

`owner` : The username hash of the owners profile.

`version` : The serialization version of the transaction.

`created` : The UNIX timestamp when the item was created.

`modified` : The UNIX timestamp when the item was last modified.

`type` : The type of register. Can be OBJECT, RAW or READONLY.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the item.

`data` : The data stored in a raw or readonly item object.

`address` : The register address of the item.

`action` : The action that occurred - CREATE | MODIFY | TRANSFER | CLAIM.

-----------------------------------

# <a name="transactions"></a> `transactions`

This will list off all of the transactions for the specified noun.

```
supply/transactions/noun
```

This command supports the `item`, `raw`, `readonly` and `any` nouns.

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

`name` : Required to **identify** the name of the item. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the item. This is optional if the `name` is provided.

`verbose` : Optional, determines how much transaction data to include in the response. Supported values are :  
- `default` : hash  

- `summary` : type, version, sequence, timestamp, operation, and confirmations.  

- `detail` : genesis, nexthash, prevhash, pubkey and signature.  

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 

### Results:

#### Return value JSON object:

```
[
    {
        "txid": "0127324285cf5c57751dd20b5d8c7d17f204101dc1625230a8199d53d0059c3e3ad1a36330d56b2442b80bed1bb65aef9b049d671d800308d807a29174db4663",
        "type": "tritium user",
        "version": 4,
        "sequence": 3,
        "timestamp": 1656663027,
        "blockhash": "9650d3eb7465976d8059b8027a1489f4ec5d9492b6786a6dfabf530d607845f74ebbd2b24685ee30c0f996d54bfe55e00cbc11781a3d661106581c41ff40882776601107ee1b19116c3829eff79ad6f4a478d64f7b57aa507c47b1f1a2041ce415fd730128121ad350459a3d5616ec5eaeef9ce5988e66a0780b2ced8a46f496",
        "confirmations": 93,
        "contracts": [
            {
                "id": 0,
                "OP": "TRANSFER",
                "address": "87nyrW2TiKX9gZwRi61q3JNoVQK3GGTpZxBJBwR634au1A8Arc3",
                "recipient": "b7a57ddfb001d5d83ab5b25c0eaa0521e6b367784a30025114d07c444aa455c0"
            }
        ]
    },
    {
        "txid": "01f5ab29bd8df00da8354d67ca337beb5091406c5d9356787ee9a1197662587ce46622fb3b3173b8d9071de4240073d87a5e2fcb4f5637abd6bad9deb2fb0072",
        "type": "tritium user",
        "version": 4,
        "sequence": 2,
        "timestamp": 1656662631,
        "blockhash": "fa0b6c7b3d58b38a5fd3d3ad5d1fd3e03d225e69cf8a3b1ef72c6a30caf9f46056b5094482c4e8b1966b313954a367e60a804451f72895dd6fd169efe4d6e6db981941e4f3203890d3e07cc1d111cf5c77136095959ee15ca552109949f88026f38f838ac30841d8491cd071493761de19248b7ff7001f4b43cd2585f5b08570",
        "confirmations": 94,
        "contracts": [
            {
                "id": 0,
                "OP": "CREATE",
                "address": "87nyrW2TiKX9gZwRi61q3JNoVQK3GGTpZxBJBwR634au1A8Arc3",
                "type": "OBJECT",
                "standard": "NONSTANDARD",
                "object": {
                    "Item": "Amvette Mobile Holder",
                    "Model": "SP-1010",
                    "Location": "Aisle: 6, Rack: 7",
                }
            }
        ]
    }
]
[Completed in 1.944840 ms]
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

`address` : The register address of the item object.

`txid` : The transaction ID hash of the transaction for the credit / claim.

`contract` : The ID of the contract within the transaction for the credit / claim.

`type`: The type of register. Can be OBJECT, RAW or READONLY.

`standard` : The type of object. Can be NONSTANDARD or JSON.

`recipient` : The transfer recipients profile username hash.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the item.

`object` : The data stored in the item.
}  

-----------------------------------