# Error Documentation

This document describes the error codes and handling for the LLL-TAO API.

## Overview

The API uses a standardized error response format to provide clear information about errors that occur during requests.

## Error Response Format

```json
{
  "success": false,
  "error": {
    "code": "ERROR_CODE",
    "message": "Error description",
    "details": {
      // Additional error details (optional)
    }
  }
}
```

## Error Codes

### Authentication Errors

| Code | Description | HTTP Status |
|:-----|:------------|:------------|
| `AUTH_REQUIRED` | Authentication required | 401 |
| `INVALID_TOKEN` | Invalid session token | 401 |
| `INVALID_PIN` | Invalid PIN | 401 |
| `TOKEN_EXPIRED` | Session token expired | 401 |
| `INVALID_CREDENTIALS` | Invalid username or password | 401 |

### Authorization Errors

| Code | Description | HTTP Status |
|:-----|:------------|:------------|
| `PERMISSION_DENIED` | Permission denied | 403 |
| `INSUFFICIENT_PERMISSIONS` | Insufficient permissions | 403 |
| `ACCOUNT_LOCKED` | Account is locked | 403 |
| `IP_BLOCKED` | IP address is blocked | 403 |

### Validation Errors

| Code | Description | HTTP Status |
|:-----|:------------|:------------|
| `INVALID_PARAM` | Invalid parameter | 400 |
| `MISSING_PARAM` | Required parameter missing | 400 |
| `INVALID_FORMAT` | Invalid data format | 400 |
| `INVALID_VALUE` | Invalid parameter value | 400 |
| `DUPLICATE_VALUE` | Duplicate value not allowed | 400 |

### Resource Errors

| Code | Description | HTTP Status |
|:-----|:------------|:------------|
| `NOT_FOUND` | Resource not found | 404 |
| `ALREADY_EXISTS` | Resource already exists | 409 |
| `CONFLICT` | Resource conflict | 409 |
| `GONE` | Resource no longer available | 410 |

### Rate Limiting Errors

| Code | Description | HTTP Status |
|:-----|:------------|:------------|
| `RATE_LIMITED` | Rate limit exceeded | 429 |
| `TOO_MANY_REQUESTS` | Too many requests | 429 |

### Server Errors

| Code | Description | HTTP Status |
|:-----|:------------|:------------|
| `SERVER_ERROR` | Internal server error | 500 |
| `SERVICE_UNAVAILABLE` | Service unavailable | 503 |
| `DATABASE_ERROR` | Database error | 500 |
| `NETWORK_ERROR` | Network error | 500 |

## Error Handling

### Client-Side Handling

1. Check the `success` field in the response
2. If `success` is `false`, handle the error
3. Extract the error code and message
4. Take appropriate action based on the error code

Example:
```javascript
try {
  const response = await api.get('/endpoint');
  if (!response.success) {
    handleError(response.error);
  }
} catch (error) {
  handleError(error);
}
```

### Server-Side Handling

1. Use appropriate HTTP status codes
2. Provide clear error messages
3. Include relevant error details
4. Log errors for debugging
5. Implement proper error recovery

## Best Practices

1. Always check for errors
2. Handle errors gracefully
3. Provide user-friendly messages
4. Log errors for debugging
5. Implement retry logic
6. Use appropriate status codes
7. Include error details
8. Document error handling
9. Test error scenarios
10. Monitor error rates
