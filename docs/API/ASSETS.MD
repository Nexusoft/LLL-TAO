# Assets API
-----------------------------------

An asset is a user-defined data structure that is stored in an object register, owned by a particular signature chain. Assets can hold one or more pieces of data and users can define the fields (name, data type, mutability) that data is stored in. The assets API supports several formats for defining the object data structures giving users and developers varying levels of functionality, including per-field type definition and mutability.  

The `basic` format allows callers to define an asset in terms of simple key-value pairs. It assumes that all fields are read-only and all values are stored using the string data type. `basic ` assets are always immutable and cannot be updated once created.

The `JSON`, `ANSI`, and `XML` ~formats allow callers to provide a detailed definition of the asset data structure, with each field defined with a specific data type and mutability. Assets defined with one of the complex formats can be updated after their initial creation.  

The `raw` format differs from the other formats in that the asset data is not stored in object register, but instead is stored in a read-only state register. The raw format is useful when developers wish to store arbitrary binary data on the Nexus blockchain, without incurring the overhead of defining an object.

## `Named Shortcuts`

For each API method we support an alternative endpoint that includes the asset name or register address at the end of the the URI.  This shortcut removes the need to include the asset name or address as an additional parameter.  

For example `assets/create/asset/myasset` is a shortcut to `assets/create/asset?name=myasset`.

Similarly `assets/get/asset/5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9` is a shortcut to `assets/get/asset?address=5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9` 

The logic for resolving the shortcut to either a name or address is that if the data is 64 characters of hexadecimal then it will be assumed to be a register address.  Otherwise it will be considered a name.  


## `Methods`

The following methods are currently supported by this API

[`create/asset`](#createasset)   
[`get/asset`](#getasset)   
[`update/asset`](#updateasset)   
[`transfer/asset`](#transferasset)   
[`claim/asset`](#claimasset)   
[`list/asset/history`](#listassethistory)   
[`tokenize/asset`](#tokenizeasset)

-----------------------------------
***


# `create/asset`

This will create a new asset or object register. The API supports an alternative endpoint that can include the new asset name in the URI.  For example `/assets/create/asset/myasset` will resolve to `assets/create/asset?name=myasset`.   


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
- `maxlength`: Only applicable to `string` or `bytes` fields where `mutable`=true, this is the maximum number of characters (bytes) that can be stored in the field.  If no maxlength parameter is provided then we will default the field size to the length of the default value rounded up to the nearest 64 bytes.  

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

This is the generic endpoint for retrieving an asset from the object register. The Assets API supports an alternative endpoint that can include the asset name (if known) in the URI.  For example `/assets/get/asset/myasset` will resolve to `assets/get/asset?name=myasset`.

Additionally the API supports passing a field name in the URL after the asset name, which will populate the `fieldname` parameter in the request.  For example `/assets/get/asset/myasset/description` will resolve to `assets/get/asset?name=myasset&fieldname=description`


### Endpoint:

`/assets/get/asset`


### Parameters:

`name` : The name identifying the asset. This is optional if the address is provided.  The name should be in username:name format.  However, if the asset was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the name in order to deduce the register address of the asset.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

`address` : The register address of the asset. This is optional if the name is provided.

`fieldname`: This optional field can be used to filter the response to return only a single field from the asset.


### Return value JSON object:
```
{
    "name": "watch",
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9",
    "timestamp": "1553227128",
    "owner": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1",
    "serial_number": "123456789123456789",
    "description": "This is the description of my asset",
    "shelf_location": "Aisle 9 Shelf 2"
}
```
### Return values:

`name` : The name identifying the asset. For privacy purposes, this is only included in the response if the caller is the owner of the asset

`address` : The register address of the asset.  

`timestamp` : The Unix timestamp of when the asset was last updated.

`owner` : The genesis hash of the signature chain that owns this asset.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the asset.


***



# `update/asset`

This is the generic endpoint for updating one or more values in an asset.  The API supports an alternative endpoint that can include the asset name (if known) or register address in the URI.  For example `/assets/update/asset/myasset` will resolve to `assets/update/asset?name=myasset`.

**NOTE** Only assets created using `JSON`, `ANSI`, or `XML` an be updated, and only those data fields that were defined as mutable. `raw` and `basic` format assets are always created as read-only (immutable).


### Endpoint:

`/assets/update/asset`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the asset should be created with. For single-user API mode the session should not be supplied.

`name` : The name identifying the asset to update. This is optional if the address is provided.  The name should be in username:name format.  However, if the asset was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the asset to update. This is optional if the name is provided.

`<fieldname>=<value>` : The caller can provide <fieldname>=<newvalue> pairs for each piece of data to update in the asset.


### Example:

The following is an example JSON payload to update two fields in an asset called "mysong"
```
{
    "pin": "1234",
    "session": "16953804956473956578",
    "name": "mysong",
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

This will transfer ownership of an asset or digital item.  This is a generic endpoint requiring the asset name or address to be passed as parameters. The API supports an alternative endpoint that can include the asset name (if known) or register address in the URI.  For example `/assets/transfer/asset/myasset` will resolve to `assets/transfer/asset?name=myasset`.  


### Endpoint:

`/assets/transfer/asset`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the asset. For single-user API mode the session should not be supplied.

`name` : The name identifying the asset to be transferred. This is optional if the address is provided.  The name should be in username:name format.  However, if the asset was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

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

# `claim/asset`

Assets that have been transferred need to be claimed by the recipient before the transfer is complete.  This method creates the claim transaction .  This is a generic endpoint requiring the transaction ID (hash) of the corresponding transfer transaction to be passed as a parameter. The API supports an alternative endpoint that can include the transaction ID in the URI.  For example `/assets/claim/asset/27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1` will resolve to `assets/claim/asset?txid=27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1`.


### Endpoint:

`/assets/claim/asset`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the asset. For single-user API mode the session should not be supplied.

`txid` : The transaction ID (hash) of the corresponding asset transfer transaction for which you are claiming.

`name` : This optional field allows the user to rename an asset when it is claimed.  By default the name is copied from the previous owner and a Name record is created for the asset in your user namespace.  If you already have an object for this name then you will need to provide a new name in order for the claim to succeed.   

### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1"
}
```

### Return values:

`txid` : The ID (hash) of the transaction that includes the asset transfer.

****

# `list/asset/history`

This will get the history of an asset or digital item as well as it's ownership.The API supports an alternative endpoint that can include the asset name (if known) or register address in the URI.  For example `/assets/list/asset/history/myasset` will resolve to `assets/list/asset/history?name=myasset`.  

### Endpoint:

`/assets/list/asset/history`


### Parameters:

`name` : The name identifying the asset. This is optional if the address is provided.  The name should be in username:name format.  However, if the asset was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the name in order to deduce the register address of the asset.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

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

`owner` : The genesis hash of the signature chain that owns the asset.

`timestamp` : The Unix timestamp of when the asset was updated.



***

# `tokenize/asset`

This will tokenize an asset into fungible tokens that represent ownership.  This is a generic endpoint requiring the asset name or address to be passed as parameters. The API supports an alternative endpoint that can include the asset name (if known) or register address in the URI.  For example `/assets/tokenize/asset/myasset` will resolve to `assets/tokenize/asset?name=myasset`.  

### Endpoint:

`/assets/tokenize/asset`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig chain) owns the asset. For single-user API mode the session should not be supplied.

`name` : The name identifying the asset to be tokenized. This is optional if the address is provided.  The name should be in username:name format.  However, if the asset was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the asset to be tokenized. This is optional if the name is provided.

`token_name` : The name of a token to use to tokenize the asset. The name should be in username:token name format.  `token_address` can be supplied as an alternative to `token_name`.

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
