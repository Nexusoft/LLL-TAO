/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#include <TAO/API/types/base.h>

/* Global TAO namespace. */
namespace Legacy
{
    /** RPC
     *
     *  RPC API Class.
     *  Manages the function pointers for all RPC commands.
     *
     **/
    class RPC : public TAO::API::Derived<RPC>
    {
    public:

        /** Default Constructor. **/
        RPC()
        : Derived<RPC>()
        {
            Initialize();
        }


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


        /** Name
         *
         *  Returns the name of this API.
         *
         **/
        static std::string Name()
        {
            return "legacy";
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
        static encoding::json SanitizeParams(const std::string& strMethod, const encoding::json& jsonParams);


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
        encoding::json Echo(const encoding::json& params, const bool fHelp);


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
        encoding::json Help( const encoding::json& params, const bool fHelp);


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
        encoding::json Stop(const encoding::json& params, const bool fHelp);


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
        encoding::json GetInfo(const encoding::json& params, const bool fHelp);


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
        encoding::json GetMiningInfo(const encoding::json& params, const bool fHelp);


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
        encoding::json GetConnectionCount(const encoding::json& params, const bool fHelp);


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
        encoding::json GetNewAddress(const encoding::json& params, const bool fHelp);


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
        encoding::json GetAccountAddress(const encoding::json& params, const bool fHelp);


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
        encoding::json SetAccount(const encoding::json& params, const bool fHelp);


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
        encoding::json GetAccount(const encoding::json& params, const bool fHelp);


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
        encoding::json GetAddressesByAccount(const encoding::json& params, const bool fHelp);


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
        encoding::json SendToAddress(const encoding::json& params, const bool fHelp);


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
        encoding::json SignMessage(const encoding::json& params, const bool fHelp);


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
        encoding::json VerifyMessage(const encoding::json& params, const bool fHelp);


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
        encoding::json GetReceivedByAddress(const encoding::json& params, const bool fHelp);


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
        encoding::json GetReceivedByAccount(const encoding::json& params, const bool fHelp);


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
        encoding::json GetBalance(const encoding::json& params, const bool fHelp);


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
        encoding::json MoveCmd(const encoding::json& params, const bool fHelp);


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
        encoding::json SendFrom(const encoding::json& params, const bool fHelp);


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
        encoding::json SendMany(const encoding::json& params, const bool fHelp);


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
        encoding::json AddMultisigAddress(const encoding::json& params, const bool fHelp);


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
        encoding::json ListReceivedByAddress(const encoding::json& params, const bool fHelp);


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
        encoding::json ListReceivedByAccount(const encoding::json& params, const bool fHelp);


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
        encoding::json ListTransactions(const encoding::json& params, const bool fHelp);


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
        encoding::json ListAddresses(const encoding::json& params, const bool fHelp);


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
        encoding::json ListAccounts(const encoding::json& params, const bool fHelp);


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
        encoding::json ListSinceBlock(const encoding::json& params, const bool fHelp);


        /** GetGlobalTransaction
        *
        *  getglobaltransaction <txid>
        *  Get detailed information about <txid>
        *
        *  @param[in] params Parameters array passed by the caller.
        *
        *  @return JSON containing the information.
        *
        **/
        encoding::json GetGlobalTransaction(const encoding::json& params, const bool fHelp);


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
        encoding::json GetTransaction(const encoding::json& params, const bool fHelp);


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
        encoding::json GetRawTransaction(const encoding::json& params, const bool fHelp);


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
        encoding::json SendRawTransaction(const encoding::json& params, const bool fHelp);


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
        encoding::json ValidateAddress(const encoding::json& params, const bool fHelp);


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
        encoding::json MakeKeyPair(const encoding::json& params, const bool fHelp);


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
        encoding::json UnspentBalance(const encoding::json& params, const bool fHelp);


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
        encoding::json ListUnspent(const encoding::json& params, const bool fHelp);


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
        encoding::json GetPeerInfo(const encoding::json& params, const bool fHelp);


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
        encoding::json GetNetworkHashps(const encoding::json& params, const bool fHelp);


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
        encoding::json GetNetworkPps(const encoding::json& params, const bool fHelp);


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
        encoding::json GetNetworkTrustKeys(const encoding::json& params, const bool fHelp);


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
        encoding::json GetBlockCount(const encoding::json& params, const bool fHelp);


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
        encoding::json GetBlockNumber(const encoding::json& params, const bool fHelp);


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
        encoding::json GetDifficulty(const encoding::json& params, const bool fHelp);


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
        encoding::json GetSupplyRates(const encoding::json& params, const bool fHelp);


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
        encoding::json GetMoneySupply(const encoding::json& params, const bool fHelp);


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
        encoding::json GetBlockHash(const encoding::json& params, const bool fHelp);


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
        encoding::json IsOrphan(const encoding::json& params, const bool fHelp);


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
        encoding::json GetBlock(const encoding::json& params, const bool fHelp);


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
        encoding::json BackupWallet(const encoding::json& params, const bool fHelp);


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
        encoding::json KeypoolRefill(const encoding::json& params, const bool fHelp);


        /** SetTxFee
        *
        *  Sets the default transaction fee to be used
        *
        *  @param[in] params Parameters array passed by the caller.
        *
        *  @return JSON containing the information.
        *
        **/
        encoding::json SetTxFee(const encoding::json& params, const bool fHelp);


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
        encoding::json WalletPassphrase(const encoding::json& params, const bool fHelp);


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
        encoding::json EncryptWallet(const encoding::json& params, const bool fHelp);


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
        encoding::json WalletPassphraseChange(const encoding::json& params, const bool fHelp);


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
        encoding::json WalletLock(const encoding::json& params, const bool fHelp);


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
        encoding::json CheckWallet(const encoding::json& params, const bool fHelp);


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
        encoding::json ListTrustKeys(const encoding::json& params, const bool fHelp);


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
        encoding::json RepairWallet(const encoding::json& params, const bool fHelp);


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
        encoding::json Rescan(const encoding::json& params, const bool fHelp);


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
        encoding::json ImportPrivKey(const encoding::json& params, const bool fHelp);


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
        encoding::json DumpPrivKey(const encoding::json& params, const bool fHelp);


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
        encoding::json ImportKeys(const encoding::json& params, const bool fHelp);


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
        encoding::json ExportKeys(const encoding::json& params, const bool fHelp);

    private:

        /** GetTotalConnectionCount
         *
         *  Returns the total number of connections for this node
         *
         **/
        uint32_t GetTotalConnectionCount();
    };

}
