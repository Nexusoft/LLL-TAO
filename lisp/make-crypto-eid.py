#!/usr/bin/env python
#
# make-crypto-eid.py
#
# This program creates a IPv6 crypto-EID. It performs the following functions:
#
# (1) Generates a key-pair. Writes private key to lisp-sig.pem in current
#     directory.
# (2) Creates a file named "lisp.config.include" in the current directory that
#     contains two "lisp database-mapping" commands. One for the crypto-EID
#     and one for the hash->pubkey. It also contains two JSON command clauses
#     for the crypto-EID signature and for the public-key encoding.
#
# Usage: python make-crypto-eid.py <iid> <hash-prefix>
#
# Both input parameters are required. <iid> is a number between 0 and 2^32-1
# and <hash-prefix> is an IPv6 prefix in slash format. An example would be:
#
#     python make-crypto-eid.py 1000 fd::/8
#
# The map-server is required to have the following command to accept Map-
# Register messages for the above generated crypto-EID:
#
# lisp eid-crypto-hash {
#     instance-id = *
#     eid-prefix = fd00::/8   
# }
#
# The complex part of this code to do key generation is from make-eid-hash.py
# and interoperates with the lispers.net implementation of draft-farinacci-
# lisp-ecdsa-auth-02.txt.
#
#------------------------------------------------------------------------------

import ecdsa
import sys
import hashlib
from binascii import b2a_base64 as b2a

#------------------------------------------------------------------------------

def bold(string):
    return("\033[1m" + string + "\033[0m")
#enddef

#------------------------------------------------------------------------------

#
# Get parameters.
#
if(len(sys.argv) != 3):
    print "Usage: python make-crypto-eid.py <iid> <hash-prefix>"
    exit(1)
#endif

iid = sys.argv[1]
if(iid.isdigit() == False):
    print "Instance-ID must be between 0 and 0xffffffff"
    exit(1)
#endif
iid = int(iid)
    
eid_prefix = sys.argv[2]
if(eid_prefix.find("/") == -1 or eid_prefix.count(":") == 0):
    print "EID-prefix must be an IPv6 address in slash format"
    exit(1)
#endif
eid_prefix, mask_len = eid_prefix.split("/")
mask_len = int(mask_len)
if((mask_len % 4) != 0):
    print "Mask-length must be a multiple of 4"
    exit(1)
#endif

#
# Generate key-pair.
#
print "Generating key-pair ...",
key = ecdsa.SigningKey.generate(curve=ecdsa.NIST256p)
pubkey = key.get_verifying_key().to_der()
print ""

#
# Build EID. The hash is <4-byte-iid><variable-length-prefix><pubkey>
#
eid_prefix = eid_prefix.replace(":", "")
eid_prefix = int(eid_prefix, 16)

hiid = hex(iid)[2::].zfill(8)
heid = hex(eid_prefix)[2::]

#
# Hash sum the 3-tuple. Put heid in IPv6 standard format.
#
hash_data = hiid + heid + pubkey
hash_value = hashlib.sha256(hash_data).hexdigest()

nibbles = (128 - mask_len) / 4
hv = hash_value[0:nibbles]
hash_value = int(hv, 16)

eid = (eid_prefix << (128-mask_len)) + hash_value
eid = hex(eid)[2:-1]
eid = eid[0:4] + ":" + eid[4:8] + ":" + eid[8:12] + ":" + eid[12:16] + ":" + \
    eid[16:20] + ":" + eid[20:24] + ":" + eid[24:28] + ":" + eid[28:32]
eid = eid.replace(":000", ":")
eid = eid.replace(":00", ":")
eid = eid.replace(":0", ":")

#
# Now do signature. Make sure sig_data is hashed with sha256. This is required
# for Go to Python interoperability. The lispers.net verifier needs to assume
# this for both signers. Note to use sha256, you need curve NIST256p.
#
sig_data = "[{}]{}".format(iid, eid)
sig = key.sign(sig_data, hashfunc=hashlib.sha256)
print "Generated crypto-EID {}".format(bold(sig_data))

#
# Put crypto-EID in PEM file as a comment header.
#
f = open("./lisp-sig.pem", "w");
pem_header = \
'''
#
# lispers.net crypto-EID: {}
#
'''.format(sig_data)

f.write(pem_header)
f.write(key.to_pem())
f.write(key.get_verifying_key().to_pem())
f.close()
print "Keys stored in file {}".format(bold("lisp-sig.pem"))

#
# Return values in base64 format
#
pubkey = b2a(key.get_verifying_key().to_pem())
sig = b2a(sig)

hvv = hv
if((len(hvv) % 4) == 0):
    hv = []
else:
    hv = [ hvv[0:2] ]
    hvv = hvv[2::]
#endif
for i in range(0, len(hvv), 4): 
    hv.append(hvv[i:i+4])
#endfor

hv = ":".join(hv)
ev = eid[-4::]
pubkey = pubkey.replace("\n", "")
sig = sig.replace("\n", "")

commands = '''
lisp json {
    json-name = pubkey-<ev>
    json-string = { "public-key" : "<pubkey>" }
}
lisp database-mapping {
    prefix {
        instance-id = <iid>
        eid-prefix = 'hash-<hv>'
    }
    rloc {
        json-name = pubkey-<ev>
        priority = 255
    }
}
lisp json {
    json-name = signature-<ev>
    json-string = { "signature-eid" : "[<iid>]<eid>", "signature" : "<sig>" }
}
lisp database-mapping {
    prefix {
        instance-id = <iid>
        eid-prefix = <eid>/128
        signature-eid = yes
    }
    rloc {
        interface = eth0
    }
    rloc {
        json-name = signature-<ev>
        priority = 255
    }
}
'''
commands = commands.replace("<ev>", ev)
commands = commands.replace("<hv>", hv)
commands = commands.replace("<iid>", str(iid))
commands = commands.replace("<eid>", eid)
commands = commands.replace("<pubkey>", pubkey)
commands = commands.replace("<sig>", sig)

f = open("./lisp.config.include", "w"); f.write(commands); f.close()

print "lispers.net commands added to file {}".format(
    bold("lisp.config.include"))
exit(0)
