#Login Paul, create token account abc_savings for token abc and logout
./api_call.sh users/login/user/paul password=paul pin=1234
./api_call.sh tokens/create/account/abc_savings token_name=abc pin=1234
./api_call.sh users/logout/user/paul password=paul pin=1234

#Login
./api_call.sh users/login/user/jack password=jack pin=1234

#Create a new token called ABC with initial supply 1000 and digits 2
./api_call.sh tokens/create/token/abc supply=1000 digits=2 pin=1234

#Create an account called abc_main to use with the abc token (note this example assumes that YOU created the token so it is in your namespace)
./api_call.sh tokens/create/account/abc_main token_name=abc pin=1234

#Create an account called abc_savings to use with the abc token
./api_call.sh tokens/create/account/abc_savings token_name=abc pin=1234

#Send (debit) 10 abc tokens from the initial supply to your abc_main account
./api_call.sh tokens/debit/token/abc name_to=abc_main amount=10 pin=1234

#Send (debit) 10 abc tokens from the initial supply to your abc_main account (referenced by address hash)
./api_call.sh tokens/debit/token/abc address_to=d964c9a45b304d7945790c2806ef3cde2e382742b513da1809bbc5522d11fc3d amount=10 pin=1234

#Receive (credit) the above debit into your account
./api_call.sh tokens/credit/account txid=4b7f1437b0a21e5913993939626602777a99496b80bde0b075c6990b2964cf1b16d16799c289b73c30f8b331602475790ad0b2037747844f1f8a3e2d801399a4 pin=1234

#Receive (credit) the above debit into your account
./api_call.sh tokens/credit/account txid=dbeb515505c7b93d5845501fcb61a6e597d1fda81b97f142e4e3cad6e7f7c3a1151b9023ec14387cc7a7b14048f1ec8f421b3b318c78519213b6b8bbc0438b66 pin=1234

#Send (debit) 10 abc tokens from your abc_main account to paul's abc_savings account
./api_call.sh tokens/debit/account/abc_main name_to=paul:abc_savings amount=10 pin=1234

#Get token information
./api_call.sh tokens/get/token/abc

#Get token account information
./api_call.sh tokens/get/account/abc_main

#Logout
./api_call.sh users/logout/user/jack password=jack pin=1234
