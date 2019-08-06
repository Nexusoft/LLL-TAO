# Names API
-----------------------------------

Names and Namespaces are special kinds of object registers that are used as locators to other object registers in the blockchain.  When an object register is first created (an asset for example) the caller can provide a name for the register.  If a name is provided then a Name object register is also created with its register address based on a hash of the name.  The Name object also has a address field, which is populated with the register address of the asset that the Name "points" to.  In this way, the asset object can be retrieved by name by first hashing the name to get the Name object's address, retrieving the Name object, and then using the address stored within it to retrieve the asset object.  A Name then, is best thought of as a named index to object registers.

By default all Name objects are created within the users local namespace, meaning that when accessing an object by name it must be prefixed with the owners username, e.g. paul:someasset.  Local namespaces are denoted by the format username:objectname, allowing the same objectname to be used by more than one user.   Alternatively Nexus allows for global namespaces to be registered and then used when creating Names, allowing the asset name to be decoupled from the user owning the asset.  Names that use global namespaces are denoted by the format objectname.namespace.      

The Names API allows callers to access and manage both Names and Namespaces.  Names can be created to "point" to any register address you wish, whether the caller owns the register or not.  This is useful, for example, if somebody gives you the register address of a NXS account to receive payments and you wish to add a friendly Name for it for future use.

Namespaces can be transferred to other signature chains, opening the possibility for a secondary market to buy and sell namespaces (similar to internet domain names).

Names that have been created within a namespace can also be transferred to other signature chains.  Names created without a namespace (user local names) cannot be transferred.


## `Methods`

The following methods are currently supported by this API

[`create/namespace`](#createnamespace)   
[`get/namespace`](#getnamespace)    
[`transfer/namespace`](#transfernamespace)   
[`claim/namespace`](#claimnamespace)   
[`list/namespace/history`](#listnamespacehistory)    
[`create/name`](#createname)   
[`get/name`](#getname)   
[`update/name`](#updatename)   
[`transfer/name`](#transfername)  
[`claim/name`](#claimname)   
[`list/name/history`](#listnamehistory)    

-----------------------------------
***

# `create/namespace`

This will create a new namespace.  The API supports an alternative endpoint that can include the new namespace name in the URL.  For example `/names/create/namespace/mynamespace` will resolve to `names/create/namespace?name=mynamespace`.  

**NOTE** : Namespace names must not contain either colon `:` or full stop `.` characters


### Endpoint:

`/names/create/namespace`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the namespace should be created with. For single-user API mode the session should not be supplied.

`name` : A name to identify the namespace. A hash of the name will determine the register address. 




### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1",
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9"
}
```

### Return values:

`txid` : The ID (hash) of the transaction that includes the namespace creation.

`address` : The register address for this namespace. 


***


# `get/namespace`

Retrieves a namespace object.  The API supports an alternative endpoint that can include the new namespace name in the URL.  For example `/names/get/namespace/mynamespace` will resolve to `names/get/namespace?name=mynamespace`.   


### Endpoint:

`/names/get/namespace`


### Parameters:

`name` : The name identifying the namespace. This is optional if the address is provided. 




### Return value JSON object:
```
{
    "name": "mynamespace",
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9",
    "owner": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1"
}
```

### Return values:

`owner` : The genesis hash of the signature chain that owns this Name.

`created` : The UNIX timestamp when the Name was created.

`name` : The name identifying the namespace. 

`address` : The register address of the namespace.  


***

# `transfer/namespace`

This will transfer ownership of an namespace .  This is a generic endpoint requiring the namespace name or address to be passed as parameters. The API supports an alternative endpoint that can include the namespace name or register address in the URL.  For example `/names/transfer/namespace/mynamespace` will resolve to `names/transfer/namespace?name=mynamespace`.  


### Endpoint:

`/names/transfer/namespace`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the namespace. For single-user API mode the session should not be supplied.

`name` : The name identifying the namespace to be transferred. This is optional if the address is provided. 

`address` : The register address of the namespace to be transferred. This is optional if the name is provided.

`username` : The username identifying the user account (sig-chain) to transfer the namespace to. This is optional if the destination is provided.

`destination` : The genesis hash of the signature chain to transfer the the namespace to. This is optional if the username is provided.


### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1",
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9"
}
```

### Return values:

`txid` : The ID (hash) of the transaction that includes the namespace transfer.

`address` : The register address for this namespace.   

****

# `claim/namespace`

Namespaces that have been transferred need to be claimed by the recipient before the transfer is complete.  This method creates the claim transaction .  This is a generic endpoint requiring the transaction ID (hash) of the corresponding transfer transaction to be passed as a parameter. The API supports an alternative endpoint that can include the transaction ID in the URI.  For example `/names/claim/namespace/27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1` will resolve to `names/claim/namespace?txid=27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1`.


### Endpoint:

`/names/claim/namespace`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the namespace. For single-user API mode the session should not be supplied.

`txid` : The transaction ID (hash) of the corresponding namespace transfer transaction for which you are claiming.

### Return value JSON object:
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

### Return values:

`claimed`: Array of addresses for each namespace that was claimed by the transaction

`txid` : The ID (hash) of the transaction that includes the namespace transfer.

****


# `list/namespace/history`

This will get the history of a namespace as well as it's ownership. The API supports an alternative endpoint that can include the namespace name (if known) or register address in the URL.  For example `/names/list/namespace/history/mynamespace` will resolve to `names/list/namespace/history?name=mynamespace`.  

### Endpoint:

`/names/list/namespace/history`


### Parameters:

`name` : The name identifying the namespace. This is optional if the address is provided.  

`address` : The register address of the namespace. This is optional if the name is provided.


### Return value JSON object:
```
[
    {
        "type": "TRANSFER",
        "owner": "2be51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0",
        "modified": 1560492117,
        "checksum": 13703027408063695802,
        "address": "3ff89f7f923bf9e3193540c95434f3bcaafabfe21e49068dca9440903d8233ec",
        "name": "test"
    },
    {
        "type": "CREATE",
        "owner": "1ff463e036cbde3595fbe2de9dff15721a89e99ef3e2e9bfa7ce48ed825e9ec2",
        "modified": 1560492117,
        "checksum": 13703027408063695802,
        "address": "3ff89f7f923bf9e3193540c95434f3bcaafabfe21e49068dca9440903d8233ec",
        "name": "test"
    }
]
```

### Return values:

The return value is a JSON array of objects for each entry in the namespaces history:

`type` : The action that occurred - CREATE | MODIFY | TRANSFER | CLAIM

`owner` : The genesis hash of the signature chain that owned the namespace.

`modified` : The Unix timestamp of when the namespace was updated.

`checksum` : A checksum of the state register used for pruning.

`address` : The register address of the namespace

`name` : The name of the namespace



****

# `create/name`

This will create a new name.  The API supports an alternative endpoint that can include the new name in the URL.  For example `/names/create/name/myname` will resolve to `names/create/name?name=myname`.   

**NOTE** : Names must not contain either colon `:` characters.


### Endpoint:

`/names/create/name`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the name should be created with. For single-user API mode the session should not be supplied.

`name` : The name of the object that this name will point to. The name can be provided in the format myname@mynamespace, in which case the caller must own the Namespace "mynamespace".  If the name is provided without the namespace suffix then the name is created in the users local namespace

`register_address` : The register address of the the object that this Name will point to. 

### Example

The following example creates a name for "asset1" in the namespace "ourcompany", which points to an asset at the register address beginning with bf501...  The caller must own the namespace "ourcompany" but the asset asset1 can be owned by someone else.  Once created, the asset can be retrieved by the name "asset1.ourcompany"

```
{
    "name": "asset1.ourcompany",
    "register_address": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1"
}
```

### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1",
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9"
}
```

### Return values:

`txid` : The ID (hash) of the transaction that includes the name creation.

`address` : The register address for this name. 


***


# `get/name`

Retrieves a name object.  There are two ways that this method can be used to retrieve a name object.  1) by using the `name` parameter the Name object can be retrieved in order to determine the register address that the Name "points" to. 2) by using the `register_address` parameter (in conjunction with `session` for multiuser mode) the Names owned by the caller will be searched to find the corresponding name.  NOTE: It is not possible (for privacy reasons) to look up a Name from a register address in signature chains other than the callers. 

The API supports an alternative endpoint that can include the name in the URL.  For example `/names/get/name/myname` will resolve to `names/get/name?name=myname`.   


### Endpoint:

`/names/get/name`


### Parameters:

`name` : The name identifying the name object. The name can either be a local name or a namespaced name in the format name@namespace.  If a local name is used then the caller must be logged in (single user mode) or the the `session` parameter must be provided (multiuser mode).If the `name` parameter is provided then all other parameters are ignored.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which signature chain should be searched. For single-user API mode the session should not be supplied, and the logged in signature chain will be used.  This parameter is ignored if `name` is provided.

`register_address` : The register address to search for.  If provided then the Names owned by the callers signature chain are searched to find a match.  This parameter is ignored if `name` is provided.

### Return value JSON object:
```
{
    "name": "asset1.ourcompany",
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9",
    "register_address": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1",
    "owner": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1"
}
```

### Return values:

`owner` : The genesis hash of the signature chain that owns this Name.

`created` : The UNIX timestamp when the Name was created.

`modified` : The UNIX timestamp when the Name was last modified.

`name` : The name identifying the object register. 

`address` : The register address of the Name.  

`register_address` : The register address of the the object that this Name points to.



***

# `update/name`

This method allows the register_address within a Name object to be changed .  The API supports an alternative endpoint that can include the name or register address in the URI.  For example `/names/update/name/myname` will resolve to `names/update/name?name=myname`.

### Endpoint:

`/names/update/name`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the name. For single-user API mode the session should not be supplied.

`name` : The name identifying the Name object to update. This is optional if the address is provided.  The name should be in the format usermame:name (for local names) or name.namespace (for names in a global namespace).  However, if the asset was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the name to update. This is optional if the name is provided.

`register_address` : The new register address that this Name should point to.


### Example:

The following example updates the register_address field for the name "asset1.ourcompany"
```
{
    "pin": "1234",
    "session": "16953804956473956578",
    "name": "asset1.ourcompany",
    "register_address": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1"
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

`txid` : The ID (hash) of the transaction that includes the name update.

`address` : The register address for the name that was updated.


****

# `transfer/name`

This will transfer ownership of a name .  Only names created in a global namespace (with a name in the format of myname.mynamespace) can be transferred. This is a generic endpoint requiring the name or address to be passed as parameters. The API supports an alternative endpoint that can include the name or register address in the URL.  For example `/names/transfer/name/myname` will resolve to `names/transfer/name?name=myname`.  


### Endpoint:

`/names/transfer/name`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the name. For single-user API mode the session should not be supplied.

`name` : The name identifying the name to be transferred. The name should be in the format usermame:name (for local names) or name.namespace (for names in a global namespace).  This is optional if the address is provided. 

`address` : The register address of the name to be transferred. This is optional if the name is provided.

`username` : The username identifying the user account (sig-chain) to transfer the name to. This is optional if the destination is provided.

`destination` : The genesis hash of the signature chain to transfer the the name to. This is optional if the username is provided.


### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1"
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9"
}
```

### Return values:

`txid` : The ID (hash) of the transaction that includes the name transfer.

`address` : The register address for this name.   

****

# `claim/name`

Names that have been transferred need to be claimed by the recipient before the transfer is complete.  This method creates the claim transaction .  This is a generic endpoint requiring the transaction ID (hash) of the corresponding transfer transaction to be passed as a parameter. The API supports an alternative endpoint that can include the transaction ID in the URI.  For example `/names/claim/name/27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1` will resolve to `names/claim/name?txid=27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1`.


### Endpoint:

`/names/claim/name`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the name. For single-user API mode the session should not be supplied.

`txid` : The transaction ID (hash) of the corresponding name transfer transaction for which you are claiming.

### Return value JSON object:
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

### Return values:

`claimed`: Array of addresses for each name that was claimed by the transaction

`txid` : The ID (hash) of the transaction that includes the name transfer.

****


# `list/name/history`

This will get the history of a name as well as it's ownership. The API supports an alternative endpoint that can include the object name (if known) or register address in the URL.  For example `/names/list/name/history/myname` will resolve to `names/list/name/history?name=myname`.  

### Endpoint:

`/names/list/name/history`


### Parameters:

`name` : The name identifying the name. The name should be in the format usermame:name (for local names) or name.namespace (for names in a global namespace). This is optional if the address is provided.  

`address` : The register address of the name. This is optional if the name is provided.


### Return value JSON object:
```
[
    {
        "type": "CREATE",
        "owner": "2be51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0",
        "modified": 1560492280,
        "checksum": 5612332250743384100,
        "address": "2af7515915a9df9e80c120f68c0ba7973e53669f791da5e6c12f36ca493ac05f",
        "name": "paul",
        "namespace": "test",
        "register_address": "94cdbf13aa1bbb2df228036e280c7b3cc5a5152ae1fed9bacca3b94e9e652bb1"
    }

]
```

### Return values:

The return value is a JSON array of objects for each entry in the names history:

`type` : The action that occurred - CREATE | MODIFY | TRANSFER | CLAIM

`owner` : The genesis hash of the signature chain that owned the name.

`modified` : The Unix timestamp of when the name was updated.

`checksum` : A checksum of the state register used for pruning.

`address` : The register address of the name

`name` : The name 

`namespace` : The namespace name

`register_address` : The register address that this name points to at this point in history


****


