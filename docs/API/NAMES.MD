# Names API
-----------------------------------

Names and Namespaces are special kinds of object registers that are used as locators to other object registers in the blockchain.  When an object register is first created (an asset for example) the caller can provide a name for the register.  If a name is provided then a Name object register is also created with its register address based on a hash of the name.  The Name object also has a address field, which is populated with the register address of the register (asset, token, account etc) that the Name "points" to.  In this way, objects can be retrieved by name by first hashing the name to get the Name object's address, retrieving the Name object, and then using the address stored within it to retrieve the object register.  A Name then, is best thought of as a named index to object registers.  

The TAO Naming System (TNS) allows Name objects to be created in one of three different contexts, or namespaces - `local`, `namespaced`, and `global`.  Names must be unique within the namespace in which it was defined.

Local Names are those created within the context of a signature chain.  To use a local name you must prefix the name with the owners username separated by a single colon, e.g. `bob:savings`.  This is equivalent to saying "look at all the Names registered in the sig chain `bob` and find one called `savings` and then see what object register it points to".   There can only be one Name called `savings` in the sig chain `bob`, but another user `alice` can also create a local name called `savings`  

Namespaced Names are those created within the context of a namespace, which itself is a globally unique keyword.  Namespaces allow users to provide user-friendly names for their object registers without needing to disclose their username.  This is useful for privacy, but also to allow names to be related to a business or some other meaningful context.  To use a namespaced name you must prefix the name with the namespace separated by a double colon, e.g. `bobscoffeeshop::payments`.  In this example bob would have first registered the namespace `bobscoffeeshop` and created an account to receive payments to (which could be called anything).  He then creates a Name with a `name=payments`, `namespace=bobscoffeeshop` and `address=(register address of the account)`.  From then on, anyone can use the name `bobscoffeeshop::payments` and it will resolve to the register address of the account.  To avoid name-squatting registering a namespace name attracts a high fee (`1000 NXS`).  However once registered, creating Names within that namespace costs only `1 NXS`  

Global Names require no username or namespace prefix, and are therefore globally unique.  These will be likely reserved for use cases where a succinct, unique, name is necessary, for example a token ticker symbol. To avoid needless name-squatting, global names attract a high fee (`2000 NXS`). 

The Names API allows callers to access and manage both Names and Namespaces.  Names can be created to "point" to any register address you wish, whether the caller owns the register or not.  This is useful, for example, if somebody gives you the register address of a NXS account to receive payments and you wish to add a friendly Name for it for future use.

Namespaces can be transferred to other signature chains, opening the possibility for a secondary market to buy and sell namespaces (similar to internet domain names).

Global Names and Names that have been created within a namespace can also be transferred to other signature chains.  Local names cannot be transferred.


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

**NOTE** : Namespace names cannot can only contain **lowercase letters, numbers, and periods (.)**.


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
    "address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P"
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
    "address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P",
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

`expires` : This optional field allows callers to specify an expiration for the transfer transaction. The expires value is the `number of seconds` from the transaction creation time after which the transaction can no longer be claimed by the recipient.  Conversely, when you apply an expiration to a transaction, you are unable to void the transaction until after the expiration time.  If omitted, a default expiration of 1 day (86400 seconds) is applied


### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1",
    "address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P"
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
        "address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P",
        "name": "test"
    },
    {
        "type": "CREATE",
        "owner": "1ff463e036cbde3595fbe2de9dff15721a89e99ef3e2e9bfa7ce48ed825e9ec2",
        "modified": 1560492117,
        "checksum": 13703027408063695802,
        "address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P",
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

`name` : The name of the object that this name will point to. The name can contain any characters, but must not START with a colon `:`

`namespace` : This optional field allows callers to specify the namespace that the name should be created in.  If the namespace is provided then the caller must also be the owner of the namespace.  i.e. you cannot create a name in someone elses namespace.  If the namespace is left blank (the default) then the Name will be created in the users local namespace (unless specifically flagged as global).

`global` : This optional, boolean field indicates that the Name should be created in the global namespace, i.e. it will be globally unique.  If the caller sets this field to true, the namespace parameter is ignored.

`register_address` : The 256-bit hexadecimal register address of the the object that this Name will point to. 


### Example

The following example creates a name for "asset1" in the namespace "ourcompany", which points to an asset at the register address beginning with bf501...  The caller must own the namespace "ourcompany" but the asset asset1 can be owned by someone else.  Once created, the asset can be retrieved by the name "ourcompany::asset1"

```
{
    "name": "asset1",
    "namespace": "ourcompany",
    "register_address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P"
}
```

### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1",
    "address": "8FJxzexVDUN5YiQYK4QjvfRNrAUym8FNu4B8yvYGXgKFJL8nBse"
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

`name` : The name identifying the name object. The name should be in the format username:name (for local names) or name.namespace (for names in a global namespace).  If the `name` parameter is provided then all other parameters are ignored.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which signature chain should be searched. For single-user API mode the session should not be supplied, and the logged in signature chain will be used.  This parameter is ignored if `name` is provided.

`register_address` : The register address to search for.  If provided then the Names owned by the callers signature chain are searched to find a match.  This parameter is ignored if `name` is provided.

### Return value JSON object:
```
{
    "owner": "a2e51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0",
    "created": 1565934825,
    "modified": 1565934825,
    "address": "8FJxzexVDUN5YiQYK4QjvfRNrAUym8FNu4B8yvYGXgKFJL8nBse",
    "name": "asset1",
    "namespace": "somenamespace",
    "register_address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P"
}
```

### Return values:

`owner` : The genesis hash of the signature chain that owns this Name.

`created` : The UNIX timestamp when the Name was created.

`modified` : The UNIX timestamp when the Name was last modified.

`name` : The name identifying the object register. 

`namespace` : The namespace that the name was created in.  For global names, this will be set to `~GLOBAL~`. 

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

`name` : The name identifying the Name object to update. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the asset was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the name to update. This is optional if the name is provided.

`register_address` : The new register address that this Name should point to.


### Example:

The following example updates the register_address field for the name "asset1.ourcompany"
```
{
    "pin": "1234",
    "name": "asset1.ourcompany",
    "register_address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P"
}
```


### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1"
    "address": "8FJxzexVDUN5YiQYK4QjvfRNrAUym8FNu4B8yvYGXgKFJL8nBse"
}
```

### Return values:

`txid` : The ID (hash) of the transaction that includes the name update.

`address` : The register address for the name that was updated.


****

# `transfer/name`

This will transfer ownership of a name .  Only global names or names created in a namespace (with a name in the format of mynamespace::myname) can be transferred. This is a generic endpoint requiring the name or address to be passed as parameters. The API supports an alternative endpoint that can include the name or register address in the URL.  For example `/names/transfer/name/myname` will resolve to `names/transfer/name?name=myname`.  


### Endpoint:

`/names/transfer/name`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the name. For single-user API mode the session should not be supplied.

`name` : The name identifying the name to be transferred. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  This is optional if the address is provided. 

`address` : The register address of the name to be transferred. This is optional if the name is provided.

`username` : The username identifying the user account (sig-chain) to transfer the name to. This is optional if the destination is provided.

`destination` : The genesis hash of the signature chain to transfer the the name to. This is optional if the username is provided.

`expires` : This optional field allows callers to specify an expiration for the transfer transaction. The expires value is the `number of seconds` from the transaction creation time after which the transaction can no longer be claimed by the recipient.  Conversely, when you apply an expiration to a transaction, you are unable to void the transaction until after the expiration time.  If omitted, a default expiration of 1 day (86400 seconds) is applied


### Return value JSON object:
```
{
    "txid": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1"
    "address": "8FJxzexVDUN5YiQYK4QjvfRNrAUym8FNu4B8yvYGXgKFJL8nBse"
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

`name` : The name identifying the name. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the address is provided.  

`address` : The register address of the name. This is optional if the name is provided.


### Return value JSON object:
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


