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

#------------------------------------------------------------------------------

def call_accounts_api(label, function):
    print "{} ...".format(label),
    json_data = function()

    bad = json_data.has_key("error")
    if (bad):
        error = json_data["error"]
        api_or_sdk = "SDK" if error.has_key("sdk-error") else "API"
        msg = error["message"]
        print "{} failed - {}".format(api_or_sdk, msg)
    else:
        print "succeeded"
    #endif
    if (detail): print "  {}".format(json_data)
#enddef

#------------------------------------------------------------------------------

detail = "detail" in sys.argv

print "Initialize SDK for username sdk-test-user1 ...",
user1 = nexus_sdk.sdk_init("sdk-test-user1", "sdk-test", "1234")
print "done"
print "Initialize SDK for username sdk-test-user2 ...",
user2 = nexus_sdk.sdk_init("sdk-test-user2", "sdk-test", "1234")
print "done"
print ""

#
# Make calls to accounts API for both users.
#
call_accounts_api("Create user account sdk-test-user1",
                  user1.nexus_accounts_create)
call_accounts_api("Create user account sdk-test-user2",
                  user2.nexus_accounts_create)

call_accounts_api("Login user sdk-test-user1", user1.nexus_accounts_login)
call_accounts_api("Login user sdk-test-user2", user2.nexus_accounts_login)

call_accounts_api("Transactions for sdk-test-user1",
                  user1.nexus_accounts_transactions)
call_accounts_api("Transactions for sdk-test-user2",
                  user2.nexus_accounts_transactions)

#
# Make calls to supply API for both users.
#

exit(0)

#------------------------------------------------------------------------------



