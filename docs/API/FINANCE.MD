# Finance API
-----------------------------------

The Finance API provides methods for sending and receiving NXS coins (tokens) between users / accounts, creating accounts, and managing staking.


## `Named Shortcuts`

For each API method we support an alternative endpoint that includes the account name or register address at the end of the the URL.  This shortcut removes the need to include the account name or address as an additional parameter.

For example `finance/create/account/savings` is a shortcut to `finance/create/account?name=savings`.

Similarly `finance/get/account/5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9` is a shortcut to `finance/get/account?address=5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9`

The logic for resolving the shortcut to either a name or address is that if the data is 64 characters of hexadecimal then it will be assumed to be a register address.  Otherwise it will be considered a name.


## `Methods`

The following methods are currently supported by this API

[`create/account`](#createaccount)    
[`debit/account`](#debitaccount)    
[`credit/account`](#creditaccount)    
[`get/account`](#getaccount)    
[`list/accounts`](#listaccounts)    
[`list/account/transactions`](#listaccounttransactions)    
[`get/stakeinfo`](#getstakeinfo)    
[`set/stake`](#setstake)    
[`migrate/stake`](#migratestake)    
[`get/balances`](#getbalances)    


-----------------------------------
***

# `create/account`

Create a new account for receiving NXS. The API supports an alternative endpoint that can include the account name in the URL.  For example `/finance/create/account/savings` will resolve to `finance/create/account?name=savings`.


### Endpoint:

`/finance/create/account`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the account should be created with. For single-user API mode the session should not be supplied.

`name` : An optional name to identify the account. If provided a Name object will also be created in the users local namespace, allowing the account to be accessed/retrieved by name. If no name is provided the account will need to be accessed/retrieved by its 256-bit register address.


### Example:

This example creates an account called "main".

```
{
    "pin": "1234",
    "name": "main"
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

`address` : The register address for this  account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.


***

# `debit/account`

Deduct an amount of NXS from an account and send it to another account or legacy UTXO address. The API supports an alternative endpoint that can include the account name in the URL.  For example `/finance/debit/account/savings` will resolve to `finance/debit/account?name=savings`.


### Endpoint:

`/finance/debit/account`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) the account owner. For single-user API mode the session should not be supplied.

`name` : The name identifying the account to debit. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the account to debit. This is optional if the name is provided.

`amount` : The amount of NXS to debit.

`name_to` : The name identifying the account to send to. This is optional if the address is provided.  The name should be in username:name format

`address_to` : The register address of the account to send to. This is optional if the name is provided.  The address_to can also contain a legacy UTXO address if sending from a signature chain account to a legacy address.

`reference` : This optional field allows callers to provide a reference, which the recipient can then use to relate the transaction to an order number, invoice number etc.  The reference is be a `64-bit unsigned integer` in the range of 0 to 18446744073709551615

`expires` : This optional field allows callers to specify an expiration for the debit transaction. The expires value is the `number of seconds` from the transaction creation time after which the transaction can no longer be credited by the recipient.  Conversely, when you apply an expiration to a transaction, you are unable to reverse the transaction until after the expiration time.    


### Example:

The following example shows the json payload for a debit of 10.5 NXS from an account called "main" (that itself was created by the sig chain logged into session "5e9d8aa625a1838f60f30e12058089169e32c968389f365428f7b0c878bb47f8") being sent to an account identified by the name "bob:savings".  In the name_to field, "bob" refers to the username of the account holder and "savings" is their NXS account.  If Bob is concerned about privacy he could have provided the register address of his token account instead

```
{
    "pin": "1234",
    "session": "5e9d8aa625a1838f60f30e12058089169e32c968389f365428f7b0c878bb47f8",
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

`txid` : The ID (hash) of the transaction that includes the debit.

***

# `credit/account`

Increment an amount received from another NXS account to an account owned by your signature chain. The API supports an alternative endpoint that can include the account name to credit in the URL.  For example `/finance/credit/account/savings` will resolve to `finance/credit/account?name=savings`.


### Endpoint:

`/finance/credit/account`


### Parameters:

`pin` : The PIN for this signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the account. For single-user API mode the session should not be supplied.

`txid` : The transaction ID (hash) of the corresponding debit transaction for which you are creating this credit for.  The transaction can also be a legacy transaction created from sendtoaddress or sendmany where the inputs are UTXO but the output is an account register address. 

`name` : The name identifying the account to credit. This is only required when crediting a coinbase or split payment transaction (where the receiving account is not included in the credit transaction) and is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the account to credit. This is only required when crediting a coinbase or split payment transaction (where the receiving account is not included in the credit transaction) and is optional if the name is provided.

`name_proof` : The name identifying the account that proves your ownership of the share of the debit. This is only required for split payments and is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the account was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address_proof` : The register address of the account that proves your ownership of the share of the debit. This is only required for spit payments and is optional if the name is provided.


### Example:

The following example shows the json payload for a credit to an account called "savings" (that itself was created by the sig chain logged into session "5e9d8aa625a1838f60f30e12058089169e32c968389f365428f7b0c878bb47f8") identified by the name "bob:savings".  In the name field, "bob" refers to the username of the account holder and "savings" is their NXS account.

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

`txid` : The ID (hash) of the transaction that includes the credit.

***

# `get/account`

Retrieves information about a NXS account . The API supports an alternative endpoint that can include the account name in the URL.  For example `/finance/get/account/savings` will resolve to `finance/get/account?name=savings`.

Additionally the API supports passing a field name in the URL after the account name, which will populate the `fieldname` parameter in the request and filter the response on that field.  For example `/finance/get/account/savings/balance` will resolve to `finance/get/account?name=savings&fieldname=balance`


### Endpoint:

`/finance/get/account`


### Parameters:

`name` : The name identifying the NXS account to retrieve. This is optional if the address is provided.  The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the object was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the name in order to deduce the register address of the object.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

`address` : The register address of the NXS account to retrieve. This is optional if the name is provided.

`fieldname`: This optional field can be used to filter the response to return only a single field from the account.


### Return value JSON object:
```
{
    "owner": "a2e51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0",
    "created": 1566534164,
    "modified": 1566616211,
    "name": "default",
    "address": "8CbkwEQ9S8owmX74joU6XmiwxJq1aoiqUoXc9fLCKzw15HscM99",
    "token_name": "NXS",
    "token": "0",
    "balance": 2903.239792,
    "pending": 0.0,
    "unconfirmed": 76.492244
}

```


### Return values:

`owner` : The genesis hash of the signature chain that owns this account.

`created` : The UNIX timestamp when the account was created.

`modified` : The UNIX timestamp when the account was last modified.

`name` : The name identifying the account.  For privacy purposes, this is only included in the response if the caller is the owner of the account

`address` : The register address of the account.  The address is a hash based on the owners username and account name.

`token` : This will always be set to 0 for NXS accounts.

`balance` : The available balance of this account.  This is the last confirmed balance less any new debits that you have made since the last block

`pending` : This is the sum of all confirmed debit transactions that have been made to this account, that have not yet been credited.  To move coins from pending into the available balance you must create a corresponding credit transaction.  NOTE: if configured to run, the events processor does this for you.

`unconfirmed` : This is the sum of all unconfirmed debit transactions that have been made to this account PLUS the sum of all unconfirmed credits that you have for confirmed debit transactions.  When someone makes a debit to your account it will immediately appear in the unconfirmed balance until that transaction is included in a block, at which point it moves into `pending`.  When you (or the events processor) creates the corresponding credit transaction for that debit, the amount will move from `pending` back into `unconfirmed` until the credit transaction is included in a block, at which point the amount moves to `balance`.  

`stake` : Only returned for the trust account, this is the amount of NXS currently staked in the trust account.


***

# `list/accounts`

This will list off all of the NXS accounts belonging to the currently logged in signature chain.


### Endpoint:

`/finance/list/accounts`


### Parameters:

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the account. For single-user API mode the session should not be supplied.


### Return value JSON object:
```
[
    {
        "created": 1568025836,
        "modified": 1568025836,
        "name": "default",
        "address": "8CbkwEQ9S8owmX74joU6XmiwxJq1aoiqUoXc9fLCKzw15HscM99",
        "token_name": "NXS",
        "token": "0",
        "balance": 5000,
        "pending": 0.0,
        "unconfirmed": 76.492244
    },
    {
        "created": 1568025836,
        "modified": 1568025836,
        "name": "savings",
        "address": "8GhrC2TKkU4ra9Uuj8LuiAyxDAtza2u483N1rKDSaVp24dNgUy9",
        "token_name": "mytoken",
        "token": "8GHrC2TKkU4ra9Uuj8LuiAyxDAtza2u483N1rKDSaVp24dNgUx8",
        "balance": 10000.0,
        "pending": 0.0,
        "unconfirmed": 0.0
    }
]
```


### Return values:

`created` : The UNIX timestamp when the account was created.

`modified` : The UNIX timestamp when the account was last modified.

`name` : The name identifying the account.  For privacy purposes, this is only included in the response if the caller is the owner of the account

`address` : The register address of the account.  The address is a hash based on the owners username and account name.

`token_name` : This will always be set to `NXS`.

`token` : This will always be set to 0 for NXS accounts.

`balance` : The available balance of this account.  This is the last confirmed balance less any new debits that you have made since the last block

`pending` : This is the sum of all confirmed debit transactions that have been made to this account, that have not yet been credited.  To move coins from pending into the available balance you must create a corresponding credit transaction.  NOTE: if configured to run, the events processor does this for you.

`unconfirmed` : This is the sum of all unconfirmed debit transactions that have been made to this account PLUS the sum of all unconfirmed credits that you have for confirmed debit transactions.  When someone makes a debit to your account it will immediately appear in the unconfirmed balance until that transaction is included in a block, at which point it moves into `pending`.  When you (or the events processor) creates the corresponding credit transaction for that debit, the amount will move from `pending` back into `unconfirmed` until the credit transaction is included in a block, at which point the amount moves to `balance`.  

`stake` : Only returned for the trust account, this is the amount of NXS currently staked in the trust account.


****


# `list/account/transactions`

This will list off all of the transactions related to a given account. You DO NOT need to be logged in to use this command. If you are logged in, then neither username or genesis are required as it will default to the logged in user. 

**NOTE** : The returned transaction data will only include contracts that related to the requested account.  Any other contracts are omitted from the transaction result. 

**NOTE** : If you use the username parameter it will take slightly longer to calculate the username genesis with our brute-force protected hashing algorithm. For higher performance, use the genesis parameter.


### Endpoint:

`/finance/list/account/transactions`


### Parameters:


`genesis` : The genesis hash identifying the signature chain to scan for transactions (optional if username is supplied or already logged in).

`username` : The username identifying the signature chain to scan for transactions(optional if genesis is supplied or already logged in).

`session` : For multi-user API mode, (configured with multiuser=1) the session can be provided in conjunction with the account name in order to deduce the register address of the account.  The `session` parameter is only required when a name parameter is also provided without a namespace in the name string.  For single-user API mode the session should not be supplied.

`name` : The name identifying the NXS account to list transactions for. This is optional if the address is provided. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace).  However, if the object was created in the callers namespace (their username), then the username can be omitted from the name if the `session` parameter is provided (as we can deduce the username from the session)

`address` : The register address of the NXS account to list transactions for. This is optional if the name is provided.

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
                "output": 0,
                "proof": "a2e51edcd41a8152bfedb24e3c22ee5a65d6d7d524146b399145bced269aeff0",
                "account": "d430231b0c1973c784cc9942daa32f704db571a5c7d2a1f091e91fdd4abe622b",
                "account_name": "default",
                "amount": 76.450762,
                "token": "0",
                "token_name": "NXS"
            },
            {
                "OP": "FEE",
                "account": "d430231b0c1973c784cc9942daa32f704db571a5c7d2a1f091e91fdd4abe622b",
                "account_name": "default",
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

`confirmations` : The number of confirmations that this transaction obtained by the network.

`genesis` : The signature chain genesis hash.

`nexthash` : The hash of the next transaction in the sequence.

`prevhash` : the hash of the previous transaction in the sequence.

`pubkey` : The public key.

`signature` : The signature hash.

`contracts` : The array of contracts bound to this transaction and their details with opcodes.    
{    
&nbsp;&nbsp;&nbsp;`OP` : The contract operation. Can be `APPEND`, `CLAIM`, `COINBASE`, `CREATE`, `CREDIT`, `DEBIT`, `FEE`, `GENESIS`, `LEGACY`, `TRANSFER`, `TRUST`, `STAKE`, `UNSTAKE`, `WRITE`.

&nbsp;&nbsp;&nbsp;`for` : For `CREDIT` transactions, the contract that this credit was created for . Can be `COINBASE`, `DEBIT`, or`LEGACY`.

&nbsp;&nbsp;&nbsp;`txid` : The transaction that was credited / claimed.

&nbsp;&nbsp;&nbsp;`proof` : The register address proving the credit.

&nbsp;&nbsp;&nbsp;`account` : For `CREDIT` transactions, the register address of the account that the transaction relates to.

&nbsp;&nbsp;&nbsp;`account_name` : For `CREDIT` transactions, the name of the account that the transaction relates to.  Only included if the name can be resolved.

&nbsp;&nbsp;&nbsp;`from` : For `DEBIT` and `CREDIT` transactions, the register address of the account that the debit is being made from.

&nbsp;&nbsp;&nbsp;`from_name` : For `DEBIT` and `CREDIT` transactions, the name of the account that the debit is being made from.  Only included if the name can be resolved.

&nbsp;&nbsp;&nbsp;`to` : For `DEBIT` transactions, the register address of the account that the debit is being made to.

&nbsp;&nbsp;&nbsp;`to_name` : For `DEBIT` transactions, the name of the account that the debit is being made to.  Only included if the name can be resolved.

&nbsp;&nbsp;&nbsp;`amount` : the token amount of the transaction.

&nbsp;&nbsp;&nbsp;`token` : the register address of the token that the transaction relates to.  Set to 0 for NXS transactions

&nbsp;&nbsp;&nbsp;`token_name` : The name of the token that the transaction relates to.

&nbsp;&nbsp;&nbsp;`reference` : For `DEBIT` and `CREDIT` transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number.

}

***

# `get/stakeinfo`

This will retrieve account values and staking metrics for the trust account belonging to the currently logged in signature chain. If called when the stake minter is not running, this method only returns trust account values. Staking metrics will return 0.


### Endpoint:

`/finance/get/stakeinfo`


### Parameters:

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the trust account. For single-user API mode the session should not be supplied.

`fieldname`: This optional field can be used to filter the response to return only a single field from the account.


### Return value JSON object:
```
{
    "address": "5efc8a9437d93e894defd50f8ba73a0b2b5529cc593d5e4a7ea413022a9c35e9",
    "balance": 150,
    "stake": 5000,
    "trust": 54322,
    "new": false,
    "staking": true
    "stakerate": 1.97,
    "trustweight": 58.97,
    "blockweight": 47.62,
    "stakeweight": 54.03,
    "change": false
}
```


### Return values:

`address` : The register address of the trust account. The address is a hash based on the owners username and account name.

`balance` : The current NXS balance of the trust account. This is general account balance that is not staked.

`stake` : The amount of NXS currently staked in the trust account.

`trust` : The current raw trust score of the trust account.

`new` : Indicates whether trust account is new (true, staking Genesis) or established (false, staking Trust).

`staking` : Indicates whether staking is actively running for user account (when false, weight metrics will be 0).

`stakerate` : The current annual reward rate earned for staking as an annual percent.

`trustweight` : The current trust weight applied to staking as a percent of maximum.

`blockweight` : The current block weight applied to staking as a percent of maximum.

`stakeweight` : The current stake weight (trust weight and block weight combined) as a percent of maximum.

`change` : Indicates whether or not there is a pending request to change stake. The remaining fields only appear when this one is true.   
{    
&nbsp;&nbsp;&nbsp;`amount` : Amount of stake change. Positive will be added to stake (moved from balance) and negative will unstake (move to balance).

&nbsp;&nbsp;&nbsp;`requested` : Timestamp when the stake change request was created.

&nbsp;&nbsp;&nbsp;`expires`: Timestamp when the stake change request expires, if assigned. Zero indicates does not expire. 

} 

***

# `set/stake`

Creates a stake change request for a signature chain's trust account. This request will add or remove stake to set the stake value to the requested amount. If the new value is more than the current stake amount, it adds stake from the account balance. If the new value is less, it removes stake to the account balance (with appropriate trust penalty, if applicable). 

Requests are saved locally and take effect with the next stake block found by staking the signature chain's the trust account. Because they are saved locally, you must continue to stake on the machine where it was created until the next stake block is found, or the request will not be processed.

Until implemented, you can update the request by calling set/stake again. 

To remove a stake change request, you can either set an expiration time, or set the amount equal to the current trust account stake. 


### Endpoint:

`/finance/set/stake`


### Parameters:

`pin` : The PIN for the signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the trust account. For single-user API mode the session should not be supplied.

`amount` : The amount of NXS to add to stake.

`expires` : Optional field to assign the number of seconds until the stake change request expires. A value of zero indicates it does not expire. Default is zero if not passed. 


### Return value JSON object:
```
{
    "txid": "318b86d2c208618aaa13946a3b75f14472ebc0cce9e659f2830b17e854984b55606738f689d886800f21ffee68a3e5fd5a29818e88f8c5b13b9f8ae67739903d"
}
```


### Return values:

`txid` : The ID (hash) of the transaction that includes the stake change.

***

# `migrate/stake`

Initiates the process to migrate a legacy trust key to a signature chain's trust account. This process will migrate the current trust key balance and accumulated trust.

You can only migrate to a new, previously unused trust account. Check get/stakeinfo to verify that the value returned for "new" is true. 

The transaction created by migrate/stake is a special form of debit, and it will show up in the output of users/list/notifications to be claimed in the same manner as a normal debit. Issuing a credit against this debit finalizes the migrate and completes the trust key migration process.


### Endpoint:

`/finance/migrate/stake`


### Parameters:

`pin` : The PIN for the signature chain.

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the trust account. For single-user API mode the session should not be supplied.

`walletpassphrase` : Optional field to provide the wallet passphrase. This value is required if the wallet is not currently unlocked.


### Return value JSON object:
```
{
    "txid": "318b86d2c208618aaa13946a3b75f14472ebc0cce9e659f2830b17e854984b55606738f689d886800f21ffee68a3e5fd5a29818e88f8c5b13b9f8ae67739903d"
}
```


### Return values:

`txid` : The ID (hash) of the migrate transaction.

***

# `get/balances`

This will retrieve a summary of balance information for all NXS accounts belonging to the currently logged in signature chain.


### Endpoint:

`/finance/get/balances`


### Parameters:

`session` : For multi-user API mode, (configured with multiuser=1) the session is required to identify which session (sig-chain) owns the trust account. For single-user API mode the session should not be supplied.


### Return value JSON object:
```
{
    "available": 1000,
    "pending": 50,
    "unconfirmed": 5,
    "stake": 5000,
    "immature": 100
}
```


### Return values:

`available` : The current balance across all NXS accounts that is available to be spent.

`pending` : The sum of all debit and coinbase transactions made to your NXS accounts that are confirmed but have not yet been credited.  This does NOT include immature and unconfirmed amounts

`unconfirmed` : The sum of all debit transactions made to your NXS accounts that are not confirmed, or credits you have made to your accounts that are not yet confirmed (not yet included in a block)

`immature`  The sum of all coinbase transactions that have not yet reached maturity.   

`stake` : The amount of NXS currently staked in the trust account.


***
