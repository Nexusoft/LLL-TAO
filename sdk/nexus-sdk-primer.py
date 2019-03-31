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
    delay = 2
    if (delay == 0): return
    print "Sleeping for {} ...".format(delay)
    time.sleep(delay)
#enddef

#------------------------------------------------------------------------------

#
# api_print
#
# Print if API succeeded or failed on a continuation output line. When program
# returns, caller should continue. Otherwise this funciton exits program.
#
def api_print(json, succeed_msg=None):
    if (json.has_key("error") == False):
        msg = "succeeded"
        if (succeed_msg != None): msg += ", {}".format(succeed_msg)
        print msg
        return(False)
    #endif

    msg = json["error"]["message"]
    print "failed, {}".format(msg)
    if (msg == "Connection refused"):
        print "Abnormal exit"
        exit(1)
    #endif
    return(True)
#enddef

#
# short_genid
#
# Print genesis-id in the form "xxxx...xxxx".
#
def short_genid(genid):
    return(genid[0:4] + "..." + genid[-5:-1])
#enddef

#------------------------------------------------------------------------------

#
# Initialize with SDK.
#
print "Initialize Nexus SDK ...".ljust(width),
primer1 = nexus_sdk.sdk_init("primer1", "primer-password", "1234")
primer2 = nexus_sdk.sdk_init("primer2", "primer-password", "1234")
temp_primer = nexus_sdk.sdk_init("temp-primer", "primer-password", "1234")
print "succeeded"

print ""
print "---------- Accounts SDK/API ----------"

#
# Call API accounts/create if not already logged in.
#
print "Create username 'primer1' ...".ljust(width),
json = primer1.nexus_accounts_create()
api_print(json)
print "Create username 'primer2' ...".ljust(width),
json = primer2.nexus_accounts_create()
api_print(json)
print "Create username 'temp-primer' ...".ljust(width),
json = temp_primer.nexus_accounts_create()
api_print(json)

#
# Call API accounts/login
#
print "Login user 'primer1' ...".ljust(width),
json = primer1.nexus_accounts_login()
api_print(json, "genesis {}".format(short_genid(primer1.genesis_id)))
print "Login user 'primer2' ...".ljust(width),
json = primer2.nexus_accounts_login()
api_print(json, "genesis {}".format(short_genid(primer2.genesis_id)))
print "Login user 'temp-primer' ...".ljust(width),
json = temp_primer.nexus_accounts_login()
api_print(json, "genesis {}".format(short_genid(temp_primer.genesis_id)))

#
# Call API accounts/logout
#
print "Logout user 'temp-primer' ...".ljust(width),
json = temp_primer.nexus_accounts_logout()
api_print(json)

#
# Call API accounts/transactions
#
print "Show transactions for user 'primer1' ...".ljust(width),
json = primer1.nexus_accounts_transactions()
api_print(json)

#
# Put in break line between accounts and supply API.
#
print ""
print "---------- Supply-Chain SDK/API ----------"

#
# Call API supply/createitem
#
address = None
data = "0xdeadbeef"
print "Supply-Chain create item, data '{}' ...".format(data).ljust(width),
json = primer1.nexus_supply_createitem(data)
if (json.has_key("result") and json["result"] != None):
    address = json["result"]["address"]
#endif
api_print(json, "address {}".format(address))

sleep()

#
# Call API supply/getitem
#
msg = None
print "Supply-Chain get item ...".ljust(width),
json = primer1.nexus_supply_getitem(address)
if (json.has_key("result") and json["result"] != None):
    owner = json["result"]["owner"]
    state = json["result"]["state"]
    msg = "stored data '{}', owner {}".format(state, owner)
#endif
api_print(json, msg)

sleep()

#
# Call API supply/transfer
#
print "Transfer genesis {} to genesis {}".format( \
    short_genid(primer1.genesis_id),  short_genid(primer2.genesis_id))
print "Transfer ownership ...".ljust(width),
json = primer1.nexus_supply_transfer(address, primer2.genesis_id)
api_print(json)

sleep()

#
# Call API supply/history
#
print "Get ownership history of item ...".ljust(width),
json = primer1.nexus_supply_history(address)
api_print(json)
if (json.has_key("result") and json["result"] != None):
    print "Ownership History:"
    for owner in json["result"]:
        print "owner {}:".format(owner["owner"])
        owner.pop("owner")
        print "  {}".format(owner)
    #endfor
#endif

print ""
print "---------- Assets SDK/API ----------"

#
# Call API assets/create
#
address = None
data = "asset-data"
print "Create Asset named '{}' ...".format("primer1-asset").ljust(width),
json = primer1.nexus_assets_create("primer1-asset", data)
if (json.has_key("result") and json["result"] != None):
    address = json["result"]["address"]
#endif
api_print(json, "address {}".format(address))

sleep()

#
# Call API assets/get
#
msg = None
print "Get Asset named 'primer1-asset' ...".ljust(width),
json = primer1.nexus_assets_get_by_name("primer1-asset")
if (json.has_key("result") and json["result"] != None):
    owner = json["result"]["owner"]
    state = json["result"]["metadata"]
    msg = "stored metadata '{}', owner {}".format(state, owner)
#endif
api_print(json, msg)

sleep()

#
# Call API asserts/transfer
#
msg = None
print "Transfer Asset 'primer1-asset' to user 'primer2'"
print "Transfer ownership ...".ljust(width),
json = primer1.nexus_assets_transfer_by_name("primer1-asset", "primer2")
if (json.has_key("result") and json["result"] != None):
    txid = json["result"]["txid"]
    address = json["result"]["address"]
    msg = "txid {}, address {}".format(txid, address)
#endif
api_print(json, msg)

sleep()

#
# Call API assets/history
#
print "Get history for Asset 'primer1-asset' ...".ljust(width),
json = primer1.nexus_assets_history_by_name("primer1-asset")
api_print(json)
if (json.has_key("result") and json["result"] != None):
    print "Ownership History:"
    for owner in json["result"]:
        print "owner {}:".format(owner["owner"])
        owner.pop("owner")
        print "  {}".format(owner)
    #endfor
#endif

print ""
print "---------- Tokens SDK/API ----------"

#
# Call API tokens/create
#
address = None
print "Create Token '{}', supply {} ...".format("primer1-token", 1000). \
    ljust(width),
json = primer1.nexus_tokens_create_token("primer1-token", 1, 1000)
if (json.has_key("result") and json["result"] != None):
    address = json["result"]["address"]
#endif
api_print(json, "address {}".format(address))

address = None
print "Create Token '{}', supply {} ...".format("primer2-token", 0). \
    ljust(width),
json = primer2.nexus_tokens_create_token("primer2-token", 2, 0)
if (json.has_key("result") and json["result"] != None):
    address = json["result"]["address"]
#endif
api_print(json, "address {}".format(address))

sleep()

#
# Call API tokens/get
#
msg = None
print "Get Token 'primer1-token' ...".ljust(width),
json = primer1.nexus_tokens_get_token_by_name("primer1-token")
if (json.has_key("result") and json["result"] != None):
    bal = json["result"]["balancee"]
    ms = json["result"]["maxsupply"]
    cs = json["result"]["currentsupply"]
    msg = "balance {}, max-supply {}, current-supply {}".format(bal, ms, cs)
#endif
api_print(json, msg)

sleep()

#
# Call API tokens/debit
#
msg = None
txid = None
print "Debit 'primer1-token' by 100 for primer2' ...".ljust(width),
json = primer1.nexus_tokens_debit_by_name("primer1-token", "primer2-token",
    100)
if (json.has_key("result") and json["result"] != None):
    amount = json["result"]["amount"]
    txid = json["result"]["txid"]
    msg = "txid {}, acount {}".format(txid, amount)
#endif
api_print(json, msg)

sleep()

#
# Call API tokens/credit
#
msg = None
print "Credit 100 to user 'primer2' ...".ljust(width),
json = primer2.nexus_tokens_credit_by_name("primer2-token", 100, txid, None)
if (json.has_key("result") and json["result"] != None):
    amount = json["result"]["amount"]
    txid = json["result"]["txid"]
    msg = "txid {}, acount {}".format(txid, amount)
#endif
api_print(json, msg)

print "All Done!"
exit(0)

#------------------------------------------------------------------------------
