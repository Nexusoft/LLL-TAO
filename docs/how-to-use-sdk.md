
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
sdk_init() to initialize with credentials:

```
    myuser = nexus_sdk.sdk_init("<username>", "<password>", "<pin>")
```

You choose values for &lt;username&gt;, &lt;password&gt;, and &lt;pin&gt;.
Keep them private since they give you access to your Nexus Tritium account.

Returns:
```    
>>> myuser
<nexus_sdk.sdk_init instance at 0x102350bd8>
>>> dir(myuser)
['__doc__', '__init__', '__module__', '_sdk_init__error', '_sdk_init__get', '_sdk_init__login', 'genesis_id', 'nexus_accounts_create', 'nexus_accounts_login', 'nexus_accounts_logout', 'nexus_accounts_transactions', 'nexus_supply_createitem', 'nexus_supply_getitem', 'nexus_supply_history', 'nexus_supply_transfer', 'password', 'pin', 'session_id', 'username']
```    

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
```

Returns:
```
>>> status = myuser.nexus_accounts_create()
>>> status
{u'result': {u'hash': u'5304a99e0096ff222f59305dd580d5a75083e8b4e6ec8c51eb0083998b4a0927e887b6df849d63ff5b678c887b6e50f492f7740c3d87cb62c1d85824f5ad427f', u'sequence': 0, u'timestamp': 1546482268, u'signature': u'3081840240332eeec4622fe74a467d8b16891596b1511db48ad48077e3e90b24949f8df20078661cf4a5138c74456b3ee43da8de825859089c3bcddbdbe55d13ce982813a0024033e108e50f835bad648c2c022d1a48223779cadcd3b95d775984d3c51f5ce5a2a44dd5639e7348b11f5c19ea70dc6d768a9b3f9f8cca58904da842187b15374f', u'version': 1, u'nexthash': u'c019bfe2dae644efe921e95356f079812ccc51096da6f6f8a181aa4749629a38', u'prevhash': u'00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000', u'pubkey': u'036ce0ecac385d5417a639e50db660b0299732e449985193630556b9464f89a5e219b29607ec29efd21d381a31c7c3f0937aef25bdaea51017fadaeeb224ed6519', u'genesis': u'8f2b585ad33bdadda440830c44d7d40036b81a39d637a0ec9efbf1dcfd5cda1a'}}
```    

Logging in a User
-----------------

If a user is stored on the Tritium Blockchain, then you may login from any
python program by calling:

```
    status = myuser.nexus_accounts_login()
```

Where myuser is the return value from sdk_init().

Returns:
```
>>> status = myuser.nexus_accounts_login()
>>> status
{u'result': {u'session': 3137402842395995342, u'genesis': u'8f2b585ad33bdadda440830c44d7d40036b81a39d637a0ec9efbf1dcfd5cda1a'}}
```

Logging out a User
------------------

If you have logged in from a python program and and want to logout, call:

```
    status = myuser.nexus_accounts_logout()
```

Where myuser is the return value from sdk_init(). You no longer can do
anymore Accounts API calls or Supply API calls. They will return error since
you are not logged in.

Returns:
```
>>> status = myuser.nexus_accounts_logout()
>>> status
{u'result': {u'genesis': u'8f2b585ad33bdadda440830c44d7d40036b81a39d637a0ec9efbf1dcfd5cda1a'}}
```

Listing Tranasctions for a User
-------------------------------                    

If you want to list transactions for a particular user, call:

```
    tx = myuser.nexus_accounts_transactions()
```

Where myuser is the return value from sdk_init(). This will return an array
of transactions in variable tx["result"].

Returns:
```
>>> tx = myuser.nexus_accounts_transactions()
>>> tx
{u'result': [{u'hash': u'5304a99e0096ff222f59305dd580d5a75083e8b4e6ec8c51eb0083998b4a0927e887b6df849d63ff5b678c887b6e50f492f7740c3d87cb62c1d85824f5ad427f', u'sequence': 0, u'timestamp': 1546482268, u'signature': u'3081840240332eeec4622fe74a467d8b16891596b1511db48ad48077e3e90b24949f8df20078661cf4a5138c74456b3ee43da8de825859089c3bcddbdbe55d13ce982813a0024033e108e50f835bad648c2c022d1a48223779cadcd3b95d775984d3c51f5ce5a2a44dd5639e7348b11f5c19ea70dc6d768a9b3f9f8cca58904da842187b15374f', u'version': 1, u'nexthash': u'c019bfe2dae644efe921e95356f079812ccc51096da6f6f8a181aa4749629a38', u'prevhash': u'00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000', u'pubkey': u'036ce0ecac385d5417a639e50db660b0299732e449985193630556b9464f89a5e219b29607ec29efd21d381a31c7c3f0937aef25bdaea51017fadaeeb224ed6519', u'genesis': u'8f2b585ad33bdadda440830c44d7d40036b81a39d637a0ec9efbf1dcfd5cda1a'}]}
```

Creating a Supply-Chain Item
----------------------------

```
    status = myuser.nexus_supply_createitem("<data>")
```

Where myuser is the return value from sdk_init(). Stores the value
&lt;data&gt; in a address register. The address is returned in
status["result"]["address"].

Returns:
```
>>> status = myuser.nexus_supply_createitem("<data>")
>>> status
{u'result': {u'txid': u'f21566f869b2375f553645ee476a2e24ea900aea7b460c42274aa36d4bb0f53afa98ba1ea037aaa24810fb108809f9eaf717ce320db8483ac1f9852ee5d4b5f9', u'address': u'26a47e31f4e7886eaf53618eb96346a6178b219ded1926927fb73e9566424989'}}
```

Lookup a Supply-Chain Item in the Blockchain
--------------------------------------------

You must supply a register address to lookup an item on the Blockchain. The
value of &lt;address&gt; below is returned from nexus_supply_createitem():

```
    status = myuser.nexus_supply_getitem("<address>")
```

Where myuser is the return value from sdk_init(). What is returned is the
value of the register address in status["result"]["state"].

Returns:
```
>>> status = myuser.nexus_supply_getitem("26a47e31f4e7886eaf53618eb96346a6178b219ded1926927fb73e9566424989")
>>> status
{u'result': {u'owner': u'8f2b585ad33bdadda440830c44d7d40036b81a39d637a0ec9efbf1dcfd5cda1a', u'state': u'<data>'}}
```

Modifying an Existing Supply-Chain Item
---------------------------------------

```
    status = myuser.nexus_supply_updateitem("<address>", "<new-data>")
```

Where myuser is the return value from sdk_init(). Modifies the value stored
in address register <address> (with current value &lt;data&gt;) with the new
value &lt;new-data&gt;.

Returns:
```
>>> status = myuser.nexus_supply_updateitem("26a47e31f4e7886eaf53618eb96346a6178b219ded1926927fb73e9566424989", "<new-data>")
>>> status
{u'result': {u'txid': u'c4bebcdccd6a18a76a52d1a03e81822d1eb7b6e8431f1daccee765a966937de651e25691fba3a843cea274825c95fecf49df231678c9a31642c94b05b55d6728', u'address': u'26a47e31f4e7886eaf53618eb96346a6178b219ded1926927fb73e9566424989'}}
```

Transfering Ownership of a Register Address
-------------------------------------------

nexus_supply_getitem() returns in status["result"]["owner"] the genesis
ID of the current owner of the item. You can change the owner by calling:

```
    status = myuser.nexus_supply_transfer("<address>", "<new-owner-genesis>")
```

Where myuser is the return value from sdk_init() and the ownership
of register address &lt;address&gt; is change to user with genesis
ID of &lt;new-owner-genesis&gt;. Obtaining the &lt;new-owner-genesis&gt;
genesis ID of another user happens out of band.

Returns:
```
>>> new_owner = nexus_sdk.sdk_init("newuser", "pw", "1234")
>>> new_owner.nexus_accounts_create()
{u'result': {u'hash': u'83a0c773e27a9d8c53727e06f5933341f00cc1278a9677c47559eb320151050f4e7b9e7ca98ba60a0d9684b00f055ed2b344e4edb525e5e7526d50323c7c1c78', u'sequence': 0, u'timestamp': 1546482626, u'signature': u'3081840240200043c510fae55c1cb24356fa240b50cf834051422db732d2c032d322eb128c803931661f58d49ea6f5753e1b85390e7aa64d2b5bddc317a26e1d8b6e929fd402406783c093edf83896f895fba1066a8bc03970434d062907a69cab1e2032f67a6bf967b57f3ac225e0844fb826e08e537ff7eb8822ce07a70b3a8c8db1ecacd971', u'version': 1, u'nexthash': u'50512cc57506b5744f5b21001e1bee8506115d0e54529f921badd58c45daaa90', u'prevhash': u'00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000', u'pubkey': u'03450f9341e180aa7071234efe06270fb33e36fa9b32f6bf9310f61a1cef844394d5903e73d014c4094242cdaef1186b9d021093f08a89ea1a843041c0c565ee4a', u'genesis': u'224449b75265d2891377a9916c9ed2e9b5cd8060d4f3dd100de1633a98c2e677'}}
>>> new_owner.nexus_accounts_login()
{u'result': {u'session': 10949755597081409007L, u'genesis': u'224449b75265d2891377a9916c9ed2e9b5cd8060d4f3dd100de1633a98c2e677'}}

>>> status = myuser.nexus_supply_transfer("26a47e31f4e7886eaf53618eb96346a6178b219ded1926927fb73e9566424989", "224449b75265d2891377a9916c9ed2e9b5cd8060d4f3dd100de1633a98c2e677")
>>> status
{u'result': {u'txid': u'7f1fc45abf8ccc3216347e3a5824d4c5ee6984c2e5b2483702b7e0a19ad98d8e48267dfd4fc0bb75dd10c90a68047baa2876ec38535f8067b5ba6cfd0efda6f6', u'address': u'26a47e31f4e7886eaf53618eb96346a6178b219ded1926927fb73e9566424989'}}
```

Obtaining a History of changes to a Register Address
----------------------------------------------------

If you want to track ownership changes and register address value changes,
you can call:

```
    status = myuser.nexus_supply_history("<address>")
```

Where myuser is the return value from sdk_init(). The variable status returns
information for each owner that had owned register address &lt;address&gt;
which is an array of dictionary arrays in status["result"].

Returns:
```
>>> status = myuser.nexus_supply_history("26a47e31f4e7886eaf53618eb96346a6178b219ded1926927fb73e9566424989")
>>> status
{u'result': [{u'owner': u'224449b75265d2891377a9916c9ed2e9b5cd8060d4f3dd100de1633a98c2e677', u'checksum': 3537596000004923786, u'state': u'063c646174613e', u'version': 1, u'type': 0}, {u'owner': u'8f2b585ad33bdadda440830c44d7d40036b81a39d637a0ec9efbf1dcfd5cda1a', u'checksum': 14512905263792948095L, u'state': u'063c646174613e', u'version': 1, u'type': 0}]}
```

Return Status Information
-------------------------

The Nexus python SDK will always return a dictionary array that maps exactly
from the JSON returned by the Nexus daemon supported APIs. In some cases,
the API will return error codes and in other cases the SDK will return error
codes.

-------------------------------------------------------------------------------
