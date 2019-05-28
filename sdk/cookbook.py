#!/usr/bin/env python
#
# cookbook.py - Nexus API Lesson Cook Book
#
# This program teaches you how to use the Nexus APIs. It is an interactive
# walk through of each API that is supported by Nexus Tritium and what
# parameters you need to for a restful call.
#
# The user interface will also use the python SDK but allows the user to
# curl directly to any Nexus node on the network.
#
# Usage: python cookbook.py [<port>]
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
import json

try:
    import nexus_sdk as nexus
except:
    print "Need to place nexus_sdk.py in this directory"
    exit(0)
#endtry

#
# Did command line have a port number to listen to?
#
cookbook_port = int(sys.argv[1]) if (len(sys.argv) == 2) else 5432

#
# Nexus node to query for API. A change to the SDK URL needs a call to
# nexus_sdk.change_sdk_url().
#
nexus_api_node = "http://localhost:8080"
nexus_sdk_node = "http://localhost:8080"

#------------------------------------------------------------------------------

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

def bold(string):                                                              
    string = string.replace("[1m", "<b>")                                      
    string = string.replace("[0m", "</b>")                                     
    return(string)                                                             
#enddef

#
# curl
#
# Call curl and return json.
#
def curl(api_command):
    global nexus_api_node
    
    url = "{}/{}".format(nexus_api_node, api_command)
    output = commands.getoutput('curl --silent "{}"'.format(url))
    if (output == "" or output == None):
        return({"error" : "curl failed, nexus daemon may not be running"})
    #endif
    return(json.loads(output))
#enddef

#
# sid_to_sdd
#
# To keep session continuity across web pages.
#
contexts = {}

def sid_to_sdk(sid):
    global contexts

    if (contexts.has_key(sid)): return(contexts[sid], "")
    output = ('"error": Login session {} does not exist - ' + \
        'login using users/login/user and click the SDK button').format(sid)
    return(None, show(output))
#enddef

#
# format_transactions
#
# Put a line per array element so the eyes can parse each transaction record
# just a tad better.
#
def format_transactions(data):
    if (data.has_key("result") == False): return(json.dumps(data))
    if (data["result"] == None): return('{"error": "no json returned"}')

    output = ""
    for tx in data["result"]:
        output += json.dumps(tx) + "<br><br>"
    #endfor
    return(output)
#enddef

#
# no_parms
#
# Checks that any supplied arg in the variable list of arguments is "" or None.
#
def no_parms(*args):
    for a in args:
        if (a == "" or a == None): return(True)
    #endfor
    return(False)
#enddef

#
# add_block_row
#
# Put blank line between token tokens and token accounts.
#
def add_blank_row(o):
    o += "<tr><td>"
    o += "&nbsp;"
    o += "</td></tr>"
    return(o)
#enddef

#------------------------------------------------------------------------------

show_html = '''
<br><table align="left" style="word-break:break-all;">
<tr><td>{}</td></tr>
</table><br>&nbsp;<br><hr>
'''

#
# show
#
# This is JSON output returned from Nexus API.
#
def show(msg, sid="", genid=""):
    msg = red(msg) if (msg.find('"error":') != -1) else green(msg)
    output = show_html.format(bold(msg)) + build_body_page(sid, genid)

    return(landing_page.format(output))
#enddef

#
# Wrapper to make all web pages look alike, from a format perspective.
#
landing_page = '''
    <html>
    <title>Nexus Interactive SDK/API Cook Book</title>
    <body bgcolor="gray">
    <div style="margin:20px;background-color:#F5F5F5;padding:15px;
         border-radius:20px;border:5px solid #666666;">
    <font face="verdana"><center>
    <br><head><a href="/" style="text-decoration:none;"><font color="black">
    <b>Nexus Interactive SDK/API Cook Book</b></a></head><br><br><hr>
    {}
    <hr></center></font></body></html>
'''

#
# Used as a generic template for all API calls. Used for form input from
# the web interface.
#
form_header = '<form action="/{}" method="post">'
form_parm = '<input type="text" name="{}" value="{}" size="15" />'
form_footer = '''
&nbsp;&nbsp;&nbsp;&nbsp;
<input type="submit" value="SDK" name="action" />
&nbsp;
<input type="submit" value="API" name="action" /> 
</form>
'''

#------------------------------------------------------------------------------

#
# build_url_html
#
# Put in two form lines for user to change SDK or API URL.
#
def build_url_html(o):
    global nexus_api_node, nexus_sdk_node

    o += "<br><b>Nexus Node URLs</b><br><br><table>"
    o += '''
        <form action="/url/sdk" method="post">
        SDK:&nbsp;
        <input type="text" name="url" value="{}" size="30" />
        &nbsp;
        <input type="submit" value="Change URL" name="action" />
        </form>

        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;

        <form action="/url/api" method="post">
        API:&nbsp;
        <input type="text" name="url" value="{}" size="30" />
        &nbsp;
        <input type="submit" value="Change URL" name="action" />
        </form>
        <br><br><hr size="5">
    '''.format(nexus_sdk_node, nexus_api_node)
    return(o)
#enddef

#
# build_system_html
#
system_get_info = '{}system/get/info{}'
system_list_peers = '{}system/list/peers{}'
system_list_lisp_eids = '{}system/list/lisp-eids{}'

def build_system_html(sid, genid, o):
    f = form_footer
    o += "<br><b>System API</b><br><br><table>"

    o += "<tr><td>"
    h = form_header.format("system-get-info")
    o += system_get_info.format(h, "<br><br>" + f)
    o += "</td>"
    o += "<td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>"

    o += "<td>"
    h = form_header.format("system-list-peers")
    o += system_list_peers.format(h, "<br><br>" + f)
    o += "</td>"
    o += "<td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>"

    o += "<td>"
    h = form_header.format("system-list-lisp-eids")
    o += system_list_lisp_eids.format(h, "<br><br>" + f)
    o += "</td></tr>"

    o += '</table><br><hr size="5">'
    return(o)
#enddef

@bottle.route('/system-get-info', method="post")
def do_system_get_info():
    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk = nexus.sdk_init("system", "get/info", "")
        if (sdk == None): return(show(red("Could not initialize SDK")))
        output = sdk.nexus_system_get_info()
        del(sdk)
    else:
        output = curl(system_get_info.format("", ""))
    #endif            
    output = json.dumps(output)
    return(show(output))
#enddef

@bottle.route('/system-list-peers', method="post")
def do_system_list_peers():
    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk = nexus.sdk_init("system", "list/peers", "")
        if (sdk == None): return(show(red("Could not initialize SDK")))
        output = sdk.nexus_system_list_peers()
        del(sdk)
    else:
        output = curl(system_list_peers.format("", ""))
    #endif            
    output = json.dumps(output)
    return(show(output))
#enddef

@bottle.route('/system-list-lisp-eids', method="post")
def do_system_list_lisp_eids():
    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk = nexus.sdk_init("system", "lisp/lisp-eids", "")
        if (sdk == None): return(show(red("Could not initialize SDK")))
        output = sdk.nexus_system_list_lisp_eids()
        del(sdk)
    else:
        output = curl(system_list_lisp_eids.format("", ""))
    #endif            
    output = json.dumps(output)
    return(show(output))
#enddef

#------------------------------------------------------------------------------

#
# build_users_html
#
users_create_user = '{}users/create/user?username={}&password={}&pin={}{}'
users_login_user = '{}users/login/user?username={}&password={}&pin={}{}'
users_logout_user = '{}users/logout/user?session={}{}'
users_lock_user = '{}users/lock/user?session={}{}'
users_unlock_user = '{}users/unlock/user?session={}&pin={}{}'
users_list_transactions = \
    '{}users/list/transactions?genesis={}&page={}&limit={}&verbose={}{}'
users_list_notifications = \
    '{}users/list/notifications?genesis={}&page={}&limit={}{}'
users_list_assets = '{}users/list/assets?genesis={}&page={}&limit={}{}'
users_list_tokens = '{}users/list/tokens?genesis={}{}'
users_list_accounts = '{}users/list/accounts?genesis={}{}'

def build_users_html(sid, genid, o):
    f = form_footer
    o += "<br><b>Users API</b><br><br><table>"

    o += "<tr><td>"
    h = form_header.format("users-login-user")
    username = form_parm.format("username", "")
    password = form_parm.format("password", "")
    pin = form_parm.format("pin", "")
    o += users_login_user.format(h, username, password, pin, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("users-logout-user")
    session = form_parm.format("session", sid)
    o += users_logout_user.format(h, session, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("users-create-user")
    username = form_parm.format("username", "")
    password = form_parm.format("password", "")
    pin = form_parm.format("pin", "")
    o += users_create_user.format(h, username, password, pin, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("users-lock-user")
    session = form_parm.format("session", sid)
    o += users_lock_user.format(h, session, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("users-unlock-user")
    session = form_parm.format("session", sid)
    pin = form_parm.format("pin", "")
    o += users_unlock_user.format(h, session, pin, f)
    o += "</td></tr>"

    o = add_blank_row(o)

    o += "<tr><td>"
    h = "users-list-transactions"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    genesis = form_parm.format("genesis", genid)
    page = form_parm.format("page", "")
    limit = form_parm.format("limit", "")
    verbose = form_parm.format("verbose", "")
    o += users_list_transactions.format(h,  genesis, page, limit, verbose, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "users-list-notifications"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    genesis = form_parm.format("genesis", genid)
    page = form_parm.format("page", "")
    limit = form_parm.format("limit", "")
    o += users_list_notifications.format(h, genesis, page, limit, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "users-list-assets"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    genesis = form_parm.format("genesis", genid)
    page = form_parm.format("page", "")
    limit = form_parm.format("limit", "")
    o += users_list_assets.format(h, genesis, page, limit, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "users-list-tokens"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    genesis = form_parm.format("genesis", genid)
    page = form_parm.format("page", "")
    limit = form_parm.format("limit", "")
    o += users_list_tokens.format(h, genesis, page, limit, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "users-list-accounts"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    genesis = form_parm.format("genesis", genid)
    page = form_parm.format("page", "")
    limit = form_parm.format("limit", "")
    o += users_list_accounts.format(h, genesis, page, limit, f)
    o += "</td></tr>"

    o += '</table><br><hr size="5">'
    return(o)
#enddef

@bottle.route('/users-create-user', method="post")
def do_users_create_user():
    username = bottle.request.forms.get("username")
    password = bottle.request.forms.get("password")
    pin = bottle.request.forms.get("pin")
    if (no_parms(username, password, pin)):
        m = red("users/create/user needs more input parameters")
        return(show(m))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk = nexus.sdk_init(username, password, pin)
        output = sdk.nexus_users_create_user()
    else:
        output = curl(users_create_user.format("", username, password, pin,
            ""))
    #endif            
    output = json.dumps(output)
    return(show(output))
#enddef

@bottle.route('/users-login-user', method="post")
def do_users_login_user():
    global contexts
    
    username = bottle.request.forms.get("username")
    password = bottle.request.forms.get("password")
    pin = bottle.request.forms.get("pin")
    if (no_parms(username, password, pin)):
        m = red("users/login/user needs more input parameters")
        return(show(m))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk = nexus.sdk_init(username, password, pin)
        output = sdk.nexus_users_login_user()
        sid, genid = [sdk.session_id, sdk.genesis_id]
        contexts[sid] = sdk
    else:
        output = curl(users_login_user.format("", username, password, pin, ""))
        sid = output["result"]["session"] if output.has_key("result") else ""
        genid = output["result"]["genesis"] if output.has_key("result") else ""
    #endif
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/users-logout-user', method="post")
def do_users_logout_user():
    session = bottle.request.forms.get("session")
    if (no_parms(session)):
        m = red("users/logout/user needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_users_logout_user()
        genid = sdk.genesis_id
    else:
        output = curl(users_logout_user.format("", session, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/users-lock-user', method="post")
def do_users_lock_user():
    session = bottle.request.forms.get("session")
    if (no_parms(session)):
        m = red("users/lock/user needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_users_lock_user()
        genid = sdk.genesis_id
    else:
        output = curl(users_lock_user.format("", session, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/users-unlock-user', method="post")
def do_users_unlock_user():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    if (no_parms(pin, session)):
        m = red("users/unlock/user needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_users_unlock_user()
        genid = sdk.genesis_id
    else:
        output = curl(users_unlock_user.format("", session, pin, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/users-list-transactions', method="post")
@bottle.route('/users-list-transactions/<sid>', method="post")
def do_users_list_transactions(sid=""):
    genesis = bottle.request.forms.get("genesis")
    page = bottle.request.forms.get("page")
    limit = bottle.request.forms.get("limit")
    verbose = bottle.request.forms.get("verbose")
    if (no_parms(genesis, page, limit, verbose)):
        m = red("users/list/transactions needs more input parameters")
        return(show(m, sid, genesis))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_users_list_transactions_by_genesis(page, limit,
            verbose)
    else:
        output = curl(users_list_transactions.format("", genesis, page, limit,
            verbose, ""))
    #endif            
    output = format_transactions(output)

    return(show(output, sid, genesis))
#enddef

@bottle.route('/users-list-notifications', method="post")
@bottle.route('/users-list-notifications/<sid>', method="post")
def do_users_list_notifications(sid=""):
    genesis = bottle.request.forms.get("genesis")
    page = bottle.request.forms.get("page")
    limit = bottle.request.forms.get("limit")
    if (no_parms(genesis, page, limit)):
        m = red("users/list/notifications needs more input parameters")
        return(show(m, sid, genesis))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_users_list_notifications_by_genesis(page, limit)
    else:
        output= curl(users_list_notifications.format("", genesis, page, limit,
            ""))
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genesis))
#enddef

@bottle.route('/users-list-assets', method="post")
@bottle.route('/users-list-assets/<sid>', method="post")
def do_users_list_assets(sid=""):
    genesis = bottle.request.forms.get("genesis")
    page = bottle.request.forms.get("page")
    limit = bottle.request.forms.get("limit")
    if (no_parms(genesis, page, limit)):
        m = red("users/list/assets needs more input parameters")
        return(show(m, sid, genesis))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_users_list_assets_by_genesis(page, limit)
    else:
        output= curl(users_list_notifications.format("", genesis, page, limit,
            ""))
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genesis))
#enddef

@bottle.route('/users-list-tokens', method="post")
@bottle.route('/users-list-tokens/<sid>', method="post")
def do_users_list_tokens(sid=""):
    genesis = bottle.request.forms.get("genesis")
    page = bottle.request.forms.get("page")
    limit = bottle.request.forms.get("limit")
    if (no_parms(genesis, page, limit)):
        m = red("users/list/tokens needs more input parameters")
        return(show(m, sid, genesis))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_users_list_tokens_by_genesis(page, limit)
    else:
        output= curl(users_list_notifications.format("", genesis, page, limit,
            ""))
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genesis))
#enddef

@bottle.route('/users-list-accounts', method="post")
@bottle.route('/users-list-accounts/<sid>', method="post")
def do_users_list_accounts(sid=""):
    genesis = bottle.request.forms.get("genesis")
    page = bottle.request.forms.get("page")
    limit = bottle.request.forms.get("limit")
    if (no_parms(genesis, page, limit)):
        m = red("users/list/accounts needs more input parameters")
        return(show(m, sid, genesis))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_users_list_accounts_by_genesis(page, limit)
    else:
        output= curl(users_list_notifications.format("", genesis, page, limit,
            ""))
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genesis))
#enddef

#------------------------------------------------------------------------------

#
# build_supply_html
#
supply_create_item = \
    '{}supply/create/item?pin={}&session={}&name={}&data={}{}'
supply_get_item_name = '{}supply/get/item?name={}{}'
supply_get_item_address = '{}supply/get/item?address={}{}'
supply_update_item = \
    '{}supply/update/item?pin={}&session={}&address={}&data={}{}'
supply_transfer_item = ('{}supply/transfer/item?pin={}&session={}' + \
    '&username={}&address={}&destination={}{}')
supply_claim_item = '{}supply/claim/item?pin={}&session={}&txid={}{}'
supply_list_item_history_name = '{}supply/list/item/history?name={}{}'
supply_list_item_history_address = '{}supply/list/item/history?address={}{}'

def build_supply_html(sid, genid, o):
    f = form_footer
    o += "<br><b>Supply Chain API</b><br><br><table>"

    o += "<tr><td>"
    h = "supply-get-item-name"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    name = form_parm.format("name", "")
    o += supply_get_item_name.format(h, name, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "supply-get-item-address"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    address = form_parm.format("address", "")
    o += supply_get_item_address.format(h, address, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("supply-create-item")
    pin = form_parm.format("pin", "")
    name = form_parm.format("name", "")
    data = form_parm.format("data", "")
    session = form_parm.format("session", sid)
    o += supply_create_item.format(h, pin, session, name, data, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("supply-update-item")
    pin = form_parm.format("pin", "")
    data = form_parm.format("data", "")
    session = form_parm.format("session", sid)
    address = form_parm.format("address", "")
    o += supply_update_item.format(h, pin, session, address, data, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("supply-transfer-item")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    username = form_parm.format("username", "")
    address = form_parm.format("address", "")
    d = form_parm.format("destination", "")
    o += supply_transfer_item.format(h, pin, session, username, address, d, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("supply-claim-item")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    txid = form_parm.format("txid", "")
    o += supply_claim_item.format(h, pin, session, txid, f)
    o += "</td></tr>"

    o = add_blank_row(o)

    o += "<tr><td>"
    h = "supply-list-item-history-name"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    name = form_parm.format("name", "")
    o += supply_list_item_history_name.format(h, name, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "supply-list-item-history-address"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    address = form_parm.format("address", "")
    o += supply_list_item_history_address.format(h, address, f)
    o += "</td></tr>"

    o += '</table><br><hr size="5">'
    return(o)
#enddef

@bottle.route('/supply-create-item', method="post")
def do_supply_create_item():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    name = bottle.request.forms.get("name")
    data = bottle.request.forms.get("data")
    if (no_parms(pin, session, data)):
        m = red("supply/create/item needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_supply_create_item(name, data)
        genid = sdk.genesis_id
    else:
        output = curl(supply_create_item.format("", pin, session, name, data,
            ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/supply-get-item-name', method="post")
@bottle.route('/supply-get-item-name/<sid>', method="post")
def do_supply_get_item_name(sid=""):
    name = bottle.request.forms.get("name")
    if (no_parms(name)):
        m = red("supply/get/item needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_supply_get_item_by_name(name)
        genid = sdk.genesis_id
    else:
        output = curl(supply_get_item_name.format("", name, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/supply-get-item-address', method="post")
@bottle.route('/supply-get-item-address/<sid>', method="post")
def do_supply_get_item_address(sid=""):
    address = bottle.request.forms.get("address")
    if (no_parms(address)):
        m = red("supply/get/item needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_supply_get_item_by_address(address)
        genid = sdk.genesis_id
    else:
        output = curl(supply_get_item_address.format("", address, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/supply-update-item', method="post")
def do_supply_update_item():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    address = bottle.request.forms.get("address")
    data = bottle.request.forms.get("data")
    if (no_parms(pin, session, address, data)):
        m = red("supply/update/item needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_supply_update_item(address, data)
        genid = sdk.genesis_id
    else:
        output = curl(supply_update_item.format("", pin, session, address,
            data, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/supply-transfer-item', method="post")
def do_supply_transfer_item():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    username = bottle.request.forms.get("username")
    address = bottle.request.forms.get("address")
    destination = bottle.request.forms.get("destinations")
    if (no_parms(pin, session, username, address, destination)):
        m = red("supply/transfer/item needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_supply_transfer_item(address, destination)
        genid = sdk.genesis_id
    else:
        output = curl(supply_transfer_item.format("", pin, session, username,
            address, destination, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/supply-claim-item', method="post")
def do_supply_claim_item():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    txid = bottle.request.forms.get("txid")
    if (no_parms(pin, session, txid)):
        m = red("supply/claim/item needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_supply_claim_item(txid)
        genid = sdk.genesis_id
    else:
        output = curl(supply_claim_item.format("", pin, session, txid, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/supply-list-item-history-name', method="post")
@bottle.route('/supply-list-item-history-name/<sid>', method="post")
def do_supply_list_item_history_name(sid=""):
    name = bottle.request.forms.get("name")
    if (no_parms(name)):
        m = red("supply/list/item/history needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_supply_list_item_history_by_name(name)
        genid = sdk.genesis_id
    else:
        output = curl(supply_list_item_history_name.format("", name, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/supply-list-item-history-address', method="post")
@bottle.route('/supply-list-item-history-address/<sid>', method="post")
def do_supply_list_item_history_address(sid=""):
    address = bottle.request.forms.get("address")
    if (no_parms(address)):
        m = red("supply/list/item/history needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_supply_list_item_history_by_address(address)
        genid = sdk.genesis_id
    else:
        output = curl(supply_list_item_history_address.format("", address, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

#------------------------------------------------------------------------------

#
# build_assets_html
#
assets_create_asset = \
    '{}assets/create/asset?pin={}&session={}&name={}&format=raw&data={}{}'
assets_get_asset_name = '{}assets/get/asset?name={}{}'
assets_get_asset_address = '{}assets/get/asset?address={}{}'
assets_update_asset = \
    '{}assets/update/asset?pin={}&session={}&address={}&data={}{}'
assets_transfer_asset = \
    '{}assets/transfer/asset?pin={}&session={}&username={}&name={}{}'
assets_claim_asset = '{}assets/claim/asset?pin={}&session={}&txid={}{}'
assets_tokenize_asset = \
    '{}assets/tokenize/asset?pin={}&session={}&token_name={}&asset_name={}{}'
assets_list_asset_history_name = '{}assets/list/asset/history?name={}{}'
assets_list_asset_history_address = '{}assets/list/asset/history?address={}{}'

def build_assets_html(sid, genid, o):
    f = form_footer
    o += "<br><b>Assets API</b><br><br><table>"

    o += "<tr><td>"
    h = "assets-get-asset-name"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    name = form_parm.format("name", "")
    o += assets_get_asset_name.format(h, name, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "assets-get-asset-address"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    address = form_parm.format("address", "")
    o += assets_get_asset_address.format(h, address, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("assets-create-asset")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    data = form_parm.format("data", "")
    name = form_parm.format("name", "")
    o += assets_create_asset.format(h, pin, session, name, data, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("assets-update-asset")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    data = form_parm.format("data", "")
    address = form_parm.format("address", sid)
    o += assets_update_asset.format(h, pin, session, data, address, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("assets-transfer-asset")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    username = form_parm.format("username", "")
    name = form_parm.format("name", "")
    o += assets_transfer_asset.format(h, pin, session, username, name, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("assets-claim-asset")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    txid = form_parm.format("txid", "")
    o += assets_claim_asset.format(h, pin, session, txid, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("assets-tokenize-asset")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    token_name = form_parm.format("token_name", "")
    asset_name = form_parm.format("asset_name", "")
    o += assets_tokenize_asset.format(h, pin, session, token_name, asset_name,
        f)
    o += "</td></tr>"

    o = add_blank_row(o)

    o += "<tr><td>"
    h = "assets-list-asset-history-name"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    name = form_parm.format("name", "")
    o += assets_list_asset_history_name.format(h, name, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "assets-list-asset-history-address"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    address = form_parm.format("address", "")
    o += assets_list_asset_history_address.format(h, address, f)
    o += "</td></tr>"

    o += '</table><br><hr size="5">'
    return(o)
#enddef

@bottle.route('/assets-create-asset', method="post")
def do_assets_create_asset():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    name = bottle.request.forms.get("name")
    data = bottle.request.forms.get("data")
    if (no_parms(pin, session, name, data)):
        m = red("assets/create/asset needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_assets_create_asset(name, data)
        genid = sdk.genesis_id
    else:
        output = curl(assets_create_asset.format("", pin, session, name, data,
            ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/assets-update-asset', method="post")
def do_assets_update_asset():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    address = bottle.request.forms.get("address")
    data = bottle.request.forms.get("data")
    if (no_parms(pin, session, address, data)):
        m = red("assets/update/asset needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_assets_update_asset_by_address(address, data)
        genid = sdk.genesis_id
    else:
        output = curl(assets_update_asset.format("", pin, session, address,
            data, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/assets-get-asset-name', method="post")
@bottle.route('/assets-get-asset-name/<sid>', method="post")
def do_assets_get_asset_name(sid=""):
    name = bottle.request.forms.get("name")
    if (no_parms(name)):
        m = red("assets/get/asset needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_assets_get_asset_by_name(name)
        genid = sdk.genesis_id
    else:
        output = curl(assets_get_asset_name.format("", name, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/assets-get-asset-address', method="post")
@bottle.route('/assets-get-asset-address/<sid>', method="post")
def do_assets_get_asset_address(sid=""):
    address = bottle.request.forms.get("address")
    if (no_parms(address)):
        m = red("assets/get/asset needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_assets_get_asset_by_address(address)
        genid = sdk.genesis_id
    else:
        output = curl(assets_get_asset_address.format("", address, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/assets-transfer-asset', method="post")
def do_assets_transfer_asset():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    username = bottle.request.forms.get("username")
    name = bottle.request.forms.get("name")
    if (no_parms(pin, session, username, name)):
        m = red("assets/transfer/asset needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_assets_transfer_asset_by_name(name, username)
        genid = sdk.genesis_id
    else:
        output = curl(assets_transfer_asset.format("", pin, session, username,
            name, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/assets-claim-asset', method="post")
def do_assets_claim_asset():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    txid = bottle.request.forms.get("txid")
    if (no_parms(session, txid)):
        m = red("assets/claim/asset needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_assets_claim_asset(txid)
        genid = sdk.genesis_id
    else:
        output = curl(assets_claim_asset.format("", pin, session, txid, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/assets-tokenize-asset', method="post")
def do_assets_tokenize_asset():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    token_name = bottle.request.forms.get("token_name")
    asset_name = bottle.request.forms.get("asset_name")
    if (no_parms(pin, session, token_name, asset_name)):
        m = red("assets/tokenize/asset needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_assets_tokenize_asset_by_name(asset_name,
            token_name)
        genid = sdk.genesis_id
    else:
        output = curl(assets_tokenize_asset.format("", pin, session,
            token_name, asset_name, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/assets-list-asset-history-name', method="post")
@bottle.route('/assets-list-asset-history-name/<sid>', method="post")
def do_assets_list_asset_history_name(sid=""):
    name = bottle.request.forms.get("name")
    if (no_parms(name)):
        m = red("assets/list/asset/history needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_assets_list_asset_history_by_name(name)
        genid = sdk.genesis_id
    else:
        output = curl(assets_list_asset_history_name.format("", name, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/assets-list-asset-history-address', method="post")
@bottle.route('/assets-list-asset-history-address/<sid>', method="post")
def do_assets_list_asset_history_address(sid=""):
    address = bottle.request.forms.get("address")
    if (no_parms(address)):
        m = red("assets/list/asset/history needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_assets_list_asset_history_by_address(address)
        genid = sdk.genesis_id
    else:
        output = curl(assets_list_asset_history_address.format("", address,
            ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

#------------------------------------------------------------------------------

#
# build_accounts_html
#
tokens_create_token = \
    '{}tokens/create/token?pin={}&session={}&name={}&supply={}{}'
tokens_create_account = \
    '{}tokens/create/account?pin={}&session={}&name={}&token_name={}{}'
tokens_get_token = '{}tokens/get/token?name={}{}'
tokens_get_account = '{}tokens/get/account?name={}{}'
tokens_debit_token = ('{}tokens/debit/token?pin={}&session={}&name={}' + \
    '&name_to={}&amount={}{}')
tokens_credit_token = ('{}tokens/credit/token?pin={}&session={}&name={}' + \
    '&amount={}&txid={}{}')
tokens_debit_account = ('{}tokens/debit/account?pin={}&session={}' + \
    '&amount={}&name={}&name_to={}{}')
tokens_credit_account = ('{}tokens/credit/account?pin={}&session={}' + \
    '&txid={}&amount={}&name={}&name_proof={}{}')

def build_tokens_html(sid, genid, o):
    f = form_footer
    o += "<br><b>Tokens API</b><br><br><table>"

    o += "<tr><td>"
    h = "tokens-get-token"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    name = form_parm.format("name", "")
    o += tokens_get_token.format(h, name, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("tokens-create-token")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    name = form_parm.format("name", "")
    supply = form_parm.format("supply", "")
    o += tokens_create_token.format(h, pin, session, name, supply, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("tokens-debit-token")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    amount = form_parm.format("amount", "")
    name = form_parm.format("name", "")
    name_to = form_parm.format("name_to", "")
    o += tokens_debit_token.format(h, pin, session, name, name_to, amount, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("tokens-credit-token")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    name = form_parm.format("name", "")
    amount = form_parm.format("amount", "")
    txid = form_parm.format("txid", "")
    o += tokens_credit_token.format(h, pin, session, name, amount, txid, f)
    o += "</td></tr>"

    o = add_blank_row(o)

    o += "<tr><td>"
    h = "tokens-get-account"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    name = form_parm.format("name", "")
    o += tokens_get_account.format(h, name, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("tokens-create-account")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    name = form_parm.format("name", "")
    token_name = form_parm.format("token_name", "")
    o += tokens_create_account.format(h, pin, session, name, token_name, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("tokens-debit-account")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    amount = form_parm.format("amount", "")
    name = form_parm.format("name", "")
    name_to = form_parm.format("name_to", "")
    o += tokens_debit_account.format(h, pin, session, amount, name, name_to, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("tokens-credit-account")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    txid = form_parm.format("txid", "")
    amount = form_parm.format("amount", "")
    name = form_parm.format("name", "")
    proof = form_parm.format("name_proof", "")
    o += tokens_credit_account.format(h, pin, session, txid, amount, name,
        proof, f)
    o += "</td></tr>"

    o += '</table><br><hr size="5">'
    return(o)
#enddef

@bottle.route('/tokens-create-token', method="post")
def do_tokens_create_token():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    name = bottle.request.forms.get("name")
    supply = bottle.request.forms.get("supply")
    if (no_parms(pin, session, name, supply)):
        m = red("tokens/create/token needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_tokens_create_token(name, supply)
        genid = sdk.genesis_id
    else:
        output = curl(tokens_create_token.format("", pin, session, name,
            supply, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/tokens-create-account', method="post")
def do_tokens_create_account():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    name = bottle.request.forms.get("name")
    token_name = bottle.request.forms.get("token_name")
    if (no_parms(pin, session, name, token_name)):
        m = red("tokens/create/account needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_tokens_create_account(name, token_name)
        genid = sdk.genesis_id
    else:
        output = curl(tokens_create_account.format("", pin, session, name, 
            token_name, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/tokens-get-token', method="post")
@bottle.route('/tokens-get-token/<sid>', method="post")
def do_tokens_get_token(sid=""):
    name = bottle.request.forms.get("name")
    if (no_parms(name)):
        m = red("tokens/get/token needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_tokens_get_token_by_name(name)
        genid = sdk.genesis_id
    else:
        output = curl(tokens_get_token.format("", name, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/tokens-get-account', method="post")
@bottle.route('/tokens-get-account/<sid>', method="post")
def do_tokens_get_account(sid=""):
    name = bottle.request.forms.get("name")
    if (no_parms(name)):
        m = red("tokens/get/account needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_tokens_get_account_by_name(name)
        genid = sdk.genesis_id
    else:
        output = curl(tokens_get_account.format("", name, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/tokens-debit-token', method="post")
def do_tokens_debit_token():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    amount = bottle.request.forms.get("amount")
    name = bottle.request.forms.get("name")
    name_to = bottle.request.forms.get("name_to")
    if (no_parms(pin, session, amount, name, name_to)):
        return(show(red("tokens/debit/token needs more input parameters"),
            session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_tokens_debit_token_by_name(name, name_to, amount)
        genid = sdk.genesis_id
    else:
        output = curl(tokens_debit_token.format("", pin, session, name,
            name_to, amount, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/tokens-credit-token', method="post")
def do_tokens_credit_token():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    amount = bottle.request.forms.get("amount")
    name = bottle.request.forms.get("name")
    txid = bottle.request.forms.get("txid")
    if (no_parms(pin, session, name, amount, txid)):
        m = red("tokens/credit/token needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_tokens_credit_token_by_name(name, amount, txid)
        genid = sdk.genesis_id
    else:
        output = curl(tokens_debit_account.format("", pin, session, name,
            amount, txid, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/tokens-debit-account', method="post")
def do_tokens_debit_account():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    amount = bottle.request.forms.get("amount")
    name = bottle.request.forms.get("name")
    name_to = bottle.request.forms.get("name_to")
    if (no_parms(pin, session, amount, name, name_to)):
        m = red("tokens/debit/account needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_tokens_debit_account_by_name(name, name_to, amount)
        genid = sdk.genesis_id
    else:
        output = curl(tokens_debit_account.format("", pin, session, amount,
            name, name_to, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/tokens-credit-account', method="post")
def do_tokens_credit_account():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    txid = bottle.request.forms.get("txid")
    amount = bottle.request.forms.get("amount")
    name = bottle.request.forms.get("name")
    name_proof = bottle.request.forms.get("name_proof")
    if (no_parms(pin, session, txid, amount, name, name_proof)):
        m = red("tokens/credit/account needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_tokens_credit_account_by_name(name, amount, txid)
        genid = sdk.genesis_id
    else:
        output = curl(tokens_credit_account.format("", pin, session, txid,
            amount, name, name_proof, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

#------------------------------------------------------------------------------

#
# build_finance_html
#
finance_create_account = '{}finance/create/account?pin={}&session={}&name={}{}'
finance_get_account_name = '{}finance/get/account?name={}{}'
finance_get_account_address = '{}finance/get/account?address={}{}'
finance_debit_account = ('{}finance/debit/account?pin={}&session={}' + \
    '&amount={}&name_from={}&name_to={}{}')
finance_credit_account = ('{}finance/credit/account?pin={}&session={}' + \
    '&txid={}&amount={}&name_to={}&name_proof={}{}')
finance_list_accounts = '{}finance/list/accounts?session={}{}'

def build_finance_html(sid, genid, o):
    f = form_footer
    o += "<br><b>Finance API</b><br><br><table>"

    o += "<tr><td>"
    h = "finance-get-account-name"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    name = form_parm.format("name", "")
    o += finance_get_account_name.format(h, name, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "finance-get-account-address"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    address = form_parm.format("address", "")
    o += finance_get_account_address.format(h, address, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("finance-create-account")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    name = form_parm.format("name", "")
    o += finance_create_account.format(h, pin, session, name, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("finance-debit-account")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    amount = form_parm.format("amount", "")
    name_from = form_parm.format("name_from", "")
    name_to = form_parm.format("name_to", "")
    o += finance_debit_account.format(h, pin, session, amount, name_from,
        name_to, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = form_header.format("finance-credit-account")
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    txid = form_parm.format("txid", "")
    amount = form_parm.format("amount", "")
    name_to = form_parm.format("name_to", "")
    proof = form_parm.format("name_proof", "")
    o += finance_credit_account.format(h, pin, session, txid, amount, name_to,
        proof, f)
    o += "</td></tr>"

    o = add_blank_row(o)

    o += "<tr><td>"
    h = form_header.format("finance-list-accounts")
    session = form_parm.format("session", sid)
    o += finance_list_accounts.format(h, session, f)
    o += "</td></tr>"

    o += '</table><br><hr size="5">'
    return(o)
#enddef

@bottle.route('/finance-get-account-name', method="post")
@bottle.route('/finance-get-account-name/<sid>', method="post")
def do_finance_get_account_name(sid=""):
    name = bottle.request.forms.get("name")
    if (no_parms(name)):
        m = red("finance/get/account needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_finance_get_account_by_name(name)
        genid = sdk.genesis_id
    else:
        output = curl(finance_get_account_name.format("", name, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/finance-get-account-address', method="post")
@bottle.route('/finance-get-account-address/<sid>', method="post")
def do_finance_get_account_address(sid=""):
    address = bottle.request.forms.get("address")
    if (no_parms(address)):
        m = red("finance/get/account needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_finance_get_account_by_address(address)
        genid = sdk.genesis_id
    else:
        output = curl(finance_get_account_address.format("", address, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/finance-create-account', method="post")
def do_finance_create_account():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    name = bottle.request.forms.get("name")
    if (no_parms(pin, session, name)):
        m = red("finance/create/account needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_finance_create_account(name)
        genid = sdk.genesis_id
    else:
        output = curl(finance_create_account.format("", pin, session, name, 
            ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/finance-debit-account', method="post")
def do_finance_debit_account():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    amount = bottle.request.forms.get("amount")
    name_from = bottle.request.forms.get("name_from")
    name_to = bottle.request.forms.get("name_to")
    if (no_parms(pin, session, amount, name_from, name_to)):
        return(show(red("finance/debit/account needs more input parameters"),
            session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_finance_debit_account_by_name(name_from, name_to,
            amount)
        genid = sdk.genesis_id
    else:
        output = curl(finance_debit_account.format("", pin, session, amount,
            name_from, name_to, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/finance-credit-account', method="post")
def do_finance_credit_account():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    txid = bottle.request.forms.get("txid")
    amount = bottle.request.forms.get("amount")
    name_to = bottle.request.forms.get("name_to")
    name_proof = bottle.request.forms.get("name_proof")
    if (no_parms(pin, session, txid, amount, name_to, name_proof)):
        m = red("finance/credit/account needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_finance_credit_account_by_name(name_to, amount,
            txid)
        genid = sdk.genesis_id
    else:
        output = curl(finance_credit_account.format("", pin, session, txid,
            amount, name_to, name_proof, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/finance-list-accounts', method="post")
def do_list_accounts():
    session = bottle.request.forms.get("session")
    if (no_parms(session)):
        m = red("finances/list/acccounts needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_finance_list_accounts()
        genid = sdk.genesis_id
    else:
        output = curl(finance_list_accounts.format("", session, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

#------------------------------------------------------------------------------

#
# build_ledger_html
#
ledger_get_blockhash = '{}ledger/get/blockhash?height={}{}'
ledger_get_block_height = '{}ledger/get/block?height={}&verbose={}{}'
ledger_get_block_hash = '{}ledger/get/block?hash={}&verbose={}{}'
ledger_get_transaction = '{}ledger/get/transaction?hash={}&verbose={}{}'
ledger_get_mininginfo = '{}ledger/get/mininginfo{}'
ledger_submit_transaction = '{}ledger/submit/transaction?data={}{}'
ledger_list_blocks_height = \
    '{}ledger/list/blocks?height={}&limit={}&verbose={}{}'
ledger_list_blocks_hash = '{}ledger/list/blocks?hash={}&limit={}&verbose={}{}'

def build_ledger_html(sid, genid, o):
    f = form_footer
    o += "<br><b>Ledger API</b><br><br><table>"

    o += "<tr><td>"
    h = "ledger-get-blockhash"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    height = form_parm.format("height", "")
    o += ledger_get_blockhash.format(h, height, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "ledger-get-block-height"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    height = form_parm.format("height", "")
    v = form_parm.format("verbose", "")
    o += ledger_get_block_height.format(h, height, v, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "ledger-get-block-hash"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    hsh = form_parm.format("hash", "")
    v = form_parm.format("verbose", "")
    o += ledger_get_block_hash.format(h, hsh, v, f)

    o += "<tr><td>"
    h = "ledger-get-transaction"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    hsh = form_parm.format("hash", "")
    v = form_parm.format("verbose", "")
    o += ledger_get_transaction.format(h, hsh, v, f)

    o += "<tr><td>"
    h = "ledger-submit-transaction"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    d = form_parm.format("data", "")
    o += ledger_submit_transaction.format(h, d, f)

    o = add_blank_row(o)

    o += "<tr><td>"
    h = "ledger-list-blocks-height"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    height = form_parm.format("height", "")
    l = form_parm.format("limit", "")
    v = form_parm.format("verbose", "")
    o += ledger_list_blocks_height.format(h, height, l, v, f)

    o += "<tr><td>"
    h = "ledger-list-blocks-hash"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    hsh = form_parm.format("hash", "")
    l = form_parm.format("limit", "")
    v = form_parm.format("verbose", "")
    o += ledger_list_blocks_hash.format(h, hsh, l, v, f)

    o += '</table><br><hr size="5">'
    return(o)
#enddef

@bottle.route('/ledger-get-blockhash', method="post")
@bottle.route('/ledger-get-blockhash/<sid>', method="post")
def do_ledger_get_blockhash(sid=""):
    height = bottle.request.forms.get("height")
    if (no_parms(height)):
        m = red("ledger/get/blockhash needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_ledger_get_blockhash(height)
        genid = sdk.genesis_id
    else:
        output = curl(ledger_get_blockhash.format("", height, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef
    
@bottle.route('/ledger-get-block-height', method="post")
@bottle.route('/ledger-get-block-height/<sid>', method="post")
def do_ledger_get_block_height(sid=""):
    height = bottle.request.forms.get("height")
    if (no_parms(height)):
        m = red("ledger/get/block needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_ledger_get_block_by_height(height)
        genid = sdk.genesis_id
    else:
        output = curl(ledger_get_block_height.format("", height, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef
    
@bottle.route('/ledger-get-block-hash', method="post")
@bottle.route('/ledger-get-block-hash/<sid>', method="post")
def do_ledger_get_block_hash(sid=""):
    hsh = bottle.request.forms.get("hash")
    if (no_parms(hsh)):
        m = red("ledger/get/block needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_ledger_get_block_by_hash(hsh)
        genid = sdk.genesis_id
    else:
        output = curl(ledger_get_block_hash.format("", hsh, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef
    
@bottle.route('/ledger-get-transaction', method="post")
@bottle.route('/ledger-get-transaction/<sid>', method="post")
def do_ledger_get_transaction(sid=""):
    hsh = bottle.request.forms.get("hash")
    verbose = bottle.request.forms.get("verbose")
    if (no_parms(hsh, verbose)):
        m = red("ledger/get/transaction needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_ledger_get_transaction(hsh, verbose)
        genid = sdk.genesis_id
    else:
        output = curl(ledger_get_transaction.format("", hsh, verbose, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef
    
@bottle.route('/ledger-submit-transaction', method="post")
@bottle.route('/ledger-submit-transaction/<sid>', method="post")
def do_ledger_submit_transaction(sid=""):
    data = bottle.request.forms.get("data")
    if (no_parms(data)):
        m = red("ledger/submit/transaction needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_ledger_submit_transaction(data)
        genid = sdk.genesis_id
    else:
        output = curl(ledger_submit_transaction.format("", data, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/ledger-list-blocks-height', method="post")
@bottle.route('/ledger-list-blocks-height/<sid>', method="post")
def do_ledger_list_blocks_height(sid=""):
    height = bottle.request.forms.get("height")
    l = bottle.request.forms.get("limit")
    verbose = bottle.request.forms.get("limit")
    if (no_parms(height, l, verbose)):
        m = red("ledger/list/blocks needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_ledger_list_blocks_by_height(height, l, verbose)
        genid = sdk.genesis_id
    else:
        output = curl(ledger_list_blocks_height.format("", height, l, verbose,
            ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/ledger-list-blocks-hash', method="post")
@bottle.route('/ledger-list-blocks-hash/<sid>', method="post")
def do_ledger_list_blocks_hash(sid=""):
    hsh = bottle.request.forms.get("hash")
    l = bottle.request.forms.get("limit")
    verbose = bottle.request.forms.get("limit")
    if (no_parms(hsh, l, verbose)):
        m = red("ledger/list/blocks needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_ledger_list_blocks_by_hsh(hsh, l, verbose)
        genid = sdk.genesis_id
    else:
        output = curl(ledger_list_blocks_hash("", hsh, l, verbose, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

#------------------------------------------------------------------------------

#
# build_objects_html
#
objects_create_schema = ('{}objects/create/schema?pin={}&session={}' +\
    '&name={}&format=json&json={}{}')
objects_get_schema_name = '{}objects/get/schema?name={}&format=json{}'
objects_get_schema_address = '{}objects/get/schema?address={}&format=json{}'

def build_objects_html(sid, genid, o):
    f = form_footer
    o += "<br><b>Objects API</b><br><br><table>"

    o += "<tr><td>"
    h = "objects-get-schema-name"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    name = form_parm.format("name", "")
    o += objects_get_schema_name.format(h, name, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "objects-get-schema-address"
    if (sid != ""): h += "/{}".format(sid)
    h = form_header.format(h)
    address = form_parm.format("address", "")
    o += objects_get_schema_address.format(h, address, f)
    o += "</td></tr>"

    o += "<tr><td>"
    h = "objects-create-schema"
    h = form_header.format(h)
    pin = form_parm.format("pin", "")
    session = form_parm.format("session", sid)
    name = form_parm.format("name", "")
    j = form_parm.format("json", "")
    o += objects_create_schema.format(h, pin, session, name, j, f)
    o += "</td></tr>"

    o += '</table><br><hr size="5">'
    return(o)
#enddef

@bottle.route('/objects-create-schema', method="post")
def do_objects_create_schema():
    pin = bottle.request.forms.get("pin")
    session = bottle.request.forms.get("session")
    name = bottle.request.forms.get("name")
    j = bottle.request.forms.get("json")
    if (no_parms(pin, session, name, j)):
        m = red("objects/create/schema needs more input parameters")
        return(show(m, session))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(session)
        if (sdk == None): return(output)
        output = sdk.nexus_objects_create_schema(name, j)
        genid = sdk.genesis_id
    else:
        output = curl(objects_create_schema.format("", pin, session, name, j,
            ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, session, genid))
#enddef

@bottle.route('/objects-get-schema-name', method="post")
@bottle.route('/objects-get-schema-name/<sid>', method="post")
def do_objects_get_schema_name(sid=""):
    name = bottle.request.forms.get("name")
    if (no_parms(name)):
        m = red("objects/get/schema needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_objects_get_schema_by_name(name)
        genid = sdk.genesis_id
    else:
        output = curl(objects_get_schema_name.format("", name, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

@bottle.route('/objects-get-schema-address', method="post")
@bottle.route('/objects-get-schema-address/<sid>', method="post")
def do_objects_get_schema_address(sid=""):
    address = bottle.request.forms.get("address")
    if (no_parms(address)):
        m = red("objects/get/schema needs more input parameters")
        return(show(m, sid))
    #endif

    action = bottle.request.forms.get("action")
    sdk_or_api = (action.find("SDK") != -1)
    
    if (sdk_or_api):
        sdk, output = sid_to_sdk(sid)
        if (sdk == None): return(output)
        output = sdk.nexus_objects_get_schema_by_address(address)
        genid = sdk.genesis_id
    else:
        output = curl(objects_get_schema_address.format("", address, ""))
        genid = ""
    #endif            
    output = json.dumps(output)

    return(show(output, sid, genid))
#enddef

#------------------------------------------------------------------------------

#
# build_body_page
#
# Fill in all API calls into body page.
#
def build_body_page(sid="", genid=""):
    output = ""
    output = build_url_html(output)
    output = build_system_html(sid, genid, output)
    output = build_users_html(sid, genid, output)
    output = build_ledger_html(sid, genid, output)
    output = build_tokens_html(sid, genid, output)
    output = build_assets_html(sid, genid, output)
    output = build_supply_html(sid, genid, output)
    output = build_finance_html(sid, genid, output)
    output = build_objects_html(sid, genid, output)
    return(output)
#enddef

@bottle.route('/')
def do_landing():
    output = build_body_page()
    return(landing_page.format(output))
#enddef

@bottle.route('/url/<api_or_sdk>', method="post")
def do_url(api_or_sdk):
    global nexus_api_node, nexus_sdk_node
    
    url = bottle.request.forms.get("url")
    if (api_or_sdk == "api"):
        nexus_api_node = url
    #endif
    if (api_or_sdk == "sdk"):
        nexus.sdk_change_url(url)
        nexus_sdk_node = url
    #endif

    output = build_body_page()
    return(landing_page.format(output))
#enddef

#------------------------------------------------------------------------------

#
# ---------- Main program entry point. ----------
#
date = commands.getoutput("date")
print "cookbook starting up at {}".format(date)

#
# Run web server.
#
bottle.run(host="0.0.0.0", port=cookbook_port, debug=True)
exit(0)

#------------------------------------------------------------------------------
