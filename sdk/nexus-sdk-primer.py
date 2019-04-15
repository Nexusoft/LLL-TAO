#
# nexus-sdk-primer.py
#
# A very simple sequential program for interacting with the Nexus Tritium
# APIs. If you want more details and want to see more interactions and
# return response detail, use LLL-TAO/sdk/nexus-sdk-test.py.
#
# If you want a bit more detail in this program, just uncomment "#print json"
# lines.
#
# Usage: python nexus-sdk-primer.py [accounts] [supply] [assets] [tokens]
#
#------------------------------------------------------------------------------

import nexus_sdk
import time
import sys

if (len(sys.argv) == 1):
    do_accounts = do_supply = do_assets = do_tokens = True
else:
    do_accounts = ("accounts" in sys.argv)
    do_supply = ("supply" in sys.argv)
    do_assets = ("assets" in sys.argv)
    do_tokens = ("tokens" in sys.argv)
#endif
do_accounts = (do_accounts or do_supply or do_assets or do_tokens)

#
# Column width for continuation output line.
#
width = 48

#
# sleep
#
# If you want to play around with delay times between API operations. Defaults
# to 1 second. You can litter sleep() calls whereever you like below.
#
def sleep():
    delay = 3
    if (delay == 0): return
    print "Sleeping for {} ...".format(delay)
    time.sleep(delay)
#enddef

#------------------------------------------------------------------------------

#
# api_print
#
# Print if API succeeded or failed on a continuation output line. When program
# returns, caller should continue. Otherwise this funciton exits program.
#
def api_print(json, succeed_msg=None):
    if (json.has_key("error") == False):
        msg = "succeeded"
        if (succeed_msg != None): msg += ", {}".format(succeed_msg)
        print msg
        return(False)
    #endif

    msg = json["error"]["message"]
    print "{}, {}".format(red("failed"), msg)
    if (msg == "Connection refused"):
        print "Abnormal exit"
        exit(1)
    #endif
    return(True)
#enddef

#
# short_genid
#
# Print genesis-id in the form "xxxx...xxxx".
#
def short_genid(genid):
    return(genid[0:4] + "..." + genid[-5:-1])
#enddef

#
# short_txid
#
# Print txid in the form "xxxxxxxx...yyyyyyyy".
#
def short_txid(genid):
    return(genid[0:8] + "..." + genid[-9:-1])
#enddef

#
# print_history
#
# Print ownership history for supply-chain, asset, or token.
#
def print_history(json):
    print "Ownership History:"
    for owner in json["result"]:
        o = short_genid(owner["owner"])
        owner.pop("owner")
        print "  owner {}: {}".format(o, owner)
    #endfor
#enddef

#
# login_or_create_username
#
# Check to see if username exists by trying to login. If it does not exist,
# create an account and login.
#
def login_or_create_username(sdk, username):
    json = sdk.nexus_accounts_login()

    msg = json["error"]["message"] if json.has_key("error") else None
    if (msg == "Account doesn't exists"):
        print "Create username '{}' ...".format(username).ljust(width),
        json = sdk.nexus_accounts_create()
        api_print(json)
    #endif

    print "Login user '{}' ...".format(username).ljust(width),
    if (msg == None):
        print "Already logged in"
        return
    #endif

    json = sdk.nexus_accounts_login()
    api_print(json, "genesis {}".format(short_genid(sdk.genesis_id)))
#enddef

#
# create_token
#
# Check to see if token or token account has been created. If not create it.
# Otherwise, show data about each.
#
def create_token(sdk, token_name, tok_or_account):
    if (tok_or_account):
        json = sdk.nexus_tokens_create_token(token_name, 1, 10000)
        token_string = "token already exists"
    else:
        json = sdk.nexus_tokens_create_account(token_name, 1)
        token_string = "token account already exists"
    #endif

    address = "?"
    if (json.has_key("result") and json["result"] != None):
        address = json["result"]["address"]
        api_print(json, "address {}".format(address))
        sleep()
    else:
        print token_string
    #endif

    #
    # Test API tokens/get.
    #
    msg = None
    if (tok_or_account):
        print "Get Token '{}' ...".format(token_name).ljust(width),
        json = sdk.nexus_tokens_get_token_by_name(token_name)
    else:
        print "Get Token Account '{}' ...".format(token_name).ljust(width),
        json = sdk.nexus_tokens_get_account_by_name(token_name)
    #endif

    if (json.has_key("result") and json["result"] != None):
        if (tok_or_account):
            ms = json["result"]["maxsupply"]
            cs = json["result"]["currentsupply"]
            msg = "max-supply {}, current-supply {}".format(ms, cs)
        else:
            balance = json["result"]["balance"]
            msg = "balance {}".format(balance)
        #endif
    #endif
    api_print(json, msg)
#enddef

#
# red
#
# Print red to make "failed" stand out.
#
def red(string):
    return("\033[91m" + string + "\033[0m")
#enddef

#
# parse_tx
#
# Parse a line from the accounts/transactions returned array.
#
def parse_tx(tx):
    try:
        op, tx = tx.split("(")
    except:
        return("")
    #endtry
    tx = tx[0:-1].split(", ")
    line = "{}: {}".format(op, tx)
    return(line)
#enddef

#
# accounts
#
# Run through supply API/SDK.
#
def accounts(primer1, primer2):

    print ""
    print "---------- Accounts SDK/API ----------"

    #
    # Use API accounts/create and accounts/login for a few users.
    #
    login_or_create_username(primer1, "primer1")
    login_or_create_username(primer2, "primer2")
    login_or_create_username(temp_primer, "temp-primer")

    #
    # Test API accounts/logout on username 'temp-primer'.
    #
    print "Logout user 'temp-primer' ...".ljust(width),
    json = temp_primer.nexus_accounts_logout()
    api_print(json)

    #
    # Call API accounts/transactions
    #
    json = primer1.nexus_accounts_transactions(0, 3, 2)
    print "Transaction history for 'primer1':"
    for entry in json["result"]:
        tx = entry["operation"]
        print "  {}".format(str(tx))
    #endfor

    json = primer2.nexus_accounts_transactions(0, 3, 2)
    print "Transaction history for 'primer2':"
    for entry in json["result"]:
        tx = entry["operation"]
        print "  {}".format(str(tx))
    #endfor
#enddef

#
# supply_chain
#
# Run through supply API/SDK.
#
def supply_chain(primer1, primer2):

    #
    # Put in break line between accounts and supply API.
    #
    print ""
    print "---------- Supply-Chain SDK/API ----------"

    #
    # Call API supply/createitem
    #
    address = None
    data = "0xdeadbeef"
    print "Supply-Chain create item, data '{}' ...".format(data).ljust(width),
    json = primer1.nexus_supply_createitem(data)
    if (json.has_key("result") and json["result"] != None):
        address = json["result"]["address"]
    #endif
    api_print(json, "address {}".format(address))

    sleep()

    #
    # Call API supply/getitem
    #
    msg = None
    print "Supply-Chain get item ...".ljust(width),

    while (True):
        json = primer1.nexus_supply_getitem(address)
        if (json.has_key("error") == False): break
        print ".",
        time.sleep(1)
    #endwhile
    if (json.has_key("result") and json["result"] != None):
        owner = json["result"]["owner"]
        state = json["result"]["state"]
        msg = "stored data '{}', owner {}".format(state, short_genid(owner))
    #endif
    api_print(json, msg)

    #
    # Call API supply/transfer
    #
    print "Transfer genesis {} to genesis {}".format( \
        short_genid(primer1.genesis_id),  short_genid(primer2.genesis_id))
    print "Transfer ownership ...".ljust(width),
    json = primer1.nexus_supply_transfer(address, primer2.genesis_id)
    api_print(json)

    sleep()

    #
    # Call API supply/history
    #
    print "Get ownership history of item ...".ljust(width),
    json = primer1.nexus_supply_history(address)
    api_print(json)
    if (json.has_key("result") and json["result"] != None):
        print_history(json)
    #endif
#endef

#
# assets
#
# Run through assets API/SDK.
#
def assets(primer1, primer2):
    print ""
    print "---------- Assets SDK/API ----------"

    #
    # Call API assets/create if the asset-name does not exist.
    #
    asset_name = "primer-asset"
    asset_data = "asset-data"
    address = None
    owner = None
    new_owner = None
    print "Create Asset named '{}' ...".format(asset_name).ljust(width),
    json = primer1.nexus_assets_get_by_name(asset_name)
    if (json.has_key("error") == False):
        owner = json["result"]["owner"]
        state = json["result"]["metadata"]
        msg = "stored metadata '{}', owner {}".format(state,
            short_genid(owner))
        print "asset already exists, {}".format(msg)
    else:
        json = primer1.nexus_assets_create(asset_name, asset_data)
        if (json.has_key("result") and json["result"] != None):
            address = json["result"]["address"]
        #endif
        api_print(json, "address {}".format(address))
        sleep()

        #
        # Call API assets/get
        #
        msg = None
        print "Get Asset named '{}' ...".format(asset_name).ljust(width),
        count = 10
        while (True):
            json = primer1.nexus_assets_get_by_name(asset_name)
            if (json.has_key("error") == False or count == 0): break
            print ".",
            time.sleep(1)
            count -= 1
        #endwhile
        if (json.has_key("result") and json["result"] != None):
            owner = json["result"]["owner"]
            state = json["result"]["metadata"]
            msg = "stored metadata '{}', owner {}".format(state,
                short_genid(owner))
        #endif
        api_print(json, msg)
    #endif

    #
    # Call API asserts/transfer back and forth from primer1 to primer2.
    #
    to = None
    if (primer1.genesis_id == owner):
        sdk = primer1
        new_owner = primer2
        fr = "primer1"
        to = "primer2"
        print "Transfer Asset '{}' from '{}' (genid {}) to '{}' (genid {})". \
            format(asset_name, fr, short_genid(owner), to,
            short_genid(primer2.genesis_id))
    #endif
    if (primer2.genesis_id == owner):
        sdk = primer2
        new_owner = primer1
        fr = "primer2"
        to = "primer1"
        print "Transfer Asset '{}' from '{}' (genid {}) to '{}' (genid {})". \
            format(asset_name, fr, short_genid(owner), to,
            short_genid(primer1.genesis_id))
    #endif

    if (to != None):
        msg = None
        print "Transfer ownership ...".ljust(width),
        json = sdk.nexus_assets_transfer_by_name(asset_name, to)
        if (json.has_key("result") and json["result"] != None):
            txid = json["result"]["txid"]
            address = json["result"]["address"]
            msg = "txid {}, address {}".format(short_txid(txid), address)
        #endif
        api_print(json, msg)
    #endif

    sleep()

    #
    # Tokenize asset 'primer-asset'.
    #
    if (new_owner != None):
        msg = None
        print ""
        print "Tokenize 'primer-asset' ...",
        json = new_owner.nexus_assets_tokenize_by_name("primer-asset", "primer-token")
        if (json.has_key("result") and json["result"] != None):
            txid = json["result"]["txid"]
            address= json["result"]["address"]
            msg = "txid {}, address {}".format(txid, address)
        #endif
        api_print(json, msg)
    #endif

    #
    # Call API assets/history
    #
    print "Get history for Asset '{}' ...".format(asset_name).ljust(width),
    json = primer1.nexus_assets_history_by_name(asset_name)
    api_print(json)
    if (json.has_key("result") and json["result"] != None):
        print_history(json)
    #endif
#enddef

#
# tokens
#
# Run through assets API/SDK.
#
def tokens(primer1, primer2):
    print ""
    print "---------- Tokens SDK/API ----------"

    #
    # Call API tokens/create for a token. Token-ID will be 1.
    #
    token_name = "primer-token"
    print "Create Token '{}', supply {} ...".format(token_name, 10000). \
        ljust(width),
    create_token(primer1, token_name, True)
    print "Create Token Account '{}' ...".format("primer1").ljust(width),
    create_token(primer1, "primer1", False)
    print "Create Token Account '{}' ...".format("primer2").ljust(width),
    create_token(primer2, "primer2", False)

    #
    # Call API tokens/debit
    #
    msg = None
    txid1 = None
    print "Debit '{}' by 100 for 'primer1' ...".format(token_name). \
        ljust(width),
    json = primer1.nexus_tokens_debit_by_name(token_name, "primer1", 100)
    if (json.has_key("result") and json["result"] != None):
        txid1 = json["result"]["txid"]
        msg = "txid {}".format(short_txid(txid1))
    #endif
    api_print(json, msg)
    sleep()

    msg = None
    txid2 = None
    print "Debit '{}' by 200 for 'primer2' ...".format(token_name). \
        ljust(width),
    json = primer1.nexus_tokens_debit_by_name(token_name, "primer2", 200)
    if (json.has_key("result") and json["result"] != None):
        txid2 = json["result"]["txid"]
        msg = "txid {}".format(short_txid(txid2))
    #endif
    api_print(json, msg)
    sleep()

    #
    # Call API tokens/credit to credit eash token account with 100 and 200
    # tokens respectively.
    #
    msg = None
    print "Credit 100 to user 'primer1' ...".ljust(width),
    json = primer1.nexus_tokens_credit_by_name("primer1", 100, txid1, None)
    if (json.has_key("result") and json["result"] != None):
        txid = json["result"]["txid"]
        msg = "txid {}".format(short_txid(txid))
    #endif
    api_print(json, msg)
    sleep()

    msg = None
    print "Credit 200 to user 'primer2' ...".ljust(width),
    json = primer2.nexus_tokens_credit_by_name("primer2", 200, txid2, None)
    if (json.has_key("result") and json["result"] != None):
        txid = json["result"]["txid"]
        msg = "txid {}".format(short_txid(txid))
    #endif
    api_print(json, msg)
    sleep()

    #
    # Show balances of token accounts.
    #
    print ""
    print "Token Acccount Balances:"
    json = primer1.nexus_tokens_get_account_by_name("primer1")
    if (json.has_key("error") == False):
        balance = json["result"]["balance"]
        print "  'primer1' token account balance {}".format(balance)
    #endif
    json = primer1.nexus_tokens_get_account_by_name("primer2")
    if (json.has_key("error") == False):
        balance = json["result"]["balance"]
        print "  'primer2' token account balance {}".format(balance)
    #endif
#enddef

#------------------------------------------------------------------------------

#
# Initialize with SDK.
#
print "Initialize Nexus SDK ...".ljust(width),
primer1 = nexus_sdk.sdk_init("primer1", "primer-password", "1234")
primer2 = nexus_sdk.sdk_init("primer2", "primer-password", "1234")
temp_primer = nexus_sdk.sdk_init("temp-primer", "primer-password", "1234")
print "succeeded"

asset = token = None

if (do_accounts): accounts(primer1, primer2)
if (do_supply): supply_chain(primer1, primer2)
if (do_assets): assets(primer1, primer2)
if (do_tokens): tokens(primer1, primer2)


print ""
print "All Done!"
exit(0)

#------------------------------------------------------------------------------
