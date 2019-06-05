cd ../../../

#Create a new NXS account called savings (note PIN is required)
echo "===Create/Account==="
./nexus finance/create/account name=savings pin=1234

#Send (debit) 10 NXS from your default account to savings
echo "===Debit/Account==="
./nexus finance/debit/account/default amount=10 name_to=savings pin=1234

#If you do NOT have the events processor running and configured to process debits, then this is how you create a credit to add the
#funds from the debit transaction to your account.
#the txid is the transaction ID of the debit transaciton (available from notifications)
echo "===Credit/Account==="
./nexus finance/credit/account txid=ca2741d25a5f00f59ffe20f1d34cac71a2dedb547e83ed20a9d59c3812527c7356f7a5662bb0305dd2a4fc36b1596583d1922fdc615a25e69a8c36596370c3cd pin=1234

#Send (debit) 10 NXS from your savings to bob's savings account
echo "===Debit/Account==="
./nexus finance/debit/account/savings amount=10 name_to=bob:savings pin=1234

#Send (debit) 10 NXS from your savings to someone's account when they only provided the address not the name
echo "===Debit/Account==="
./nexus finance/debit/account/savings amount=10 address_to=a73b0598e1aabf97398fd02563f7074acd5292d493aaed2697f4e2037519bc20 pin=1234

#List your NXS accounts
echo "===List/Accounts==="
./nexus finance/list/accounts

#List a NXS account by name
echo "===Get/Account==="
./nexus finance/get/account/default

#Get a particular field (the balance in this case) from the account info
echo "===Get/Account==="
./nexus finance/get/account/default/balance
