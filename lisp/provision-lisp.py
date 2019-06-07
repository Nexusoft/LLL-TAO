#
# provision-lisp.py
#
# For a nexus Tritium node, provision a lisp.config file to be used by the
# lispers.net LISP subsystem. Use template file lisp.config.xtr to add EIDs
# to the configuration. Configure EIDs in the kernel set kernel routing tables
# for EID source selection.
#
# Usage: python provision-lisp.py <device> [<ipv4-eid>] [<ipv6-eid>]
#
# EIDs can be passed into this program for a seed node. If you need to get an
# allocation for an EID that doesn't conflict with other side nodes, use this
# script:
#
#     python lisp_allocate-seed-eid.py
#
# It is located in the repo LLL-TAO/lisp directory.
#

import sys
import os
import random
import commands
import time

#------------------------------------------------------------------------------

#
# allocate_eids
#
# Allocate random EIDs in the IPv4 240/8 range and fe00::/8 range. Right now
# we are provisioning for testnet only. It runs in LISP instance-ID 200.
#
def allocate_eids():
    rn = random.randint(0, 0xffffff)
    b2, b1, b0 = (rn >> 16) & 0xff, (rn >> 8) & 0xff, rn & 0xff
    eid4 = "240.{}.{}.{}".format(b2, b1, b0)
    eid6 = "fe00::{}:{}:{}".format(b2, b1, b0)
    return(eid4, eid6)
#enddef

#
# get_rloc
#
# Get IP address on interface. Various versions of ubumtu put gratitious
# whitespaces between keywords and hence the two types of greps below.
#
def get_rloc(device):
    addr = commands.getoutput('ip route | egrep "link src "'.format(device))
    if(addr == ""): 
        addr = commands.getoutput('ip route | egrep "link  src "'.format( \
            device))
        if(addr == ""): return(None)
    #endif
    addr = addr.split()[-1]
    return(addr)
#enddef

#
# get_eids
#
# Get EIDs from lisp.config file.
#
def get_eids(lisp_config):
    lines = lisp_config.split("\n")

    iid = eid4 = eid6 = None
    for line in lines:
        if(iid == None and line.find("instance-id = ") != -1):
            iid = line.split(" = ")[-1]
        #endif
        if(line.find("eid-prefix = ") != -1):
            addr = line.split(" = ")[-1]
            addr = addr.split("/")[0]
            if(eid4 == None and addr.count(".") == 3): eid4 = addr
            if(eid6 == None and addr.find(":") != -1): eid6 = addr
        #endif
        if(iid != None and eid4 != None and eid6 != None): break
    #endfor
    return(iid, eid4, eid6)
#enddef

#
# get_parms
#
# Get command-line parameters.
#
def get_parms():
    parm_len = len(sys.argv)
    if(parm_len == 1): return(None, None, None)

    device = sys.argv[1]
    if(parm_len == 2): return(device, None, None)

    eid4 = sys.argv[2]
    if(parm_len == 3): return(device, None, None)

    eid6 = sys.argv[3]
    return(device, eid4, eid6)
#enddef    

#------------------------------------------------------------------------------

#
# Get command-line parameters.
#
device, eid4, eid6 = get_parms()
if(device == None): exit(1)

#
# Check to see if file lisp.config exists. If not, provision new EIDs.
# Otherwise, grab EIDs from file.
#
lisp_config = "./lisp.config"
if(os.path.exists(lisp_config) == False): lisp_config += ".xtr"
f = open(lisp_config, "r"); lisp_config = f.read(); f.close()

#
# Determine if we have the template in lisp_config. If we do, we are
# provisioning.
#
provision = lisp_config.find("<iid>") != -1

if(provision):
    iid = "200"
    if(eid4 == None): eid4, eid6 = allocate_eids()
    rloc = get_rloc(device)

    print "Provisioning lisp.config file with EIDs [{}]{} & [{}]{}".format(iid,
        eid4, iid, eid6)
    lisp_config = lisp_config.replace("<iid>", iid)
    lisp_config = lisp_config.replace("<v4-eid>", eid4)
    lisp_config = lisp_config.replace("<v6-eid>", eid6)
    lisp_config = lisp_config.replace("<v4-rloc>", rloc)
    lisp_config = lisp_config.replace("<device>", device)
    f = open("./lisp.config", "w"); f.write(lisp_config); f.close()
else:
    iid, eid4, eid6 = get_eids(lisp_config)
    if(iid == None):
        print "lisp.config file corrupt, remove it and rerun script"
        exit(1)
    #endif
    print "Using EIDs [{}]{} & [{}]{} found in lisp.config".format(iid, eid4,
        iid, eid6)
#endif

#
# Configure IPv4 EIDs on interface lo and routes for EID source selection.
#
os.system("sudo ip addr add {}/32 dev lo 2> /dev/null".format(eid4))
os.system("sudo ip route del 240.0.0.0/8 2> /dev/null")
os.system("sudo ip route add 240.0.0.0/8 dev lo src {}".format(eid4))
os.system("sudo ip route del 224.0.0.0/4 2> /dev/null")
os.system("sudo ip route add 224.0.0.0/4 dev lo src {}".format(eid4))

#
# Set MTU so applications that run over LISP don't cause IP fragmentation.
#
os.system("sudo ifconfig {} mtu 1400".format(device))

#
# Tell kernel LISP wants to use IPv6 and configure IPv6 routing.
#
os.system("sysctl -w net.ipv6.conf.all.disable_ipv6=0 > /dev/null")
os.system("sysctl -w net.ipv6.conf.all.forwarding=1 > /dev/null")

#
# An IPv6 EID MUST not be assigned to interface lo or it will be unreachable.
#
os.system("sudo ip addr add {}/128 dev {} 2> /dev/null".format(eid6, device))
os.system(("sudo ip neighbor add fe80::1 lladdr 0:0:0:0:0:1 dev {} " + \
    "2> /dev/null").format(device))

#
# Need to sleep or the IPv6 routes below don't get added.
#
time.sleep(2)

#
# Setup IPv6 routes to do EID source selection.
#
os.system("sudo ip route del fd00::/8 2> /dev/null")
os.system("sudo ip route add fd00::/8 via fe80::1 dev {} src {}".format( \
    device, eid6))

os.system("sudo ip route del fe00::/8 2> /dev/null")
os.system("sudo ip route add fe00::/8 via fe80::1 dev {} src {}".format( \
    device, eid6))

os.system("sudo ip route del ff00::/8 2> /dev/null")
os.system("sudo ip route add ff00::/8 via fe80::1 dev {} src {}".format( \
    device, eid6))

exit(0)

#------------------------------------------------------------------------------

