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
[`list/accounts`](#listaccounts)   
[`list/assets`](#listassets)    
[`list/items`](#listitems)    
[`list/names`](#listnames)    
[`list/namespaces`](#listnamespaces)  
[`list/notifications`](#listnotifications)  
[`list/tokens`](#listtokens)  
[`list/transactions`](#listtransactions) 

 


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
        "name": "myasset1",
        "address": "8195384518cc4daf2da54479a38c9d93fde145be0bbcd129f420054ff777917c",
        "timestamp": "1553227256"
        "owner": "bf501d4f3d81c31f62038984e923ad01546ff678e305a7cc11b1931742524ce1"
        "serial_number": "123456",
        "description": "This is the description of my asset",
        "shelf_location": "A9S2"
    },
    {
        "name": "myasset2",
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

`created` : The UNIX timestamp when the asset was created.

`modified` : The UNIX timestamp when the asset was last modified.

`name` : The name identifying the asset.  For privacy purposes, this is only included in the response if the caller is the owner of the asset

`address` : The register address of the asset.

`timestamp` : The Unix timestamp of when the asset was last updated.

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
        "name": "default",
        "address": "f9ee3e63c845794d577d54c5087f0ff2f2053195bf007907e8134977bb1f1e24",
        "token_name": "NXS",
        "token": "0",
        "balance": 5000
    },
    {
        "name": "savings",
        "address": "a74774dc075e59d03639b56da8c29736fe248888a43c6103060409dd11273417",
        "token_name": "mytoken",
        "token": "f9ee3e63c845794d577d54c5087f0ff2f2053195bf007907e8134977bb1f1e24",
        "balance": 60
    }


]
```
### Return values:

`created` : The UNIX timestamp when the account was created.

`modified` : The UNIX timestamp when the account was last modified.

`name` : The name identifying the account.  For privacy purposes, this is only included in the response if the caller is the owner of the account

`address` : The register address for the account

`token_name` : The name of the token that this account can be used with.  For privacy purposes, this is only included in the response if the caller is the owner of the token

`token` : The register address of the token that this account can be used for.

`balance` : The current balance of the token account.


****


# `list/items`

This will list off all of the supply chain items (append registers) owned by a signature chain.

**NOTE** : If you use the username parameter, it will take slightly longer to calculate the username genesis with our brute-force protected hashing algorithm. For higher performance, use the genesis parameter.


### Endpoint:

`/users/list/items`


### Parameters:

`genesis` : The genesis hash identifying the signature chain (optional if username is supplied).

`username` : The username identifying the signature chain (optional if genesis is supplied).

`page` : Allows the results to be returned by page (zero based).  E.g. passing in page=1 will return the second set of (limit) results.  The default value is 0 if not supplied.

`limit` : The number of results to return for the current page.  Default is 100


### Return value JSON object:
```
[
    {
        "created": 1560982537,
        "modified": 1560982615,
        "name": "myitem",
        "address": "70f49d996d53317cf2175b89efc2694b85c4ebede5fc468bedb860b3e4dada68",
        "owner": "2be51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0",
        "data": "xxxxxx"
    }
]

```
### Return values:

`created` : The UNIX timestamp when the item was created.

`modified` : The UNIX timestamp when the item was last modified.

`name` : The name identifying the item.  For privacy purposes, this is only included in the response if the caller is the owner of the item

`address` : The register address of the item.

`owner` : The genesis hash of the owner of the item.

`data` : The data stored in the item.


****


# `list/names`

This will list off all of the names owned by the signature chain.  For privacy reasons Names are only returned for the currently logged in user (multiuser=0) or for the logged in session (multiuser=1)

**NOTE** : If you use the username parameter, it will take slightly longer to calculate the username genesis with our brute-force protected hashing algorithm. For higher performance, use the genesis parameter.


### Endpoint:

`/users/list/names`


### Parameters:

`session` : For multi-user API mode (configured with multiuser=1), the session is required to identify  which signature chain should be used to return the Names for. For single-user API mode the session should not be supplied an the currently logged in users signature chain is used.

`page` : Allows the results to be returned by page (zero based).  E.g. passing in page=1 will return the second set of (limit) results.  The default value is 0 if not supplied.

`limit` : The number of results to return for the current page.  Default is 100



### Return value JSON object:
```
[
    {
        "name": "asset1.ourcompany",
        "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9",
        "register_address": "597164ddc65ed8e4e481958d578daf0890a625a4957a987c5e966fcfff7084f3"
    }
    {
        "name": "asset2.ourcompany",
        "address": "a74774dc075e59d03639b56da8c29736fe248888a43c6103060409dd11273417",
        "register_address": "55c559eabe469eb566f302712edf089b165001da8dfda7acdcf9347896e1fff4"
    },
    {
        "name": "default",
        "address": "97c2e9fe863e51adeee82b28bf43747291ca801e258a98453d79edb261a02e7a",
        "register_address": "2be51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0"
    }


]
```
### Return values:

`created` : The UNIX timestamp when the Name was created.

`modified` : The UNIX timestamp when the Name was last modified.

`name` : The name identifying the object register. 

`address` : The register address of the Name.  

`register_address` : The register address of the the object that this Name points to.


****


# `list/namespaces`

This will list off all of the namespaces owned by the signature chain.

**NOTE** : If you use the username parameter, it will take slightly longer to calculate the username genesis with our brute-force protected hashing algorithm. For higher performance, use the genesis parameter.


### Endpoint:

`/users/list/namespaces`


### Parameters:

`genesis` : The genesis hash identifying the signature chain (optional if username is supplied).

`username` : The username identifying the signature chain (optional if genesis is supplied).

`page` : Allows the results to be returned by page (zero based).  E.g. passing in page=1 will return the second set of (limit) results.  The default value is 0 if not supplied.

`limit` : The number of results to return for the current page.  Default is 100



### Return value JSON object:
```
[
    {
        "name": "mynamespace1",
        "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9"
    },
    {
        "name": "mynamespace2",
        "address": "a74774dc075e59d03639b56da8c29736fe248888a43c6103060409dd11273417"
    }


]
```
### Return values:

`created` : The UNIX timestamp when the namespace was created.

`modified` : The UNIX timestamp when the namespace was last modified.

`name` : The name identifying the namespace. 

`address` : The register address of the namespace.  


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
[
    {
        "txid": "0afb32d8316c9ad97504fbf26093a86cbce03ba8137c5f63e16991e3a99aee3cbb5696f0c889a75fe61541898d350f4023edca2e2b66545f7c2ef9af254f99f0",
        "contracts": [
            {
                "OP": "DEBIT",
                "from": "6118a477d187277d167bd7965d89d228a4fd04bdc48ae83157554f074eca8e61",
                "to": "464f3ab4a704a100e6413c2891efaba61375513b00bdc903d8a1f32500f01e36",
                "amount": 10000
            }
        ]
    },
]

```

### Return values:

`txid` : The transaction hash.

`contracts` : The list of contracts bound to this transaction and their details with opcodes.


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
        "name": "mytoken1",
        "address": "f9ee3e63c845794d577d54c5087f0ff2f2053195bf007907e8134977bb1f1e24",
        "balance": 0,
        "maxsupply": 26738388,
        "currentsupply": 26738388,
        "digits": 2
    },
    {
        "name": "mytoken2",
        "address": "a74774dc075e59d03639b56da8c29736fe248888a43c6103060409dd11273417",
        "balance": 990,
        "maxsupply": 1000,
        "currentsupply": 10,
        "digits": 8
    }


]
```
### Return values:

`created` : The UNIX timestamp when the token was created.

`modified` : The UNIX timestamp when the token was last modified.

`name` : The name identifying the token.  For privacy purposes, this is only included in the response if the caller is the owner of the token

`address` : The register address for the account

`balance` : The current balance of the token account.

`maxsupply` : The maximum available supply for this token.

`currentsupply` : The current number of tokens that have been distributed from this token register.

`digits` : The maximum number of decimal places that can be applied to token amounts.  For example digits=2 will allow a token amount to be given to 2 decimal places.


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
    "txid": "222fafd56cf1fa5dc8d274d83dc49d1fcd4ecee958c62338fa865c7d43e5ed9f0abb843fdbf5b161dd1d1b9e2e64a9a1cb3a32657b8559b03ef60210257f8206",
    "contracts": [
        {
            "OP": "COINBASE",
            "genesis": "1ff463e036cbde3595fbe2de9dff15721a89e99ef3e2e9bfa7ce48ed825e9ec2",
            "nonce": 3,
            "amount": 76499504
        }
    ],
    "type": "tritium base",
    "version": 1,
    "sequence": 10,
    "timestamp": 1560196174,
    "confirmations": 6,
    "genesis": "1ff463e036cbde3595fbe2de9dff15721a89e99ef3e2e9bfa7ce48ed825e9ec2",
    "nexthash": "117ac949f88a65f6466e2b332e88abb1b7548cc7a09fdf6512ef4d96b44c1b2e",
    "prevhash": "cdfd0721fd860f52046a4752c6e67a30b11541662f89815091800f54c994455ecf20994651c38d32c0d8a3c3affc32d88977987c25af30c4c907b9666802c229",
    "pubkey": "095afa24f8dcdf15ac8c7701d7a0429ca866b2881ad5a2e59af77d8c6ebeb04a150b5ecc23d4c60593d583022cefea32cdca51f8dd476653208c72a14150ae85567e90f90e649a67fd1a32e501ea135ee88859e2606aa8ec22f524e784c8e9e9722401512028878fa1a56606e4481641b58295d6879e665f1e180777d011e709d120b1214f647bf68713be98c0b968d86815c22a1d9a7128e780e910c8ad82c595b20504772b8c842507d4a26a9345500829a1d4b28665507e1789a526eb6be60662fb4da11e0cd50bd78e698ba0f4220b1798bc64492910dc84876a6c95ace3563e7a23da96627d7b04a1284606d029567bb26a0a412a4a8a82a09dc7062968894b11c22410d8711b90e33f4d20a5385a969d69b05118d2d17585c999e1ef35a97444c8b2da5b54c1ce556da90477c2020bb780e402daa0f4bdee92b5745433f71e4a30a42816f518e2a0da4d38fa49ce136e18aadfcef9463ad641e05b4220f54139605c5328c27fd0d5cf1329f8928094aeebbd70f116e8271e7bb280e7f4aac58115bb0064920a36fcb028f50b0b376553a6af00334899abe5be0cad14b435b377c6514680a58072691947181fa0a85b70ad15e585265e7f8bc6c22954a5bf95065815cc18771eb1a486ad6810b80c4a38f92716500220804324ff01728e3b4e87b9a34984a1456f7ab14459e4859e62961eedbb788e820b4580d12fe0266aec3a82da88c190b94d715a01793c69bf16d94bb89abc5e116f03792aa2e8306604ce559415f74948ebb3e63b8371d0e58a0d5b2798aadeb4a5dafbd44a4ca27e0a523e57a49600c826208fe4932848eb82a7c96b32bb19bb5a2e1c64ffa31756d484eddd521eae75bcf633227783d7ad0150c6198c2754c2e166e34ac00cc488573d40b82ad650e25221e414b419c66450ec266516605759e985ec1cd3f7ebc89525cf67aa2401a3cc2469e686a97f00e47484499554fa2917616e9d9c5d6f42f9d32438910fd1c00bc10a2a9234856ec6021aa69b76da10299e73540ce3175328ccb68c0b1d1878e584bd40b60af0c323c49aad7efef783d5049dae18a53794a5432f30b6c4ec6e0a43dc11f5d869f04dbe5eb963240e716ce220c75a165e6a8b5b33cb318c985e50e5d26aa9c25c2b746b4f3edcc318cae0b060a8dcc628878070b60c24f9e35a9596a0c4e1278d1a9d5b33c8e76984340cf7ec991e767c071b47d47e2ba89cb99145bcae716f402fcdb5416ab7d3b0d50b1ec4399a076faa",
    "signature": "29381098b52dd52a0f4d2f82ec4fcab2c5044ad4f9bb299c713be43345b4c215c8125936ab3445a6bde280e5f22bbf35d588458ed5169f25f0711efc2299bc6d7885c96c4cd978149dd5a1bc3b554e9bc3b8c5cbaf1d3185202ca3778c42d1d8161eeab49c05b3733e90959e7d626e96b75adc5ba18e514f36cac68ad111e8f1a8e9a2710cd5252cc54121d0b39459ec49662531a250e64e1d2571c863c8697dc91f04551dfb3517555e91014f4a36a2763452ac1698d32cadd727b1dca320c6c0efc0a6cc63f8d8e850ec5592b0d6358b25fa0246f50a7ba89161cab487f4d6925f43a3978367300c59f1f64bac24cdb1ea6ce3d7457620c9b2c508a6619f29a91a19f816393b57503ff7059954dac507fd5ab756ee1c5b861352b9e5086fb6fe388c317e59778c79a7a1ccd2152543bf56ed58c8cfefb788ecef56c9ae84ab26f71545926d1c26b91a6bc20896b99087c3eb36720c9998cb7018c2247d321c7eadf4c6d1def93b451c4ddb94123ef8691fa83f81eddc426a5cc1afbeb1f615331a97eda571e57b8cf2b46eb25cdd19691d3b8723786e9b4476afdb7e99adc56e5d237d33311f09162c74fc24c5386458a4290a7d1b8b870fdeae68e689675b5290c9704d7f1323d1657bdc2c6be2914a7de447d7c475f8ce74487b6225958831cdbbc1887af2c8016141568bd51e039c648c6541578067b2c626fe40455791ae345d0d5702356567fcda5c24365ed6b039cd9b0286dffad2c982044ae2f0b38e76deac4b19e5bac5773e45eaa78125b13df9db726cffe8c6e266cb68520ce3febf4a66fd66225f639e095ea6bdb833529a5f2bb1c9b1c544c17dee717256b471ab703d11541d55f0f78bd60d7e671f9a22a2328b70fb034b8f6459f155a17c21ad2750ce7282257980ba571c6296"
},

```

### Return values:

`txid` : The transaction hash.

`contracts` : The list of contracts bound to this transaction and their details with opcodes.

`type` : The description of the transaction (`legacy` | `tritium base` | `trust` | `genesis` | `user`).

`version` : The serialization version of the transaction.

`sequence` : The sequence number of this transaction within the signature chain.

`timestamp` : The Unix timestamp of when the transaction was created.

`confirmations` : The number of confirmations that this transaction obtained by the network.

`genesis` : The signature chain genesis hash.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : the hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature` : The signature hash.

****