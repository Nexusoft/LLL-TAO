cd ../../../

#Create a supply item with simple data
echo "===Create/Item==="
./nexus supply/create/item name=item1 data=123456789 pin=1234

#Retrieve an item by name
echo "===Get/Item==="
./nexus supply/get/item/item1

#Retrieve an item by address
echo "===Get/Item==="
./nexus supply/get/item/ee0ec5255644f730a9574b6813b6de2e7a892253c039cac77cd9af0ffcac10b8

#Update the item data
echo "===Update/Item==="
./nexus supply/update/item/item1 data=987654321 pin=1234

#View an item history
echo "===List/Item==="
./nexus supply/list/item/history/item1

#Transfer an item to another user
echo "===Transfer/Item==="
./nexus supply/transfer/item/item1 username=jack pin=1234

#Claim an item transferred to you
echo "===Claim/Item==="
./nexus supply/claim/item txid=430a91929c003f577f8d3a8f6388287d5353df2c3aef4149d33b9de0564808abd12d2a14c4c056968e73bc1276f93bfbfff7d6e4eabbae801b9d76c5a1f65015 pin=1234
