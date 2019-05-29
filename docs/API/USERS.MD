# Users API
-------------------------------------

The Users API provides methods for creating and managing users.  A user is synonymous with a signature chain.


## `Named Shortcuts`

For each API method we support an alternative endpoint that includes the username at the end of the the URI.  This shortcut removes the need to include the username or address as an additional parameter.  

For example `/users/login/user/myusername` is a shortcut to `users/login/user?username=myusername`.

Similarly `/users/list/accounts/myusername` is a shortcut to `users/list/accounts?username=myusername`.

## `Methods`

The following methods are currently supported by this API

[`create/user`](#createuser)  
[`login/user`](#loginuser)   
[`logout/user`](#logoutuser)   
[`unlock/user`](#unlockuser)  
[`lock/user`](#lockuser)  
[`list/transactions`](#listtransactions)    
[`list/notifications`](#listnotifications)    
[`list/assets`](#listassets)    
[`list/accounts`](#listaccounts)    
[`list/tokens`](#listtokens)    


-----------------------------------
****



# `create/user`

This will create a new user account (signature chain) for use on the network. The user account is secured by a combination of username, password, and PIN.


### Endpoint:

`/users/create/user`


### Parameters:

`username` : The username to be associated with this user. The signature chain genesis (used to uniquely identify user accounts) is a hash of this username, therefore the username must be unique on the blockchain.

`password` : The password to be associated with this user.

`pin` : The PIN can be a combination of letters/numbers/symbols or could be tied into an external digital fingerprint. The PIN is required for all API calls that modify a user account (such as sending or claiming transactions).


### Return value JSON object:
```
{
    "genesis": "c8749c4e748f3a9a3ca2b3199ed34d9501ef8a5df07d9dadcbacbef74202deb9",
    "hash": "9652d37fcd0ebd9cea0b4ee282aacdb0751c8bc37c10e866f8a8e57f39f397680752ec5295b23fea0252234f494066ab2ded541709c8224f9ec30438b47c8691",
    "nexthash": "257a3811555410db58922e206d2fb9e727c00e63074966effa5acc4247879bcd",
    "prevhash": "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
    "pubkey": "031cdb48d689ac689e4d3cc7b3ff863d6cfb969623226584d7231b8842e514a357c5e1ed9784e96dc5496e04a39ac17ebe6b28cfb98416175980b34bb535ad5304",
    "sequence": 0,
    "signature": "30818402404117fdb407d26996f035e670e12f7ca820b073a7d172f6caf93ed8c7b20d5c99d1ffebdbc61ef348400b503c6305217da96fa01ed51da6279f4022293a74a6c102406a19edbb56427a58a2560b6048c2dfcfd120df2a0509f31420f9b6f9cb8864112a7dee458ad0505020fa81f293c31d2469cc7e3ebd3967b575df299f0b610993",
    "timestamp": 1545445970,
    "version": 1
}
```

### Return values:

`genesis` : The signature chain genesis hash. This is a hash of the username used to create the user.

`hash` : The hash of the transaction that created the user.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : The hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature` : The signature hash.

`sequence` : The sequence number of this transaction within the signature chain.

`timestamp` : The Unix timestamp of when the transaction was created.

`version` : The serialization version of the transaction.


****


# `login/user`

This will start a session for your user account with this specific API instance. Username, password, and pin fields are mandatory for login.   


### Endpoint:

`/users/login/user`


### Parameters:

`username` : The username associated with this signature chain.   

`password` : The password to be associated with this signature chain.

`PIN` : The PIN for this signature chain.


### Return value JSON object:    
```
{
    "genesis": "27ef3f31499b6f55482088ba38b7ec7cb02bd4383645d3fd43745ef7fa3db3d1",
    "session": "16953804956473956578"
}

```

### Return values:

`genesis` : The signature chain genesis hash. This is a hash of the username used to create the user.

`session` : When using multi-user API mode, an additional session value is returned
and must be supplied in subsequent API calls, to allow the managing of multiple login sessions.


****


# `logout/user`

This will log you out of this specific API, and delete your credentials stored
in encrypted memory.


### Endpoint:

`/users/logout/user`


### Parameters:

`session` : For multi-user API mode (configured with multiuser=1), the session is required to identify  which session you are logging out of. For single-user API mode the session should not be supplied.



### Return value JSON object:
```
{
    "genesis": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1"
}
```


****


# `unlock/user`

This will unlock your signature chain and cache the PIN in encrypted memory to be used for all subsequent API calls. This method is only available when using single-user API mode (multiuser=0).    

The PIN field is mandatory for unlock. The minting/transactions parameters are optional and, if supplied, allow the signature chain to be used for either mining/staking or creating transactions (or both) when in single-user API mode.

**NOTE** : If the minting/transactions parameters are not supplied then it is assumed that the signature chain can be used for both.
**NOTE** : If the Unlock method is called once with either `minting` or `transactions`=1 (but not both) then it can be called a second time to unlock the other option.  


### Endpoint:

`/users/unlock/user`


### Parameters:


`PIN` : The PIN for this signature chain.

`minting` : This boolean value determines whether the user account can be used for minting (staking or mining).

`transactions` : This boolean value determines whether the user account can be used for creating or claiming transactions.



### Return value JSON object:
```
{
    true
}
```

****


# `lock/user`

This will lock your signature chain, making it unavailable for use unless it is
either unlocked or the PIN is passed in to all API requests. Only available in single-user API mode (multiuser=0).

The minting/transactions parameters are optional and, if supplied, allow the signature chain to be locked for either mining/staking or creating transactions.  This allows, for example, a signature chain to be continually unlocked for minting, but only temporarily unlocked for sending a transaction.    

### Endpoint:

`/users/lock/user`


### Parameters:

`minting` : If set this will lock the users sig chain for minting (staking or mining).

`transactions` : If set this will lock the users sig chain from creating any transactions.



### Return value JSON object:
```
{
    true
}
```

****


# `list/transactions`

This will list off all of the transactions for the requested signature chain genesis or username (either can be used). You DO NOT need to be logged in to use this command. If you are using single-user API mode and are logged in, then neither username or genesis are required. It will return transactions for the currently logged in user.

**NOTE** : If you use the username parameter, it will take slightly longer to calculate the username genesis with our brute-force protected hashing algorithm. For higher performance, use the genesis parameter.


### Endpoint:

`/users/list/transactions`


### Parameters:


`genesis` : The genesis hash identifying the signature chain (optional if username is supplied).

`username` : The username identifying the signature chain (optional if genesis is supplied).

`page` : Allows the results to be returned by page (zero based). E.g. passing in page=1 will return the second set of (limit) transactions. The default value is 0 if not supplied.

`limit` : The number of transactions to return for the current page. The default is 100.

`verbose` : Optional, determines how much transaction data to include in the response. Supported values are :
 - `default` : hash
 - `summary` : type, version, sequence, timestamp, operation, and confirmations.
 - `detail` : genesis, nexthash, prevhash, pubkey and signature.



### Return value JSON object:
```
{
    "type": "tritium base",
    "genesis": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1",
    "hash": "f92fd92b7671d8358b82633fdf147e82a95ced60b54ceb4465fb34410345534bff3e9a3224032af08e01cb3b2c1dd4f6dcbb2a600e4b8b3c1109ea11b59ae7b8",
    "nexthash": "943aaa1f98c14852ff301c743bf25b962aaf5bc04912dd3e6fb925f1f7d8147e",
    "prevhash": "ef2cde465647953604d7bd49a76b5dde26b55eab8de88661451164f6a8d242f4282b3af9b1893e73a635b51c062a5c8f99217384bf05abbddbf55e1a0e86ffd5",
    "pubkey": "0338a2acd3dde1a845ac9b959797f891556747a317f6f71caea39977eb2f4d5b15d6cad37aa3a3acc61ed7306b06631a038f6fcd6b585c7fb1d55cd96acc82db7b",
    "sequence": 6,
    "signature": "30818402405f1b486b556912edc1b87139c822409e4be7e1aae73a92aadf4ad36f46c90d3c4190ee9bff04e6e79fe27fad4557670ea04533d1ccdb4581de252f04403e36b402401734b064192c106ff070d922349a11eec82c57942824a8b92e44fcb6f5702fa31f190946f9b3bb98815be12f98649cdf7b7204942d1437557635060fb91095c0",
    "timestamp": 1545445264,
    "version": 1
    "operation": [
            {
                "OP": "REGISTER",
                "address": "140e9de108b17a6c532e088cfa6ffa7f5075a10ef1daa07924ba3a27ff20edb9",
                "type": 4,
                "data": "0762616c616e6365ff040000000000000000057472757374ff040000000000000000057374616b65ff0400000000000000000d746f6b656e5f61646472657373050000000000000000000000000000000000000000000000000000000000000000"
            }
        ]
},
```

### Return values:

`type` : The description of the transaction (`legacy` | `tritium base` | `trust` | `genesis` | `user`).

`genesis` : The signature chain genesis hash.

`hash` : The transaction hash.

`confirmations` : The number of confirmations that this transaction obtained by the network.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : the hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature` : The signature hash.

`sequence` : The sequence number of this transaction within the signature chain.

`timestamp` : The Unix timestamp of when the transaction was created.

`version` : The serialization version of the transaction.

`operation` : The description of the opcodes included in the transaction.

****


# `list/notifications`

This will list off all of the transactions sent to a particular genesis or username. It is useful for identifying transactions that you need to accept such as credits.

**NOTE** : If you use the username parameter, it will take slightly longer to calculate the username genesis with our brute-force protected hashing algorithm. For higher performance, use the genesis parameter.


### Endpoint:

`/users/list/notifications`


### Parameters:

`genesis` : The genesis hash identifying the signature chain (optional if username is supplied).

`username` : The username identifying the signature chain (optional if genesis is supplied).

`page` : Allows the results to be returned by page (zero based).  E.g. passing in page=1 will return the second set of (limit) notifications.  The default value is 0 if not supplied.

`limit` : The number of notifications to return for the current page.  Default is 100


### Return value JSON object:
```
{
    "genesis": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1",
    "hash": "f92fd92b7671d8358b82633fdf147e82a95ced60b54ceb4465fb34410345534bff3e9a3224032af08e01cb3b2c1dd4f6dcbb2a600e4b8b3c1109ea11b59ae7b8",
    "nexthash": "943aaa1f98c14852ff301c743bf25b962aaf5bc04912dd3e6fb925f1f7d8147e",
    "prevhash": "ef2cde465647953604d7bd49a76b5dde26b55eab8de88661451164f6a8d242f4282b3af9b1893e73a635b51c062a5c8f99217384bf05abbddbf55e1a0e86ffd5",
    "pubkey": "0338a2acd3dde1a845ac9b959797f891556747a317f6f71caea39977eb2f4d5b15d6cad37aa3a3acc61ed7306b06631a038f6fcd6b585c7fb1d55cd96acc82db7b",
    "sequence": 6,
    "signature": "30818402405f1b486b556912edc1b87139c822409e4be7e1aae73a92aadf4ad36f46c90d3c4190ee9bff04e6e79fe27fad4557670ea04533d1ccdb4581de252f04403e36b402401734b064192c106ff070d922349a11eec82c57942824a8b92e44fcb6f5702fa31f190946f9b3bb98815be12f98649cdf7b7204942d1437557635060fb91095c0",
    "timestamp": 1545445264,
    "version": 1
    "operation": "Coinbase(nAmount=55)"
},
```

### Return values:

`genesis` : The signature chain genesis hash.

`hash` : The transaction hash.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : The hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature` : The signature hash.

`sequence` : The sequence number of this transaction within the signature chain.

`timestamp` : The Unix timestamp of when the transaction was created.

`version` : The serialization version of the transaction.

`operation` : The description of the opcodes included in the transaction.

****


# `list/assets`

This will list off all of the assets owned by a signature chain.

**NOTE** : If you use the username parameter, it will take slightly longer to calculate the username genesis with our brute-force protected hashing algorithm. For higher performance, use the genesis parameter.


### Endpoint:

`/users/list/assets`


### Parameters:

`genesis` : The genesis hash identifying the signature chain (optional if username is supplied).

`username` : The username identifying the signature chain (optional if genesis is supplied).

`page` : Allows the results to be returned by page (zero based).  E.g. passing in page=1 will return the second set of (limit) results.  The default value is 0 if not supplied.

`limit` : The number of results to return for the current page.  Default is 100


### Return value JSON object:
```
[
    {
        "address": "8195384518cc4daf2da54479a38c9d93fde145be0bbcd129f420054ff777917c",
        "timestamp": "1553227256"
        "owner": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1"
        "serial_number": "123456",
        "description": "This is the description of my asset",
        "shelf_location": "A9S2"
    },
    {
        "address": "2673bd356146d1b7ab34dac8f6503cd0c587158b6a017fb9e8ff3e627be8f01c",
        "timestamp": "1553227128"
        "owner": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1"
        "serial_number": "78901234",
        "description": "This is the description of another asset",
        "shelf_location": "A3S6"
    }
]
```
### Return values:

`address` : The register address of the asset. 

`timestamp` : The Unix timestamp of when the asset was last updated.

`owner` : The genesis hash of the signature chain that owns this asset.

`<fieldname>=<value>` : The key-value pair for each piece of data stored in the asset.

****

# `list/accounts`

This will list off all of the token accounts belonging to a particular signature chain.

**NOTE** : If you use the username parameter, it will take slightly longer to calculate the username genesis with our brute-force protected hashing algorithm. For higher performance, use the genesis parameter.


### Endpoint:

`/users/list/accounts`


### Parameters:

`genesis` : The genesis hash identifying the signature chain (optional if username is supplied).

`username` : The username identifying the signature chain (optional if genesis is supplied).

`page` : Allows the results to be returned by page (zero based).  E.g. passing in page=1 will return the second set of (limit) results.  The default value is 0 if not supplied.

`limit` : The number of results to return for the current page.  Default is 100



### Return value JSON object:
```
[
    {
        "address": "f9ee3e63c845794d577d54c5087f0ff2f2053195bf007907e8134977bb1f1e24",
        "token_address": "0",
        "balance": 5000
    },
    {
        "address": "a74774dc075e59d03639b56da8c29736fe248888a43c6103060409dd11273417",
        "token_address": "f9ee3e63c845794d577d54c5087f0ff2f2053195bf007907e8134977bb1f1e24",
        "balance": 60
    }


]
```
### Return values:

`address` : The register address for the account

`token_address` : The register address of the token that this account can be used for.

`balance` : The current balance of the token account.

****

# `list/tokens`

This will list off all of the tokens that were created by a particular signature chain.

**NOTE** : If you use the username parameter, it will take slightly longer to calculate the username genesis with our brute-force protected hashing algorithm. For higher performance, use the genesis parameter.


### Endpoint:

`/users/list/tokens`


### Parameters:

`genesis` : The genesis hash identifying the signature chain (optional if username is supplied).

`username` : The username identifying the signature chain (optional if genesis is supplied).

`page` : Allows the results to be returned by page (zero based).  E.g. passing in page=1 will return the second set of (limit) results.  The default value is 0 if not supplied.

`limit` : The number of results to return for the current page.  Default is 100



### Return value JSON object:
```
[
    {
        "address": "f9ee3e63c845794d577d54c5087f0ff2f2053195bf007907e8134977bb1f1e24",
        "balance": 0,
        "maxsupply": 26738388,
        "currentsupply": 26738388
    },
    {
        "address": "a74774dc075e59d03639b56da8c29736fe248888a43c6103060409dd11273417",
        "balance": 990,
        "maxsupply": 1000,
        "currentsupply": 10
    }


]
```
### Return values:

`address` : The register address for the account

`balance` : The current balance of the token account.

`maxsupply` : The maximum available supply for this token.

`currentsupply` : The current number of tokens that have been distributed from this token register.

****
