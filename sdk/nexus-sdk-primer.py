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
import time

#
# Column width for continuation output line.
#
width = 48

#
# sleep
#
# If you want to play around with delay times between API operations. Defaults
# to 1 second. You can litter sleep() calls whereever you like below.
#
def sleep():
    delay = 1
    if (delay == 0): return
    print "Sleeping for {} ...".format(delay)
    time.sleep(delay)
#enddef    

#------------------------------------------------------------------------------

#
# api_print
#
# Print if API succeeded or failed on a continuation output line. When
# program returns, caller should continue. Otherwise this funciton exits
# program.
#
def api_print(json, succeed_msg=None):
    if (json.has_key("error") == False):
        msg = "succeeded"
        if (succeed_msg != None): msg += ", {}".format(succeed_msg)
        print msg
        return
    #endif
        
    msg = json["error"]["message"]
    print "failed, {}".format(msg)
    if (msg == "Connection refused"):
        print "Abnormal exit"
        exit(1)
    #endif
#enddef    

#------------------------------------------------------------------------------

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
api_print(json)
#print json
print "Create username 'new-owner' ...".ljust(width),
json = new_owner.nexus_accounts_create()
api_print(json)
#print json

#
# Call API accounts/login
#
print "Login user 'primer' ...".ljust(width),
json = primer.nexus_accounts_login()
api_print(json, "genesis {}".format(primer.genesis_id))
#print json
print "Login user 'new-owner' ...".ljust(width),
json = new_owner.nexus_accounts_login()
api_print(json, "genesis {}".format(new_owner.genesis_id))
#print json

#
# Call API accounts/logout
#
print "Logout user 'new-owner' ...".ljust(width),
json = new_owner.nexus_accounts_logout()
api_print(json)
#print json

#
# Call API accounts/transactions
#
print "Show transactions for user 'primer' ...".ljust(width),
json = primer.nexus_accounts_transactions()
api_print(json)
#print json

#
# Put in break line between accounts and supply API.
#
print ""

#
# Call API supply/createitem
#
data = "0xdeadbeef"
print "Supply-Chain create item, data '{}' ...".format(data).ljust(width),
json = primer.nexus_supply_createitem(data)
#print json
address = json["result"]["address"] if json.has_key("result") else "?"
api_print(json, "address {}".format(address))

sleep()

#
# Call API supply/getitem
#
print "Supply-Chain get item ...".ljust(width),
json = primer.nexus_supply_getitem(address)
owner = json["result"]["owner"] if json.has_key("result") else "?"
state = json["result"]["state"] if json.has_key("result") else "?"
msg = "stored data '{}', owner {}".format(state, owner)
api_print(json, msg)
#print json

sleep()

#
# Call API supply/transfer
#
print "Transfer genesis {} to genesis {}".format(primer.genesis_id,
    new_owner.genesis_id)
print "Transfer ownership ...".ljust(width),
json = primer.nexus_supply_transfer(address, new_owner.genesis_id)
api_print(json)
#print json

sleep()

#
# Call API supply/history
#
print "Get ownership history of item ...".ljust(width),
json = primer.nexus_supply_history(address)
api_print(json)
#print json

if (json.has_key("error") == False):
    print "Ownership History:"
    for owner in json["result"]:
        print "owner {}:".format(owner["owner"])
        owner.pop("owner")
        print "  {}".format(owner)
    #endfor
#endif

print ""
print "All Done!"
exit(0)

#------------------------------------------------------------------------------
