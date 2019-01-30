#!/bin/bash

# get the dir that the script is executing in so that we can work relative to that
# to get to the default nexus app location
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

[[ "${NEXUS:-}" == "" ]] && NEXUS="${DIR}/../../nexus"
echo "NEXUS=${NEXUS}"
if [[ -x ${NEXUS} ]]; then
	echo "Nexus executable exists, starting tests"
else
	echo "Nexus executable didn't exist or wasn't executable, exiting."
	echo "Call the script like this to set it NEXUS=/path/to/binary/nexus ./run-start-nexus-test.sh"
	exit 42
fi

START_CMD="${NEXUS}"

# function to test an RPC command.  
# first param is the RPC method name, second param is the arguments to pass
testcommand () 
{
    local ts=$(date +%s%N) 
    echo "#### $1 ####"
    
    #support up to 4 args
    if (($# == 1 )); then echo "$1" ; eval $START_CMD $1;  fi
    if (($# == 2 )); then echo "$1 '$2'" ; eval $START_CMD $1 \'$2\';  fi
    if (($# == 3 )); then echo "$1 '$2' '$3'" ; eval $START_CMD $1 \'$2\' \'$3\'; fi
    if (($# == 4 )); then echo "x'$2' '$3' '$4'" ; eval $START_CMD $1 \'$2\' \'$3\' \'$4\'; fi
    
    local tt=$((($(date +%s%N) - $ts)/1000000))
    echo -e "#### $1 completed in $tt milliseconds ####\n"

}

ts=$(date +%s%N) 
#help
#testcommand help

#echo
testcommand echo 'this is the echo test' '{"teststring" : "string", "testbool" : true, "testnumber" : 123.456}' 

#stop

#getinfo
testcommand getinfo

#getmininginfo
testcommand getmininginfo

#getconnectioncount
testcommand getconnectioncount

#getnewaddress
testcommand getnewaddress

##getaccountaddress
testcommand getaccountaddress default

#setaccount
testcommand setaccount 2R6U417n8u8UFC2buPAbahv5qaWKpQ57H4apPN3LZCVeWzmMq7y testaccount

#getaccount
testcommand getaccount 2R6U417n8u8UFC2buPAbahv5qaWKpQ57H4apPN3LZCVeWzmMq7y

#getaddressesbyaccount
testcommand getaddressesbyaccount default

#walletpassphrase
testcommand walletpassphrase testwallet

#sendtoaddress
testcommand sendtoaddress 2R6U417n8u8UFC2buPAbahv5qaWKpQ57H4apPN3LZCVeWzmMq7y 0.1

#signmessage
testcommand signmessage 2R6U417n8u8UFC2buPAbahv5qaWKpQ57H4apPN3LZCVeWzmMq7y 'This is a test message'

#verifymessage
testcommand verifymessage 2R6U417n8u8UFC2buPAbahv5qaWKpQ57H4apPN3LZCVeWzmMq7y 'IAN1Vyr0TVU+IY+PSA9MU2QLx4ripOl0t5tav3S79ttKMXLHxxDKenAfwCGzjcDKMa9/sjgq3bLh4Hj+cggemDH/DfZwthliNAAMDj93ALBnNNfmwKEK8U01AZsVWTgb7CGpvs2z+YzsRsbV5Nru1IXaLbgEAXAnAlwcjqIKoGZbo+RoYsulyfhRplnp6hoRFA==' 'This is a test message'

#getreceivedbyaddress
testcommand getreceivedbyaddress 2R6U417n8u8UFC2buPAbahv5qaWKpQ57H4apPN3LZCVeWzmMq7y

#getreceivedbyaccount
testcommand getreceivedbyaccount testaccount

#getbalance
testcommand getbalance

#move

#addmultisigaddress
testcommand addmultisigaddress 2 '["2SXPpxvLa9r42mHHborGwvAJabinE5BL4XmuJXiHhobRuuy4pA1", "2SVMUciU5PYHKVZcq3ikA96Uq5oDXrGcmkiqsiP89Tp3idN13tf", "2SUPqf3EuE75TLa3EqpWfRtb9wvsVRVY77mat6zbZNYnFVExCTb"]'

#sendfrom
testcommand sendfrom default 2R6U417n8u8UFC2buPAbahv5qaWKpQ57H4apPN3LZCVeWzmMq7y 0.1

#sendmany
testcommand sendmany default '{"2R6U417n8u8UFC2buPAbahv5qaWKpQ57H4apPN3LZCVeWzmMq7y":0.1}'

#listreceivedbyaddress
testcommand listreceivedbyaddress 2R6U417n8u8UFC2buPAbahv5qaWKpQ57H4apPN3LZCVeWzmMq7y

#listreceivedbyaccount
testcommand listreceivedbyaccount testaccount

#listtransactions
testcommand listtransactions

#listaddresses
testcommand listaddresses

#listaccounts
testcommand listaccounts

#listsinceblock
#testcommand listsinceblock 2000000

#gettransaction

#getrawtransaction

#sendrawtransaction

#validateaddress
testcommand validateaddress 2R6U417n8u8UFC2buPAbahv5qaWKpQ57H4apPN3LZCVeWzmMq7y

#makekeypair
testcommand makekeypair

#unspentbalance
testcommand unspentbalance

#listunspent
testcommand listunspent

#getpeerinfo
testcommand getpeerinfo

#reset
testcommand reset


#getnetworkhashps
testcommand getnetworkhashps

#getnetworkpps
testcommand getnetworkpps

#getnetworktrustkeys
testcommand getnetworktrustkeys

#getblockcount
testcommand getblockcount

#getblocknumber

#getdifficulty
testcommand getdifficulty

#getsupplyrates
testcommand getsupplyrates

#getmoneysupply
testcommand getmoneysupply

#getblockhash

#isorphan

#getblock

#backupwallet

#keypoolrefill
testcommand keypoolrefill

#encryptwallet

#walletpassphrasechange

#walletlock

#checkwallet
testcommand checkwallet

#listtrustkeys
testcommand listtrustkeys

#repairwallet
testcommand repairwallet

#rescan

#importprivkey

#dumpprivkey


