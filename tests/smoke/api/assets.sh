#Create a simple raw asset
./api_call.sh assets/create/asset name=myasset format=raw data=somepieceofmeaningfuldata pin=1234

#Create an asset in basic format (read only) with multiple fields
./api_call.sh assets/create/asset name=myasset2 somefield=somevalue someotherfield=someothervalue lastfield=lastfieldvalue pin=1234

#Create an asset using the json format to define multiple fields with explicit field type and mutability.
#NOTE In this example the description field is updatable (mutable)
#NOTE The json payload is designed to be submitted via "application/json" content in a GET/POST HTTP request, which is why this
#is hard to read on a CLI but it looks like this:
#[
#    {
#        "name": "serial_number",
#        "type": "uint64",
#        "value": "1234",
#        "mutable" : "false"
#    },
#    {
#        "name": "description",
#        "type": "string",
#        "value": "This is the description of my asset",
#        "mutable" : "true"
#    }
#]
./api_call.sh assets/create/asset name=myasset3 format=JSON json='[{"name": "serial_number","type": "uint64","value": "1234","mutable" : "false"},{"name": "description","type": "string","value": "This is the description of my asset","mutable" : "true"}]' pin=1234

#Retrieve an asset by name that you own
./api_call.sh assets/get/asset/myasset

#Retrieve an asset by name that someone else owns
./api_call.sh assets/get/asset/jack:myasset

#Retrieve an asset by address
./api_call.sh assets/get/asset/a3285be4e4d29f902de73d62aed93e1b28d4d7596cc61a02b489a0e6298e68b9

#Update a mutable field in an asset (you must own it)
./api_call.sh assets/update/asset/myasset3 description="this is the new description" pin=1234

#Transfer an asset to another signature chain (by username)
./api_call.sh assets/transfer/asset/myasset username=jack pin=1234

#Transer an asset to another signature chain (by genesis has)
./api_call.sh assets/transfer/asset/myasset2 destination=1ff463e036cbde3595fbe2de9dff15721a89e99ef3e2e9bfa7ce48ed825e9ec2 pin=1234

#Claim ownership of an asset that has been transferred to you
#(or claim back ownwership if you transferred it and it hasn't yet been claimed)
./api_call.sh assets/claim/asset txid=5fa2381c0b7ba0fbb3cc43f7ac46751b4399dcb6a0ebe2dac4a0c14d57c32695fd185564a2041b1ea04482bfb27fdb7c3ff4792767fa265fdf9849baa8c92e77

#Tokenize an asset
./api_call.sh assets/tokenize/asset name=myasset token_name=abc pin=1234
