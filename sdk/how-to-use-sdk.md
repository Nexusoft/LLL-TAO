
How to use the Nexus Python SDK
-------------------------------

This is a document brief on how the Nexus SDK calling sequence flow can work
for your Blockchain application. See LLL-TAO/sdk/nexus-sdk-primer.py for a
real code sequence you change and experiment with.    

Initializing with the SDK
-------------------------

To initialize with the python SDK, your python program should:

```
    import nexus_sdk
```

The nexus_sdk SDK library is in LLL-TAO/sdk/nexus_sdk.py. Then you can call
sdk_init() to initialize with a credentials:

```
    myuser = nexus_sdk.sdk_init("<username>", "<password>", "<pin>")
```

You choose values for &lt;<username&gt;>, &lt;<password&gt;>, and
&lt;<pin&gt;>. Keep them private since they give you access to your Nexus
Tritium account.    
            
Creating a User
---------------

To create a user that is stored on the Tritium Blockchain, you call:

```
    status = myuser.nexus_accounts_create()
```

Where myuser is the return value from sdk_init(). A dictionary array is always
returned. There are no errors in any SDK call when:

```
    if (status.has_key("error") == False):
        print "accounts/create successful"
    #endif               
```

Logging in a User
-----------------

If a user is stored on the Tritium Blockchain, then you may login from any
python program by calling:

```
    status = myuser.nexus_accounts_login()
```

Where myuser is the return value from sdk_init().    

Logging out a User
------------------

If you have logged in from a python program and and want to logout, call:
    
```
    status = myuser.nexus_accounts_logout()
```

Where myuser is the return value from sdk_init(). You no longer can do
anymore Accounts API calls or Supply API calls. They will return error since
you are not logged in.        

Listing Tranasctions for a User
-------------------------------                    

If you want to list transactions for a particular user, call:
    
```
    tx = myuser.nexus_accounts_transactions()
```

Where myuser is the return value from sdk_init(). This will return an array
of transactions in variable tx.

Creating a Supply-Chain Item
----------------------------

```
    status = myuser.nexus_supply_createitem("<data>")
```

Where myuser is the return value from sdk_init(). Stores the value
&lt;<data&gt;> in a address register. The address is returned in
status["result"]["address"].

Lookup a Supply-Chain Item in the Blockchain
--------------------------------------------

You must supply a register address to lookup an item on the Blockchain. The
value of &lt;<address&gt;> below is retunrned from nexus_supply_createitem():

```
    status = myuser.nexus_supply_getitem("<address>")
```

Where myuser is the return value from sdk_init(). What is returned is the
value of the register address in status["result"]["state"].

Transfering Ownership of a Register Address
-------------------------------------------

nexus_supply_getitem() returns in status["result"]["owner"] the genesis
ID of the current owner of the item. You can change the owner by calling:

```
    status = myuser.nexus_supply_transfer("<address>", "<new-owner-genesis>")
```

Where myuser is the return value from sdk_init(). Where the ownership
of register address &lt;<address&gt;> is change to user with genesis
ID of &lt;<new-owner-genesis&gt;>. Obtaining the &lt;<new-owner-genesis&gt;>
genesis ID of another user happens out of band.

Obtaining a History of changes to a Register Address
----------------------------------------------------

If you want to track ownership changes and register address value changes,
you can call:

```
    status = myuser.nexus_supply_history("<address>")
```

Where myuser is the return value from sdk_init(). The variable status returns
information for each owner that had owned register address &lt;<address&gt;>.

Return Status Information
-------------------------

The Nexus python SDK will always return a dictionary array that maps exactly
from the JSON returned by the Nexus daemon support4ed APIs. In some cases,
the API will return error codes and in other cases the SDK will return error
codes.
    
-------------------------------------------------------------------------------
