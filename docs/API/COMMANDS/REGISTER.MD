# Register API
-----------------------------------

The Register API gives access to the register data and this allows network wide information to be presented. The full supported endpoint of the profiles URI is as follows:

```
register/verb/noun/filter/operator
```

The minimum required components of the URI are:

```
register/verb/noun
```

## `Supported Verbs`

The following verbs are currently supported by this API command-set:

[`get`](#get) - Get object of supported type.  
[`list`](#list) - List all objects owned by given user.  
[`history`](#history) - Generate the history of all last states.  
[`transactions`](#transactions) - List all transactions that modified specified object.  

## `Supported Nouns`

The following nouns are supported for this API command-set:

[`finance:account`] - An object register containing a token-id and balance.  
[`finance:trust`] - An object register containing a token-id, balance, and trust.  
[`finance:token`] - An object register containing a token-id, balance, supply, and decimals.  
[`names:name`] - An object register containing names and global names.  
[`names:namespace`] - An object register containing namespaces.
[`names:global`] - An object register containing global names.
[`crypto`] - An object register which holds public key hashes.  
[`object`] - An object register containing user-defined data structure.  
[`assets:raw`] -  An object register of type raw.  
[`assets:readonly`] - An object register of type readonly.  
[`append`] - An object register of type append.  
[`any`] - An object selection noun allowing mixed accounts of different tokens.  

-----------------------------------


# <a name="get"></a> `get`

Retrieves information for a single object for a type specified by the noun

```
register/get/noun
```

This command supports all the nouns.

#### get/finance:account

Retrieves information for a specified NXS or token account.

#### get/finance:trust

Retrieves information for a specified trust account.

#### get/finance:token

Retrieves information for a specified token address.

#### get/names:name

Retrieves information for a specified name.

#### get/names:namespace

Retrieves information for a specified namespace.

#### get/crypto

Retrieves information for a specified profile crypto object register.

#### get/object

Retrieves information for a specified object register.

#### get/assets:readonly

Retrieves information for a specified readonly register.

#### get/assets:raw

Retrieves information for a specified raw register.

#### get/append

Retrieves information for a specified append register.

### Parameters:

`name` : Required to **identify** the name of the account/trust/token. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the account/trust/token. This is optional if the `name` is provided.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD). 


-----------------------------------

# <a name="list"></a> `list`

This method provides the user with the ability to directly access the object register data specified by the noun and does not need the user to be logged in.

```
register/list/noun
```

This command supports all the nouns.

#### list/finance:account

Returns a list of all NXS and token accounts.

#### list/finance:trust

Returns a list of all the trust accounts on the blockchain.

#### list/finance:token

Returns a list of all the token address issued on the blockchain.

#### list/names:name

Returns a list of all the names.

#### list/names:namespace

Returns a list of all the namespaces.

#### list/names:global

Returns a list of all the global names.

#### list/crypto

Returns a list of all the crypto object registers.

#### list/object

Returns a list of all the object registers.

#### list/readonly

Returns a list of all the readonly registers.

#### list/raw

Returns a list of all the raw registers.

#### list/append

Returns a list of all the append registers.

### Parameters:

`where` : An array of clauses to **filter** the JSON results. More information on filtering the results from /list/xxx API methods can be found here: [`Queries`](/docs/API/QUERIES.md)

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD). 

[`Operators`](//docs/API/OPERATORS.MD). 

-----------------------------------

# <a name="history"></a> `history`

This will get the history of the specified noun.

```
register/history/noun
```

This command supports all the nouns.

#### history/finance:account

This will get the history and ownership of the specified NXS or token account.

#### history/finance:trust

This will get the history and ownership of the specified trust account. 

#### history/finance:token

This will get the history and ownership of the specified token addresses.

#### history/names:name

This will get the history and ownership of the specified name.

#### history/names:namespace

This will get the history and ownership of the specified namespace.

#### history/crypto

This will get the history of the specified crypto object register.

#### history/object

This will get the history and ownership of the specified object register.

##### history/assets:readonly

This will get the history and ownership of the specified readonly register.

#### history/assets:raw

This will get the history and ownership of the specified raw register.

#### history/append

This will get the history and ownership of the specified append register.

### Parameters:

`name` : Required to **identify** the name of the account/trust/token. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the account/trust/token. This is optional if the `name` is provided.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 

-----------------------------------

# <a name="transactions"></a> `transactions`

This will list off all of the transactions for the specified noun.

```
register/transactions/noun
```

This command supports all the nouns.

#### transactions/account

List out all the transactions for the specified NXS account or token account.

#### transactions/trust

List out all the transactions for the specified trust account

#### transactions/token

List out all the transactions for the specified token address.

#### transactions/name

List out all the transactions for the specified name.

#### transactions/namespace

List out all the transactions for the specified namespace.

#### transactions/crypto

List out all the transactions for the specified crypto object register.

#### transactions/object

List out all the transactions for the specified object register.

#### transactions/readonly

List out all the transactions for the specified readonly register.

#### transactions/raw

List out all the transactions for the specified raw register.

#### transactions/append

List out all the transactions for the specified append register.

### Parameters:

`verbose` : Optional, determines how much transaction data to include in the response. Supported values are :
- `default` : hash

- `summary` : type, version, sequence, timestamp, operation, and confirmations.

- `detail` : genesis, nexthash, prevhash, pubkey and signature.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 


## Register API with Query DSL

A few register API calls which will showcase the power of registers, LLD, which is the database and  Query-DSL. This is going to be a powerful tool for developers.

All these calls will require the latest build of the core and will not work with the current stable version.

To calculate the sum of all NXS on the tritium chain:

```
register/list/accounts,trust/total/sum sort=total order=desc limit=none where='object.token=0'
```

List all the namespaces created in ascending order:

```
register/list/namespaces sort=created order=asc
```

List all the global names on the network:

```
register/list/names where=object.namespace=~GLOBAL~
```

Create a rich list:

```
register/list/accounts,trust sort=total order=desc page=0 where=(object.token=0)
```

-----------------------------------
