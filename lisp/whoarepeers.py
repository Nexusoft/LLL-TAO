#!/usr/bin/python
#
# whoarepeers.py
#
# Ask getpeerinfo to get peers, and if an EID peer, lig the EID so the
# LISP mapping system can return an rloc-name. This gives an operator a rough
# idea who owns and manages the nexus node.
#

import commands

peerinfo = "/nexus/nexus getpeerinfo | egrep addr"
lig = ('python -O /lispers.net/lisp-lig.pyo "[200]{}" to node1.nexusoft.io' + \
  "| egrep rloc-name")

peers = commands.getoutput(peerinfo)
if (peers == ""):
    print "No peers"
    exit(0)
#endif
peers = peers.split("\n")

for peer in peers:
    p = peer.split(":")[1]
    p = p.replace(' "', '')
    pp = (p + " ...").ljust(25)
    print "Found peer {}".format(pp),

    if (pp[0:4] == "240." or pp[0:4] == "fe::"):
        out = commands.getoutput(lig.format(p))
        out = out.split()
        print out[-1]
    else:
        print "underlay"
    #endif
#endfor


