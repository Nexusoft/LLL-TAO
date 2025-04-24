# Templates API

The Templates API provides methods for managing template objects, including creating, reading, updating, and transferring templates.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `template` | A template object |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`create`](#create) | Create a new template | `template` |
| [`get`](#get) | Get template data | `template` |
| [`list`](#list) | List templates | `template` |
| [`update`](#update) | Update a template | `template` |
| [`transfer`](#transfer) | Transfer a template | `template` |
| [`claim`](#claim) | Claim a transferred template | `template` |
| [`history`](#history) | Get template history | `template` |
| [`transactions`](#transactions) | Get template transactions | `template` |

## Commands

### <a name="create"></a> Create Template

Create a new template object.

```bash
templates/create/template
```

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Template Details
The following parameters are required to create a template:

- `format` (required): The format of the template. Supported values are:
  - `standard`: A standard object register
  - `readonly`: A readonly register
  - `raw`: A raw register
  - `basic`: A basic object register

#### Optional Parameters

- `data`: Optional data to be stored with the template
- `maxlength`: Maximum length of the data (default is calculated based on data size)
- `scheme`: The scheme to use for the template

#### Results

##### Return Value
```json
{
    "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
    "created": 1581389015,
    "modified": 1581389107,
    "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440"
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `address` | The register address of the template |
| `created` | The UNIX timestamp when the template was created |
| `modified` | The UNIX timestamp when the template was last modified |
| `owner` | The username hash of the template owner |

### <a name="get"></a> Get Template

Get data from a template.

```bash
templates/get/template
```

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Template Identification
One of the following parameters is required to identify the template:

- `name` (required if `address` not provided): The name of the template. The name should be in the format username:name (for local names)
- `address` (required if `name` not provided): The register address of the template

#### Results

##### Return Value
```json
{
    "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
    "created": 1581389015,
    "modified": 1581389107,
    "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
    "data": {
        "key": "value"
    }
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `address` | The register address of the template |
| `created` | The UNIX timestamp when the template was created |
| `modified` | The UNIX timestamp when the template was last modified |
| `owner` | The username hash of the template owner |
| `data` | The data stored in the template |

### <a name="list"></a> List Templates

List all templates owned by the signature chain.

```bash
templates/list/template
```

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

#### Optional Parameters

##### Filtering
The following parameters can be used to filter the results:

- `limit`: The number of records to return for the current page. The default is 100
- `offset`: The number of records to skip before starting to return results
- `order`: The sort order to apply (default is "desc")
- `column`: The column to sort by (default is "modified")

#### Results

##### Return Value
```json
[
    {
        "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
        "created": 1581389015,
        "modified": 1581389107,
        "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
        "data": {
            "key": "value"
        }
    }
]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `address` | The register address of the template |
| `created` | The UNIX timestamp when the template was created |
| `modified` | The UNIX timestamp when the template was last modified |
| `owner` | The username hash of the template owner |
| `data` | The data stored in the template | 