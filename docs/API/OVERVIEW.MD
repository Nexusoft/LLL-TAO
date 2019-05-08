# Nexus API Overview
-------------------------------------

Nexus provides several API's to allow users and integrators to easily interact with the functionality of the Nexus software stack.  Each API is a logically grouped collection of methods that either interact with a specific part of the stack (e.g. ledger, registers), by functional area (accounts, assets, tokens), or by industry specific use case (supply, music).


### Protocol
The API server uses the HTTP protocol and supports both **GET** and **POST** request methods.  

The API's use **JSON** (JavaScript Object Notation) as their primary data transport mechanism for both input and output.  However the API server also supports URL encoded requests (**application/x-www-form-urlencoded**).   

### Port
By default the API server runs on port 8080, thought this can be changed by setting apiport=xxxx in your nexus.conf or starting your daemon with the -apiport=xxxx parameter


### Endpoint 

Each API endpoint can be invoked through the following semantics:

**`<api>/<verb>/<noun>`**

e.g.
```
http://127.0.0.1:8080/ledger/get/block
```

**NOTE**:  URI's are always in lowercase.

## Verbs (actions)
The following list of verbs are used throughout the Nexus API's.  Where the action is generic in nature (i.e. list, get, create), implementations of these actions will adhere to certain semantics or mandatory parameters regardless of the noun (resource)

* `list`
   - Retrieves a sorted list of objects or data
   - Sorting is always youngest to oldest
   - Supported parameters:
     - **page** + **limit** (where the start point is known) *or* **index** + **limit**
     - **verbose** to indicate the verbosity of the data returned.  Values can be **default**, **summary**, or **detail**
     - **format** indicates the format of the returned data (where applicable). Values can be **RAW**, **JSON**, **XML**   

* `get`
   - Retrieves a single object or piece of data
   - Supported parameters:
     - **format** indicates the format of the returned data (where applicable). Values can be **RAW**, **JSON**, **XML**
       
* `create`
   - Creates a single resource / object 
   - Supported parameters:
     - **format** indicates the format of the data being provided to create the item / object (where applicable). Values can be **RAW**, **JSON**, **XML**, **ANSI C**, and **PYTHON**
* `debit`
* `create`
* `transfer`
* `claim`
* `reverse`
* `buy`
* `sell`
* `login`
* `logout`
* `unlock`
* `lock`
* `subscribe`


## Nouns (resources)

Within each API endpoint, a noun is used to describe the resource or object upon which the action is taking place.  

The commonly used resources are described below:

1. `transaction` : 
    
    A transaction is a cryptographic object that contains the contracts that operated on the registers.   
    It is indexed by an uint512_t, so any index used must be sanitized to this format

2. `block` : 

    A block is a cryptographic object that contains the transactions in a specific sequence of time.  
    It is indexed by an uint1024_t, so any index used must be sanitized to this format.  
    A block index is prepended with 0x to imply hexadecimal encoding.  
    Other forms of encoding may be supported at later times, but for now, Base-16 is all that is allowed (Possibly Base58 at a later time as well)

3. `account` :

    An account is an object register of the following types: trust or base.   
    It is responsible for holding the state of a user account’s balance

4. `object` : 

    An object is a register with nType being REGISTER::OBJECT   
    It is always parsed, and access to its data members given

5. `token` : 

    A token is an object register of standard OBJECT::TOKEN   
    It contains the domain parameters of a token such as supply or total number of significant digits

6. `asset` : 

    An asset is an object register of specialized type
    


## Error Handling

In the event that an API call results in an error, a JSON object will be returned with the following format:
```
{
    "error" : 
    {
        "code" : -123,
        "message" : An error has occurred"
    }
}
```

Error Codes 

| Code   | Message   |
| :-----:| :-------- |
| -4 | Missing Parameter (parameter name) |
| -5 | Invalid Transaction (GetLastError) |
| -6 | Missing Entry (index) |


**THIS LIST IS NOT UP TO DATE AND OUR ERROR CODES / MESSAGES NEED TO BE UPDATED THROUGHOUT THE CODE**



