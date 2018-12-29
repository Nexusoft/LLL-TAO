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

detail = "detail" in sys.argv

print "Login username sdk-test-user1 ...",
user1 = nexus_sdk.sdk_init("sdk-test-user1", "sdk-test", "1234")
print "{}".format("succeeded" if user1 != None else "failed")
if (user1 == None): exit(0)

print "Login username sdk-test-user2 ...",
user2 = nexus_sdk.sdk_init("sdk-test-user2", "sdk-test", "1234")
print "{}".format("succeeded" if user2 != None else "failed")
if (user2 == None): exit(0)

print "Show transactions for sdk-test-user1 ..."
json_data = user1.nexus_accounts_transactions()
print json_data

print "Show transactions for sdk-test-user2 ..."
json_data = user2.nexus_accounts_transactions()
print json_data

#------------------------------------------------------------------------------



