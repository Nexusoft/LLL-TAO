#
# nexus_sdk,py
#
# This is the Nexus python SDK to interface with the Nexus Tritium APIs.
#
# Currently, the following <api>/<method> APIs are supported:
#
# Accounts API:          Supply API:
# accounts/create        supply/getitem
# accounts/login         supply/transfer
# accounts/logout        supply/createitem
# accounts/transactions  supply/history
#
# Here is a program calling sequence to list transactions for 2 users:
#
# import nexus_sdk
#
# foo = nexus-sdk.sdk_init("user-foo", "pw-foo", "1234")
# if (foo):
#     json_data = foo.nexus_accounts_transactions()
#     print json_data
# #endif
#
# bar = nexus-sdk.sdk_init("user-bar", "pw-bar", "1234)
# if (bar):
#     json_data = bar.nexus_accounts_transactions()
#     print json_data
# #endif
#
#------------------------------------------------------------------------------

import requests
import json

accounts_url = "http://localhost:8080/accounts/{}"
supply_url = "http://localhost:8080/supply/{}"

#------------------------------------------------------------------------------

class sdk_init():
    def __init__(self, user, pw, pin, create_account=True):
        self.username = user
        self.password = pw
        self.pin = pin
        self.session_id = None
        self.genesis_id = None
    #enddef

    def nexus_accounts_login(self):
        parms = "?username={}&password={}".format(self.username, self.password)
        url = accounts_url.format("login") + parms
        json_data = self.__get(url)
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

    def nexus_accounts_logout(self):
        parms = "?session={}".format(self.session_id)
        url = accounts_url.format("logout") + parms
        json_data = self.__get(url)
        if (json_data != {}): self.session_id = None
        return(json_data)
    #enddef

    def nexus_accounts_transactions(self):
        parms = "?genesis={}".format(self.genesis_id)
        url = accounts_url.format("transactions") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef
       
    def nexus_supply_createitem(self):
        parms = "?pin={}&session={}".format(self.pin, self.session_id)
        url = supply_url.format("createitem") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_getitem(self, address):
        parms = "?address=0x{}".format(self.pin, address)
        url = supply_url.format("getitem") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_transfer(self, address, new_owner):
        parms = "?pin={}&session={}&address=0x{}&destinatio={}".format( \
            self.pin, self.session_id, address, new_owner)
        url = supply_url.format("transfer") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_history(self, address):
        parms = "?address=0x{}".format(address)
        url = supply_url.format("history") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def __get(self, url):
        r = requests.get(url)
        if (r == None or r.text == None): return(None)
        if (r.text == None):
            api = url.split("?")[0]
            api = api.split("/")
            api = api[-2] + "/" + api[-1]
            return(self.__error("{} returned no JSON".format(api)))
        return(json.loads(r.text))
    #enddef

    def __error(message):
        json_data = { "error" : {}, "result" : None }
        json_data["error"]["code"] = -1
        json_data["error"]["message"] = message
        return(json_data)
    #enddef
                
#endclass

#------------------------------------------------------------------------------


