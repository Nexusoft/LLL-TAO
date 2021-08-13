# Query DSL
-----------------------------------

The Query DSL allows you to sort and filter recursively to any logical depth. This DSL can be used in conjunction with operators to sort, filter, and compute data in real-time.


## `Selector Keys`

The following operators are supported for this API command-set:

[`object`] - This selector key will evaluate the comparison on the raw binary object.   
[`results`]   - This selector key will evaluate the comparison on the results json.


## `Selector Keys`





## Supported Objects

The following object classes are supported:

* object - operates on a raw object register
* results - operates and filters based on the JSON results. This can be about 10x slower, so use sparingly.


## Using wildcards

If you are searching by a string parameter, you can include '*' as an any character wildcard match, so that you can search values
based on a partial match.

```
register/list/accounts WHERE 'object.name=d*'
```

The above will return all accounts that start with a letter 'd'.

### Following wildcard

The following demonstrates how to check with wildcards.

```
register/list/accounts WHERE 'object.name=*d'
```

This above will return all accounts that have a name ending with letter 'd'.


## Examples

To filter, you can use where='statements' or follow the command with WHERE string:

### Filtering objects with WHERE clause

The below clause will filter all name object registers, that are Global names that start with letter 'P', or any objects that start
with letter 'S'.

```
register/list/names WHERE '(object.namespace=*GLOBAL* AND object.name=P*) OR object.name=S*'
```

Using the object class i.e. 'object.namespace' will invoke the filter on the binary object.

### Filtering objects with where=

The below clause will filter all name object registers, that are Global names that start with letter 'P', or any objects that start
with letter 'S'.

```
register/list/names where='(object.namespace=*GLOBAL* AND object.name=P*) OR object.name=S*'
```

### Filtering with multiple operators

The below will return all NXS accounts that have a balance greater than 10 NXS.

```
register/list/accounts WHERE 'object.token=0 AND object.balance>10'
```

### Creating logical grouping

The following demonstrates how to query using multiple recursive levels.

```
register/list/accounts WHERE '(object.token=0 AND object.balance>10) OR (object.token=8Ed7Gzybwy3Zf6X7hzD4imJwmA2v1EYjH2MNGoVRdEVCMTCdhdK AND object.balance>1)'
```

This will give all NXS accounts with balance greater than 10, or all accounts for token '8Ed7Gzybwy3Zf6X7hzD4imJwmA2v1EYjH2MNGoVRdEVCMTCdhdK' with balance greater than 1.


### More complex queries

There is no current limit to the number of levels of recursion, such as:

```
register/list/names WHERE '((object.name=d* AND object.namespace=~GLOBAL~) OR (object.name=e* AND object.namespace=send.to)) OR object.namespace=*s'
```

The above command will return all objects starting with letter 'd' that are global names, or all objects starting with letter 'e' in
the 'send.to' namespace, or finally all objects that are in a namespace that ends with the letter 's'.
