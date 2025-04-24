# Tokens API

The Tokens API provides methods for managing tokens and token accounts. Note that many of the token operations have been moved to the Finance API in version 6.0.0.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `token` | A token object |
| `account` | A token account |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`list`](#list) | List token accounts | `token` |

## Deprecated Endpoints

The following endpoints are deprecated and will be removed in a future version:

- `tokens/burn` - Deprecated in favor of `finance/burn`
- `tokens/create` - Deprecated in favor of `finance/create`
- `tokens/credit` - Deprecated in favor of `finance/credit`
- `tokens/debit` - Deprecated in favor of `finance/debit`
- `tokens/get` - Deprecated in favor of `finance/get`
- `tokens/list` - Deprecated in favor of `finance/list`
- `tokens/list/token/transactions` - Deprecated in favor of `finance/transactions/token`
- `tokens/list/account/transactions` - Deprecated in favor of `finance/transactions/account`

## Commands

### <a name="list"></a> List Token Accounts

List all accounts that have been created for a particular token.

```bash
tokens/list/token/accounts
```

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Token Identification
One of the following parameters is required to identify the token:

- `name` (required if `address` not provided): The name of the token. The name should be in the format username:name (for local names)
- `address` (required if `name` not provided): The register address of the token

#### Optional Parameters

##### Filtering
The following parameters can be used to filter the results:

- `limit`: The number of records to return for the current page. The default is 100
- `offset`: The number of records to skip before starting to return results
- `order`: The sort order to apply (default is "desc")
- `sort`: The field to sort by (default is "trust"). Supported values are:
  - `balance`: Sort by account balance
  - `created`: Sort by creation timestamp
  - `modified`: Sort by last modification timestamp
  - `trust`: Sort by trust score

#### Results

##### Return Value
```json
[
    {
        "address": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97",
        "balance": 1000000,
        "created": 1581389015,
        "modified": 1581389107,
        "owner": "a2056d518d6e6d65c6c2e05af7fe2d3182a93def20e960fcfa0d35777a082440",
        "token": "822G7ZSsHh1ncTj4AJt6FnZ6MTWG3Q9NNTZ9KvG3CWA1SP3aq97"
    }
]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `address` | The register address of the account |
| `balance` | The current balance of the account |
| `created` | The UNIX timestamp when the account was created |
| `modified` | The UNIX timestamp when the account was last modified |
| `owner` | The username hash of the account owner |
| `token` | The register address of the token | 