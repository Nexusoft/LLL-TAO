#!/usr/bin/env python3
#
# nexus-sdk-primer.py
#
# A very simple sequential program for interacting with the Nexus Tritium++
# APIs. If you want more details and want to see more interactions and
# return response detail, use LLL-TAO/sdk/nexus-sdk-test.py.
#
# If you want a bit more detail in this program, just uncomment "#print(json)"
# lines.
#
# Usage: python3 nexus-sdk-primer.py [sessions] [supply] [assets] [finance]
#
#------------------------------------------------------------------------------

import nexus_sdk
import time
import sys

if len(sys.argv) == 1:
    do_sessions = do_supply = do_assets = do_finance = True
else:
    do_sessions = "sessions" in sys.argv
    do_supply = "supply" in sys.argv
    do_assets = "assets" in sys.argv
    do_finance = "finance" in sys.argv
#endif
do_sessions = do_sessions or do_supply or do_assets or do_finance

#
# Column width for continuation output line.
#
width = 48

#
# sleep
#
# If you want to play around with delay times between API operations. Defaults
# to 3 seconds. You can litter sleep() calls wherever you like below.
#
def sleep():
    delay = 3
    if delay == 0:
        return
    print("Sleeping for {} seconds...".format(delay))
    time.sleep(delay)
#enddef

#------------------------------------------------------------------------------

#
# api_print
#
# Print if API succeeded or failed on a continuation output line. When program
# returns, caller should continue. Otherwise this function exits program.
#
def api_print(json, succeed_msg=None):
    if "error" not in json:
        msg = "succeeded"
        if succeed_msg is not None:
            msg += ", {}".format(succeed_msg)
        print(msg)
        return False
    #endif

    msg = json["error"]["message"]
    print("{}, {}".format(red("failed"), msg))
    if msg == "Connection refused":
        print("Abnormal exit")
        exit(1)
    #endif
    return True
#enddef

#
# short_genid
#
# Print genesis-id in the form "xxxx...xxxx".
#
def short_genid(genid):
    if not genid or len(genid) < 10:
        return genid or "N/A"
    return genid[0:4] + "..." + genid[-4:]
#enddef

#
# short_txid
#
# Print txid in the form "xxxxxxxx...yyyyyyyy".
#
def short_txid(txid):
    if not txid or len(txid) < 20:
        return txid or "N/A"
    return txid[0:8] + "..." + txid[-8:]
#enddef

#
# print_history
#
# Print ownership history for supply-chain, asset, or token.
#
def print_history(json):
    print("Ownership History:")
    for owner in json["result"]:
        o = short_genid(owner.get("owner", ""))
        owner_copy = dict(owner)
        owner_copy.pop("owner", None)
        print("  owner {}: {}".format(o, owner_copy))
    #endfor
#enddef

#
# login_or_create_username
#
# Check to see if username exists by trying to login. If it does not exist,
# create a profile and login.
#
def login_or_create_username(sdk, username):
    json = sdk.sessions_create()

    msg = json["error"]["message"] if "error" in json else None
    if msg and "doesn't exist" in msg.lower():
        print("Create profile '{}' ...".format(username).ljust(width), end="")
        json = sdk.profiles_create()
        api_print(json)
        sleep()
    #endif

    print("Login user '{}' ...".format(username).ljust(width), end="")
    if msg is None:
        print("Already logged in")
        return
    #endif

    json = sdk.sessions_create()
    if sdk.genesis_id is None:
        print("sessions/create failed, nexus daemon may not be running")
        exit(1)
    #endif
    api_print(json, "genesis {}".format(short_genid(sdk.genesis_id)))
#enddef

#
# create_token_or_account
#
# Check to see if token or token account has been created. If not create it.
# Otherwise, show data about each.
#
def create_token_or_account(sdk, name, is_token):
    if is_token:
        json = sdk.finance_create("token", name, supply=10000, decimals=2)
        exists_string = "token already exists"
    else:
        json = sdk.finance_create("account", name, token="0")  # NXS token
        exists_string = "account already exists"
    #endif

    address = "?"
    if "result" in json and json["result"] is not None:
        address = json["result"].get("address", "?")
        api_print(json, "address {}".format(address))
        sleep()
    else:
        print(exists_string)
    #endif

    #
    # Get the token or account.
    #
    msg = None
    if is_token:
        print("Get Token '{}' ...".format(name).ljust(width), end="")
        json = sdk.finance_get("token", name=name)
    else:
        print("Get Account '{}' ...".format(name).ljust(width), end="")
        json = sdk.finance_get("account", name=name)
    #endif

    if "result" in json and json["result"] is not None:
        if is_token:
            ms = json["result"].get("maxsupply", 0)
            cs = json["result"].get("currentsupply", 0)
            msg = "max-supply {}, current-supply {}".format(ms, cs)
        else:
            balance = json["result"].get("balance", 0)
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
    return "\033[91m" + string + "\033[0m"
#enddef

#
# parse_tx
#
# Parse a line from the profiles/transactions returned array.
#
def parse_tx(tx):
    try:
        op, tx = tx.split("(")
    except:
        return ""
    #endtry
    tx = tx[0:-1].split(", ")
    line = "{}: {}".format(op, tx)
    return line
#enddef

#
# sessions
#
# Run through sessions/profiles API/SDK.
#
def sessions(primer1, primer2):
    print("")
    print("---------- Sessions/Profiles SDK/API ----------")

    #
    # Use API profiles/create and sessions/create for users.
    #
    login_or_create_username(primer1, "primer1")
    login_or_create_username(primer2, "primer2")
    login_or_create_username(temp_primer, "temp-primer")

    #
    # Test API sessions/terminate on username 'temp-primer'.
    #
    print("Logout user 'temp-primer' ...".ljust(width), end="")
    json = temp_primer.sessions_terminate()
    api_print(json)

    #
    # Call API profiles/transactions
    #
    json = primer1.profiles_transactions(page=0, limit=3)
    print("Transaction history for 'primer1':")
    if "result" in json and json["result"]:
        for entry in json["result"]:
            tx = entry.get("operation", entry)
            print("  {}".format(str(tx)))
        #endfor
    #endif

    json = primer2.profiles_transactions(page=0, limit=3)
    print("Transaction history for 'primer2':")
    if "result" in json and json["result"]:
        for entry in json["result"]:
            tx = entry.get("operation", entry)
            print("  {}".format(str(tx)))
        #endfor
    #endif
#enddef

#
# supply_chain
#
# Run through supply API/SDK.
#
def supply_chain(primer1, primer2):
    print("")
    print("---------- Supply-Chain SDK/API ----------")

    #
    # Call API supply/create/item.
    #
    address = None
    data = "0xdeadbeef"
    item_name = "primer-item-{}".format(int(time.time()) % 10000)
    print("Supply-Chain create item '{}', data '{}' ...".format(
        item_name, data).ljust(width), end="")
    json = primer1.supply_create(item_name, data)
    if "result" in json and json["result"] is not None:
        address = json["result"].get("address", "?")
    #endif
    api_print(json, "address {}".format(address))

    sleep()

    #
    # Call API supply/get/item.
    #
    msg = None
    print("Supply-Chain get item ...".ljust(width), end="")

    retries = 10
    while retries > 0:
        json = primer1.supply_get(name=item_name)
        if "error" not in json:
            break
        print(".", end="", flush=True)
        time.sleep(1)
        retries -= 1
    #endwhile
    if "result" in json and json["result"] is not None:
        owner = json["result"].get("owner", "?")
        state = json["result"].get("state", "?")
        msg = "stored data '{}', owner {}".format(state, short_genid(owner))
    #endif
    api_print(json, msg)

    #
    # Call API supply/transfer/item.
    #
    print("Transfer genesis {} to genesis {}".format(
        short_genid(primer1.genesis_id), short_genid(primer2.genesis_id)))
    print("Transfer ownership ...".ljust(width), end="")
    json = primer1.supply_transfer(primer2.genesis_id, name=item_name)
    api_print(json)

    sleep()

    #
    # Call API supply/history/item.
    #
    print("Get ownership history of item ...".ljust(width), end="")
    json = primer1.supply_history(name=item_name)
    api_print(json)
    if "result" in json and json["result"] is not None:
        print_history(json)
    #endif
#enddef

#
# assets
#
# Run through assets API/SDK.
#
def assets(primer1, primer2):
    print("")
    print("---------- Assets SDK/API ----------")

    #
    # Call API assets/create/asset if the asset-name does not exist.
    #
    asset_name = "primer-asset"
    asset_data = "asset-data"
    address = None
    owner = None
    print("Create Asset named '{}' ...".format(asset_name).ljust(width), end="")
    json = primer1.assets_get(name=asset_name)
    if "error" not in json:
        owner = json["result"].get("owner")
        state = json["result"].get("metadata", json["result"].get("state", ""))
        msg = "stored data '{}', owner {}".format(state, short_genid(owner))
        print("asset already exists, {}".format(msg))
    else:
        json = primer1.assets_create(asset_name, asset_data)
        if "result" in json and json["result"] is not None:
            address = json["result"].get("address", "?")
        #endif
        api_print(json, "address {}".format(address))
        sleep()

        #
        # Call API assets/get/asset.
        #
        msg = None
        print("Get Asset named '{}' ...".format(asset_name).ljust(width), end="")
        retries = 10
        while retries > 0:
            json = primer1.assets_get(name=asset_name)
            if "error" not in json:
                break
            print(".", end="", flush=True)
            time.sleep(1)
            retries -= 1
        #endwhile
        if "result" in json and json["result"] is not None:
            owner = json["result"].get("owner")
            state = json["result"].get("metadata", json["result"].get("state", ""))
            msg = "stored data '{}', owner {}".format(state, short_genid(owner))
        #endif
        api_print(json, msg)
    #endif

    #
    # Call API assets/transfer back and forth from primer1 to primer2.
    #
    to_user = None
    if primer1.genesis_id == owner:
        sdk = primer1
        fr = "primer1"
        to_user = "primer2"
        print("Transfer Asset '{}' from '{}' (genid {}) to '{}' (genid {})".format(
            asset_name, fr, short_genid(owner), to_user,
            short_genid(primer2.genesis_id)))
    #endif
    if primer2.genesis_id == owner:
        sdk = primer2
        fr = "primer2"
        to_user = "primer1"
        print("Transfer Asset '{}' from '{}' (genid {}) to '{}' (genid {})".format(
            asset_name, fr, short_genid(owner), to_user,
            short_genid(primer1.genesis_id)))
    #endif

    if to_user is not None:
        msg = None
        print("Transfer ownership ...".ljust(width), end="")
        json = sdk.assets_transfer(to_user, name=asset_name)
        if "result" in json and json["result"] is not None:
            txid = json["result"].get("txid", "?")
            address = json["result"].get("address", "?")
            msg = "txid {}, address {}".format(short_txid(txid), address)
        #endif
        api_print(json, msg)
    #endif

    sleep()

    #
    # Call API assets/history/asset
    #
    print("Get history for Asset '{}' ...".format(asset_name).ljust(width), end="")
    json = primer1.assets_history(name=asset_name)
    api_print(json)
    if "result" in json and json["result"] is not None:
        print_history(json)
    #endif
    print("")

    #
    # Tokenize an asset. Create new asset and token now.
    #
    asset_name = "tokenized-asset"
    print("Get tokenized asset '{}' ...".format(asset_name).ljust(width), end="")
    json = primer1.assets_get(name=asset_name)
    if "error" in json:
        print("")
        print("Create tokenized asset '{}' ...".format(asset_name).ljust(width), end="")
        json = primer1.assets_create(asset_name, asset_name)
        if "result" in json and json["result"] is not None:
            address = json["result"].get("address", "?")
        #endif
        api_print(json, "address {}".format(address))
    else:
        api_print(json)
    #endif
    sleep()

    tn = "shares-token"
    print("Get token '{}' ...".format(tn).ljust(width), end="")
    json = primer1.finance_get("token", name=tn)
    if "error" in json:
        print("")
        print("Create token '{}', supply {} ...".format(tn, 100).ljust(width), end="")
        json = primer1.finance_create("token", tn, supply=100, decimals=0)
        if "result" in json and json["result"] is not None:
            address = json["result"].get("address", "?")
        #endif
        api_print(json, "address {}".format(address))
    else:
        api_print(json)
    #endif
    sleep()

    print("")
    print("Tokenize '{}' ...".format(asset_name).ljust(width), end="")
    json = primer1.assets_tokenize(tn, name=asset_name)
    msg = None
    if "result" in json and json["result"] is not None:
        txid = json["result"].get("txid", "?")
        address = json["result"].get("address", "?")
        msg = "txid {}, address {}".format(txid, address)
    #endif
    api_print(json, msg)
#enddef

#
# finance
#
# Run through finance API/SDK (replaces tokens API).
#
def finance(primer1, primer2):
    print("")
    print("---------- Finance SDK/API ----------")

    #
    # Call API finance/create/token.
    #
    token_name = "primer-token"
    print("Create Token '{}', supply {} ...".format(token_name, 10000).ljust(width), end="")
    create_token_or_account(primer1, token_name, True)
    
    print("Create Account 'primer1-account' ...".ljust(width), end="")
    create_token_or_account(primer1, "primer1-account", False)
    
    print("Create Account 'primer2-account' ...".ljust(width), end="")
    create_token_or_account(primer2, "primer2-account", False)

    #
    # Call API finance/debit.
    #
    msg = None
    txid1 = None
    print("Debit '{}' by 100 for 'primer1-account' ...".format(token_name).ljust(width), end="")
    json = primer1.finance_debit(100, name_from=token_name, name_to="primer1-account")
    if "result" in json and json["result"] is not None:
        txid1 = json["result"].get("txid")
        msg = "txid {}".format(short_txid(txid1))
    #endif
    api_print(json, msg)
    sleep()

    msg = None
    txid2 = None
    print("Debit '{}' by 200 for 'primer2-account' ...".format(token_name).ljust(width), end="")
    json = primer1.finance_debit(200, name_from=token_name, name_to="primer2-account")
    if "result" in json and json["result"] is not None:
        txid2 = json["result"].get("txid")
        msg = "txid {}".format(short_txid(txid2))
    #endif
    api_print(json, msg)
    sleep()

    #
    # Call API finance/credit to credit each account.
    #
    if txid1:
        msg = None
        print("Credit 100 to 'primer1-account' ...".ljust(width), end="")
        json = primer1.finance_credit(txid1, name="primer1-account")
        if "result" in json and json["result"] is not None:
            txid = json["result"].get("txid", "?")
            msg = "txid {}".format(short_txid(txid))
        #endif
        api_print(json, msg)
        sleep()
    #endif

    if txid2:
        msg = None
        print("Credit 200 to 'primer2-account' ...".ljust(width), end="")
        json = primer2.finance_credit(txid2, name="primer2-account")
        if "result" in json and json["result"] is not None:
            txid = json["result"].get("txid", "?")
            msg = "txid {}".format(short_txid(txid))
        #endif
        api_print(json, msg)
        sleep()
    #endif

    #
    # Show balances of accounts.
    #
    print("")
    print("Account Balances:")
    json = primer1.finance_get("account", name="primer1-account")
    if "error" not in json and json.get("result"):
        balance = json["result"].get("balance", 0)
        print("  'primer1-account' balance {}".format(balance))
    #endif
    json = primer2.finance_get("account", name="primer2-account")
    if "error" not in json and json.get("result"):
        balance = json["result"].get("balance", 0)
        print("  'primer2-account' balance {}".format(balance))
    #endif

    #
    # Show staking info.
    #
    print("")
    print("Staking Info for 'primer1':")
    json = primer1.finance_get_stakeinfo()
    if "error" not in json and json.get("result"):
        result = json["result"]
        print("  stake: {}, trust: {}, rate: {}%".format(
            result.get("stake", 0),
            result.get("trust", 0),
            result.get("stakerate", 0)))
    else:
        print("  (staking info not available)")
    #endif
#enddef

#------------------------------------------------------------------------------

#
# Initialize with SDK.
#
print("Initialize Nexus SDK ...".ljust(width), end="")
primer1 = nexus_sdk.NexusSDK("primer1", "primer-password", "1234")
primer2 = nexus_sdk.NexusSDK("primer2", "primer-password", "1234")
temp_primer = nexus_sdk.NexusSDK("temp-primer", "primer-password", "1234")
print("succeeded")

if do_sessions:
    sessions(primer1, primer2)
if do_supply:
    supply_chain(primer1, primer2)
if do_assets:
    assets(primer1, primer2)
if do_finance:
    finance(primer1, primer2)

print("")
print("All Done!")
exit(0)

#------------------------------------------------------------------------------
