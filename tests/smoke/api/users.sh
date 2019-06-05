cd ../../../


#Create
echo "===Create==="
./nexus users/create/user/paul password=paul pin=1234

#Login
echo "===Login==="
./nexus users/login/user/paul password=paul pin=1234

#List your accounts (note that account names are only returned for the logged in user)
echo "===List/Accounts==="
./nexus users/list/accounts/paul

#List someone elses accounts when you know their username
echo "===List/Accounts==="
./nexus users/list/accounts/jack

#list someone eles account when you only know their genesis
echo "===List/Accounts==="
./nexus users/list/accounts/1ff463e036cbde3595fbe2de9dff15721a89e99ef3e2e9bfa7ce48ed825e9ec2

#List your tokens
echo "===List/Tokens==="
./nexus users/list/tokens/paul

#List your assets
echo "===List/Assets==="
./nexus users/list/assets/paul

#List your notifications (transactions you need to credit/claim)
echo "===List/Notifications==="
./nexus users/list/notifications/paul
