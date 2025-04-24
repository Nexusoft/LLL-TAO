# Local API

The Local API provides methods for managing local database records, including creating, reading, updating, and deleting records.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `record` | A record in the local database |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`push`](#push) | Create or update a record | `record` |
| [`list`](#list) | List records from a table | `record` |
| [`has`](#has) | Check if a record exists | `record` |
| [`erase`](#erase) | Delete a record | `record` |

## Commands

### <a name="push"></a> Push Record

Create or update a record in the local database.

```bash
local/push/record
```

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Record Details
The following parameters are required to create or update a record:

- `table` (required): The name of the table to store the record in
- `key` (required): The key for the record (can be string or number)
- `value` (required): The value to store for the record

#### Results

##### Return Value
```json
{
    "success": true
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `success` | Boolean flag indicating that the record was created or updated successfully |

### <a name="list"></a> List Records

List records from a table in the local database.

```bash
local/list/record
```

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Table Details
The following parameters are required to list records:

- `table` (required): The name of the table to list records from

#### Optional Parameters

##### Filtering
The following parameters can be used to filter the results:

- `limit`: The number of records to return for the current page. The default is 100
- `offset`: The number of records to skip before starting to return results
- `order`: The sort order to apply (default is "desc")

#### Results

##### Return Value
```json
{
    "key1": "value1",
    "key2": "value2"
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `key` | The key of the record |
| `value` | The value stored for the record |

### <a name="has"></a> Check Record

Check if a record exists in the local database.

```bash
local/has/record
```

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Record Details
The following parameters are required to check a record:

- `table` (required): The name of the table to check
- `key` (required): The key to check for (can be string or number)

#### Results

##### Return Value
```json
{
    "exists": true
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `exists` | Boolean flag indicating whether the record exists |

### <a name="erase"></a> Erase Record

Delete a record from the local database.

```bash
local/erase/record
```

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Record Details
The following parameters are required to delete a record:

- `table` (required): The name of the table to delete from
- `key` (required): The key of the record to delete (can be string or number)

#### Results

##### Return Value
```json
{
    "success": true
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `success` | Boolean flag indicating that the record was deleted successfully | 