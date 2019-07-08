#!/usr/bin/env python
#
# watermark.py - Document Watermark application
#
# This program implements a web-server application that allows a document
# to be watermarked with a genesis-id, so only the user that is logged in
# with that owns the genesis-id can read the file. 
#
# The document file is modified so any other application cannot read it. That
# is, the file is obfuscated with the genesis-id so only this watermark
# application can read the file.
#
# This app allows others to read the file as well, so multi genesis-id
# watermarking is supported and as long as one of the genesis-ids are logged
# in, they can read the file.
#
# Usage: python watermark.py [<port>]
#
# This program has depends on the nexus_sdk.py SDK library where the master
# copy is in LLL-TAO/sdk/nexus_sdk.py. A shadow copy is in a peer
# directory in TAO-App/sdk/nexus_sdk.py and this application directory
# symlinks to the sdk library (via "import nexus_sdk as nexus")..
#
#------------------------------------------------------------------------------

import commands
import bottle
import sys
import time
import os
import urllib2
import random
import json

try:
    sdk_dir = os.getcwd().split("/")[0:-2]
    sdk_dir = "/".join(sdk_dir) + "/sdk"
    sys.path.append(sdk_dir)
    import nexus_sdk as nexus
except:
    print "Need to place nexus_sdk.py in this directory"
    exit(0)
#endtry

#------------------------------------------------------------------------------

#
# Did command line have a port number to listen to?
#
wm_port = int(sys.argv[1]) if (len(sys.argv) == 2) else 5555

#
# Indexed by nexus session-id and returns class from nexus_sdk.sdk_init().
# Allows session continuity across web pages.
#
contexts = {}

#
# Need some delays so block verification can happen in the daemon.
#
def sleep(s=2):
    time.sleep(s)
#enddef    

#------------------------------------------------------------------------------

#
# ---------- Simple function definitions. ----------
#

def green(string):
    output = '<font color="green">{}</font>'.format(string)
    return(output)
#enddef

def red(string):
    output = '<font color="red">{}</font>'.format(string)
    return(output)
#enddef

def blue(string):
    output = '<font color="blue">{}</font>'.format(string)
    return(output)
#enddef

def cour(string):
    output = '<font face="Courier New">{}</font>'.format(string)
    return(output)
#enddef             

def bold(string):                                                              
    string = string.replace("[1m", "<b>")                                      
    string = string.replace("[0m", "</b>")                                     
    return(string)                                                             
#enddef

#------------------------------------------------------------------------------

#
# ---------- Support functions for app specific semantics.
#

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
        return(None, show_html(msg))
    #endif
    api = contexts[sid]
    return(api, "")
#enddef

#
# cache_document
#
# Store file on local disk. Location is:
# form is:
#
#     documents/<username>/<document-name>-<genid>
#
def cache_document(document_name, document, username, genid):
    directory = "./documents/" + username
    os.system("mkdir -p {}".format(directory))

    filename = directory + "/" + document_name + "-" + genid
    f = open(filename, "w"); f.write(document); f.close()
#enddef

#
# get_doucments
#
# Get documents from local documents directory.
#
def get_documents():
    directory = "./documents/"
    documents = commands.getoutput("ls -1 {}".format(directory))
    documents = documents.split("\n")
    return(documents)
#enddef

#
# normalize_filename
#
# This is temporary until the Nexus blockchain will allow "." to be stored
# as supply-chain item names.
#
def normalize_filename(filename):
    f = filename.replace(".", "-")
    return(f)
#enddef

#
# put_whitelist
#
# Create supply-change item to store a list of genids that are part of the
# whitelist for this file.
#
def put_whitelist(api, filename, genids):
    item_data = json.dumps(genids)
    f = normalize_filename(filename)

    status = api.nexus_supply_create_item(f, item_data)
    if (status.has_key("error")):
        msg = status["error"]["message"]
        print "nexus_supply_create_item() failed: {}".format(msg)
    #endif
#enddef

#
# get_whitelist
#
# Look up supply-change item to get a list of genids that are part of the
# whitelist for this file.
#
def get_whitelist(api, user, filename):
    f = normalize_filename(filename)
    item_name = user + ":" + f

    status = api.nexus_supply_get_item_by_name(item_name)
    if (status.has_key("error")): return([])

    item_data = status["result"]["data"]
    item_data = json.loads(item_data)
    if (type(item_data) != list): return([])

    #
    # Validate contents of item-data. It must be a list of 32-byte genesis
    # IDs in a 64 character string.
    #
    for item in item_data:
        if (type(item) != unicode or len(item) != 64): return([])
    #endfor
    return(item_data)
#enddef

#
# show_files
#
# Display list of files that have been loaded by this watermark application.
#
def show_files(sid, api):
    ws = "&nbsp;&nbsp;"
    users = commands.getoutput("ls -1 ./documents").split("\n")

    output = "<hr size='5'>"
    output += "Watermarked Documents<br><br>"
    
    if (users == [""]):
        output += "No files downloaded"
        return(output)
    #endif

    output += table_header(ws, ws, ws, "Document Name", ws,
        "Creation Time<br>Modification Time", ws, "Creator Username", ws,
        "Creator Genesis-ID")
    output += table_row(ws)

    for user in users:
        cmd = "ls -1 ./documents/{}".format(user)
        files = commands.getoutput(cmd).split("\n")
        for filename in files:
            f = filename.split("-")

            #
            # This app didn't create this file. ;-)
            #
            if (len(f) == 1): continue

            genid = f[-1]
            f = "<i>" + "-".join(f[0:-1]) + "</i>"
            view = button_view.format(sid, user + "%" + filename)

            path = "./documents/{}/{}".format(user, filename)
            ct = time.ctime(os.path.getctime(path))
            mt = time.ctime(os.path.getmtime(path))

            output += table_row(view, ws, ws, f, ws, ct, ws, user, ws, genid)

            genids = get_whitelist(api, user, filename)
            if (genids == []): continue

            wl = "<b>white-list:</b>"
            for genid in genids:
                w, m = (wl, mt) if (genids.index(genid) == 0) else (ws, ws)
                g = '<font face="Courier New">{}</font>'.format(genid)
                output += table_row(ws, ws, ws, ws, ws, m, ws, w, ws, g)
            #endfor
            output += table_row(ws)

        #endfor
    #endfor
    output += table_footer()

    return(output)
#enddef

#
# read_and_store_document
#
# Read document from URL.
#
def read_and_store_document(url, api):
    if (url[0:4] == "http"):
        try:
            u = urllib2.urlopen(url); buf = u.read(); u.close()
        except:
            return([None, "Could not read URL file {}".format(url)])
        #endtry
    else:
        try:
            f = open(url, "r"); buf = f.read(); f.close()
        except:
            return([None, "Could not read local file {}".format(url)])
        #endtry
    #endif

    filename = url.split("/")[-1]
    cache_document(filename, buf, api.username, api.genesis_id)
    return([filename, None])
#enddef

#
# read_document
#
# Read document from local disk.
#
def read_document(filename, username, genid):
    try:
        f = open("documents/{}/{}".format(username, filename + "-" + genid))
        contents = f.read()
        f.close()
    except:
        contents = None
    #endtry
    return(contents)
#enddef

#
# watermark
#
# Embed supplied genesis-IDs in file. The format is a 2-digit length as a
# string followed by 32-byte (64 ascii digits) genesis-IDs.
#
def watermark(contents, genids):
    prefix = ""
    for genid in genids:
        prefix += str(genid)
    #endfor

    count = str(len(genids))
    count = count.zfill(2)
    prefix = count + prefix

    return(prefix + contents)
#enddef

#
# unwatermark
#
# Do the opposite of watermark(). Return contents with watermark removed if
# genid removed equals supplied genid. Othersise, return None.
#
def unwatermark(contents, genid):
    count = int(contents[0:2])
    skip = 2 + (64 * count)

    genid = str(genid)
    file_genid = contents[2::]
    for i in range(0, count):
        if (genid == file_genid[0:64]): return(contents[skip::])
        file_genid = file_genid[64::]
    #endfor

    #
    # Did not find a match.
    #
    return(None)
#enddef

#
# watermark_document
#
# Read file from documents directory, watermark with genid and put
#
def watermark_document(filename, username, genid, readers):
    contents = read_document(filename, username, genid)

    wm_contents = watermark(contents, readers)
    cache_document(filename, wm_contents, username, genid)
    return
#enddef

#------------------------------------------------------------------------------

#
# Wrapper to make all web pages look alike, from a format perspective.
#
wm_page = '''
    <html>
    <title>Nexus Document Watermak Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
         border-radius:20px;border:5px solid #666666;">
    <font face="verdana"><center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Document Watermark Application</b></a></head><br><br>
    <hr size="5">

    {}

    <hr size="5"></center></font></body></html>
'''
wm_page_logged_in = '''
    <html>
    <title>Nexus Document Watermak Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
         border-radius:20px;border:5px solid #666666;">
    <font face="verdana"><center>
    <br><head><a href="/logged-in/{}" style="text-decoration:none;"><font 
                 color="black">
    <b>Nexus Document Watermark Application</b></a></head><br><br>
    <hr size="5">

    {}

    <hr size="5"></center></font></body></html>
'''

#
# table_header
#
# Print a consistent table header row for all displays.
#
def table_header(*args):
    output = '<center><table align="center" border="0"><tr>'

    for arg in args:
        output += '''
            <td align="left"><font face="Sans-Serif"><b>{}</b></font></td>
        '''.format(arg)
    #endfor
    output += "</tr></center>"
    return(output)
#enddef

#
# table_row
#
# Print a consistent table row for all displays.
#
def table_row(*args):
    output = "<tr>"
    for arg in args:
        output += '''
            <td valign="top"><font face="Sans-Serif">{}</font></td>
        '''.format(arg)
    #endfor
    output += "</tr>"
    return(output)
#enddef

#
# table_footer
#
# Print a consistent table footer for all displays ending a html table.
#
def table_footer():
    return("</table>")
#enddef

button_view = '''
    <a href="/view-document/{}/{}">
    <button style="background-color:transparent;border-radius:10px;
        height:20px;" type="button">view</button></a>
'''
button_wm = '''
    <a href="/watermark-document/{}">
    <button style="background-color:transparent;border-radius:10px;
        height:20px;" type="button">watermark</button></a>
'''
button_load = '''
    <a href="/load-document/{}">
    <button style="background-color:transparent;border-radius:10px;
        height:20px;" type="button">Load Document</button></a>
'''
button_refresh = '''
    <a href="/logged-in/{}">
    <button style="background-color:transparent;border-radius:10px;
        height:20px;" type="button">Refresh</button></a>
'''
button_sigchain = '''
    <a href="/account-info/{}">
    <button style="background-color:transparent;border-radius:10px;
        height:20px;" type="button">Account Info</button></a>
'''

#
# show_html
#
# Returns a ascii string to the web user. Wrapped in wm_page_logged_in.
#
def show_html(msg, sid=None):
    msg = bold(msg)
    if (sid == None): return(wm_page.format(msg))
    return(wm_page_logged_in.format(sid, msg))
#enddef

#------------------------------------------------------------------------------

#
# All web page and action functions follow.
#

landing_page_html = '''
    <center>
    <br><form action="/login" method="post">
    <table align="center" border="0">
    <tr>
      <td><i>Username:&nbsp;&nbsp;</i></td>
      <td><input type="text" name="username" size="40" /></td>
    </tr>
    <tr>
      <td><i>Password:&nbsp;&nbsp;</i></td>
      <td><input type="text" name="password" size="40" /></td>
    </tr>
    <tr>
      <td><i>PIN:&nbsp;&nbsp;</i></td>
      <td><input type="text" name="pin" size="40" /></td>
    </tr>
    </table>
    <br>
    <input type="submit" value="Login" name="action" 
        style="background-color:transparent;border-radius:10px;height:20px;" />
    <br><br>
    <input type="submit" value="Create Login Account" name="action"
       style="background-color:transparent;border-radius:10px;height:20px;" />
    </form>
    </center>
'''

#
# landing_page
#
# Output landing page to web browser client.
#
@bottle.route('/')
def landing_page():
    output = landing_page_html
    return(wm_page.format(output))
#enddef

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
    create_account = (action.find("Create") != -1)

    if (user == ""):
        create_login = "Create" if create_account else "Login"
        msg = "{} failed for empty username".format(create_login)
        return(show_html(msg))
    #endif

    api = nexus.sdk_init(user, pw, pin)

    #
    # When we create a user account, create a token account so user can send
    # and receive tokens.
    #
    if (create_account):
        status = api.nexus_users_create_user()
        if (status.has_key("error")):
            r = status["error"]["message"]
            msg = "Account create failed for user '{}', {}".format(user, r)
            return(show_html(msg))
        #endif
        sleep()
    #endif

    #
    # Log into new account or existing account.
    #
    status = api.nexus_users_login_user()
    if (status.has_key("error")):
        r = status["error"]["message"]
        msg = "Login failed for user '{}', {}".format(user, r)
        return(show_html(msg))
    #endif
    sid = status["result"]["session"]
    sleep()

    contexts[sid] = api
    output = display_user(sid, api)
    return(output)
#enddef

#
# display_user
#
# Show user data and transaction history.
#
def display_user(sid, api):
    ws = "&nbsp;"
    output = table_header(ws, ws)
    output += table_row("Username:&nbsp;&nbsp;", api.username)
    output += table_row("Genesis-ID:&nbsp;&nbsp;", api.genesis_id)
    output += table_row(ws, ws)
    output += table_footer()

    output += button_load.format(sid) + "&nbsp;"
    output += button_refresh.format(sid) + "&nbsp;"
    output += button_sigchain.format(sid)

    output += show_files(sid, api)
    return(wm_page.format(output))
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

    output = display_user(sid, api)
    return(output)
#enddef

account_info_header = '''
    <table>
    <tr valign="top" align="left">
    <td align="center"><b>{}</b></td><td>&nbsp;</td><td>&nbsp;</td>
    <td><b>{}</b></td>
    </tr>
'''
account_info_row = '''
    <tr><td>{}</td>
    <td style="word-break:break-all;">{}</td>
'''
account_info_footer = "</table>"

#
# display_account_info
#
# Display account transactions or notifications with this single routin.
#
def display_account_info(transactions):
    output = account_info_header.format("", "")

    for tx in transactions:
        if (tx.has_key("txid")):
            txid = tx["txid"]
            output += account_info_row.format("txid:", txid)
        #endif

        if (tx.has_key("contracts") == False or tx["contracts"] == None):
            output += account_info_row.format(tx, "")
            output += account_info_row.format("&nbsp;", "&nbsp;")
            continue
        #endif

        for contract in tx["contracts"]:
            op = contract["OP"]
            op = "OP: {}&nbsp;&nbsp;".format(op)
            address = ""
            if (contract.has_key("address")):
                address = contract["address"]
                address = "address: {}".format(address)
            #endif
            output += account_info_row.format(op, address)
        #endfor
        output += account_info_row.format("&nbsp;", "&nbsp;")
    #endfor
    output += account_info_footer

    return(output)
#enddef

#
# account_info
#
# Show sigchain transactions and notification queue.
#
@bottle.route('/account-info/<sid>', method="get")
def account_info(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    output = ""
    
    output += "<br>"
    ts = api.nexus_users_list_transactions_by_username(0, 50)
    if (ts.has_key("error")):
        output += "Could not retrieve account transactions"
    else:
        output += "Transactions for User <b>{}</b>".format(api.username)
        output += "<br><br>"
        output += display_account_info(ts["result"])
    #endif

    return(wm_page_logged_in.format(sid, output))
#enddef

load_html = '''
    <center>
    <br><form action="/do-watermark/{}" method="post">
    <table align="center" border="0">
    <tr>
      <td><i>Enter Document URL or local File Name:&nbsp;&nbsp;</i></td>
      <td><input type="text" name="url" size="60" /></td>
    </tr>
    <tr>
      <td>&nbsp;</td>
    </tr>
    </table>

    <i>Enter user Genesis-IDs to allow file access:&nbsp;&nbsp;</i>

    <table align="center" border="0">
    <tr>
      <td>&nbsp;</td>
    </tr>
    <tr>
      <td><i>User-1 Genesis-ID:&nbsp;&nbsp;</i></td>
      <td><input type="text" name="user1" size="40" /></td>
    </tr>
    <tr>
      <td><i>User-2 Genesis-ID:&nbsp;&nbsp;</i></td>
      <td><input type="text" name="user2" size="40" /></td>
    </tr>
    <tr>
      <td><i>User-3 Genesis-ID:&nbsp;&nbsp;</i></td>
      <td><input type="text" name="user3" size="40" /></td>
    </tr>
    <tr>
      <td><i>User-4 Genesis-ID:&nbsp;&nbsp;</i></td>
      <td><input type="text" name="user4" size="40" /></td>
    </tr>
    </table>
    <br>
    <input type="submit" value="Watermark Document" name="action" 
        style="background-color:transparent;border-radius:10px;height:23px;" />
    </form>
    </center>
'''

#
# load_document
#
# Display page with genesis-ID QR-code.
#
@bottle.get('/load-document/<sid>')
def load_document(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    output = load_html.format(sid)
    return(wm_page_logged_in.format(sid, output))
#enddef

#
# documents
#
# For displaying images stored in png/pdf/jpg files.
#
@bottle.route('/tmp/<filename>', method="get")
def documents(filename):
    return(bottle.static_file(filename, root="./tmp"))
#enddef

#
# view_document
#
# Display document on line user clicked the view button
#
@bottle.get('/view-document/<sid>/<filename>')
def view_document(sid, filename):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    #
    # Read file bytes to pass to unwatermark().
    #
    filename = filename.replace("%", "/")
    filename = filename.replace(" ", "%20")
    filename = filename.replace(",", "%2C")
    try:
        f = open("documents/{}".format(filename), "r")
        contents = f.read(); f.close()
    except:
        output = "Could not find or read file {}".format(filename)
        return(wm_page_logged_in.format(sid, output))
    #endif

    #
    # Un watermark file and check to see if this logged in user can read it
    # by passing its genesis-id to unwatermark().
    #
    contents = unwatermark(contents, api.genesis_id)
    if (contents == None):
        output = "File access prohibited for your genesis-ID {}".format( \
            "<b>" + api.genesis_id + "</b>")
        return(wm_page_logged_in.format(sid, output))
    #endif

    #
    # Display file contents to validated user.
    #
    handle = str(random.randint(0, 0xffffffff))
    filename = "./tmp/{}".format(handle)
    f = open(filename, "w"); f.write(contents); f.close()
    filename = filename[1::]
    output = "<image align='center' src='{}'".format(filename)

    return(wm_page_logged_in.format(sid, output))
#enddef

#
# do_watermark
#
@bottle.post('/do-watermark/<sid>')
def do_load_document(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    url = bottle.request.forms.get("url")
    readers = [api.genesis_id]

    for u in ["user1", "user2", "user3", "user4"]:
        user = bottle.request.forms.get(u)
        if (user != "" and len(user) == 64): readers.append(user)
    #endfor

    #
    # Read file and store locally so we can watermark next.
    #
    filename, msg = read_and_store_document(url, api)
    if (filename == None):
        return(wm_page_logged_in.format(sid, msg))
    #endif
    
    #
    # Put list of genids in file.
    #
    watermark_document(filename, api.username, api.genesis_id, readers)

    #
    # Put list of genids in blockchain supply-chain item wo we can display
    # which have file access in show_files().
    #
    filename += "-" + api.genesis_id
    put_whitelist(api, filename, readers)

    output = display_user(sid, api)
    return(output)
#enddef

#------------------------------------------------------------------------------

#
# ---------- Main program entry point. ----------
#
date = commands.getoutput("date")
print "watermark starting up at {}".format(date)

#
# Create documents and tmp directories if they does not exist.
#
os.system("mkdir -p ./documents")
os.system("mkdir -p ./tmp")

#
# Run web server.
#
bottle.run(host="0.0.0.0", port=wm_port, debug=True)
exit(0)

#------------------------------------------------------------------------------
