# Query Documentation

This document describes the query syntax and capabilities for the LLL-TAO API.

## Overview

The API supports powerful query capabilities for filtering, sorting, and retrieving data. Queries can be used with most endpoints that return lists of items.

## Query Syntax

### Basic Syntax

Queries are specified using the `query` parameter in the URL:

```
GET /api/endpoint?query=<query_string>
```

### Query Components

A query consists of:

1. Field name
2. Operator
3. Value

Example:
```
field:operator:value
```

## Available Operators

See [OPERATORS.MD](OPERATORS.MD) for a complete list of available operators.

Common operators include:
- `eq` - Equal to
- `ne` - Not equal to
- `gt` - Greater than
- `lt` - Less than
- `gte` - Greater than or equal to
- `lte` - Less than or equal to
- `in` - In list
- `nin` - Not in list
- `like` - Pattern match
- `ilike` - Case-insensitive pattern match

## Filtering

See [FILTERING.MD](FILTERING.MD) for detailed information about filtering options.

### Basic Filters

```
GET /api/endpoint?query=status:eq:active
```

### Multiple Filters

Use `AND` and `OR` to combine filters:

```
GET /api/endpoint?query=status:eq:active AND type:eq:user
```

### Complex Filters

```
GET /api/endpoint?query=(status:eq:active OR status:eq:pending) AND type:eq:user
```

## Sorting

See [SORTING.MD](SORTING.MD) for detailed information about sorting options.

### Basic Sorting

```
GET /api/endpoint?sort=field:asc
```

### Multiple Sort Fields

```
GET /api/endpoint?sort=field1:asc,field2:desc
```

## Pagination

### Basic Pagination

```
GET /api/endpoint?page=1&limit=10
```

### Pagination with Filters

```
GET /api/endpoint?query=status:eq:active&page=1&limit=10
```

## Examples

### Simple Query

```
GET /api/users?query=status:eq:active
```

### Complex Query

```
GET /api/users?query=(status:eq:active OR status:eq:pending) AND type:eq:user&sort=created_at:desc&page=1&limit=10
```

### Query with Multiple Conditions

```
GET /api/transactions?query=amount:gt:100 AND type:eq:transfer&sort=timestamp:desc
```

## Best Practices

1. Use appropriate operators for the data type
2. Combine filters logically
3. Use pagination for large result sets
4. Sort results when order matters
5. Test queries with small datasets first
6. Use indexes for frequently queried fields
7. Avoid overly complex queries
8. Cache common query results
9. Monitor query performance
10. Document complex queries
