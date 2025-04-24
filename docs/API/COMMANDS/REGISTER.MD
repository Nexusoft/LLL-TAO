# Register API

The Register API provides methods for managing registers in the system, including creating, updating, and querying registers.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `register` | A standard register |
| `object` | An object register |
| `state` | A state register |
| `any` | Any type of register |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`create`](#create) | Generate a new object of supported type | `register`, `object`, `state` |
| [`get`](#get) | Get object of supported type | `any`, `register`, `object`, `state` |
| [`list`](#list) | List all objects owned by given user | `any`, `register`, `object`, `state` |
| [`update`](#update) | Update an object | `any`, `register`, `object`, `state` |
| [`delete`](#delete) | Delete an object | `any`, `register`, `object`, `state` |

## Direct Endpoints

The following commands are direct endpoints and thus do not support the verb and noun structure:

| Command | Description |
|:--------|:------------|
| [`get/register`](#getregister) | Get register information |
| [`get/object`](#getobject) | Get object register information |
| [`get/state`](#getstate) | Get state register information |
| [`list/registers`](#listregisters) | List registers |
| [`list/objects`](#listobjects) | List object registers |
| [`list/states`](#liststates) | List state registers |

## Deprecated Endpoints

The following endpoints are deprecated and will be removed in a future version:

- [`list/register`](#listregister) - Deprecated in favor of `register/get/register`
- [`list/object`](#listobject) - Deprecated in favor of `register/get/object`
- [`list/state`](#liststate) - Deprecated in favor of `register/get/state`

## Commands

### <a name="create"></a> Create Register

Create a new register object.

```bash
register/create/noun
```

This command supports the register, object, and state nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Register Details
The following parameters are required to create the register:

- `type` (required): The type of register to create (register, object, or state)
- `data` (required): The data to store in the register

#### Optional Parameters

- `name`: Optional name for the register
- `metadata`: Optional metadata to be stored with the register

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
| `success` | Boolean flag indicating that the register was created successfully |
| `address` | The register address for the register. The address is needed when retrieving or updating the register |
| `txid` | The ID (hash) of the transaction that includes the register creation |

### <a name="get"></a> Get Register

Retrieves information about a register.

```bash
register/get/noun
```

This command supports the any, register, object, and state nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Lookup
One of the following parameters is required to identify the register:

- `name` (required if `address` not provided): The name of the register. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace)
- `address` (required if `name` not provided): The register address of the register

#### Results

##### Return Value
```json
{
    "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
    "created": 1581389015,
    "modified": 1581389107,
    "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
    "type": "object",
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
| `address` | The register address of the register |
| `created` | The UNIX timestamp when the register was created |
| `modified` | The UNIX timestamp when the register was last modified |
| `owner` | The username hash of the owners profile |
| `type` | The type of register (register, object, or state) |
| `data` | The data stored in the register |
| `metadata` | Optional metadata stored with the register |

### <a name="list"></a> List Registers

This will list all registers owned by the signature chain.

```bash
register/list/noun
```

This command supports the any, register, object, and state nouns.

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
        "type": "object",
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
| `address` | The register address of the register |
| `created` | The UNIX timestamp when the register was created |
| `modified` | The UNIX timestamp when the register was last modified |
| `owner` | The username hash of the owners profile |
| `type` | The type of register (register, object, or state) |
| `data` | The data stored in the register |
| `metadata` | Optional metadata stored with the register |

### <a name="update"></a> Update Register

Update a register object.

```bash
register/update/noun
```

This command supports the any, register, object, and state nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Update Details
The following parameters are required to update the register:

- `name` (required if `address` not provided): The name of the register to update. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace)
- `address` (required if `name` not provided): The register address of the register to update
- `data` (required): The new data to update the register with

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
| `success` | Boolean flag indicating that the update was successful |
| `txid` | The ID (hash) of the transaction that includes the update |

### <a name="delete"></a> Delete Register

Delete a register object.

```bash
register/delete/noun
```

This command supports the any, register, object, and state nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Delete Details
The following parameters are required to delete the register:

- `name` (required if `address` not provided): The name of the register to delete. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace)
- `address` (required if `name` not provided): The register address of the register to delete

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
| `success` | Boolean flag indicating that the delete was successful |
| `txid` | The ID (hash) of the transaction that includes the delete |
