# Operators Documentation

This document describes the operators available for filtering and querying in the LLL-TAO API.

## Overview

The API supports a variety of operators for filtering and querying data. These operators can be used in combination to create complex queries.

## Comparison Operators

### Basic Comparison

| Operator | Description | Example |
|:---------|:------------|:--------|
| `eq` | Equal to | `field:eq:value` |
| `ne` | Not equal to | `field:ne:value` |
| `gt` | Greater than | `field:gt:value` |
| `lt` | Less than | `field:lt:value` |
| `gte` | Greater than or equal to | `field:gte:value` |
| `lte` | Less than or equal to | `field:lte:value` |

### String Comparison

| Operator | Description | Example |
|:---------|:------------|:--------|
| `like` | Pattern match | `field:like:pattern` |
| `ilike` | Case-insensitive pattern match | `field:ilike:pattern` |
| `starts` | Starts with | `field:starts:prefix` |
| `ends` | Ends with | `field:ends:suffix` |
| `contains` | Contains substring | `field:contains:substring` |

### List Operations

| Operator | Description | Example |
|:---------|:------------|:--------|
| `in` | In list | `field:in:value1,value2` |
| `nin` | Not in list | `field:nin:value1,value2` |
| `all` | All values in list | `field:all:value1,value2` |
| `any` | Any value in list | `field:any:value1,value2` |

## Logical Operators

### Basic Logic

| Operator | Description | Example |
|:---------|:------------|:--------|
| `AND` | Logical AND | `field1:eq:value1 AND field2:eq:value2` |
| `OR` | Logical OR | `field1:eq:value1 OR field2:eq:value2` |
| `NOT` | Logical NOT | `NOT field:eq:value` |

### Complex Logic

| Operator | Description | Example |
|:---------|:------------|:--------|
| `()` | Grouping | `(field1:eq:value1 OR field2:eq:value2) AND field3:eq:value3` |

## Examples

### Basic Comparison

```
GET /api/users?query=age:gt:18
```

### String Comparison

```
GET /api/users?query=name:like:John*
```

### List Operations

```
GET /api/users?query=status:in:active,pending
```

### Complex Logic

```
GET /api/users?query=(status:eq:active OR status:eq:pending) AND age:gte:18
```

## Best Practices

1. Use appropriate operators
2. Combine operators logically
3. Use grouping for complex queries
4. Test query performance
5. Document operator usage
6. Validate operator values
7. Handle operator errors
8. Monitor query complexity
9. Cache common queries
10. Use indexes effectively
