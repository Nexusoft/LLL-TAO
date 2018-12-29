The Nexus API Interface
-----------------------   

Accessing the Nexus API is a seamless and simple experience. To access the API, you can use one of three options:


### Use a web browser with a URL based request:

Make a GET request with the parameters in the URL as seen below.

```
http://localhost:8080/<api>/<method>?<key>=<value>&<key1>=<value1>
```

The API uses form encoding, so you are free to use any characters you wish. The only thing to be aware of is when using
a '+' character in the encoding, which acts as an escape character for a space as well as '%20'. The API will detect this
behavior as long as the form encodes the + sign in form encoding. At times making a GET request, the web browser will
assume that + is a space and not encode it, the same is true for '%' when making a GET request. 


### Use a web browser with a POST based request

The POST body contains parameters, and would go to an endpoint such as:
```
http://localhost:8080/<api>/<method>
```

An example web form would look like this:
```
<form action='http://localhost:8080/<api>/<method>' method='post'>
<input type = 'text' name = '<key>'  value = '<value>' >
<input type = 'text' name = '<key1>' value = '<value2>'>
<input type = 'submit'>
</form>
```

This will allow you to integrate the Nexus API into existing web forms.
This allows you to harness the power of the Nexus Blockchain in all your existing web services.


### Use the the nexus command-line by specifying the -api flag:

The Nexus Daemon must already be running for this to work. Make sure you are in the same directory as the Nexus Daemon.

```
./nexus -api <api>/<method> <key>=<value> <key1>=<value1>
```

This will return all data in JSON format into your console.


## Accounts API

The accounts API is responsible for maintaining account level
functions. DO NOT USE this API on a foreign node you are not the
manager of. Your username and password are stored in secure
allocators, but the remote endpoint can still gather your credentials
when you submit an API call. This API is meant for a service node you
control only.

The Accounts API can be found in the following repo path:

[LLL-TAO/docs/API/ACCOUNTS.MD](API/ACCOUNTS.MD)


## Supply API

The supply API is responsible for handling supply chain
logistics. It's main aim is designed for managing the routes and
movements along a supply chain, along with reviewing the history of
events associated with changes of custody.

The Supply API can be found in the following repo path:

The list of commands can be found:
[LLL-TAO/docs/API/SUPPLY.MD](API/SUPPLY.MD)
