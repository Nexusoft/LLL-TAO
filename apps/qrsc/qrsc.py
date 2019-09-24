#!/usr/bin/env python
#
# qrsc.py - QR-code Supply Chain Application
#
# This program implements a web-server, creates QR-codes, and stores and
# retrieves from the Nexus Tritium ledger using the Tritium APIs.
#
# Usage: python qrsc.py [<port>]
#
# This program has two dependencies:
#
# apt-get install python-qrtools
#
# and the nexus_sdk.py SDK library that is located in LLL-TAO/sdk/nexus_sdk.py
# that should be copied into the directory qrsc runs in.
#
# The ./qrcodes directory contain QR-code images for genesis-IDs and register
# addresses. They are mapped to filenames from qrcodes{}.
# operate:
#
#------------------------------------------------------------------------------

import commands
import os
import bottle
import json
import random
import hashlib
import urllib2
import sys
import time
import socket

try:
    sdk_dir = os.getcwd().split("/")[0:-2]
    sdk_dir = "/".join(sdk_dir) + "/sdk"
    sys.path.append(sdk_dir)
    import nexus_sdk as nexus
except:
    print "Need to place nexus_sdk.py in this directory"
    exit(0)
#endtry    
try:
    from qrtools import QR
except:
    print 'Need to run "apt-get install python-qrtools" on linux'
    exit(0)
#endtry

#------------------------------------------------------------------------------

#
# Indexed by nexus session-id and returns class from nexus_sdk.sdk_init().
# Allows session continuity across web pages.
#
contexts = {}

#
# This URL cache is a way to store URLs between web pages. We are doing this
# because it is difficult to put a URL inside another URL. Indexed by random
# number which is passed to the /checksum/<address>/<url_index> method.
#
url_cache = {}

#
# Did command line have a port number to listen to?
#
qrsc_port = int(sys.argv[1]) if (len(sys.argv) == 2) else 2222

#
# Need some delays so block verification can happen in the daemon.
#
def sleep():
    time.sleep(.5)
#enddef    

#------------------------------------------------------------------------------

#
# qrcodes
#
# For display QR-code png files.
#
@bottle.route('/qrcodes/<filename>', method="get")
def qrcodes_filename(filename):
    return(bottle.static_file(filename, root="./qrcodes"))
#enddef

#
# transfer_ownership
#
# Check the next record in a item history array to see if it is a different
# owner than the current record.
#
def transfer_ownership(history, tx):
    if (history[-1] == tx): return(False)

    index = history.index(tx) + 1
    next_tx= history[index]
    return(tx["owner"] != next_tx["owner"])
#enddef

#
# show_item
#
# Based an a regisger address, get the item in the blockchain, get its QR-code,
# display the item with its transaction history.
#
def show_item(sid, api, address, state):
    f = get_qrcode_filename(address, "item")
    if (f == None): f = allocate_qrcode(address, "item")

    serial = state["serial-number"]
    sn = print_blue(serial)
    url = state["document-url"]
    url = '<a href="{}">{}</a>'.format(url, print_cour(url))
    owner = print_red("?")

    status = api.nexus_supply_list_item_history_by_address(address)
    if (status.has_key("error")):
        history = []
    else:
        history = status["result"]
        if (history == None): history = []
    #endif

    if (history == []):
        output = "History not available for unclaimed item<br><br>"
    else:
        output = ""
        owner = history[0]["owner"]
        owner = print_green(owner) if owner == api.genesis_id else owner
    #endif

    output = show_item_html_header.format(sid, f, address, owner, sn,
        state["description"], url, print_checksum(sid, address, state),
        state["creation-date"], address) + output

    for tx in history:
        if (tx.has_key("data") == False):
            output += show_item_html_row.format("No Data Returned:", tx)
            output += show_item_html_row.format("&nbsp;", "&nbsp;")
            continue
        #endif
        claim = (tx["type"] == "CLAIM")

        state = json.loads(tx["data"])

        sn = print_blue(state["serial-number"])
        output += show_item_html_row.format("Serial Number:", sn)

        description = state["description"].replace("%26", "&")
        output += show_item_html_row.format("Description:", description)

        url = print_cour(state["document-url"])
        output += show_item_html_row.format("Document URL:", url)

        checksum = state["document-checksum"]
        output += show_item_html_row.format("Document Checksum:", checksum)

        output += show_item_html_row.format("Owner Genesis-ID:", tx["owner"])

        #
        # Do not show action if the ownership changed, display action
        # transfer-ownership.
        #
        action = state["action"] if state.has_key("action") else "?"
        if (transfer_ownership(history, tx)): action = "transfer-ownership"
        if (claim): action = "claim-ownership"
        action = "<i>{}</i>".format(action)
        output += show_item_html_row.format("Action:", action)

        output += show_item_html_row.format("Date:", state["creation-date"])
        output += show_item_html_row.format("&nbsp;", "&nbsp;")
    #endfor        
    
    output += show_item_html_footer.format(sid, sid, address, sid, address,
        serial, sid, address, sid, sid)
    return(output)
#enddef    

#
# get_session_id
#
# Check to see if the session-id is cached for a user. A 2-tuple (api, output)
# is returned. One or the other will be None to check for success or failure.
#
def get_session_id(sid):
    global contexts

    if (contexts.has_key(sid) == False):
        msg = "Login session {} does not exist".format(sid)
        return(None, show_html.format(bold(msg)))
    #endif
    api = contexts[sid]
    return(api, "")
#enddef    

#
# allocate_qrcode
#
# Allocate QR-code, assign it input tag and write it to disk. Returns basename
# of png file.
#
def allocate_qrcode(tag, prefix):
    qr = QR(data=tag, pixel_size = 4) 
    qr.encode() 

    path = qr.filename
    f = get_qrcode_filename(tag, prefix)
    os.system("mkdir -p qrcodes; cp {} qrcodes/{}".format(path, f))
    return(f)
#enddef    

#
# get_qrcode_filename
#
# Allocate QR-code, assign it input tag and write it to disk. Returns basename
# of png file. We may not have the png file cached if the qrcode was created
# on another nexus node.
#
def get_qrcode_filename(tag, prefix):
    filename = prefix + "-" + tag + ".png"
    if (os.path.exists("qrcodes/{}".format(filename)) == False): return(None)

    return(filename)
#enddef    

#
# create_register_state
#
# Create JSON object to store as a register value.
#
def create_register_state(sn, description, url, action):
    value = {}
    value["creation-date"] = commands.getoutput("date")
    value["serial-number"] = sn
    value["description"] = description
    value["document-url"] = url
    value["document-checksum"] = "?"
    value["last-checksum"] = None
    value["checksum-date"] = None
    value["action"] = action

    #
    # Create values for checksum entries.
    #
    checksum_file(url, value)
    return(value)
#enddef

#
# print_checksum
#
# Print the display line for a document checksum.
#
def print_checksum(sid, address, state):
    global url_cache
    
    url = state["document-url"]
    checksum = state["document-checksum"]
    date = state["checksum-date"]

    url_index = str(random.randint(0, 0xffffffff))
    url_cache[url_index] = url
    good = state["last-checksum"] == state["document-checksum"]
    good = print_green(" good") if good else print_red(" bad")
    if (date == None):
        date = print_red("never")
        good = ""
    #endif

    line = "{}, last verified{}: {},&nbsp;".format(checksum, good, date)
    line += '<a href="/checksum/{}/{}/{}">verify-now</a>'.format(sid, address,
        url_index)
    return(line)
#enddef

def print_cour(string):
    output = '<font face="Courier New">{}</font>'.format(string)
    return(output)
#enddef

def print_green(string):
    output = '<font color="green">{}</font>'.format(string)
    return(output)
#enddef
def print_red(string):
    output = '<font color="red">{}</font>'.format(string)
    return(output)
#enddef
def print_blue(string):
    output = '<font color="blue">{}</font>'.format(string)
    return(output)
#enddef
def bold(string):                                                              
    string = string.replace("[1m", "<b>")                                      
    string = string.replace("[0m", "</b>")                                     
    return(string)                                                             
#enddef

#
# checksum_file
#
# Read data at URL and checksum.
#
def checksum_file(url, state):
    try:
        u = urllib2.urlopen(url)
    except:
        return("?")
    #endtry

    m = hashlib.sha256()
    while (True):
        buf = u.read(8192)
        if (buf == ""): break
        m.update(buf)
    #endwhile
    checksum = m.hexdigest()

    old_checksum = state["document-checksum"]
    state["document-checksum"] = checksum

    if (state["checksum-date"] == None): state["last-checksum"] = checksum
    state["checksum-date"] = commands.getoutput("date")

    return(old_checksum != checksum)
#enddef

#
# get_sigchain_length
#
# Get chain height for this username.
#
def get_sigchain_length(api):
    length = "?"
    status = api.nexus_users_list_transactions_by_username(0, -1)
    if (status.has_key("result")): length = len(status["result"])
    return(length)
#enddef

claims_html = '''
    <tr><td style="word-break:break-all;">item: {}</td></tr>
    <tr><td style="word-break:break-all;">txid: {}&nbsp;&nbsp;</td>
    <td><a href="/claim-item/{}/{}/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Claim</button></a></td>
    </tr>
    <tr><td>&nbsp;</td></tr>
'''

#
# check_notifications
#
# Check user's notifications to see if there has been any item transfers that
# can be claimed. Print out only the transfers that have not been claimed
# by examining the transaction history for OP=CLAIM transactions.
#
def check_notifications(api, sid):
    status = api.nexus_users_list_notifications_by_username(0, -1)
    if (status.has_key("error")): return("")

    #
    # Load all transfer notifications. If we find them already claimed in
    # the list of transactions, we will remove them.
    #
    transfers = []
    for entry in status["result"]:
        if (entry["OP"] != "TRANSFER"): continue
        transfers.append({ "txid" : entry["txid"], "address" :
            entry["address"] })
    #endfor
    if (transfers == []): return("")

    status = api.nexus_users_list_transactions_by_username(0, -1)
    if (status.has_key("error")): return("")

    output = ""
    for entry in transfers:
        txid = entry["txid"]
        address = entry["address"]
        found = False
        for contracts in status["result"]:
            if (contracts.has_key("contracts") == False): continue
            for contract in contracts["contracts"]:
                if (contract["OP"] != "TRANSFER"): continue
                if (contract["txid"] != txid): continue
                if (contract["address"] != address): continue
                found = True
                break
            #endfor
            if (found): break
        #endfor
        if (found == False):
            output += claims_html.format(address, txid, sid, address, txid)
        #endif
    #endfor
    if (output == ""): return(output)

    #
    # Now format output for unclaimed transfers.
    #
    header = ("<br><b>Unclaimed Item Ownership Transfers" + \
        "</b><br><br> <table>")
    footer = '</table><hr size="5"'
    output = header + output + footer
    return(output)
#enddef

#------------------------------------------------------------------------------

landing_page_html = '''
    <html><title>Nexus Supply-Chain Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
    border-radius:20px;border:5px solid #666666;">

    <font face="verdana">
    <center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Supply-Chain Application</b></a></head><br><br><hr size="5">

    <br><form action="/login" method="post">
    <table align="center" border="0">
    <tr>
      <td><i>Enter Username:</i></td>
      <td><input type="text" name="username" size="40" /></td>
    </tr>
    <tr>
      <td><i>Enter Password:</i></td>
      <td><input type="text" name="password" size="40" /></td>
    </tr>
    <tr>
      <td><i>Enter PIN:</i></td>
      <td><input type="text" name="pin" size="40" /></td>
    </tr>
    </table>
    <br>
    <input type="submit" value="Login User Account" name="action"
           style="background-color:transparent;border-radius:10px;" />
    <input type="submit" value="Create User Account" name="action"
           style="background-color:transparent;border-radius:10px;" />
    </form>
    </font>
    <hr size="5">
    <font size="2">Served by {}<br></font>
    </center></body></html>
'''
show_html = '''
    <html><title>Nexus Supply-Chain Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
    border-radius:20px;border:5px solid #666666;">

    <font face="verdana">
    <center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Supply-Chain Application</b></a></head><br><br><hr size="5">
    {}
    </font>
    <hr size="5"></center></body></html>
'''

#
# landing_page
#
# Output landing page to web browser client.
#
@bottle.route('/')
def landing_page():
    host = print_blue(socket.gethostname())
    return(landing_page_html.format(host))
#enddef

login_html = '''
    <html><title>Nexus Supply-Chain Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
    border-radius:20px;border:5px solid #666666;">

    <font face="verdana">
    <center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Supply-Chain Application</b></a></head><br><br><hr size="5">

    <table align="center" border="0"">
    <tr>
      <td><img src="/qrcodes/{}"></td>
      <td><table align="center" border="0">
        <tr><td>Username:</td><td>{}</td>
        <tr><td>Genesis-ID:</td>
        <td style="word-break:break-all;">{}</td>
        <tr><td>Sigchain Entries:</td>
        <td style="word-break:break-all;">{}</td>
      </table></td>
    </tr>
    </table>
    <hr size="5">

    {}

    <a href="/create-item/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Create Item QR-code</button></a>
    <a href="/lookup-item/{}/0">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Lookup Item</button></a>
    <a href="/modify-item/{}/0/0">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Modify Item</button></a>
    <a href="/transfer-item/{}/0">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Transfer Item Ownership</button></a>
    <a href="/claim-item/{}/0/0">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Claim Item Ownership</button></a>

    <br><br>

    <a href="/account-history/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Show User Account Transaction History</button></a>

    </font>
    <hr size="5"></center></body></html>
'''

#
# login
#
# Login or Create account.
#
@bottle.post('/login')
def login():
    global contexts

    user = bottle.request.forms.get("username")
    pw = bottle.request.forms.get("password")
    pin = bottle.request.forms.get("pin")
    action = bottle.request.forms.get("action")
    create_account = (action == "Create User Account")

    if (user == ""):
        msg = "{} failed for empty username".format(action)
        return(show_html.format(bold(msg)))
    #endif

    api = nexus.sdk_init(user, pw, pin)
    if (create_account):
        status = api.nexus_users_create_user()
        if (status.has_key("error")):
            reason = status["error"]["message"]
            msg = "User account creation failed for username '{}', {}". \
                format(user, reason)
            return(show_html.format(bold(msg)))
        #endif
    #endif

    sleep()

    status = api.nexus_users_login_user()
    if (status.has_key("error")):
        reason = status["error"]["message"]
        msg = "Login failed for username '{}', {}".format(user, reason)
        return(show_html.format(bold(msg)))
    #endif

    genesis = status["result"]["genesis"]
    sid = status["result"]["session"]
    contexts[sid] = api

    f = allocate_qrcode(genesis, "genid")
    sl = get_sigchain_length(api)

    claims = check_notifications(api, sid)
    
    output = login_html.format(f, user, genesis, sl, claims, sid, sid, sid,
        sid, sid, sid)
    return(output)
#enddef

#
# logged-in
#
# Display page with genesis-ID QR-code.
#
@bottle.get('/logged-in/<sid>')
def logged_in(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    genesis = api.genesis_id
    user = api.username
    f = get_qrcode_filename(genesis, "genid")
    if (f == None): f = allocate_qrcode(genesis, "genid")

    sl = get_sigchain_length(api)

    claims = check_notifications(api, sid)
    
    output = login_html.format(f, user, genesis, sl, claims, sid, sid, sid,
        sid, sid, sid)
    return(output)
#enddef

account_history_html_header = '''
    <html><title>Nexus Supply-Chain Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
    border-radius:20px;border:5px solid #666666;">

    <font face="verdana">
    <center>
    <br><head><a href="/logged-in/{}" style="text-decoration:none;"><font 
                 color="black">
    <b>Nexus Supply-Chain Application</b></a></head><br><br><hr size="5">

    <table align="center" border="0" style="word-break:break-all;">
    <tr><td>Transaction History for User <b>{}</b> Genesis-ID <b>{}</b>
    <br><br></td></tr>
    </table>
    <table align="center" border="0">
'''
account_history_html_footer = '''
    </table>
    </font>
    <hr size="5"></center></body></html>
'''
account_history_html_row = '''
    <tr><td>{}</td>
    <td style="word-break:break-all;">{}</td>
'''

#
# display_item_for_history
#
# Get item from transaction history.
#
def display_item_for_history(sid, api, address, output):
    status = api.nexus_supply_get_item_by_address(address)
    if (status.has_key("error")): return(output)

    try:
        state = json.loads(status["result"]["data"])
        sn = state["serial-number"]
        sn = '<a href="/display-item/{}/{}">{}</a>'.format(sid, address, sn)
        output += account_history_html_row.format("item serial number:", sn)
    except:
        return(output)
    #endtry

    return(output)
#enddef

@bottle.route('/account-history/<sid>', method="get")
def account_history(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)
    
    status = api.nexus_users_list_transactions_by_username(0, -1)

    output = account_history_html_header.format(sid, api.username,
        api.genesis_id)
    for tx in status["result"]:
        if (tx.has_key("txid")):
            txid = tx["txid"]
            output += account_history_html_row.format("txid:", txid)
        #endif
        if (tx.has_key("contracts") == False or tx["contracts"] == None):
            continue
        #endif

        for contract in tx["contracts"]:
            op = contract["OP"]
            op = "OP: {}&nbsp;&nbsp;".format(op)
            address = contract["address"]
            address = "address: {}".format(address)
            output += account_history_html_row.format(op, address)
            output = display_item_for_history(sid, api, address, output)
        #endfor

        output += account_history_html_row.format("&nbsp;", "&nbsp;")
    #endfor
        
    output += account_history_html_footer
    return(output)
#enddef

create_item_html = '''
    <html><title>Nexus Supply-Chain Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
    border-radius:20px;border:5px solid #666666;">

    <font face="verdana">
    <center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Supply-Chain Application</b></a></head><br><br><hr size="5">

    <br><form action="/do-create-item/{}" method="post">
    <table align="center" border="0">
    <tr>
      <td><i>Enter Item Serial Number:</i></td>
      <td><input type="text" name="serial" size="70" /></td>
    </tr>
    <tr>
      <td><i>Enter Item Description:</i></td>
      <td><input type="text" name="description" size="70" /></td>
    </tr>
    <tr>
      <td><i>Enter Document URL:</i></td>
      <td><input type="text" name="url" size="70" /></td>
    </tr>
    </table>
    <br>
    <input type="submit" value="Create QR-code" 
           style="background-color:transparent;border-radius:10px;" />
    </form>
    </font>
    <hr size="5"></center></body></html>
'''
lookup_item_html = '''
    <html><title>Nexus Supply-Chain Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
    border-radius:20px;border:5px solid #666666;">

    <font face="verdana">
    <center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Supply-Chain Application</b></a></head><br><br><hr size="5">

    <br><form action="/do-lookup-item/{}" method="post">
    <table align="center" border="0">
    <tr>
      <td><i>Enter Item Address:</i></td>
      <td><input type="text" name="address" value="{}" size="70" /></td>
    </tr>
    </table>
    <br>
    <input type="submit" value="Lookup Item" 
           style="background-color:transparent;border-radius:10px;" />
    </form>
    </font>
    <hr size="5"></center></body></html>
'''
show_item_html_header = '''
    <html><title>Nexus Supply-Chain Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
    border-radius:20px;border:5px solid #666666;">


    <font face="verdana">
    <center>
    <br><head><a href="/logged-in/{}" style="text-decoration:none;"><font 
                 color="black">
    <b>Nexus Supply-Chain Application</b></a></head><br><br><hr size="5">

    <table align="center" border="0">
    <tr>
      <td><img src="/qrcodes/{}"></td>
      <td><table align="center" border="0">
        <tr><td>Item Address:</td>
        <td style="word-break:break-all;">{}</td>
        <tr><td>Owner Genesis-ID:</td>
        <td style="word-break:break-all;">{}</td>
        <tr><td>Item Serial Number:</td><td>{}</td>
        <tr><td>Item Description:</td><td>{}</td>
        <tr><td>Document URL:</td><td>{}</td>
        <tr><td>Document Checksum:</td><td>{}</td>
        <tr><td>Creation Date:</td><td>{}</td>
      </table></td>
    </tr>
    </table>
    <hr size="5">
    <table align="center" border="0" style="word-break:break-all;">
    <tr><td>Item History for Address <b>{}</b><br><br></td></tr>
    </table>
    <br>
    <table align="center" border="0">
'''
show_item_html_footer = '''
    </table>
    </font>
    <hr size="5">
    <a href="/create-item/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Create Item QR-code</button></a>
    <a href="/lookup-item/{}/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Lookup Item</button></a>
    <a href="/modify-item/{}/{}/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Modify Item</button></a>
    <a href="/transfer-item/{}/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Transfer Item Ownership</button></a>
    <a href="/claim-item/{}/0/0">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Claim Item Ownership</button></a>

    <br><br>

    <a href="/account-history/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Show User Account Transaction History</button></a>

    <hr size="5"></center></body></html>
'''
show_item_html_row = '''
    <tr><td>{}</td>
    <td style="word-break:break-all;">{}</td>
'''

@bottle.route('/create-item/<sid>', method="get")
def create_item(sid):
    return(create_item_html.format(sid))
#enddef

@bottle.route('/do-create-item/<sid>', method="post")
def do_create_item(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    sn = bottle.request.forms.get("serial")
    description = bottle.request.forms.get("description")
    url = bottle.request.forms.get("url")
    state = create_register_state(sn, description, url, "create-item")
  
    #
    # Do an API supply/createitem.
    #
    status = api.nexus_supply_create_item(sn, json.dumps(state))
    if (status.has_key("error")):
        msg = "Item creation failed"
        return(show_html.format(bold(msg)))
    #endif

    #
    # Get address from createitem and build QR-code filename.
    #
    address = status["result"]["address"]
    allocate_qrcode(address, "item")

    #
    # Get item from blockchain and display for user.
    #
    for i in range(0,8):
        sleep()
        status = api.nexus_supply_get_item_by_address(address)
        if (status.has_key("error") == False): break
    #endfor
    if (status.has_key("error")):
        msg = "Item lookup failed for address '{}' but item created".format( \
            address)
        return(show_html.format(bold(msg)))
    #endif
    state = json.loads(status["result"]["data"])

    output = show_item(sid, api, address, state)
    return(output)
#enddef

@bottle.route('/lookup-item/<sid>/<address>', method="get")
def lookup_item(sid, address):
    if (address == "0"): address = ""
    return(lookup_item_html.format(sid, address))
#enddef

@bottle.route('/do-lookup-item/<sid>', method="post")
def do_lookup_item(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    address = bottle.request.forms.get("address")

    #
    # Do an API supply/getitem.
    #
    status = api.nexus_supply_get_item_by_address(address)
    if (status.has_key("error")):
        msg = "Item lookup failed for address '{}'".format(address)
        return(show_html.format(bold(msg)))
    #endif

    state = json.loads(status["result"]["data"])
    output = show_item(sid, api, address, state)
    return(output)
#enddef

modify_item_html = '''
    <html><title>Nexus Supply-Chain Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
    border-radius:20px;border:5px solid #666666;">

    <font face="verdana">
    <center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Supply-Chain Application</b></a></head><br><br><hr size="5">

    <br><form action="/do-modify-item/{}" method="post">
    <table align="center" border="0">
    <tr>
      <td><i>Enter Item Address:</i></td>
      <td><input type="text" name="address" value="{}" size="70" /></td>
    </tr>
    <tr>
      <td><i>Enter Item Serial Number:</i></td>
      <td><input type="text" name="serial" value="{}" size="70" /></td>
    </tr>
    <tr>
      <td><i>Enter New Item Description:</i></td>
      <td><input type="text" name="description" size="70" /></td>
    </tr>
    <tr>
      <td><i>Enter Document URL:</i></td>
      <td><input type="text" name="url" size="70" /></td>
    </tr>
    </table>
    <br>
    <input type="submit" value="Modify Item" 
           style="background-color:transparent;border-radius:10px;" />
    </form>
    </font>
    <hr size="5"></center></body></html>
'''

@bottle.route('/modify-item/<sid>/<address>/<sn>', method="get")
def modify_item(sid, address, sn):
    if (address == "0"): address = ""
    if (sn == "0"): sn = ""
    return(modify_item_html.format(sid, address, sn))
#enddef

@bottle.route('/do-modify-item/<sid>', method="post")
def do_modify_item(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    address = bottle.request.forms.get("address")
    sn = bottle.request.forms.get("serial")
    description = bottle.request.forms.get("description")
    url = bottle.request.forms.get("url")
    state = create_register_state(sn, description, url, "modify-item")

    #
    # Do an API supply/updateitem.
    #
    status = api.nexus_supply_update_item_by_address(address,
        json.dumps(state))
    if (status.has_key("error")):
        genesis = api.genesis_id
        reason = status["error"]["message"]
        msg = ("Item {}<br>modification failed to<br>" + \
            "Genesis-ID {}<br>Reason: {}").format(address, genesis, reason)
        return(show_html.format(bold(msg)))
    #endif
    address = status["result"]["address"]

    sleep()

    #
    # Get item's state to make sure it didn't change during the change of
    # ownership.
    #
    status = api.nexus_supply_get_item_by_address(address)
    if (status.has_key("error")):
        reason = status["error"]["message"]
        msg = "API supply/get/item failed - {}".format(reason)
        output = show_html.format(bold(msg))
    else:
        state = json.loads(status["result"]["data"])
        output = show_item(sid, api, address, state)
    #endif
    return(output)
#enddef

transfer_item_html = '''
    <html><title>Nexus Supply-Chain Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
    border-radius:20px;border:5px solid #666666;">

    <font face="verdana">
    <center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Supply-Chain Application</b></a></head><br><br><hr size="5">

    <br><form action="/do-transfer-item/{}" method="post">
    <table align="center" border="0">
    <tr>
      <td><i>Enter Item Address:</i></td>
      <td><input type="text" name="address" value="{}" size="70" /></td>
    </tr>
    <tr>
      <td><i>Enter New Owner Genesis-ID:</i></td>
      <td><input type="text" name="genesis" size="70" /></td>
    </tr>
    </table>
    <br>
    <input type="submit" value="Transfer Item Ownership" 
           style="background-color:transparent;border-radius:10px;" />
    </form>
    </font>
    <hr size="5"></center></body></html>
'''

@bottle.route('/transfer-item/<sid>/<address>', method="get")
def transfer_item(sid, address):
    if (address == "0"): address = ""
    return(transfer_item_html.format(sid, address))
#enddef

@bottle.route('/do-transfer-item/<sid>', method="post")
def do_transfer_item(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    address = bottle.request.forms.get("address")
    genesis = bottle.request.forms.get("genesis")

    #
    # Do an API supply/transfer.
    #
    status = api.nexus_supply_transfer_item_by_address(address, genesis)
    if (status.has_key("error")):
        reason = status["error"]["message"]
        msg = ("Item {}<br>ownership transfer failed to<br>" + \
            "Genesis-ID {}<br>Reason: {}").format(address, genesis, reason)
        return(show_html.format(bold(msg)))
    #endif
    address = status["result"]["address"]

    sleep()

    #
    # Get item's state to make sure it didn't change during the change of
    # ownership.
    #
    status = api.nexus_supply_get_item_by_address(address)
    if (status.has_key("error")):
        reason = status["error"]["message"]
        msg = "API supply/get/item failed - {}".format(reason)
        output = show_html.format(bold(msg))
    else:
        state = json.loads(status["result"]["data"])
        output = show_item(sid, api, address, state)
    #endif
    return(output)
#enddef

claim_item_html = '''
    <html><title>Nexus Supply-Chain Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
    border-radius:20px;border:5px solid #666666;">

    <font face="verdana">
    <center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Supply-Chain Application</b></a></head><br><br><hr size="5">

    <br><form action="/do-claim-item/{}/0/0" method="post">
    <table align="center" border="0">
    <tr>
      <td><i>Enter txid from Transfer Transaction:</i></td>
      <td><input type="text" name="txid" size="70" /></td>
    </tr>
    </table>
    <br>
    <input type="submit" value="Claim Item Ownership" 
           style="background-color:transparent;border-radius:10px;" />
    </form>
    </font>
    <hr size="5"></center></body></html>
'''

@bottle.route('/claim-item/<sid>/<address>/<txid>', method="get")
def claim_item(sid, address, txid):
    if (address == "0"):
        return(claim_item_html.format(sid))
    else:
        return(do_claim_item(sid, address, txid))
    #endif
#enddef

@bottle.route('/do-claim-item/<sid>/<address>/<txid>', method="post")
def do_claim_item(sid, address, txid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    if (address == "0"):
        txid = bottle.request.forms.get("txid")
        address = bottle.request.forms.get("address")
    #endif

    #
    # Do an API supply/transfer.
    #
    status = api.nexus_supply_claim_item(txid)
    if (status.has_key("error")):
        reason = status["error"]["message"]
        msg = ("Item ownership claim failed for<br>txid {}<br>Reason: {}"). \
            format(txid, reason)
        return(show_html.format(bold(msg)))
    #endif
    address = status["result"]["claimed"][0]

    sleep()

    #
    # Get item's state to make sure it didn't change during the change of
    # ownership.
    #
    status = api.nexus_supply_get_item_by_address(address)
    if (status.has_key("error")):
        reason = status["error"]["message"]
        msg = "API supply/get/item failed - {}".format(reason)
        output = show_html.format(bold(msg))
    else:
        state = json.loads(status["result"]["data"])
        output = show_item(sid, api, address, state)
    #endif
    return(output)
#enddef

show_checksum_html = '''
    <html><title>Nexus Supply-Chain Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
    border-radius:20px;border:5px solid #666666;">

    <font face="verdana">
    <center>
    <br><head><a href="/logged-in/{}" style="text-decoration:none;"><font 
                 color="black">
    <b>Nexus Supply-Chain Application</b></a></head><br><br><hr size="5">

    <table>
    <tr><td>Computed Checksum:</td><td>{}</td></tr>
    <tr><td>Document URL:</td><td>{}</td></tr>
    </table>

    <hr size="5">

    <a href="/display-item/{}/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Display Item</button></a>
    <hr size="5"></center></body></html>
'''

@bottle.route('/checksum/<sid>/<address>/<url_index>', method="get")
def checksum(sid, address, url_index):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    if (url_cache.has_key(url_index) == False):
        msg = "Cannot find URL for index {}".format(url_index)
        return(show_html.format(bold(msg)))
    else:
        url = url_cache[url_index]
    #endif

    #
    # Get state and update the checksum and checksum verified time.
    #
    status = api.nexus_supply_get_item_by_address(address)
    if (status.has_key("error")):
        reason = status["error"]["message"]
        msg = "API supply/get/item failed - {}".format(reason)
        output = show_html.format(bold(msg))
    else:
        state = json.loads(status["result"]["data"])
    #endif

    #
    # Do checksum of file.
    #
    changed = checksum_file(url, state)
    checksum = state["document-checksum"]
    checksum = print_red(checksum) if changed else  print_green(checksum)

    #
    # Update item in blockchain.
    #
    state["creation-date"] = commands.getoutput("date")
    state["action"] = "verify-checksum"
    status = api.nexus_supply_update_item_by_address(address,
        json.dumps(state))

    #
    # Return response to user.
    #
    output = show_checksum_html.format(sid, checksum, print_cour(url), sid,
        address)
    return(output)
#enddef

@bottle.route('/display-item/<sid>/<address>', method="get")
def display_item(sid, address):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    status = api.nexus_supply_get_item_by_address(address)
    if (status.has_key("error")):
        reason = status["error"]["message"]
        msg = "API supply/get/item failed - {}".format(reason)
        output = show_html.format(bold(msg))
    else:
        state = json.loads(status["result"]["data"])
        output = show_item(sid, api, address, state)
    #endif
    return(output)
#enddef

#------------------------------------------------------------------------------

date = commands.getoutput("date")
print "qrsc starting up at {}".format(date)

bottle.run(host="0.0.0.0", port=qrsc_port, debug=True)
exit(0)

#------------------------------------------------------------------------------
