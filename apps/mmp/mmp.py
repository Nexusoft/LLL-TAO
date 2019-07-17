#!/usr/bin/env python
#
# mmp.py - Music Market Place
#
# This program implements a web-server application that allows music artists,
# record label companies, music distributors, and music buyers to come to
# exchange song assets for tokens.
#
# Usage: python mmp.py [<port>]
#
# This program has depends on the nexus_sdk.py SDK library where the master
# copy is in LLL-TAO/sdk/nexus_sdk.py. A shadow copy is in a peer
# directory in TAO-App/sdk/nexus_sdk.py and this application directory
# symlinks to the sdk library (via "import nexus_sdk as nexus")..
#
#------------------------------------------------------------------------------

import commands
import os
import bottle
import sys
import time
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
# Indexed by nexus session-id and returns class from nexus_sdk.sdk_init().
# Allows session continuity across web pages.
#
contexts = {}

#
# Did command line have a port number to listen to?
#
mmp_port = int(sys.argv[1]) if (len(sys.argv) == 2) else 3333

#
# API handle for account 'admin'. Created in create_music_tokens()
#
admin_account = None

#
# Store assets. Write them to disk. Read them on application restart. Indexed
# by asset name, value is dictionary array that is stored in assets API
# metadata.
#
assets = {}

#
# Need some delays so block verification can happen in the daemon.
#
def sleep():
    time.sleep(2)
#enddef    

#------------------------------------------------------------------------------

#
# ---------- Simple function definitions. ----------
#

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

def print_cour(string):
    output = '<font face="Courier New">{}</font>'.format(string)
    return(output)
#enddef             

#
# cache_asset
#
# Put txid and register address in dictionary array "txids" and write a
# file out named 'txid' with contents address in directory "txids".
#
def cache_asset(asset_name, asset_data):
    assets[asset_name] = asset_data
    asset_data = json.dumps(asset_data)

    directory = "assets/"
    os.system("mkdir -p {}".format(directory))
    f = open(directory + asset_name, "w"); f.write(asset_data); f.close()
#enddef

#
# read_assets
#
# Load up the assets dictionary array from disk.
# 
def read_assets():
    global assets

    directory = "./assets/"
    if (os.path.exists(directory) == False): return
    
    files = commands.getoutput('ls -1 {}'.format(directory)).split("\n")
    for asset in files:
        f = open(directory + asset, "r"); data = f.read(); f.close()
        if (data == ""): continue
        assets[asset] = json.loads(data)
    #endfor
    print "Read {} assets from disk".format(len(files))
#enddef

#
# get_asset_owner
#
# Get current asset owner from blockchaini.
#
def get_asset_owner(api, asset_name):
    status = api.nexus_assets_get_asset_by_name(asset_name)
    if (status.has_key("error")): return("?")
    return(status["result"]["owner"])
#enddef

#
# show_assets
#
# Show all entries from the asset dictionary array.
#
def show_assets(api):
    global assets
    
    output = "<hr><br>Song Assets<br><br>"
    iown = print_green("I own")
    output += table_header1("Asset Name", "Song Title", "Song<br>Price",
        "Song Creator", "Owner Genesis-ID ({})".format(iown),
        "Share Owner/Pct", "Share Owner/Pct", "Share Owner/Pct")

    for a in assets.values():
        s1 = a["share-owner1"]
        if (s1 != ""): s1 += "&nbsp;->&nbsp;" + a["share-pct1"] + "%"
        s2 = a["share-owner2"]
        if (s2 != ""): s2 += "&nbsp;->&nbsp;" + a["share-pct2"] + "%"
        s3 = a["share-owner3"]
        if (s3 != ""): s3 += "&nbsp;->&nbsp;" + a["share-pct3"] + "%"

        an = a["asset-creator"] + ":" + a["asset-name"]
        genid = get_asset_owner(api, an)
        genid = print_green(genid) if api.genesis_id == genid else \
            print_red(genid)
        output += table_row(a["asset-name"], a["song-title"], a["song-price"],
            a["asset-creator"], genid, s1, s2, s3)
    #endfor
    output += table_footer() + "<br>"
    return(output)
#enddef

#
# wait_for_debit_notifcation
#
# Check notifications for a debit and return the txid. Loop for a few tries.
#
def wait_for_debit_notifcation(api):
    for i in range(0,4):
        status = api.nexus_users_list_notifications_by_username(0, 1)

        if (status.has_key("error")): 
            sleep()
            continue
        #endif
        notification = status["result"]
        if (notification == []):
            sleep()
            continue
        #endif

        contract = notification[0]["contracts"]
        txid = notification[0]["txid"]
        entry = contract[0]

        if (entry.has_key("OP") == False or entry["OP"] != "DEBIT"):
            sleep()
            continue
        #endif
        return(txid)
    #endfor

    return(None)
#enddef

#------------------------------------------------------------------------------

#
# Wrapper to make all web pages look alike, from a format perspective.
#
mmp_page = '''
    <html>
    <title>Nexus Music Market Place Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
         border-radius:20px;border:5px solid #666666;">
    <font face="verdana"><center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Music Market Place Application</b></a></head><br><br><hr>

    {}

    <hr></center></font></body></html>
'''
mmp_page_logged_in = '''
    <html>
    <title>Nexus Music Market Place Application</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
         border-radius:20px;border:5px solid #666666;">
    <font face="verdana"><center>
    <br><head><a href="/logged-in/{}" style="text-decoration:none;"><font 
                 color="black">
    <b>Nexus Music Market Place Application</b></a></head><br><br><hr>

    {}

    <hr></center></font></body></html>
'''

#
# table_header/table_header1
#
# Print a consistent table header row for all displays.
#
def table_header(*args):
    output = '<center><table align="center" border="0"><tr>'

    for arg in args:
        output += '''
            <td align="center"><font face="Sans-Serif"><b>{}</b></font></td>
        '''.format(arg)
    #endfor
    output += "</tr></center>"
    return(output)
#enddef
def table_header1(*args):
    output = '<center><table align="center" border="1"><tr>'

    for arg in args:
        output += '''
            <td align="center"><font face="Sans-Serif"><b>{}</b></font></td>
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

#
# images
#
# For displaying images stored in png files.
#
@bottle.route('/images/<filename>', method="get")
def images_filename(filename):
    return(bottle.static_file(filename, root="./images"))
#enddef

#
# show_html
#
# Returns a ascii string to the web user. Wrapped in mmp_page_logged_in.
#
def show_html(msg, sid=None):
    if (sid == None): return(mmp_page.format(msg))
    return(mmp_page_logged_in.format(sid, msg))
#enddef

landing_page_html = '''
    <center>
    <br><form action="/login" method="post">
    {}<br><br>
    <table align="center" border="0">
    <tr>
      <td><i>Username:</i></td>
      <td><input type="text" name="username" size="40" /></td>
    </tr>
    <tr>
      <td><i>Password:</i></td>
      <td><input type="text" name="password" size="40" /></td>
    </tr>
    <tr>
      <td><i>PIN:</i></td>
      <td><input type="text" name="pin" size="40" /></td>
    </tr>
    </table>
    <br>
    <input type="submit" value="{}" name="action"
           style="background-color:transparent;border-radius:10px;" />
    <input type="submit" value="{}" name="action"
           style="background-color:transparent;border-radius:10px;" />
    </form>
    </center>
'''

#
# Html for images used in application.
#
admin_html = '''
  <img align="center" width="100%" height="100%" src="/images/admin.png">
'''
artist_html = '''
  <img align="center" width="100%" height="100%" src="/images/artist.png">
'''
label_html = '''
  <img align="center" width="100%" height="100%" src="/images/label.png">
'''
dist_html = '''
  <img align="center" width="100%" height="100%" src="/images/distributor.png">
'''
buyer_html = '''
  <img align="center" width="100%" height="100%" src="/images/buyer.png">
'''
overview_html = '''
  <img align="center" height="auto" width="auto" src="/images/overview.png">
'''

#
# landing_page
#
# Output landing page to web browser client.
#
@bottle.route('/')
def landing_page():
    output = table_header(artist_html, label_html, dist_html, buyer_html)

    a = landing_page_html.format("<b>Artists</b> Login Here", "Login Artist",
        "Create Artist Account")
    l = landing_page_html.format("Record <b>Labels</b> Login Here",
        "Login Label", "Create Label Account")
    d = landing_page_html.format("Music <b>Distributors</b> Login Here",
        "Login Distributor", "Create Distributor Account")
    b = landing_page_html.format("Music <b>Buyers</b> Login Here",
        "Login Buyer", "Create Buyer Account")
    output += table_row(a, l, d, b)
    output += table_footer()

    output += "<hr>"
    output += button_overview
    return(mmp_page.format(output))
#enddef

button_refresh = '''
    <a href="/logged-in/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Refresh</button></a>
'''
button_overview = '''
    <a href="/overview">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">See Overview</button></a>
'''
button_create_token = '''
    <a href="/create-token/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Create Token</button></a>
'''
button_get_token = '''
    <a href="/lookup-token/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Lookup Token</button></a>
'''
button_create_token_account = '''
    <a href="/create-token-account/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Create Token Account</button></a>
'''
button_get_token_account = '''
    <a href="/lookup-token-account/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Lookup Token Account</button></a>
'''
button_create_asset = '''
    <a href="/create-asset/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Create Song Asset</button></a>
'''
button_get_asset = '''
    <a href="/lookup-asset/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Lookup Song Asset</button></a>
'''
button_transfer_asset = '''
    <a href="/transfer-asset/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Transfer Asset to Buyer</button></a>
'''
button_debit_tokens = '''
    <a href="/debit-tokens/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Debit Tokens</button></a>
'''
button_credit_tokens = '''
    <a href="/credit-tokens/{}">
    <button style="background-color:transparent;border-radius:10px;"
        type="button">Credit Tokens</button></a>
'''

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

account_history_html_row = '''
    <tr valign="top" align="center"><td>{}</td><td>{}</td>
    <td style="word-break:break-all;">{}</td>
    </tr>
'''

#
# show_transactoins
#
# So all transactions for a user account.
#
def show_transactions(api):
    tstatus = api.nexus_users_list_transactions_by_username(0, -1, 2)
    if (tstatus.has_key("error")): return("")

    nstatus = api.nexus_users_list_notifications_by_username(0, -1, 2)
    if (nstatus.has_key("error")): return("")

    user = "<b>{}</b>".format(api.username)
    output = ""
    for i in range(2):
        output += "<hr><br>"
        if (i == 0):
            output += "Transactions for User '{}'".format(user)
            status = tstatus
        else:
            output += "Notifications Posted for '{}'".format(user)
            status = nstatus
        #endif
        output += "<br><br>"

        output += table_header("sequence", "op", "data")
        for tx in status["result"]:
            seq = tx["sequence"]
            op = ""
            data = tx["operation"] if tx.has_key("operation") else ""
            if (data != "" and data != None):
                op = data["OP"]
                data.pop("OP")
                data = json.dumps(data)
                data = data.replace("u'", "'")
            #endif
            output += account_history_html_row.format(seq, op, data)
        #endfor
        output += table_footer()
    #endfor

    return(output)
#enddef

#
# display_buttons
#
# Write buttons to existing output buffer.
#
def display_buttons(api, sid, output):
    user = api.username

    output += "<hr>"
    if (user.find("artist") != -1 or user.find("admin") != -1):
        output += button_create_asset.format(sid)
        output += button_get_asset.format(sid)
        output += button_transfer_asset.format(sid)
        output += "<br>"
    #endif

    output += button_create_token_account.format(sid)
    output += button_get_token_account.format(sid)
    output += "<br>"

    if (user.find("admin") != -1):
        output += button_create_token.format(sid)
        output += button_get_token.format(sid)
        output += "<br>"
    #endif

    output += button_debit_tokens.format(sid)
    output += button_credit_tokens.format(sid)
    return(output)
#enddef

#
# display_user
#
# Show user data and transaction history.
#
def display_user(api, user, user_type, genesis, sl):
    status = api.nexus_tokens_get_account_by_name(api.username)
    balance = str(status["result"]["balance"])
    balance = balance.replace(".0", "")
    token_name = "admin-admin:music-token"

    tab1 = table_header("", "")
    tab1 += table_row("Username:", user)
    tab1 += table_row("Genesis-ID:", genesis)
    tab1 += table_row("Sigchain Entries:", sl)
    tab1 += table_row("Account Token Name:", api.username)
    balance = "<b>{}</b>".format(balance)
    tab1 += table_row("Account Token Balance:", balance)
    tab1 += table_row("Token-Name:", token_name)
    tab1 += table_row("&nbsp;", "&nbsp;")
    tab1 += table_row("", button_refresh.format(api.session_id))
    tab1 += table_footer()

    output = table_header(user_type, tab1)
    output += table_footer()

    sid = api.session_id
    output += show_assets(api)
    output = display_buttons(api, sid, output)
    return(mmp_page.format(output))
#enddef

user_type_to_html = { "artist" : artist_html, "label" : label_html,
    "distributor" : dist_html, "buyer" : buyer_html, "admin" : admin_html }

#
# overview
#
# Display image with overview slides.
#
@bottle.route('/overview')
def overview():
    output = overview_html
    return(mmp_page.format(output))
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
        msg = "{} failed for empty username".format(action)
        return(show_html(msg))
    #endif

    if (user == "admin"):
        user_prefix = "admin-"
    else:
        if (action.find("Artist") != -1): user_prefix = "artist-"
        if (action.find("Label") != -1): user_prefix = "label-"
        if (action.find("Distributor") != -1): user_prefix = "distributor-"
        if (action.find("Buyer") != -1): user_prefix = "buyer-"
    #endif

    api = nexus.sdk_init(user_prefix + user, pw, pin)

    #
    # When we create a user account, create a token account so user can send
    # and receive tokens. We will debit by default 100 music-tokens from the
    # admin token account.
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
    genesis = status["result"]["genesis"]
    sid = status["result"]["session"]
    sleep()

    #
    # If new account, create a token account and debit some tokens from admin
    # token account.
    #
    admin = admin_account
    if (create_account):
        tn = "admin-admin:music-token"
        status = api.nexus_tokens_create_account(api.username, tn)
        if (status.has_key("error")):
            r = status["error"]["message"]
            msg = "Token Account create failed for user '{}', {}".format(user,
                r)
            return(show_html(msg))
        #endif
        sleep()

        ta = api.username + ":" + api.username
        status = admin.nexus_tokens_debit_token_by_name(tn, ta, 100)
        if (status.has_key("error")):
            r = status["error"]["message"]
            msg = "Token Account debit failed for user '{}', {}".format(user,
                r)
            return(show_html(msg))
        #endif

        txid = wait_for_debit_notifcation(api)
        if (txid == None):
            msg = "Timeout waiting for debit notification for user {}". \
                format(user)
            return(show_html(msg))
        #endif

        txid = status["result"]["txid"]
        status = api.nexus_tokens_credit_account_by_name(api.username, 100,
            txid)
        if (status.has_key("error")):
            r = status["error"]["message"]
            msg = "tokens/credit failed for '{}', {}".format(user, r)
            return(show_html(msg))
        #endif
    #endif

    contexts[sid] = api
    sl = get_sigchain_length(api)
    user_type = api.username.split("-")[0]
    user_type = user_type_to_html[user_type]
    output = display_user(api, user, user_type, genesis, sl)
    output += show_transactions(api) + "<hr>"
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
    sl = get_sigchain_length(api)
    user_type, user = api.username.split("-")
    user_type = user_type_to_html[user_type]
    output = display_user(api, user, user_type, genesis, sl)
    output += show_transactions(api) + "<hr>"
    return(output)
#enddef

form_header = '''
    <br><form action="/{}/{}" method="post">
    <table align="center" border="0">
'''
form_row = '''
    <tr>
      <td><i>{}:</i></td>
      <td><input type="text" name="{}" size="70" /></td>
    </tr>
'''
form_footer = '''
    </table>
    <br>
    <input type="submit" value="{}" 
           style="background-color:transparent;border-radius:10px;" />
    </form>
'''

@bottle.route('/create-asset/<sid>', method="get")
def create_asset(sid):
    output = "<br>Create Song Asset<br>"
    output += form_header.format("do-create-asset", sid)
    output += form_row.format("Enter Asset Name", "asset-name")
    output += form_row.format("Enter Song Title", "song-title")
    output += form_row.format("Enter Song Price", "song-price")
    output += form_row.format("Enter 1st Share Owner", "share-owner1")
    output += form_row.format("Enter Share Owner Percentage", "share-pct1")
    output += form_row.format("Enter 2nd Share Owner", "share-owner2")
    output += form_row.format("Enter Share Owner Percentage", "share-pct2")
    output += form_row.format("Enter 3rd Share Owner", "share-owner3")
    output += form_row.format("Enter Share Owner Percentage", "share-pct3")
    output += form_footer.format("Create Asset")
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/do-create-asset/<sid>', method="post")
def do_create_asset(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    asset_data = {}
    asset_data["asset-creator"] = api.username
    an = bottle.request.forms.get("asset-name")
    asset_data["asset-name"] = an
    st = bottle.request.forms.get("song-title")
    asset_data["song-title"] = st
    sp = bottle.request.forms.get("song-price")
    asset_data["song-price"] = sp
    so1 = bottle.request.forms.get("share-owner1")
    asset_data["share-owner1"] = so1
    sp1 = bottle.request.forms.get("share-pct1")
    asset_data["share-pct1"] = sp1
    so2 = bottle.request.forms.get("share-owner2")
    asset_data["share-owner2"] = so2
    sp2 = bottle.request.forms.get("share-pct2")
    asset_data["share-pct2"] = sp2
    so3 = bottle.request.forms.get("share-owner3")
    asset_data["share-owner3"] = so3
    sp3 = bottle.request.forms.get("share-pct3")
    asset_data["share-pct3"] = sp3

    #
    # Valiate percentages.
    #
    if (so1 == "" and so2 == "" and so3 == ""):
        msg = "No share owners specified"
        return(show_html(msg, sid))
    #endif
    if (so1 != "" and sp1 == "" and so2 == "" and so3 == ""):
        asset_data["share-pct1"] = "100"
    #endif
    if (sp1 == "" and sp2 == "" and sp3 == ""):
        count = 0
        for i in [so1, so2, so3]:
            if (i != ""): count += 1
        #endfor
        pct = 100 / count
        for i in [(so1, "share-pct1"),  (so2, "share-pct2"),
                  (so3, "share-pct3")]:
            if (i[0] != ""): asset_data[i[1]] = str(pct)
        #endfor
    #endif
    pct = int(sp1) if sp1.isdigit() else 0
    pct += int(sp2) if sp2.isdigit() else 0
    pct += int(sp3) if sp3.isdigit() else 0
    if (pct != 100 and pct != 0):
        msg = "Percentages need to add up to 100%"
        return(show_html(msg, sid))
    #endif        
        
    data = json.dumps(asset_data)
    
    #
    # Do an API assets/create call.
    #
    status = api.nexus_assets_create_asset(an, data)
    if (status.has_key("error")):
        r = status["error"]["message"]
        msg = "assets/create failed, {}".format(r)
        return(show_html(msg, sid))
    #endif
    a = print_cour(status["result"]["address"])
    t = print_cour(status["result"]["txid"])
    output =  "Asset <b>'{}'</b> created".format(an)
    output += table_header("", "")
    output += table_row("address", a)
    output += table_row("txid", t)
    output += table_footer()

    #
    # Save Asset on disk.
    #
    cache_asset(an, asset_data)

    output = display_buttons(api, sid, output)
    output += show_assets(api)
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

#
# display_asset
#
# Display data from returned status["result"]
#
def display_asset(result):
    data = json.loads(result["metadata"])

    output = table_header("", "")
    output += table_row("Asset Name:", data["asset-name"])
    output += table_row("Song Title:", data["song-title"])
    output += table_row("Song Price:", data["song-price"])
    output += table_row("Song Owner:", result["owner"])
    if (data["share-owner1"] != ""):
        o = data["share-owner1"] + "&nbsp;->&nbsp;" + data["share-pct1"] + "%"
        output += table_row("Share Owner:", o)
    #endif
    if (data["share-owner2"] != ""):
        o = data["share-owner2"] + "&nbsp;->&nbsp;" + data["share-pct2"] + "%"
        output += table_row("Share Owner:", o)
    #endif
    if (data["share-owner3"] != ""):
        o = data["share-owner3"] + "&nbsp;->&nbsp;" + data["share-pct3"] + "%"
        output += table_row("Share Owner:", o)
    #endif

    output += table_footer()
    return(output)
#enddef

@bottle.route('/lookup-asset/<sid>', method="get")
def lookup_asset(sid):
    output = "<br>Lookup Song Asset<br>"
    output += form_header.format("do-lookup-asset", sid)
    output += form_row.format("Enter Asset Name", "asset-name")
    output += form_footer.format("Lookup Asset")
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/do-lookup-asset/<sid>', method="post")
def do_lookup_asset(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    an = bottle.request.forms.get("asset-name")

    #
    # Do an API assets/get call.
    #
    status = api.nexus_assets_get_asset_by_name(an)
    if (status.has_key("error")):
        r = status["error"]["message"]
        msg = "assets/get failed for asset-name '{}', {}".format(an, r)
        return(show_html(msg, sid))
    #endif

    output = display_asset(status["result"])
    output = display_buttons(api, sid, output)
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/transfer-asset/<sid>', method="get")
def transfer_asset(sid):
    output = "<br>Transfer Ownership of Song Asset<br>"
    output += form_header.format("do-transfer-asset", sid)
    output += form_row.format("Enter Asset Name", "asset-name")
    output += form_row.format("Enter username of new Owner", "owner-name")
    output += form_footer.format("Transfer Asset")
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/do-transfer-asset/<sid>', method="post")
def do_transfer_asset(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    an = bottle.request.forms.get("asset-name")
    on = bottle.request.forms.get("owner-name")

    #
    # Do an API assets/transfer call.
    #
    status = api.nexus_assets_transfer_asset_by_name(an, on)
    if (status.has_key("error")):
        r = status["error"]["message"]
        msg = "assets/transfer failed for asset-name '{}', {}".format(an, r)
        return(show_html(msg, sid))
    #endif
    output = "<br><b>Transfer Complete</b><br><br>"
    output += table_header1("Transfer Asset", "Transfer From Account",
        "Transfer To Account", "Txid")
    output += table_row(an, api.username, on, status["result"]["txid"])
    output += table_footer() + "<br>"

    output = display_buttons(api, sid, output)
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/create-token/<sid>', method="get")
def create_token(sid):
    output = "<br>Create Token<br>"
    output += form_header.format("do-create-token", sid)
    output += form_row.format("Enter Token Name", "token-name")
    output += form_row.format("Enter Token Supply", "token-supply")
    output += form_footer.format("Create Token")
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/do-create-token/<sid>', method="post")
def do_create_token(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    tn = bottle.request.forms.get("token-name")
    ts = bottle.request.forms.get("token-supply")

    #
    # Do an API tokens/create call.
    #
    status = api.nexus_tokens_create_token(tn, ts)
    if (status.has_key("error")):
        r = status["error"]["message"]
        msg = "tokens/create token failed, {}".format(r)
        return(show_html(msg, sid))
    #endif
    a = print_cour(status["result"]["address"])
    t = print_cour(status["result"]["txid"])
    tn = "<b>{}</b>".format(tn)
    output = "Token '{}' created".format(tn)
    output += table_header("", "")
    output += table_row("address", a)
    output += table_row("txid", t)
    output += table_footer()

    output = display_buttons(api, sid, output)
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

#
# display_token
#
# Display data from returned status["result"]
#
def display_token(tn, result):
    output = table_header("", "")
    output += table_row("Token Name:", tn)
    output += table_row("Token Max Supply:", result["maxsupply"])
    output += table_row("Token Current Supply:", result["currentsupply"])
    output += table_footer()
    return(output)
#enddef

@bottle.route('/lookup-token/<sid>', method="get")
def lookup_token(sid):
    output = "<br>Lookup Token<br>"
    output += form_header.format("do-lookup-token", sid)
    output += form_row.format("Enter Token Name", "token-name")
    output += form_footer.format("Lookup Token")
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/do-lookup-token/<sid>', method="post")
def do_lookup_token(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    tn = bottle.request.forms.get("token-name")

    #
    # Do an API assets/get call.
    #
    status = api.nexus_tokens_get_token_by_name(tn)
    if (status.has_key("error")):
        r = status["error"]["message"]
        msg = "tokens/get token failed for account '{}', {}".format(tn, r)
        return(show_html(msg, sid))
    #endif
    output = display_token(tn, status["result"])

    output = display_buttons(api, sid, output)
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/create-token-account/<sid>', method="get")
def create_token_account(sid):
    output = "<br>Create Token Account<br>"
    output += form_header.format("do-create-token-account", sid)
    output += form_row.format("Enter Token Name for Account", "token-name")
    output += form_row.format("Enter Token Supply for Account", "token-supply")
    output += form_footer.format("Create Token Account")
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/do-create-token-account/<sid>', method="post")
def do_create_token_account(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    tn = bottle.request.forms.get("token-name")
    ts = bottle.request.forms.get("token-supply")

    #
    # Do an API tokens/create call.
    #
    status = api.nexus_tokens_create_account(tn, ts)
    if (status.has_key("error")):
        r = status["error"]["message"]
        msg = "tokens/create account failed, {}".format(r)
        return(show_html(msg, sid))
    #endif
    a = print_cour(status["result"]["address"])
    t = print_cour(status["result"]["txid"])
    tn = "<b>{}</b>".format(tn)
    output = "Token Account '{}' created".format(tn)
    output += table_header("", "")
    output += table_row("address", a)
    output += table_row("txid", t)
    output += table_footer()

    output = display_buttons(api, sid, output)
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

#
# display_token_account
#
# Display data from returned status["result"]
#
def display_token_account(tn, result):
    output = table_header("", "")
    output += table_row("Token Account Name:", tn)
    output += table_row("Token Name:", result["token_name"])
    bal = "<b>{}</b>".format(result["balance"])
    output += table_row("Token Account Balance:", bal)
    output += table_footer()
    return(output)
#enddef

@bottle.route('/lookup-token-account/<sid>', method="get")
def lookup_token_account(sid):
    output = "<br>Lookup Token Account<br>"
    output += form_header.format("do-lookup-token-account", sid)
    output += form_row.format("Enter Token Account Name", "token-name")
    output += form_footer.format("Lookup Token Account")
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/do-lookup-token-account/<sid>', method="post")
def do_lookup_token_account(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    tn = bottle.request.forms.get("token-name")

    #
    # Do an API assets/get call.
    #
    status = api.nexus_tokens_get_account_by_name(tn)
    if (status.has_key("error")):
        r = status["error"]["message"]
        msg = "tokens/get account failed for account '{}', {}".format(tn, r)
        return(show_html(msg, sid))
    #endif
    output = display_token_account(tn, status["result"])

    output = display_buttons(api, sid, output)
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/debit-tokens/<sid>', method="get")
def debit_tokens(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    output = "Debit from Account <b>'{}'</b><br>".format(api.username)
    output += form_header.format("do-debit-tokens", sid)
    output += form_row.format("Enter Account Name to Receive Debit",
        "account-name1")
    output += form_row.format("Enter Debit Amount", "debit-amount1")
    output += form_row.format("Enter 2nd Account Name to Receive Debit",
        "account-name2")
    output += form_row.format("Enter Debit Amount", "debit-amount2")
    output += form_row.format("Enter 3rd Account Name to Receive Debit",
        "account-name3")
    output += form_row.format("Enter Debit Amount", "debit-amount3")
    output += form_footer.format("Debit Now")
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/do-debit-tokens/<sid>', method="post")
def do_debit_tokens(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    an1 = bottle.request.forms.get("account-name1")
    a1 = bottle.request.forms.get("debit-amount1")
    an2 = bottle.request.forms.get("account-name2")
    a2 = bottle.request.forms.get("debit-amount2")
    an3 = bottle.request.forms.get("account-name3")
    a3 = bottle.request.forms.get("debit-amount3")
    accounts = [ (an1, a1), (an2, a2), (an3, a3) ]

    #
    # Do an API assets/get call.
    #
    output = "<br><b>Debit Complete</b><br><br>"
    output += table_header1("Debit From Account", "Debit To Account", "Amount",
        "Txid")                            
    for account, amount in accounts:
        if (account == ""): continue
        status = api.nexus_tokens_debit_account_by_name(api.username, account,
            amount)
        if (status.has_key("error")):
            r = status["error"]["message"]
            msg = "debit failed for account '{}', {}".format(account, r)
            output = show_html(msg, sid)
            continue
        #endif
        txid = status["result"]["txid"]
        output += table_row(api.username, account, amount, txid)
        sleep()
    #endfor
    output += table_footer() + "<br>"

    output = display_buttons(api, sid, output)
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/credit-tokens/<sid>', method="get")
def credit_tokens(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)
    
    output = "Credit to Account <b>'{}'</b><br>".format(api.username)
    output += form_header.format("do-credit-tokens", sid)
    output += form_row.format("Enter Token Amount to Credit", "amount")
    output += form_row.format("Enter Transaction-ID of Debit", "txid")
    output += form_footer.format("Credit Now")
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

@bottle.route('/do-credit-tokens/<sid>', method="post")
def do_credit_tokens(sid):
    api, output = get_session_id(sid)
    if (api == None): return(output)

    amount = bottle.request.forms.get("amount")
    txid = bottle.request.forms.get("txid")

    #
    # Do an API assets/get call.
    #
    status = api.nexus_tokens_credit_account_by_name(api.username, amount,
        txid, None)
    if (status.has_key("error")):
        r = status["error"]["message"]
        msg = "credit failed for account '{}', {}".format(api.username, r)
        output = show_html(msg, sid)
        return(output)
    #endif
    output = "<br><b>Credit Complete</b><br><br>"
    output += table_header1("Credit To Account", "Amount", "Txid")
    output += table_row(api.username, amount, status["result"]["txid"])
    output += table_footer() + "<br>"

    output = display_buttons(api, sid, output)
    output = mmp_page_logged_in.format(sid, output)
    return(output)
#enddef

#
# create_music_tokens
#
# Create Token music-token to be used for this application. If it fails, it is
# likely either the nexus daemon is not running or this app is restarting and
# the token has already been created. Reserve 1000 tokens.
#
def create_music_tokens():
    global admin_account
    
    #
    # Need to create/use an account to create the token.
    #
    api = nexus.sdk_init("admin-admin", "pw", "1234")
    admin_account = api
    login = api.nexus_users_login_user()
    if (login.has_key("error")):
        create = api.nexus_users_create_user()
        if (create.has_key("error")):
            if (api.nexus_users_login_user().has_key("error")):
                print "Account create for 'admin' failed, exit"
                exit(1)
            #endif
        #endif
        print "Created user account 'admin'"
        sleep()
        login = api.nexus_users_login_user()
    #endif
    print "User account 'admin' logged in"

    #
    # Create the token if it does not exist.
    #
    get = api.nexus_tokens_get_token_by_name("music-token")
    if (get.has_key("error")):
        create = api.nexus_tokens_create_token("music-token", 1000)
        if (create.has_key("error")):
            r = create["error"]["message"]
            print "tokens/create token failed, {}".format(r)
            exit(1)
        #endif
        print "Created 1000 music tokens"
        sleep()
    #endif

    #
    # Move some tokens to the 'admin' account. Create a token account first.
    #
    admin = "admin-admin"
    get = api.nexus_tokens_get_account_by_name(admin)
    if (get.has_key("error") == False):
        b = get["result"]["balance"]
        print "Token account 'admin' exists, balance is {} tokens".format(b)
    else:
        create = api.nexus_tokens_create_account(admin, "music-token")
        if (create.has_key("error")):
            r = create["error"]["message"]
            print "tokens/create token account failed, {}".format(r)
            exit(1)
        #endif
        sleep()
        debit = api.nexus_tokens_debit_token_by_name("music-token", admin, 100)
        if (debit.has_key("error")):
            r = debit["error"]["message"]
            print "tokens/debit failed for 'admin', {}".format(r)
            exit(1)
        #endif
    #endif
#enddef

#------------------------------------------------------------------------------

#
# ---------- Main program entry point. ----------
#
date = commands.getoutput("date")
print "mmp starting up at {}".format(date)

#
# Create some token reserves.
#
create_music_tokens()

#
# Read previously created assets from disk.
#
read_assets()

#
# Run web server.
#
bottle.run(host="0.0.0.0", port=mmp_port, debug=True)
exit(0)

#------------------------------------------------------------------------------


