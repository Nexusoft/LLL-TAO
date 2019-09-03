#Login
./api_call.sh users/login/user/jack password=jack pin=1234

#Create a new token called ABC with initial supply 1000 and decimals 2
./api_call.sh tokens/create/token/abc supply=1000 decimals=2 pin=1234

#Logout
./api_call.sh users/logout/user/jack

#Login Paul, create token account abc_savings for token abc and logout
./api_call.sh users/login/user/paul password=paul pin=1234
./api_call.sh tokens/create/account/abc_savings token_name=jack:abc pin=1234
./api_call.sh users/logout/user/paul password=paul pin=1234


#Login Jack
./api_call.sh users/login/user/jack password=jack pin=1234

#Create an account called abc_main to use with the abc token (note this example assumes that YOU created the token so it is in your namespace)
./api_call.sh tokens/create/account/abc_main token_name=abc pin=1234

#Create an account called abc_savings to use with the abc token
./api_call.sh tokens/create/account/abc_savings token_name=abc pin=1234

#Send (debit) 10 abc tokens from the initial supply to your abc_main account
./api_call.sh tokens/debit/token/abc name_to=abc_main amount=10 pin=1234

#Send (debit) 10 abc tokens from the initial supply to your abc_main account (referenced by address hash)
./api_call.sh tokens/debit/token/abc address_to=d964c9a45b304d7945790c2806ef3cde2e382742b513da1809bbc5522d11fc3d amount=10 pin=1234

#Receive (credit) the above debit into your account
./api_call.sh tokens/credit/account txid=cb5f3ffe03738d6d5aa2dd5d04195bf4048c9e70698471fc2f28b0d0d64a975e3d782274727c9e76e7f6cd03fad0e0388a84998903d6ddb8a1d40b17ad518bc0 pin=1234

#Receive (credit) the above debit into your account
./api_call.sh tokens/credit/account txid=1156e603cb962596b328e6457a8db47fa83a1d5e7ed1f2efbc5c29ec76251b4b57e5712859409d09a891cdb497853790dd9a0b7a39dd8c42f5c5dfa10425c8d3 pin=1234

#Send (debit) 10 abc tokens from your abc_main account to paul's abc_savings account
./api_call.sh tokens/debit/account/abc_main name_to=paul:abc_savings amount=10 pin=1234

#Get token information
./api_call.sh tokens/get/token/abc

#Get token account information
./api_call.sh tokens/get/account/abc_main

#Logout
./api_call.sh users/logout/user/jack password=jack pin=1234


