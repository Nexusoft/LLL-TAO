# Market API

The Market API provides methods for managing market operations in the system, including creating orders, executing trades, and querying market data.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `order` | A market order |
| `trade` | A market trade |
| `book` | The order book |
| `ticker` | Market ticker information |
| `history` | Market history information |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`create`](#create) | Generate a new object of supported type | `order` |
| [`get`](#get) | Get object of supported type | `any`, `order`, `trade`, `book`, `ticker`, `history` |
| [`list`](#list) | List all objects owned by given user | `any`, `order`, `trade`, `book`, `ticker`, `history` |
| [`cancel`](#cancel) | Cancel an order | `order` |
| [`execute`](#execute) | Execute a trade | `trade` |

## Direct Endpoints

The following commands are direct endpoints and thus do not support the verb and noun structure:

| Command | Description |
|:--------|:------------|
| [`get/order`](#getorder) | Get order information |
| [`get/trade`](#gettrade) | Get trade information |
| [`get/book`](#getbook) | Get order book information |
| [`get/ticker`](#getticker) | Get ticker information |
| [`get/history`](#gethistory) | Get market history information |
| [`create/order`](#createorder) | Create a new order |
| [`cancel/order`](#cancelorder) | Cancel an order |
| [`execute/trade`](#executetrade) | Execute a trade |

## Deprecated Endpoints

The following endpoints are deprecated and will be removed in a future version:

- [`list/order`](#listorder) - Deprecated in favor of `market/get/order`
- [`list/trade`](#listtrade) - Deprecated in favor of `market/get/trade`
- [`list/book`](#listbook) - Deprecated in favor of `market/get/book`
- [`list/ticker`](#listticker) - Deprecated in favor of `market/get/ticker`
- [`list/history`](#listhistory) - Deprecated in favor of `market/get/history`

## Commands

### <a name="create"></a> Create Order

Create a new market order.

```bash
market/create/noun
```

This command only supports the order noun.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Order Details
The following parameters are required to create the order:

- `type` (required): The type of order (buy or sell)
- `amount` (required): The amount of the order
- `price` (required): The price of the order
- `token` (required): The token to trade

#### Optional Parameters

- `expires`: The number of seconds from the order creation time after which the order will expire. If set to 0, the order will never expire. If omitted, a default expiration of 7 days (604800 seconds) is applied
- `reference`: A 64-bit unsigned integer in the range of 0 to 18446744073709551615 that can be used to relate the order to an external reference

#### Results

##### Return Value
```json
{
    "success": true,
    "address": "8ESvYizqdApiuKEBjZMF1hnB8asDqECaDwAstcH3UtJ4Z6ceCn2",
    "txid": "0131e17af8029b414814283a3d90813d12c238db6ddab213440249b795090a9cd77079d5804ec38303a59414d87108d4e44bf31f54a6c176285281a88ab5d737"
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `success` | Boolean flag indicating that the order was created successfully |
| `address` | The register address for the order. The address is needed when retrieving or canceling the order |
| `txid` | The ID (hash) of the transaction that includes the order creation |

### <a name="get"></a> Get Market Information

Retrieves market information based on the specified noun.

```bash
market/get/noun
```

This command supports the any, order, trade, book, ticker, and history nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

#### Optional Parameters

##### Filtering
The following parameters can be used to filter the results:

- `limit`: The number of records to return for the current page. The default is 100
- `page`: Allows the results to be returned by page (zero based). E.g. passing in page=1 will return the second set of (limit) records. The default value is 0 if not supplied
- `offset`: An alternative to page, offset can be used to return a set of (limit) results from a particular index
- `where`: An array of clauses to filter the JSON results. More information on filtering the results from /list/xxx API methods can be found here Filtering Results

#### Results

##### Return Value
```json
{
    "orders": [
        {
            "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
            "created": 1581389015,
            "modified": 1581389107,
            "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
            "type": "buy",
            "amount": 100,
            "price": 1.5,
            "token": "NXS",
            "status": "open"
        }
    ],
    "trades": [
        {
            "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
            "created": 1581389015,
            "modified": 1581389107,
            "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
            "type": "buy",
            "amount": 100,
            "price": 1.5,
            "token": "NXS",
            "status": "completed"
        }
    ],
    "book": {
        "bids": [
            {
                "price": 1.5,
                "amount": 100
            }
        ],
        "asks": [
            {
                "price": 1.6,
                "amount": 100
            }
        ]
    },
    "ticker": {
        "last": 1.5,
        "high": 1.6,
        "low": 1.4,
        "volume": 1000
    }
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `orders` | Array of market orders |
| `orders[].address` | The register address of the order |
| `orders[].created` | The UNIX timestamp when the order was created |
| `orders[].modified` | The UNIX timestamp when the order was last modified |
| `orders[].owner` | The username hash of the owners profile |
| `orders[].type` | The type of order (buy or sell) |
| `orders[].amount` | The amount of the order |
| `orders[].price` | The price of the order |
| `orders[].token` | The token to trade |
| `orders[].status` | The status of the order (open, filled, canceled, or expired) |
| `trades` | Array of market trades |
| `trades[].address` | The register address of the trade |
| `trades[].created` | The UNIX timestamp when the trade was created |
| `trades[].modified` | The UNIX timestamp when the trade was last modified |
| `trades[].owner` | The username hash of the owners profile |
| `trades[].type` | The type of trade (buy or sell) |
| `trades[].amount` | The amount of the trade |
| `trades[].price` | The price of the trade |
| `trades[].token` | The token traded |
| `trades[].status` | The status of the trade (completed or failed) |
| `book` | The order book |
| `book.bids` | Array of bid orders |
| `book.bids[].price` | The price of the bid |
| `book.bids[].amount` | The amount of the bid |
| `book.asks` | Array of ask orders |
| `book.asks[].price` | The price of the ask |
| `book.asks[].amount` | The amount of the ask |
| `ticker` | Market ticker information |
| `ticker.last` | The last trade price |
| `ticker.high` | The highest trade price |
| `ticker.low` | The lowest trade price |
| `ticker.volume` | The total trading volume |

### <a name="list"></a> List Market Information

Lists market information based on the specified noun.

```bash
market/list/noun
```

This command supports the any, order, trade, book, ticker, and history nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

#### Optional Parameters

##### Identification
One of the following parameters can be used to identify the signature chain:

- `genesis`: The genesis hash identifying the signature chain (optional if username is supplied)
- `username`: The username identifying the signature chain (optional if genesis is supplied)

##### Filtering
The following parameters can be used to filter the results:

- `limit`: The number of records to return for the current page. The default is 100
- `page`: Allows the results to be returned by page (zero based). E.g. passing in page=1 will return the second set of (limit) records. The default value is 0 if not supplied
- `offset`: An alternative to page, offset can be used to return a set of (limit) results from a particular index
- `where`: An array of clauses to filter the JSON results. More information on filtering the results from /list/xxx API methods can be found here Filtering Results

#### Results

##### Return Value
```json
[
    {
        "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
        "created": 1581389015,
        "modified": 1581389107,
        "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
        "type": "buy",
        "amount": 100,
        "price": 1.5,
        "token": "NXS",
        "status": "open"
    }
]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `address` | The register address of the order |
| `created` | The UNIX timestamp when the order was created |
| `modified` | The UNIX timestamp when the order was last modified |
| `owner` | The username hash of the owners profile |
| `type` | The type of order (buy or sell) |
| `amount` | The amount of the order |
| `price` | The price of the order |
| `token` | The token to trade |
| `status` | The status of the order (open, filled, canceled, or expired) |

### <a name="cancel"></a> Cancel Order

Cancel a market order.

```bash
market/cancel/noun
```

This command only supports the order noun.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Cancel Details
The following parameters are required to cancel the order:

- `name` (required if `address` not provided): The name of the order to cancel. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace)
- `address` (required if `name` not provided): The register address of the order to cancel

#### Results

##### Return Value
```json
{
    "success": true,
    "txid": "01f51d6b23b871fc8da848afa57cf066cb9e3b8fb845a666335e8c678ef5249e98d4f3e477659098918e4bb590472a63d0ed0a17fa87904fcff6316158e9edfd"
}
[Completed in 4979.735275 ms]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `success` | Boolean flag indicating that the cancel was successful |
| `txid` | The ID (hash) of the transaction that includes the cancel |

### <a name="execute"></a> Execute Trade

Execute a market trade.

```bash
market/execute/noun
```

This command only supports the trade noun.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Trade Details
The following parameters are required to execute the trade:

- `order` (required): The order to execute the trade against. This can be the name or the register address of the order
- `amount` (required): The amount to trade

#### Results

##### Return Value
```json
{
    "success": true,
    "txid": "01f51d6b23b871fc8da848afa57cf066cb9e3b8fb845a666335e8c678ef5249e98d4f3e477659098918e4bb590472a63d0ed0a17fa87904fcff6316158e9edfd"
}
[Completed in 4979.735275 ms]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `success` | Boolean flag indicating that the trade was executed successfully |
| `txid` | The ID (hash) of the transaction that includes the trade execution |