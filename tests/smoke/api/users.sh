
#Create
./api_call.sh users/create/user/paul password=paul pin=1234

./api_call.sh users/create/user/jack password=jack pin=1234

./api_call.sh users/create/user username=bob password=bob pin=1234

#Login
./api_call.sh users/login/user/paul password=paul pin=1234

#Unlock
./api_call.sh users/unlock/user/paul password=paul pin=1234

#Lock
./api_call.sh users/lock/user/paul password=paul pin=1234

#Logout
./api_call.sh users/logout/user/paul password=paul pin=1234

./api_call.sh users/login/user/jack password=jack pin=1234

#List your accounts (note that account names are only returned for the logged in user)
./api_call.sh users/list/accounts/jack

#List someone elses accounts when you know their username
./api_call.sh users/list/accounts/paul

#list someone elses account when you only know their genesis
./api_call.sh users/list/accounts/1ff463e036cbde3595fbe2de9dff15721a89e99ef3e2e9bfa7ce48ed825e9ec2

#List your tokens
./api_call.sh users/list/tokens/jack

#List your assets
./api_call.sh users/list/assets/jack

#List your notifications (transactions you need to credit/claim)
./api_call.sh users/list/notifications/jack

./api_call.sh users/logout/user/jack password=jack pin=1234
