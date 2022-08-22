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

[`account`] - An object register containing a token-id and balance.  
[`trust`] - An object register containing a token-id, balance, and trust.  
[`token`] - An object register containing a token-id, balance, supply, and decimals.  
[`name`] - An object register containing Names and Global Names.  
[`namespace`] - An object register containing namespaces.  
[`crypto`] - An object register which holds public key hashes.  
[`object`] - An object register containing user-defined data structure.  
[`raw`] -  An object register containing raw data.  
[`readonly`] - An object register which is readonly.  
[`append`] - An object register which can be appended.  
[`any`] - An object selection noun allowing mixed accounts of different tokens.  

-----------------------------------


# <a name="get"></a> `get`

Retrieves information for a single object for a type specified by the noun

```
register/get/noun
```

This command  supports the account, trust and token nouns.

#### get/account

Retrieves information for a specified NXS or token account.

#### get/trust

Retrieves information for a specified trust account.

#### get/token

Retrieves information for a specified token address.

#### get/name

Retrieves information for a specified name.

#### get/namespace

Retrieves information for a specified namespace.

#### get/crypto

Retrieves information for a specified profile crypto object register.

#### get/object

Retrieves information for a specified object register.

#### get/readonly

Retrieves information for a specified readonly register.

#### get/raw

Retrieves information for a specified raw register.

#### get/append

Retrieves information for a specified append register.

### Parameters:

`name` : Required to **identify** the name of the account/trust/token. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the account/trust/token. This is optional if the `name` is provided.


-----------------------------------

# <a name="list"></a> `list`

This method provides the user with the ability to directly access the object register data specified by the noun and does not need the user to be logged in.

```
register/list/noun
```

#### list/account

Returns a list of all NXS and token accounts.

#### list/trust

Returns a list of all the trust accounts on the blockchain.

#### list/token

Returns a list of all the token address issued on the blockchain.

#### list/name

Returns a list of all the names.

#### list/namespace

Returns a list of all the namespaces.

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

The parameters used are Filtering Query DSL, along with the sorting / filtering


-----------------------------------

# <a name="history"></a> `history`

This will get the history of the specified noun.

```
register/history/noun
```

This command supports the account, trust and token nouns.

#### history/account

This will get the history and ownership of the specified NXS or token account.

#### history/trust

This will get the history and ownership of the specified trust account. 

#### history/token

This will get the history and ownership of the specified token addresses.

#### history/name

This will get the history and ownership of the specified name.

#### history/namespace

This will get the history and ownership of the specified namespace.

#### history/crypto

This will get the history of the specified crypto object register.

#### history/object

This will get the history and ownership of the specified object register.

##### history/readonly

This will get the history and ownership of the specified readonly register.

#### history/raw

This will get the history and ownership of the specified raw register.

#### history/append

This will get the history and ownership of the specified append register.

### Parameters:

`name` : Required to **identify** the name of the account/trust/token. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). This is optional if the `address` is provided

`address` : Required to **identify** the register address of the account/trust/token. This is optional if the `name` is provided.


### Results:

##### Return value JSON object:

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

-----------------------------------

# <a name="transactions"></a> `transactions`

This will list off all of the transactions for the specified noun.

```
register/transactions/noun
```

This command supports the account, trust and token nouns.

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

This method supports the Sorting / Filtering parameters.

### Results:

Return value JSON object:

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

`for` : For CREDIT transactions, the contract that this credit was created for . Can be COINBASE, DEBIT, orLEGACY.

`txid` : The transaction that was credited / claimed.

`contract` : The ID of the contract within the transaction that was credited / claimed.

`proof` : The register address proving the credit.

`from` : For DEBIT, CREDIT, FEE transactions, the register address of the account that the debit is being made from.

`from_name` : For DEBIT, CREDIT, FEE transactions, the name of the account that the debit is being made from. Only included if the name can be resolved.

`to` : For DEBIT and CREDIT transactions, the register address of the recipient account.

`to_name` : For DEBIT and CREDIT transactions, the name of the recipient account. Only included if the name can be resolved.

`amount` : the token amount of the transaction.

`token` : the register address of the token that the transaction relates to. Set to 0 for NXS transactions

`token_name` : The name of the token that the transaction relates to.

`reference` : For DEBIT and CREDIT transactions this is the user supplied reference used by the recipient to relate the transaction to an order or invoice number.

`object` : Returns a list of all hashed public keys in the crypto object register for the specified profile. The object result will contain the nine default keys(app1, app2, app3, auth, cert lisp, network, sign  and verify).  
}  


## Register API with Query DSL

A few register API calls which will showcase the power of registers, LLD, which is the database and  Query-DSL. This is going to be a powerful tool for developers.

All these calls will require the latest build of the core and will not work with the current stable version.

To calculate the sum of all NXS on the tritium chain.

```
register/list/accounts,trust/total/sum sort=total order=desc limit=none where='object.token=0'
```

List all the namespaces created in ascending order (First to recent):

```
register/list/namespaces sort=created order=asc
```

List all the Global Names on the Network:

```
register/list/names where=object.namespace=~GLOBAL~
```

Create a rich list:

```
register/list/accounts,trust sort=total order=desc page=0 where=(object.token=0)
```

-----------------------------------