# Assets API
-----------------------------------

An asset is a user-defined data structure that is stored in an object register, owned by a particular signature chain. Assets can hold one or more pieces of data and users can define the fields (name, data type, mutability) that data is stored in. The assets API supports several formats for defining the object data structures giving users and developers varying levels of functionality, including per-field type definition and mutability.  

The `basic` format allows callers to define an asset in terms of simple key-value pairs. It assumes that all fields are read-only and all values are stored using the string data type. `basic ` assets are always immutable and cannot be updated once created.

The `JSON`, `ANSI`, and `XML` formats allow callers to provide a detailed definition of the asset data structure, with each field defined with a specific data type and mutability. Assets defined with one of the complex formats can be updated after their initial creation.  

The `raw` format differs from the other formats in that the asset data is not stored in object register, but instead is stored in a read-only state register. The raw format is useful when developers wish to store arbitrary binary data on the Nexus blockchain, without incurring the overhead of defining an object.

***


# `create/asset`

This will create a new asset or object register. The API supports an alternative endpoint that can include the new asset name in the URI.  For example `/assets/create/myasset` will resolve to `assets/create/asset?name=myasset`.   


### Endpoint:

`/assets/create/asset`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the asset should be created with. For single-user API mode the session should not be supplied.

`name` : A name to identify the asset. A hash of the name will determine the register address. If no name is provided, a random 256-bit register hash will be generated.

`format` : The format the caller is using to define the asset. Values can be `basic` (the default), `raw`, `JSON`, `ANSI` (not currently supported), or `XML` (not currently supported). This is an optional field and the value `basic` is assumed if omitted.

`data` : If format is `raw`, then this field contains the hex-encoded data to be stored in this asset. Raw assets are always read-only. All other preceding fields are ignored.

`<fieldname>=<value>` : If format is `basic`, then the caller can provide additional <fieldname>=<value> pairs for each piece of data to store in the asset.

`json` : If format is `JSON`, then this field will hold the json definition of the asset as a JSON array of objects representing each field in the object. It uses the following format:

- `name` : The name of the data field.
- `type` : The data type to use for this field. Values can be `uint8`, `uint16`, `uint32`, `uint64`, `uint256`, `uint512`, `uint1024`, `string`, or `bytes`.
- `value` : The default value of the field.
- `mutable` : The boolean field to indicate whether the field is writable (true) or read-only (false).


`token_name` : The name of a token to use to tokenize the asset. If provided, this will immediately tokenize the asset into fungible tokens that represent ownership of the asset.
`token_address` : The register address of a token to use to tokenize the asset. If provided, this will immediately tokenize the asset into fungible tokens that represent ownership of the asset.  `token_name` can be supplied as an alternative to `token_address`.


### Example:

The following is an example of an asset defined using the `basic` format:   
**NOTE**: In this example the asset is entirely read-only and no fields can be updated.
```
{
    "pin": "1234",
    "session": "16953804956473956578",
    "name": "watch",
    "format": "basic",
    "serial_number": "123456789123456789",
    "description": "This is the description of my asset",
    "shelf_location": "Aisle 9 Shelf 2"
}
```

The following is an example of an asset defined using the `JSON` format:   
**NOTE**: In this example the shelf_location field is defined as mutable and can therefore be updated.
```
{
    "pin": "1234",
    "session": "16953804956473956578",
    "name": "watch",
    "format": "JSON",
    "json" :
        [
            {
                "name": "serial_number",
                "type": "uint64",
                "value": "123456789123456789",
                "mutable" : "false"
            },
            {
                "name": "description",
                "type": "string",
                "value": "This is the description of my asset",
                "mutable" : "false"
            },
            {
                "name": "shelf_location",
                "type": "string",
                "value": "Aisle 9 Shelf 2",
                "mutable" : "true"
            }
        ]
}
```




### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1"
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9"
}
```

### Return values:

`txid` : The ID (hash) of the transaction that includes the asset creation.

`address` : The register address for this asset. The address (or name that hashes to this address) is needed for downstream transactions / api methods for this asset.    


****


# `get/asset`

This is the generic endpoint for retrieving an asset from the object register. The Assets API supports an alternative endpoint that can include the asset name (if known) in the URI.  For example `/assets/get/myasset` will resolve to `assets/get/asset?name=myasset`.


### Endpoint:

`/assets/get/asset`


### Parameters:

`name` : The name identifying the asset. This is optional if the address is provided.

`address` : The register address of the asset. This is optional if the name is provided.


### Return value JSON object:
```
{
    "timestamp": "1553227128"
    "owner": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1"
    "serial_number": "123456789123456789",
    "description": "This is the description of my asset",
    "shelf_location": "Aisle 9 Shelf 2"
}
```
### Return values:

`timestamp` : The Unix timestamp of when the asset was last updated.

`owner` : The genesis hash of the signature chain that owns this asset.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the asset.


***


# `get/schema`

Retrieves the schema / object definition of an asset.  

### Endpoint:

`/assets/get/schema`


### Parameters:

`name` : The name identifying the asset. This is optional if the address is provided.

`address` : The register address of the asset. This is optional if the name is provided.

`format` : The format that the schema should be returned in. Values can be `JSON` (the default), `ANSI` (not currently supported), or `XML` (not currently supported). This is optional field and the value `JSON` is assumed if omitted.

**NOTE**: If the asset was initially created using the `raw` format then this API method will error as there is no schema associated with a raw state register.  

### Return value JSON object:
```
{
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9"
    "json" :
        [
            {
                "name": "serial_number",
                "type": "uint32",
                "value": "123456789123456789",
                "mutable" : "false"
            },
            {
                "name": "description",
                "type": "string",
                "value": "This is the description of my asset",
                "mutable" : "false"
            },
            {
                "name": "shelf_location",
                "type": "string",
                "value": "Aisle 9 Shelf 2",
                "mutable" : "true"
            },
        ]
}
```

### Return values:

`address` : The register address for this asset. The address (or name that hashes to this address) is needed for downstream transactions / api methods for this asset.   

`json` : If format is `JSON`, then this field will hold the json definition of the asset as an array of JSON objects representing each field in the object. It uses the following format:

- `name` : The name of the data field.
- `type` : The data type to use for this field. Values can be `uint8`, `uint16`, `uint32`, `uint64`, `uint256`, `uint512`, `uint1024`, `string`, or `bytes`.
- `value` : The current value of the field.
- `mutable` : The boolean field to indicate whether the field is writable (true) or read-only (false).


***


# `update/asset`

This is the generic endpoint for updating one or more values in an asset.  The API supports an alternative endpoint that can include the asset name (if known) in the URI.  For example `/assets/update/myasset` will resolve to `assets/update/asset?name=myasset`.

**NOTE** Only assets created using `JSON`, `ANSI`, or `XML` an be updated, and only those data fields that were defined as mutable. `raw` and `basic` format assets are always created as read-only (immutable).


### Endpoint:

`/assets/update/asset`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the asset should be created with. For single-user API mode the session should not be supplied.

`name` : The name identifying the asset to update. This is optional if the address is provided.

`address` : The register address of the asset to update. This is optional if the name is provided.

`format` : The format the caller is using to provide the data to update the asset. Values can be `JSON` (the default), or `XML` (not currently supported). This is an optional field and the value `JSON` is assumed if omitted.

`<fieldname>=<value>` : If format is `JSON` (the default if omitted), then the caller can provide <fieldname>=<newvalue> pairs for each piece of data to update in the asset.


### Example:

The following is an example JSON payload to update two fields in an asset called "mysong"
```
{
    "pin": "1234",
    "session": "16953804956473956578",
    "name": "mysong",
    "format": "JSON",
    "last_downloaded": "2019/02/03 14:22:08",
    "download_count": "428"
}
```


### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1"
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9"
}
```

### Return values:

`txid` : The ID (hash) of the transaction that includes the asset update.

`address` : The register address for this asset.


****

# `transfer/asset`

This will transfer ownership of an asset or digital item.  This is a generic endpoint requiring the asset name or address to be passed as parameters. The API supports an alternative endpoint that can include the asset name (if known) in the URI.  For example `/assets/transfer/myasset` will resolve to `assets/transfer/asset?name=myasset`.  


### Endpoint:

`/assets/transfer/asset`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the asset. For single-user API mode the session should not be supplied.

`name` : The name identifying the asset to be transferred. This is optional if the address is provided.

`address` : The register address of the asset to be transferred. This is optional if the name is provided.

`username` : The username identifying the user account (sig-chain) to transfer the asset to. This is optional if the destination is provided.

`destination` : The genesis hash of the signature chain to transfer the the asset to. This is optional if the username is provided.


### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1"
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9"
}
```

### Return values:

`txid` : The ID (hash) of the transaction that includes the asset transfer.

`address` : The new register address for this asset.   

****


# `list/history`

This will get the history of an asset or digital item as well as it's ownership.

### Endpoint:

`/assets/list/history`


### Parameters:

`name` : The name identifying the asset. This is optional if the address is provided.

`address` : The register address of the asset. This is optional if the name is provided.


### Return value JSON object:
```
[
    {
        "owner": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9",
        "timestamp": 1553227128
    },
    {
        "owner": bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1,
        "timestamp": 1553249316
    }
]
```

### Return values:

The return value is a JSON array of objects for each entry in the assets history:

`owner` : The genesis hash of the signature chain that owns this asset.

`timestamp` : The Unix timestamp of when the asset was first created.



***

# `tokenize/asset`

This will tokenize an asset into fungible tokens that represent ownership.  This is a generic endpoint requiring the asset name or address to be passed as parameters. The API supports an alternative endpoint that can include the asset name (if known) in the URI.  For example `/assets/tokenize/myasset` will resolve to `assets/tokenize/asset?name=myasset`.  

### Endpoint:

`/assets/tokenize/asset`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig chain) owns the asset. For single-user API mode the session should not be supplied.

`name` : The name identifying the asset to be tokenized. This is optional if the address is provided.

`address` : The register address of the asset to be tokenized. This is optional if the name is provided.

`token_name` : The name of a token to use to tokenize the asset. `token_address` can be supplied as an alternative to `token_name`.

`token_address` : The register address of a token to use to tokenize the asset. `token_name` can be supplied as an alternative to `token_address`.


### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1"
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9"
}

```

### Return values:

`txid` : The ID (hash) of the transaction that includes the asset tokenization.

`address` : The register address for this asset.
