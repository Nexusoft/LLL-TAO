# Protocol Documentation

This document describes the protocol used for communication between clients and the API server.

## Overview

The API uses a RESTful protocol over HTTP/HTTPS. All requests and responses are in JSON format.

## Authentication

### Session Management

Sessions are managed using tokens. To authenticate:

1. Create a session using the `sessions/create` endpoint
2. Use the returned session token in subsequent requests
3. Include the token in the `Authorization` header:
   ```
   Authorization: Bearer <session_token>
   ```

### Multi-User Mode

When running in multi-user mode (`-multiuser=1`), each request must include:

- `session` parameter: The session token
- `pin` parameter (if profile is locked): The PIN for the profile

## Request Format

### Headers

| Header | Description |
|:-------|:------------|
| `Content-Type` | Must be `application/json` |
| `Authorization` | Bearer token for authentication |
| `Accept` | Must be `application/json` |

### Parameters

Parameters can be passed in two ways:

1. Query Parameters:
   ```
   GET /api/endpoint?param1=value1&param2=value2
   ```

2. JSON Body:
   ```json
   {
     "param1": "value1",
     "param2": "value2"
   }
   ```

## Response Format

### Success Response

```json
{
  "success": true,
  "data": {
    // Response data
  }
}
```

### Error Response

```json
{
  "success": false,
  "error": {
    "code": "ERROR_CODE",
    "message": "Error description"
  }
}
```

## Error Codes

| Code | Description |
|:-----|:------------|
| `AUTH_REQUIRED` | Authentication required |
| `INVALID_TOKEN` | Invalid session token |
| `INVALID_PIN` | Invalid PIN |
| `INVALID_PARAM` | Invalid parameter |
| `NOT_FOUND` | Resource not found |
| `PERMISSION_DENIED` | Permission denied |
| `SERVER_ERROR` | Internal server error |

## Rate Limiting

The API implements rate limiting to prevent abuse. Limits are:

- 100 requests per minute per IP
- 1000 requests per hour per IP
- 10000 requests per day per IP

When rate limited, the API will return:

```json
{
  "success": false,
  "error": {
    "code": "RATE_LIMITED",
    "message": "Rate limit exceeded",
    "retry_after": 60
  }
}
```

## Versioning

The API version is included in the URL path:

```
/api/v1/endpoint
```

Current versions:
- v1: Current stable version
- v2: Beta version (subject to change)

## WebSocket Support

The API also supports WebSocket connections for real-time updates. Connect to:

```
ws://host:port/ws
```

### WebSocket Events

| Event | Description |
|:------|:------------|
| `connect` | Connection established |
| `disconnect` | Connection closed |
| `error` | Error occurred |
| `update` | Data update received |

### WebSocket Message Format

```json
{
  "event": "event_name",
  "data": {
    // Event data
  }
}
```

## Security

### HTTPS

All API endpoints should be accessed over HTTPS. The server will:

1. Redirect HTTP to HTTPS
2. Use TLS 1.2 or higher
3. Support modern cipher suites

### CORS

Cross-Origin Resource Sharing (CORS) is supported with the following headers:

```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, PUT, DELETE
Access-Control-Allow-Headers: Content-Type, Authorization
```

## Best Practices

1. Always use HTTPS
2. Include proper error handling
3. Implement retry logic with exponential backoff
4. Cache responses when appropriate
5. Use compression for large responses
6. Monitor rate limits
7. Keep session tokens secure
8. Validate all input data
9. Handle timeouts gracefully
10. Log errors for debugging
