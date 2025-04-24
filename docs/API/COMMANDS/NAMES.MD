# Names API

The Names API provides methods for managing names in the system, including creating, transferring, and querying names.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `name` | A standard name |
| `global` | A global name |
| `local` | A local name |
| `namespace` | A namespace |
| `any` | Any type of name |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`create`](#create) | Generate a new object of supported type | `name`, `global`, `local`, `namespace` |
| [`get`](#get) | Get object of supported type | `any`, `name`, `global`, `local`, `namespace` |
| [`list`](#list) | List all objects owned by given user | `any`, `name`, `global`, `local`, `namespace` |
| [`transfer`](#transfer) | Transfer a name to another user | `any`, `name`, `global`, `local`, `namespace` |
| [`claim`](#claim) | Claim ownership of a name | `any`, `name`, `global`, `local`, `namespace` |

## Direct Endpoints

The following commands are direct endpoints and thus do not support the verb and noun structure:

| Command | Description |
|:--------|:------------|
| [`get/name`](#getname) | Get name information |
| [`get/namespace`](#getnamespace) | Get namespace information |
| [`list/names`](#listnames) | List names |
| [`list/namespaces`](#listnamespaces) | List namespaces |
| [`transfer/name`](#transfername) | Transfer a name |
| [`claim/name`](#claimname) | Claim a name |

## Deprecated Endpoints

The following endpoints are deprecated and will be removed in a future version:

- [`list/name`](#listname) - Deprecated in favor of `names/get/name`
- [`list/namespace`](#listnamespace) - Deprecated in favor of `names/get/namespace`
- [`list/names`](#listnames) - Deprecated in favor of `names/list/names`
- [`list/namespaces`](#listnamespaces) - Deprecated in favor of `names/list/namespaces`

## Commands

### <a name="create"></a> Create Name

Create a new name object.

```bash
names/create/noun
```

This command supports the name, global, local, and namespace nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Name Details
The following parameters are required to create the name:

- `name` (required): The name to create
- `type` (required): The type of name to create (global, local, or namespace)

#### Optional Parameters

- `data`: Optional data to be stored with the name
- `metadata`: Optional metadata to be stored with the name

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
| `success` | Boolean flag indicating that the name was created successfully |
| `address` | The register address for the name. The address is needed when retrieving or transferring the name |
| `txid` | The ID (hash) of the transaction that includes the name creation |

### <a name="get"></a> Get Name

Retrieves information about a name.

```bash
names/get/noun
```

This command supports the any, name, global, local, and namespace nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Lookup
One of the following parameters is required to identify the name:

- `name` (required if `address` not provided): The name to retrieve information for. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace)
- `address` (required if `name` not provided): The register address of the name

#### Results

##### Return Value
```json
{
    "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
    "created": 1581389015,
    "modified": 1581389107,
    "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
    "name": "My Name",
    "type": "local",
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
| `address` | The register address of the name |
| `created` | The UNIX timestamp when the name was created |
| `modified` | The UNIX timestamp when the name was last modified |
| `owner` | The username hash of the owners profile |
| `name` | The name |
| `type` | The type of name (global, local, or namespace) |
| `data` | Optional data stored with the name |
| `metadata` | Optional metadata stored with the name |

### <a name="list"></a> List Names

This will list all names owned by the signature chain.

```bash
names/list/noun
```

This command supports the any, name, global, local, and namespace nouns.

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
        "name": "My Name",
        "type": "local",
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
| `address` | The register address of the name |
| `created` | The UNIX timestamp when the name was created |
| `modified` | The UNIX timestamp when the name was last modified |
| `owner` | The username hash of the owners profile |
| `name` | The name |
| `type` | The type of name (global, local, or namespace) |
| `data` | Optional data stored with the name |
| `metadata` | Optional metadata stored with the name |

### <a name="transfer"></a> Transfer Name

Transfer a name to another user.

```bash
names/transfer/noun
```

This command supports the any, name, global, local, and namespace nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Transfer Details
The following parameters are required to transfer the name:

- `name` (required): The name to transfer. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace)
- `recipient` (required): The recipient of the name. This can be either:
  - The username identifying the user account (sig-chain)
  - The genesis hash of the signature chain to issue the name to

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
| `success` | Boolean flag indicating that the transfer was successful |
| `txid` | The ID (hash) of the transaction that includes the transfer |

### <a name="claim"></a> Claim Name

Claim ownership of a name that has been transferred to you.

```bash
names/claim/noun
```

This command supports the any, name, global, local, and namespace nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Claim Details
The following parameter is required to claim the name:

- `txid` (required): The hash in hexadecimal encoding of the transaction that we are claiming

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
| `success` | Boolean flag indicating that the claim was successful |
| `txid` | The ID (hash) of the transaction that includes the claim |