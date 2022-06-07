# Templates
-----------------------------------

A lot of the current commands available in the API operate on generic templated code that allows insertion of any object type without the need for repeated functions to handle each object type. The following list of generic endpoints have default parameters that can be useful to know outside of each individual command-set.

# `Scope`

The following templates do not exceed a local scope, meaning they will return results derived from the internal indexing services which requires login and authorization. The data-set returned from these templates is stored in the Logical Database, and thus only provides data local to a profile-id, meaning `history` and `transactions` will only show events that happened on the profile that generated indexes with login.

# `Verbs`
The following list of verbs are templates throughout the Nexus APIs.  While the action is generic in nature (i.e. list, get, create), implementations of these actions will adhere to certain semantics or mandatory parameters regardless of the noun (resource).

* `list`
   - Retrieves a sorted list of objects or data
   - Sorting is based on transaction/object age, by default is always newest to oldest.  Some list methods support additional sort fields.
   - Common parameters:
     - `order` : The order of the results.  Values can be `desc` (default) or `asc`
     - `limit` : The number of records to return, default 100. `limit=none` is allowed.
     - `page`: Allows the results to be returned by page (zero based). E.g. passing in page=1 will return the second set of (limit) transactions. The default value is 0 if not supplied.
     - `offset` : An alternative to `page`, offset can be used to return a set of (limit) results from a particular index.
     - `verbose` : to indicate the verbosity of the data returned.  Values can be `default`, `summary`, or `detail`   

* `get`
   - Retrieves a single object or piece of data
   - Supported parameters:
     - `verbose` : to indicate the verbosity of the data returned.  Values can be `default`, `summary`, or `detail`   

* `create`
   - Creates a single resource or object.
   - Common parameters:
     - `format` : indicates the format of the data being provided to create the item / object (where applicable). Values can be `basic`, `raw`, and `JSON`
     - `scheme` : sets the key type to be used for the given transaction.
     Values can be `falcon` or `brainbpool`

 * `update`
    - Updates a single resource or object.
    - Common parameters:
      - `format` : indicates the format of the data being provided to create the item / object (where applicable). Values can be `basic`, `raw`, `standard`, and `JSON`
      - `scheme` : sets the key type to be used for the given transaction.
      Values can be `falcon` or `brainbpool`

* `transfer`
    - Transfers a single resource or object to another users's profile
    - Common parameters:
      - `address`: the address of object to transfer
      - `recipient`: the profile-id of the sigchain receiving object.
      - `scheme` : sets the key type to be used for the given transaction.
      Values can be `falcon` or `brainbpool`

* `claim`
    - Claims a single resource or object from a given transfer
    - Common parameters:
      - `address`: the address of object to claim
      - `txid`: the transaction-id of the transfer we are claiming.
      - `scheme` : sets the key type to be used for the given transaction.
      Values can be `falcon` or `brainbpool`
* `history`
    - List the history of state changes for a given object.
    - Common parameters:
      - `address`: the address of object to view history for.
      - `order` : The order of the results.  Values can be `desc` (default) or `asc`
      - `limit` : The number of records to return, default 100. `limit=none` is allowed.
      - `page`: Allows the results to be returned by page (zero based). E.g. passing in page=1 will return the second set of (limit) transactions. The default value is 0 if not supplied.
      - `offset` : An alternative to `page`, offset can be used to return a set of (limit) results from a particular index.
      - `verbose` : to indicate the verbosity of the data returned.  Values can be `default`, `summary`, or `detail`   

* `transactions`
    - List all of the transactions that modified a given register.
    - Common parameters:
      - `address`: the address of object to view transactions for.
      - `order` : The order of the results.  Values can be `desc` (default) or `asc`
      - `limit` : The number of records to return, default 100. `limit=none` is allowed.
      - `page`: Allows the results to be returned by page (zero based). E.g. passing in page=1 will return the second set of (limit) transactions. The default value is 0 if not supplied.
      - `offset` : An alternative to `page`, offset can be used to return a set of (limit) results from a particular index.
      - `verbose` : to indicate the verbosity of the data returned.  Values can be `default`, `summary`, or `detail`   
