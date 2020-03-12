Tokens API
-----------------------------------

The Tokens API provides methods for creating fungible tokens, creating accounts to store them in, and sending and receiving tokens between users.


## `Named Shortcuts`

For each API method we support an alternative endpoint that includes the token/account name or register address at the end of the URI.  This shortcut removes the need to include the name name or address as an additional parameter.  

For example `tokens/create/account/savings` is a shortcut to `tokens/create/account?name=savings`.

Similarly `tokens/get/token/5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9` is a shortcut to `tokens/get/token?address=5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9`  

The logic for resolving the shortcut to either a name or address is that if the data is 64 characters of hexadecimal then it will be assumed to be a register address.  Otherwise it will be considered a name.  


## `Methods`

The following methods are currently supported by this API

[`create/token`](#createtoken)   
[`debit/token`](#debittoken)      
[`credit/token`](#credittoken)    
[`get/token`](#gettoken)  
[`list/token/transactions`](#listtokentransactions)   
[`create/account`](#createaccount)   
[`debit/account`](#debitaccount)   
[`credit/account`](#creditaccount)   
[`get/account`](#getaccount)   
[`list/account/transactions`](#listaccounttransactions)   

-----------------------------------
***

# `create/token`

Create a new token object register. The API supports an alternative endpoint that can include the new token name in the URI.  For example `/tokens/create/token/mytoken` will resolve to `tokens/create/token?name=mytoken`.   


### Endpoint:

`/tokens/create/token`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the token should be created with. For single-user API mode the session should not be supplied.

`name` : An optional name to identify the token. If provided a Name object will also be created in the users local namespace, allowing the token to be accessed/retrieved by name. If no name is provided the token will need to be accessed/retrieved by its 256-bit register address.

`supply` : The initial token supply amount.  Must be a whole number amount.

`decimals` : The maximum number of decimal places that can be applied to token amounts.  For example decimals=2 will allow a token amount to be given to 2 decimal places.

### Example:

```
{
    "pin": "1234",
    "name": "ABC",
    "supply": 1000000,
    "decimals": 4
}
```

### Return value JSON object:
```    
{
    "txid": "f9dcd28bce2563ab288fab76cf3ee5149ea938c735894ce4833b55e474e08e8a519e8005e09e2fc19623577a8839a280ca72b6430ee0bdf13b3d9f785bc7397d",
    "address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P"
}
```   

### Return values:

`txid` : The ID (hash) of the transaction that includes the token creation.

`address` : The register address for this token. The address (or name that hashes to this address) is needed when creating token accounts, so that we know which token the account can be used with.

***

# `debit/token`

Deduct a token amount from a token's initial supply and send it to a token account. The API supports an alternative endpoint that can include the token name in the URI.  For example `/tokens/debit/token/mytoken` will resolve to `tokens/debit/token?name=mytoken`.   
Any fee that is applicable fo this transaction is deducted from the `default` account.

The method supports the ability to send to multiple recipients in one transaction by supplying a `recipients` array instead of the single `name_to`/`address_to`/`amount`. The method allows up to 99 recipients to be supplied.  

### Endpoint:

`/tokens/debit/token`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the token owner. For single-user API mode the session should not be supplied.

`name` : The name identifying the token to debit. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the token was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the token to debit. This is optional if the name is provided.

`amount` : The amount of tokens to debit.

`name_to` : The name identifying the token account to send to. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address_to` : The register address of the token account to send to. This is optional if the name is provided.

`reference` : This optional field allows callers to provide a reference, which the recipient can then use to relate the transaction to an order number, invoice number etc.  The reference is be a 64-bit unsigned integer in the range of 0 to 18446744073709551615

`expires` : This optional field allows callers to specify an expiration for the debit transaction. The expires value is the `number of seconds` from the transaction creation time after which the transaction can no longer be credited by the recipient.  Conversely, when you apply an expiration to a transaction, you are unable to void the transaction until after the expiration time.  If omitted, a default expiration of 1 day (86400 seconds) is applied 

`recipients` : This optional array can be provided as an alternative to the single `name_to`/`address_to`, `amount`, `reference`, and `expires` fields.  Each object in the array can have `name_to`/`address_to`, `amount`, `reference`, and `expires` fields, as described above.  Up to 99 recipients can be included in the array.

### Example:

The following example shows the json payload for a debit of 10.5 ABC tokens (that itself was created by the sig chain logged into session "5e9d8aa625a1838f60f30e12058089169e32c968389f365428f7b0c878bb47f8") being sent to a token account identified by the name "jack:savings".  In the name_to field, "jack" refers to the username of the account holder and "savings" is their token account.  If jack is concerned about privacy he could have provided the register address of his token account instead

```
{
    "pin": "1234",
    "session": "5e9d8aa625a1838f60f30e12058089169e32c968389f365428f7b0c878bb47f8",
    "name": "bob:ABC",
    "amount": 10.5,
    "name_to": "jack:savings"
}
```

### Example Multiple Recipients:

The following example shows the json payload for a debit from a token called "ABC" made to 3 separate recipients, each receiving 5 ABC tokens, and each with a different reference  

```
{
    "pin": "1234",
    "name": "ABC"
    "recipients" :
    [
        {
            "name_to": "bob:abc",
            "amount": 5,
            "reference": 1234
        },
        {
            "address_to": "8CHjS5Qe7Mghgrqb6NeEaVxjexbdp9p2QVdRTt8W4rzbWhu3fL8",
            "amount": 5,
            "reference": 5678
        },
        {
            "address_to": "8CM4SYrd6ohcBB8tfqyZG3MCtDStMUXN37gwC2RtkNSUayM2FSN",
            "amount": 5
        }
    ]
}
```

### Return value JSON object:
```    
{
    "txid": "f9dcd28bce2563ab288fab76cf3ee5149ea938c735894ce4833b55e474e08e8a519e8005e09e2fc19623577a8839a280ca72b6430ee0bdf13b3d9f785bc7397d"
}
```   

### Return values:

`txid` : The ID (hash) of the transaction that includes the token debit.

****

# `credit/token`

Increment the token balance by an amount received from a token account. This method allows fungible tokens to be recycled back to the token creator in circumstances where they have reached the end of their life-cycle, for example a loyalty token that has been used by the customer to pay for a product can be debited from the customer's account and back to the token for reuse.  The API supports an alternative endpoint that can include the token name to credit in the URL.  For example `/tokens/credit/token/mytoken` will resolve to `tokens/credit/token?name=mytoken`.   


### Endpoint:

`/tokens/credit/token`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the token account. For single-user API mode the session should not be supplied.

`name` : The name identifying the token to credit. This is optional if the address is provided.  `name` : The name identifying the token to debit. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the token was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the token to credit. This is optional if the name is provided.

`txid` : The transaction ID (hash) of the corresponding debit transaction for which you are creating this credit for.



### Example:

The following example shows the json payload for a credit to a token called "mytoken" (that itself was created by the sig chain logged into session "5e9d8aa625a1838f60f30e12058089169e32c968389f365428f7b0c878bb47f8").  

```
{
    "pin": "1234",
    "session": "5e9d8aa625a1838f60f30e12058089169e32c968389f365428f7b0c878bb47f8",
    "name": "mytoken",
    "txid": "f9dcd28bce2563ab288fab76cf3ee5149ea938c735894ce4833b55e474e08e8a519e8005e09e2fc19623577a8839a280ca72b6430ee0bdf13b3d9f785bc7397d"
}
```

### Return value JSON object:
```    
{
    "txid": "318b86d2c208618aaa13946a3b75f14472ebc0cce9e659f2830b17e854984b55606738f689d886800f21ffee68a3e5fd5a29818e88f8c5b13b9f8ae67739903d"
}
```   

### Return values:

`txid` : The ID (hash) of the transaction that includes the token credit.

****

# `get/token`

Retrieves information about a token and its supply. The API supports an alternative endpoint that can include the token name in the URI.  For example `/tokens/get/token/mytoken` will resolve to `tokens/get/token?name=mytoken`.   

Additionally the API supports passing a field name in the URL after the token name, which will populate the `fieldname` parameter in the request.  For example `/tokens/get/token/mytoken/supply` will resolve to `tokens/get/token?name=mytoken&fieldname=supply`

### Endpoint:

`/tokens/get/token`


### Parameters:


`name` : The name identifying the token to retrieve. This is optional if the address is provided.  `name` : The name identifying the token to debit. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the token was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the name in order to deduce the register address of the token.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

`address` : The register address of the token to retrieve. This is optional if the name is provided.

`count` : Optional boolean field that determines whether the response includes the transaction `count` field.  This defaults to false, as including the transaction count can slow the response time of the method considerably.   

`fieldname`: This optional field can be used to filter the response to return only a single field from the token.

### Return value JSON object:
```    
{
    "owner": "a2e51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0",
    "created": 1566534164,
    "modified": 1566616211,
    "name": "mytoken",
    "address": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P",
    "balance": 990,
    "pending": 0,
    "unconfirmed": 0,
    "maxsupply": 1000,
    "currentsupply": 10,
    "decimals": 0
}
```   

### Return values:

`owner` : The genesis hash of the signature chain that owns this token.

`created` : The UNIX timestamp when the token was created.

`modified` : The UNIX timestamp when the token was last modified.

`name` : The name identifying the token. For privacy purposes, this is only included in the response if the caller is the owner of the token

`address` : The register address of the token.    

`balance` : The available balance of this token.  Initially the balance of the token is equal to the max supply, until the tokens have been distributed to accounts. This is the last confirmed balance less any new debits that you have made since the last block.

`pending` : This is the sum of all confirmed debit transactions that have been made to this token, that have not yet been credited.  To move tokens from pending into the available balance you must create a corresponding credit transaction.  NOTE: if configured to run, the events processor does this for you.

`unconfirmed` : This is the sum of all unconfirmed debit transactions that have been made to this token PLUS the sum of all unconfirmed credits that you have for confirmed debit transactions.  When someone makes a debit to the token it will immediately appear in the unconfirmed balance until that transaction is included in a block, at which point it moves into `pending`.  When you (or the events processor) creates the corresponding credit transaction for that debit, the amount will move from `pending` back into `unconfirmed` until the credit transaction is included in a block, at which point the amount moves to `balance`.  

`maxsupply` : The maximum token supply amount.

`currentsupply` : The current amount of tokens that have been distributed to token accounts.

`decimals` : The maximum number of decimal places that can be applied to token amounts.  For example decimals=2 will allow a token amount to be given to 2 decimal places.

`count` : Only returned if the caller requested `count` : true.  This is the number of transactions made to/from the token.

****


# `list/token/transactions`

This will list off all of the transactions related to a given token. You DO NOT need to be logged in to use this command. If you are logged in, then neither username or genesis are required as it will default to the logged in user. 

**NOTE** : The returned transaction data will only include contracts that related to the requested token.  Any other contracts are omitted from the transaction result. 

**NOTE** : If you use the username parameter it will take slightly longer to calculate the username genesis with our brute-force protected hashing algorithm. For higher performance, use the genesis parameter.


### Endpoint:

`/tokens/list/token/transactions`


### Parameters:


`genesis` : The genesis hash identifying the signature chain to scan for transactions (optional if username is supplied or already logged in).

`username` : The username identifying the signature chain to scan for transactions(optional if genesis is supplied or already logged in).

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the token name in order to deduce the register address of the token.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

`name` : The name identifying the token to list transactions for. This is optional if the address is provided. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the object was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the token to list transactions for. This is optional if the name is provided.

`page` : Allows the results to be returned by page (zero based). E.g. passing in page=1 will return the second set of (limit) transactions. The default value is 0 if not supplied.

`limit` : The number of transactions to return for the current page. The default is 100.

`verbose` : Optional, determines how much transaction data to include in the response. Supported values are :
 - `default` : hash, contracts
 - `summary` : type, version, sequence, timestamp, operation, and confirmations.
 - `detail` : genesis, nexthash, prevhash, pubkey and signature.



### Return value JSON object:
```
[
    {
        "txid": "01034b39cb3635d370f97339e6f87b8751d4c0d62676da7d6ec20416966f298f47dea99603d03a74e638b0d50b31b1e721790e5b103abfe3353a709ccf5d1e7c",
        "contracts": [
            {
                "OP": "CREDIT",
                "txid": "01e73b498dbabbf4629ad674b9ae3824b96cca83199c25a67901db53b271d19acf1411b0c4f9a3d8ded80860ffe2dcf683d2d227a675d453303b31f86f868f9e",
                "contract": 0,
                "proof": "8FJxzexVDUN5YiQYK4QjvfRNrAUym8FNu4B8yvYGXgKFJL8nBse",
                "to": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P",
                "to_name": "main",
                "amount": 76.450762,
                "token": "8FJxzexVDUN5YiQYK4QjvfRNrAUym8FNu4B8yvYGXgKFJL8nBse",
                "token_name": "XXX"
            },
            {
                "OP": "FEE",
                "from": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P",
                "from_name": "default",
                "amount": 0.1
            }
        ]
    }
]

```

### Return values:

`txid` : The transaction hash.

`type` : The description of the transaction (`legacy` | `tritium base` | `trust` | `genesis` | `user`).

`version` : The serialization version of the transaction.

`sequence` : The sequence number of this transaction within the signature chain.

`timestamp` : The Unix timestamp of when the transaction was created.

`blockhash` : The hash of the block that this transaction is included in.  Blank if not yet included in a block.

`confirmations` : The number of confirmations that this transaction obtained by the network.

`genesis` : The signature chain genesis hash.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : the hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature` : The signature hash.

`contracts` : The array of contracts bound to this transaction and their details with opcodes.    
{    
&nbsp;&nbsp;&nbsp;`id` : The sequential ID of this contract within the transaction.

&nbsp;&nbsp;&nbsp;`OP` : The contract operation. Can be `APPEND`, `CLAIM`, `COINBASE`, `CREATE`, `CREDIT`, `DEBIT`, `FEE`, `GENESIS`, `LEGACY`, `TRANSFER`, `TRUST`, `STAKE`, `UNSTAKE`, `WRITE`.

&nbsp;&nbsp;&nbsp;`for` : For `CREDIT` transactions, the contract that this credit was created for . Always `DEBIT` for token transactions.

&nbsp;&nbsp;&nbsp;`txid` : The transaction that was credited / claimed.

&nbsp;&nbsp;&nbsp;`contract` : The ID of the contract within the transaction that was credited / claimed.

&nbsp;&nbsp;&nbsp;`proof` : The register address proving the credit.

&nbsp;&nbsp;&nbsp;`from` : For `DEBIT`, `CREDIT`, `FEE` transactions, the register address of the account that the debit is being made from.

&nbsp;&nbsp;&nbsp;`from_name` : For `DEBIT`, `CREDIT`, `FEE` transactions, the name of the account that the debit is being made from.  Only included if the name can be resolved.

&nbsp;&nbsp;&nbsp;`to` : For `DEBIT` and `CREDIT` transactions, the register address of the recipient account.

&nbsp;&nbsp;&nbsp;`to_name` : For `DEBIT` and `CREDIT` transactions, the name of the recipient account.  Only included if the name can be resolved.

&nbsp;&nbsp;&nbsp;`amount` : the token amount of the transaction.

&nbsp;&nbsp;&nbsp;`token` : the register address of the token that the transaction relates to.  Set to 0 for NXS transactions

&nbsp;&nbsp;&nbsp;`token_name` : The name of the token that the transaction relates to.

&nbsp;&nbsp;&nbsp;`reference` : For `DEBIT` and `CREDIT` transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number.

}

***

# `create/account`

Create a new token account for receiving tokens. The API supports an alternative endpoint that can include the account name in the URI.  For example `/tokens/create/account/savings` will resolve to `tokens/create/account?name=savings`.   


### Endpoint:

`/tokens/create/account`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the token account should be created with. For single-user API mode the session should not be supplied.

`name` : An optional name to identify the account. If provided a Name object will also be created in the users local namespace, allowing the account to be accessed/retrieved by name. If no name is provided the account will need to be accessed/retrieved by its 256-bit register address.

`token_name` : This is the name of the token that the account can be used for.  The token name should be supplied in the format `username:tokenname`.  This is optional if the token address is supplied.

`token` : This is the address of the token that the account can be used for.  This is optional if token_name is supplied.



### Example:

This example creates a token account called "main" for a token identified by the token name.  NOTE the name has been qualified with a namespace which is the username of the signature chain that created the token

```
{
    "pin": "1234",
    "name": "main",
    "token_name": "bob:ABC"
}
```

### Example:

This example creates a token account called "savings" for a token identified by its register address.

```
{
    "pin": "1234",
    "name": "savings",
    "token": "8CvLySLAWEKDB9SJSUDdRgzAG6ALVcXLzPQREN9Nbf7AzuJkg5P"
}
```

### Return value JSON object:
```    
{
    "txid": "f9dcd28bce2563ab288fab76cf3ee5149ea938c735894ce4833b55e474e08e8a519e8005e09e2fc19623577a8839a280ca72b6430ee0bdf13b3d9f785bc7397d",
    "address": "8FJxzexVDUN5YiQYK4QjvfRNrAUym8FNu4B8yvYGXgKFJL8nBse"
}
```   

### Return values:

`txid` : The ID (hash) of the transaction that includes the account creation.

`address` : The register address for this token account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.    


***

# `debit/account`

Deduct a token amount from a token account and send it to another token account. The API supports an alternative endpoint that can include the account name in the URI.  For example `/tokens/debit/account/savings` will resolve to `tokens/debit/account?name=savings`.   

Any fee that is applicable fo this transaction is deducted from the `default` account.

The method supports the ability to send to multiple recipients in one transaction by supplying a `recipients` array instead of the single `name_to`/`address_to`/`amount`. The method allows up to 99 recipients to be supplied.  

### Endpoint:

`/tokens/debit/account`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) that owns the token account. For single-user API mode the session should not be supplied.

`name` : The name identifying the token account to debit. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the token account to debit. This is optional if the name is provided.

`amount` : The amount of tokens to debit.

`name_to` : The name identifying the token account to send to. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address_to` : The register address of the token account to send to. This is optional if the name is provided.

`reference` : This optional field allows callers to provide a reference, which the recipient can then use to relate the transaction to an order number, invoice number etc.  The reference is be a 64-bit unsigned integer in the range of 0 to 18446744073709551615

`expires` : This optional field allows callers to specify an expiration for the debit transaction. The expires value is the `number of seconds` from the transaction creation time after which the transaction can no longer be credited by the recipient.  Conversely, when you apply an expiration to a transaction, you are unable to void the transaction until after the expiration time.  If omitted, a default expiration of 1 day (86400 seconds) is applied 

`recipients` : This optional array can be provided as an alternative to the single `name_to`/`address_to`, `amount`, `reference`, and `expires` fields.  Each object in the array can have `name_to`/`address_to`, `amount`, `reference`, and `expires` fields, as described above.  Up to 99 recipients can be included in the array.

### Example:

The following example shows the json payload for a debit of 10.5 tokens from an account called "main" (that itself was created by the sig chain logged into session "5e9d8aa625a1838f60f30e12058089169e32c968389f365428f7b0c878bb47f8") being sent to a token account identified by the name "bob:savings".  In the name_to field, "bob" refers to the username of the account holder and "savings" is their token account.  If Bob is concerned about privacy he could have provided the register address of his token account instead

```
{
    "pin": "1234",
    "session": "5e9d8aa625a1838f60f30e12058089169e32c968389f365428f7b0c878bb47f8",
    "name": "main",
    "amount": 10.5,
    "name_to": "bob:savings"
}
```

### Example Multiple Recipients:

The following example shows the json payload for a debit from an account called "main" made to 3 separate recipients, each receiving 5 tokens, and each with a different reference  

```
{
    "pin": "1234",
    "name": "main"
    "recipients" :
    [
        {
            "name_to": "bob:abc",
            "amount": 5,
            "reference": 1234
        },
        {
            "address_to": "8CHjS5Qe7Mghgrqb6NeEaVxjexbdp9p2QVdRTt8W4rzbWhu3fL8",
            "amount": 5,
            "reference": 5678
        },
        {
            "address_to": "8CM4SYrd6ohcBB8tfqyZG3MCtDStMUXN37gwC2RtkNSUayM2FSN",
            "amount": 5
        }
    ]
}
```

### Return value JSON object:
```    
{
    "txid": "f9dcd28bce2563ab288fab76cf3ee5149ea938c735894ce4833b55e474e08e8a519e8005e09e2fc19623577a8839a280ca72b6430ee0bdf13b3d9f785bc7397d"
}
```   

### Return values:

`txid` : The ID (hash) of the transaction that includes the token debit.

****

# `credit/account`

Increment an amount received from another token account to an account owned by your signature chain. The API supports an alternative endpoint that can include the account name to credit in the URI.  For example `/tokens/credit/account/savings` will resolve to `tokens/credit/account?name=savings`.   


### Endpoint:

`/tokens/credit/account`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the token account. For single-user API mode the session should not be supplied.

`name` : The name identifying the token account to credit. This is only required for split payments (where the receiving account is not included in the credit transaction) and is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the account to credit. This is only required for split payments (where the receiving account is not included in the credit transaction) and is optional if the name is provided.

`txid` : The transaction ID (hash) of the corresponding debit transaction for which you are creating this credit for.

`name_proof` : The name identifying the account that proves your ability to credit the debit transaction.  This is only required for split payments, where your right to receive a payment is determined by the number of tokens you hold in the account proof account. The `name_proof` parameter can be used as alternative to `proof`  

`address_proof` : The register address the account that proves your ability to credit the debit transaction.  This is only required for split payments, where your right to receive a payment is determined by the number of tokens you hold in the account proof account. The `address_proof` parameter can be used as alternative to `name_proof`

### Example:

The following example shows the json payload for a credit to an account called "savings" (that itself was created by the sig chain logged into session "5e9d8aa625a1838f60f30e12058089169e32c968389f365428f7b0c878bb47f8") identified by the name "bob:savings".  In the name field, "bob" refers to the username of the account holder and "savings" is their token account.  

```
{
    "pin": "1234",
    "session": "5e9d8aa625a1838f60f30e12058089169e32c968389f365428f7b0c878bb47f8",
    "name": "bob:savings",
    "txid": "f9dcd28bce2563ab288fab76cf3ee5149ea938c735894ce4833b55e474e08e8a519e8005e09e2fc19623577a8839a280ca72b6430ee0bdf13b3d9f785bc7397d"
}
```

### Return value JSON object:
```    
{
    "txid": "318b86d2c208618aaa13946a3b75f14472ebc0cce9e659f2830b17e854984b55606738f689d886800f21ffee68a3e5fd5a29818e88f8c5b13b9f8ae67739903d"
}
```   

### Return values:

`txid` : The ID (hash) of the transaction that includes the token credit.

****



# `get/account`

Retrieves information about a token account . The API supports an alternative endpoint that can include the account name in the URI.  For example `/tokens/get/account/savings` will resolve to `tokens/get/account?name=savings`.

Additionally the API supports passing a field name in the URL after the account name, which will populate the `fieldname` parameter in the request and filter the response on that field.  For example `/tokens/get/account/savings/balance` will resolve to `tokens/get/account?name=savings&fieldname=balance`


### Endpoint:

`/tokens/get/account`


### Parameters:


`name` : The name identifying the token account to retrieve. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the name in order to deduce the register address of the account.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

`address` : The register address of the token account to retrieve. This is optional if the name is provided.

`count` : Optional boolean field that determines whether the response includes the transaction `count` field.  This defaults to false, as including the transaction count can slow the response time of the method considerably.   

`fieldname`: This optional field can be used to filter the response to return only a single field from the token account.

### Return value JSON object:
```    
{
    "owner": "a2e51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0",
    "created": 1566534164,
    "modified": 1566616211,
    "name": "myaccount",
    "address": "8CbkwEQ9S8owmX74joU6XmiwxJq1aoiqUoXc9fLCKzw15HscM99",
    "token_name": "mytoken"
    "token": "8EHRNnxn5qX9gMF1Jbqvo1jkAeP7XY1PETnMtegv9rdr6QG3ZJy",
    "balance": 990,
    "pending": 0.0,
    "unconfirmed": 10
}
```   

### Return values:

`owner` : The genesis hash of the signature chain that owns this account.

`created` : The UNIX timestamp when the account was created.

`modified` : The UNIX timestamp when the account was last modified.

`name` : The name identifying the token account.  For privacy purposes, this is only included in the response if the caller is the owner of the token account

`address` : The register address of the account.  

`token_name` : The name of the token that this account can be used with.  

`token` : This is register address of the token that this account can be used with.  

`balance` : The available balance of this account.  This is the last confirmed balance less any new debits that you have made since the last block

`pending` : This is the sum of all confirmed debit transactions that have been made to this account, that have not yet been credited.  To move coins from pending into the available balance you must create a corresponding credit transaction.  NOTE: if configured to run, the events processor does this for you.

`unconfirmed` : This is the sum of all unconfirmed debit transactions that have been made to this account PLUS the sum of all unconfirmed credits that you have for confirmed debit transactions.  When someone makes a debit to your account it will immediately appear in the unconfirmed balance until that transaction is included in a block, at which point it moves into `pending`.  When you (or the events processor) creates the corresponding credit transaction for that debit, the amount will move from `pending` back into `unconfirmed` until the credit transaction is included in a block, at which point the amount moves to `balance`.  

`count` : Only returned if the caller requested `count` : true.  This is the number of transactions made to/from the account.

****


# `list/account/transactions`

This will list off all of the transactions related to a given account. You DO NOT need to be logged in to use this command. If you are logged in, then neither username or genesis are required as it will default to the logged in user. 

**NOTE** : The returned transaction data will only include contracts that related to the requested account.  Any other contracts are omitted from the transaction result. 

**NOTE** : If you use the username parameter it will take slightly longer to calculate the username genesis with our brute-force protected hashing algorithm. For higher performance, use the genesis parameter.


### Endpoint:

`/tokens/list/account/transactions`


### Parameters:


`genesis` : The genesis hash identifying the signature chain to scan for transactions (optional if username is supplied or already logged in).

`username` : The username identifying the signature chain to scan for transactions(optional if genesis is supplied or already logged in).

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the account name in order to deduce the register address of the account.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

`name` : The name identifying the token account to list transactions for. This is optional if the address is provided. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the object was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the token account to list transactions for. This is optional if the name is provided.

`page` : Allows the results to be returned by page (zero based). E.g. passing in page=1 will return the second set of (limit) transactions. The default value is 0 if not supplied.

`limit` : The number of transactions to return for the current page. The default is 100.

`verbose` : Optional, determines how much transaction data to include in the response. Supported values are :
 - `default` : hash, contracts
 - `summary` : type, version, sequence, timestamp, operation, and confirmations.
 - `detail` : genesis, nexthash, prevhash, pubkey and signature.



### Return value JSON object:
```
[
    {
        "txid": "01034b39cb3635d370f97339e6f87b8751d4c0d62676da7d6ec20416966f298f47dea99603d03a74e638b0d50b31b1e721790e5b103abfe3353a709ccf5d1e7c",
        "contracts": [
            {
                "OP": "CREDIT",
                "txid": "01e73b498dbabbf4629ad674b9ae3824b96cca83199c25a67901db53b271d19acf1411b0c4f9a3d8ded80860ffe2dcf683d2d227a675d453303b31f86f868f9e",
                "contract": 0,
                "proof": "a2e51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0",
                "to": "8CbkwEQ9S8owmX74joU6XmiwxJq1aoiqUoXc9fLCKzw15HscM99",
                "to_name": "test",
                "amount": 10,
                "token": "8EHRNnxn5qX9gMF1Jbqvo1jkAeP7XY1PETnMtegv9rdr6QG3ZJy",
                "token_name": "TST"
            }
        ]
    }
]

```

### Return values:

`txid` : The transaction hash.

`type` : The description of the transaction (`legacy` | `tritium base` | `trust` | `genesis` | `user`).

`version` : The serialization version of the transaction.

`sequence` : The sequence number of this transaction within the signature chain.

`timestamp` : The Unix timestamp of when the transaction was created.

`blockhash` : The hash of the block that this transaction is included in.  Blank if not yet included in a block.

`confirmations` : The number of confirmations that this transaction obtained by the network.

`genesis` : The signature chain genesis hash.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : the hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature` : The signature hash.

`contracts` : The array of contracts bound to this transaction and their details with opcodes.    
{    
&nbsp;&nbsp;&nbsp;`id` : The sequential ID of this contract within the transaction.

&nbsp;&nbsp;&nbsp;`OP` : The contract operation. Can be `APPEND`, `CLAIM`, `COINBASE`, `CREATE`, `CREDIT`, `DEBIT`, `FEE`, `GENESIS`, `LEGACY`, `TRANSFER`, `TRUST`, `STAKE`, `UNSTAKE`, `WRITE`.

&nbsp;&nbsp;&nbsp;`for` : For `CREDIT` transactions, the contract that this credit was created for . Always `DEBIT` for token transactions.

&nbsp;&nbsp;&nbsp;`txid` : The transaction that was credited / claimed.

&nbsp;&nbsp;&nbsp;`contract` : The ID of the contract within the transaction that was credited / claimed.

&nbsp;&nbsp;&nbsp;`proof` : The register address proving the credit.

&nbsp;&nbsp;&nbsp;`from` : For `DEBIT`, `CREDIT`, `FEE` transactions, the register address of the account that the debit is being made from.

&nbsp;&nbsp;&nbsp;`from_name` : For `DEBIT`, `CREDIT`, `FEE` transactions, the name of the account that the debit is being made from.  Only included if the name can be resolved.

&nbsp;&nbsp;&nbsp;`to` : For `DEBIT` and `CREDIT` transactions, the register address of the recipient account.

&nbsp;&nbsp;&nbsp;`to_name` : For `DEBIT` and `CREDIT` transactions, the name of the recipient account.  Only included if the name can be resolved.

&nbsp;&nbsp;&nbsp;`amount` : the token amount of the transaction.

&nbsp;&nbsp;&nbsp;`token` : the register address of the token that the transaction relates to.  Set to 0 for NXS transactions

&nbsp;&nbsp;&nbsp;`token_name` : The name of the token that the transaction relates to.

&nbsp;&nbsp;&nbsp;`reference` : For `DEBIT` and `CREDIT` transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number.

}

***
