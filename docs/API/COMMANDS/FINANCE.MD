# Finance API
-----------------------------------

The Finance API provides methods for sending and receiving NXS or other tokens between users / accounts, creating accounts, and managing staking. The full supported endpoint of the finance URI is as follows:

```
finance/verb/noun/filter/operator
```

The minimum required components of the URI are:

```
finance/verb/noun
```
## `Supported Verbs`

The following verbs are currently supported by this API command-set:

[`burn`](#burn)     - Remove a given token from circulation.   
[`create`](#create) - Generate a new object of supported type.   
[`credit`](#credit) - Claim funds issued to account from debit.     
[`debit`](#debit)   - Issue funds from supported type.  
[`get`](#get)       - Get object of supported type.     
[`list`](#list)     - List all objects owned by given user.     
[`history`](#history) - Generate the history of all last states.          
[`transactions`](#transactions) - List all transactions that modified specified object.     

-----------------------------------

## `Supported Nouns`

The following nouns are supported for this API command-set:

[`account`] - An object register containing a token-id and balance.     
[`trust`]   - An object register containing a token-id, balance, and trust.     
[`token`]   - An object register containing a token-id, balance, supply, and decimals.      
[`any`]     - An object selection noun allowing mixed accounts of different tokens.     
[`all`]     - An object selection noun to collect all accounts for given token type.    

-----------------------------------

## `Direct Endpoints`

The following commands are direct endpoints and thus do not support the above `verb` and `noun` structure available above.

[`get/balances`](#get/balance)      
[`get/stakeinfo`](#get/stakeinfo)   
[`set/stake`](#set/stake)  
[`void/transaction`](#void/transaction)   
[`migrate/accounts`](#migrate/account)   

Direct endpoints support filters and operators.

-----------------------------------

# <a name="create"></a> `create`

Create a new object register specified by given noun.

```
finance/create/noun
```

This command does not support the `any` or `all` nouns.

### Parameters:

`pin` : Required if **locked**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Optional for **noun** `name` as a *UTF-8* encoded string that will generate a name object register that points to new object. If noun is `token` this will be created as a global name.

`data` : Optional for **any** noun, allows caller to add arbitrary data to object.


#### create/token

`supply` : Required by **noun** `token` that sets the maximum token supply.

`decimals` : Required by **noun** `token` that sets the total number of significant figures. Defaults to 2.

#### create/account

`token` : Required by **noun** `account` as a *Base58* encoded address or ticker name. Defaults to `NXS`.


### Results:

#### Return value JSON object:


```
{
    "success": true,
    "address": "8ESvYizqdApiuKEBjZMF1hnB8asDqECaDwAstcH3UtJ4Z6ceCn2",
    "txid": "0131e17af8029b414814283a3d90813d12c238db6ddab213440249b795090a9cd77079d5804ec38303a59414d87108d4e44bf31f54a6c176285281a88ab5d737"
}
```
#### Return values:

`txid` : The hash of the transaction that was generated for this tx. If using `-autotx` this field will be ommitted.

`address` : The register address for this  account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.

-----------------------------------

# <a name="debit"></a> `debit`

Deduct an amount of NXS or token specified by the noun and send it to another account or legacy UTXO address. Only NXS can be sent to the legacy address.

```
finance/debit/noun
```

This command supports the any wildcard noun.

#### debit/account

This deducts an amount of NXS or tokens from a token account or token address to be sent to a receiving account.

#### debit/token

This deducts an amount of tokens from a token address to send to a token account

### Parameters:

`pin` : Required if **locked**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`from` : Required to **identify** the account to debit, this can be the name or the register address. The name should be in the format name (for global token names), username:name (for local names) or namespace::name (for names in a namespace).

`amount` : The amount of NXS or token to debit.

`to` : Required to **identify** the receiving account. This can be the account name or the register address. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). The address_to can also contain a legacy UTXO address if sending from a signature chain account to a legacy address.

`reference` : Optional field allows callers to provide a **reference**, which the recipient can then use to relate the transaction to an order number, invoice number etc. The reference is be a 64-bit unsigned integer in the range of 0 to 18446744073709551615

`expires` : Optional field allows callers to specify an **expiration** for the debit transaction. The expires value is the number of seconds from the transaction creation time after which the transaction can no longer be credited by the recipient. Conversely, when you apply an expiration to a transaction, you are unable to void the transaction until after the expiration time. If expires is set to 0, the transaction will never expire, making the sender unable to ever void the transaction. If omitted, a default expiration of 7 days (604800 seconds) is applied.

### Results:

#### Return value JSON object:

```
{
    "success": true,
    "txid": "01f51d6b23b871fc8da848afa57cf066cb9e3b8fb845a666335e8c678ef5249e98d4f3e477659098918e4bb590472a63d0ed0a17fa87904fcff6316158e9edfd"
}
[Completed in 4979.735275 ms]
```

#### Return values:

`success` : Boolean flag indicating that the debit was successful.

`txid` : The ID (hash) of the transaction that includes the debit.

-----------------------------------

# <a name="credit"></a> `credit`

Create a new object register specified by given noun.

```
finance/credit/noun
```

This command supports the any wildcard noun.

#### credit/account

Increment an amount received from a NXS account, token account or token address.

#### credit/token

Increment an amount of tokens received from a token account.

### Parameters:

`pin` : Required if **locked**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`txid` : The hash in **hexadecimal** encoding of the transaction that we are crediting.

### Results:

#### Return value JSON object:

```
{
    "success": true,
    "txid": "01f51d6b23b871fc8da848afa57cf066cb9e3b8fb845a666335e8c678ef5249e98d4f3e477659098918e4bb590472a63d0ed0a17fa87904fcff6316158e9edfd"
}
[Completed in 4979.735275 ms]
```
#### Return values:

`txid` : The ID (hash) of the transaction that includes the account creation.

`address` : The register address for this  account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.

-----------------------------------

# <a name="burn"></a> `burn`

This method can be used to take tokens permanently out of the current supply, a process commonly known as burning. The method will debit a token account and send the tokens back to the token address. However the transaction contains a condition that will always evaluate to false, guaranteeing that the debit can not be credited by the token issuer nor the sender. The result is that the amount burned will always appear in the "pending" balance of the token.

```
finance/burn/noun
```

This command only supports the `account` noun.

### Parameters:

`pin` : Required if  **locked**. The `PIN` to authorize the transaction.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`name` : Required to **identify** the name of account to debit the tokens from to be burnt. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the  account to debit the tokens from the be burnt. This is optional if the `name` is provided.

`amount` : Required the **amount** of tokens to burn.

`reference` : Optional field allows callers to provide a **reference**, which the recipient can then use to relate the transaction to an order number, invoice number etc. The reference is be a 64-bit unsigned integer in the range of 0 to 18446744073709551615

### Results:

#### Return value JSON object:

```
{
    "success": true,
    "address": "8BfLgEprhbHs82LxUkJR9jhQufRZf49g73Nt8XTGevfiyy7ijhb",
    "txid": "01854fe4fdf0d59aebb3a880141484f0542af061cbebfd468db3fcecd13f63a986d990cf669ca4a60822a82b2d4fc7e7e474801a01bff86a35fd0a147a5a62da"
}
[Completed in 4973.117685 ms]
```

#### Return values:
`success` : Boolean flag indicating that the debit was successful.

`address` : The register address for this account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.

`txid` : The hash of the transaction that was generated for this tx. If using -autotx this field will be omitted.

-----------------------------------

# <a name="get"></a> `get`

Retrieves information for a single object for a type specified by the noun


```
finance/get/noun
```

This command supports the `account`, `trust` and `token` nouns.

#### get/account

Retrieves information for a specified NXS or token account.

#### get/trust

Retrieves information for a specified trust account.

#### get/token

Retrieves information for a specified token address.

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.  

`name` : Required to **identify** the name of the account / trust / token. This is optional if the `address` is provided.

`address` : Required to **identify** the register address of the account / trust / token. This is optional if the `name` is provided.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD). 

### Results:

#### Return value JSON object:

```
{
    "owner": "b1b5b4f4197548886016586f95735f0cb8235183a9185b8720bd27502a2e2850",
    "version": 1,
    "created": 1638020495,
    "modified": 1655118914,
    "type": "OBJECT",
    "balance": 300.536104,
    "stake": 15000.0,
    "token": "0",
    "ticker": "NXS",
    "trust": 3399813,
    "age": "39 days, 8 hours, 23 minutes",
    "rate": 3.0,
    "address": "8EunQ82qVdnuQkX2gXKZr5P55kQRz4KbpaLdCVBjBNu8jeys4C4"
}
[Completed in 0.301107 ms]
```

#### Return values:

`owner` : The genesis hash of the profile that owns this account.

`version` : The serialization version of the transaction.

`created` : The UNIX timestamp when the account was created.

`modified` : The UNIX timestamp when the account was last modified.

`type` : The type of register. Can be OBJECT, RAW or READONLY.

`balance` : The available balance of this account. This is the last confirmed balance less any new debits that you have made since the last block

`stake` : Only returned for the trust account, this is the amount of NXS currently staked in the trust account.

`decimals` : The maximum number of decimal places that can be applied to token amounts. For example decimals=2 will allow a token amount to be given to 2 decimal places.

`currentsupply `: The current amount of tokens that have been distributed to token accounts.

`maxsupply` : The maximum token supply amount.

`token` : The register address of the token. Set to 0 for NXS accounts. 

`ticker` : The name of the token. For NXS accounts will show NXS.

`trust` : The current raw trust score of the trust account.

`age` : The age of the trust account in days, hours and minutes.

`rate` : The current annual reward rate earned for staking as an annual percent.

`address` : The register address of the account, trust or token.

-----------------------------------

# <a name="list"></a> `list`

This will list off all of the object register details specified by the noun.

```
finance/list/noun
```

This command supports the `account`, `trust` and `token` nouns.

#### list/account

This lists all the NXS and token accounts, except the trust account for the logged in user.

#### list/trust

This lists the trust account for the logged in user.

#### list/token

This lists all the token addresses for the logged in user.

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
        "owner": "b7fa11647c02a3a65a72970d8e703d8804eb127c7e7c41d565c3514a4d3fdf13",
        "version": 1,
        "created": 1654808816,
        "modified": 1655061950,
        "type": "OBJECT",
        "balance": 5998.0,
        "decimals": 2,
        "currentsupply": 4002.0,
        "maxsupply": 10000.0,
        "token": "8DXmAmkTtysSZUxM3ePA8wRmbSUofuHKSoCyDpN28aLuSrm1nDG",
        "ticker": "XYZ",
        "address": "8DXmAmkTtysSZUxM3ePA8wRmbSUofuHKSoCyDpN28aLuSrm1nDG"
    }
]
[Completed in 0.473708 ms]
```

#### Return values:

`owner` : The genesis hash of the profile that owns this account.

`version` : The serialization version of the transaction.

`created` : The UNIX timestamp when the account was created.

`modified` : The UNIX timestamp when the account was last modified.

`type` : The type of register. Can be OBJECT, RAW or READONLY.

`balance` : The available balance of this account. This is the last confirmed balance less any new debits that you have made since the last block

`stake` : Only returned for the trust account, this is the amount of NXS currently staked in the trust account.

`decimals` : The maximum number of decimal places that can be applied to token amounts. For example decimals=2 will allow a token amount to be given to 2 decimal places.

`currentsupply `: The current amount of tokens that have been distributed to token accounts.

`maxsupply` : The maximum token supply amount.

`token` : The register address of the token. Set to 0 for NXS accounts. 

`ticker` : The name of the token. For NXS accounts will show NXS.

`trust` : The current raw trust score of the trust account.

`age` : The age of the trust account in days, hours and minutes.

`rate` : The current annual reward rate earned for staking as an annual percent.

`address` : The register address of the account, trust or token.


-----------------------------------

# <a name="history"></a> `history` 

This will get the history of the specified noun.

```
finance/history/noun
```

This command supports the `account`, `trust` and `token `nouns.

#### history/account

This lists all the NXS except 'trust and token accounts for the logged in user.

#### history/trust

This lists the trust account for the logged in user.

#### history/token

This lists all the token addresses for the logged in user.

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.  

`name` : Required to **identify** the name of the account / trust / token. This is optional if the `address` is provided.

`address` : Required to **identify** the register address of the account / trust / token. This is optional if the `name` is provided.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 

### Results:

#### Return value JSON object:


```
[
    {
        "owner": "b7fa11647c02a3a65a72970d8e703d8804eb127c7e7c41d565c3514a4d3fdf13",
        "version": 1,
        "created": 1654808903,
        "modified": 1654809207,
        "type": "OBJECT",
        "balance": 1000.0,
        "token": "8DXmAmkTtysSZUxM3ePA8wRmbSUofuHKSoCyDpN28aLuSrm1nDG",
        "ticker": "XYZ",
        "address": "8Bk5PxsecfXWpbHsDXeZ47MCgDF7qDLsU4Y4MJw2VB29LsTR98z",
        "name": "local:XYZToken",
        "action": "CREDIT"
    },
    {
        "owner": "b7fa11647c02a3a65a72970d8e703d8804eb127c7e7c41d565c3514a4d3fdf13",
        "version": 1,
        "created": 1654808903,
        "modified": 1654808903,
        "type": "OBJECT",
        "balance": 0.0,
        "token": "8DXmAmkTtysSZUxM3ePA8wRmbSUofuHKSoCyDpN28aLuSrm1nDG",
        "ticker": "XYZ",
        "address": "8Bk5PxsecfXWpbHsDXeZ47MCgDF7qDLsU4Y4MJw2VB29LsTR98z",
        "name": "local:XYZToken",
        "action": "CREATE"
    }
]
[Completed in 12.427076 ms]
```

#### Return value :

`owner` : The username hash of the profile that owns this object.

`version` : The serialization version of the transaction.

`created` : The UNIX timestamp when the account was created.

`modified` : The UNIX timestamp when the account was last modified.

`type` : The type of register. Can be OBJECT, RAW or READONLY.

`balance` : The available balance of this account. This is the last confirmed balance less any new debits that you have made since the last block

`stake` : Only returned for the trust account, this is the amount of NXS currently staked in the trust account.

`decimals` : The maximum number of decimal places that can be applied to token amounts. For example decimals=2 will allow a token amount to be given to 2 decimal places.

`currentsupply `: The current amount of tokens that have been distributed to token accounts.

`maxsupply` : The maximum token supply amount.

`token` : The register address of the token. Set to 0 for NXS accounts. 

`ticker` : The name of the token. For NXS accounts will show NXS.

`trust` : The current raw trust score of the trust account.

`age` : The age of the trust account in days, hours and minutes.

`rate` : The current annual reward rate earned for staking as an annual percent.

`address` : The register address of the account, trust or token.

`name` : The name identifying the account, trust or token.

`action` : The action that occurred - CREATE | MODIFY | TRANSFER | CLAIM.

-----------------------------------

# <a name="transactions"></a> `transactions`

This will list off all of the transactions for the specified noun.

```
finance/transactions/noun
```

This command supports the account, trust and token nouns.

#### transactions/account

List out all the transactions for the NXS account or token account.

#### transactions/trust

List out all the transactions for the trust account

#### transactions/token

List out all the transactions for the token address.

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied. 

`verbose` : Optional, determines how much **transaction data** to include in the response. Supported values are :
- default : hash.

- summary : type, version, sequence, timestamp, operation, and confirmations.

- detail : genesis, nexthash, prevhash, pubkey and signature.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 

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

`for` : For CREDIT transactions, the contract that this credit was created for . Can be COINBASE, DEBIT or LEGACY.

`txid` : The transaction that was credited / claimed.

`contract` : The ID of the contract within the transaction that was credited / claimed.

`proof` : The register address proving the credit.

`from` : For DEBIT, CREDIT, FEE transactions, the register address of the account that the debit is being made from.

`from_name` : For DEBIT, CREDIT, FEE transactions, the name of the account that the debit is being made from. Only included if the name can be resolved.

`to` : For DEBIT and CREDIT transactions, the register address of the recipient account.

`to_name` : For DEBIT and CREDIT transactions, the name of the recipient account. Only included if the name can be resolved.

`amount` : the token amount of the transaction.

`token` : the register address of the token that the transaction relates to. Set to 0 for NXS transactions.

`ticker` : The name of the token that the transaction relates to.

`reference` : For DEBIT and CREDIT transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number.

`object` : Returns a list of all hashed public keys in the crypto object register for the specified profile. The object result will contain the nine default keys(app1, app2, app3, auth, cert lisp, network, sign and verify).  
}

-----------------------------------

# <a name="get/balances"></a> `get/balances`

This will retrieve a summary of balance information across all accounts belonging to the currently logged in profile.

Endpoint:

```
finance/get/balances
```

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

[`Filtering`](/docs/API/FILTERING.MD).



### Results:

#### Return value JSON object:

```
[
    {
        "available": 1573155.354522,
        "unclaimed": 0.0,
        "unconfirmed": 0.0,
        "decimals": 6,
        "token": "0",
        "ticker": "NXS",
        "stake": 15000.0,
        "immature": 0.0
    },
    {
        "available": 10000.0,
        "unclaimed": 0.0,
        "unconfirmed": 0.0,
        "decimals": 2,
        "token": "8E61JVQENSGtFsr3ivscvC77Tvr3oLm8Zqnw3K4H4vbbTUBeGqW",
        "ticker": "NEX"
    }
]
[Completed in 6521.434131 ms]
```

#### Return values:

`available` : The current balance across all accounts that is available to be spent.

`unclaimed`: The sum of all debit transactions made to other accounts that are not confirmed.

`unconfirmed` : The sum of all debit transactions made to your accounts that are not confirmed, or credits you have made to your accounts that are not yet confirmed (not yet included in a block).

`decimals` : The no of decimals for the token.

`token` : The register address of the token that these balances are for.

`ticker` : The name of the token that these balances are for, if known.

`pending` : The sum of all debit and coinbase transactions made to your accounts that are confirmed but have not yet been credited. This does NOT include immature and unconfirmed amounts.

`stake` : The amount of NXS currently staked in the trust account. Only included when returning NXS balances.

`immature`: The sum of all coinbase transactions that have not yet reached maturity. Only included when returning NXS balances.

-----------------------------------

# <a name="get/stakeinfo"></a> `get/stakeinfo`

This will retrieve account values and staking metrics for the trust account belonging to the currently logged in signature chain. If called when the stake minter is not running, this method only returns trust account values. Staking metrics will return 0.

Endpoint:

```
finance/get/stakeinfo
```

### Parameters:

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

[`Filtering`](/docs/API/FILTERING.MD).

### Results:

#### Return value JSON object:

```
{
    "address": "8EunQ82qVdnuQkX2gXKZr5P55kQRz4KbpaLdCVBjBNu8jeys4C4",
    "balance": 337.362522,
    "stake": 15000.0,
    "trust": 338495,
    "onhold": false,
    "stakerate": 1.8052413746600344,
    "trustweight": 53.440432749937834,
    "blockweight": 15.358176013767272,
    "stakeweight": 49.63220707632078,
    "staking": true,
    "change": false
}
[Completed in 0.237830 ms]
```

#### Return values:

`address` : The register address of the trust account.

`balance` : The current NXS balance of the trust account. This is general account balance that is not staked.

`stake` : The amount of NXS currently staked in the trust account.

`trust` : The current raw trust score of the trust account.

`onhold` : When trust account is new, any change to balance requires a minimum wait period before staking begins. During this period, staking is on hold and this field returns true. "staking" field will still be true when on hold.

`stakerate` : The current annual reward rate earned for staking as an annual percent.

`trustweight` : The current trust weight applied to staking as a percent of maximum.

`blockweight` : The current block weight applied to staking as a percent of maximum.

`stakeweight` : The current stake weight (trust weight and block weight combined) as a percent of maximum.

`staking` : Indicates whether staking is actively running for user account (when false, weight metrics will be 0).

`change` : Indicates whether or not there is a pending request to change stake. The remaining fields only appear when this one is true.

-----------------------------------

# <a name="set/stake"></a> `set/stake`

Creates a stake change request for a profile's trust account. This request will add or remove stake to set the stake value to the requested amount. If the new value is more than the current stake amount, it adds stake from the account balance. If the new value is less, it removes stake to the account balance (with appropriate trust penalty, if applicable).

Requests are saved locally and take effect with the next stake block found by staking the profiles's trust account. Because they are saved locally, you must continue to stake on the machine where it was created until the next stake block is found, or the request will not be processed. Until implemented, you can update the request by calling set/stake again.

To remove a stake change request, you can either set an expiration time, or set the amount equal to the current trust account stake.

Endpoint:

```
finance/set/stake
```

### Parameters:

`pin` : Required if **locked**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

`amount` : Required the **new amount** of NXS to stake. (This is the total amount of NXS to be staked. If staking 15,000 NXS and want to increase the stake to 20,000 NXS then the amount is 20,000. When withdrawing 2,000 NXS from a 20,000 NXS stake then the amount is 18,000)

`expires` : Optional field to assign the number of seconds until the stake change request **expires**. A value of zero indicates it does not expire. Default is zero if not passed.

### Results:

#### Return value JSON object:

```
{
    "txid": "318b86d2c208618aaa13946a3b75f14472ebc0cce9e659f2830b17e854984b55606738f689d886800f21ffee68a3e5fd5a29818e88f8c5b13b9f8ae67739903d"
}
```

#### Return values:

`txid` : The ID (hash) of the transaction that includes the stake change.

-----------------------------------

# <a name="void/transaction"></a> `void/transaction`

Voids (reverses) a debit or transfer transaction, only if it has not been credited or claimed by the recipient. The method creates a corresponding credit or claim transaction but back to the originating account / profile. This means that any applicable fees will apply, as will conditions on the debit / transfer transaction (such as expiration conditions).

For debits that were made to a tokenized asset as part of a split payment transaction, the reversing credit will be made for the debit amount minus any partial amounts that have already been credited by the token holders.

```
finance/void/transaction
```

### Parameters:

`pin` : Required if **locked**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

`txid` : The **transaction ID** (hash) of the debit or transfer transaction that you wish to void.

### Results:

#### Return value JSON object:

```
{
    "hash": "47959e245f45aab773c0ce5320a5454f49ac15f63e15acb36855ac654d54d6314fe36b61dd64ec7a9a546bcc439a628e9badcdccb6e5f8072d04a0a3b67f8679"
}
```

#### Return values:

`hash` : The transaction hash of the credit transaction, if successfully committed to the mempool / broadcast.

-----------------------------------

# <a name="migrate/accounts"></a> `migrate/accounts`

This method will migrate your legacy accounts to signature chain accounts, sending the balance across in the process. A new account will be created in your signature chain for each legacy account, with a corresponding matching name (unless flagged not to create names). The balance of each legacy account is sent to the newly created signature chain account in individual transactions. As such, each transaction incurs the default legacy fee of 0.01 NXS, which is deducted from the amount being migrated.

The method uses the arbitrary data field in the account object register to track which legacy account it was created from. As a result, it is possible to invoke this method multiple times, and each time it will sweep any NXS from legacy accounts to existing signature chain accounts (as well as creating any necessary new accounts).

Endpoint:

```
finance/migrate/accounts
```

### Parameters:

`pin` : Required if **locked**. The `PIN` for this profile.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

`walletpassphrase` : Optional field to provide the **wallet passphrase**. This value is required if the wallet is not currently unlocked.

`createname` : Optional boolean field indicating whether to **create a Name record** for the newly created signature chain accounts. If omitted, the default behaviour is to create a Name record, which incurs a fee of 1 NXS per account.

### Results:

#### Return value JSON object:

```
[
    {
        "account": "default",
        "address": "8BzPhYgcHAP26CYnCvHTQmPiBH7FBC2PAB4rR9YgWxcK7L4c46X",
        "amount": 44.9,
        "txid": "022fed45add52c82110411c703c070519ef1136f3fe401def06a9fe29b0fe378935f17c3846c8824410e197f1caea2aaae5e994530cf6ee3d06207dd46126171"
    },
    {
        "account": "test1",
        "address": "8CEwJUPAkQQuBbAoQtiiswwk38ASuUZ63JQ2yLakV2cEXPbsmsd",
        "amount": 1.0,
        "txid": "02b88a4a9972aafdf09461bd976617162b8f8df853f46a5e0d98d608b987d5d52a3cb67c3861e6b47b65721d83b4b47c851af32b01df3292d1e814242966b658"
    }
]
```

#### Return values:

`account` : The legacy account name (and tritium account name, unless createname=false was explicitly set in the request).

`address` : The register address of the signature chain account that the legacy funds have been migrated to.

`amount` : The NXS amount transferred from the legacy account to the signature chain account.

`txid` : If the the legacy send was successful, the ID (hash) of the legacy transaction.

`error` : If the legacy send failed, this field includes the legacy send error message.

-----------------------------------

