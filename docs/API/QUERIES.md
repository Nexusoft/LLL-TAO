# Query DSL
-----------------------------------

The Query DSL allows you to sort and filter recursively to any logical depth. This DSL can be used in conjunction with operators to sort, filter, and compute data in real-time.


## Using wildcards

If you are searching by a string parameter, you can includeM' as an any character wildcard match, so that you can search values
based on a partial match.

```
register/list/accounts WHERE 'results.name=d*'
```

The above will return all accounts that start with a letter 'd'.

### Following wildcard

The following demonstrates how to check with wildcards.

```
register/list/accounts WHERE 'results.name=*d'
```

This above will return all accounts that have a name ending with letter 'd'.


## Examples

To filter, you can use where='statements' or follow the command with WHERE string:

### Filtering resultss with WHERE clause

The below clause will filter all name results registers, that are Global names that start with letter 'P', or any resultss that start
with letter 'S'.

```
register/list/names WHERE '(results.namespace=*GLOBAL* AND results.name=P*) OR results.name=S*'
```

Using the results class i.e. 'results.namespace' will invoke the filter on the binary results.

### Filtering resultss with where=

The below clause will filter all name results registers, that are Global names that start with letter 'P', or any resultss that start
with letter 'S'.

```
register/list/names where='(results.namespace=*GLOBAL* AND results.name=P*) OR results.name=S*'
```

### Filtering with multiple operators

The below will return all NXS accounts that have a balance greater than 10 NXS.

```
register/list/accounts WHERE 'results.token=0 AND results.balance>10'
```

### Creating logical grouping

The following demonstrates how to query using multiple recursive levels.

```
register/list/accounts WHERE '(results.token=0 AND results.balance>10) OR (results.token=8Ed7Gzybwy3Zf6X7hzD4imJwmA2v1EYjH2MNGoVRdEVCMTCdhdK AND results.balance>1)'
```

This will give all NXS accounts with balance greater than 10, or all accounts for token '8Ed7Gzybwy3Zf6X7hzD4imJwmA2v1EYjH2MNGoVRdEVCMTCdhdK' with balance greater than 1.


### More complex queries

There is no current limit to the number of levels of recursion, such as:

```
register/list/names WHERE '((results.name=d* AND results.namespace=~GLOBAL~) OR (results.name=e* AND results.namespace=send.to)) OR results.namespace=*s'
```

The above command will return all resultss starting with letter 'd' that are global names, or all resultss starting with letter 'e' in
the 'send.to' namespace, or finally all resultss that are in a namespace that ends with the letter 's'.
