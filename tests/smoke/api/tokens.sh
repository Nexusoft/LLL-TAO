cd ../../../


#Create a new token called ABC with initial supply 1000 and digits 2
echo "===Create/Token==="
./nexus tokens/create/token/abc supply=1000 digits=2 pin=1234

#Create an account called abc_main to use with the abc token (note this example assumes that YOU created the token so it is in your namespace)
echo "===Create/Account==="
./nexus tokens/create/account/abc_main token_name=abc pin=1234

#Create an account called abc_svings to use with the abc token (note this example assumes that user paul created the token but you are logged in as someone else )
echo "===Create/Account==="
./nexus tokens/create/account/abc_savings token_name=paul:abc pin=4567

#Send (debit) 100 abc tokens from the initial supply to your abc_main account
echo "===Debit/Token==="
./nexus tokens/debit/token/abc name_to=abc_main amount=100 pin=1234

#Receive (credit) the above debit into your account
echo "===Credit/Account==="
./nexus tokens/credit/account txid=df5a97bc105170ec7722476586c66c08e12750242e31a8ab3953fc103dff9326894bc42628cb718311f4f0b3617018b0c1ab2d9cc34d43d2586c0e006f846ebc pin=1234

#Send (debit) 10 abc tokens from your abc_main account to jack's abc_savings account
echo "===Debit/Account==="
./nexus tokens/debit/account/abc_main name_to=jack:abc_savings amount=10 pin=1234

#Get token information
echo "===Get/Token==="
./nexus tokens/get/token/abc

#Get token account information
echo "===Get/Account==="
./nexus tokens/get/account/abc_main
