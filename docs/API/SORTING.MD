# Sorting Documentation

This document describes the sorting capabilities of the LLL-TAO API.

## Overview

The API supports sorting results by one or more fields in ascending or descending order.

## Sorting Syntax

### Basic Sorting

Use the `sort` parameter in the URL:

```
GET /api/endpoint?sort=field:direction
```

Where:
- `field` is the name of the field to sort by
- `direction` is either `asc` (ascending) or `desc` (descending)

Example:
```
GET /api/users?sort=created_at:desc
```

### Multiple Sort Fields

Sort by multiple fields by separating them with commas:

```
GET /api/endpoint?sort=field1:direction1,field2:direction2
```

Example:
```
GET /api/users?sort=status:asc,created_at:desc
```

## Supported Fields

### Common Sort Fields

| Field | Description |
|:------|:------------|
| `created_at` | Creation timestamp |
| `updated_at` | Last update timestamp |
| `name` | Resource name |
| `type` | Resource type |
| `status` | Resource status |
| `id` | Resource identifier |

### Field-Specific Sort Fields

See the specific endpoint documentation for additional sort fields.

## Examples

### Sort by Single Field

```
GET /api/users?sort=name:asc
```

### Sort by Multiple Fields

```
GET /api/users?sort=status:asc,name:asc
```

### Sort with Filtering

```
GET /api/users?query=status:eq:active&sort=created_at:desc
```

### Sort with Pagination

```
GET /api/users?sort=created_at:desc&page=1&limit=10
```

## Best Practices

1. Use appropriate sort fields
2. Consider performance impact
3. Use multiple fields when needed
4. Combine with filtering
5. Use with pagination
6. Document sort fields
7. Test sort combinations
8. Monitor sort performance
9. Cache sorted results
10. Validate sort parameters
