# Filtering API Results
-----------------------------------

All `list` methods in the Nexus API provide callers with the ability to limit and filter the returned results.


## `limit`

The number of records to return, default 100.


## `offset`

An alternative to `page`, offset can be used to return a set of (limit) results from a particular index.


## `page`

Allows the results to be returned by page (zero based). E.g. passing in page=1 will return the second set of (limit) transactions. The default value is 0 if not supplied. 


## `where`


Takes the returned JSON data and filters it based on a certain set of criteria.


### Parameters:
 
`field` : The field that the filter will be applied to. A prefix can be added to certain fields if necessary to access hierarchical results.

`op` : The operation to be performed to filter the results. Acceptable operations include: `>`, `<`, `=`, `>=`, `<=`, `==`.

`value` : The value that the JSON result is being compared to. 


### Hierarchy:

Certain /list/xxx API methods return arrays in their results. For example, the `users/list/transactions` results returned contains a  `contracts` array. To filter the JSON result by a contracts value `OP`, you would use `contracts.OP` as the field parameter.


### Example:

The following is an example JSON request: 
```
{
    "limit": 10,
    "offset": 0,
    "where":
    [
        {
            "field": "somefield",
            "op": "=",
            "value": "somevalue"
        },
        {
            "field": "balance",
            "op": ">",
            "value": 0
        }
    ],
}
```