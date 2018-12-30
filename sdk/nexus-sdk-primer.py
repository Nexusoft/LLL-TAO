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

width = 40

print "Initialize Nexus SDK ...".ljust(width),
primer = nexus_sdk.sdk_init("primer", "primer-password", "1234")
print "succeeded"

print "Create username 'primer' ...".ljust(width),
json = primer.nexus_accounts_create()
good = "failed" if json.has_key("error") else "succeeded"
print good
#print json

print "Login user 'primer' ...".ljust(width),
json = primer.nexus_accounts_login()
good = "failed" if json.has_key("error") else "succeeded"
print good
#print json

print "Show transactions for user 'primer' ...".ljust(width),
json= primer.nexus_accounts_transactions()
good = "failed" if json.has_key("error") else "succeeded"
print good
#print json

print "Logout user 'primer' ...".ljust(width),
json= primer.nexus_accounts_logout()
good = "failed" if json.has_key("error") else "succeeded"
print good
#print json

print "All Done!"
exit(0)

#------------------------------------------------------------------------------
