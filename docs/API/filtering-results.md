# Filtering API Results
-----------------------------------

All `list` methods in the Nexus API provide callers with the ability to limit and filter the returned results.


## `limit`

The number of records to return, default 100


## `offset`

An alternative to `page`, offset can be used to return a set of (limit) results from a particular index.


## `page`

Allows the results to be returned by page (zero based). E.g. passing in page=1 will return the second set of (limit) transactions. The default value is 0 if not supplied. 


## `where`

 - Describe how where clause works
 - define parameters (`field`, `op`, `value`)
 - list acceptable op values (`>`, `<`, `=`, `>=`, `<=`, `==`)
 - describe the hierarchical nature of the field, eg  `contracts.OP`

Example request
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
