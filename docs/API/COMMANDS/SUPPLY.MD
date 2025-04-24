# Supply API

The Supply API provides methods for managing and querying the supply of tokens in the system, including total supply, circulating supply, and supply metrics.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `supply` | Total supply information |
| `circulating` | Circulating supply information |
| `metrics` | Supply metrics information |
| `history` | Supply history information |
| `projection` | Supply projection information |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`get`](#get) | Get supply information | `supply`, `circulating`, `metrics` |
| [`list`](#list) | List supply information | `history`, `projection` |
| [`calculate`](#calculate) | Calculate supply metrics | `metrics` |
| [`project`](#project) | Project future supply | `projection` |

## Direct Endpoints

The following commands are direct endpoints and thus do not support the verb and noun structure:

| Command | Description |
|:--------|:------------|
| [`get/total`](#gettotal) | Get total supply |
| [`get/circulating`](#getcirculating) | Get circulating supply |
| [`get/metrics`](#getmetrics) | Get supply metrics |
| [`get/history`](#gethistory) | Get supply history |
| [`get/projection`](#getprojection) | Get supply projection |

## Deprecated Endpoints

The following endpoints are deprecated and will be removed in a future version:

- [`list/supply`](#listsupply) - Deprecated in favor of `supply/get/total`
- [`list/circulating`](#listcirculating) - Deprecated in favor of `supply/get/circulating`
- [`list/metrics`](#listmetrics) - Deprecated in favor of `supply/get/metrics`

## Commands

### <a name="get"></a> Get Supply Information

Retrieves supply information based on the specified noun.

```bash
supply/get/noun
```

This command supports the supply, circulating, and metrics nouns.

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
    "total": 1000000,
    "circulating": 750000,
    "locked": 250000,
    "metrics": {
        "inflation_rate": 0.05,
        "growth_rate": 0.02,
        "velocity": 1.5
    }
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `total` | The total supply of the token |
| `circulating` | The circulating supply of the token |
| `locked` | The amount of tokens locked in contracts or staking |
| `metrics` | Supply metrics information |
| `metrics.inflation_rate` | The current inflation rate |
| `metrics.growth_rate` | The current growth rate |
| `metrics.velocity` | The current velocity of the token |

### <a name="list"></a> List Supply Information

Lists supply information based on the specified noun.

```bash
supply/list/noun
```

This command supports the history and projection nouns.

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
        "total": 1000000,
        "circulating": 750000,
        "locked": 250000,
        "metrics": {
            "inflation_rate": 0.05,
            "growth_rate": 0.02,
            "velocity": 1.5
        }
    }
]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `timestamp` | The UNIX timestamp when the supply information was recorded |
| `total` | The total supply of the token at that time |
| `circulating` | The circulating supply of the token at that time |
| `locked` | The amount of tokens locked in contracts or staking at that time |
| `metrics` | Supply metrics information at that time |
| `metrics.inflation_rate` | The inflation rate at that time |
| `metrics.growth_rate` | The growth rate at that time |
| `metrics.velocity` | The velocity of the token at that time |

### <a name="calculate"></a> Calculate Supply Metrics

Calculates supply metrics based on the specified parameters.

```bash
supply/calculate/noun
```

This command only supports the metrics noun.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Calculation Parameters
The following parameters are required to calculate the metrics:

- `start_date` (required): The start date for the calculation period
- `end_date` (required): The end date for the calculation period

#### Results

##### Return Value
```json
{
    "inflation_rate": 0.05,
    "growth_rate": 0.02,
    "velocity": 1.5
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `inflation_rate` | The calculated inflation rate for the period |
| `growth_rate` | The calculated growth rate for the period |
| `velocity` | The calculated velocity for the period |

### <a name="project"></a> Project Future Supply

Projects future supply based on the specified parameters.

```bash
supply/project/noun
```

This command only supports the projection noun.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Projection Parameters
The following parameters are required to project the supply:

- `start_date` (required): The start date for the projection period
- `end_date` (required): The end date for the projection period
- `growth_rate` (required): The expected growth rate for the period

#### Results

##### Return Value
```json
{
    "projections": [
        {
            "date": "2023-01-01",
            "total": 1050000,
            "circulating": 787500,
            "locked": 262500
        }
    ]
}
```

##### Return Values

| Field | Description |
|:------|:------------|
| `projections` | Array of supply projections |
| `projections[].date` | The date of the projection |
| `projections[].total` | The projected total supply |
| `projections[].circulating` | The projected circulating supply |
| `projections[].locked` | The projected locked supply |