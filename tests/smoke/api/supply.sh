#Login
./api_call.sh users/login/user/jack password=jack pin=1234

#Create a supply item with simple data
./api_call.sh supply/create/item name=item1 data=123456789 pin=1234

#Retrieve an item by name
./api_call.sh supply/get/item/item1

#Retrieve an item by address
./api_call.sh supply/get/item/ee0ec5255644f730a9574b6813b6de2e7a892253c039cac77cd9af0ffcac10b8

#Update the item data
./api_call.sh supply/update/item/item1 data=987654321 pin=1234

#View an item history
./api_call.sh supply/list/item/history/item1

#Transfer an item to another user
./api_call.sh supply/transfer/item/item1 username=jack pin=1234

#Claim an item transferred to you
./api_call.sh supply/claim/item txid=430a91929c003f577f8d3a8f6388287d5353df2c3aef4149d33b9de0564808abd12d2a14c4c056968e73bc1276f93bfbfff7d6e4eabbae801b9d76c5a1f65015 pin=1234

#Logout
./api_call.sh users/logout/user/jack password=jack pin=1234
