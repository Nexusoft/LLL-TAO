# Finance API

The Finance API provides methods for managing financial operations in the system, including accounts, tokens, and trust accounts.

## Supported Nouns

The following nouns are supported for this API command set:

| Noun | Description |
|:-----|:------------|
| `account` | An object register containing a token-id and balance |
| `token` | An object register containing a token-id, balance, supply, and decimals |
| `trust` | An object register containing a token-id, balance, and trust |
| `any` | An object selection noun allowing mixed accounts of different tokens |
| `all` | An object selection noun to collect all accounts for given token type |

## Supported Verbs

The following verbs are currently supported by this command set:

| Verb | Description | Nouns |
|:-----|:------------|:------|
| [`create`](#create) | Generate a new object of supported type | `account`, `token`, `trust` |
| [`get`](#get) | Get object of supported type | `any`, `account`, `token`, `trust` |
| [`list`](#list) | List all objects owned by given user | `any`, `account`, `token`, `trust` |
| [`history`](#history) | Generate the history of all last states | `any`, `account`, `token`, `trust` |
| [`transactions`](#transactions) | List all transactions that modified specified object | `any`, `account`, `token`, `trust` |
| [`burn`](#burn) | Remove a given token from circulation | `token` |
| [`credit`](#credit) | Claim funds issued to account from debit | `any`, `account`, `token`, `trust` |
| [`debit`](#debit) | Issue funds from supported type | `any`, `account`, `token`, `trust` |

## Direct Endpoints

The following commands are direct endpoints and thus do not support the verb and noun structure:

| Command | Description |
|:--------|:------------|
| [`get/balances`](#getbalances) | Get account balances |
| [`get/stakeinfo`](#getstakeinfo) | Get stake information |
| [`migrate/accounts`](#migrateaccounts) | Migrate accounts |
| [`set/stake`](#setstake) | Set stake amount |
| [`void/transaction`](#voidtransaction) | Void a transaction |

## Deprecated Endpoints

The following endpoints are deprecated and will be removed in a future version:

- [`list/balances`](#listbalances) - Deprecated in favor of `finance/get/balances`
- [`list/trustaccounts`](#listtrustaccounts) - Deprecated in favor of `register/list/trust`
- [`list/account/transactions`](#listaccounttransactions) - Deprecated in favor of `finance/transactions/account`

## Commands

### <a name="create"></a> Create Object

Create a new object register specified by given noun.

```bash
finance/create/noun
```

This command does not support the `any` or `all` nouns.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

#### Optional Parameters

- `name`: A UTF-8 encoded string that will generate a name object register that points to new object. If noun is `token` this will be created as a global name
- `data`: Optional for any noun, allows caller to add arbitrary data to object

##### Token-Specific Parameters
For `create/token`:

- `supply` (required): Sets the maximum token supply
- `decimals` (required): Sets the total number of significant figures. Defaults to 2

##### Account-Specific Parameters
For `create/account`:

- `token` (required): A Base58 encoded address or ticker name. Defaults to `NXS`

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
| `txid` | The hash of the transaction that was generated for this transaction. If using `-autotx` this field will be omitted |
| `address` | The register address for this account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account |

### <a name="debit"></a> Debit Account

Deduct an amount of NXS or token specified by the noun and send it to another account or legacy UTXO address. Only NXS can be sent to the legacy address.

```bash
finance/debit/noun
```

This command supports the any wildcard noun.

#### Debit Types

- `debit/account`: Deducts an amount of NXS or tokens from a token account or token address to be sent to a receiving account
- `debit/token`: Deducts an amount of tokens from a token address to send to a token account

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Transaction
The following parameters are required to identify and process the transaction:

- `from` (required): The account to debit, this can be the name or the register address. The name should be in the format name (for global token names), username:name (for local names) or namespace::name (for names in a namespace)
- `amount` (required): The amount of NXS or token to debit
- `to` (required): The receiving account. This can be the account name or the register address. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace). The address_to can also contain a legacy UTXO address if sending from a signature chain account to a legacy address

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
[Completed in 4979.735275 ms]
```

##### Return Values

| Field | Description |
|:------|:------------|
| `success` | Boolean flag indicating that the debit was successful |
| `txid` | The ID (hash) of the transaction that includes the debit |

### <a name="credit"></a> Credit Account

Create a new object register specified by given noun.

```bash
finance/credit/noun
```

This command supports the any wildcard noun.

#### Credit Types

- `credit/account`: Increment an amount received from a NXS account, token account or token address
- `credit/token`: Increment an amount of tokens received from a token account

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN for this profile
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Transaction
The following parameter is required to identify the transaction:

- `txid` (required): The hash in hexadecimal encoding of the transaction that we are crediting

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
| `txid` | The ID (hash) of the transaction that includes the account creation |
| `address` | The register address for this account. The address (or name that hashes to this address) is needed when creating crediting or debiting the account |

### <a name="burn"></a> Burn Tokens

This method can be used to take tokens permanently out of the current supply, a process commonly known as burning. The method will debit a token account and send the tokens back to the token address. However the transaction contains a condition that will always evaluate to false, guaranteeing that the debit can not be credited by the token issuer nor the sender. The result is that the amount burned will always appear in the "pending" balance of the token.

```bash
finance/burn/noun
```

This command only supports the `account` noun.

#### Required Parameters

##### Session
This command requires a valid session to be active with these parameters required to identify and authenticate the given session:

- `pin` (required if **locked**): The PIN to authorize the transaction
- `session` (required if `-multiuser=1` is set): Used to identify the session to invoke the command under. For single-user API mode the session should not be supplied

##### Transaction
The following parameters are required to identify and process the transaction:

- `name` (required if `address` not provided): The name of account to debit the tokens from to be burnt. The name should be in the format username:name (for local names) or namespace::name (for names in a namespace)
- `address` (required if `name` not provided): The register address of the account to debit the tokens from to be burnt
- `amount` (required): The amount of tokens to burn

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