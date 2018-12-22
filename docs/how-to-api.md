# The Nexus API

Accessing the Nexus API is a seamless and simple experience. You have three options:

* Use a web browser, and make a GET request to the API endpoint using key=value pairs after the ? seperated by & (ex. http://localhost:8080/api/method?key=value&key1=value1)

* Use a web browser, and build a form to do a POST request with the items in the form as the parameters. Set the action to the endpoint (ex. http://localhost:8080/api/method)

* Use the CLI utility, using the -api flag to make a local request from commandline (ex. ./nexus -api api method key=value key1=value)

This DOES NOT yet support URL encoding, so don't use spaces in any of your values otherwise you will run into problems.


## Accounts API

The accounts API is responsible for maintaining account level functions. DO NOT USE this API on a foreign node you are not the manager of. Your username and password are stored in secure allocators, but the remote endpoint can still gather your credentials when you submit an API call. This API is meant for a service node you control only.

The list of commands can be found:
[COMMANDS](API/ACCOUNTS.MD)


## Supply API

The supply API is responsible for handling supply chain logistics. It's main aim is designed for managing the routes and movements along a supply chain, along with reviewing the history of events associated with changes of custody.

The list of commands can be found:
[COMMANDS](API/SUPPLY.MD)
