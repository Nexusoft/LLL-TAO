/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>

namespace TAO::API
{

    /* getnewaddress [account]
       Returns a new Nexus address for receiving payments.
       If [account] is specified (recommended), it is added to the address book
       so payments received with the address will be credited to [account] */
    json::json RPC::GetNewAddress(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 1)
            return std::string(
                "getnewaddress [account]"
                " - Returns a new Nexus address for receiving payments.  "
                "If [account] is specified (recommended), it is added to the address book "
                "so payments received with the address will be credited to [account].");

    //     // Parse the account first so we don't generate a key if there's an error
    //     string strAccount = "default";
    //     if (jsonParams.size() > 0)
    //         strAccount = AccountFromValue(params[0]);

    //     if (!pwalletMain->IsLocked())
    //         pwalletMain->TopUpKeyPool();

    //     // Generate a new key that is added to wallet
    //     std::vector<unsigned char> newKey;
    //     if (!pwalletMain->GetKeyFromPool(newKey, false))
    //         throw JSONRPCError(-12, "Error: Keypool ran out, please call keypoolrefill first");
    //     Wallet::NexusAddress address(newKey);

    //     pwalletMain->SetAddressBookName(address, strAccount);

    //     return address.ToString();
     }


    // Wallet::NexusAddress GetAccountAddress(string strAccount, bool bForceNew=false)
    // {
    //     Wallet::CWalletDB walletdb(pwalletMain->strWalletFile);

    //     Wallet::CAccount account;
    //     walletdb.ReadAccount(strAccount, account);

    //     bool bKeyUsed = false;

    //     // Check if the current key has been used
    //     if (!account.vchPubKey.empty())
    //     {
    //         Wallet::CScript scriptPubKey;
    //         scriptPubKey.SetNexusAddress(account.vchPubKey);
    //         for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin();
    //              it != pwalletMain->mapWallet.end() && !account.vchPubKey.empty();
    //              ++it)
    //         {
    //             const Wallet::CWalletTx& wtx = (*it).second;
    //             BOOST_FOREACH(const Core::CTxOut& txout, wtx.vout)
    //                 if (txout.scriptPubKey == scriptPubKey)
    //                     bKeyUsed = true;
    //         }
    //     }

    //     // Generate a new key
    //     if (account.vchPubKey.empty() || bForceNew || bKeyUsed)
    //     {
    //         if (!pwalletMain->GetKeyFromPool(account.vchPubKey, false))
    //             throw JSONRPCError(-12, "Error: Keypool ran out, please call keypoolrefill first");

    //         pwalletMain->SetAddressBookName(Wallet::NexusAddress(account.vchPubKey), strAccount);
    //         walletdb.WriteAccount(strAccount, account);
    //     }

    //     return Wallet::NexusAddress(account.vchPubKey);
    // }

    /* getaccountaddress <account>
       Returns the current Nexus address for receiving payments to this account */
    json::json RPC::GetAccountAddress(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 1)
            return std::string(
                "getaccountaddress <account>"
                " - Returns the current Nexus address for receiving payments to this account.");

    //     // Parse the account first so we don't generate a key if there's an error
    //     string strAccount = AccountFromValue(params[0]);

    //     json::json ret;
    //     ret = GetAccountAddress(strAccount).ToString();

    //     return ret;
    }


    /* setaccount <Nexusaddress> <account>
       Sets the account associated with the given address */
    json::json RPC::SetAccount(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() < 1 || jsonParams.size() > 2)
            return std::string(
                "setaccount <Nexusaddress> <account>"
                " - Sets the account associated with the given address.");

    //     Wallet::NexusAddress address(params[0].get_str());
    //     if (!address.IsValid())
    //         throw JSONRPCError(-5, "Invalid Nexus address");


    //     string strAccount;
    //     if (jsonParams.size() > 1)
    //         strAccount = AccountFromValue(params[1]);

    //     // Detect when changing the account of an address that is the 'unused current key' of another account:
    //     if (pwalletMain->mapAddressBook.count(address))
    //     {
    //         string strOldAccount = pwalletMain->mapAddressBook[address];
    //         if (address == GetAccountAddress(strOldAccount))
    //             GetAccountAddress(strOldAccount, true);
    //     }

    //     pwalletMain->SetAddressBookName(address, strAccount);

    //     return Value::null;
     }

    /* getaccount <Nexusaddress>
       Returns the account associated with the given address */
    json::json RPC::GetAccount(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 1)
            return std::string(
                "getaccount <Nexusaddress>"
                " - Returns the account associated with the given address.");

    //     Wallet::NexusAddress address(params[0].get_str());
    //     if (!address.IsValid())
    //         throw JSONRPCError(-5, "Invalid Nexus address");

    //     string strAccount;
    //     map<Wallet::NexusAddress, string>::iterator mi = pwalletMain->mapAddressBook.find(address);
    //     if (mi != pwalletMain->mapAddressBook.end() && !(*mi).second.empty())
    //         strAccount = (*mi).second;
    //     return strAccount;
    }

    /* getaddressesbyaccount <account>
       Returns the list of addresses for the given account */
    json::json RPC::GetAddressesByAccount(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 1)
            return std::string(
                "getaddressesbyaccount <account>"
                " - Returns the list of addresses for the given account.");

    //     string strAccount = AccountFromValue(params[0]);

    //     // Find all addresses that have the given account
    //     Array ret;
    //     BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, string)& item, pwalletMain->mapAddressBook)
    //     {
    //         const Wallet::NexusAddress& address = item.first;
    //         const string& strName = item.second;
    //         if (strName == strAccount || (strName == "" && strAccount == "default"))
    //             ret.push_back(address.ToString());
    //     }
    //     return ret;
    }

    // json::json RPC::SetTxFee(const json::json& jsonParams, bool fHelp)
    // {
    //     if (fHelp || jsonParams.size() < 1 || jsonParams.size() > 1 || AmountFromValue(params[0]) < Core::MIN_TX_FEE)
    //         return std::string(
    //             "settxfee <amount>"
    //             " - <amount> is a real and is rounded to 0.01 (cent)"
    //             " Minimum and default transaction fee per KB is 1 cent");

    //     Core::nTransactionFee = AmountFromValue(params[0]);
    //     Core::nTransactionFee = (Core::nTransactionFee / CENT) * CENT;  // round to cent
    //     return true;
    //}

    /* sendtoaddress <Nexusaddress> <amount> [comment] [comment-to]
    *  - <amount> is a real and is rounded to the nearest 0.000001
    *  requires wallet passphrase to be set with walletpassphrase first */
    // json::json sendtoaddress(const json::json& jsonParams, bool fHelp)
    // {
    //     if (pwalletMain->IsCrypted() && (fHelp || jsonParams.size() < 2 || jsonParams.size() > 4))
    //         return std::string(
    //             "sendtoaddress <Nexusaddress> <amount> [comment] [comment-to]"
    //             " - <amount> is a real and is rounded to the nearest 0.000001"
    //             " requires wallet passphrase to be set with walletpassphrase first");
    //     if (!pwalletMain->IsCrypted() && (fHelp || jsonParams.size() < 2 || jsonParams.size() > 4))
    //         return std::string(
    //             "sendtoaddress <Nexusaddress> <amount> [comment] [comment-to]"
    //             "<amount> is a real and is rounded to the nearest 0.000001");

    //     Wallet::NexusAddress address(params[0].get_str());
    //     if (!address.IsValid())
    //         throw JSONRPCError(-5, "Invalid Nexus address");

    //     // Amount
    //     int64 nAmount = AmountFromValue(params[1]);
    //     if (nAmount < Core::MIN_TXOUT_AMOUNT)
    //         throw JSONRPCError(-101, "Send amount too small");

    //     // Wallet comments
    //     Wallet::CWalletTx wtx;
    //     if (jsonParams.size() > 2 && params[2].type() != null_type && !params[2].get_str().empty())
    //         wtx.mapValue["comment"] = params[2].get_str();
    //     if (jsonParams.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
    //         wtx.mapValue["to"]      = params[3].get_str();

    //     if (pwalletMain->IsLocked())
    //         throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    //     string strError = pwalletMain->SendToNexusAddress(address, nAmount, wtx);
    //     if (strError != "")
    //         throw JSONRPCError(-4, strError);

    //     return wtx.GetHash().GetHex();
    // }

    /* signmessage <Nexusaddress> <message>
       Sign a message with the private key of an address */
    json::json RPC::SignMessage(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 2)
            return std::string(
                "signmessage <Nexusaddress> <message>"
                " - Sign a message with the private key of an address");

    //     if (pwalletMain->IsLocked())
    //         throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    //     string strAddress = params[0].get_str();
    //     string strMessage = params[1].get_str();

    //     Wallet::NexusAddress addr(strAddress);
    //     if (!addr.IsValid())
    //         throw JSONRPCError(-3, "Invalid address");

    //     Wallet::CKey key;
    //     if (!pwalletMain->GetKey(addr, key))
    //         throw JSONRPCError(-4, "Private key not available");

    //     DataStream ss(SER_GETHASH, 0);
    //     ss << Core::strMessageMagic;
    //     ss << strMessage;

    //     vector<unsigned char> vchSig;
    //     if (!key.SignCompact(SK256(ss.begin(), ss.end()), vchSig))
    //         throw JSONRPCError(-5, "Sign failed");

    //     return EncodeBase64(&vchSig[0], vchSig.size());
    }

    /* verifymessage <Nexusaddress> <signature> <message>
       Verify a signed message */
    json::json RPC::VerifyMessage(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 3)
            return std::string(
                "verifymessage <Nexusaddress> <signature> <message>"
                " - Verify a signed message");

    //     string strAddress  = params[0].get_str();
    //     string strSign     = params[1].get_str();
    //     string strMessage  = params[2].get_str();

    //     Wallet::NexusAddress addr(strAddress);
    //     if (!addr.IsValid())
    //         throw JSONRPCError(-3, "Invalid address");

    //     bool fInvalid = false;
    //     vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    //     if (fInvalid)
    //         throw JSONRPCError(-5, "Malformed base64 encoding");

    //     DataStream ss(SER_GETHASH, 0);
    //     ss << Core::strMessageMagic;
    //     ss << strMessage;

    //     Wallet::CKey key;
    //     if (!key.SetCompactSignature(SK256(ss.begin(), ss.end()), vchSig))
    //         return false;

    //     return (Wallet::NexusAddress(key.GetPubKey()) == addr);
    }

    /* getreceivedbyaddress <Nexusaddress> [minconf=1]
       Returns the total amount received by <Nexusaddress> in transactions with at least [minconf] confirmations */
    json::json RPC::GetReceivedByAddress(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() < 1 || jsonParams.size() > 2)
            return std::string(
                "getreceivedbyaddress <Nexusaddress> [minconf=1]"
                " - Returns the total amount received by <Nexusaddress> in transactions with at least [minconf] confirmations.");

    //     // Nexus address
    //     Wallet::NexusAddress address = Wallet::NexusAddress(params[0].get_str());
    //     Wallet::CScript scriptPubKey;
    //     if (!address.IsValid())
    //         throw JSONRPCError(-5, "Invalid Nexus address");
    //     scriptPubKey.SetNexusAddress(address);
    //     if (!IsMine(*pwalletMain,scriptPubKey))
    //         return (double)0.0;

    //     // Minimum confirmations
    //     int nMinDepth = 1;
    //     if (jsonParams.size() > 1)
    //         nMinDepth = params[1].get_int();

    //     // Tally
    //     int64 nAmount = 0;
    //     for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    //     {
    //         const Wallet::CWalletTx& wtx = (*it).second;
    //         if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
    //             continue;

    //         BOOST_FOREACH(const Core::CTxOut& txout, wtx.vout)
    //             if (txout.scriptPubKey == scriptPubKey)
    //                 if (wtx.GetDepthInMainChain() >= nMinDepth)
    //                     nAmount += txout.nValue;
    //    }

    //     return  ValueFromAmount(nAmount);
    }


    // void GetAccountAddresses(string strAccount, set<Wallet::NexusAddress>& setAddress)
    // {
    //     BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, string)& item, pwalletMain->mapAddressBook)
    //     {
    //         const Wallet::NexusAddress& address = item.first;
    //         const string& strName = item.second;
    //         if (strName == strAccount)
    //             setAddress.insert(address);
    //     }
    // }

    /* getreceivedbyaccount <account> [minconf=1]
       Returns the total amount received by addresses with <account> in transactions with at least [minconf] confirmations */
    json::json RPC::GetReceivedByAccount(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() < 1 || jsonParams.size() > 2)
            return std::string(
                "getreceivedbyaccount <account> [minconf=1]"
                " - Returns the total amount received by addresses with <account> in transactions with at least [minconf] confirmations.");

    //     // Minimum confirmations
    //     int nMinDepth = 1;
    //     if (jsonParams.size() > 1)
    //         nMinDepth = params[1].get_int();

    //     // Get the set of pub keys assigned to account
    //     string strAccount = AccountFromValue(params[0]);
    //     set<Wallet::NexusAddress> setAddress;
    //     GetAccountAddresses(strAccount, setAddress);

    //     // Tally
    //     int64 nAmount = 0;
    //     for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    //     {
    //         const Wallet::CWalletTx& wtx = (*it).second;
    //         if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
    //             continue;

    //         BOOST_FOREACH(const Core::CTxOut& txout, wtx.vout)
    //         {
    //             Wallet::NexusAddress address;
    //             if (ExtractAddress(txout.scriptPubKey, address) && pwalletMain->HaveKey(address) && setAddress.count(address))
    //                 if (wtx.GetDepthInMainChain() >= nMinDepth)
    //                     nAmount += txout.nValue;
    //         }
        }

    //     return (double)nAmount / (double)COIN;
    // }

    // int64 GetAccountBalance(Wallet::CWalletDB& walletdb, const string& strAccount, int nMinDepth)
    // {
    //     int64 nBalance = 0;

    //     // Tally wallet transactions
    //     for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    //     {
    //         const Wallet::CWalletTx& wtx = (*it).second;
    //         if (!wtx.IsFinal())
    //             continue;

    //         int64 nGenerated, nReceived, nSent, nFee;
    //         wtx.GetAccountAmounts(strAccount, nGenerated, nReceived, nSent, nFee);

    //         if (nReceived != 0 && wtx.GetDepthInMainChain() >= nMinDepth)
    //             nBalance += nReceived;
    //         nBalance += nGenerated - nSent - nFee;
    //     }

    //     // Tally internal accounting entries
    //     nBalance += walletdb.GetAccountCreditDebit(strAccount);

    //     return nBalance;
    // }


    // int64 GetAccountBalance(const string& strAccount, int nMinDepth)
    // {
    //     Wallet::CWalletDB walletdb(pwalletMain->strWalletFile);
    //     return GetAccountBalance(walletdb, strAccount, nMinDepth);
    // }

    /* getbalance [account] [minconf=1]
    *  If [account] is not specified, returns the server's total available balance.
    *  If [account] is specified, returns the balance in the account */
    json::json RPC::GetBalance(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 2)
            return std::string(
                "getbalance [account] [minconf=1]"
                " - If [account] is not specified, returns the server's total available balance."
                " If [account] is specified, returns the balance in the account.");

    //     if (jsonParams.size() == 0)
    //         return  ValueFromAmount(pwalletMain->GetBalance());

    //     int nMinDepth = 1;
    //     if (jsonParams.size() > 1)
    //         nMinDepth = params[1].get_int();

    //     if (params[0].get_str() == "*") {
    //         // Calculate total balance a different way from GetBalance()
    //         // (GetBalance() sums up all unspent TxOuts)
    //         // getbalance and getbalance '*' should always return the same number.
    //         int64 nBalance = 0;
    //         for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    //         {
    //             const Wallet::CWalletTx& wtx = (*it).second;
    //             if (!wtx.IsFinal())
    //                 continue;

    //             int64 allGeneratedImmature, allGeneratedMature, allFee;
    //             allGeneratedImmature = allGeneratedMature = allFee = 0;
    //             string strSentAccount;
    //             list<pair<Wallet::NexusAddress, int64> > listReceived;
    //             list<pair<Wallet::NexusAddress, int64> > listSent;
    //             wtx.GetAmounts(allGeneratedImmature, allGeneratedMature, listReceived, listSent, allFee, strSentAccount);
    //             if (wtx.GetDepthInMainChain() >= nMinDepth)
    //             {
    //                 BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress,int64)& r, listReceived)
    //                     nBalance += r.second;
    //             }
    //             BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress,int64)& r, listSent)
    //                 nBalance -= r.second;
    //             nBalance -= allFee;
    //             nBalance += allGeneratedMature;
    //         }
    //         return  ValueFromAmount(nBalance);
    //     }

    //     string strAccount = AccountFromValue(params[0]);

    //     int64 nBalance = GetAccountBalance(strAccount, nMinDepth);

    //     return ValueFromAmount(nBalance);
    }

    /* move <fromaccount> <toaccount> <amount> [minconf=1] [comment]
       Move from one account in your wallet to another */
    json::json RPC::MoveCmd(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() < 3 || jsonParams.size() > 5)
            return std::string(
                "move <fromaccount> <toaccount> <amount> [minconf=1] [comment]"
                " - Move from one account in your wallet to another.");

    //     string strFrom = AccountFromValue(params[0]);
    //     string strTo = AccountFromValue(params[1]);
    //     int64 nAmount = AmountFromValue(params[2]);
    //     if (jsonParams.size() > 3)
    //         // unused parameter, used to be nMinDepth, keep type-checking it though
    //         (void)params[3].get_int();
    //     string strComment;
    //     if (jsonParams.size() > 4)
    //         strComment = params[4].get_str();

    //     Wallet::CWalletDB walletdb(pwalletMain->strWalletFile);
    //     if (!walletdb.TxnBegin())
    //         throw JSONRPCError(-20, "database error");

    //     int64 nNow = GetUnifiedTimestamp();

    //     // Debit
    //     Wallet::CAccountingEntry debit;
    //     debit.strAccount = strFrom;
    //     debit.nCreditDebit = -nAmount;
    //     debit.nTime = nNow;
    //     debit.strOtherAccount = strTo;
    //     debit.strComment = strComment;
    //     walletdb.WriteAccountingEntry(debit);

    //     // Credit
    //     Wallet::CAccountingEntry credit;
    //     credit.strAccount = strTo;
    //     credit.nCreditDebit = nAmount;
    //     credit.nTime = nNow;
    //     credit.strOtherAccount = strFrom;
    //     credit.strComment = strComment;
    //     walletdb.WriteAccountingEntry(credit);

    //     if (!walletdb.TxnCommit())
    //         throw JSONRPCError(-20, "database error");

    //     return true;
    }


    // json::json sendfrom(const json::json& jsonParams, bool fHelp)
    // {
    //     if (pwalletMain->IsCrypted() && (fHelp || jsonParams.size() < 3 || jsonParams.size() > 6))
    //         return std::string(
    //             "sendfrom <fromaccount> <toNexusaddress> <amount> [minconf=1] [comment] [comment-to]"
    //             " - <amount> is a real and is rounded to the nearest 0.000001"
    //             " requires wallet passphrase to be set with walletpassphrase first");
    //     if (!pwalletMain->IsCrypted() && (fHelp || jsonParams.size() < 3 || jsonParams.size() > 6))
    //         return std::string(
    //             "sendfrom <fromaccount> <toNexusaddress> <amount> [minconf=1] [comment] [comment-to]"
    //             "<amount> is a real and is rounded to the nearest 0.000001");

    //     string strAccount = AccountFromValue(params[0]);
    //     Wallet::NexusAddress address(params[1].get_str());
    //     if (!address.IsValid())
    //         throw JSONRPCError(-5, "Invalid Nexus address");
    //     int64 nAmount = AmountFromValue(params[2]);
    //     if (nAmount < Core::MIN_TXOUT_AMOUNT)
    //         throw JSONRPCError(-101, "Send amount too small");
    //     int nMinDepth = 1;
    //     if (jsonParams.size() > 3)
    //         nMinDepth = params[3].get_int();

    //     Wallet::CWalletTx wtx;
    //     wtx.strFromAccount = strAccount;
    //     if (jsonParams.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
    //         wtx.mapValue["comment"] = params[4].get_str();
    //     if (jsonParams.size() > 5 && params[5].type() != null_type && !params[5].get_str().empty())
    //         wtx.mapValue["to"]      = params[5].get_str();

    //     if (pwalletMain->IsLocked())
    //         throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    //     // Check funds
    //     int64 nBalance = GetAccountBalance(strAccount, nMinDepth);
    //     if (nAmount > nBalance)
    //         throw JSONRPCError(-6, "Account has insufficient funds");

    //     // Send
    //     string strError = pwalletMain->SendToNexusAddress(address, nAmount, wtx);
    //     if (strError != "")
    //         throw JSONRPCError(-4, strError);

    //     return wtx.GetHash().GetHex();
    // }


    // json::json sendmany(const json::json& jsonParams, bool fHelp)
    // {
    //     if (pwalletMain->IsCrypted() && (fHelp || jsonParams.size() < 2 || jsonParams.size() > 4))
    //         return std::string(
    //             "sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]"
    //             " - amounts are double-precision floating point numbers"
    //             " requires wallet passphrase to be set with walletpassphrase first");
    //     if (!pwalletMain->IsCrypted() && (fHelp || jsonParams.size() < 2 || jsonParams.size() > 4))
    //         return std::string(
    //             "sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]"
    //             "amounts are double-precision floating point numbers");

    //     string strAccount = AccountFromValue(params[0]);
    //     Object sendTo = params[1].get_obj();
    //     int nMinDepth = 1;
    //     if (jsonParams.size() > 2)
    //         nMinDepth = params[2].get_int();

    //     Wallet::CWalletTx wtx;
    //     wtx.strFromAccount = strAccount;
    //     if (jsonParams.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
    //         wtx.mapValue["comment"] = params[3].get_str();

    //     set<Wallet::NexusAddress> setAddress;
    //     vector<pair<Wallet::CScript, int64> > vecSend;

    //     int64 totalAmount = 0;
    //     BOOST_FOREACH(const Pair& s, sendTo)
    //     {
    //         Wallet::NexusAddress address(s.name_);
    //         if (!address.IsValid())
    //             throw JSONRPCError(-5, string("Invalid Nexus address:")+s.name_);

    //         if (setAddress.count(address))
    //             throw JSONRPCError(-8, string("Invalid parameter, duplicated address: ")+s.name_);
    //         setAddress.insert(address);

    //         Wallet::CScript scriptPubKey;
    //         scriptPubKey.SetNexusAddress(address);
    //         int64 nAmount = AmountFromValue(s.value_);
    //         if (nAmount < Core::MIN_TXOUT_AMOUNT)
    //             throw JSONRPCError(-101, "Send amount too small");
    //         totalAmount += nAmount;

    //         vecSend.push_back(make_pair(scriptPubKey, nAmount));
    //     }

    //     if (pwalletMain->IsLocked())
    //         throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    //     if (Wallet::fWalletUnlockMintOnly)
    //         throw JSONRPCError(-13, "Error: Wallet unlocked for block minting only.");

    //     // Check funds
    //     int64 nBalance = GetAccountBalance(strAccount, nMinDepth);
    //     if (totalAmount > nBalance)
    //         throw JSONRPCError(-6, "Account has insufficient funds");

    //     // Send
    //     Wallet::CReserveKey keyChange(pwalletMain);
    //     int64 nFeeRequired = 0;
    //     bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired);
    //     if (!fCreated)
    //     {
    //         if (totalAmount + nFeeRequired > pwalletMain->GetBalance())
    //             throw JSONRPCError(-6, "Insufficient funds");
    //         throw JSONRPCError(-4, "Transaction creation failed");
    //     }
    //     if (!pwalletMain->CommitTransaction(wtx, keyChange))
    //         throw JSONRPCError(-4, "Transaction commit failed");

    //     return wtx.GetHash().GetHex();
    // }

    /* addmultisigaddress <nrequired> <'[\"key\",\"key\"]'> [account]
    *  Add a nrequired-to-sign multisignature address to the wallet
    *  each key is a nexus address or hex-encoded public key.
    *  If [account] is specified, assign address to [account]. */
    json::json RPC::AddMultisigAddress(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() < 2 || jsonParams.size() > 3)
        {
            std::string msg = "addmultisigaddress <nrequired> <'[\"key\",\"key\"]'> [account]"
                " - Add a nrequired-to-sign multisignature address to the wallet"
                " each key is a nexus address or hex-encoded public key."
                " If [account] is specified, assign address to [account].";
            return std::string(msg);
        }

    //     int nRequired = params[0].get_int();
    //     const Array& keys = params[1].get_array();
    //     string strAccount;
    //     if (jsonParams.size() > 2)
    //         strAccount = AccountFromValue(params[2]);

    //     // Gather public keys
    //     if (nRequired < 1)
    //         return std::string("a multisignature address must require at least one key to redeem");
    //     if ((int)keys.size() < nRequired)
    //         return std::string(
    //             strprintf("not enough keys supplied "
    //                       "(got %d keys, but need at least %d to redeem)", keys.size(), nRequired));
    //     std::vector<Wallet::CKey> pubkeys;
    //     pubkeys.resize(keys.size());
    //     for (unsigned int i = 0; i < keys.size(); i++)
    //     {
    //         const std::string& ks = keys[i].get_str();

    //         // Case 1: nexus address and we have full public key:
    //         Wallet::NexusAddress address(ks);
    //         if (address.IsValid())
    //         {
    //             if (address.IsScript())
    //                 return std::string(
    //                     strprintf("%s is a pay-to-script address",ks.c_str()));
    //             std::vector<unsigned char> vchPubKey;
    //             if (!pwalletMain->GetPubKey(address, vchPubKey))
    //                 return std::string(
    //                     strprintf("no full public key for address %s",ks.c_str()));
    //             if (vchPubKey.empty() || !pubkeys[i].SetPubKey(vchPubKey))
    //                 return std::string(" Invalid public key: "+ks);
    //         }

    //         // Case 2: hex public key
    //         else if (IsHex(ks))
    //         {
    //             vector<unsigned char> vchPubKey = ParseHex(ks);
    //             if (vchPubKey.empty() || !pubkeys[i].SetPubKey(vchPubKey))
    //                 return std::string(" Invalid public key: "+ks);
    //         }
    //         else
    //         {
    //             return std::string(" Invalid public key: "+ks);
    //         }
    //    }

    //     // Construct using pay-to-script-hash:
    //     Wallet::CScript inner;
    //     inner.SetMultisig(nRequired, pubkeys);

    //     uint256 scriptHash = SK256(inner);
    //     Wallet::CScript scriptPubKey;
    //     scriptPubKey.SetPayToScriptHash(inner);
    //     pwalletMain->AddCScript(inner);
    //     Wallet::NexusAddress address;
    //     address.SetScriptHash256(scriptHash);

    //     pwalletMain->SetAddressBookName(address, strAccount);
    //     return address.ToString();
    }


    // struct tallyitem
    // {
    //     int64 nAmount;
    //     int nConf;
    //     tallyitem()
    //     {
    //         nAmount = 0;
    //         nConf = std::numeric_limits<int>::max();
    //     }
    // };

    // json::json ListReceived(const json::json& jsonParams, bool fByAccounts)
    // {
    //     // Minimum confirmations
    //     int nMinDepth = 1;
    //     if (jsonParams.size() > 0)
    //         nMinDepth = params[0].get_int();

    //     // Whether to include empty accounts
    //     bool fIncludeEmpty = false;
    //     if (jsonParams.size() > 1)
    //         fIncludeEmpty = params[1].get_bool();

    //     // Tally
    //     map<Wallet::NexusAddress, tallyitem> mapTally;
    //     for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    //     {
    //         const Wallet::CWalletTx& wtx = (*it).second;

    //         if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
    //             continue;

    //         int nDepth = wtx.GetDepthInMainChain();
    //         if (nDepth < nMinDepth)
    //             continue;

    //         BOOST_FOREACH(const Core::CTxOut& txout, wtx.vout)
    //         {
    //             Wallet::NexusAddress address;
    //             if (!ExtractAddress(txout.scriptPubKey, address) || !pwalletMain->HaveKey(address) || !address.IsValid())
    //                 continue;

    //             tallyitem& item = mapTally[address];
    //             item.nAmount += txout.nValue;
    //             item.nConf = min(item.nConf, nDepth);
    //         }
    //     }

    //     // Reply
    //     Array ret;
    //     map<string, tallyitem> mapAccountTally;
    //     BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, string)& item, pwalletMain->mapAddressBook)
    //     {
    //         const Wallet::NexusAddress& address = item.first;
    //         const string& strAccount = item.second;
    //         map<Wallet::NexusAddress, tallyitem>::iterator it = mapTally.find(address);
    //         if (it == mapTally.end() && !fIncludeEmpty)
    //             continue;

    //         int64 nAmount = 0;
    //         int nConf = std::numeric_limits<int>::max();
    //         if (it != mapTally.end())
    //         {
    //             nAmount = (*it).second.nAmount;
    //             nConf = (*it).second.nConf;
    //         }

    //         if (fByAccounts)
    //         {
    //             tallyitem& item = mapAccountTally[strAccount];
    //             item.nAmount += nAmount;
    //             item.nConf = min(item.nConf, nConf);
    //         }
    //         else
    //         {
    //             Object obj;
    //             obj.push_back(Pair("address",       address.ToString()));
    //             obj.push_back(Pair("account",       strAccount));
    //             obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
    //             obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
    //             ret.push_back(obj);
    //         }
    //     }

    //     if (fByAccounts)
    //     {
    //         for (map<string, tallyitem>::iterator it = mapAccountTally.begin(); it != mapAccountTally.end(); ++it)
    //         {
    //             int64 nAmount = (*it).second.nAmount;
    //             int nConf = (*it).second.nConf;
    //             Object obj;
    //             obj.push_back(Pair("account",       (*it).first));
    //             obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
    //             obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
    //             ret.push_back(obj);
    //         }
    //     }

    //     return ret;
    // }

    /* listreceivedbyaddress [minconf=1] [includeempty=false]
    *  [minconf] is the minimum number of confirmations before payments are included.
    *  [includeempty] whether to include addresses that haven't received any payments
    *  Returns an array of objects containing:
    *  \"address\" : receiving address
    *  \"account\" : the account of the receiving address
    *  \"amount\" : total amount received by the address
    *  \"confirmations\" : number of confirmations of the most recent transaction included */
    json::json RPC::ListReceivedByAddress(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 2)
            return std::string(
                "listreceivedbyaddress [minconf=1] [includeempty=false]"
                " - [minconf] is the minimum number of confirmations before payments are included."
                " [includeempty] whether to include addresses that haven't received any payments.\n"
                " - Returns an array of objects containing:\n"
                "  \"address\" : receiving address\n"
                "  \"account\" : the account of the receiving address\n"
                "  \"amount\" : total amount received by the address\n"
                "  \"confirmations\" : number of confirmations of the most recent transaction included");

    //     return ListReceived(params, false);
    }

    /* listreceivedbyaccount [minconf=1] [includeempty=false]
    *  [minconf] is the minimum number of confirmations before payments are included.
    *  [includeempty] whether to include accounts that haven't received any payments
    *  Returns an array of objects containing:
    *  \"address\" : receiving address
    *  \"account\" : the account of the receiving address
    *  \"amount\" : total amount received by the address
    *  \"confirmations\" : number of confirmations of the most recent transaction incl*/
    json::json RPC::ListReceivedByAccount(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 2)
            return std::string(
                "listreceivedbyaccount [minconf=1] [includeempty=false]"
                " - [minconf] is the minimum number of confirmations before payments are included."
                " [includeempty] whether to include accounts that haven't received any payments.\n"
                " - Returns an array of objects containing:\n"
                "  \"account\" : the account of the receiving addresses\n"
                "  \"amount\" : total amount received by addresses with this account\n"
                "  \"confirmations\" : number of confirmations of the most recent transaction included");

    //     return ListReceived(params, true);
    }

    // void ListTransactions(const Wallet::CWalletTx& wtx, const string& strAccount, int nMinDepth, bool fLong, Array& ret)
    // {
    //     int64 nGeneratedImmature, nGeneratedMature, nFee;
    //     string strSentAccount;
    //     list<pair<Wallet::NexusAddress, int64> > listReceived;
    //     list<pair<Wallet::NexusAddress, int64> > listSent;

    //     wtx.GetAmounts(nGeneratedImmature, nGeneratedMature, listReceived, listSent, nFee, strSentAccount);

    //     bool fAllAccounts = (strAccount == string("*"));

    //     // Generated blocks assigned to account ""
    //     if ((nGeneratedMature+nGeneratedImmature) != 0 && (fAllAccounts || strAccount == ""))
    //     {
    //         Object entry;
    //         entry.push_back(Pair("account", string("")));
    //         if (nGeneratedImmature)
    //         {
    //             entry.push_back(Pair("category", wtx.GetDepthInMainChain() ? "immature" : "orphan"));
    //             entry.push_back(Pair("amount", ValueFromAmount(nGeneratedImmature)));
    //         }
    //         else if (wtx.IsCoinStake())
    //         {
    //             entry.push_back(Pair("category", "stake"));
    //             entry.push_back(Pair("amount", ValueFromAmount(nGeneratedMature)));
    //         }
    //         else
    //         {
    //             entry.push_back(Pair("category", "generate"));
    //             entry.push_back(Pair("amount", ValueFromAmount(nGeneratedMature)));
    //         }
    //         if (fLong)
    //             WalletTxToJSON(wtx, entry);
    //         ret.push_back(entry);
    //     }

    //     // Sent
    //     if ((!listSent.empty() || nFee != 0) && (fAllAccounts || strAccount == strSentAccount))
    //     {
    //         BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, int64)& s, listSent)
    //         {
    //             Object entry;
    //             entry.push_back(Pair("account", strSentAccount));
    //             entry.push_back(Pair("address", s.first.ToString()));
    //             entry.push_back(Pair("category", "send"));
    //             entry.push_back(Pair("amount", ValueFromAmount(-s.second)));
    //             entry.push_back(Pair("fee", ValueFromAmount(-nFee)));
    //             if (fLong)
    //                 WalletTxToJSON(wtx, entry);
    //             ret.push_back(entry);
    //         }
    //     }

    //     // Received
    //     if (listReceived.size() > 0 && wtx.GetDepthInMainChain() >= nMinDepth)
    //     {
    //         BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, int64)& r, listReceived)
    //         {
    //             string account;
    //             if (pwalletMain->mapAddressBook.count(r.first))
    //                 account = pwalletMain->mapAddressBook[r.first];
    //             if (fAllAccounts || (account == strAccount))
    //             {
    //                 Object entry;
    //                 entry.push_back(Pair("account", account));
    //                 entry.push_back(Pair("address", r.first.ToString()));
    //                 entry.push_back(Pair("category", "receive"));
    //                 entry.push_back(Pair("amount", ValueFromAmount(r.second)));
    //                 if (fLong)
    //                     WalletTxToJSON(wtx, entry);
    //                 ret.push_back(entry);
    //             }
    //         }
    //     }
    // }

    // void AcentryToJSON(const Wallet::CAccountingEntry& acentry, const string& strAccount, Array& ret)
    // {
    //     bool fAllAccounts = (strAccount == string("*"));

    //     if (fAllAccounts || acentry.strAccount == strAccount)
    //     {
    //         Object entry;
    //         entry.push_back(Pair("account", acentry.strAccount));
    //         entry.push_back(Pair("category", "move"));
    //         entry.push_back(Pair("time", (boost::int64_t)acentry.nTime));
    //         entry.push_back(Pair("amount", ValueFromAmount(acentry.nCreditDebit)));
    //         entry.push_back(Pair("otheraccount", acentry.strOtherAccount));
    //         entry.push_back(Pair("comment", acentry.strComment));
    //         ret.push_back(entry);
    //     }
    // }

    /* listtransactions [account] [count=10] [from=0]
       Returns up to [count] most recent transactions skipping the first [from] transactions for account [account]*/
    json::json RPC::ListTransactions(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 3)
            return std::string(
                "listtransactions [account] [count=10] [from=0]"
                " - Returns up to [count] most recent transactions skipping the first [from] transactions for account [account].");

    //     string strAccount = "*";
    //     if (jsonParams.size() > 0)
    //         strAccount = params[0].get_str();
    //     int nCount = 10;
    //     if (jsonParams.size() > 1)
    //         nCount = params[1].get_int();
    //     int nFrom = 0;
    //     if (jsonParams.size() > 2)
    //         nFrom = params[2].get_int();

    //     if (nCount < 0)
    //         throw JSONRPCError(-8, "Negative count");
    //     if (nFrom < 0)
    //         throw JSONRPCError(-8, "Negative from");

    //     Array ret;
    //     Wallet::CWalletDB walletdb(pwalletMain->strWalletFile);

    //     // First: get all Wallet::CWalletTx and Wallet::CAccountingEntry into a sorted-by-time multimap.
    //     typedef pair<Wallet::CWalletTx*, Wallet::CAccountingEntry*> TxPair;
    //     typedef multimap<int64, TxPair > TxItems;
    //     TxItems txByTime;

    //     // Note: maintaining indices in the database of (account,time) --> txid and (account, time) --> acentry
    //     // would make this much faster for applications that do this a lot.
    //     for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    //     {
    //         Wallet::CWalletTx* wtx = &((*it).second);
    //         txByTime.insert(make_pair(wtx->GetTxTime(), TxPair(wtx, (Wallet::CAccountingEntry*)0)));
    //     }
    //     list<Wallet::CAccountingEntry> acentries;
    //     walletdb.ListAccountCreditDebit(strAccount, acentries);
    //     BOOST_FOREACH(Wallet::CAccountingEntry& entry, acentries)
    //     {
    //         txByTime.insert(make_pair(entry.nTime, TxPair((Wallet::CWalletTx*)0, &entry)));
    //     }

    //     // iterate backwards until we have nCount items to return:
    //     for (TxItems::reverse_iterator it = txByTime.rbegin(); it != txByTime.rend(); ++it)
    //     {
    //         Wallet::CWalletTx *const pwtx = (*it).second.first;
    //         if (pwtx != 0)
    //             ListTransactions(*pwtx, strAccount, 0, true, ret);
    //         Wallet::CAccountingEntry *const pacentry = (*it).second.second;
    //         if (pacentry != 0)
    //             AcentryToJSON(*pacentry, strAccount, ret);

    //         if (ret.size() >= (nCount+nFrom)) break;
    //     }
    //     // ret is newest to oldest

    //     if (nFrom > (int)ret.size())
    //         nFrom = ret.size();
    //     if ((nFrom + nCount) > (int)ret.size())
    //         nCount = ret.size() - nFrom;
    //     Array::iterator first = ret.begin();
    //     std::advance(first, nFrom);
    //     Array::iterator last = ret.begin();
    //     std::advance(last, nFrom+nCount);

    //     if (last != ret.end()) ret.erase(last, ret.end());
    //     if (first != ret.begin()) ret.erase(ret.begin(), first);

    //     std::reverse(ret.begin(), ret.end()); // Return oldest to newest

    //     return ret;
     }


    /* listaddresses [max=100]
       Returns list of addresses */
    json::json RPC::ListAddresses(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 0)
            return std::string(
                "listaddresses [max=100]"
                " - Returns list of addresses");

    //     /* Limit the size. */
    //     int nMax = 100;
    //     if (jsonParams.size() > 0)
    //         nMax = params[0].get_int();

    //     /* Get the available addresses from the wallet */
    //     map<Wallet::NexusAddress, int64> mapAddresses;
    //     if(!pwalletMain->AvailableAddresses((unsigned int)GetUnifiedTimestamp(), mapAddresses))
    //         throw JSONRPCError(-3, "Error Extracting the Addresses from Wallet File. Please Try Again.");

    //     /* Find all the addresses in the list */
    //     Object list;
    //     for (map<Wallet::NexusAddress, int64>::iterator it = mapAddresses.begin(); it != mapAddresses.end() && list.size() < nMax; ++it)
    //         list.push_back(Pair(it->first.ToString(), ValueFromAmount(it->second)));

    //     return list;
    }

    /* listaccounts
       Returns Object that has account names as keys, account balances as values */
    json::json RPC::ListAccounts(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 1)
            return std::string(
                "listaccounts"
                " - Returns Object that has account names as keys, account balances as values.");

    //     int nMinDepth = 1;
    //     if (jsonParams.size() > 0)
    //         nMinDepth = params[0].get_int();

    //     map<string, int64> mapAccountBalances;
    //     BOOST_FOREACH(const PAIRTYPE(Wallet::NexusAddress, string)& entry, pwalletMain->mapAddressBook) {
    //         if (pwalletMain->HaveKey(entry.first)) // This address belongs to me
    //         {
    //             if(entry.second == "" || entry.second == "default")
    //                 mapAccountBalances["default"] = 0;
    //             else
    //                 mapAccountBalances[entry.second] = 0;
    //         }
    //     }

    //     /* Get the available addresses from the wallet */
    //     map<Wallet::NexusAddress, int64> mapAddresses;
    //     if(!pwalletMain->AvailableAddresses((unsigned int)GetUnifiedTimestamp(), mapAddresses))
    //         throw JSONRPCError(-3, "Error Extracting the Addresses from Wallet File. Please Try Again.");

    //     /* Find all the addresses in the list */
    //     for (map<Wallet::NexusAddress, int64>::iterator it = mapAddresses.begin(); it != mapAddresses.end(); ++it)
    //         if(pwalletMain->mapAddressBook.count(it->first))
    //         {
    //             string strAccount = pwalletMain->mapAddressBook[it->first];
    //             if(strAccount == "")
    //                 strAccount = "default";

    //             mapAccountBalances[strAccount] += it->second;
    //         }
    //         else
    //             mapAccountBalances["default"] += it->second;

    //     Object ret;
    //     BOOST_FOREACH(const PAIRTYPE(string, int64)& accountBalance, mapAccountBalances) {
    //         ret.push_back(Pair(accountBalance.first, ValueFromAmount(accountBalance.second)));
    //     }

    //     return ret;
    }

    /* listsinceblock [blockhash] [target-confirmations]
       Get all transactions in blocks since block [blockhash], or all transactions if omitted **/
    json::json RPC::ListSinceBlock(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp)
            return std::string(
                "listsinceblock [blockhash] [target-confirmations]"
                " - Get all transactions in blocks since block [blockhash], or all transactions if omitted");

    //     Core::CBlockIndex *pindex = NULL;
    //     int target_confirms = 1;

    //     if (jsonParams.size() > 0)
    //     {
    //         uint1024 blockId = 0;

    //         blockId.SetHex(params[0].get_str());
    //         pindex = Core::CBlockLocator(blockId).GetBlockIndex();
    //     }

    //     if (jsonParams.size() > 1)
    //     {
    //         target_confirms = params[1].get_int();

    //         if (target_confirms < 1)
    //             throw JSONRPCError(-8, "Invalid parameter");
    //     }

    //     int depth = pindex ? (1 + Core::ChainState::nBestHeight - pindex->nHeight) : -1;

    //     Array transactions;

    //     for (map<uint512, Wallet::CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); it++)
    //     {
    //         Wallet::CWalletTx tx = (*it).second;

    //         if (depth == -1 || tx.GetDepthInMainChain() < depth)
    //             ListTransactions(tx, "*", 0, true, transactions);
    //     }


    //     //NOTE: Do we need this code for anything?
    //     uint1024 lastblock;

    //     if (target_confirms == 1)
    //     {
    //         lastblock = Core::ChainState::hashBestChain;
    //     }
    //     else
    //     {
    //         int target_height = Core::pindexBest->nHeight + 1 - target_confirms;

    //         Core::CBlockIndex *block;
    //         for (block = Core::pindexBest;
    //              block && block->nHeight > target_height;
    //              block = block->pprev)  { }

    //         lastblock = block ? block->GetBlockHash() : 0;
    //     }

    //     Object ret;
    //     ret.push_back(Pair("transactions", transactions));
    //     ret.push_back(Pair("lastblock", lastblock.GetHex()));

    //     return ret;
    }

    /* gettransaction <txid>
       Get detailed information about <txid> */
    json::json RPC::GetTransaction(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 1)
            return std::string(
                "gettransaction <txid>"
                " - Get detailed information about <txid>");

    //     uint512 hash;
    //     hash.SetHex(params[0].get_str());

    //     Object entry;

    //     if (!pwalletMain->mapWallet.count(hash))
    //         throw JSONRPCError(-5, "Invalid or non-wallet transaction id");
    //     const Wallet::CWalletTx& wtx = pwalletMain->mapWallet[hash];

    //     int64 nCredit = wtx.GetCredit();
    //     int64 nDebit = wtx.GetDebit();
    //     int64 nNet = nCredit - nDebit;
    //     int64 nFee = (wtx.IsFromMe() ? wtx.GetValueOut() - nDebit : 0);

    //     entry.push_back(Pair("amount", ValueFromAmount(nNet - nFee)));
    //     if (wtx.IsFromMe())
    //         entry.push_back(Pair("fee", ValueFromAmount(nFee)));

    //     WalletTxToJSON(pwalletMain->mapWallet[hash], entry);

    //     Array details;
    //     ListTransactions(pwalletMain->mapWallet[hash], "*", 0, false, details);
    //     entry.push_back(Pair("details", details));

    //     return entry;
    }

    /* getrawtransaction <txid>
       Returns a string that is serialized, hex-encoded data for <txid> */
    json::json RPC::GetRawTransaction(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 1)
            return std::string(
                "getrawtransaction <txid>"
                " - Returns a string that is serialized,"
                " hex-encoded data for <txid>.");

    //     uint512 hash;
    //     hash.SetHex(params[0].get_str());

    //     Core::CTransaction tx;
    //     uint1024 hashBlock = 0;
    //     if (!Core::GetTransaction(hash, tx, hashBlock))
    //         throw JSONRPCError(-5, "No information available about transaction");

    //     DataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    //     ssTx << tx;
    //     return HexStr(ssTx.begin(), ssTx.end());
    }

    /* sendrawtransaction <hex string> [checkinputs=0]
       Submits raw transaction (serialized, hex-encoded) to local node and network.
       If checkinputs is non-zero, checks the validity of the inputs of the transaction before sending it */
    json::json RPC::SendRawTransaction(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() < 1 || jsonParams.size() > 2)
            return std::string(
                "sendrawtransaction <hex string> [checkinputs=0]"
                " - Submits raw transaction (serialized, hex-encoded) to local node and network."
                " If checkinputs is non-zero, checks the validity of the inputs of the transaction before sending it.");

    //     // parse hex string from parameter
    //     vector<unsigned char> txData(ParseHex(params[0].get_str()));
    //     DataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    //     bool fCheckInputs = false;
    //     if (jsonParams.size() > 1)
    //         fCheckInputs = (params[1].get_int() != 0);
    //     Core::CTransaction tx;

    //     // deserialize binary data stream
    //     try {
    //         ssData >> tx;
    //     }
    //     catch (std::exception &e) {
    //         throw JSONRPCError(-22, "TX decode failed");
    //     }
    //     uint512 hashTx = tx.GetHash();

    //     // See if the transaction is already in a block
    //     // or in the memory pool:
    //     Core::CTransaction existingTx;
    //     uint1024 hashBlock = 0;
    //     if (Core::GetTransaction(hashTx, existingTx, hashBlock))
    //     {
    //         if (hashBlock != 0)
    //             throw JSONRPCError(-5, string("transaction already in block ")+hashBlock.GetHex());
    //         // Not in block, but already in the memory pool; will drop
    //         // through to re-relay it.
    //     }
    //     else
    //     {
    //         // push to local node
    //         LLD::CIndexDB txdb("r");
    //         if (!tx.AcceptToMemoryPool(txdb, fCheckInputs))
    //             throw JSONRPCError(-22, "TX rejected");

    //         SyncWithWallets(tx, NULL, true);
    //     }
    //     RelayMessage(CInv(MSG_TX, hashTx), tx);

    //     return hashTx.GetHex();
    }

    /* validateaddress <Nexusaddress>
       Return information about <Nexusaddress> */
    json::json RPC::ValidateAddress(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 1)
            return std::string(
                "validateaddress <Nexusaddress>"
                " - Return information about <Nexusaddress>.");

    //     Wallet::NexusAddress address(params[0].get_str());
    //     bool isValid = address.IsValid();

    //     Object ret;
    //     ret.push_back(Pair("isvalid", isValid));
    //     if (isValid)
    //     {
    //         string currentAddress = address.ToString();
    //         ret.push_back(Pair("address", currentAddress));
    //         if (pwalletMain->HaveKey(address))
    //         {
    //             ret.push_back(Pair("ismine", true));
    //             std::vector<unsigned char> vchPubKey;
    //             pwalletMain->GetPubKey(address, vchPubKey);
    //             ret.push_back(Pair("pubkey", HexStr(vchPubKey)));
    //             Wallet::CKey key;
    //             key.SetPubKey(vchPubKey);
    //             ret.push_back(Pair("iscompressed", key.IsCompressed()));
    //         }
    //         else if (pwalletMain->HaveCScript(address.GetHash256()))
    //         {
    //             ret.push_back(Pair("isscript", true));
    //             Wallet::CScript subscript;
    //             pwalletMain->GetCScript(address.GetHash256(), subscript);
    //             ret.push_back(Pair("ismine", Wallet::IsMine(*pwalletMain, subscript)));
    //             std::vector<Wallet::NexusAddress> addresses;
    //             Wallet::TransactionType whichType;
    //             int nRequired;
    //             Wallet::ExtractAddresses(subscript, whichType, addresses, nRequired);
    //             ret.push_back(Pair("script", Wallet::GetTxnOutputType(whichType)));
    //             Array a;
    //             BOOST_FOREACH(const Wallet::NexusAddress& addr, addresses)
    //                 a.push_back(addr.ToString());
    //             ret.push_back(Pair("addresses", a));
    //             if (whichType == Wallet::TX_MULTISIG)
    //                 ret.push_back(Pair("sigsrequired", nRequired));
    //         }
    //         else
    //             ret.push_back(Pair("ismine", false));
    //         if (pwalletMain->mapAddressBook.count(address))
    //             ret.push_back(Pair("account", pwalletMain->mapAddressBook[address]));
    //     }
    //     return ret;
    }

    /* Make a public/private key pair. [prefix] is optional preferred prefix for the public key */
    json::json RPC::MakeKeyPair(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 1)
            return std::string(
                "makekeypair [prefix]"
                " - Make a public/private key pair."
                " [prefix] is optional preferred prefix for the public key.");

    //     string strPrefix = "";
    //     if (jsonParams.size() > 0)
    //         strPrefix = params[0].get_str();

    //     Wallet::CKey key;
    //     int nCount = 0;
    //     do
    //     {
    //         key.MakeNewKey(false);
    //         nCount++;
    //     } while (nCount < 10000 && strPrefix != HexStr(key.GetPubKey()).substr(0, strPrefix.size()));

    //     if (strPrefix != HexStr(key.GetPubKey()).substr(0, strPrefix.size()))
    //         return Value::null;

    //     Wallet::CPrivKey vchPrivKey = key.GetPrivKey();
    //     Object result;
    //     result.push_back(Pair("PrivateKey", HexStr<Wallet::CPrivKey::iterator>(vchPrivKey.begin(), vchPrivKey.end())));
    //     result.push_back(Pair("PublicKey", HexStr(key.GetPubKey())));
    //     return result;
    }

    /* unspentbalance [\"address\",...]
       Returns the total amount of unspent Nexus for given address. This is a more accurate command than Get Balance */
    /* TODO: Account balance based on unspent outputs in the core.
        This will be good to determine the actual balance based on the registry of the current addresses
        Associated with the specific accounts. This needs to be fixed properly so thaat the wallet accounting
        is done properly.

        Second TODO: While at this the wallet core code needs to be reworked in orcder to mitigate the issue of
        having a large number of transactions in the actual memory map which can slow the entire process down. */
    json::json RPC::UnspentBalance(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 1)
            return std::string(
                "unspentbalance [\"address\",...]"
                " - Returns the total amount of unspent Nexus for given address."
                " This is a more accurate command than Get Balance.");

    //     set<Wallet::NexusAddress> setAddresses;
    //     if (jsonParams.size() > 0)
    //     {
    //         for(int i = 0; i < jsonParams.size(); i++)
    //         {
    //             Wallet::NexusAddress address(params[i].get_str());
    //             if (!address.IsValid()) {
    //                 throw JSONRPCError(-5, string("Invalid Nexus address: ")+params[i].get_str());
    //             }
    //             if (setAddresses.count(address)){
    //                 throw JSONRPCError(-8, string("Invalid parameter, duplicated address: ")+params[i].get_str());
    //             }
    //            setAddresses.insert(address);
    //         }
    //     }

    //     vector<Wallet::COutput> vecOutputs;
    //     pwalletMain->AvailableCoins((unsigned int)GetUnifiedTimestamp(), vecOutputs, false);

    //     int64 nCredit = 0;
    //     BOOST_FOREACH(const Wallet::COutput& out, vecOutputs)
    //     {
    //         if(setAddresses.size())
    //         {
    //             Wallet::NexusAddress address;
    //             if(!ExtractAddress(out.tx->vout[out.i].scriptPubKey, address))
    //                 continue;

    //             if (!setAddresses.count(address))
    //                 continue;
    //         }

    //         int64 nValue = out.tx->vout[out.i].nValue;
    //         const Wallet::CScript& pk = out.tx->vout[out.i].scriptPubKey;
    //         Wallet::NexusAddress address;

    //         printf("txid %s | ", out.tx->GetHash().GetHex().c_str());
    //         printf("vout %u | ", out.i);
    //         if (Wallet::ExtractAddress(pk, address))
    //             printf("address %s |", address.ToString().c_str());

    //         printf("amount %f |", (double)nValue / COIN);
    //         printf("confirmations %u\n",out.nDepth);

    //         nCredit += nValue;
    //     }

    //     return ValueFromAmount(nCredit);
    }


    /* listunspent [minconf=1] [maxconf=9999999]  [\"address\",...]
       Returns array of unspent transaction outputs with between minconf and maxconf (inclusive) confirmations.
       Optionally filtered to only include txouts paid to specified addresses.
       Results are an array of Objects, each of which has:
       {txid, vout, scriptPubKey, amount, confirmations} */
    json::json RPC::ListUnspent(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 3)
            return std::string(
                "listunspent [minconf=1] [maxconf=9999999]  [\"address\",...]"
                " - Returns array of unspent transaction outputs"
                "with between minconf and maxconf (inclusive) confirmations."
                " Optionally filtered to only include txouts paid to specified addresses.\n"
                " - Results are an array of Objects, each of which has:"
                "{txid, vout, scriptPubKey, amount, confirmations}");

    //     int nMinDepth = 1;
    //     if (jsonParams.size() > 0)
    //         nMinDepth = params[0].get_int();

    //     int nMaxDepth = 9999999;
    //     if (jsonParams.size() > 1)
    //         nMaxDepth = params[1].get_int();

    //     set<Wallet::NexusAddress> setAddress;
    //     if (jsonParams.size() > 2)
    //     {
    //         Array inputs = params[2].get_array();
    //         BOOST_FOREACH(Value& input, inputs)
    //         {
    //             Wallet::NexusAddress address(input.get_str());
    //             if (!address.IsValid())
    //                 throw JSONRPCError(-5, string("Invalid Nexus address: ")+input.get_str());

    //             if (setAddress.count(address))
    //                 throw JSONRPCError(-8, string("Invalid parameter, duplicated address: ")+input.get_str());

    //            setAddress.insert(address);
    //         }
    //     }

    //     Array results;
    //     vector<Wallet::COutput> vecOutputs;
    //     pwalletMain->AvailableCoins((unsigned int)GetUnifiedTimestamp(), vecOutputs, false);
    //     BOOST_FOREACH(const Wallet::COutput& out, vecOutputs)
    //     {
    //         if (out.nDepth < nMinDepth || out.nDepth > nMaxDepth)
    //             continue;

    //         if(!setAddress.empty())
    //         {
    //             Wallet::NexusAddress address;
    //             if(!ExtractAddress(out.tx->vout[out.i].scriptPubKey, address))
    //                 continue;

    //             if (!setAddress.count(address))
    //                 continue;
    //         }

    //         int64 nValue = out.tx->vout[out.i].nValue;
    //         const Wallet::CScript& pk = out.tx->vout[out.i].scriptPubKey;
    //         Wallet::NexusAddress address;
    //         Object entry;
    //         entry.push_back(Pair("txid", out.tx->GetHash().GetHex()));
    //         entry.push_back(Pair("vout", out.i));
    //         if (Wallet::ExtractAddress(pk, address))
    //         {
    //             entry.push_back(Pair("address", address.ToString()));
    //             if (pwalletMain->mapAddressBook.count(address))
    //                 entry.push_back(Pair("account", pwalletMain->mapAddressBook[address]));
    //         }
    //         entry.push_back(Pair("scriptPubKey", HexStr(pk.begin(), pk.end())));
    //         entry.push_back(Pair("amount",ValueFromAmount(nValue)));
    //         entry.push_back(Pair("confirmations",out.nDepth));

    //         results.push_back(entry);
    //     }

    //     return results;
    }


}