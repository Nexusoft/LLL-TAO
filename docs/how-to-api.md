The Nexus API Interface
-----------------------   

Accessing the Nexus API is a seamless and simple experience. You have three options:

* Use a web browser with a URL based request:

```
http://localhost:8080/api/&lt;method&gt;?&lt;key&gt;=&lt;value&gt;&&lt;key1&gt;=&lt;value1&gt;
```

* Use a web browser with a POST based request, where POST body contains
parameters:

http://localhost:8080/api/<method>

* Use the the nexus command-line by specifying the -api flag:

./nexus -api api <method> <key>=<value> <key1>=<value1>

The API DOES NOT yet support URL encoding, so don't use spaces in any of your
values otherwise you will run into problems.


## Accounts API

The accounts API is responsible for maintaining account level
functions. DO NOT USE this API on a foreign node you are not the
manager of. Your username and password are stored in secure
allocators, but the remote endpoint can still gather your credentials
when you submit an API call. This API is meant for a service node you
control only.

The Accounts API can be found repo path:

[LLL-TAO/docs/API/ACCOUNTS.MD](API/ACCOUNTS.MD)


## Supply API

The supply API is responsible for handling supply chain
logistics. It's main aim is designed for managing the routes and
movements along a supply chain, along with reviewing the history of
events associated with changes of custody.

The Supply API can be found in repo path:

The list of commands can be found:
[LLL-TAO/docs/API/SUPPLY.MD](API/SUPPLY.MD)
