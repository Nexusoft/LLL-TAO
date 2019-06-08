/*__________________________________________________________________________________________

        (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

        (c) Copyright The Nexus Developers 2014 - 2019

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>

#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>
#include <Legacy/wallet/accountingentry.h>
#include <Legacy/wallet/reservekey.h>
#include <Legacy/wallet/output.h>

#include <Legacy/include/money.h>
#include <Legacy/include/evaluate.h>
#include <LLC/hash/SK.h>
#include <Util/include/base64.h>
#include <Util/include/hex.h>
#include <Legacy/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>
#include <LLD/include/global.h>

#include <LLP/include/version.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        std::string AccountFromValue(const std::string& value)
        {
            if(value == "*")
                throw APIException(-11, "Invalid account name");
            return value;
        }

        /* getnewaddress [account]
        Returns a new Nexus address for receiving payments.
        If [account] is specified (recommended), it is added to the address book
        so payments received with the address will be credited to [account] */
        json::json RPC::GetNewAddress(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() > 1)
                return std::string(
                    "getnewaddress [account]"
                    " - Returns a new Nexus address for receiving payments.  "
                    "If [account] is specified (recommended), it is added to the address book "
                    "so payments received with the address will be credited to [account].");

            // Parse the account first so we don't generate a key if there's an error
            std::string strAccount = "default";
            if(params.size() > 0)
                strAccount = params[0];

            if(!Legacy::Wallet::GetInstance().IsLocked())
                Legacy::Wallet::GetInstance().GetKeyPool().TopUpKeyPool();

            // Generate a new key that is added to wallet
            std::vector<unsigned char> newKey;
            if(!Legacy::Wallet::GetInstance().GetKeyPool().GetKeyFromPool(newKey, false))
                throw APIException(-12, "Error: Keypool ran out, please call keypoolrefill first");
            Legacy::NexusAddress address(newKey);

            Legacy::Wallet::GetInstance().GetAddressBook().SetAddressBookName(address, strAccount);

            return address.ToString();
            json::json ret;
            return ret;
        }




        /* getaccountaddress <account>
        Returns the current Nexus address for receiving payments to this account */
        json::json RPC::GetAccountAddress(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 1)
                return std::string(
                    "getaccountaddress <account>"
                    " - Returns the current Nexus address for receiving payments to this account.");

            // Parse the account first so we don't generate a key if there's an error
            std::string strAccount = AccountFromValue(params[0]);

            json::json ret;
            ret = Legacy::Wallet::GetInstance().GetAddressBook().GetAccountAddress(strAccount).ToString();

            return ret;
        }


        /* setaccount <Nexusaddress> <account>
        Sets the account associated with the given address */
        json::json RPC::SetAccount(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() < 1 || params.size() > 2)
                return std::string(
                    "setaccount <Nexusaddress> <account>"
                    " - Sets the account associated with the given address.");

            Legacy::NexusAddress address(params[0].get<std::string>());
            if(!address.IsValid())
                throw APIException(-5, "Invalid Nexus address");


            std::string strAccount;
            if(params.size() > 1)
                strAccount = AccountFromValue(params[1]);

            // Detect when changing the account of an address that is the 'unused current key' of another account:
            if(Legacy::Wallet::GetInstance().GetAddressBook().HasAddress(address))
            {
                std::string strOldAccount = Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().at(address);
                if(address == Legacy::Wallet::GetInstance().GetAddressBook().GetAccountAddress(strOldAccount))
                    Legacy::Wallet::GetInstance().GetAddressBook().GetAccountAddress(strOldAccount, true);
            }

            Legacy::Wallet::GetInstance().GetAddressBook().SetAddressBookName(address, strAccount);

            json::json ret;
            return ret;
        }

        /* getaccount <Nexusaddress>
        Returns the account associated with the given address */
        json::json RPC::GetAccount(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 1)
                return std::string(
                    "getaccount <Nexusaddress>"
                    " - Returns the account associated with the given address.");

            Legacy::NexusAddress address(params[0].get<std::string>());
            if(!address.IsValid())
                throw APIException(-5, "Invalid Nexus address");

            std::string strAccount;
            std::map<Legacy::NexusAddress, std::string>::const_iterator mi = Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().find(address);
            if(mi != Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().end() && !(*mi).second.empty())
                strAccount = (*mi).second;
            return strAccount;
        }

        /* getaddressesbyaccount <account>
        Returns the list of addresses for the given account */
        json::json RPC::GetAddressesByAccount(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 1)
                return std::string(
                    "getaddressesbyaccount <account>"
                    " - Returns the list of addresses for the given account.");

            std::string strAccount = AccountFromValue(params[0]);

            // Find all addresses that have the given account
            json::json ret;
            for(const auto& entry : Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap())
            {
                const Legacy::NexusAddress& address = entry.first;
                const std::string& strName = entry.second;
                if(strName == strAccount || (strName == "" && strAccount == "default"))
                    ret.push_back(address.ToString());
            }

            return ret;
        }



        /* sendtoaddress <Nexusaddress> <amount> [comment] [comment-to]
        *  - <amount> is a real and is rounded to the nearest 0.000001
        *  requires wallet passphrase to be set with walletpassphrase first */
        json::json RPC::SendToAddress(const json::json& params, bool fHelp)
        {
            if(Legacy::Wallet::GetInstance().IsCrypted() && (fHelp || params.size() < 2 || params.size() > 4))
                return std::string(
                    "sendtoaddress <Nexusaddress> <amount> [comment] [comment-to]"
                    " - <amount> is a real and is rounded to the nearest 0.000001"
                    " requires wallet passphrase to be set with walletpassphrase first");
            if(!Legacy::Wallet::GetInstance().IsCrypted() && (fHelp || params.size() < 2 || params.size() > 4))
                return std::string(
                    "sendtoaddress <Nexusaddress> <amount> [comment] [comment-to]"
                    "<amount> is a real and is rounded to the nearest 0.000001");

            Legacy::NexusAddress address(params[0].get<std::string>());
            if(!address.IsValid())
                throw APIException(-5, "Invalid Nexus address");

            // Amount
            int64_t nAmount = Legacy::AmountToSatoshis(params[1]);
            if(nAmount < Legacy::MIN_TXOUT_AMOUNT)
                throw APIException(-101, "Send amount too small");

            // Wallet comments
            Legacy::WalletTx wtx;
            if(params.size() > 2 && !params[2].is_null() && params[2].get<std::string>() != "")
                wtx.mapValue["comment"] = params[2].get<std::string>();
            if(params.size() > 3 && !params[3].is_null()&& params[3].get<std::string>() != "")
                wtx.mapValue["to"]      = params[3].get<std::string>();

            if(Legacy::Wallet::GetInstance().IsLocked())
                throw APIException(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

            std::string strError = Legacy::Wallet::GetInstance().SendToNexusAddress(address, nAmount, wtx);
            if(strError != "")
                throw APIException(-4, strError);

            return wtx.GetHash().GetHex();
        }

        /* signmessage <Nexusaddress> <message>
        Sign a message with the private key of an address */
        json::json RPC::SignMessage(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 2)
                return std::string(
                    "signmessage <Nexusaddress> <message>"
                    " - Sign a message with the private key of an address");

            if(Legacy::Wallet::GetInstance().IsLocked())
                throw APIException(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

            std::string strAddress = params[0].get<std::string>();
            std::string strMessage = params[1].get<std::string>();

            Legacy::NexusAddress addr(strAddress);
            if(!addr.IsValid())
                throw APIException(-3, "Invalid address");

            LLC::ECKey key;
            if(!Legacy::Wallet::GetInstance().GetKey(addr, key))
                throw APIException(-4, "Private key not available");

            DataStream ss(SER_GETHASH, 0);
            ss << Legacy::strMessageMagic;
            ss << strMessage;

            std::vector<unsigned char> vchSig;
            if(!key.SignCompact(LLC::SK256(ss.begin(), ss.end()), vchSig))
                throw APIException(-5, "Sign failed");

            return encoding::EncodeBase64(&vchSig[0], vchSig.size());

        }

        /* verifymessage <Nexusaddress> <signature> <message>
        Verify a signed message */
        json::json RPC::VerifyMessage(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 3)
                return std::string(
                    "verifymessage <Nexusaddress> <signature> <message>"
                    " - Verify a signed message");

            std::string strAddress  = params[0].get<std::string>();
            std::string strSign     = params[1].get<std::string>();
            std::string strMessage  = params[2].get<std::string>();

            Legacy::NexusAddress addr(strAddress);
            if(!addr.IsValid())
                throw APIException(-3, "Invalid address");

            bool fInvalid = false;
            std::vector<unsigned char> vchSig = encoding::DecodeBase64(strSign.c_str(), &fInvalid);

            if(fInvalid)
                throw APIException(-5, "Malformed base64 encoding");

            DataStream ss(SER_GETHASH, 0);
            ss << Legacy::strMessageMagic;
            ss << strMessage;

            LLC::ECKey key;
            if(!key.SetCompactSignature(LLC::SK256(ss.begin(), ss.end()), vchSig))
                return false;

            return (Legacy::NexusAddress(key.GetPubKey()) == addr);

        }

        /* getreceivedbyaddress <Nexusaddress> [minconf=1]
        Returns the total amount received by <Nexusaddress> in transactions with at least [minconf] confirmations */
        json::json RPC::GetReceivedByAddress(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() < 1 || params.size() > 2)
                return std::string(
                    "getreceivedbyaddress <Nexusaddress> [minconf=1]"
                    " - Returns the total amount received by <Nexusaddress> in transactions with at least [minconf] confirmations.");

            // Nexus address
            Legacy::NexusAddress address = Legacy::NexusAddress(params[0].get<std::string>());
            Legacy::Script scriptPubKey;
            if(!address.IsValid())
                throw APIException(-5, "Invalid Nexus address");
            scriptPubKey.SetNexusAddress(address);
            if(!Legacy::IsMine(Legacy::Wallet::GetInstance(),scriptPubKey))
                return (double)0.0;

            // Minimum confirmations
            int nMinDepth = 1;
            if(params.size() > 1)
                nMinDepth = params[1];

            // Tally
            int64_t nAmount = 0;
            for(const auto& entry : Legacy::Wallet::GetInstance().mapWallet)
            {
                const Legacy::WalletTx& wtx = entry.second;
                if(wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
                    continue;

                for(const Legacy::TxOut& txout : wtx.vout)
                    if(txout.scriptPubKey == scriptPubKey)
                        if(wtx.GetDepthInMainChain() >= nMinDepth)
                            nAmount += txout.nValue;
           }

            return  Legacy::SatoshisToAmount(nAmount);

        }


        void GetAccountAddresses(const std::string& strAccount, std::set<Legacy::NexusAddress>& setAddress)
        {
            for(const auto& item : Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap())
            {
                const Legacy::NexusAddress& address = item.first;
                const std::string& strName = item.second;
                if(strName == strAccount)
                    setAddress.insert(address);
            }
        }

        /* getreceivedbyaccount <account> [minconf=1]
        Returns the total amount received by addresses with <account> in transactions with at least [minconf] confirmations */
        json::json RPC::GetReceivedByAccount(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() < 1 || params.size() > 2)
                return std::string(
                    "getreceivedbyaccount <account> [minconf=1]"
                    " - Returns the total amount received by addresses with <account> in transactions with at least [minconf] confirmations.");

            // Minimum confirmations
            int nMinDepth = 1;
            if(params.size() > 1)
                nMinDepth = params[1];

            // Get the set of pub keys assigned to account
            std::string strAccount = AccountFromValue(params[0]);
            std::set<Legacy::NexusAddress> setAddress;
            GetAccountAddresses(strAccount, setAddress);

            // Tally
            int64_t nAmount = 0;
            for(const auto& entry : Legacy::Wallet::GetInstance().mapWallet)
            {
                const Legacy::WalletTx& wtx = entry.second;
                if(wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
                    continue;

                for(const Legacy::TxOut& txout : wtx.vout)
                {
                    Legacy::NexusAddress address;
                    if(ExtractAddress(txout.scriptPubKey, address) && Legacy::Wallet::GetInstance().HaveKey(address) && setAddress.count(address))
                        if(wtx.GetDepthInMainChain() >= nMinDepth)
                            nAmount += txout.nValue;
                }

            }

             return Legacy::SatoshisToAmount(nAmount);
        }

        int64_t GetAccountBalance(Legacy::WalletDB& walletdb, const std::string& strAccount, int nMinDepth)
        {
            int64_t nBalance = 0;
            Legacy::Wallet::GetInstance().BalanceByAccount(strAccount, nBalance, nMinDepth);

            return nBalance;
        }


        int64_t GetAccountBalance(const std::string& strAccount, int nMinDepth)
        {
            Legacy::WalletDB walletdb(Legacy::Wallet::GetInstance().GetWalletFile());
            return GetAccountBalance(walletdb, strAccount, nMinDepth);
        }

        /* getbalance [account] [minconf=1]
        *  If [account] is not specified, returns the server's total available balance.
        *  If [account] is specified, returns the balance in the account */
        json::json RPC::GetBalance(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() > 2)
                return std::string(
                    "getbalance [account] [minconf=1]"
                    " - If [account] is not specified, returns the server's total available balance."
                    " If [account] is specified, returns the balance in the account.");

            if(params.size() == 0)
                return  Legacy::SatoshisToAmount(Legacy::Wallet::GetInstance().GetBalance());

            int nMinDepth = 1;
            if(params.size() > 1)
                nMinDepth = params[1];

            if(params[0].get<std::string>() == "*") {
                // Calculate total balance a different way from GetBalance()
                // (GetBalance() sums up all unspent TxOuts)
                // getbalance and getbalance '*' should always return the same number.
                int64_t nBalance = 0;
                for(const auto& entry : Legacy::Wallet::GetInstance().mapWallet)
                {
                    const Legacy::WalletTx& wtx = entry.second;
                    if(!wtx.IsFinal())
                        continue;

                    int64_t allGeneratedImmature, allGeneratedMature, allFee;
                    allGeneratedImmature = allGeneratedMature = allFee = 0;
                    std::string strSentAccount;
                    std::list<std::pair<Legacy::NexusAddress, int64_t> > listReceived;
                    std::list<std::pair<Legacy::NexusAddress, int64_t> > listSent;
                    wtx.GetAmounts(allGeneratedImmature, allGeneratedMature, listReceived, listSent, allFee, strSentAccount);
                    if(wtx.GetDepthInMainChain() >= nMinDepth)
                    {
                        for(const auto& r : listReceived)
                            nBalance += r.second;
                    }
                    for(const auto& r : listSent)
                        nBalance -= r.second;
                    nBalance -= allFee;
                    nBalance += allGeneratedMature;
                }
                return  Legacy::SatoshisToAmount(nBalance);
            }

            std::string strAccount = AccountFromValue(params[0]);
            int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);

            return Legacy::SatoshisToAmount(nBalance);

        }


        template<typename key, typename value>
        bool Find(std::map<key, value> map, value search)
        {
            for(typename std::map<key, value>::iterator it = map.begin(); it != map.end(); ++it)
                if(it->second == search)
                    return true;

            return false;
        }

        /* move <fromaccount> <toaccount> <amount> [minconf=1] [comment]
        Move from one account in your wallet to another */
        json::json RPC::MoveCmd(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() < 3 || params.size() > 5)
                return std::string(
                    "move <fromaccount> <toaccount> <amount> [minconf=1] [comment]"
                    " - Move from one account in your wallet to another.");

            std::string strFrom = AccountFromValue(params[0]);
            std::string strTo = AccountFromValue(params[1]);
            int64_t nAmount = Legacy::AmountToSatoshis(params[2]);

            // unused parameter, used to be nMinDepth, keep type-checking it though
            if(params.size() > 3 && !params[3].is_number())
                throw APIException(-3, "Invalid minconf value");

            std::string strComment;
            if(params.size() > 4)
                strComment = params[4].get<std::string>();

            /* Check for users. */
            if((strFrom != "default" || strFrom != "") && !Find(Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap(), strFrom))
                throw APIException(-5, debug::safe_printstr(strFrom, " from account doesn't exist."));

            if((strTo != "default" || strTo != "") && !Find(Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap(), strTo))
                throw APIException(-5, debug::safe_printstr(strTo, " to account doesn't exist."));

            /* Build the from transaction. */
            Legacy::WalletTx wtx;
            wtx.strFromAccount = strFrom;

            /* Watch for encryption. */
            if(Legacy::Wallet::GetInstance().IsLocked())
                throw APIException(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

            /* Check the account balance. */
            int64_t nBalance = GetAccountBalance(strFrom, 1);
            if(nAmount > nBalance)
                throw APIException(-6, "Account has insufficient funds");

            Legacy::NexusAddress address = Legacy::Wallet::GetInstance().GetAddressBook().GetAccountAddress(strTo);

            std::string strError = Legacy::Wallet::GetInstance().SendToNexusAddress(address, nAmount, wtx, false, 1);
            if(strError != "")
                throw APIException(-4, strError);

            return wtx.GetHash().GetHex();
        }

        /* sendfrom <fromaccount> <toNexusaddress> <amount> [minconf=1] [comment] [comment-to]
        * <amount> is a real and is rounded to the nearest 0.000001
        * requires wallet passphrase to be set with walletpassphrase first */
        json::json RPC::SendFrom(const json::json& params, bool fHelp)
        {
            if(Legacy::Wallet::GetInstance().IsCrypted() && (fHelp || params.size() < 3 || params.size() > 6))
                return std::string(
                    "sendfrom <fromaccount> <toNexusaddress> <amount> [minconf=1] [comment] [comment-to]"
                    " - <amount> is a real and is rounded to the nearest 0.000001"
                    " requires wallet passphrase to be set with walletpassphrase first");
            if(!Legacy::Wallet::GetInstance().IsCrypted() && (fHelp || params.size() < 3 || params.size() > 6))
                return std::string(
                    "sendfrom <fromaccount> <toNexusaddress> <amount> [minconf=1] [comment] [comment-to]"
                    "<amount> is a real and is rounded to the nearest 0.000001");

            std::string strAccount = AccountFromValue(params[0]);
            Legacy::NexusAddress address(params[1].get<std::string>());
            if(!address.IsValid())
                throw APIException(-5, "Invalid Nexus address");
            int64_t nAmount = Legacy::AmountToSatoshis(params[2]);
            if(nAmount < Legacy::MIN_TXOUT_AMOUNT)
                throw APIException(-101, "Send amount too small");
            int nMinDepth = 1;
            if(params.size() > 3)
                nMinDepth = params[3];

            Legacy::WalletTx wtx;
            wtx.strFromAccount = strAccount;
            if(params.size() > 4 && !params[4].is_null() && params[4].get<std::string>() != "")
                wtx.mapValue["comment"] = params[4].get<std::string>();
            if(params.size() > 5 && !params[5].is_null() && params[5].get<std::string>() != "")
                wtx.mapValue["to"]      = params[5].get<std::string>();

            if(Legacy::Wallet::GetInstance().IsLocked())
                throw APIException(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

            // Check funds
            int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);
            if(nAmount > nBalance)
                throw APIException(-6, "Account has insufficient funds");

            // Send
            std::string strError = Legacy::Wallet::GetInstance().SendToNexusAddress(address, nAmount, wtx);
            if(strError != "")
                throw APIException(-4, strError);

            return wtx.GetHash().GetHex();
        }

        /* sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]
        * - amounts are double-precision floating point numbers
        * requires wallet passphrase to be set with walletpassphrase first*/
        json::json RPC::SendMany(const json::json& params, bool fHelp)
        {
            if(Legacy::Wallet::GetInstance().IsCrypted() && (fHelp || params.size() < 2 || params.size() > 4))
                return std::string(
                    "sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]"
                    " - amounts are double-precision floating point numbers"
                    " requires wallet passphrase to be set with walletpassphrase first");
            if(!Legacy::Wallet::GetInstance().IsCrypted() && (fHelp || params.size() < 2 || params.size() > 4))
                return std::string(
                    "sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]"
                    "amounts are double-precision floating point numbers");

            std::string strAccount = AccountFromValue(params[0]);
            if(!params[1].is_object())
                throw APIException(-8, std::string("Invalid recipient list format"));

            json::json sendTo = params[1];
            int nMinDepth = 1;
            if(params.size() > 2)
                nMinDepth = params[2];

            Legacy::WalletTx wtx;
            wtx.strFromAccount = strAccount;
            if(params.size() > 3 && !params[3].is_null() && params[3].get<std::string>() != "")
                wtx.mapValue["comment"] = params[3].get<std::string>();

            std::set<Legacy::NexusAddress> setAddress;
            std::vector<std::pair<Legacy::Script, int64_t> > vecSend;

            int64_t totalAmount = 0;
            for(json::json::iterator it = sendTo.begin(); it != sendTo.end(); ++it)
            {
                Legacy::NexusAddress address(it.key());
                if(!address.IsValid())
                    throw APIException(-5, std::string("Invalid Nexus address:")+it.key());

                if(setAddress.count(address))
                    throw APIException(-8, std::string("Invalid parameter, duplicated address: ")+it.key());
                setAddress.insert(address);

                Legacy::Script scriptPubKey;
                scriptPubKey.SetNexusAddress(address);
                int64_t nAmount = Legacy::AmountToSatoshis(it.value());
                if(nAmount < Legacy::MIN_TXOUT_AMOUNT)
                    throw APIException(-101, "Send amount too small");
                totalAmount += nAmount;

                vecSend.push_back(make_pair(scriptPubKey, nAmount));
            }

            if(Legacy::Wallet::GetInstance().IsLocked())
                throw APIException(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
            if(Legacy::fWalletUnlockMintOnly)
                throw APIException(-13, "Error: Wallet unlocked for block minting only.");

            // Check funds
            int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);
            if(totalAmount > nBalance)
                throw APIException(-6, "Account has insufficient funds");

            // Send
            Legacy::ReserveKey keyChange(Legacy::Wallet::GetInstance());
            int64_t nFeeRequired = 0;
            bool fCreated = Legacy::Wallet::GetInstance().CreateTransaction(vecSend, wtx, keyChange, nFeeRequired);
            if(!fCreated)
            {
                if(totalAmount + nFeeRequired > Legacy::Wallet::GetInstance().GetBalance())
                    throw APIException(-6, "Insufficient funds");
                throw APIException(-4, "Transaction creation failed");
            }
            if(!Legacy::Wallet::GetInstance().CommitTransaction(wtx, keyChange))
                throw APIException(-4, "Transaction commit failed");

            return wtx.GetHash().GetHex();
        }

        /* addmultisigaddress <nrequired> <'[\"key\",\"key\"]'> [account]
        *  Add a nrequired-to-sign multisignature address to the wallet
        *  each key is a nexus address or hex-encoded public key.
        *  If [account] is specified, assign address to [account]. */
        json::json RPC::AddMultisigAddress(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() < 2 || params.size() > 3)
            {
                std::string msg = "addmultisigaddress <nrequired> <'[\"key\",\"key\"]'> [account]"
                    " - Add a nrequired-to-sign multisignature address to the wallet"
                    " each key is a nexus address or hex-encoded public key."
                    " If [account] is specified, assign address to [account].";
                return std::string(msg);
            }

            int nRequired = params[0];

            if(!params[1].is_array())
                throw APIException(-8, std::string("Invalid address array format"));

            json::json keys = params[1];
            std::string strAccount;
            if(params.size() > 2)
                strAccount = AccountFromValue(params[2]);

            // Gather public keys
            if(nRequired < 1)
                return std::string("a multisignature address must require at least one key to redeem");
            if((int)keys.size() < nRequired)
                return debug::safe_printstr("not enough keys supplied ",
                              "(got ", keys.size(), " keys, but need at least ", nRequired, " to redeem)");
            std::vector<LLC::ECKey> pubkeys;
            pubkeys.resize(keys.size());
            for(unsigned int i = 0; i < keys.size(); i++)
            {
                const std::string& ks = keys[i].get<std::string>();

                // Case 1: nexus address and we have full public key:
                Legacy::NexusAddress address(ks);
                if(address.IsValid())
                {
                    if(address.IsScript())
                        return debug::safe_printstr(ks, " is a pay-to-script address");

                    std::vector<unsigned char> vchPubKey;
                    if(!Legacy::Wallet::GetInstance().GetPubKey(address, vchPubKey))
                        return debug::safe_printstr("no full public key for address ", ks);

                    if(vchPubKey.empty() || !pubkeys[i].SetPubKey(vchPubKey))
                        return debug::safe_printstr(" Invalid public key: ", ks);
                }

                // Case 2: hex public key
                else if(IsHex(ks))
                {
                    std::vector<unsigned char> vchPubKey = ParseHex(ks);
                    if(vchPubKey.empty() || !pubkeys[i].SetPubKey(vchPubKey))
                        return std::string(" Invalid public key: "+ks);
                }
                else
                {
                    return std::string(" Invalid public key: "+ks);
                }
           }

            // Construct using pay-to-script-hash:
            Legacy::Script inner;
            inner.SetMultisig(nRequired, pubkeys);

            uint256_t scriptHash = LLC::SK256(inner);
            Legacy::Script scriptPubKey;
            scriptPubKey.SetPayToScriptHash(inner);
            Legacy::Wallet::GetInstance().AddScript(inner);
            Legacy::NexusAddress address;
            address.SetScriptHash256(scriptHash);

            Legacy::Wallet::GetInstance().GetAddressBook().SetAddressBookName(address, strAccount);
            return address.ToString();
        }


        struct tallyitem
        {
            int64_t nAmount;
            int nConf;
            tallyitem()
            {
                nAmount = 0;
                nConf = std::numeric_limits<int>::max();
            }
        };

        json::json ListReceived(const json::json& params, bool fByAccounts)
        {
            // Minimum confirmations
            int nMinDepth = 1;
            if(params.size() > 0)
                nMinDepth = params[0];

            // Whether to include empty accounts
            bool fIncludeEmpty = false;
            if(params.size() > 1)
                fIncludeEmpty = params[1];

            // Tally
            std::map<Legacy::NexusAddress, tallyitem> mapTally;
            for(const auto& entry : Legacy::Wallet::GetInstance().mapWallet)
            {
                const Legacy::WalletTx& wtx = entry.second;

                if(wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
                    continue;

                int nDepth = wtx.GetDepthInMainChain();
                if(nDepth < nMinDepth)
                    continue;

                for(const Legacy::TxOut& txout : wtx.vout)
                {
                    Legacy::NexusAddress address;
                    if(!ExtractAddress(txout.scriptPubKey, address) || !Legacy::Wallet::GetInstance().HaveKey(address) || !address.IsValid())
                        continue;

                    tallyitem& item = mapTally[address];
                    item.nAmount += txout.nValue;
                    item.nConf = std::min(item.nConf, nDepth);
                }
            }

            // Reply
            json::json ret = json::json::array();
            std::map<std::string, tallyitem> mapAccountTally;
            for(const auto& item : Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap())
            {
                const Legacy::NexusAddress& address = item.first;
                const std::string& strAccount = item.second;
                std::map<Legacy::NexusAddress, tallyitem>::iterator it = mapTally.find(address);
                if(it == mapTally.end() && !fIncludeEmpty)
                    continue;

                int64_t nAmount = 0;
                int nConf = std::numeric_limits<int>::max();
                if(it != mapTally.end())
                {
                    nAmount = (*it).second.nAmount;
                    nConf = (*it).second.nConf;
                }

                if(fByAccounts)
                {
                    tallyitem& item = mapAccountTally[strAccount];
                    item.nAmount += nAmount;
                    item.nConf = std::min(item.nConf, nConf);
                }
                else
                {
                    json::json obj;
                    obj["address"] =       address.ToString();
                    obj["account"] =       strAccount;
                    obj["amount"] =        Legacy::SatoshisToAmount(nAmount);
                    obj["confirmations"] = (nConf == std::numeric_limits<int>::max() ? 0 : nConf);
                    ret.push_back(obj);
                }
            }

            if(fByAccounts)
            {
                for(std::map<std::string, tallyitem>::iterator it = mapAccountTally.begin(); it != mapAccountTally.end(); ++it)
                {
                    int64_t nAmount = (*it).second.nAmount;
                    int nConf = (*it).second.nConf;
                    json::json obj;
                    obj["account"] =       (*it).first;
                    obj["amount"] =        Legacy::SatoshisToAmount(nAmount);
                    obj["confirmations"] = (nConf == std::numeric_limits<int>::max() ? 0 : nConf);
                    ret.push_back(obj);
                }
            }

            return ret;
        }

        /* listreceivedbyaddress [minconf=1] [includeempty=false]
        *  [minconf] is the minimum number of confirmations before payments are included.
        *  [includeempty] whether to include addresses that haven't received any payments
        *  Returns an array of objects containing:
        *  \"address\" : receiving address
        *  \"account\" : the account of the receiving address
        *  \"amount\" : total amount received by the address
        *  \"confirmations\" : number of confirmations of the most recent transaction included */
        json::json RPC::ListReceivedByAddress(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() > 2)
                return std::string(
                    "listreceivedbyaddress [minconf=1] [includeempty=false]"
                    " - [minconf] is the minimum number of confirmations before payments are included."
                    " [includeempty] whether to include addresses that haven't received any payments.\n"
                    " - Returns an array of objects containing:\n"
                    "  \"address\" : receiving address\n"
                    "  \"account\" : the account of the receiving address\n"
                    "  \"amount\" : total amount received by the address\n"
                    "  \"confirmations\" : number of confirmations of the most recent transaction included");

             return ListReceived(params, false);

        }

        /* listreceivedbyaccount [minconf=1] [includeempty=false]
        *  [minconf] is the minimum number of confirmations before payments are included.
        *  [includeempty] whether to include accounts that haven't received any payments
        *  Returns an array of objects containing:
        *  \"address\" : receiving address
        *  \"account\" : the account of the receiving address
        *  \"amount\" : total amount received by the address
        *  \"confirmations\" : number of confirmations of the most recent transaction incl*/
        json::json RPC::ListReceivedByAccount(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() > 2)
                return std::string(
                    "listreceivedbyaccount [minconf=1] [includeempty=false]"
                    " - [minconf] is the minimum number of confirmations before payments are included."
                    " [includeempty] whether to include accounts that haven't received any payments.\n"
                    " - Returns an array of objects containing:\n"
                    "  \"account\" : the account of the receiving addresses\n"
                    "  \"amount\" : total amount received by addresses with this account\n"
                    "  \"confirmations\" : number of confirmations of the most recent transaction included");

            return ListReceived(params, true);
        }

        void WalletTxToJSON(const Legacy::WalletTx& wtx, json::json& entry)
        {
            int confirms = wtx.GetDepthInMainChain();
            entry["confirmations"] = confirms;
            if(confirms)
            {
                entry["blockhash"] = wtx.hashBlock.GetHex();
                entry["blockindex"] = wtx.nIndex;
            }
            entry["txid"] = wtx.GetHash().GetHex();
            entry["time"] = (int64_t)wtx.GetTxTime();
            for(const auto& item : wtx.mapValue)
                entry[item.first] = item.second;
        }

        void ListTransactionsJSON(const Legacy::WalletTx& wtx, const std::string& strAccount, int nMinDepth, bool fLong, json::json& ret)
        {
            int64_t nGeneratedImmature, nGeneratedMature, nFee;
            std::string strSentAccount;
            std::list<std::pair<Legacy::NexusAddress, int64_t> > listReceived;
            std::list<std::pair<Legacy::NexusAddress, int64_t> > listSent;

            wtx.GetAmounts(nGeneratedImmature, nGeneratedMature, listReceived, listSent, nFee, strSentAccount);

            bool fAllAccounts = (strAccount == std::string("*"));

            // Generated blocks assigned to account ""
            if((nGeneratedMature+nGeneratedImmature) != 0 && (fAllAccounts || strAccount == ""))
            {
                json::json entry;
                entry["account"] = std::string("");
                if(nGeneratedImmature)
                {
                    entry["category"] = wtx.GetDepthInMainChain() ? "immature" : "orphan";
                    entry["amount"] = Legacy::SatoshisToAmount(nGeneratedImmature);
                }
                else if(wtx.IsCoinStake())
                {
                    entry["category"] = "stake";
                    entry["amount"] = Legacy::SatoshisToAmount(nGeneratedMature);
                }
                else
                {
                    entry["category"] = "generate";
                    entry["amount"] = Legacy::SatoshisToAmount(nGeneratedMature);
                }
                if(fLong)
                    WalletTxToJSON(wtx, entry);
                ret.push_back(entry);
            }

            std::map<Legacy::NexusAddress, std::string> mapExclude;
            for(auto r : listReceived)
            {
                /* Set default address string. */
                std::string account = "";
                if(Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().count(r.first))
                    account = Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().at(r.first);

                /* Catch for blank being default. */
                if(!config::GetBoolArg("-legacy"))
                {
                    if(account == "" || account == "*")
                        account = "default";
                }
                else
                {
                    if(strSentAccount == "default")
                        strSentAccount = "";

                    if(account == "default" || account == "*")
                        account = "";
                }

                /* Check if there are change transactions. */
                if(strSentAccount == account)
                {
                    auto result = std::find(listSent.begin(), listSent.end(), r);
                    if(result != listSent.end())
                        mapExclude[r.first] = account;
                }
            }

            // Sent
            if((!listSent.empty() || nFee != 0) && (fAllAccounts || strAccount == strSentAccount))
            {
                for(const auto& s : listSent)
                {
                    if(mapExclude.count(s.first))
                        continue;

                    if(config::GetBoolArg("-legacy") && strSentAccount == "default")
                        strSentAccount = "";

                    json::json entry;
                    entry["account"] = strSentAccount;
                    entry["address"] = s.first.ToString();
                    entry["category"] = "send";
                    entry["amount"] = Legacy::SatoshisToAmount(-s.second);
                    entry["fee"] = Legacy::SatoshisToAmount(-nFee);
                    if(fLong)
                        WalletTxToJSON(wtx, entry);
                    ret.push_back(entry);
                }
            }

            // Received
            if(listReceived.size() > 0 && wtx.GetDepthInMainChain() >= nMinDepth)
            {
                for(const auto& r : listReceived)
                {
                    if(mapExclude.count(r.first))
                        continue;

                    std::string account;
                    if(Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().count(r.first))
                        account = Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().at(r.first);

                    if(account == "" || account == "*")
                        account = "default";

                    if(strAccount == "" && account == "default")
                        account = "";

                    if(fAllAccounts || (account == strAccount))
                    {
                        if(config::GetBoolArg("-legacy") && account == "default")
                            account = "";

                        json::json entry;
                        entry["account"] = account;
                        entry["address"] = r.first.ToString();
                        entry["category"] = "receive";
                        entry["amount"] = Legacy::SatoshisToAmount(r.second);
                        if(fLong)
                            WalletTxToJSON(wtx, entry);
                        ret.push_back(entry);
                    }
                }
            }
        }

        /* listtransactions [account] [count=10] [from=0]
        Returns up to [count] most recent transactions skipping the first [from] transactions for account [account]*/
        json::json RPC::ListTransactions(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() > 3)
                return std::string(
                    "listtransactions [account] [count=10] [from=0]"
                    " - Returns up to [count] most recent transactions skipping the first [from] transactions for account [account].");

            std::string strAccount = "*";
            if(params.size() > 0)
                strAccount = params[0].get<std::string>();
            int nCount = 10;
            if(params.size() > 1)
                nCount = params[1];
            int nFrom = 0;
            if(params.size() > 2)
                nFrom = params[2];

            if(nCount < 0)
                throw APIException(-8, "Negative count");
            if(nFrom < 0)
                throw APIException(-8, "Negative from");

            json::json ret = json::json::array();
            Legacy::WalletDB walletdb(Legacy::Wallet::GetInstance().GetWalletFile());

            // First: get all Legacy::WalletTx and Wallet::AccountingEntry into a sorted-by-time multimap.
            typedef std::multimap<int64_t, const Legacy::WalletTx* > TxItems;
            TxItems txByTime;

            // Note: maintaining indices in the database of (account,time) --> txid and (account, time) --> acentry
            // would make this much faster for applications that do this a lot.
            for(const auto& entry : Legacy::Wallet::GetInstance().mapWallet)
            {
                const Legacy::WalletTx* wtx = &(entry.second);
                txByTime.insert(std::make_pair(wtx->GetTxTime(), wtx));
            }

            // iterate backwards until we have nCount items to return:
            for(TxItems::reverse_iterator it = txByTime.rbegin(); it != txByTime.rend(); ++it)
            {
                const Legacy::WalletTx* pwtx = (*it).second;
                if(pwtx != 0)
                    ListTransactionsJSON(*pwtx, strAccount, 0, true, ret);

                if(ret.size() >= (nCount+nFrom)) break;
            }
            // ret is newest to oldest

            if(nFrom > (int)ret.size())
                nFrom = ret.size();
            if((nFrom + nCount) > (int)ret.size())
                nCount = ret.size() - nFrom;
            auto first = ret.begin();
            std::advance(first, nFrom);
            auto last = ret.begin();
            std::advance(last, nFrom+nCount);

            if(last != ret.end()) ret.erase(last, ret.end());
            if(first != ret.begin()) ret.erase(ret.begin(), first);

            std::reverse(ret.begin(), ret.end()); // Return oldest to newest

            return ret;
        }


        /* listaddresses [max=100]
        Returns list of addresses */
        json::json RPC::ListAddresses(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 0)
                return std::string(
                    "listaddresses [max=100]"
                    " - Returns list of addresses");

            /* Limit the size. */
            int nMax = 100;
            if(params.size() > 0)
                nMax = params[0];

            /* Get the available addresses from the wallet */
            std::map<Legacy::NexusAddress, int64_t> mapAddresses;
            if(!Legacy::Wallet::GetInstance().GetAddressBook().AvailableAddresses((uint32_t)runtime::unifiedtimestamp(), mapAddresses))
                throw APIException(-3, "Error Extracting the Addresses from Wallet File. Please Try Again.");

            /* Find all the addresses in the list */
            json::json list;
            for(std::map<Legacy::NexusAddress, int64_t>::iterator it = mapAddresses.begin(); it != mapAddresses.end() && list.size() < nMax; ++it)
                list[it->first.ToString()] = Legacy::SatoshisToAmount(it->second);

            return list;
        }

        /* listaccounts
        Returns Object that has account names as keys, account balances as values */
        json::json RPC::ListAccounts(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() > 0)
                return std::string(
                    "listaccounts"
                    " - Returns Object that has account names as keys, account balances as values.");

            std::map<std::string, int64_t> mapAccountBalances;
            for(const auto& entry : Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap())
            {
                if(Legacy::Wallet::GetInstance().HaveKey(entry.first)) // This address belongs to me
                {
                    if(entry.second == "" || entry.second == "default")
                    {
                        if(config::GetBoolArg("-legacy"))
                            mapAccountBalances[""] = 0;
                        else
                            mapAccountBalances["default"] = 0;
                    }
                    else
                        mapAccountBalances[entry.second] = 0;
                }
            }

            /* Get the available addresses from the wallet */
            std::map<Legacy::NexusAddress, int64_t> mapAddresses;
            if(!Legacy::Wallet::GetInstance().GetAddressBook().AvailableAddresses((uint32_t)runtime::unifiedtimestamp(), mapAddresses))
                throw APIException(-3, "Error Extracting the Addresses from Wallet File. Please Try Again.");

            /* Find all the addresses in the list */
            for(const auto& entry : mapAddresses)
            {
                if(Legacy::Wallet::GetInstance().GetAddressBook().HasAddress(entry.first))
                {
                    std::string strAccount = Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().at(entry.first);
                    if(strAccount == "")
                        strAccount = "default";

                    if(config::GetBoolArg("-legacy") && strAccount == "default")
                        strAccount = "";

                    mapAccountBalances[strAccount] += entry.second;
                }
                else
                {
                    if(config::GetBoolArg("-legacy"))
                        mapAccountBalances[""] += entry.second;
                    else
                        mapAccountBalances["default"] += entry.second;
                }
            }

            json::json ret;
            for(const auto& accountBalance :  mapAccountBalances)
            {
                ret[accountBalance.first] = Legacy::SatoshisToAmount(accountBalance.second);
            }

            return ret;
        }

        /* listsinceblock [blockhash] [target-confirmations]
        Get all transactions in blocks since block [blockhash], or all transactions if omitted **/
        json::json RPC::ListSinceBlock(const json::json& params, bool fHelp)
        {
            if(fHelp)
                return std::string(
                    "listsinceblock [blockhash] [target-confirmations]"
                    " - Get all transactions in blocks since block [blockhash], or all transactions if omitted");

            TAO::Ledger::BlockState block;
            int target_confirms = 1;
            uint32_t nBlockHeight = 0;
            if(params.size() > 0)
            {
                uint1024_t blockId = 0;

                blockId.SetHex(params[0].get<std::string>());

                if(!LLD::Ledger->ReadBlock(blockId, block))
                {
                    throw APIException(-1, "Unknown blockhash parameter");
                    return "";
                }

                nBlockHeight = block.nHeight;
            }

            if(params.size() > 1)
            {
                target_confirms = params[1];

                if(target_confirms < 1)
                    throw APIException(-8, "Invalid parameter");
            }

            int32_t depth = nBlockHeight ? (1 + TAO::Ledger::ChainState::nBestHeight.load() - nBlockHeight) : -1;

            json::json transactions = json::json::array();

            for(const auto& entry : Legacy::Wallet::GetInstance().mapWallet)
            {
                Legacy::WalletTx tx = entry.second;

                if(depth == -1 || tx.GetDepthInMainChain() < depth)
                    ListTransactionsJSON(tx, "*", 0, true, transactions);
            }


            json::json ret;
            ret["transactions"] = transactions;

            return ret;
        }

        /* gettransaction <txid>
        Get detailed information about <txid> */
        json::json RPC::GetTransaction(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 1)
                return std::string(
                    "gettransaction <txid>"
                    " - Get detailed information about <txid>");

            uint512_t hash;
            hash.SetHex(params[0].get<std::string>());

            json::json ret;

            if(!Legacy::Wallet::GetInstance().mapWallet.count(hash))
                throw APIException(-5, "Invalid or non-wallet transaction id");
            const Legacy::WalletTx& wtx = Legacy::Wallet::GetInstance().mapWallet[hash];

            int64_t nCredit = wtx.GetCredit();
            int64_t nDebit = wtx.GetDebit();
            int64_t nNet = nCredit - nDebit;

            debug::log(0, FUNCTION, "credit ", nCredit, " debut ", nDebit, " net ", nNet);
            int64_t nFee = (wtx.IsFromMe() ? wtx.GetValueOut() - nDebit : 0);

            ret["amount"] = Legacy::SatoshisToAmount(nNet - nFee);
            if(wtx.IsFromMe())
                ret["fee"] = Legacy::SatoshisToAmount(nFee);

            WalletTxToJSON(Legacy::Wallet::GetInstance().mapWallet[hash], ret);

            json::json details = json::json::array();
            ListTransactionsJSON(Legacy::Wallet::GetInstance().mapWallet[hash], "*", 0, false, details);
            ret["details"] = details;

            return ret;
        }

        /* getrawtransaction <txid>
        Returns a std::string that is serialized, hex-encoded data for <txid> */
        json::json RPC::GetRawTransaction(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 1)
                return std::string(
                    "getrawtransaction <txid>"
                    " - Returns a std::string that is serialized,"
                    " hex-encoded data for <txid>.");

            uint512_t hash;
            hash.SetHex(params[0].get<std::string>());

            Legacy::Transaction tx;
            if(!TAO::Ledger::mempool.Get(hash, tx))
            {
                if(!LLD::Legacy->ReadTx(hash, tx))
                    throw APIException(-5, "No information available about transaction");
            }

            DataStream ssTx(SER_NETWORK, LLP::PROTOCOL_VERSION);
            ssTx << tx;
            return HexStr(ssTx.begin(), ssTx.end());
        }

        /* sendrawtransaction <hex std::string> [checkinputs=0]
        Submits raw transaction (serialized, hex-encoded) to local node and network.
        If checkinputs is non-zero, checks the validity of the inputs of the transaction before sending it */
        json::json RPC::SendRawTransaction(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() < 1 || params.size() > 2)
                return std::string(
                    "sendrawtransaction <hex std::string> [checkinputs=0]"
                    " - Submits raw transaction (serialized, hex-encoded) to local node and network."
                    " If checkinputs is non-zero, checks the validity of the inputs of the transaction before sending it.");

        //     // parse hex std::string from parameter
        //     std::vector<unsigned char> txData(ParseHex(params[0].get<std::string>()));
        //     DataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
        //     bool fCheckInputs = false;
        //     if(params.size() > 1)
        //         fCheckInputs = (params[1] != 0);
        //     Core::CTransaction tx;

        //     // deserialize binary data stream
        //     try {
        //         ssData >> tx;
        //     }
        //     catch (const std::exception &e) {
        //         throw APIException(-22, "TX decode failed");
        //     }
        //     uint512 hashTx = tx.GetHash();

        //     // See if the transaction is already in a block
        //     // or in the memory pool:
        //     Core::CTransaction existingTx;
        //     uint1024 hashBlock = 0;
        //     if(Core::GetTransaction(hashTx, existingTx, hashBlock))
        //     {
        //         if(hashBlock != 0)
        //             throw APIException(-5, std::string("transaction already in block ")+hashBlock.GetHex());
        //         // Not in block, but already in the memory pool; will drop
        //         // through to re-relay it.
        //     }
        //     else
        //     {
        //         // push to local node
        //         LLD::CIndexDB txdb("r");
        //         if(!tx.AcceptToMemoryPool(txdb, fCheckInputs))
        //             throw APIException(-22, "TX rejected");

        //         SyncWithWallets(tx, NULL, true);
        //     }
        //     RelayMessage(CInv(MSG_TX_LEGACY, hashTx), tx);

        //     return hashTx.GetHex();
            json::json ret;
            ret = "NOT AVAILABLE IN THIS RELEASE";
            return ret;
        }

        /* validateaddress <Nexusaddress>
        Return information about <Nexusaddress> */
        json::json RPC::ValidateAddress(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 1)
                return std::string(
                    "validateaddress <Nexusaddress>"
                    " - Return information about <Nexusaddress>.");

            Legacy::NexusAddress address(params[0].get<std::string>());
            bool isValid = address.IsValid();

            json::json ret;
            ret["isvalid"] = isValid;
            if(isValid)
            {
                std::string currentAddress = address.ToString();
                ret["address"] = currentAddress;
                if(Legacy::Wallet::GetInstance().HaveKey(address))
                {
                    ret["ismine"] = true;
                    std::vector<unsigned char> vchPubKey;
                    Legacy::Wallet::GetInstance().GetPubKey(address, vchPubKey);
                    ret["pubkey"] = HexStr(vchPubKey);
                    LLC::ECKey key;
                    key.SetPubKey(vchPubKey);
                    ret["iscompressed"] = key.IsCompressed();
                }
                else if(Legacy::Wallet::GetInstance().HaveScript(address.GetHash256()))
                {
                    ret["isscript"] = true;
                    Legacy::Script subscript;
                    Legacy::Wallet::GetInstance().GetScript(address.GetHash256(), subscript);
                    ret["ismine"] = Legacy::IsMine(Legacy::Wallet::GetInstance(), subscript);
                    std::vector<Legacy::NexusAddress> addresses;
                    Legacy::TransactionType whichType;
                    int nRequired;
                    Legacy::ExtractAddresses(subscript, whichType, addresses, nRequired);
                    ret["script"] = Legacy::GetTxnOutputType(whichType);
                    json::json addressesJSON = json::json::array();
                    for(const Legacy::NexusAddress& addr : addresses)
                        addressesJSON.push_back(addr.ToString());
                    ret["addresses"] = addressesJSON;
                    if(whichType == Legacy::TX_MULTISIG)
                        ret["sigsrequired"] = nRequired;
                }
                else
                    ret["ismine"] = false;
                if(Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().count(address))
                    ret["account"] = Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().at(address);
            }
            return ret;

        }

        /* Make a public/private key pair. [prefix] is optional preferred prefix for the public key */
        json::json RPC::MakeKeyPair(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() > 1)
                return std::string(
                    "makekeypair [prefix]"
                    " - Make a public/private key pair."
                    " [prefix] is optional preferred prefix for the public key.");

            std::string strPrefix = "";
            if(params.size() > 0)
                strPrefix = params[0].get<std::string>();

            LLC::ECKey key;
            uint32_t nCount = 0;
            do
            {
                key.MakeNewKey(false);
                ++nCount;
            } while(nCount < 10000 && strPrefix != HexStr(key.GetPubKey()).substr(0, strPrefix.size()));

            if(strPrefix != HexStr(key.GetPubKey()).substr(0, strPrefix.size()))
                return "";

            LLC::CPrivKey vchPrivKey = key.GetPrivKey();
            json::json result;
            result["PrivateKey"] = HexStr<LLC::CPrivKey::iterator>(vchPrivKey.begin(), vchPrivKey.end());
            result["PublicKey"] = HexStr(key.GetPubKey());
            return result;

        }

        /* unspentbalance [\"address\",...]
        Returns the total amount of unspent Nexus for given address. This is a more accurate command than Get Balance */
        /* TODO: Account balance based on unspent outputs in the core.
            This will be good to determine the actual balance based on the registry of the current addresses
            Associated with the specific users. This needs to be fixed properly so thaat the wallet accounting
            is done properly.

            Second TODO: While at this the wallet core code needs to be reworked in orcder to mitigate the issue of
            having a large number of transactions in the actual memory map which can slow the entire process down. */
        json::json RPC::UnspentBalance(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() > 1)
                return std::string(
                    "unspentbalance [\"address\",...]"
                    " - Returns the total amount of unspent Nexus for given address."
                    " This is a more accurate command than Get Balance.");

            std::set<Legacy::NexusAddress> setAddresses;
            if(params.size() > 0)
            {
                for(uint32_t i = 0; i < params.size(); ++i)
                {
                    Legacy::NexusAddress address(params[i].get<std::string>());
                    if(!address.IsValid())
                    {
                        throw APIException(-5, std::string("Invalid Nexus address: ")+params[i].get<std::string>());
                    }
                    if(setAddresses.count(address))
                    {
                        throw APIException(-8, std::string("Invalid parameter, duplicated address: ")+params[i].get<std::string>());
                    }
                   setAddresses.insert(address);
                }
            }

            std::vector<Legacy::Output> vecOutputs;
            Legacy::Wallet::GetInstance().AvailableCoins((unsigned int)runtime::unifiedtimestamp(), vecOutputs, false);

            int64_t nCredit = 0;
            for(const Legacy::Output& out : vecOutputs)
            {
                if(setAddresses.size())
                {
                    Legacy::NexusAddress address;
                    if(!ExtractAddress(out.walletTx.vout[out.i].scriptPubKey, address))
                        continue;

                    if(!setAddresses.count(address))
                        continue;
                }

                int64_t nValue = out.walletTx.vout[out.i].nValue;
                const Legacy::Script& pk = out.walletTx.vout[out.i].scriptPubKey;
                Legacy::NexusAddress address;

                debug::log(4, "txid ", out.walletTx.GetHash().GetHex(), " | ");
                debug::log(4, "vout ", out.i, " | ");

                if(Legacy::ExtractAddress(pk, address))
                    debug::log(4, "address ", address.ToString(), " |");

                debug::log(4, "amount ", Legacy::SatoshisToAmount(nValue), " |");
                debug::log(4, "confirmations ",out.nDepth);

                nCredit += nValue;
            }

            return Legacy::SatoshisToAmount(nCredit);

        }


        /* listunspent [minconf=1] [maxconf=9999999]  [\"address\",...]
        Returns array of unspent transaction outputs with between minconf and maxconf (inclusive) confirmations.
        Optionally filtered to only include txouts paid to specified addresses.
        Results are an array of Objects, each of which has:
        {txid, vout, scriptPubKey, amount, confirmations} */
        json::json RPC::ListUnspent(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() > 3)
                return std::string(
                    "listunspent [minconf=1] [maxconf=9999999]  [\"address\",...]"
                    " - Returns array of unspent transaction outputs"
                    "with between minconf and maxconf (inclusive) confirmations."
                    " Optionally filtered to only include txouts paid to specified addresses.\n"
                    " - Results are an array of Objects, each of which has:"
                    "{txid, vout, scriptPubKey, amount, confirmations}");

            int nMinDepth = 1;
            if(params.size() > 0)
                nMinDepth = params[0];

            int nMaxDepth = 9999999;
            if(params.size() > 1)
                nMaxDepth = params[1];

            std::set<Legacy::NexusAddress> setAddress;
            if(params.size() > 2)
            {
                if(!params[2].is_array())
                    throw APIException(-8, std::string("Invalid address array format"));

                json::json inputs = params[2];
                for(uint32_t i = 0; i < inputs.size(); ++i)
                {
                    Legacy::NexusAddress address(inputs[i].get<std::string>());
                    if(!address.IsValid())
                        throw APIException(-5, std::string("Invalid Nexus address: ")+inputs[i].get<std::string>());

                    if(setAddress.count(address))
                        throw APIException(-8, std::string("Invalid parameter, duplicated address: ")+inputs[i].get<std::string>());

                   setAddress.insert(address);
                }
            }

            json::json results = json::json::array();
            std::vector<Legacy::Output> vecOutputs;
            Legacy::Wallet::GetInstance().AvailableCoins((uint32_t)runtime::unifiedtimestamp(), vecOutputs, false);
            for(const Legacy::Output& out : vecOutputs)
            {
                if(out.nDepth < nMinDepth || out.nDepth > nMaxDepth)
                    continue;

                if(!setAddress.empty())
                {
                    Legacy::NexusAddress address;
                    if(!ExtractAddress(out.walletTx.vout[out.i].scriptPubKey, address))
                        continue;

                    if(!setAddress.count(address))
                        continue;
                }

                int64_t nValue = out.walletTx.vout[out.i].nValue;
                const Legacy::Script& pk = out.walletTx.vout[out.i].scriptPubKey;
                Legacy::NexusAddress address;
                json::json entry;
                entry["txid"] = out.walletTx.GetHash().GetHex();
                entry["vout"] = out.i;
                if(Legacy::ExtractAddress(pk, address))
                {
                    entry["address"] = address.ToString();
                    if(Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().count(address))
                        entry["account"] = Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap().at(address);
                }
                entry["scriptPubKey"] = HexStr(pk.begin(), pk.end());
                entry["amount"] = Legacy::SatoshisToAmount(nValue);
                entry["confirmations"] = out.nDepth;

                results.push_back(entry);
            }

            return results;

        }

    }
}
