
#Create a new token called ABC with initial supply 1000 and digits 2
./api_call.sh tokens/create/token/abc supply=1000 digits=2 pin=1234

#Create an account called abc_main to use with the abc token (note this example assumes that YOU created the token so it is in your namespace)
./api_call.sh tokens/create/account/abc_main token_name=abc pin=1234

#Create an account called abc_savings to use with the abc token (note this example assumes that user paul created the token but you are logged in as someone else)
./api_call.sh tokens/create/account/abc_savings token_name=paul:abc pin=1234

#Send (debit) 100 abc tokens from the initial supply to your abc_main account
./api_call.sh tokens/debit/token/abc name_to=abc_main amount=100 pin=1234

#Receive (credit) the above debit into your account
./api_call.sh tokens/credit/account txid=0afb32d8316c9ad97504fbf26093a86cbce03ba8137c5f63e16991e3a99aee3cbb5696f0c889a75fe61541898d350f4023edca2e2b66545f7c2ef9af254f99f0 pin=1234

#Send (debit) 10 abc tokens from your abc_main account to paul's abc_savings account
./api_call.sh tokens/debit/account/abc_main name_to=paul:abc_savings amount=10 pin=1234

#Get token information
./api_call.sh tokens/get/token/abc

#Get token account information
./api_call.sh tokens/get/account/abc_main
