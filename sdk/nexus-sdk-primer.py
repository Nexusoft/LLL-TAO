#
# nexus-sdk-primer.py
#
# A very simple sequential program for interacting with the Nexus Tritium
# API. If you want more details and want to see more interactions and
# return response detail, use LLL-TAO/sdk/nexus-sdk-test.py.
#
# If you want a bit more detail in this program, just uncomment "#print json"
# lines.
#
# Usage: python nexus-sdk-primer.py
#
#------------------------------------------------------------------------------

import nexus_sdk

width = 48

#
# Initialize with SDK.
#
print "Initialize Nexus SDK ...".ljust(width),
primer = nexus_sdk.sdk_init("primer", "primer-password", "1234")
new_owner = nexus_sdk.sdk_init("new-owner", "primer-password", "1234")
print "succeeded"

#
# Call API accounts/create if not already logged in.
#
print "Create username 'primer' ...".ljust(width),
json = primer.nexus_accounts_create()
good = "failed" if json.has_key("error") else "succeeded"
print good
print "Create username 'new-owner' ...".ljust(width),
json = new_owner.nexus_accounts_create()
good = "failed" if json.has_key("error") else "succeeded"
print good

#
# Call API accounts/login
#
print "Login user 'primer' ...".ljust(width),
json = primer.nexus_accounts_login()
good = "failed" if json.has_key("error") else "succeeded"
if (good == "succeeded"):
    print "succeeded, genesis {}".format(primer.genesis_id)
else:
    print "failed"
#endif    
print "Login user 'new-owner' ...".ljust(width),
json = new_owner.nexus_accounts_login()
good = "failed" if json.has_key("error") else "succeeded"
if (good == "succeeded"):
    print "succeeded, genesis {}".format(new_owner.genesis_id)
else:
    print "failed"
#endif    

#
# Call API accounts/logout
#
print "Logout user 'new-owner' ...".ljust(width),
json = new_owner.nexus_accounts_logout()
good = "failed" if json.has_key("error") else "succeeded"
print good

#
# Call API accounts/transactions
#
print "Show transactions for user 'primer' ...".ljust(width),
json = primer.nexus_accounts_transactions()
good = "failed" if json.has_key("error") else "succeeded"
print good
#print json

print ""

#
# Call API supply/createitem
#
data = "0xdeadbeef"
print "Supply-Chain create item, data '{}' ...".format(data).ljust(width),
json = primer.nexus_supply_createitem(data)
good = "failed" if json.has_key("error") else "succeeded"
#print json
if (good == "succeeded"):
    address = json["result"]["address"]
    print "succeeded, address {}".format(address)
else:
    print "Abnormal exit"
    exit(1)
#endif    

#
# Call API supply/getitem
#
print "Supply-Chain get item ...".ljust(width),
json = primer.nexus_supply_getitem(address)
good = "failed" if json.has_key("error") else "succeeded"
if (good == "succeeded"):
    print "succeeded, stored data '{}'".format(json["result"]["state"])
else:
    print "Abnormal exit"
    exit(1)
#endif

print ""

#
# Call API supply/transfer
#
print "Transfer genesis {} to genesis {}".format(primer.genesis_id,
    new_owner.genesis_id)
print "Transfer ownership ...".ljust(width),
json = primer.nexus_supply_transfer(address, new_owner.genesis_id)
good = "failed" if json.has_key("error") else "succeeded"
print good
#print json

#
# Call API supply/history
#
print "History of item ownership...".ljust(width),
json = primer.nexus_supply_history(address)
good = "failed" if json.has_key("error") else "succeeded"
print good

print "Ownership History:"
for owner in json["result"]:
    print "owner {}\n  checksum {}, state {}, version {}, type {}".format( \
        owner["owner"], owner["checksum"], owner["state"], owner["version"],
        owner["type"])
#endfor                                                                        

print "All Done!"
exit(0)

#------------------------------------------------------------------------------
