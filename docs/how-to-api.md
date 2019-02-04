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


### Results JSON
The response JSON you will receive will contain one of two keys, results or error.
When a command is successfully executed, you will receive a JSON string such as:
```
{"result":{"genesis":"69c2479c6780782448f419a0865542002ee85fec39228275b2db44bb6d3aa503","session":4940881975319897416}}
```

You will need to parse out the values of the result object in your corresponding language of choice. If you happen to receive a
failure, it will look like this:

```
{"error":{"code":-24,"message":"Missing Password"}}
```

Note that you will be parsing for the error key now, and there will be no results. This allows you the choice as a programmer to check for the existence of the "error" key to check for errors, or check for the existence of the "result" key to find if it executed successfully. The choice is yours how you handle it, just know that it is an either / or situation. You will get only one or the other. 


## Accounts API

The accounts API is responsible for maintaining account level
functions. DO NOT USE this API on a foreign node you are not the
manager of. Your username and password are stored in secure
allocators, but the remote endpoint can still gather your credentials
when you submit an API call. This API is meant for a service node you
control only.

The Accounts API can be found in the following repo path:

[LLL-TAO/docs/API/ACCOUNTS.MD](API/ACCOUNTS.MD)

NOTE: Login sessions do not persist if you restart your node.
They are stored in secure allocator in memory only, do not cache
them on disk otherwise you could open potential security issues.


## Supply API

The supply API is responsible for handling supply chain
logistics. It's main aim is designed for managing the routes and
movements along a supply chain, along with reviewing the history of
events associated with changes of custody.

The Supply API can be found in the following repo path:

[LLL-TAO/docs/API/SUPPLY.MD](API/SUPPLY.MD)

NOTE: some of the commands in this API require LOGIN. Make sure to use the
accounts API to login before using LOGIN required commands.


## Lisp API

The lisp API provides information on the lisp configuration and status for this node.
The purpose of this API is to offer a summarized and simplified wrapper around the lispers.net API,
which itself must be running on the local node at localhost:9090/lisp 

The Lisp API can be found in the following repo path:

[LLL-TAO/docs/API/LISP.MD](API/SUPPLY.MD)
