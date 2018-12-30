#
# nexus-sdk-primer.py
#
# A very simple sequential program for interacting with the Nexus Tritium
# API. If you want more details and want to see more interactions and
# return response detail, use LLL-TAO/sdk/nexus-sdk-test.py.
#
# Usage: python nexus-sdk-primer.py
#
#------------------------------------------------------------------------------

import nexus_sdk

print "Initialize Nexus SDK ...".ljust(40),
primer = nexus_sdk.sdk_init("primer", "primer", "1234")
print "succeeded"

print "Create username 'primer' ...".ljust(40),
json = primer.nexus_accounts_create()
good = "failed" if json.has_key("error") else "succeeded"
print good
#print json

print "Login user with username 'primer' ...".ljust(40),
json = primer.nexus_accounts_login()
good = "failed" if json.has_key("error") else "succeeded"
print good
#print json

print "Show transactions for user 'primer' ...".ljust(40),
json= primer.nexus_accounts_transactions()
good = "failed" if json.has_key("error") else "succeeded"
print good
#print json

print "Logout user 'primer' ...".ljust(40),
json= primer.nexus_accounts_logout()
good = "failed" if json.has_key("error") else "succeeded"
print good
#print json

print "All Done!"
exit(0)

#------------------------------------------------------------------------------
