# `Protocol`

The following document explains the protocol and how to create basic calls to your given command-set.

## `Endpoint`

The API server uses the `HTTP/HTTPS` protocol and supports both **`GET`** and **`POST`**. Each API endpoint can be invoked through the following URI syntax:

``<command>/<verb>/<noun>/<filter>/<operator>``

This can be invoked by either providing a direct link as follows:

```
http://localhost:8080/system/get/info
```

Or you can use the commandline tools provided with the `LLL-TAO` to invoke from the CLI:

```
./nexus system/get/info
```

**NOTE**:  URI's are always in lowercase.


## `Port`

By default the API server runs on port **8080** for the main network, and **7080** for a testnet or private network. This can be changed by setting `apiport=xxxx` in your nexus.conf followed by a restart, or starting your daemon with the `-apiport=xxxx` parameter.


## `HTTPS`

The Nexus API supports `TLS` which can be accessed through the `HTTPS` protocol.  By default the API server is started with SSL disabled but to enable it, you can add `apissl=1` to your nexus.conf. The default port for the main network is **8443** when running over **HTTPS**, and for testnet's or private networks it is **7443**. To invoke a command using `TLS` use the following syntax:

```
https://localhost:8443/system/get/info
```

Unless you have setup a certificate signed by a CA, you will have issues with certification verification. If using curl, you can proceed anyhow using the following syntax:

```
curl --insecure https://localhost:8443/system/get/info
```

If accessing through php or other means, refer to their relevant documentation to allow self-signed certificates.


## `Authentication`

The API supports the `HTTP Basic` authentication scheme.  This requires the caller to set an `Authorization` HTTP header with the value `Basic <credentials>` with the `<credendtials>` portion set to `apiuser:apipassword` in `base64` encoding. On the API server side the authentication is configured by adding `apiuser=<username>` and `apipassword=<password>` to the nexus.conf.  If you wish to disable authentication entirely, you can do so by setting `apiauth=0` in your nexus.conf, the daemon will not start the API server if you do not configure credentials or set `-apiauth=0` in your configuration.


## `Sessions`
The API can be configured to run in multiuser mode, allowing multiple signature chains to be logged in and use the API simultaneously.  This feature can be enabled by setting `multiuser=1` in your nexus.conf (default off).  When multiuser is enabled, the initial login response will include a session ID value.  This session ID must be then be included in all subsequent API calls to endpoints that require a user to be logged in.


## `Request Encoding`

All API requests can be made using multiple encoding methods. To make a command in your web-browser, or through the simplest interface you can use a `GET` form:

```
http://localhost:8080/sessions/create/local?username=user1&password=pass1&pin=1234
```

If making request using the `POST` endpoint, you can follow the same semantics using **url-form-encoding** inside the `POST` data:

```
http://localhost:8080/sessions/create/local

POST-DATA:
username=user&password=pass1&pin=1234
```

The simplest way to obtain this is by using an HTML form:

```
<form type='post' action='http://localhost:8080/sessions/create/local'>
<input type='text' id='username'>
<input type='text' id='password'>
<input type='text' id='pin'>
<input type='submit' value='Login!'>
</form>
```

The above form will encode in **url-form-encoding** and send the form via `POST`. The above is a sample login page to show you the ease of working with the Nexus Framework to replace existing systems such as OAuth. You are also able to provide JSON data for all POST requests using **application/json**:

```
http://localhost:8080/sessions/create/local

POST-DATA:
{
    "username" : "user1",
    "password" : "pass1",
    "pin"      : "1234"
}
```



## `Response Encoding`
All commands use **`JSON`** (JavaScript Object Notation) to structure the response data with the following structure:

```
"result":
{
    "key" : "value"
},
"info": {
  "method": "<commands>/<verb>/<noun>",
  "status": "active",
  "address": "127.0.0.1:41260",
  "latency": "0.017555 ms"
}
```

The data can be accessed using the `result` key if the method returned successfully. You can also see information related to the command such as the command invoked, the command's status, calling address, and the time it took to complete the command.

## `Error Handling`

In the event that an API call results in an error, a JSON object will be returned with the `error` key rather than `result`. This is a safe assumption to tell if your command succeeded, by checking if your JSON object contains either `error` or `result`.

```
"error" :
{
    "code" : -123,
    "message" : "An error has occurred"
},
"info": {
  "method": "<commands>/<verb>/<noun>",
  "status": "active",
  "address": "127.0.0.1:41260",
  "latency": "0.017555 ms"
}
```
