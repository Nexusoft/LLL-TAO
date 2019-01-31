#!/usr/bin/env python
#
# lisp-allocate-seed-eid.py
#
# Use this script to allocate a random EID in the 240.0.x.x/16 and fe::x:x/8
# address space.
#
# This script should NOT be used for client nodes.
#
# Usage: python lisp-allocate-seed-eid.py
#
#------------------------------------------------------------------------------

import random
import sys
import socket

dns_seeds = [
    "test1.nexusoft.io",
    "test1.mercuryminer.com",
    "test2.mercuryminer.com",
    "test3.mercuryminer.com",
    "test4.mercuryminer.com",
    "test5.mercuryminer.com",
    "nexus-lisp-seed.lispers.net"
]    

def allocate():
    rand = random.randint(0, 0xffff)
    byte1 = (rand >> 8) & 0xff
    byte0 = rand & 0xff
    v4_eid = "240.0.{}.{}".format(byte1, byte0)
    v6_eid = "fe::{}:{}".format(byte1, byte0)
    print "Allocate EIDs {}, {}".format(v4_eid, v6_eid)
    return(v4_eid, v6_eid)
#enddef

def bold(string):
    return("\033[1m" + string + "\033[0m")
#enddef

#
# Check for duplicates in already allocated DNS seeds.
#
for i in range(5):
    v4, v6 = allocate()
    print "Checking for duplicates ... "
    for seed in dns_seeds:
        print "Lookup {}:".format(seed).ljust(40),
        addr = socket.gethostbyname(seed)
        print addr
        if (addr == v4 or addr == v6):
            print "Found duplicate"
            v4 = None
            break
        #endif
    #endfor
    if (v4 != None): break
#endfor
print ""

if (v4 == None):
    print "Retry limit exceeded, run script again"
    exit(1)
#endif
print "You can use IPv4 EID {} and IPv6 EID {}".format(bold(v4), bold(v6))
exit(0)
