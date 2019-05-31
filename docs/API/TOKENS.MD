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
[`create/account`](#createaccount)   
[`debit/account`](#debitaccount)   
[`credit/account`](#creditaccount)   
[`get/account`](#getaccount)   

-----------------------------------
***

# `create/token`

Create a new token object register. The API supports an alternative endpoint that can include the new token name in the URI.  For example `/tokens/create/token/mytoken` will resolve to `tokens/create/token?name=mytoken`.   


### Endpoint:

`/tokens/create/token`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the token should be created with. For single-user API mode the session should not be supplied.

`name` : A name to identify the token. A hash of the name will determine the register address. If no name is provided, a random 256-bit register hash will be generated.

`supply` : The initial token supply amount.

`digits` : The maximum number of decimal places that can be applied to token amounts.  For example digits=2 will allow a token amount to be given to 2 decimal places.

### Example:

```
{
    "pin": "1234",
    "session": "16953804956473956578",
    "name": "ABC",
    "supply": 1000000,
    "digits": 4
}
```

### Return value JSON object:
```    
{
    "txid": "f9dcd28bce2563ab288fab76cf3ee5149ea938c735894ce4833b55e474e08e8a519e8005e09e2fc19623577a8839a280ca72b6430ee0bdf13b3d9f785bc7397d",
    "address": "f2f71204bda6e6512d21c48eb4ca98db37a25c4d152afb284f3fc36760c0c717"
}
```   

### Return values:

`txid` : The ID (hash) of the transaction that includes the token creation.

`address` : The register address for this token. The address (or name that hashes to this address) is needed when creating token accounts, so that we know which token the account can be used with.

***

# `debit/token`

Deduct a token amount from a token's initial supply and send it to a token account. The API supports an alternative endpoint that can include the token name in the URI.  For example `/tokens/debit/token/mytoken` will resolve to `tokens/debit/token?name=mytoken`.   


### Endpoint:

`/tokens/debit/token`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the token owner. For single-user API mode the session should not be supplied.

`name` : The name identifying the token to debit. This is optional if the address is provided.  The name should be in username:name format.  However, if the token was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the token to debit. This is optional if the name is provided.

`amount` : The amount of tokens to debit.

`name_to` : The name identifying the token account to send to. This is optional if the address is provided.  The name should be in username:name format.  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address_to` : The register address of the token account to send to. This is optional if the name is provided.

### Example:

The following example shows the json payload for a debit of 10.5 ABC tokens (that itself was created by the sig chain logged into session "16953804956473956578") being sent to a token account identified by the name "jack:savings".  In the name_to field, "jack" refers to the username of the account holder and "savings" is their token account.  If jack is concerned about privacy he could have provided the register address of his token account instead

```
{
    "pin": "1234",
    "session": "16953804956473956578",
    "name": "bob:ABC",
    "amount": 10.5,
    "name_to": "jack:savings"
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

`name` : The name identifying the token to credit. This is optional if the address is provided.  `name` : The name identifying the token to debit. This is optional if the address is provided.  The name should be in username:name format.  However, if the token was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the token to credit. This is optional if the name is provided.

`txid` : The transaction ID (hash) of the corresponding debit transaction for which you are creating this credit for.



### Example:

The following example shows the json payload for a credit to a token called "mytoken" (that itself was created by the sig chain logged into session "16953804956473956578").  

```
{
    "pin": "1234",
    "session": "16953804956473956578",
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


`name` : The name identifying the token to retrieve. This is optional if the address is provided.  `name` : The name identifying the token to debit. This is optional if the address is provided.  The name should be in username:name format.  However, if the token was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the name in order to deduce the register address of the token.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

`address` : The register address of the token to retrieve. This is optional if the name is provided.

`fieldname`: This optional field can be used to filter the response to return only a single field from the token.

### Return value JSON object:
```    
{
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9",
    "balance": 990,
    "maxsupply": 1000,
    "currentsupply": 10,
    "digits": 0
}
```   

### Return values:

`address` : The register address of the token.  If the token was created with a token name then the address is a hash based on the owners username and token name.   

`balance` : The current balance of this token.  Initially the balance of the token is equal to the max supply, until the tokens have been distributed to accounts.

`maxsupply` : The maximum token supply amount.

`currentsupply` : The current amount of tokens that have been distributed to token accounts.

`digits` : The maximum number of decimal places that can be applied to token amounts.  For example digits=2 will allow a token amount to be given to 2 decimal places.

***

# `create/account`

Create a new token account for receiving tokens. The API supports an alternative endpoint that can include the account name in the URI.  For example `/tokens/create/account/savings` will resolve to `tokens/create/account?name=savings`.   


### Endpoint:

`/tokens/create/account`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the token account should be created with. For single-user API mode the session should not be supplied.

`name` : A name to identify the token account. A hash of the name will determine the register address. If no name is provided, a random 256-bit register hash will be generated.

`token_name` : This is the name of the token that the account can be used for.  The token name should be supplied in the format `username:tokenname`.  This is optional if the token_address is supplied.

`token_addresss` : This is the address of the token that the account can be used for.  This is optional if token_name is supploed.



### Example:

This example creates a token account called "main" for a token identified by the token name.  NOTE the name has been qualified with a namespace which is the username of the signature chain that created the token 

```
{
    "pin": "1234",
    "session": "16953804956473956578",
    "name": "main",
    "token_name": "bob:ABC"
}
```

### Example:

This example creates a token account called "savings" for a token identified by its register address. 

```
{
    "pin": "1234",
    "session": "16953804956473956578",
    "name": "savings",
    "token_address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9"
}
```

### Return value JSON object:
```    
{
    "txid": "f9dcd28bce2563ab288fab76cf3ee5149ea938c735894ce4833b55e474e08e8a519e8005e09e2fc19623577a8839a280ca72b6430ee0bdf13b3d9f785bc7397d",
    "address": "f2f71204bda6e6512d21c48eb4ca98db37a25c4d152afb284f3fc36760c0c717"
}
```   

### Return values:

`txid` : The ID (hash) of the transaction that includes the account creation.

`address` : The register address for this token account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.    


***

# `debit/account`

Deduct a token amount from a token account and send it to another token account. The API supports an alternative endpoint that can include the account name in the URI.  For example `/tokens/debit/account/savings` will resolve to `tokens/debit/account?name=savings`.   


### Endpoint:

`/tokens/debit/account`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) that owns the token account. For single-user API mode the session should not be supplied.

`name` : The name identifying the token account to debit. This is optional if the address is provided.  The name should be in username:name format.  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the token account to debit. This is optional if the name is provided.

`amount` : The amount of tokens to debit.

`name_to` : The name identifying the token account to send to. This is optional if the address is provided.  The name should be in username:name format.  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address_to` : The register address of the token account to send to. This is optional if the name is provided.

### Example:

The following example shows the json payload for a debit of 10.5 tokens from an account called "main" (that itself was created by the sig chain logged into session "16953804956473956578") being sent to a token account identified by the name "bob:savings".  In the name_to field, "bob" refers to the username of the account holder and "savings" is their token account.  If Bob is concerned about privacy he could have provided the register address of his token account instead

```
{
    "pin": "1234",
    "session": "16953804956473956578",
    "name": "main",
    "amount": 10.5,
    "name_to": "bob:savings"
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

`name` : The name identifying the token account to credit. This is optional if the address is provided.  The name should be in username:name format.  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the account to credit. This is optional if the name is provided.

`txid` : The transaction ID (hash) of the corresponding debit transaction for which you are creating this credit for.

`name_proof` : The name identifying the account that proves your ability to credit the debit transaction.  This is only required for split payments, where your right to receive a payment is determined by the number of tokens you hold in the account proof account. The `name_proof` parameter can be used as alternative to `proof`  

`address_proof` : The register address the account that proves your ability to credit the debit transaction.  This is only required for split payments, where your right to receive a payment is determined by the number of tokens you hold in the account proof account. The `address_proof` parameter can be used as alternative to `name_proof`

### Example:

The following example shows the json payload for a credit to an account called "savings" (that itself was created by the sig chain logged into session "16953804956473956578") identified by the name "bob:savings".  In the name field, "bob" refers to the username of the account holder and "savings" is their token account.  

```
{
    "pin": "1234",
    "session": "16953804956473956578",
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


`name` : The name identifying the token account to retrieve. This is optional if the address is provided.  The name should be in username:name format.  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the name in order to deduce the register address of the account.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

`address` : The register address of the token account to retrieve. This is optional if the name is provided.

`fieldname`: This optional field can be used to filter the response to return only a single field from the token account.

### Return value JSON object:
```    
{
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9",
    "token_address": "f9ee3e63c845794d577d54c5087f0ff2f2053195bf007907e8134977bb1f1e24",
    "balance": 990
}
```   

### Return values:

`address` : The register address of the account.  The address is a hash based on the owners username and account name.

`token_address` : This is register address of the token that this account can be used with.  

`balance` : The current balance of this token account.  


***