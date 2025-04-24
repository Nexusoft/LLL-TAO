# Assets API

The Assets API provides methods for managing digital assets in the system, including creating, transferring, and querying assets.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `asset` | A digital asset |
| `token` | A token representing an asset |
| `balance` | The balance of an asset |
| `history` | The transaction history of an asset |
| `transfer` | A transfer of an asset |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`create`](#create) | Generate a new object of supported type | `asset`, `token` |
| [`get`](#get) | Get object of supported type | `asset`, `token`, `balance`, `history` |
| [`list`](#list) | List all objects owned by given user | `asset`, `token`, `balance`, `history` |
| [`transfer`](#transfer) | Transfer an asset to another user | `asset`, `token` |
| [`burn`](#burn) | Remove an asset from circulation | `asset`, `token` |

## Direct Endpoints

The following commands are direct endpoints and thus do not support the verb and noun structure:

| Command | Description |
|:--------|:------------|
| [`get/balance`](#getbalance) | Get asset balance |
| [`get/history`](#gethistory) | Get asset history |
| [`get/transfers`](#gettransfers) | Get asset transfers |
| [`get/tokens`](#gettokens) | Get asset tokens |
| [`get/assets`](#getassets) | Get all assets |

## Deprecated Endpoints

The following endpoints are deprecated and will be removed in a future version:

- [`list/balances`](#listbalances) - Deprecated in favor of `assets/get/balance`
- [`list/history`](#listhistory) - Deprecated in favor of `assets/get/history`
- [`list/transfers`](#listtransfers) - Deprecated in favor of `assets/get/transfers`

## Commands

### <a name="create"></a> Create Asset

Create a new asset object.

```bash
assets/create/noun
```

This command only supports the asset and token nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Asset Details
The following parameters are required to create the asset:

- `name` (required): The name of the asset
- `description` (required): A description of the asset
- `supply` (required): The total supply of the asset
- `decimals` (required): The number of decimal places for the asset

#### Optional Parameters

- `data`: Optional data to be stored with the asset
- `metadata`: Optional metadata to be stored with the asset

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
| `success` | Boolean flag indicating that the asset was created successfully |
| `address` | The register address for the asset. The address is needed when retrieving or transferring the asset |
| `txid` | The ID (hash) of the transaction that includes the asset creation |

### <a name="get"></a> Get Asset

Retrieves information about an asset.

```bash
assets/get/noun
```

This command supports the asset, token, balance, and history nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Lookup
One of the following parameters is required to identify the asset:

- `name` (required if `address` not provided): The name of the asset. The name should be in the format username:name (for local names)
- `address` (required if `name` not provided): The register address of the asset

#### Results

##### Return Value
```json
{
    "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
    "created": 1581389015,
    "modified": 1581389107,
    "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
    "name": "My Asset",
    "description": "A description of my asset",
    "supply": 1000000,
    "decimals": 8,
    "data": {
        "key": "value"
    },
    "metadata": {
        "key": "value"
    }
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `address` | The register address of the asset |
| `created` | The UNIX timestamp when the asset was created |
| `modified` | The UNIX timestamp when the asset was last modified |
| `owner` | The username hash of the owners profile |
| `name` | The name of the asset |
| `description` | A description of the asset |
| `supply` | The total supply of the asset |
| `decimals` | The number of decimal places for the asset |
| `data` | Optional data stored with the asset |
| `metadata` | Optional metadata stored with the asset |

### <a name="list"></a> List Assets

This will list all assets owned by the signature chain.

```bash
assets/list/noun
```

This command supports the asset, token, balance, and history nouns.

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
        "name": "My Asset",
        "description": "A description of my asset",
        "supply": 1000000,
        "decimals": 8,
        "data": {
            "key": "value"
        },
        "metadata": {
            "key": "value"
        }
    }
]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `address` | The register address of the asset |
| `created` | The UNIX timestamp when the asset was created |
| `modified` | The UNIX timestamp when the asset was last modified |
| `owner` | The username hash of the owners profile |
| `name` | The name of the asset |
| `description` | A description of the asset |
| `supply` | The total supply of the asset |
| `decimals` | The number of decimal places for the asset |
| `data` | Optional data stored with the asset |
| `metadata` | Optional metadata stored with the asset |

### <a name="transfer"></a> Transfer Asset

Transfer an asset to another user.

```bash
assets/transfer/noun
```

This command supports the asset and token nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Transfer Details
The following parameters are required to transfer the asset:

- `from` (required): The account to transfer from. This can be the name or the register address. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace)
- `to` (required): The account to transfer to. This can be the name or the register address. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace)
- `amount` (required): The amount of the asset to transfer

#### Optional Parameters

- `reference`: A 64-bit unsigned integer in the range of 0 to 18446744073709551615 that the recipient can use to relate the transaction to an order number, invoice number etc
- `expires`: The number of seconds from the transaction creation time after which the transaction can no longer be credited by the recipient. If set to 0, the transaction will never expire. If omitted, a default expiration of 7 days (604800 seconds) is applied

#### Results

##### Return Value
```json
{
    "success": true,
    "txid": "01f51d6b23b871fc8da848afa57cf066cb9e3b8fb845a666335e8c678ef5249e98d4f3e477659098918e4bb590472a63d0ed0a17fa87904fcff6316158e9edfd"
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `success` | Boolean flag indicating that the transfer was successful |
| `txid` | The ID (hash) of the transaction that includes the transfer |

### <a name="burn"></a> Burn Asset

This method can be used to take assets permanently out of the current supply, a process commonly known as burning. The method will debit an asset account and send the assets back to the asset address. However the transaction contains a condition that will always evaluate to false, guaranteeing that the debit can not be credited by the asset issuer nor the sender. The result is that the amount burned will always appear in the "pending" balance of the asset.

```bash
assets/burn/noun
```

This command only supports the asset and token nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN to authorize the transaction
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Burn Details
The following parameters are required to burn the asset:

- `name` (required if `address` not provided): The name of account to debit the assets from to be burnt. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace)
- `address` (required if `name` not provided): The register address of the account to debit the assets from to be burnt
- `amount` (required): The amount of assets to burn

#### Optional Parameters

- `reference`: A 64-bit unsigned integer in the range of 0 to 18446744073709551615 that the recipient can use to relate the transaction to an order number, invoice number etc

#### Results

##### Return Value
```json
{
    "success": true,
    "address": "8BfLgEprhbHs82LxUkJR9jhQufRZf49g73Nt8XTGevfiyy7ijhb",
    "txid": "01854fe4fdf0d59aebb3a880141484f0542af061cbebfd468db3fcecd13f63a986d990cf669ca4a60822a82b2d4fc7e7e474801a01bff86a35fd0a147a5"
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `success` | Boolean flag indicating that the burn was successful |
| `address` | The register address for this account |
| `txid` | The ID (hash) of the transaction that includes the burn |