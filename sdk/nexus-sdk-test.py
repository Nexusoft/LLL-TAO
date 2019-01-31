#
# nexus-sdk-test.py
#
# Tests basic functions of the python nexus-sdk.
#
# Usage: python nexus-sdk-test.py [detail]
#
#------------------------------------------------------------------------------

import sys
import nexus_sdk

width = 40

#------------------------------------------------------------------------------

def call_api(label, function):
    print "{} ...".format(label).ljust(width),
    json_data = function()

    if (json_data.has_key("error")):
        error = json_data["error"]
        api_or_sdk = "SDK" if error.has_key("sdk-error") else "API"
        msg = error["message"]
        print "{} failed - {}".format(api_or_sdk, msg)
    else:
        error = None
        print "succeeded"
    #endif
    if (detail): print "  {}".format(json_data)
    return(json_data["result"] if error == None else None)
#enddef

def print_transactions(j):
    if (j == None): return
    for tx in j: print "  tx {}".format(tx["hash"])
#enddef

#------------------------------------------------------------------------------

detail = "detail" in sys.argv

print "Initialize SDK for user sdk-test-user1 ...",
user1 = nexus_sdk.sdk_init("sdk-test-user1", "sdk-test", "1234")
print "done"
print "Initialize SDK for user sdk-test-user2 ...",
user2 = nexus_sdk.sdk_init("sdk-test-user2", "sdk-test", "1234")
print "done"
print ""

#
# Make calls to accounts API for both users.
#
j = call_api("Create user account sdk-test-user1", user1.nexus_accounts_create)
j = call_api("Create user account sdk-test-user2", user2.nexus_accounts_create)

j = call_api("Login user sdk-test-user1", user1.nexus_accounts_login)
if (j): print "  genesis {}".format(j["genesis"])
j = call_api("Login user sdk-test-user2", user2.nexus_accounts_login)
if (j): print "  genesis {}".format(j["genesis"])

j = call_api("Transactions for sdk-test-user1",
    user1.nexus_accounts_transactions)
print_transactions(j)
j = call_api("Transactions for sdk-test-user2",
    user2.nexus_accounts_transactions)
print_transactions(j)

#
# Create 10 supply-chain items for sdk-test-user1.
#
print "Create 10 supply-chain items for sdk-test-user1 ...",
address = []
for i in range(10):
    j = user1.nexus_supply_createitem("data" + str(i))
    if (j.has_key("error")):
        print "failed"
        exit(1)
    #endif
    address.append(j["result"]["address"])
#endfor
print "done"

#
# Print 10 data items.
#
print "Print 10 addresses and data values for sdk-test-user1:"
for a in address:
    j = user1.nexus_supply_getitem(a)
    print "  owner {}:".format(j["result"]["owner"])
    print "    address {}, data {}".format(a, j["result"]["state"])
#endfor

#
# Move data6 to data9 to sdk-test-user2.
#
print "Move ownership for addresses with data6 - data9 stored ...",
for a in address:
    j = user1.nexus_supply_getitem(a)
    value = user1.nexus_supply_getitem(a)["result"]["state"]
    if (value in ["data6", "data7", "data8", "data9"]):
        user1.nexus_supply_transfer(a, user2.genesis_id)
    #endif        
#endfor
print "done"

#
# Print 10 data items showing what ownership has changed verus not.
#
print "Print 10 addresses with mixed ownershup:"
for a in address:
    j = user1.nexus_supply_getitem(a)
    print "  owner {}:".format(j["result"]["owner"])
    print "    address {}, data {}".format(a, j["result"]["state"])
#endfor

exit(0)

#------------------------------------------------------------------------------



