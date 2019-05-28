#
# nexus_sdk.py
#
# This is the Nexus python SDK to interface with the Nexus Tritium APIs. If
# your python programs call nexus_sdk.sdk_init() with login credentials, you
# can get access to all Nexus API calls. This SDK has been optmized to abstract
# the out specific API parameters to make the API easier to use.
#
# This file produces sdk/nexus_sdk.txt using the pydoc tool. To run pydoc
# tool, type:
#
#    pydoc nexus_sdk > ./nexus_sdk.txt
#
# -----------------------------------------------------------------------------

"""

The Nexus python SDK supports the following Nexus API calls.

Low-level APIs:

System API:               Objects API:           Ledger API:
-----------               ------------           -----------
system/get/info           objects/create/schema  ledger/get/blockhash
system/list/peers         objects/get/schema     ledger/get/block
system/list/lisp-eids                            ledger/list/blocks
                                                 ledger/get/transaction
                                                 ledger/submit/transaction
                                                 ledger/get/mininginfo

Use-case APIs:

Users API:                Supply API:               Assets API:       
----------                -----------               -----------       
users/create/user         supply/create/item        assets/create/asset
users/login/user          supply/get/item           assets/get/asset       
users/logout/user         supply/udpate/item        assets/update/asset  
users/unlock/user         supply/transfer/item      assets/transfer/asset
users/lock/user           supply/claim/item         assets/claim/asset
users/list/transactions   supply/list/item/history  assets/list/asset/history
users/list/notifications                            assets/tokenize/asset
users/list/assets
users/list/accounts
users/list/tokens

Tokens API:               Finance API:
-----------               ------------
tokens/create/token       finance/create/account
tokens/create/account     finance/debit/account
tokens/get/token          finance/credit/account
tokens/get/account        finance/get/account
tokens/debit/token        finance/list/accounts
tokens/debit/account
tokens/credit/account

"""

#------------------------------------------------------------------------------

import urllib
import urllib2
import json

sdk_default_url = "http://localhost:8080"
sdk_url = sdk_default_url

system_url = "{}/system/{}"
objects_url = "{}/objects/{}"
ledger_url = "{}/ledger/{}"
users_url = "{}/users/{}"
supply_url = "{}/supply/{}"
assets_url = "{}/assets/{}"
tokens_url = "{}/tokens/{}"
finance_url = "{}/finance/{}"

#------------------------------------------------------------------------------

#
# sdk_change_url
#
# Change URL that the SDK uses. Make sure that the URL is formatted with http,
# double slashes and colon with number after colon.
#
def sdk_change_url(url=sdk_default_url):
    global sdk_url

    #
    # Check if input is a string. Return False if not.
    #
    if (type(url) != str): return(False)

    if (url[0:7] != "http://"): return(False)
    if (url.count(":") != 2): return(False)
    port = url.split(":")[2]
    if (port.isdigit() == False): return(False)

    sdk_url = url
    return(True)
#enddef

#
# Call sdk_change_url() to initialize URLs with default http string.
#
sdk_change_url()

class sdk_init():
    def __init__(self, user, pw, pin):
        """ 
        Initialize the API with user account credentials. This function
        must be calledd to create a login instance before using any other
        calls.
        """
        self.username = user
        self.password = pw
        self.pin = pin
        self.session_id = None
        self.genesis_id = None
    #enddef

    def nexus_system_get_info(self):
        """
        Get system information about Nexus node. Login not required.
        """
        url = system_url.format(sdk_url, "get/info")
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_system_list_peers(self):
        """
        List established Nexus peer connections. Login not required.
        """
        url = system_url.format(sdk_url, "list/peers")
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_system_list_lisp_eids(self):
        """
        List configured LISP EIDs for this node. Login not required.
        """
        url = system_url.format(sdk_url, "list/lisp-eids")
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_objects_create_schema(self, name, data):
        """
        Create object register schema. See API documentation for format
        of 'data' parameter.
        """
        parms = "?session={}&pin={}&name={}&format=json&json={}".format( \
            self.session_id, self.pin, name, data)
        url = objects_url.format(sdk_url, "create/schema") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_objects_get_schema_by_name(self, name):
        """
        Get object register scheme definition by supplying 'name' it was
        created by.
        """
        parms = "?name={}&format=json".format(name)
        url = objects_url.format(sdk_url, "get/schema") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_objects_get_schema_by_address(self, address):
        """
        Get object register scheme definition by supplying 'address' returned
        from nexus_objects_create_schema_by_name().
        """
        parms = "?address={}&format=json".format(address)
        url = objects_url.format(sdk_url, "get/schema") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_ledger_get_blockhash(self, height):
        """
        Retrieve the block hash for a block with supplied parameter 'height'.
        """
        parms = "?height={}&format=json".format(height)
        url = ledger_url.format(sdk_url, "get/blockhash") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_ledger_get_block_by_hash(self, block_hash, verbosity):
        """
        Retrieve the block with supplied hash 'block_hash'. See API 
        documentation for 'verbosity' values.
        """
        parms = "?hash={}&verbose={}".format(block_hash, verbosity)
        url = ledger_url.format(sdk_url, "get/block") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_ledger_get_block_by_height(self, block_height, verbosity):
        """
        Retrieve the block with supplied hash 'block_height'. See API 
        documentation for 'verbosity' values.
        """
        parms = "?height={}&verbose={}".format(block_height, verbosity)
        url = ledger_url.format(sdk_url, "get/block") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_ledger_list_blocks_by_hash(self, block_hash, limit, verbosity):
        """
        Retrieve list of blocks starting with hash 'block_hash'. The number
        of blocks is specified in 'limit'. See API documentation for 
        'verbosity' values.
        """
        parms = "?hash={}&limit={}&verbose={}".format(block_hash, limit,
            verbosity)
        url = ledger_url.format(sdk_url, "list/blocks") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_ledger_list_blocks_by_height(self, block_height, limit,
        verbosity):
        """
        Retrieve list of blocks starting with block height 'block_height'. 
        The number of blocks is specified in 'limit'. See API documentation 
        for 'verbosity' values.
        """
        parms = "?height={}&limit={}&verbose={}".format(block_height, limit,
            verbosity)
        url = ledger_url.format(sdk_url, "list/blocks") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_ledger_get_transaction(self, tx_hash, verbosity):
        """
        Retrieve transaction with transactioin hash 'tx_hash' with specific
        verbose. See API documentation for 'verbosity' values.
        """
        parms = "?hash={}&format=json".format(tx_hash, verbosity)
        url = ledger_url.format(sdk_url, "get/transaction") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_ledger_submit_transaction(self, tx_data):
        """
        Submit a transaction to the Nexus blockchain. The data stored for
        the transaction is specified in 'tx_data'.
        """
        parms = "?data{}".format(tx_data)
        url = ledger_url.format(sdk_url, "submit/transaction") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_ledger_get_mininginfo(self):
        """
        Retrieve mining information for the Nexus blockchain.
        """
        url = ledger_url.format(sdk_url, "get/mininginfo")
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_users_create_user(self):
        """
        Create a user account on the Nexus blockchain. Username, password,
        and pin parameters are supplied on the sdk_init() call.
        """
        pw = self.password.replace("&", "%26")
        pw = urllib.quote_plus(pw)
        parms = "?username={}&password={}&pin={}".format(self.username, pw,
            self.pin)
        url = users_url.format(sdk_url, "create/user") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_users_login_user(self):
        """
        Login a user on the Nexus blockchain. Username, password, and pin 
        parameters are supplied on the sdk_init() call.
        """
        if (self.session_id != None): return(self.__error("Already logged in"))

        pw = self.password.replace("&", "%26")
        pw = urllib.quote_plus(pw)
        parms = "?username={}&password={}&pin={}".format(self.username, pw,
            self.pin)
        url = users_url.format(sdk_url, "login/user") + parms
        json_data = self.__get(url)
        if (json_data.has_key("error")): return(json_data)

        if (json_data.has_key("result")):
            if json_data["result"].has_key("genesis"):
                self.genesis_id = json_data["result"]["genesis"]
            #endif
            if json_data["result"].has_key("session"):
                self.session_id = json_data["result"]["session"]
            #endif
        #endif
        return(json_data)
    #enddef

    def nexus_users_logout_user(self):
        """
        Logout a user from the Nexus blockchain.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?session={}".format(self.session_id)
        url = users_url.format(sdk_url, "logout/user") + parms
        json_data = self.__get(url)
        if (json_data.has_key("error")): return(json_data)

        if (json_data != {}): self.session_id = None
        return(json_data)
    #enddef

    def nexus_users_lock_user(self, minting=0, transactions=0):
        """
        Lock a user account on the Nexus blockchain. This disallows any API
        activity to the blockchain for this user account signature chain.
        Set 'minting' to 1 if the user account is staking or mining. Set 
        'transactions' to 0 if the user account should not create or claim 
        transactions.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?session={}&minting={}&transactions={}".format( \
            self.session_id, minting, transactions)
        url = users_url.format(sdk_url, "lock/users") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef
       
    def nexus_users_unlock_user(self, minting=0, transactions=0):
        """
        Unlock a user account on the Nexus blockchain that was previously
        locked with nexus_users_lock(). This now allows API activity 
        to the blockchain for this user account signature chain.
        Set 'minting' to 1 if the user account is staking or mining. Set 
        'transactions' to 0 if the user account should not create or claim 
        transactions.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&minting={}&transactions={}".format( \
            self.pin, self.session_id, minting, transactions)
        url = users_url.format(sdk_url, "unlock/user") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef
       
    def nexus_users_list_transactions_by_genesis(self, page, limit,
        verbosity="default"):
        """
        List user account transactions by genesis-id. The entry offset (first 
        is 0) is specified in 'page' and the number of transactions returned 
        is specified in 'limit'. Valid values for 'verbosity' are "default", 
        "summary", "detail".
        """
        if (self.genesis_id == None): return(self.__error("Not logged in"))

        if (self.__test_tx_verbosity(verbosity) == False):
            return(self.__error("verbosity value invalid"))
        #endif

        parms = "?genesis={}&page={}&limit={}&verbose={}".format( \
            self.genesis_id, page, limit, verbosity)
        url = users_url.format(sdk_url, "list/transactions") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef
       
    def nexus_users_list_transactions_by_username(self, page, limit,
        verbosity="default"):
        """
        List user account transactions by username. The entry offset (first 
        is 0) is specified in 'page' and the number of transactions returned 
        is specified in 'limit'. Valid values for 'verbosity' are "default", 
        "summary", "detail".
        """
        if (self.genesis_id == None): return(self.__error("Not logged in"))

        if (self.__test_tx_verbosity(verbosity) == False):
            return(self.__error("verbosity value invalid"))
        #endif

        parms = "?usernam={}&page={}&limit={}&verbose={}".format( \
            self.username, page, limit, verbosity)
        url = users_url.format(sdk_url, "list/transactions") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef
       
    def nexus_users_list_notifications_by_genesis(self, page, limit):
        """
        List user account notifications by genesis-id. The entry offset (first
        is 0) is specified in 'page' and the number of transactions returned 
        is specified in 'limit'.
        """
        if (self.genesis_id == None): return(self.__error("Not logged in"))

        parms = "?genesis={}&page={}&limit={}".format(self.genesis_id, page,
            limit)
        url = users_url.format(sdk_url, "list/notifications") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef
       
    def nexus_users_list_notifications_by_username(self, page, limit):
        """
        List user account notifications by username. The entry offset (first
        is 0) is specified in 'page' and the number of transactions returned 
        is specified in 'limit'.
        """
        if (self.genesis_id == None): return(self.__error("Not logged in"))

        parms = "?username={}&page={}&limit={}".format(self.username, page,
            limit)
        url = users_url.format(sdk_url, "list/notifications") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef
       
    def nexus_users_list_assets_by_genesis(self, page, limit):
        """
        List user account created assets by genesis-id. The number of assets
        entries returned is specified in 'limit' starting with entry offset 
        specified in 'page'. 
        """
        if (self.genesis_id == None): return(self.__error("Not logged in"))

        parms = "?genesis={}&page={}&limit={}".format(self.genesis_id, page,
            limit)
        url = users_url.format(sdk_url, "list/assets") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef
       
    def nexus_users_list_assets_by_username(self, page, limit):
        """
        List user account created assets by username. The number of assets
        entries returned is specified in 'limit' starting with entry offset 
        specified in 'page'. 
        """
        if (self.genesis_id == None): return(self.__error("Not logged in"))

        parms = "?username={}&page={}&limit={}".format(self.username, page,
            limit)
        url = users_url.format(sdk_url, "list/assets") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_users_list_accounts_by_genesis(self):
        """
        List user created token accounts by genesis-id.
        """
        if (self.genesis_id == None): return(self.__error("Not logged in"))

        parms = "?genesis={}".format(self.genesis_id)
        url = users_url.format(sdk_url, "list/accounts") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_users_list_accounts_by_username(self):
        """
        List user created token accounts by username.
        """
        if (self.genesis_id == None): return(self.__error("Not logged in"))

        parms = "?username={}".format(self.username)
        url = users_url.format(sdk_url, "list/accounts") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_users_list_tokens_by_genesis(self):
        """
        List user created tokens by genesis-id.
        """
        if (self.genesis_id == None): return(self.__error("Not logged in"))

        parms = "?genesis={}".format(self.genesis_id)
        url = users_url.format(sdk_url, "list/tokens") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_users_list_tokens_by_username(self):
        """
        List user created tokens by username.
        """
        if (self.genesis_id == None): return(self.__error("Not logged in"))

        parms = "?username={}".format(self.username)
        url = users_url.format(sdk_url, "list/tokens") + parms
        json_data = self.__get(url) 
        return(json_data)
    #enddef

    def nexus_supply_create_item(self, name, data):
        """
        Create supply chain item with name specified in 'name'. Store data 
        with item specified in 'data'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # URL quote data since specical characters in data string don't
        # conflict with URI encoding characters.
        #
        if (type(data) != str): data = str(data)
        data = data.replace("&", "%26")
        data = urllib.quote_plus(data)

        parms = "?pin={}&session={}&name={}&data={}".format(self.pin,
            self.session_id, name, data)

        url = supply_url.format(sdk_url, "create/item") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_get_item_by_name(self, name):
        """
        Get supply chain item from blockchain using name specified in 'name'.
        """
        parms = "?name={}".format(name)
        url = supply_url.format(sdk_url, "get/item") + parms
        json_data = self.__get(url)

        #
        # Unquote data if "state" key is present.
        #
        if (json_data.has_key("result")):
            data = urllib.unquote_plus(json_data["result"]["data"])
            json_data["result"]["state"] = data.replace("%26", "&")
        #endif
        return(json_data)
    #enddef

    def nexus_supply_get_item_by_address(self, address):
        """
        Get supply chain item from blockchain using register address specified
        in 'address'.
        """
        parms = "?address={}".format(address)
        url = supply_url.format(sdk_url, "get/item") + parms
        json_data = self.__get(url)

        #
        # Unquote data if "state" key is present.
        #
        if (json_data.has_key("result")):
            data = urllib.unquote_plus(json_data["result"]["data"])
            json_data["result"]["state"] = data.replace("%26", "&")
        #endif
        return(json_data)
    #enddef

    def nexus_supply_transfer_item_by_name(self, name, new_owner):
        """
        Transfer supply chain item, specified by name in 'name' to new owner.
        The new owner username is specified in 'new_owner'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&name={}&username={}".format( \
            self.pin, self.session_id, name, new_owner)
        url = supply_url.format(sdk_url, "transfer/item") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_transfer_item_by_address(self, address, new_owner):
        """
        Transfer supply chain item, specified by address in 'address' to new 
        owner. The new owner genesis-id is specified in 'new_owner'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&address={}&destination={}".format( \
            self.pin, self.session_id, address, new_owner)
        url = supply_url.format(sdk_url, "transfer/item") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_claim_item(self, txid):
        """
        Claim supply chain item that has been transfered to username. The
        transaction id, specified in 'txid' is returned from when the
        user did a supply/transfer/item API call.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&txid={}".format(self.pin, self.session_id,
            txid)
        url = supply_url.format(sdk_url, "claim/item") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_update_item_by_name(self, name, data):
        """
        Update data stored from a previously created supply chain item using
        name specified in 'name' with data stored in 'data'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # URL quote data since specical characters in data string don't
        # conflict with URI encoding characters.
        #
        if (type(data) != str): data = str(data)
        data = data.replace("&", "%26")
        data = urllib.quote_plus(data)

        parms = "?pin={}&session={}&name={}&data={}".format(self.pin,
            self.session_id, name, data)
        url = supply_url.format(sdk_url, "update/item") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_update_item_by_address(self, address, data):
        """
        Update data stored from a previously created supply chain item using
        register address specifiied in 'address' with data stored in 'data'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # URL quote data since specical characters in data string don't
        # conflict with URI encoding characters.
        #
        if (type(data) != str): data = str(data)
        data = data.replace("&", "%26")
        data = urllib.quote_plus(data)

        parms = "?pin={}&session={}&address={}&data={}".format(self.pin,
            self.session_id, address, data)
        url = supply_url.format(sdk_url, "update/item") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_list_item_history_by_name(self, name):
        """
        List supply chain item history by name specified in 'name'.
        """
        parms = "?name={}".format(name)
        url = supply_url.format(sdk_url, "list/item/history") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_list_item_history_by_address(self, address):
        """
        List supply chain item history by address specified in 'address'.
        """
        parms = "?address={}".format(address)
        url = supply_url.format(sdk_url, "list/item/history") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_assets_create_asset(self, asset_name, data):
        """
        Create asset with name 'asset_name' and store data specified in
        'data' for asset transaction.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # URL quote data since specical characters in data string don't
        # conflict with URI encoding characters.
        #
        if (type(data) != str): data = str(data)
        data = data.replace("&", "%26")
        data = urllib.quote_plus(data)

        parms = "?pin={}&session={}&name={}&format=raw&data={}".format( \
            self.pin, self.session_id, asset_name, data)
        url = assets_url.format(sdk_url, "create/asset") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_assets_get_asset_by_name(self, asset_name):
        """
        Get asset with by name 'asset_name'.
        """
        parms = "?name={}".format(asset_name)
        url = assets_url.format(sdk_url, "get/asset") + parms
        json_data = self.__get(url)

        #
        # Unquote data if "metadata" key is present.
        #
        if (json_data.has_key("result")):
            data = urllib.unquote_plus(json_data["result"]["metadata"])
            json_data["result"]["metadata"] = data.replace("%26", "&")
        #endif
        return(json_data)
    #enddef

    def nexus_assets_get_asset_by_address(self, asset_address):
        """
        Get asset with by register address 'asset_address'.
        """
        parms = "?address={}".format(asset_address)
        url = assets_url.format(sdk_url, "get/asset") + parms
        json_data = self.__get(url)

        #
        # Unquote data if "metadataa" key is present.
        #
        if (json_data.has_key("result")):
            data = urllib.unquote_plus(json_data["result"]["metadata"])
            json_data["result"]["metadata"] = data.replace("%26", "&")
        #endif
        return(json_data)
    #enddef

    def nexus_assets_update_asset_by_name(self, asset_name, data):
        """
        Update data stored in asset by name specified in 'asset_name' with
        JSON data stored in 'data'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&name={}&data={}".format(self.pin,
            self.session_id, asset_name, data)
        url = assets_url.format(sdk_url, "update/asset") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_assets_update_asset_by_address(self, asset_address, data):
        """
        Update data stored in asset by address specified in 'asset_address'
        with JSON data stored in 'data'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&address={}&data={}".format( \
            self.pin, self.session_id, asset_address, data)
        url = assets_url.format(sdk_url, "update/asset") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_assets_transfer_asset_by_name(self, asset_name, dest_username):
        """
        Transfer ownership of an asset by name specified in 'asset_name' to 
        username specified in 'dest_username'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&name={}&username={}".format(self.pin,
            self.session_id, asset_name, dest_username)
        url = assets_url.format(sdk_url, "transfer/asset") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_assets_transfer_asset_by_address(self, asset_address,
        dest_address):
        """
        Transfer ownership of an asset by address specified in 'asset_address'
        to address specified in 'dest_address'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&address={}&destination={}".format( \
            self.pin, self.session_id, asset_address, dest_address)
        url = assets_url.format(sdk_url, "transfer/asset") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_assests_claim_asset(self, txid):
        """
        Claim an asset that has been transfered to username. The transaction 
        id, specified in 'txid' is returned from when the user did a 
        assets//transfer/asset API call.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&txid={}".format(self.pin, self.session_id,
            txid)
        url = supply_url.format(sdk_url, "claim/asset") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_assets_tokenize_by_name(self, asset_name, token_name):
        """
        Tokenize an asset specified by 'asset_name' to a previously created 
        token specified by 'token_name'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&name={}&token_name={}".format( \
            self.pin, self.session_id, asset_name, token_name)
        url = assets_url.format(sdk_url, "tokenize/asset") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_assets_tokenize_by_address(self, asset_address, token_address):
        """
        Tokenize an asset specified by 'asset_address' to a previously created 
        token specified by 'token_address'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&address={}&token_address={}". \
            format(self.pin, self.session_id, asset_address, token_address)
        url = assets_url.format(sdk_url, "tokenize/asset") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_assets_list_asset_history_by_name(self, asset_name):
        """
        List assets history by name specified in 'name'.
        """
        parms = "?name={}".format(asset_name)
        url = assets_url.format(sdk_url, "list/asset/history") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_assets_list_asset_history_by_address(self, asset_address):
        """
        List assets history by address specified in 'address'.
        """
        parms = "?address={}".format(asset_address)
        url = assets_url.format(sdk_url, "list/asset/history") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef


    def nexus_tokens_create_token(self, token_name, supply, digits=2):
        """
        Create a token by name with with an initial reserve supply of 
        'supply'. Give the token accuracy precision specified in 'digits'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&name={}&supply={}&digits={}".format( \
            self.pin, self.session_id, token_name, supply, digits)
        url = tokens_url.format(sdk_url, "create/token") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_create_account(self, account_name, token_name):
        """
        Create a token account associated with previously create token
        with token name 'token_name'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&name={}&token_name={}".format(self.pin,
            self.session_id, account_name, token_name)
        url = tokens_url.format(sdk_url, "create/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_get_token_by_name(self, token_name):
        """
        Get token by name specified in 'token_name'.
        """
        parms = "?name={}".format(token_name)
        url = tokens_url.format(sdk_url, "get/token") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_get_token_by_address(self, token_address):
        """
        Get token by address specified in 'token_address'.
        """
        parms = "?address={}".format(token_address)
        url = tokens_url.format(sdk_url, "get/token") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_get_account_by_name(self, account_name):
        """
        Get token account by name specified in 'account_name'.
        """
        parms = "?name={}&type=account".format(account_name)
        url = tokens_url.format(sdk_url, "get/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_get_account_by_address(self, account_address):
        """
        Get token account by address specified in 'account_address'.
        """
        parms = "?address={}&type=account".format(account_address)
        url = tokens_url.format(sdk_url, "get/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_debit_account_by_name(self, from_name, to_name, amount):
        """
        Debit token account from token account 'from_name' for the amount
        specified in 'amount' for token account 'to_name'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # Arguments from_name and to_name are token account names.
        #
        parms = "?pin={}&session={}&amount={}&name={}&name_to={}". \
            format(self.pin, self.session_id, amount, from_name, to_name)
        url = tokens_url.format(sdk_url, "debit/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_debit_account_by_address(self, from_address, to_address,
        amount):
        """
        Debit token account from token account 'from_name' for the amount
        specified in 'amount' for token account 'to_name'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # Arguments from_address and to_address are token account addresses.
        #
        parms = "?pin={}&session={}&amount={}&address={}&address_to={}". \
            format(self.pin, self.session_id, amount, from_address, to_address)
        url = tokens_url.format(sdk_url, "debit/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_debit_token_by_name(self, from_name, to_name, amount):
        """
        Debit token with name specified iin 'from_name' for the amount
        specified in 'amount' for token account 'to_name'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # Arguments from_name and to_name are token account names.
        #
        parms = "?pin={}&session={}&amount={}&name={}&name_to={}". \
            format(self.pin, self.session_id, amount, from_name, to_name)
        url = tokens_url.format(sdk_url, "debit/token") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_debit_token_by_address(self, from_address, to_address,
        amount):
        """
        Debit token by address specified iin 'from_address' for the amount
        specified in 'amount' for token account address 'to_address'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # Arguments from_address and to_address are token account addresses.
        #
        parms = "?pin={}&session={}&amount={}&address={}&address_to={}". \
            format(self.pin, self.session_id, amount, from_address, to_address)
        url = tokens_url.format(sdk_url, "debit/token") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_credit_token_by_name(self, to_name, amount, txid):
        """
        Credit token by name specified in 'name' for the amount specified in 
        'amount' from the debit transaction with transaction id 'txid'. This
        function is used to replenish tokens.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # Argument from_name is a token account name.
        #
        parms = "?pin={}&session={}&txid={}&amount={}&name={}".format( \
            self.pin, self.session_id, txid, amount, to_name)

        url = tokens_url.format(sdk_url, "credit/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_credit_token_by_address(self, to_address, amount, txid):
        """
        Credit token account by address specified in 'to_address' for the 
        amount specified in 'amount' from the debit transaction with 
        transaction id 'txid'. This function is used to replenish tokens.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # Argument from_address is a token account address.
        #
        parms = ("?pin={}&session={}&txid={}&amount={}&address={}"). \
            format(self.pin, self.session_id, txid, amount, to_address)

        url = tokens_url.format(sdk_url, "credit/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_credit_account_by_name(self, to_name, amount, txid,
        name_proof=None):
        """
        Credit token account by name specified in 'to_name' for the amount
        specified in 'amount' from the debit transaction with transaction
        id 'txid'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # Argument from_name is a token account name.
        #
        parms = "?pin={}&session={}&txid={}&amount={}&name={}".format( \
            self.pin, self.session_id, txid, amount, to_name)

        if (name_proof != None): parms += "&name_proof={}".format(name_proof)

        url = tokens_url.format(sdk_url, "credit/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_tokens_credit_account_by_address(self, to_address, amount, txid,
        address_proof):
        """
        Credit token account by address specified in 'to_address' for the 
        amount specified in 'amount' from the debit transaction with 
        transaction id 'txid'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # Argument from_address is a token account address.
        #
        parms = ("?pin={}&session={}&txid={}&amount={}&address={}"). \
            format(self.pin, self.session_id, txid, amount, to_address)

        if (address_proof != None): parms += "&proof={}".format(address_proof)
                 
        url = tokens_url.format(sdk_url, "credit/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_finance_create_account(self, name):
        """
        Create a NXS crypto-currency account with name specified in 'name'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # Argument from_address is a token account address.
        #
        parms = "?pin={}&session={}&name={}".format(self.pin, self.session_id,
            name)
        url = finance_url.format(sdk_url, "create/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_finance_get_account_by_name(self, name):
        """
        Get NXS crypto-currency account information with name specified in 
        'name'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?session={}&name={}".format(self.session_id, name)
        url = finance_url.format(sdk_url, "get/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_finance_get_account_by_address(self, address):
        """
        Get NXS crypto-currency account information with register address
        specified in 'address'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?session={}&address={}".format(self.session_id, address)
        url = finance_url.format(sdk_url, "get/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_finance_debit_account_by_name(self, from_name, to_name, amount):
        """
        Debit NXS by name from account 'from_name' to account 'to_name' for the
        amount 'amount'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&amount={}&name={}&name_to={}". \
            format(self.pin, self.session_id, amount, from_name, to_name)
        url = finance_url.format(sdk_url, "debit/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_finance_debit_account_by_address(self, from_address, to_address,
        amount):
        """
        Debit NXS by address from account 'from_address' to account 
        'to_address' for the amount 'amount'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&amount={}&address={}&address_to={}". \
            format(self.pin, self.session_id, amount, from_address, to_address)
        url = finance_url.format(sdk_url, "debit/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_finance_credit_account_by_name(self, to_name, amount, txid):
        """
        Credit NXS by name to account 'to_name' for account 'amount' from
        debit transaction-id 'txid'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # Argument from_name is a token account name.
        #
        parms = "?pin={}&session={}&txid={}&amount={}&name={}".format( \
            self.pin, self.session_id, txid, amount, to_name)

        url = finance_url.format(sdk_url, "credit/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_finance_credit_account_by_address(self, to_address, amount,
        txid):
        """
        Credit NXS by address to account 'to_address' for account 'amount' from
        debit transaction-id 'txid'.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # Argument from_name is a token account name.
        #
        parms = "?pin={}&session={}&txid={}&amount={}&addrss={}".format( \
            self.pin, self.session_id, txid, amount, to_address)

        url = finance_url.format(sdk_url, "credit/account") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_finance_list_accounts(self):
        """
        List NXS transactions for user account.
        """
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?session={}".format(self.session_id)

        url = finance_url.format(sdk_url, "list/accounts") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def __get(self, url):
        try:
            connect = urllib2.urlopen(url)
            text = connect.read()
        except:
            try:
                connect = urllib.urlopen(url)
                text = connect.read()
            except:
                return(self.__error("Connection refused"))
            #endtry
        #endtry

        if (text == None):
            api = url.split("?")[0]
            api = api.split("/")
            api = api[-2] + "/" + api[-1]
            return(self.__error("{} returned no JSON".format(api)))
        #endif
        return(json.loads(text))
    #enddef

    def __error(self, message):
        json_data = { "error" : {}, "result" : None }
        json_data["error"]["code"] = -1
        json_data["error"]["sdk-error"] = True
        json_data["error"]["message"] = message
        return(json_data)
    #enddef

    def __test_tx_verbosity(self, verbosity):
        return(verbosity in ["default", "summary", "detail"])
    #enddef
#endclass

#------------------------------------------------------------------------------


