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


## `Supported Operators`

The following operators are supported for this API command-set:

[`array`](#array) - Generate a list of values given from a set of filtered results.     
[`mean`](#mean)   - Calculate the mean or average value across a set of filtered results.  
[`sum`](#sum)   - Compute a sum of a set of values derived from filtered results.

**Example:**

```
finance/list/accounts/balance/sum
```

**Result:**

This command will return a sum of the balances for all accounts:

```
{
    "balance": 333.376
}
```

-----------------------------------

## `Supported Filters`

This command-set supports single or csv field-name filters.

**Example:**

```
finance/list/accounts/balance,ticker
```

The above command will return an array of objects with only the `balance` and `ticker` JSON keys.

### `Recursive Filtering`

Nested JSON objects and arrays can be filtered recursively using the `.` operator.

```
finance/transactions/account/contracts.OP
```

When using recursive filtering, the nested hiearchy is retained.

```
[
    {
        "contracts": [
            {
                "OP": "CREATE"
            }
        ]
    },
    {
        "contracts": [
            {
                "OP": "CREDIT"
            }
        ]
    }
]

```

-----------------------------------

## `Supported Nouns`

The following nouns are supported for this API command-set:

[`account`] - An object register containing a token-id and balance.     
[`trust`]   - An object register containing a token-id, balance, and trust.     
[`token`]   - An object register containing a token-id, balance, supply, and decimals.      
[`any`]     - An object selection noun allowing mixed accounts of different tokens.     
[`all`]     - An object selection noun to collect all accounts for given token type.    

**Example:**

```
finance/debit/any
```

The above command will create a debit contract withdrawing from a random sample of your accounts, for all tokens you own.

-----------------------------------


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


## `Direct Endpoints`

The following commands are direct endpoints and thus do not support the above `verb` and `noun` structure available above.

[`get/balances`](#create)    
[`get/stakeinfo`](#credit)  
[`migrate/accounts`](#debit)    
[`set/stake`](#get)

Direct endpoints support filters and operators.

-----------------------------------

## <a name="create"></a> `Sorting / Filtering`

The following parameters can be used to apply **sorting** and **filtering** to the returned data-set.

`limit`: The number of records to return. *Default: 100*.

`page`: Zero-indexed page number that depends on `limit` for page boundaries.

`offset`: Alternative to `page`, offset can be used to page the results by index.

`order`: Descending **desc** or ascending **asc** as only permitted values.

`sort`: The column or field-name to apply the sorting logic to. This parameter supports moving up levels of JSON keys by using `.`, such as `sort=json.date` would apply a sort to a nested JSON object:

```
{
    "modified": 1621782289,
    "json": {
        "account": "8Cdr874GBd8t6MaQ4BVK8fXVVpzVHrGwZpQquUVzUXZroruYdeR",
        "date": "12-21-2020"
    }
}
```

`where`: Apply a boolean statement to the results of command, following the SQL-DSL syntax.

### Alternative input

The `limit` and `offset` parameters can be given with the following format:

```
limit=100.10
```

This above will map to the parameters of `limit=100` and `offset=10`.


```
finance/get/balances/balance/sum
```

-----------------------------------

# <a name="create"></a> `create`

Create a new object register specified by given noun.

```
finance/create/noun
```

This command does not support the `any` or `all` nouns.

## Parameters:

`pin` : Required if  **locked**. The `PIN` to authorize the transaction.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session that is creating the transaction.

`name` : Optional for **noun** `name` as a *UTF-8* encoded string that will generate a name object register that points to new object. If noun is `token` this will be created as a global name.

`data` : Optional for **any** noun, allows caller to add arbitrary data to object.


#### create/token ####

`supply` : Required by **noun** `token` that sets the maximum token supply.

`decimals` : Required by **noun** `token` that sets the total number of significant figures. Defaults to 2.

#### create/account ####

`token` : Required by **noun** `account` as a *Base58* encoded address or ticker name. Defaults to `NXS`.


## Results: ##

`txid` : The hash of the transaction that was generated for this tx. If using `-autotx` this field will be ommitted.

`address` : The register address for this  account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.

```
{
    "success": true,
    "address": "8ESvYizqdApiuKEBjZMF1hnB8asDqECaDwAstcH3UtJ4Z6ceCn2",
    "txid": "0131e17af8029b414814283a3d90813d12c238db6ddab213440249b795090a9cd77079d5804ec38303a59414d87108d4e44bf31f54a6c176285281a88ab5d737"
}
```

-----------------------------------

# <a name="credit"></a> `credit`

Create a new object register specified by given noun.

```
finance/credit/noun
```

This command supports the `any` wildcard noun.

## Parameters:

`pin` : Required if  **locked**. The `PIN` to authorize the transaction.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session that is creating the transaction.

`txid` : The hash in **hexadecimal** encoding of the transaction that we are crediting.


### Return values:

`txid` : The ID (hash) of the transaction that includes the account creation.

`address` : The register address for this  account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.

-----------------------------------
