#
# nexus_sdk.py
#
# This is the Nexus python SDK to interface with the Nexus Tritium APIs.
#
# Currently, the following <api>/<method> APIs are supported:
#
#   Accounts API:          Supply API:
#   accounts/create        supply/getitem
#   accounts/login         supply/transfer
#   accounts/logout        supply/createitem
#   accounts/transactions  supply/updateitem
#                          supply/history
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

import urllib
import urllib2
import json

accounts_url = "http://localhost:8080/accounts/{}"
supply_url = "http://localhost:8080/supply/{}"

#------------------------------------------------------------------------------

class sdk_init():
    def __init__(self, user, pw, pin):
        self.username = user
        self.password = pw
        self.pin = pin
        self.session_id = None
        self.genesis_id = None
    #enddef

    def nexus_accounts_create(self):
        pw = self.password.replace("&", "%26")
        pw = urllib.quote_plus(pw)
        parms = "?username={}&password={}&pin={}".format(self.username, pw,
            self.pin)
        url = accounts_url.format("create") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_accounts_login(self):
        if (self.session_id != None): return(self.__error("Already logged in"))

        pw = self.password.replace("&", "%26")
        pw = urllib.quote_plus(pw)
        parms = "?username={}&password={}".format(self.username, pw)
        url = accounts_url.format("login") + parms
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

    def nexus_accounts_logout(self):
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?session={}".format(self.session_id)
        url = accounts_url.format("logout") + parms
        json_data = self.__get(url)
        if (json_data.has_key("error")): return(json_data)

        if (json_data != {}): self.session_id = None
        return(json_data)
    #enddef

    def nexus_accounts_transactions(self):
        if (self.genesis_id == None): return(self.__error("Not logged in"))

        parms = "?genesis={}".format(self.genesis_id)
        url = accounts_url.format("transactions") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef
       
    def nexus_supply_createitem(self, data):
        if (self.session_id == None): return(self.__error("Not logged in"))

        #
        # URL quote data since specical characters in data string don't
        # conflict with URI encoding characters.
        #
        if (type(data) != str): data = str(data)
        data = data.replace("&", "%26")
        data = urllib.quote_plus(data)

        parms = "?pin={}&session={}&data={}".format(self.pin, self.session_id,
            data)
        url = supply_url.format("createitem") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_updateitem(self, address, data):
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
        url = supply_url.format("updateitem") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_getitem(self, address):
        parms = "?address={}".format(address)
        url = supply_url.format("getitem") + parms
        json_data = self.__get(url)

        #
        # Unquote data if "state" key is present.
        #
        if (json_data.has_key("result")):
            data = urllib.unquote_plus(json_data["result"]["state"])
            json_data["result"]["state"] = data.replace("%26", "&")
        #endif
        return(json_data)
    #enddef

    def nexus_supply_transfer(self, address, new_owner):
        if (self.session_id == None): return(self.__error("Not logged in"))

        parms = "?pin={}&session={}&address={}&destination={}".format( \
            self.pin, self.session_id, address, new_owner)
        url = supply_url.format("transfer") + parms
        json_data = self.__get(url)
        return(json_data)
    #enddef

    def nexus_supply_history(self, address):
        parms = "?address={}".format(address)
        url = supply_url.format("history") + parms
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
                
    def __login(self, user, pw, pin, create_account):
        json_data = self.nexus_accounts_login()

        #
        # Already logged in?
        #
        if (json_data.has_key("error") and json_data["error"].has_key("code")):
            if (json_data["error"]["code"] == -22): return
            if (create_account == False): return

            #
            # Account does not exist. Creatr it and then log in.
            #
            if (json_data["error"]["code"] == -26):
                json_data = self.nexus_accounts_create()
            #endif
        #endry
        return(json_data)
#endclass

#------------------------------------------------------------------------------


