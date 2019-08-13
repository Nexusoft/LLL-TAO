/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_API_TYPES_RPC_H
#define NEXUS_TAO_API_TYPES_RPC_H

#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** RPC
         *
         *  RPC API Class.
         *  Manages the function pointers for all RPC commands.
         *
         **/
        class RPC : public Base
        {
        public:

            /** Default Constructor. **/
            RPC() { Initialize(); }


            /** Virtual destructor. **/
            virtual ~RPC()
            {
                mapFunctions.clear();
            }


            /** Initialize.
             *
             *  Sets the function pointers for this API.
             *
             **/
            void Initialize() final;


            /** GetName
             *
             *  Returns the name of this API.
             *
             **/
            std::string GetName() const final
            {
                return "RPC";
            }


            /** SanitizeParams
            *
            *  Allows derived API's to check the values in the parameters array for the method being called.
            *  The return json contains the sanitized parameter values, which derived implementations might convert to the correct type
            *  for the method being called.
            *
            *  @param[in] strMethod The name of the method being invoked.
            *  @param[in] jsonParams The json array of parameters being passed to this method.
            *
            *  @return the sanitized json parameters array.
            *
            **/
            virtual json::json SanitizeParams( const std::string& strMethod, const json::json& jsonParams );


            /** Echo
             *
             *  echo <params>
             *  Test method to echo back the parameters passed by the caller
             *
             *  @param[in] params Parameters array passed by the caller
             *
             *  @return JSON containing the user supplied parameters array
             *
             **/
            json::json Echo(const json::json& params, bool fHelp);


            /** Help
             *
             *  help
             *  Returns help list. Iterates through all functions in mapFunctions
             *  and calls each one with fHelp=true
             *
             *  @param[in] params Parameters array passed by the caller
             *
             *  @return JSON containing the help list
             *
             **/
            json::json Help( const json::json& params, bool fHelp);


            /** Stop
             *
             *  stop
             *  Stop Nexus server
             *
             *  @param[in] params Parameters array passed by the caller
             *
             *  @return JSON containing the help list
             *
             **/
            json::json Stop(const json::json& params, bool fHelp);


            /** GetInfo
             *
             *  getinfo
             *  Returns the general information about the current state of the wallet
             *
             *  @param[in] params Parameters array passed by the caller.
             *
             *  @return JSON containing the information.
             *
             **/
            json::json GetInfo(const json::json& params, bool fHelp);


            /** GetMiningInfo
             *
             *  getmininginfo
             *  Returns data about each connected network node
             *
             *  @param[in] params Parameters array passed by the caller.
             *
             *  @return JSON containing the information.
             *
             **/
            json::json GetMiningInfo(const json::json& params, bool fHelp);


            /** GetConnectionCount
             *
             *  getconnectioncount
             *  Returns the number of connections to other nodes
             *
             *  @param[in] params Parameters array passed by the caller.
             *
             *  @return JSON containing the information.
             *
             **/
            json::json GetConnectionCount(const json::json& params, bool fHelp);


            /** GetNewAddress
             *
             *  getnewaddress [account]
             *  Returns a new Nexus address for receiving payments
             *
             *  @param[in] params Parameters array passed by the caller.
             *
             *  @return JSON containing the information.
             *
             **/
            json::json GetNewAddress(const json::json& params, bool fHelp);


            /** GetAccountAddress
             *
             *  getaccountaddress <account>
             *  Returns the current Nexus address for receiving payments to this account
             *
             *  @param[in] params Parameters array passed by the caller.
             *
             *  @return JSON containing the information.
             *
             **/
            json::json GetAccountAddress(const json::json& params, bool fHelp);


            /** SetAccount
             *
             *  setaccount <Nexusaddress> <account>
             *  Sets the account associated with the given address
             *
             *  @param[in] params Parameters array passed by the caller.
             *
             *  @return JSON containing the information.
             *
             **/
            json::json SetAccount(const json::json& params, bool fHelp);


            /** GetAccount
             *
             *  getaccount <Nexusaddress>
             *  Returns the account associated with the given address
             *
             *  @param[in] params Parameters array passed by the caller.
             *
             *  @return JSON containing the information.
             *
             **/
            json::json GetAccount(const json::json& params, bool fHelp);


            /** GetAddressesByAccount
             *
             *  getaddressesbyaccount <account>
             *  Sets the account associated with the given address
             *
             *  @param[in] params Parameters array passed by the caller.
             *
             *  @return JSON containing the information.
             *
             **/
            json::json GetAddressesByAccount(const json::json& params, bool fHelp);


            /** SendToAddress
             *
             *  sendtoaddress <Nexusaddress> <amount> [comment] [comment-to]
             *  - <amount> is a real and is rounded to the nearest 0.000001
             *  requires wallet passphrase to be set with walletpassphrase first.
             *
             *  @param[in] params Parameters array passed by the caller.
             *
             *  @return JSON containing the information.
             *
             **/
            json::json SendToAddress(const json::json& params, bool fHelp);


            /** SignMessage
             *
             *  signmessage <Nexusaddress> <message>
             *  Sign a message with the private key of an address
             *
             *  @param[in] params Parameters array passed by the caller.
             *
             *  @return JSON containing the information.
             *
             **/
            json::json SignMessage(const json::json& params, bool fHelp);


            /** VerifyMessage
            *
            *  verifymessage <Nexusaddress> <signature> <message>
            *  Verify a signed message
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json VerifyMessage(const json::json& params, bool fHelp);


            /** GetReceivedByAddress
            *
            *  getreceivedbyaddress <Nexusaddress> [minconf=1]
            *  Returns the total amount received by <Nexusaddress> in transactions with at least [minconf] confirmations
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetReceivedByAddress(const json::json& params, bool fHelp);


            /** GetReceivedByAccount
            *
            *  getreceivedbyaccount <account> [minconf=1]
            *  Sets the account associated with the given address
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetReceivedByAccount(const json::json& params, bool fHelp);


            /** GetBalance
            *
            *  getbalance [account] [minconf=1]
            *  If [account] is not specified, returns the server's total available balance
            *  If [account] is specified, returns the balance in the account
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetBalance(const json::json& params, bool fHelp);


            /** MoveCmd
            *
            *  move <fromaccount> <toaccount> <amount> [minconf=1] [comment]
            *  Move from one account in your wallet to another
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json MoveCmd(const json::json& params, bool fHelp);


            /** SendFrom
            *
            *  sendfrom <fromaccount> <toNexusaddress> <amount> [minconf=1] [comment] [comment-to]
            *  <amount> is a real and is rounded to the nearest 0.000001
            *  requires wallet passphrase to be set with walletpassphrase firsther
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json SendFrom(const json::json& params, bool fHelp);


            /** SendMany
            *
            *  sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]
            *  - amounts are double-precision floating point numbers
            *  requires wallet passphrase to be set with walletpassphrase first
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json SendMany(const json::json& params, bool fHelp);


            /** AddMultisigAddress
            *
            *  addmultisigaddress <nrequired> <'[\"key\",\"key\"]'> [account]
            *  Add a nrequired-to-sign multisignature address to the wallet
            *  each key is a nexus address or hex-encoded public key.
            *  If [account] is specified, assign address to [account].
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json AddMultisigAddress(const json::json& params, bool fHelp);


            /** ListReceivedByAddress
            *
            *  listreceivedbyaddress [minconf=1] [includeempty=false]
            *  [minconf] is the minimum number of confirmations before payments are included.
            *  [includeempty] whether to include addresses that haven't received any payments
            *  Returns an array of objects containing:
            *  \"address\" : receiving address
            *  \"account\" : the account of the receiving address
            *  \"amount\" : total amount received by the address
            *  \"confirmations\" : number of confirmations of the most recent transaction included
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ListReceivedByAddress(const json::json& params, bool fHelp);


            /** ListReceivedByAccount
            *
            *  listreceivedbyaccount [minconf=1] [includeempty=false]
            *  [minconf] is the minimum number of confirmations before payments are included.
            *  [includeempty] whether to include accounts that haven't received any payments
            *  Returns an array of objects containing:
            *  \"address\" : receiving address
            *  \"account\" : the account of the receiving address
            *  \"amount\" : total amount received by the address
            *  \"confirmations\" : number of confirmations of the most recent transaction incl
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ListReceivedByAccount(const json::json& params, bool fHelp);


            /** ListTransactions
            *
            *  listtransactions [account] [count=10] [from=0]
            *  Returns up to [count] most recent transactions skipping the first [from] transactions for account [account]
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ListTransactions(const json::json& params, bool fHelp);


            /** ListAddresses
            *
            *  listaddresses [max=100]
            *  Returns list of addresses
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ListAddresses(const json::json& params, bool fHelp);


            /** ListAccounts
            *
            *  listaccounts
            *  Returns Object that has account names as keys, account balances as values
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ListAccounts(const json::json& params, bool fHelp);


            /** ListSinceBlock
            *
            *  listsinceblock [blockhash] [target-confirmations]
            *  Get all transactions in blocks since block [blockhash], or all transactions if omitted
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ListSinceBlock(const json::json& params, bool fHelp);


            /** GetTransaction
            *
            *  gettransaction <txid>
            *  Get detailed information about <txid>
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetTransaction(const json::json& params, bool fHelp);


            /** GetRawTransaction
            *
            *  getrawtransaction <txid>
            *  Returns a string that is serialized, hex-encoded data for <txid>
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetRawTransaction(const json::json& params, bool fHelp);


            /** SendRawTransaction
            *
            *  sendrawtransaction <hex string> [checkinputs=0]
            *  Submits raw transaction (serialized, hex-encoded) to local node and network.
            *  If checkinputs is non-zero, checks the validity of the inputs of the transaction before sending it
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json SendRawTransaction(const json::json& params, bool fHelp);


            /** ValidateAddress
            *
            *  validateaddress <Nexusaddress>
            *  Return information about <Nexusaddress>
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ValidateAddress(const json::json& params, bool fHelp);


            /** MakeKeyPair
            *
            *  makekeypair [prefix]
            *  Make a public/private key pair. [prefix] is optional preferred prefix for the public key
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json MakeKeyPair(const json::json& params, bool fHelp);


            /** UnspentBalance
            *
            *  unspentbalance [\"address\",...]
            *  Returns the total amount of unspent Nexus for given address. This is a more accurate command than Get Balance
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json UnspentBalance(const json::json& params, bool fHelp);


            /** ListUnspent
            *
            *  listunspent [minconf=1] [maxconf=9999999]  [\"address\",...]
            *  Returns array of unspent transaction outputs with between minconf and maxconf (inclusive) confirmations.
            *  Optionally filtered to only include txouts paid to specified addresses.
            *  Results are an array of Objects, each of which has:
            *  {txid, vout, scriptPubKey, amount, confirmations}
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ListUnspent(const json::json& params, bool fHelp);


            /** Reset
            *
            *  reset
            *  Restart all node connections
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json Reset(const json::json& params, bool fHelp);


            /** GetPeerInfo
            *
            *  getpeerinfo
            *  Returns data about each connected network node
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetPeerInfo(const json::json& params, bool fHelp);


            /** GetNetworkHashps
            *
            *  getnetworkhashps
            *  Get network hashrate for the hashing channel
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetNetworkHashps(const json::json& params, bool fHelp);


            /** GetNetworkPps
            *
            *  getnetworkpps
            *  Get network prime searched per second
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetNetworkPps(const json::json& params, bool fHelp);


            /** GetNetworkTrustKeys
            *
            *  getnetworktrustkeys
            *  List all the Trust Keys on the Network
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetNetworkTrustKeys(const json::json& params, bool fHelp);


            /** GetBlockCount
            *
            *  getblockcount
            *  Returns the number of blocks in the longest block chain
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetBlockCount(const json::json& params, bool fHelp);


            /** GetBlockNumber
            *
            *  getblocknumber
            *  Deprecated.  Use getblockcount
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetBlockNumber(const json::json& params, bool fHelp);


            /** GetDifficulty
            *
            *  getdifficulty
            *  Returns difficulty as a multiple of the minimum difficulty
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetDifficulty(const json::json& params, bool fHelp);


            /** GetSupplyRates
            *
            *  getsupplyrates
            *  Returns an object containing current Nexus production rates in set time intervals.
            *  Time Frequency is in base 13 month, 28 day totalling 364 days.
            *  This is to prevent error from Gregorian Figures
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetSupplyRates(const json::json& params, bool fHelp);


            /** GetMoneySupply
            *
            *  getmoneysupply <timestamp>
            *  Returns the total supply of Nexus produced by miners, holdings, developers, and ambassadors.
            *  Default timestamp is the current Unified timestamp. The timestamp is recorded as a UNIX timestamp
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetMoneySupply(const json::json& params, bool fHelp);


            /** GetBlockHash
            *
            *  getblockhash <index>"
            *  Returns hash of block in best-block-chain at <index>
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetBlockHash(const json::json& params, bool fHelp);


            /** IsOrphan
            *
            *  isorphan <hash>"
            *  Returns whether a block is an orphan or not
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json IsOrphan(const json::json& params, bool fHelp);


            /** GetBlock
            *
            *  getblock <hash> [txinfo]"
            *  txinfo optional to print more detailed tx info."
            *  Returns details of a block with given block-hash
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json GetBlock(const json::json& params, bool fHelp);


            /** BackupWallet
            *
            *  backupwallet <destination>
            *  Safely copies wallet.dat to destination, which can be a directory or a path with filename
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json BackupWallet(const json::json& params, bool fHelp);


            /** KeypoolRefill
            *
            *  keypoolrefill
            *  Fills the keypool, requires wallet passphrase to be set
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json KeypoolRefill(const json::json& params, bool fHelp);


            /** SetTxFee
            *
            *  Sets the default transaction fee to be used
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json SetTxFee(const json::json& params, bool fHelp);


            /** WalletPassphrase
            *
            *  walletpassphrase <passphrase> [timeout] [mintonly]
            *  Stores the wallet decryption key in memory for <timeout> seconds.
            *  mintonly is optional true/false allowing only block minting
            *  timeout is ignored if mintonly is true
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json WalletPassphrase(const json::json& params, bool fHelp);


            /** EncryptWallet
            *
            *  encryptwallet <passphrase>
            *  Encrypts the wallet with <passphrase>
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json EncryptWallet(const json::json& params, bool fHelp);


            /** WalletPassphraseChange
            *
            *  walletpassphrasechange <oldpassphrase> <newpassphrase>
            *  Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json WalletPassphraseChange(const json::json& params, bool fHelp);


            /** WalletLock
            *
            *  walletlock
            *  Removes the wallet encryption key from memory, locking the wallet.
            *  After calling this method, you will need to call walletpassphrase again
            *  before being able to call any methods which require the wallet to be unlocked
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json WalletLock(const json::json& params, bool fHelp);


            /** CheckWallet
            *
            *  checkwallet
            *  Check wallet for integrity
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json CheckWallet(const json::json& params, bool fHelp);


            /** ListTrustKeys
            *
            *  listtrustkeys
            *  List all the Trust Keys this Node owns
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ListTrustKeys(const json::json& params, bool fHelp);


            /** RepairWallet
            *
            *  repairwallet
            *  Repair wallet if checkwallet reports any problem
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json RepairWallet(const json::json& params, bool fHelp);


            /** Rescan
            *
            *  rescan
            *  Rescans the database for relevant wallet transactions
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json Rescan(const json::json& params, bool fHelp);


            /** ImportPrivKey
            *
            *  importprivkey <PrivateKey> [label]
            *  Adds a private key (as returned by dumpprivkey) to your wallet
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ImportPrivKey(const json::json& params, bool fHelp);


            /** DumpPrivKey
            *
            *  dumpprivkey <NexusAddress>
            *  Reveals the private key corresponding to <NexusAddress>
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json DumpPrivKey(const json::json& params, bool fHelp);


            /** ImportKeys
            *
            *  importkeys
            *  Import List of Private Keys and return if they import properly or fail with their own code in the output sequence
            *  You need to list the imported keys in a JSON array of {[account],[privatekey]}
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ImportKeys(const json::json& params, bool fHelp);


            /** ExportKeys
            *
            *  exportkeys
            *  Export the private keys of the current UTXO values.
            *  This will allow the importing and exporting of private keys much easier
            *
            *  @param[in] params Parameters array passed by the caller.
            *
            *  @return JSON containing the information.
            *
            **/
            json::json ExportKeys(const json::json& params, bool fHelp);

        private:

            /** GetTotalConnectionCount
             *
             *  Returns the total number of connections for this node
             *
             **/
            uint32_t GetTotalConnectionCount();
        };

        /** The instance of RPC commands. */
        extern RPC RPCCommands;
    }
}

#endif
