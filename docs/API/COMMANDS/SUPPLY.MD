# Supply API
-----------------------------------

The Supply API provides functionality to support the ownership transfer requirements typical of a supply chain process. Items in the supply chain can be given a value and this value can be updated over time. The supply API supports the readonly, raw, basic and JSON formats and the user is required to specify the format.

The readonly and raw formats are useful when developers wish to store arbitrary data, without incurring the overhead of defining an object. The value will be provided with the `data` parameter. The readonly format cannot be updated and is stored in a readonly register. The raw format is stored in a raw register and allows the data to be updated.

The basic format allows callers to define an asset in terms of simple key=value pairs. It assumes all values are stored using the string data type. The key=value pairs can be updated
.
The JSON format allows callers to provide a detailed definition of the supply data structure, with each field defined with a specific data type and mutability. Items defined with one of the complex formats can be updated after their initial creation.

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

This command only supports the `item`, `raw` and `readonly` nouns..

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

# <a name="update"></a> `update`

Update an object register specified by given noun.

```
supply/update/noun
```

This command only supports the item and raw nouns.

#### update/item

This updates the values of basic and JSON formats with the item noun.

#### update/raw

This updates the data value of the raw item.

### Parameters:

`pin` : Required if authenticate. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to  identify the item to update by name.  This is optional if the address is provided.

`address` : Required to identify the item to update by register address. This is optional if the name is provided.

`data` : The new value of the data field in this item

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

# <a name="get"></a> `get`

Retrieves information for a specified item.

```
supply/get/noun
```

This command supports the `item`,  `raw`, `readonly` and `any` nouns.

### Parameters:


`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the name of the item. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the item. This is optional if the `name` is provided.

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


-----------------------------------

# <a name="list"></a> `list`

Retrieves information for a single object for a type specified by the noun

```
supply/list/noun
```

This command supports the `item`, `raw`, `readonly` and `any` nouns.

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

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

owner : The username hash of the profile that owns this asset.

created : The UNIX timestamp when the asset was created.

`modified` : The UNIX timestamp when the asset was last modified.

`type` : Asset register type. Can be OBJECT, RAW or READONLY.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the asset.name : The name identifying the item. For privacy purposes, this is only included in the response if the caller is the owner of the item

`data` : The data stored in the item.

`address` : The register address of the item.

`name` : The name identifying the asset. For privacy purposes, this is only included in the response if the caller is the owner of the asset

-----------------------------------

# <a name="transfer"></a> `transfer`

This will initiate ownership transfer of the specified noun.

```
supply/transfer/noun
```

This command supports the `item`, `raw` and `readonly` nouns.

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

This method will claim ownership of the specified noun by the recipient to complete the transaction.

```
supply/claim/noun
```

This command supports the `item`,  `raw` and `readonly` nouns.

#### claim/item
This claims ownership of an item which has been transferred.

#### claim/raw
This claims ownership of a raw item which has been transferred.

#### claim/readonly
This claims ownership of a readonly item which has been transferred.

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

`pin` : Required if **locked**. The PIN for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the name of the item. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the item. This is optional if the `name` is provided.


### Results:

#### Return value JSON object:

```
[
    {
        "type": "CREATE",
        "owner": "2be51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0",
        "modified": 1560492280,
        "checksum": 5612332250743384100,
        "address": "8FJxzexVDUN5YiQYK4QjvfRNrAUym8FNu4B8yvYGXgKFJL8nBse",
        "name": "paul",
        "namespace": "test",
        "register_address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P"
    }

]
```

#### Return values:

`owner` : The username hash of the profile that owns this asset.

`created` : The UNIX timestamp when the asset was created.

`modified` : The UNIX timestamp when the asset was last modified.

`type` : The type of register. Can be OBJECT, RAW or READONLY.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the asset.name : The name identifying the item. For privacy purposes, this is only included in the response if the caller is the owner of the item

`data` : The data stored in the item.

`address` : The register address of the item.

`name` : The name identifying the asset. For privacy purposes, this is only included in the response if the caller is the owner of the asset

-----------------------------------

# <a name="transactions"></a> `transactions`

This will list off all of the transactions for the specified noun.

```
supply/transactions/noun
```

This command supports the `item`, `raw`, `readonly` and `any` nouns.

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`verbose` : Optional, determines how much transaction data to include in the response. Supported values are :  

- `default` : hash  

- `summary` : type, version, sequence, timestamp, operation, and confirmations.  

- `detail` : genesis, nexthash, prevhash, pubkey and signature.  

This method supports the Sorting / Filtering parameters.

### Results:

#### Return value JSON object:

```
[
    {
        "txid": "0123517ca0f1ca110c7b07de9e3c9b33ccbe717f96911e1449b7c73bb9695fbc9c14a58f01f5fb7e9b64756f658af91daec9f0f579df2fad8df61843defae833",
        "type": "tritium user",
        "version": 4,
        "sequence": 23,
        "timestamp": 1655061950,
        "blockhash": "8b206ab2ee4b46a835f74af0ff5d4e0b395acdb94d66468a24083f2a5fd01a07a93956774001bab1a801d53d7bf6ed60ee84a573650eef1a9feaf6fa9beb308bd20b567663cc7ec4f85796b261164ef3452ebfaa13a60141b42fc49d6d2eb2792440925b1b19248ad9fe65e01d3742f2d3dec2817c56c8e4f6e03a10f4147308",
        "confirmations": 4,
        "contracts": [
            {
                "id": 0,
                "OP": "DEBIT",
                "from": "8DXmAmkTtysSZUxM3ePA8wRmbSUofuHKSoCyDpN28aLuSrm1nDG",
                "to": "8Bk5PxsecfXWpbHsDXeZ47MCgDF7qDLsU4Y4MJw2VB29LsTR98z",
                "amount": 1.0,
                "token": "8DXmAmkTtysSZUxM3ePA8wRmbSUofuHKSoCyDpN28aLuSrm1nDG",
                "ticker": "XYZ",
                "reference": 57891358795
            }
        ]
    },
    {
        "txid": "01f1a3f9227a69382f9811a5b1497a865ace17ad83b03118b24f875f6ade83117887c35d08375c259aa1076b91f42206110314756a11a943760bb5c0dd0523d7",
        "type": "tritium user",
        "version": 4,
        "sequence": 21,
        "timestamp": 1655060214,
        "blockhash": "048f3b308e8bd8c1aa31ec1ec2e136a9ccc91ec4498283d07fc5d0a00c8576e2c199567a44058222961f474626c6f2c5d7e774eee34c34f98acafaeb50b7abaaade7e9c641fe9727fe62533b1ec6bf2f75ffbf19d17d74671e2458bd73b6407b4bba1951fc84e1af11c2c4fbce1d05d7739e910fdb8a37197c1c422521e2e9f3",
        "confirmations": 6,
        "contracts": [
            {
                "id": 0,
                "OP": "DEBIT",
                "from": "8DXmAmkTtysSZUxM3ePA8wRmbSUofuHKSoCyDpN28aLuSrm1nDG",
                "to": "8Bk5PxsecfXWpbHsDXeZ47MCgDF7qDLsU4Y4MJw2VB29LsTR98z",
                "amount": 1.0,
                "token": "8DXmAmkTtysSZUxM3ePA8wRmbSUofuHKSoCyDpN28aLuSrm1nDG",
                "ticker": "XYZ",
                "reference": 0
            }
        ]
    }
]
[Completed in 2.187165 ms]
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

`OP` : The contract operation. Can be APPEND, CLAIM, COINBASE, CREATE, CREDIT, DEBIT, FEE, GENESIS, LEGACY, TRANSFER, TRUST, STAKE, UNSTAKE, WRITE.

`for` : For CREDIT transactions, the contract that this credit was created for . Can be COINBASE, DEBIT, orLEGACY.

`txid` : The transaction that was credited / claimed.

`contract` : The ID of the contract within the transaction that was credited / claimed.

`proof` : The register address proving the credit.

`from` : For DEBIT, CREDIT, FEE transactions, the register address of the account that the debit is being made from.

`from_name` : For DEBIT, CREDIT, FEE transactions, the name of the account that the debit is being made from. Only included if the name can be resolved.

`to` : For DEBIT and CREDIT transactions, the register address of the recipient account.

`to_name` : For DEBIT and CREDIT transactions, the name of the recipient account. Only included if the name can be resolved.

`amount` : the token amount of the transaction.

}  

-----------------------------------