# Market API
-----------------------------------
The Market API is an on-chain, order book based P2P marketplace for trading tokens and assets. Assets can include both digital and physical goods. Users start by creating a new market with a token pair (NXS/XYZ) and other users can participate in that particular market, execute a particular order or cancel a placed order.

## `How market prices Are quoted`

Each market pair represents the current exchange rate for the two tokens. Here’s how to interpret that information, using market=NXS/XYZ or the Nexus-to-XYZ exchange rate as an example:

**The current price is the last traded price and has nothing to do with the bid or ask price.** 

- The token on the left (NXS) is the base token.  
- The token on the right (XYZ) is the quote token.  
- The price quoted represents how much of the quote token is needed to buy 1 unit of the base token. As a result, the base token is always expressed as 1 unit while the quote currency varies based on the current market and how much is needed to buy 1 unit of the base currency.  
- If the NXS/XYZ exchange rate is 1.2, that means 1 NXS will buy 1.20 XYZ (or, put another way, it will cost 1.20 XYZ to buy 1 NXS).  
- When the market rate rises, that means the base token has risen in value relative to the quote token (because 1 NXS will buy more XYZ tokens) and conversely, if the market rate falls, that means the base token has fallen in value.  


**Note**: Market pairs are usually presented with the base token first and the quote token second, The market API allows to use the reverse market=XYZ/NXS and its treated as bid/ask depending on the noun.

The full supported endpoint of the market URI is as follows:

```
market/verb/noun/filter/operator
```

The minimum required components of the URI are:

```
market/verb/noun
```


## `Supported Verbs`

The following verbs are currently supported by this API command-set:

[`create`](#create) - Creates a new market place or becomes part of an existing one.  
[`list`](#list) - List all orders for a specified marketplace.  
[`execute`](#execute) - To full-fill a specified market order.  
[`cancel`](#cancel) - To cancel a specified market order.  
[`user`](#user) -  Retrieves all market orders for a user or profile.  

## `Supported Nouns`

The following nouns are supported for this API command-set:

[`bid`] - A market order placed to buy a token or asset.  
[`ask`] - A market order placed to sell a token or asset.  
[`order`] - A market order which can be a bid or ask.  
[`executed`] - A market order which has been full-filled.  

-----------------------------------

# <a name="create"></a> `create`

This creates a new market pair or becomes part of an existing market represented by the token or asset pair token1/token2. Market pair token2/token1 is also part of the same market.

```
market/create/noun
```

This command supports the `bid` and `ask` nouns.

#### create/bid

This creates a bid market order for market=token1/token2, ask for market=token2/token1.

#### create/ask

This creates a ask market order for market=token1/token2, bid for market=token2/token1.

**NOTE:** When market=token1/token2  is reversed to market=token2/token1 the price has to be changed accordingly. But the market price is determined by the first order which creates the market pair.


### Parameters:

`pin` : Required if **locked**. The `PIN` to authorize the transaction.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

`market` : token1/token2 - The **token pair** for which the order is created.

`amount` : The **amount** of token2 to be exchanged.

`price` : The **price** of the token2 in relation to token1.

`to` : This is the NXS or token **receiving account** name or register address to credit.

`from` : This is the NXS or token **sending account** name or register address to debit. 

### Results:

#### Return value JSON object:

```
{
    "success": true,
    "address": "8CupQ2dym1CZGZZ7U3F8UQ2xR2fr2PZmtEqB5Ds2EJsG1jA3JZw",
    "txid": "012bc5f80460a9605706e643f9364722e97ffe3dd4e94bce8e41ea6ae70b3336fe6274cfe74583bf72cf77d8bbdc951ae1b25c706253590e0c6d74fa4f78f4df"
}
[Completed in 4991.918611 ms]
```

#### Return Values:  

`success` : Boolean flag indicating that the market order was created successfully.

`address` : The register address for this account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.

`txid` : The hash of the transaction that was generated for the market order. If using -autotx this field will be ommitted.

-----------------------------------

# <a name="list"></a> `list`

This retrives a list of all the active orders for a particular market, specified by given noun.

```
market/list/noun
```

This command supports all the nouns.

### Parameters:

`pin` : Required if **locked**. The `PIN` to authorize the transaction.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

`market` : Required to **identify** the market token pair.

`where` : An array of clauses to **filter** the JSON results. More information on filtering the results from /list/xxx API methods can be found at [`Queries`](/docs/API/QUERIES.md).

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 

### Results:

#### Return value JSON object:

```
{
    "bids": [
        {
            "txid": "012bc5f80460a9605706e643f9364722e97ffe3dd4e94bce8e41ea6ae70b3336fe6274cfe74583bf72cf77d8bbdc951ae1b25c706253590e0c6d74fa4f78f4df",
            "timestamp": 1655815697,
            "owner": "b7392196b83aca438567558462cd0c5d982569c7cefa668500c4bf3e61a03b7a",
            "market": "XYZ/BIT",
            "price": 1.0,
            "type": "bid",
            "contract": {
                "OP": "DEBIT",
                "from": "8CupQ2dym1CZGZZ7U3F8UQ2xR2fr2PZmtEqB5Ds2EJsG1jA3JZw",
                "amount": 10.0,
                "token": "8EdBM41i1MAgb7w2tjEsdd7DeKntUcsw4JWK1LswW457e2jDa8a",
                "ticker": "BIT"
            },
            "order": {
                "OP": "DEBIT",
                "to": "8Cu9QHELK6ofHzbAwtWGnUcfkDz3zvGoSf7eDGZfh81AeMuqcYY",
                "amount": 10.0,
                "token": "8DXmAmkTtysSZUxM3ePA8wRmbSUofuHKSoCyDpN28aLuSrm1nDG",
                "ticker": "XYZ"
            }
        }
    ],
    "asks": [
        {
            "txid": "016d62c69f05a82eefb463604325dd0331aeee787748a06619d6ca665c5a0576705c62de29f20c3eccfb8cbb25c4df169a53baa1e55493f5293fbbf864af7389",
            "timestamp": 1655817006,
            "owner": "b7a57ddfb001d5d83ab5b25c0eaa0521e6b367784a30025114d07c444aa455c0",
            "market": "XYZ/BIT",
            "price": 1.0,
            "type": "ask",
            "contract": {
                "OP": "DEBIT",
                "from": "8B9adrgaFX8hZvH1vQGjibHVYhsLuDTDu2Kn1FM25RZQtuUFyiE",
                "amount": 10.0,
                "token": "8DXmAmkTtysSZUxM3ePA8wRmbSUofuHKSoCyDpN28aLuSrm1nDG",
                "ticker": "XYZ"
            },
            "order": {
                "OP": "DEBIT",
                "to": "8C3AiDrbDmbkeza1eBeeEJ2r5wqrnWGr9D1kewKqHfPzdY7jRkF",
                "amount": 10.0,
                "token": "8EdBM41i1MAgb7w2tjEsdd7DeKntUcsw4JWK1LswW457e2jDa8a",
                "ticker": "BIT"
            }
        }
    ]
}
```

### Return Values:

`order` : The array for bid or ask orders.

`txid` : The hash of the transaction that was generated for this tx. If using -autotx this field will be ommitted.

`timestamp` : The Unix timestamp of when the transaction was created.

`owner` : The username hash of the profile that owns this account.

`market` : The market for which the orders are listed.

`price` : The price for that particular order.

`type`  : The type of order bid or ask

`contracts` : The array of contracts bound to this transaction and their details with opcodes.  
{  
`OP` :  The contract operation. Can be APPEND, CLAIM, COINBASE, CREATE, CREDIT, DEBIT, FEE, GENESIS, LEGACY, TRANSFER, TRUST, STAKE, UNSTAKE, WRITE.

`from` : For DEBIT and CREDIT transactions, the register address of the senders account.

`to` : For DEBIT and CREDIT transactions, the register address of the recipient account.

`amount` : the token amount of the transaction.

`token` :  the register address of the token that the transaction relates to. Set to 0 for NXS transactions.

`ticker` :  The global name assigned to the token.    
}    

`address` : The register address for this account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account.

-----------------------------------

# <a name="execute"></a> `execute`

This allows to execute a particular order specified by given noun.

```
market/execute/noun
```

This command supports the `bid`, `ask` and `order` nouns.

### Parameters:

`pin` : Required if **locked**. The PIN to authorize the transaction.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

`txid`: The **transaction hash** of the bid /ask which is being executed.

`to` : This is the NXS or token **receiving account** name or register address to credit.

`from` : This is the NXS or token **sending account** name or register address to debit.

### Results:

#### Return value JSON object:

```
{
    "success": true,
    "address": "8CupQ2dym1CZGZZ7U3F8UQ2xR2fr2PZmtEqB5Ds2EJsG1jA3JZw",
    "txid": "012bc5f80460a9605706e643f9364722e97ffe3dd4e94bce8e41ea6ae70b3336fe6274cfe74583bf72cf77d8bbdc951ae1b25c706253590e0c6d74fa4f78f4df"
}
[Completed in 4991.918611 ms]
```

#### Return Values:

`success` :  Boolean flag indicating that the order was executed successfully.

`txid` : The hash of the transaction that was generated for this tx. If using -autotx this field will be ommitted.

`address` : The register address for this account. The address (or name that hashes to this address) is needed when
creating crediting or debiting the account.

-----------------------------------

# <a name="cancel"></a> `cancel`

This allows to cancel an active order specified by given noun.

```
market/cancel/noun
```

This command supports the `bid`, `ask` and `order` nouns.

### Parameters:

`pin` : Required if **locked**. The PIN to authorize the transaction.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

`txid` : The **transaction hash** of the order to be cancelled.

### Results:

#### Return value JSON object:

```
{
    "success": true,
    "txid": "010b0dbb226835c59b62d0761feac027cceb75ceb7d8d52abbd8072158415cf8c210af810f4d41ee6ad86dc6d754a17a0269db9025ba47becdb87a19ee71e99f"
}
[Completed in 4976.551936 ms]
```

#### Return Values:

`success` : Boolean flag indicating that cancelling the order was successful. 

`txid` : The hash of the transaction that was generated for cancelling the order. If using -autotx this field will be ommitted.

-----------------------------------

# <a name="user"></a> `user`

Retrieves a users market order information based on the noun.

```
market/user/noun
```

This command supports all the nouns.


### Parameters:

`pin` : Required if **locked**. The PIN to authorize the transaction.

`session` : Required by **argument** `-multiuser=1` to be supplied to identify the user session. For single-user API mode the session should not be supplied.

`market` : Required to **identify** the market token pair.

[`Sorting`](//docs/API/SORTING.MD).

[`Filtering`](/docs/API/FILTERING.MD).

[`Operators`](//docs/API/OPERATORS.MD). 


### Results:

#### Return value JSON object:

```
{
    "bids": [
        {
            "txid": "01722ce68a0fd5b22fdd5d1e90ec3ec4036f24592aafd9f414968a9c69355bb34012704bea1046557d81e297911c058b7623bd1f6c8a16974e5ed41528f1bb70",
            "timestamp": 1657201172,
            "owner": "b7fa11647c02a3a65a72970d8e703d8804eb127c7e7c41d565c3514a4d3fdf13",
            "market": "ETH/UPS",
            "price": 0.12,
            "type": "bid",
            "contract": {
                "OP": "DEBIT",
                "from": "8BJ747ASK45oU7UC5e2dePXeMviskmU1t5Kd4iyKLdSiCgKtLcJ",
                "amount": 10.0,
                "token": "8DS2qGLhuEC2reKrzxyWaXXwtVq2KmGWGQCWKBQwrCQc4XS2b8V",
                "ticker": "UPS"
            },
            "order": {
                "OP": "DEBIT",
                "to": "8CvvLqDpGNAtihtKcueSmW1SgLZPAKYANFMqW4A1Un5EW5nTsoh",
                "amount": 83.33,
                "token": "8DaVWMVaid9AULRZYVQL6tM2w2UzXJYE6B8AewTERF3FqbQngqQ",
                "ticker": "ETH"
            }
        }
    ],
    "asks": []
}
[Completed in 7.841892 ms]
```

#### Return Values:

`order` : The array for bid or ask orders.

`txid` : The hash of the transaction that was generated for this tx. If using -autotx this field will be ommitted.

`timestamp` : The Unix timestamp of when the transaction was created.

`owner` : The username hash of the profile that owns this account.

`market` : The market for which the orders are listed.

`price` : The price for that particular market order.

`type`  : The type of order bid or ask

`contracts` : The array of contracts bound to this transaction and their details with opcodes.  
{  
`OP` :  The contract operation. Can be CREDIT or DEBIT.

`from` : For DEBIT or CREDIT transactions, the register address of the senders account.

`to` : For DEBIT or CREDIT transactions, the register address of the recipient account.

`amount` : The token amount of the transaction.

`token` :  The register address of the token that the transaction relates to. Set to 0 for NXS transactions

`ticker` :  The global name assigned to the token.  
}  

-----------------------------------