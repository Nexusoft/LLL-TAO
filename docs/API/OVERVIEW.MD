# Nexus API Overview
-------------------------------------

Nexus provides several APIs to allow users and developers to easily interact with the functionality of the Nexus software stack. Each API is a logically grouped collection of methods that either interact with a specific part of the stack (e.g. ledger, registers), by functional area (accounts, assets, tokens), or by industry specific use case (supply, music).


### `Protocol`
The API server uses the `HTTP` protocol and supports both **`GET`** and **`POST`** request methods.  

The API's use **`JSON`** (JavaScript Object Notation) as their primary data transport mechanism for both input and output.  However the API server also supports URL encoded requests (**`application/x-www-form-urlencoded`**).   

### `Port`
By default the API server runs on port 8080. This can be changed by setting `apiport=xxxx` in your nexus.conf or starting your daemon with the `-apiport=xxxx` parameter.


### `Endpoint`

Each API endpoint can be invoked through the following semantics:

**`<api>/<verb>/<noun>`**

e.g.
```
http://127.0.0.1:8080/ledger/get/block
```

**NOTE**:  URI's are always in lowercase.

****

## `Verbs (actions)`
The following list of verbs are used throughout the Nexus APIs.  While the action is generic in nature (i.e. list, get, create), implementations of these actions will adhere to certain semantics or mandatory parameters regardless of the noun (resource).

* `list`
   - Retrieves a sorted list of objects or data
   - Sorting is always newest to oldest
   - Supported parameters:
     - `page` + `limit` (where the start point is known) *or* `index` + `limit`
     - `verbose` to indicate the verbosity of the data returned.  Values can be `default`, `summary`, or `detail`
     - `format` indicates the format of the returned data (where applicable). Values can be `raw`, `JSON`, `XML`   

* `get`
   - Retrieves a single object or piece of data
   - Supported parameters:
     - `format` indicates the format of the returned data (where applicable). Values can be `raw`, `JSON`, `XML`

* `create`
   - Creates a single resource / object
   - Supported parameters:
     - `format` indicates the format of the data being provided to create the item / object (where applicable). Values can be `basic`, `raw`, `JSON`, `XML`, and `ANSI C`
* `debit`
* `create`
* `transfer`
* `claim`
* `reverse`
* `buy`
* `sell`
* `login`
* `logout`
* `unlock`
* `lock`
* `subscribe`

****

## `Nouns (resources)`

Within each API endpoint, a noun is used to describe the resource or object upon which the action is taking place.  

The commonly used resources are described below:

1. `transaction` :

    A transaction is a cryptographic object that contains the contracts that operated on the registers.   
    It is indexed by an uint512_t, so any index used must be sanitized to this format.

2. `block` :

    A block is a cryptographic object that contains the transactions in a specific sequence of time.  
    It is indexed by an uint1024_t, so any index used must be sanitized to this format.  
    A block index is prepended with 0x to imply hexadecimal encoding.
    Base-16 encoding is all that is supported at present.
    Other forms of encoding may be supported at a later time (possibly base-58).

3. `account` :

    An account is an object register of the following types: trust or base.   
    It is responsible for holding the state of a user's account balance.

4. `object` :

    An object is a register with nType being REGISTER::OBJECT.
    An object is always parsed, and access to it's data members given.

5. `token` :

    A token is an object register of standard OBJECT::TOKEN.   
    It contains the domain parameters of a token such as supply or total number of significant digits.

6. `asset` :

    An asset is an object register of a specialized type.


****
## `Error Handling`

In the event that an API call results in an error, a JSON object will be returned with the following format:
```
{
    "error" :
    {
        "code" : -123,
        "message" : An error has occurred"
    }
}
```

Error Codes

| Code   | Message   |
| :-----:| :-------- |
| -1 | Method not available |
| -2 | Method not found (method) |
| -3 | Unspecified API error |
| -4 | API not found (API name) |
| -5 | content-type (content type) not supported |
| -6 | content-type not provided when content included |
| -7 | Invalid transaction id |
| -8 | Unable to Extract Input Address |
| -9 | Session (session) doesn't exist |
| -10 | Invalid session ID |
| -11 | User not logged in |
| -12 | Missing Session ID |
| -13 | Account not found |
| -14 | Object failed to parse |
| -15 | Object is not an account or token |
| -16 | Account has not been unlocked for transactions |
| -17 | Failed to create transaction |
| -18 | Missing data |
| -19 | Data must be a string with this asset format |
| -20 | Missing asset value fields |
| -21 | Missing json parameter |
| -21 | json field must be an array |
| -22 | Missing name field in json definition |
| -23 | Missing type field in json definition |
| -24 | Missing value field in json definition |
| -25 | Missing mutable field in json definition |
| -26 | maxlength value is less than the specified data length |
| -27 | Malformed base64 encoding |
| -28 | Missing asset field definitions |
| -29 | Unsupported format specified |
| -30 | Operations failed to execute |
| -31 | Ledger failed to sign transaction |
| -32 | Failed to accept |
| -33 | Missing name / address |
| -34 | Asset not found |
| -35 | Specified name/address is not an asset |
| -36 | Failed to parse object register |
| -37 | Missing token name / address |
| -38 | Missing asset name / address |
| -39 | Missing name_from / address_from |
| -40 | Previous transaction not found |
| -41 | Failed to parse object from debit transaction |
| -42 | No valid debits to buy from |
| -43 | No valid contracts in debit tx |
| -44 | Transaction failed to build |
| -45 | Missing name_to / address_to |
| -46 | Missing amount |
| -47 | Missing price |
| -48 | Failed to read to state |
| -49 | Failed to parse to state |
| -50 | Missing txid |
| -51 | Debit transaction is not for a NXS account.  Please use the tokens API for crediting token accounts |
| -52 | Failed to parse asset owner object |
| -53 | Missing name / address of account to credit |
| -54 | Missing name_proof / address_proof of token account that proves your credit share |
| -55 | Invalid name / address of account to credit |
| -56 | Failed to parse account to credit |
| -57 | Failed to parse object to debit from |
| -58 | Debit transaction is not from a NXS account |
| -59 | Account to credit is not a NXS account |
| -60 | Failed to parse proof object |
| -61 | Proof account is for a different token than the asset token |
| -62 | Coinbase transaction mined by different user |
| -63 | Could not retrieve default NXS account to credit |
| -64 | Missing recipient account name_to / address_to |
| -65 | Object is not an account |
| -66 | Account is not a NXS account.  Please use the tokens API for debiting non-NXS token accounts |
| -67 | Account is not a NXS account.  Please use the tokens API for accessing non-NXS token accounts |
| -68 | Amount too small |
| -69 | Insufficient funds |
| -70 | Trust account not found |
| -71 | Unable to parse trust account |
| -72 | Register is not a trust account |
| -73 | Trust account is not a NXS account |
| -74 | No registers found |
| -75 | Unable to retrieve trust account |
| -76 | Cannot set stake for trust account until after Genesis transaction |
| -77 | Insufficient trust account balance to add to stake |
| -78 | Stake not changed |
| -79 | getblockhash requires the daemon to be started with the -indexheight flag |
| -80 | Missing height |
| -81 | Invalid height parameter |
| -82 | Block number out of range |
| -83 | Block not found |
| -84 | Missing hash or height |
| -85 | getblock by height requires the daemon to be started with the -indexheight flag |
| -86 | Missing hash or txid |
| -87 | Invalid or unknown transaction |
| -88 | Missing name |
| -89 | Invalid register_address |
| -90 | Namespace already exists |
| -91 | Missing name / register_address |
| -92 | Name not found |
| -93 | Missing namespace name |
| -94 | Invalid namespace |
| -95 | Namespace does not exist: (namespace) |
| -96 | Cannot create a name in namespace (namespace) as you are not the owner |
| -97 | An object with this name already exists in this namespace |
| -98 | An object with this name already exists for this user |
| -99 | Transfer transaction not found |
| -100 | Missing username prefix before name |
| -101 | Unknown name: (name) |
| -102 | Invalid address |
| -103 | Missing register_address parameter |
| -104 | Object not found |
| -105 | Missing address |
| -106 | Invalid name / address |
| -107 | No history found |
| -108 | Failed to read transaction |
| -109 | Specified name/address is not of type (type) |
| -110 | Contract execution failed |
| -111 | Missing genesis / username |
| -112 | Missing username / destination |
| -113 | Destination user doesn't exist |
| -114 | (type) not found |
| -115 | Name / address is not of type (type) |
| -116 | Cannot transfer names created without a namespace |
| -117 | Item not found |
| -118 | Missing type |
| -119 | Missing supply |
| -120 | Debit transaction is for a NXS account.  Please use the Finance API for crediting NXS accounts |
| -121 | Account to credit is not for the same token as the debit |
| -122 | Token/account not found |
| -123 | Object is not a token |
| -124 | Unknown token / account |
| -125 | Token not found |
| -126 | Address is not for a NXS account" |
| -127 | Missing username |
| -128 | Missing password |
| -129 | Missing PIN |
| -130 | Account already exists |
| -131 | Lock not supported in multiuser mode |
| -132 | Account already locked |
| -133 | Zero-length username |
| -134 | Zero-length password |
| -135 | Zero-length PIN |
| -136 | Account doesn't exist |
| -137 | Couldn't get transaction |
| -138 | No previous transaction found |
| -139 | Invalid credentials |
| -140 | Already logged in with a different username |
| -141 | Already logged out |
| -142 | Username parameter not supported for this method.  Names can only be obtained for the logged in user |
| -143 | No notifications available |
| -144 | No transactions found |
| -145 | Unlock not supported in multiuser mode |
| -146 | Account already unlocked for minting |
| -147 | Account already unlocked for transactions |
| -148 | Account already unlocked |
| -149 | Invalid PIN |
| -150 | Transaction rejected |
| -151 | Transaction already in database |
| -152 | Transaction contains no valid transfers |
| -153 | Transfer recipient object not found |
| -154 | Invalid field type (field) |
| -155 | Raw assets can not be updated |
| -156 | Field not found in asset (field) |
| -157 | Field not mutable in asset (field) |
| -158 | Value longer than maximum length (field) |
| -159 | Values must be passed in as strings |
| -160 | Username contains invalid characters |
| -161 | Name contains invalid characters.  Please escape them with '\' |
| -162 | Name contains invalid characters |
| -163 | Could not retrieve default NXS account to debit fees |
| -164 | Account 'default' is not a NXS account |
| -165 | Invalid address_to |
| -166 | "Account is a NXS account.  Please use the finance API for accessing NXS accounts." |
| -167 Name cannot be less than 3 characters long |




