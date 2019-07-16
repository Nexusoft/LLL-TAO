#!/usr/bin/env python
#
# scale-test.py - Load a bunch of supply-chain items to the blockchain
#
# A quick program to load-up the blockchain. Used with qrsc to look at
# transaction history. Put data in JSON so qrsc can read it.
#
# Usage: python scale-test.py [<number-of-items>]
#
#------------------------------------------------------------------------------

import sys
import commands
import time
import json
import socket

try:
    sdk_dir = os.getcwd().split("/")[0:-2]
    sdk_dir = "/".join(sdk_dir) + "/sdk"
    sys.path.append(sdk_dir)
except:
    print "Need to place nexus_sdk.py in this directory"
    exit(0)
#endtry

entries = 100 if len(sys.argv) < 2 else int(sys.argv[1])
sdk = nexus.sdk_init("dino", "pw", "1234")

print "Login with username dino ..."
status = sdk.nexus_accounts_login()
if (status.has_key("error")):
    print "accounts/login failed"
    exit(1)
#endif

#
# Have document-URL be the getinfo display from the nexus daemon.
#
host = socket.gethostname()
host2port = { "ups-seed1" : (8081, 9091), "ups-seed2" : (8082, 9092),
    "ups-qrsc-server1" : (8083, 9093), "ups-qrsc-server2" : (8084, 9094) }

if (host2port.has_key(host)):
    url_odd = "http://localhost:{}/system/get/info".format(host2port[host][0])
    url_even = "http://localhost:{}/lisp".format(host2port[host][1])
else:
    url_odd = "?"
    url_even = "?"
#endif

#
# This is important so qrsc can display the contents of the item.
#
state = {}
state["creation-date"] = commands.getoutput("date")
state["description"] = "created by scale-test"
state["document-checksum"] = "?"
state["last-checksum"] = None
state["checksum-date"] = None
state["action"] = "create-item"

print "Create {} supply-chain items ...".format(entries)
for i in range(0,entries):
    state["serial-number"] = str(i + 1)
    state["document-url"] = url_even if (i & 1) == 0 else url_odd
    status = sdk.nexus_supply_createitem(json.dumps(state))
    if (status.has_key("error")):
        print "supply/createitem failed for {}th item".format(i+1)
        print "reason: {}".format(status["error"]["message"])
        exit(1)
    #endif
    print status["result"]["address"]
    time.sleep(2)
#endfor

print "Done"
exit(0)
    
#------------------------------------------------------------------------------
