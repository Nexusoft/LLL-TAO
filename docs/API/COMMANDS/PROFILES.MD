# Profiles API

The Profiles API provides methods for managing user profiles in the system, including creating, updating, and querying profiles.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `profile` | A user profile |
| `account` | A user account |
| `settings` | User settings |
| `preferences` | User preferences |
| `any` | Any type of profile object |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`create`](#create) | Generate a new object of supported type | `profile`, `account` |
| [`get`](#get) | Get object of supported type | `any`, `profile`, `account`, `settings`, `preferences` |
| [`list`](#list) | List all objects owned by given user | `any`, `profile`, `account`, `settings`, `preferences` |
| [`update`](#update) | Update an object | `any`, `profile`, `account`, `settings`, `preferences` |
| [`delete`](#delete) | Delete an object | `any`, `profile`, `account`, `settings`, `preferences` |

## Direct Endpoints

The following commands are direct endpoints and thus do not support the verb and noun structure:

| Command | Description |
|:--------|:------------|
| [`get/profile`](#getprofile) | Get profile information |
| [`get/account`](#getaccount) | Get account information |
| [`get/settings`](#getsettings) | Get user settings |
| [`get/preferences`](#getpreferences) | Get user preferences |
| [`update/profile`](#updateprofile) | Update profile information |
| [`update/account`](#updateaccount) | Update account information |
| [`update/settings`](#updatesettings) | Update user settings |
| [`update/preferences`](#updatepreferences) | Update user preferences |

## Deprecated Endpoints

The following endpoints are deprecated and will be removed in a future version:

- [`list/profile`](#listprofile) - Deprecated in favor of `profiles/get/profile`
- [`list/account`](#listaccount) - Deprecated in favor of `profiles/get/account`
- [`list/settings`](#listsettings) - Deprecated in favor of `profiles/get/settings`
- [`list/preferences`](#listpreferences) - Deprecated in favor of `profiles/get/preferences`

## Commands

### <a name="create"></a> Create Profile

Create a new profile object.

```bash
profiles/create/noun
```

This command supports the profile and account nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Profile Details
The following parameters are required to create the profile:

- `username` (required): The username for the profile
- `password` (required): The password for the profile

#### Optional Parameters

- `data`: Optional data to be stored with the profile
- `metadata`: Optional metadata to be stored with the profile

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
| `success` | Boolean flag indicating that the profile was created successfully |
| `address` | The register address for the profile. The address is needed when retrieving or updating the profile |
| `txid` | The ID (hash) of the transaction that includes the profile creation |

### <a name="get"></a> Get Profile

Retrieves information about a profile.

```bash
profiles/get/noun
```

This command supports the any, profile, account, settings, and preferences nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Lookup
One of the following parameters is required to identify the profile:

- `username` (required if `address` not provided): The username of the profile
- `address` (required if `username` not provided): The register address of the profile

#### Results

##### Return Value
```json
{
    "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
    "created": 1581389015,
    "modified": 1581389107,
    "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
    "username": "myusername",
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
| `address` | The register address of the profile |
| `created` | The UNIX timestamp when the profile was created |
| `modified` | The UNIX timestamp when the profile was last modified |
| `owner` | The username hash of the owners profile |
| `username` | The username of the profile |
| `data` | Optional data stored with the profile |
| `metadata` | Optional metadata stored with the profile |

### <a name="list"></a> List Profiles

This will list all profiles owned by the signature chain.

```bash
profiles/list/noun
```

This command supports the any, profile, account, settings, and preferences nouns.

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
        "username": "myusername",
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
| `address` | The register address of the profile |
| `created` | The UNIX timestamp when the profile was created |
| `modified` | The UNIX timestamp when the profile was last modified |
| `owner` | The username hash of the owners profile |
| `username` | The username of the profile |
| `data` | Optional data stored with the profile |
| `metadata` | Optional metadata stored with the profile |

### <a name="update"></a> Update Profile

Update a profile object.

```bash
profiles/update/noun
```

This command supports the any, profile, account, settings, and preferences nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Update Details
The following parameters are required to update the profile:

- `username` (required if `address` not provided): The username of the profile to update
- `address` (required if `username` not provided): The register address of the profile to update
- `data` (required): The new data to update the profile with

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

### <a name="delete"></a> Delete Profile

Delete a profile object.

```bash
profiles/delete/noun
```

This command supports the any, profile, account, settings, and preferences nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Delete Details
The following parameters are required to delete the profile:

- `username` (required if `address` not provided): The username of the profile to delete
- `address` (required if `username` not provided): The register address of the profile to delete

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