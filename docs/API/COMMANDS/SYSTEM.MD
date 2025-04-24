# System API

The System API provides methods for managing system-level operations, including configuration, status, and maintenance tasks.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `config` | System configuration settings |
| `status` | System status information |
| `maintenance` | System maintenance operations |
| `log` | System log information |
| `backup` | System backup operations |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`get`](#get) | Get system information | `config`, `status`, `log` |
| [`set`](#set) | Set system configuration | `config` |
| [`list`](#list) | List system information | `log`, `backup` |
| [`backup`](#backup) | Perform system backup | `backup` |
| [`restore`](#restore) | Restore system from backup | `backup` |

## Direct Endpoints

The following commands are direct endpoints and thus do not support the verb and noun structure:

| Command | Description |
|:--------|:------------|
| [`get/version`](#getversion) | Get system version |
| [`get/status`](#getstatus) | Get system status |
| [`get/config`](#getconfig) | Get system configuration |
| [`set/config`](#setconfig) | Set system configuration |
| [`get/logs`](#getlogs) | Get system logs |
| [`backup/create`](#backupcreate) | Create system backup |
| [`backup/restore`](#backuprestore) | Restore system from backup |

## Deprecated Endpoints

The following endpoints are deprecated and will be removed in a future version:

- [`list/config`](#listconfig) - Deprecated in favor of `system/get/config`
- [`list/status`](#liststatus) - Deprecated in favor of `system/get/status`
- [`list/logs`](#listlogs) - Deprecated in favor of `system/get/logs`

## Commands

### <a name="get"></a> Get System Information

Retrieves system information based on the specified noun.

```bash
system/get/noun
```

This command supports the config, status, and log nouns.

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
    "version": "1.0.0",
    "status": "running",
    "config": {
        "setting1": "value1",
        "setting2": "value2"
    },
    "logs": [
        {
            "timestamp": 1581389015,
            "level": "INFO",
            "message": "System started"
        }
    ]
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `version` | The system version |
| `status` | The current system status |
| `config` | The system configuration settings |
| `logs` | Array of system log entries |
| `logs[].timestamp` | The UNIX timestamp when the log entry was created |
| `logs[].level` | The log level (INFO, WARNING, ERROR, etc.) |
| `logs[].message` | The log message |

### <a name="set"></a> Set System Configuration

Sets system configuration settings.

```bash
system/set/noun
```

This command only supports the config noun.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Configuration
The following parameters are required to set the configuration:

- `setting` (required): The name of the setting to set
- `value` (required): The value to set for the setting

#### Results

##### Return Value
```json
{
    "success": true,
    "setting": "setting1",
    "value": "value1"
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `success` | Boolean flag indicating that the setting was set successfully |
| `setting` | The name of the setting that was set |
| `value` | The value that was set for the setting |

### <a name="list"></a> List System Information

Lists system information based on the specified noun.

```bash
system/list/noun
```

This command supports the log and backup nouns.

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
[
    {
        "timestamp": 1581389015,
        "level": "INFO",
        "message": "System started"
    }
]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `timestamp` | The UNIX timestamp when the log entry was created |
| `level` | The log level (INFO, WARNING, ERROR, etc.) |
| `message` | The log message |

### <a name="backup"></a> Backup System

Performs a system backup.

```bash
system/backup/noun
```

This command only supports the backup noun.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Backup Details
The following parameters are required to perform the backup:

- `path` (required): The path where the backup should be stored
- `name` (required): The name of the backup

#### Results

##### Return Value
```json
{
    "success": true,
    "path": "/path/to/backup",
    "name": "backup1"
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `success` | Boolean flag indicating that the backup was successful |
| `path` | The path where the backup was stored |
| `name` | The name of the backup |

### <a name="restore"></a> Restore System

Restores the system from a backup.

```bash
system/restore/noun
```

This command only supports the backup noun.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Restore Details
The following parameters are required to restore from a backup:

- `path` (required): The path where the backup is stored
- `name` (required): The name of the backup to restore from

#### Results

##### Return Value
```json
{
    "success": true,
    "path": "/path/to/backup",
    "name": "backup1"
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `success` | Boolean flag indicating that the restore was successful |
| `path` | The path where the backup was stored |
| `name` | The name of the backup that was restored from |