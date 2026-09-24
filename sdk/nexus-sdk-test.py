#!/usr/bin/env python3
#
# nexus-sdk-test.py
#
# Tests basic functions of the python nexus-sdk.
#
# Usage: python3 nexus-sdk-test.py [detail]
#
#------------------------------------------------------------------------------

import sys
import nexus_sdk

width = 40

#------------------------------------------------------------------------------

def call_api(label, function):
    print("{} ...".format(label).ljust(width), end="")
    json_data = function()

    if "error" in json_data:
        error = json_data["error"]
        api_or_sdk = "SDK" if "sdk-error" in error else "API"
        msg = error["message"]
        print("{} failed - {}".format(api_or_sdk, msg))
    else:
        error = None
        print("succeeded")
    #endif
    if detail:
        print("  {}".format(json_data))
    return json_data["result"] if error is None else None
#enddef

def print_transactions(j):
    if j is None:
        return
    for tx in j:
        print("  tx {}".format(tx["hash"]))
#enddef

#------------------------------------------------------------------------------

detail = "detail" in sys.argv

print("Initialize SDK for user sdk-test-user1 ...", end="")
user1 = nexus_sdk.NexusSDK("sdk-test-user1", "sdk-test", "1234")
print("done")
print("Initialize SDK for user sdk-test-user2 ...", end="")
user2 = nexus_sdk.NexusSDK("sdk-test-user2", "sdk-test", "1234")
print("done")
print("")

#
# Make calls to profiles API to create users.
#
j = call_api("Create user profile sdk-test-user1", user1.profiles_create)
j = call_api("Create user profile sdk-test-user2", user2.profiles_create)

#
# Login using sessions API.
#
j = call_api("Login user sdk-test-user1", user1.sessions_create)
if j:
    print("  genesis {}".format(j.get("genesis", "N/A")))
j = call_api("Login user sdk-test-user2", user2.sessions_create)
if j:
    print("  genesis {}".format(j.get("genesis", "N/A")))

#
# Get transactions using profiles API.
#
j = call_api("Transactions for sdk-test-user1", user1.profiles_transactions)
print_transactions(j)
j = call_api("Transactions for sdk-test-user2", user2.profiles_transactions)
print_transactions(j)

#
# Create 10 supply-chain items for sdk-test-user1.
#
print("Create 10 supply-chain items for sdk-test-user1 ...", end="")
addresses = []
for i in range(10):
    j = user1.supply_create("item{}".format(i), "data{}".format(i))
    if "error" in j:
        print("failed - {}".format(j["error"]["message"]))
        break
    #endif
    if j.get("result"):
        addresses.append(j["result"]["address"])
#endfor
if addresses:
    print("done, created {} items".format(len(addresses)))
else:
    print("no items created (may already exist)")

#
# Print addresses and data values.
#
if addresses:
    print("Print addresses and data values for sdk-test-user1:")
    for addr in addresses:
        j = user1.supply_get(address=addr)
        if "result" in j and j["result"]:
            result = j["result"]
            owner = result.get("owner", "N/A")[:8] + "..."
            state = result.get("state", "N/A")
            print("  owner {}: address {}, data {}".format(owner, addr[:16] + "...", state))
#endif

#
# Move some items to sdk-test-user2.
#
if addresses and user2.genesis_id:
    print("Transfer items with data6-data9 to sdk-test-user2 ...", end="")
    transferred = 0
    for addr in addresses:
        j = user1.supply_get(address=addr)
        if "result" in j and j["result"]:
            value = j["result"].get("state", "")
            if value in ["data6", "data7", "data8", "data9"]:
                user1.supply_transfer(user2.genesis_id, address=addr)
                transferred += 1
    print("done, transferred {} items".format(transferred))
#endif

#
# Show session status for both users.
#
j = call_api("Session status for sdk-test-user1", user1.sessions_status)
j = call_api("Session status for sdk-test-user2", user2.sessions_status)

#
# Logout users.
#
j = call_api("Logout sdk-test-user1", user1.sessions_terminate)
j = call_api("Logout sdk-test-user2", user2.sessions_terminate)

print("")
print("All tests completed!")

#------------------------------------------------------------------------------
