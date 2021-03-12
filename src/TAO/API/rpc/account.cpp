/*__________________________________________________________________________________________

        (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

        (c) Copyright The Nexus Developers 2014 - 2019

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/rpc/types/rpc.h>

#include <Legacy/include/constants.h>

#include <Legacy/include/money.h>
#include <Legacy/include/evaluate.h>
#include <Legacy/types/script.h>

#include <Legacy/types/output.h>
#include <Legacy/types/reservekey.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>

#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <LLP/include/version.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/include/json.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/mempool.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/unpack.h>
#include <TAO/Register/types/address.h>

#include <Util/include/allocators.h>
#include <Util/include/base64.h>
#include <Util/include/hex.h>
#include <Util/include/json.h>

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
            /* Check to see if the caller has specified a token / token_name */
            json::json jsonCreate;
            if(parse_token(params, jsonCreate))
            {
                if(fHelp || params.size() > 2)
                    return std::string(
                        "getnewaddress <token=<address> or token_name=<name>> [pin]"
                        " - Returns a new Nexus address for receiving payments.  "
                        "\n - <token> <token_name> the token or token name to create the address for"
                        "\n - [pin] the optional pin to use. ");

                /* Add the optional pin if passed in*/
                if(params.size() == 2)
                    jsonCreate["pin"] = params[1];
                else
                {
                    /* Otherwise attempt to get the pin from session */
                    SecureString strPIN = TAO::API::users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);
                    jsonCreate["pin"] = strPIN.c_str();
                }

                json::json jsonAccount = TAO::API::finance->Create(jsonCreate, false);
                return jsonAccount["address"];
            }
            else
            {
                if(fHelp || params.size() > 1)
                return std::string(
                    "getnewaddress [account]"
                    " - Returns a new Nexus address for receiving payments.  "
                    "If [account] is specified (recommended), it is added to the address book "
                    "so payments received with the address will be credited to [account].");
            

                Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();


                // Parse the account first so we don't generate a key if there's an error
                std::string strAccount = "default";
                if(params.size() > 0)
                    strAccount = params[0];

                if(!wallet.IsLocked())
                    wallet.GetKeyPool().TopUpKeyPool();

                // Generate a new key that is added to wallet
                std::vector<unsigned char> newKey;
                if(!wallet.GetKeyPool().GetKeyFromPool(newKey, false))
                    throw APIException(-12, "Error: Keypool ran out, please call keypoolrefill first");
                    
                Legacy::NexusAddress address(newKey);
                wallet.GetAddressBook().SetAddressBookName(address, strAccount);

                return address.ToString();
            }
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
            Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();

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
            if(wallet.GetAddressBook().HasAddress(address))
            {
                std::string strOldAccount = wallet.GetAddressBook().GetAddressBookMap().at(address);
                if(address == wallet.GetAddressBook().GetAccountAddress(strOldAccount))
                    wallet.GetAddressBook().GetAccountAddress(strOldAccount, true);
            }

            wallet.GetAddressBook().SetAddressBookName(address, strAccount);

            json::json ret;
            return ret;
        }

        /* getaccount <Nexusaddress>
        Returns the account associated with the given address */
        json::json RPC::GetAccount(const json::json& params, bool fHelp)
        {
            Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();

            if(fHelp || params.size() != 1)
                return std::string(
                    "getaccount <Nexusaddress>"
                    " - Returns the account associated with the given address.");

            Legacy::NexusAddress address(params[0].get<std::string>());
            if(!address.IsValid())
                throw APIException(-5, "Invalid Nexus address");

            std::string strAccount;
            std::map<Legacy::NexusAddress, std::string>::const_iterator mi = wallet.GetAddressBook().GetAddressBookMap().find(address);
            if(mi != wallet.GetAddressBook().GetAddressBookMap().end() && !(*mi).second.empty())
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



        /* sendtoaddress <Nexusaddress> <amount> [comment] [comment-to] [passphrase]
        *  - <amount> is a real and is rounded to the nearest 0.000001
        *  - requires wallet unlocked or [passphrase] provided
        *  - [passphrase] temporarily unlocks wallet for send operation only */
        json::json RPC::SendToAddress(const json::json& params, bool fHelp)
        {
            Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();

            /* Help for encrypted wallet */
            if(wallet.IsCrypted() && (fHelp || params.size() < 2 || params.size() > 5))
                return std::string(
                    "sendtoaddress <Nexusaddress> <amount> [comment] [comment-to] [passphrase]"
                    "\n - <amount> is a real and is rounded to the nearest 0.000001"
                    "\n - requires wallet unlocked or [passphrase] provided"
                    "\n - [passphrase] temporarily unlocks wallet for send operation only");

            /* Help for unencrypted wallet */
            if(!wallet.IsCrypted() && (fHelp || params.size() < 2 || params.size() > 4))
                return std::string(
                    "sendtoaddress <Nexusaddress> <amount> [comment] [comment-to]"
                    "\n - <amount> is a real and is rounded to the nearest 0.000001");

            /* Extract the address, which will either be a legacy address or a sig chain account address */
            std::string strAddress = params[0].get<std::string>();
            Legacy::NexusAddress address(strAddress);
            TAO::Register::Address hashAccount;

            /* The script to contain the recipient */
            Legacy::Script scriptPubKey;

            /* Decode the address string */
            hashAccount.SetBase58(strAddress);

            /* Check the type */
            if(hashAccount.IsValid() && (hashAccount.IsAccount() || hashAccount.IsTrust()))
            {
                /* Get the account object. */
                TAO::Register::Object account;
                if(!LLD::Register->ReadState(hashAccount, account))
                    throw APIException(-5, "Invalid Nexus address");

                /* Parse the object register. */
                if(!account.Parse())
                    throw APIException(-5, "Invalid Nexus address");

                /* Get the object standard. */
                uint8_t nStandard = account.Standard();

                /* Check the object standard. */
                if(nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                    throw APIException(-126, "Address is not for a NXS account");

                /* Check the account is a NXS account */
                if(account.get<uint256_t>("token") != 0)
                    throw APIException(-126, "Address is not for a NXS account");

                scriptPubKey.SetRegisterAddress(hashAccount);
            }
            else if(hashAccount.IsValid() && hashAccount.IsLegacy())
                scriptPubKey.SetNexusAddress(address);
            else
                throw APIException(-5, "Invalid Nexus address");

            /* Amount */
            int64_t nAmount = Legacy::AmountToSatoshis(params[1]);
            if(nAmount < Legacy::MIN_TXOUT_AMOUNT)
                throw APIException(-101, "Send amount too small");

            /* Wallet comments (allow for empty strings as placeholders when only want to provide passphrase) */
            Legacy::WalletTx wtx;
            if(params.size() > 2 && !params[2].is_null() && params[2].get<std::string>() != "")
                wtx.mapValue["comment"] = params[2].get<std::string>();

            if(params.size() > 3 && !params[3].is_null() && params[3].get<std::string>() != "")
                wtx.mapValue["to"] = params[3].get<std::string>();

            /* Wallet passphrase */
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if(params.size() > 4 && !params[4].is_null() && params[4].get<std::string>() != "")
                strWalletPass = params[4].get<std::string>().c_str();

            /* Save the current lock state of wallet */
            bool fLocked = wallet.IsLocked();
            bool fMintOnly = Legacy::fWalletUnlockMintOnly;

            /* Must provide passphrase to send if wallet locked or unlocked for minting only */
            if(wallet.IsCrypted() && (fLocked || fMintOnly))
            {
                if(strWalletPass.length() == 0)
                    throw APIException(-13, "Error: Wallet is locked.");

                /* Unlock returns true if already unlocked, but passphrase must be validated for mint only so must lock first */
                if(fMintOnly)
                {
                    wallet.Lock();
                    Legacy::fWalletUnlockMintOnly = false; //Assures temporary unlock is a full unlock for send
                }

                /* Handle temporary unlock (send false for fStartStake so stake minter does not start during send)
                 * An incorrect passphrase will leave the wallet locked, even if it was previously unlocked for minting.
                 */
                if(!wallet.Unlock(strWalletPass, 0, false))
                    throw APIException(-14, "Error: The wallet passphrase entered was incorrect.");
            }

            std::string strError = wallet.SendToNexusAddress(scriptPubKey, nAmount, wtx);

            /* If used walletpassphrase to temporarily unlock wallet, return to prior state
             * Note that passing 0 for nUnlockSeconds means it will not change or reset any prior unlock time limit
             */
            if(wallet.IsCrypted() && (fLocked || fMintOnly))
            {
                wallet.Lock();

                if(fMintOnly)
                {
                    wallet.Unlock(strWalletPass, 0); //restarts the stake minter

                    Legacy::fWalletUnlockMintOnly = true;
                }
            }

            /* Check result of SendToNexusAddress only after returning to prior lock state */
            if(strError != "")
                throw APIException(-3, strError);

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

        int64_t GetAccountBalance(const std::string& strAccount, int nMinDepth)
        {
            int64_t nBalance = 0;
            Legacy::Wallet::GetInstance().BalanceByAccount(strAccount, nBalance, nMinDepth);

            return nBalance;
        }

        /* getbalance [account] [minconf=1]
        *  If [account] is not specified, returns the server's total available balance.
        *  If [account] is specified, returns the balance in the account */
        json::json RPC::GetBalance(const json::json& params, bool fHelp)
        {
            /* Check to see if the caller has specified a token / token_name */
            json::json jsonRequest;
            if(parse_token(params, jsonRequest))
            {

                if(fHelp || params.size() > 2)
                    return std::string(
                        "getbalance <token=<address> | token_name=<name>> [account] "
                        " - If [account] is not specified, returns the server's total available balance."
                        " If [account] is specified, returns the balance in the account."
                        "\n - <token> <token_name> the token or token name to get the balance for.");

                /* Check to see if an account name or address was specified */
                if(params.size() > 1)
                {
                    std::string strNameOrAddress = params[1].get<std::string>();

                    /* Determine if they provided an account name or address and populate into balance request JSON */
                    if(IsRegisterAddress(strNameOrAddress))
                        jsonRequest["address"] = strNameOrAddress;
                    else
                        jsonRequest["name"] = strNameOrAddress;

                    /* Get the balance for the account */
                    json::json jsonBalance = TAO::API::finance->Get(jsonRequest, false);
                    return jsonBalance["balance"];

                }
                else
                {
                    /* Get the balances for all accounts */
                    json::json jsonBalances = TAO::API::finance->GetBalances(jsonRequest, false);
                    return jsonBalances["available"];
                }
            }
            else
            {
                if(fHelp || params.size() > 2 )
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
                        std::list<std::pair<Legacy::Script, int64_t> > listReceived;
                        std::list<std::pair<Legacy::Script, int64_t> > listSent;
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
        }


        template<typename key, typename value>
        bool Find(const std::map<key, value>& map, const value& search)
        {
            for(const auto& it : map)
                if(it.second == search)
                    return true;

            return false;
        }

        /* move <fromaccount> <toaccount> <amount> [minconf=1] [comment]
        Move from one account in your wallet to another */
        json::json RPC::MoveCmd(const json::json& params, bool fHelp)
        {
            Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();

            if(wallet.IsCrypted() && (fHelp || params.size() < 3 || params.size() > 6))
                return std::string(
                    "move <fromaccount> <toaccount> <amount> [minconf=1] [comment] [passphrase]"
                    "\n - Move from one account in your wallet to another."
                    "\n - <amount> is a real and is rounded to the nearest 0.000001"
                    "\n - <fromaccount> can be a valid account name (including default), or use empty string if no preference"
                    "\n - <toaccount> must be a valid account name"
                    "\n - requires wallet unlocked or [passphrase] provided"
                    "\n - [passphrase] temporarily unlocks wallet for send operation only");

            if(!wallet.IsCrypted() && (fHelp || params.size() < 3 || params.size() > 5))
                return std::string(
                    "move <fromaccount> <toaccount> <amount> [minconf=1] [comment]"
                    "\n - Move from one account in your wallet to another."
                    "\n - <amount> is a real and is rounded to the nearest 0.000001"
                    "\n - <fromaccount> can be a valid account name (including default), or use empty string if no preference"
                    "\n - <toaccount> must be a valid account name");

            /* Account to MoveFrom */
            std::string strFrom = AccountFromValue(params[0]);
            if(strFrom == "")
                strFrom = "*"; //replace empty string with wildcard (to retrieve correct wallet balance)

            if(strFrom != "default" && strFrom != "*" && !Find(wallet.GetAddressBook().GetAddressBookMap(), strFrom))
                throw APIException(-5, debug::safe_printstr(strFrom, " from account doesn't exist."));

            /* Account to MoveTo */
            std::string strTo = AccountFromValue(params[1]);

            if(!Find(wallet.GetAddressBook().GetAddressBookMap(), strTo))
                throw APIException(-5, debug::safe_printstr(strTo, " to account doesn't exist."));

            Legacy::NexusAddress address = wallet.GetAddressBook().GetAccountAddress(strTo);

            /* Amount */
            int64_t nAmount = Legacy::AmountToSatoshis(params[2]);
            if(nAmount < Legacy::MIN_TXOUT_AMOUNT)
                throw APIException(-101, "Send amount too small");

            /* Min number of confirmations for transactions to source NXS */
            uint32_t nMinDepth = 1;
            if(params.size() > 3)
                nMinDepth = params[3];

            /* Wallet comments (allow for empty strings as placeholders when only want to provide passphrase) */
            Legacy::WalletTx wtx;

            if(params.size() > 4 && !params[4].is_null() && params[4].get<std::string>() != "")
                wtx.mapValue["comment"] = params[4].get<std::string>();

            /* Wallet passphrase */
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if(params.size() > 5 && !params[5].is_null() && params[5].get<std::string>() != "")
                strWalletPass = params[5].get<std::string>().c_str();

            /* Save the current lock state of wallet */
            bool fLocked = wallet.IsLocked();
            bool fMintOnly = Legacy::fWalletUnlockMintOnly;

            /* Must provide passphrase to send if wallet locked or unlocked for minting only */
            if(wallet.IsCrypted() && (fLocked || fMintOnly))
            {
                if(strWalletPass.length() == 0)
                    throw APIException(-13, "Error: Wallet is locked.");

                /* Unlock returns true if already unlocked, but passphrase must be validated for mint only so must lock first */
                if(fMintOnly)
                {
                    wallet.Lock();
                    Legacy::fWalletUnlockMintOnly = false; //Assures temporary unlock is a full unlock for send
                }

                /* Handle temporary unlock (send false for fStartStake so stake minter does not start during send)
                 * An incorrect passphrase will leave the wallet locked, even if it was previously unlocked for minting.
                 */
                if(!wallet.Unlock(strWalletPass, 0, false))
                    throw APIException(-14, "Error: The wallet passphrase entered was incorrect.");
            }

            bool fInsufficientBalance = false;

            /* Check the account balance.
             * This calls wallet BalanceByAccount() which does not support "" for strFrom, why it was changed to * above
             */
            int64_t nBalance = GetAccountBalance(strFrom, nMinDepth);
            if(nAmount > nBalance)
                fInsufficientBalance = true;

            /* Assign from account into wtx only when there is a specific one requested, send will use this */
            if(strFrom != "*")
                wtx.strFromAccount = strFrom;

            /* Send */
            std::string strError;
            if(!fInsufficientBalance)
            {
                /* The script to contain the recipient */
                Legacy::Script scriptPubKey;
                scriptPubKey.SetNexusAddress(address);

                strError = wallet.SendToNexusAddress(scriptPubKey, nAmount, wtx, false, nMinDepth);
            }

            /* If used walletpassphrase to temporarily unlock wallet, return to prior state. */
            if(wallet.IsCrypted() && (fLocked || fMintOnly))
            {
                wallet.Lock();

                if(fMintOnly)
                {
                    wallet.Unlock(strWalletPass, 0); //restarts the stake minter

                    Legacy::fWalletUnlockMintOnly = true;
                }
            }

            /* Only throw errors from insufficient balance or SendToNexusAddress after returning to prior lock state */
            if(fInsufficientBalance)
                throw APIException(-6, "Account has insufficient funds");

            if(strError != "")
                throw APIException(-3, strError);

            return wtx.GetHash().GetHex();
        }

        /* sendfrom <fromaccount> <toNexusaddress> <amount> [minconf=1] [comment] [comment-to]
        * <amount> is a real and is rounded to the nearest 0.000001
        * requires wallet passphrase to be set with walletpassphrase first */
        json::json RPC::SendFrom(const json::json& params, bool fHelp)
        {
            /* Check to see if the caller has specified a token / token_name */
            json::json jsonDebitParams;
            if(RPC::parse_token(params, jsonDebitParams))
            {
                if(fHelp || params.size() < 4 || params.size() > 5)
                    return std::string(
                        "sendfrom <token=<address> or token_name=<name>> <fromaccount> <toNexusaddress> <amount> [pin]"
                        "\n - <token> <token_name> the token or token name to send "
                        "\n - <fromaccount> must be a valid tritium account name or address"
                        "\n - <toNexusaddress> the recipient address"
                        "\n - <amount> is a real and is rounded to the nearest 0.000001"
                        "\n - [pin] the optional pin to use. ");

                /* Get the From address */
                std::string strFrom = params[1].get<std::string>();

                if(IsRegisterAddress(strFrom))
                    jsonDebitParams["address"] = strFrom;
                else
                    jsonDebitParams["name"] = strFrom;
                
                /* Add the TO address */
                jsonDebitParams["address_to"] = params[2];

                /* Add the amount */
                jsonDebitParams["amount"] = params[3];

                /* Add the optional pin if passed in*/
                if(params.size() == 5)
                    jsonDebitParams["pin"] = params[4];
                else
                {
                    /* Otherwise attempt to get the pin from session */
                    SecureString strPIN = TAO::API::users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);
                    jsonDebitParams["pin"] = strPIN.c_str();
                }

                /* Invoke the debit API method */
                json::json jsonDebitRS = TAO::API::finance->Debit(jsonDebitParams, false);

                /* Return the transaction ID */
                return jsonDebitRS["txid"].get<std::string>();

            }
            else
            {

                Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();

                if(wallet.IsCrypted() && (fHelp || params.size() < 3 || params.size() > 7))
                    return std::string(
                        "sendfrom <fromaccount> <toNexusaddress> <amount> [minconf=1] [comment] [comment-to] [passphrase]"
                        "\n - <amount> is a real and is rounded to the nearest 0.000001"
                        "\n - <fromaccount> can be a valid account name (including default), or use empty string if no preference"
                        "\n - requires wallet unlocked or [passphrase] provided"
                        "\n - [passphrase] temporarily unlocks wallet for send operation only");

                if(!wallet.IsCrypted() && (fHelp || params.size() < 3 || params.size() > 6))
                    return std::string(
                        "sendfrom <fromaccount> <toNexusaddress> <amount> [minconf=1] [comment] [comment-to]"
                        "\n - <amount> is a real and is rounded to the nearest 0.000001"
                        "\n - <fromaccount> can be a valid account name (including default), or use empty string if no preference");

                /* Account to SendFrom */
                std::string strFrom = AccountFromValue(params[0]);
                if(strFrom == "")
                    strFrom = "*"; //replace empty string with wildcard (to retrieve correct wallet balance)

                if(strFrom != "default" && strFrom != "*" && !Find(wallet.GetAddressBook().GetAddressBookMap(), strFrom))
                    throw APIException(-5, debug::safe_printstr(strFrom, " from account doesn't exist."));

                /* Nexus Address (supports register addresses) */
                std::string strAddress = params[1].get<std::string>();
                Legacy::NexusAddress address(strAddress);
                TAO::Register::Address hashAccount;

                /* The script to contain the recipient */
                Legacy::Script scriptPubKey;

                /* Decode the address string */
                hashAccount.SetBase58(strAddress);

                /* Check the type */
                if(hashAccount.IsValid() && (hashAccount.IsAccount() || hashAccount.IsTrust()))
                {
                    /* Get the account object. */
                    TAO::Register::Object account;
                    if(!LLD::Register->ReadState(hashAccount, account))
                        throw APIException(-5, "Invalid Nexus address");

                    /* Parse the object register. */
                    if(!account.Parse())
                        throw APIException(-5, "Invalid Nexus address");

                    /* Get the object standard. */
                    uint8_t nStandard = account.Standard();

                    /* Check the object standard. */
                    if(nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                        throw APIException(-126, "Address is not for a NXS account");

                    /* Check the account is a NXS account */
                    if(account.get<uint256_t>("token") != 0)
                        throw APIException(-126, "Address is not for a NXS account");

                    scriptPubKey.SetRegisterAddress(hashAccount);
                }
                else if(hashAccount.IsValid() && hashAccount.IsLegacy())
                    scriptPubKey.SetNexusAddress(address);
                else
                    throw APIException(-5, "Invalid Nexus address");

                /* Amount */
                int64_t nAmount = Legacy::AmountToSatoshis(params[2]);
                if(nAmount < Legacy::MIN_TXOUT_AMOUNT)
                    throw APIException(-101, "Send amount too small");

                /* Min number of confirmations for transactions to source NXS */
                uint32_t nMinDepth = 1;
                if(params.size() > 3)
                    nMinDepth = params[3];

                /* Wallet comments (allow for empty strings as placeholders when only want to provide passphrase) */
                Legacy::WalletTx wtx;

                if(params.size() > 4 && !params[4].is_null() && params[4].get<std::string>() != "")
                    wtx.mapValue["comment"] = params[4].get<std::string>();

                if(params.size() > 5 && !params[5].is_null() && params[5].get<std::string>() != "")
                    wtx.mapValue["to"] = params[5].get<std::string>();

                /* Wallet passphrase */
                SecureString strWalletPass;
                strWalletPass.reserve(100);

                if(params.size() > 6 && !params[6].is_null() && params[6].get<std::string>() != "")
                    strWalletPass = params[6].get<std::string>().c_str();

                /* Save the current lock state of wallet */
                bool fLocked = wallet.IsLocked();
                bool fMintOnly = Legacy::fWalletUnlockMintOnly;

                /* Must provide passphrase to send if wallet locked or unlocked for minting only */
                if(wallet.IsCrypted() && (fLocked || fMintOnly))
                {
                    if(strWalletPass.length() == 0)
                        throw APIException(-13, "Error: Wallet is locked.");

                    /* Unlock returns true if already unlocked, but passphrase must be validated for mint only so must lock first */
                    if(fMintOnly)
                    {
                        wallet.Lock();
                        Legacy::fWalletUnlockMintOnly = false; //Assures temporary unlock is a full unlock for send
                    }

                    /* Handle temporary unlock (send false for fStartStake so stake minter does not start during send)
                    * An incorrect passphrase will leave the wallet locked, even if it was previously unlocked for minting.
                    */
                    if(!wallet.Unlock(strWalletPass, 0, false))
                        throw APIException(-14, "Error: The wallet passphrase entered was incorrect.");
                }

                bool fInsufficientBalance = false;

                /* Check funds */
                int64_t nBalance = GetAccountBalance(strFrom, nMinDepth);
                if(nAmount > nBalance)
                    fInsufficientBalance = true;

                /* Assign from account into wtx only when there is a specific one requested, send will use this */
                if(strFrom != "*")
                    wtx.strFromAccount = strFrom;

                /* Send */
                std::string strError;
                if(!fInsufficientBalance)
                    strError = wallet.SendToNexusAddress(scriptPubKey, nAmount, wtx, false, nMinDepth);

                /* If used walletpassphrase to temporarily unlock wallet, return to prior state. */
                if(wallet.IsCrypted() && (fLocked || fMintOnly))
                {
                    wallet.Lock();

                    if(fMintOnly)
                    {
                        wallet.Unlock(strWalletPass, 0); //restarts the stake minter

                        Legacy::fWalletUnlockMintOnly = true;
                    }
                }

                /* Only throw errors from insufficient balance or SendToNexusAddress after returning to prior lock state */
                if(fInsufficientBalance)
                    throw APIException(-6, "Account has insufficient funds");

                if(strError != "")
                    throw APIException(-3, strError);

                return wtx.GetHash().GetHex();
            }

            return "";
        }

        /* sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]
        * - amounts are double-precision floating point numbers
        * requires wallet passphrase to be set with walletpassphrase first*/
        json::json RPC::SendMany(const json::json& params, bool fHelp)
        {
            Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();

            if(wallet.IsCrypted() && (fHelp || params.size() < 2 || params.size() > 5))
                return std::string(
                    "sendmany <fromaccount> {address:amount,...} [minconf=1] [comment] [passphrase]"
                    "\n - amounts are real numbers and are rounded to the nearest 0.000001"
                    "\n - requires wallet unlocked or [passphrase] provided"
                    "\n - [passphrase] temporarily unlocks wallet for send operation only");

            if(!wallet.IsCrypted() && (fHelp || params.size() < 2 || params.size() > 4))
                return std::string(
                    "sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]"
                    "\n - amounts are real numbers and are rounded to the nearest 0.000001");

            /* Account to SendFrom */
            std::string strAccount = AccountFromValue(params[0]);

            /* Recipient list with amounts */
            if(!params[1].is_object())
                throw APIException(-8, std::string("Invalid recipient list format"));

            json::json sendTo = params[1];

            /* Min number of confirmations for transactions to source NXS */
            int nMinDepth = 1;
            if(params.size() > 2)
                nMinDepth = params[2];

            /* Wallet comments (allow for empty strings as placeholders when only want to provide passphrase) */
            Legacy::WalletTx wtx;
            wtx.strFromAccount = strAccount;

            if(params.size() > 3 && !params[3].is_null() && params[3].get<std::string>() != "")
                wtx.mapValue["comment"] = params[3].get<std::string>();

            /* Wallet passphrase */
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if(params.size() > 4 && !params[4].is_null() && params[4].get<std::string>() != "")
                strWalletPass = params[4].get<std::string>().c_str();

            /* Process address/amount list */
            std::set<Legacy::NexusAddress> setAddress;
            std::vector<std::pair<Legacy::Script, int64_t> > vecSend;
            TAO::Register::Address hashAccount;

            int64_t totalAmount = 0;
            for(json::json::iterator it = sendTo.begin(); it != sendTo.end(); ++it)
            {
                Legacy::Script scriptPubKey;
                std::string strAddress = it.key();

                /* Decode the address string */
                hashAccount.SetBase58(strAddress);

                /* handle recipient being a register address */
                if(hashAccount.IsValid() && (hashAccount.IsAccount() || hashAccount.IsTrust()))
                {
                    /* Get the account object. */
                    TAO::Register::Object account;
                    if(!LLD::Register->ReadState(hashAccount, account))
                        throw APIException(-5, "Invalid Nexus address");

                    /* Parse the object register. */
                    if(!account.Parse())
                        throw APIException(-5, "Invalid Nexus address");

                    /* Get the object standard. */
                    uint8_t nStandard = account.Standard();

                    /* Check the object standard. */
                    if(nStandard != TAO::Register::OBJECTS::ACCOUNT && nStandard != TAO::Register::OBJECTS::TRUST)
                        throw APIException(-126, "Address is not for a NXS account");

                    /* Check the account is a NXS account */
                    if(account.get<uint256_t>("token") != 0)
                        throw APIException(-126, "Address is not for a NXS account");

                    scriptPubKey.SetRegisterAddress( hashAccount );
                }
                else
                {
                    Legacy::NexusAddress address(it.key());
                    if(!address.IsValid())
                        throw APIException(-5, std::string("Invalid Nexus address:")+it.key());

                    if(setAddress.count(address))
                        throw APIException(-8, std::string("Invalid parameter, duplicated address: ")+it.key());

                    setAddress.insert(address);

                    scriptPubKey.SetNexusAddress(address);
                }

                int64_t nAmount = Legacy::AmountToSatoshis(it.value());

                if(nAmount < Legacy::MIN_TXOUT_AMOUNT)
                    throw APIException(-101, "Send amount too small");

                totalAmount += nAmount;

                vecSend.push_back(make_pair(scriptPubKey, nAmount));
            }

            /* Save the current lock state of wallet */
            bool fLocked = wallet.IsLocked();
            bool fMintOnly = Legacy::fWalletUnlockMintOnly;

            /* Must provide passphrase to send if wallet locked or unlocked for minting only */
            if(wallet.IsCrypted() && (fLocked || fMintOnly))
            {
                if(strWalletPass.length() == 0)
                    throw APIException(-13, "Error: Wallet is locked.");

                /* Unlock returns true if already unlocked, but passphrase must be validated for mint only so must lock first */
                if(fMintOnly)
                {
                    wallet.Lock();
                    Legacy::fWalletUnlockMintOnly = false; //Assures temporary unlock is a full unlock for send
                }

                /* Handle temporary unlock (send false for fStartStake so stake minter does not start during send)
                 * An incorrect passphrase will leave the wallet locked, even if it was previously unlocked for minting.
                 */
                if(!wallet.Unlock(strWalletPass, 0, false))
                    throw APIException(-14, "Error: The wallet passphrase entered was incorrect.");
            }

            bool fInsufficientBalance = false;
            bool fCreateFailed = false;
            bool fCommitFailed = false;

            /* Check funds */
            int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);
            if(totalAmount > nBalance)
                fInsufficientBalance = true;

            /* Send */
            if(!fInsufficientBalance)
            {
                Legacy::ReserveKey keyChange(wallet);
                int64_t nFeeRequired = 0;

                if(!wallet.CreateTransaction(vecSend, wtx, keyChange, nFeeRequired))
                {
                    if(totalAmount + nFeeRequired > wallet.GetBalance())
                        fInsufficientBalance = true;
                    else
                        fCreateFailed = true;
                }
                else if(!wallet.CommitTransaction(wtx, keyChange))
                    fCommitFailed = true;
            }

            /* If used walletpassphrase to temporarily unlock wallet, return to prior state. */
            if(wallet.IsCrypted() && (fLocked || fMintOnly))
            {
                wallet.Lock();

                if(fMintOnly)
                {
                    wallet.Unlock(strWalletPass, 0); //restarts the stake minter

                    Legacy::fWalletUnlockMintOnly = true;
                }
            }

            /* Only throw errors from insufficient balance or transaction creation after returning to prior lock state */
            if(fInsufficientBalance)
                throw APIException(-6, "Account has insufficient funds");

            if(fCreateFailed)
                throw APIException(-4, "Transaction creation failed");

            if(fCommitFailed)
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
            /* The response JSON */
            json::json ret = json::json::array();

            /* Check to see if the caller has specified a token / token_name */
            json::json token;
            if(RPC::parse_token(params, token))
            {
                // Minimum confirmations
                int nMinDepth = 1;
                if(params.size() > 1)
                    nMinDepth = params[1];

                // Whether to include empty accounts
                bool fIncludeEmpty = false;
                if(params.size() > 2)
                    fIncludeEmpty = params[2];

                /* Get the tritium accounts */
                json::json jsonAccounts = TAO::API::finance->List(token, false);
                
                /* Iterate through all accounts and add their available balance to the map */
                for(const auto& account : jsonAccounts)
                {
                    /* Get the address */
                    std::string strAddress = account["address"].get<std::string>();

                    /* Get the name */
                    std::string strName = account["name"].get<std::string>();

                    /* Get the amount */
                    double dBalance = account["balance"].get<double>();

                    /* Get the unconfirmed */
                    double dUnconfirmed = account["unconfirmed"].get<double>();

                    /* Get the amount to return */
                    double dAmount = nMinDepth > 0 ? dBalance : dBalance + dUnconfirmed;

                    json::json obj;

                    if(fIncludeEmpty || dAmount > 0)
                    {

                        if(!fByAccounts)
                            obj["address"] = strAddress;
                        
                        obj["account"] = strName;

                        obj["amount"] = dAmount;
                        
                        obj["confirmations"] = nMinDepth > 0 ? 1 : 0;

                        ret.push_back(obj);
                    }
                }
            }
            else
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
            /* Check to see if the caller has specified a token / token_name */
            json::json token;
            if(RPC::parse_token(params, token))
            {
                if(fHelp || params.size() > 1)
                    return std::string(
                        "listreceivedbyaddress <token=<address> | token_name=<name>>"
                        "\n - <token> <token_name> the token or token name to get the received amounts for.");
            }
            else
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
            }
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
            /* Check to see if the caller has specified a token / token_name */
            json::json token;
            if(RPC::parse_token(params, token))
            {
                if(fHelp || params.size() > 1)
                    return std::string(
                        "listreceivedbyaddress <token=<address> | token_name=<name>>"
                        "\n - <token> <token_name> the token or token name to get the received amounts for.");
            }
            else
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
            }
            return ListReceived(params, true);
        }

        /* Added ftxValid because wallet data can potentially include invalid transactions that are not on chain.
         * If one is recognized, this flag allows us to still generate output for it. Set the value false and function
         * will generate output as a tx with zero confirms rather than potentially incorrect data.
         */
        void WalletTxToJSON(const Legacy::WalletTx& wtx, json::json& entry, bool ftxValid = true)
        {
            if(!ftxValid)
            {
                entry["confirmations"] = 0;
            }
            else
            {
                int confirms = wtx.GetDepthInMainChain();
                entry["confirmations"] = confirms;

                if(confirms > 0)
                {
                    entry["blockhash"] = wtx.hashBlock.GetHex();
                    entry["blockindex"] = wtx.nIndex;
                }
            }

            /* If this transaction has no inputs but is not a coinbase/coinstake then it must be a pseudo-legacy transation
               from a tritium OP::LEGACY.  In this case the pseudo legacy transaction hash is not the correct one to output
               so we must search for it instead in mapwallet, where the map key is the correct hash*/
            if(wtx.vin.size() == 0 && !wtx.IsCoinBase() && !wtx.IsCoinStake())
            {
                /* use lamda shortcut with find_if to find the entry for this wtx */
                auto it = std::find_if(std::begin(Legacy::Wallet::GetInstance().mapWallet),
                                       std::end(Legacy::Wallet::GetInstance().mapWallet),
                                       [&](const std::pair<uint512_t, Legacy::WalletTx> &p) { return p.second == wtx; });

                if(it != std::end(Legacy::Wallet::GetInstance().mapWallet))
                    entry["txid"] = it->first.GetHex();

            }
            else
                entry["txid"] = wtx.GetHash().GetHex();

            entry["time"] = (int64_t)wtx.GetTxTime();

            for(const auto& item : wtx.mapValue)
                entry[item.first] = item.second;
        }

        void ListTransactionsJSON(const Legacy::WalletTx& wtx, const std::string& strAccount, int nMinDepth, bool fLong, json::json& ret)
        {
            int64_t nGeneratedImmature, nGeneratedMature, nFee;
            std::string strSentAccount;
            std::list<std::pair<Legacy::Script, int64_t> > listReceived;
            std::list<std::pair<Legacy::Script, int64_t> > listSent;

            Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();
            wtx.GetAmounts(nGeneratedImmature, nGeneratedMature, listReceived, listSent, nFee, strSentAccount);

            bool fAllAccounts = (strAccount == std::string("*"));

            // Generated blocks assigned to account ""
            if((nGeneratedMature + nGeneratedImmature) != 0 && (fAllAccounts || strAccount == ""))
            {
                json::json entry;
                entry["account"] = std::string("default");

                /* For coinbase / coinstake transactions we need to extract the address from the first TxOut in vout */
                if(wtx.vout.size() == 0)
                    throw APIException(-8, "Invalid coinbase/coinstake transaction");

                const Legacy::TxOut& txout = wtx.vout[0];
                Legacy::NexusAddress address;
                TAO::Register::Address hashRegister;

                /* Get the Nexus address from the txout public key */
                if(ExtractAddress(txout.scriptPubKey, address))
                    entry["address"] = address.ToString();
                else if(ExtractRegister(txout.scriptPubKey, hashRegister))
                    entry["address"] = hashRegister.ToString();
                else
                {
                    debug::log(0, FUNCTION, "Unknown transaction type found, txid ", wtx.GetHash().ToString());

                    address = "unknown";
                }

                /* The amount to output */
                int64_t nAmount = nGeneratedImmature > 0 ? nGeneratedImmature : nGeneratedMature;

                /* Substract the input amount for stake transactions as the generated amount includes the stake amount */
                if (wtx.IsGenesis() || wtx.IsCoinStake())
                {
                    std::map<uint512_t, std::pair<uint8_t, DataStream>> mapInputs;
                    wtx.FetchInputs(mapInputs);

                    /* If there is are invalid stake transactions saved in wallet file, FetchInputs can fail with logged error.
                     * Thus inputs are unavailable. If we allow it to proceed with GetValueIn, it will throw an exception
                     * and this method will not return anything besides the exception. To handle things more gracefully,
                     * set amount for current tx to zero and set txValid flag to false. This will allow us to still produce
                     * a transaction list that will show the invalid one as "pending" confirms where it can be seen and
                     * dealt with.
                     */
                    if(wtx.FetchInputs(mapInputs))
                        nAmount -= wtx.GetValueIn(mapInputs);
                    else
                        nAmount = 0;
                }

                if (nGeneratedImmature)
                {
                    entry["category"] = wtx.GetDepthInMainChain() ? "immature" : "orphan";
                }
                else if(wtx.IsGenesis() || wtx.IsCoinStake())
                {
                    entry["category"] = wtx.IsGenesis() ? "genesis" : "stake";
                }
                else
                {
                    entry["category"] = "generate";
                }

                /* Add the amount */
                entry["amount"] = Legacy::SatoshisToAmount(nAmount);

                if(fLong)
                    WalletTxToJSON(wtx, entry);

                ret.push_back(entry);
            }

            /* Check the time-lock of new account logic. */
            bool fExclusions = (wtx.nTime < Legacy::WALLET_ACCOUNTING_TIMELOCK);

            /* Transactions sent from wallet will likely include a change output.
             *
             * This output will show up as both sent and received. Sent because it is part of the output of the Send transaction,
             * and received because the output pays funds to the wallet.
             *
             * Filter out this pair of entries from listSent and listReceived to remove them from output.
             */
            std::map<Legacy::Script, int64_t> mapExclude;

            /* Only add EXCLUSIONS with prior algorithm before accounting timelock. */
            if (wtx.IsFromMe() && fExclusions)
            {
                for(auto r : listReceived)
                {
                    auto result = std::find(listSent.begin(), listSent.end(), r);
                    if(result != listSent.end())
                        mapExclude[r.first] = r.second;
                }
            }

            // Sent
            if((!listSent.empty() || nFee != 0) && (fAllAccounts || strAccount == strSentAccount))
            {
                for(const auto& s : listSent)
                {
                    Legacy::NexusAddress address;
                    Legacy::ExtractAddress(s.first, address);

                    TAO::Register::Address hashRegister;
                    Legacy::ExtractRegister(s.first, hashRegister);

                    if(fExclusions && mapExclude.count(s.first))
                        continue;

                    if(config::GetBoolArg("-legacy") && strSentAccount == "default")
                        strSentAccount = "";

                    json::json entry;
                    entry["account"] = strSentAccount;
                    entry["address"] = address.IsValid() ? address.ToString() : hashRegister.ToString(); // handle sending to register address
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
                    Legacy::NexusAddress address;
                    Legacy::ExtractAddress(r.first, address);

                    TAO::Register::Address hashRegister;
                    Legacy::ExtractRegister(r.first, hashRegister);

                    if(fExclusions && mapExclude.count(r.first))
                        continue;

                    std::string account;
                    if(wallet.GetAddressBook().GetAddressBookMap().count(address))
                        account = wallet.GetAddressBook().GetAddressBookMap().at(address);

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
                        entry["address"] = address.IsValid() ? address.ToString() : hashRegister.ToString(); // handle sending to register address
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
            /* Response JSON */
            json::json ret = json::json::array();

            /* Check to see if the caller has specified a token / token_name */
            json::json token;
            bool fToken = parse_token(params, token);

            if(fToken )
            {
                if(fHelp || params.size() > 4)
                    return std::string(
                        "listtransactions <token=<address> | token_name=<name>> [account] [count=10] [from=0]"
                        " - Returns up to [count] most recent transactions skipping the first [from] transactions for account [account]."
                        "\n - <token> <token_name> the token or token name to list transactions for.");
            }
            else
            {
                if(fHelp || params.size() > 3)
                    return std::string(
                        "listtransactions [account] [count=10] [from=0]"
                        " - Returns up to [count] most recent transactions skipping the first [from] transactions for account [account].");
            }
            
            /* Parse the remaining params */
            std::string strAccount = "*";
            if(params.size() > (fToken ? 1 : 0))
                strAccount = params[fToken ? 1 : 0].get<std::string>();
            uint64_t nCount = 10;
            if(params.size() > (fToken ? 2 : 1))
                nCount = params[fToken ? 2 : 1];
            uint64_t nFrom = 0;
            if(params.size() > (fToken ? 3 : 2))
                nFrom = params[fToken ? 3 : 2];

            if(fToken)
            {
                /* Check the account filter, as this is not supported for tritium accounts because the account name is dependent
                   upon the operation as it can be credit to or debit from */
                if(strAccount != "*" && strAccount != "")
                    return "Account filter is not supported for tritium transactions";

                /* Parameters to pass to users/list/transactions */
                json::json jsonListParams;

                /* Add verbosity as we need summary */
                jsonListParams["verbose"] = "summary";

                /* Add where array */
                json::json jsonWhere = json::json::array();

                /* Add token name / address to the where */
                if(token.find("token") != token.end())
                {
                    json::json jsonClause;
                    jsonClause["field"] = "contracts.token";
                    jsonClause["op"] = "=";
                    jsonClause["value"] = token["token"];
                    jsonWhere.push_back(jsonClause);
                }
                else if(token.find("token_name") != token.end())
                {
                    json::json jsonClause;
                    jsonClause["field"] = "contracts.token_name";
                    jsonClause["op"] = "=";
                    jsonClause["value"] = token["token_name"];
                    jsonWhere.push_back(jsonClause);
                }

                if(nCount > 0)
                    jsonListParams["limit"] = nCount;

                if(nFrom > 0)
                    jsonListParams["offset"] = nFrom;

                /* Add Where */
                jsonListParams["where"] = jsonWhere;

                /* Get the transactions JSON */
                json::json jsonTransactions = TAO::API::users->Transactions(jsonListParams, false);

                /* Iterate through all transactions.  We need to write a response transaction for each contract. */
                for(const auto& transaction : jsonTransactions)
                {
                    /* interate contracts */
                    json::json jsonContracts = transaction["contracts"];

                    for(const auto& contract : jsonContracts)
                    {
                        /* The category */
                        std::string strCategory = "";

                        /* Get the operation as we only want credit, debit, fee, legacy, genesis, and trust */
                        std::string strOp = contract["OP"].get<std::string>();

                        if(strOp == "DEBIT" || strOp == "LEGACY")
                            strCategory = "send";
                        else if(strOp == "FEE")
                            strCategory = "fee";
                        else if(strOp == "CREDIT")
                            strCategory = "receive";
                        else if(strOp == "GENESIS")
                            strCategory = "genesis";
                        else if(strOp == "TRUST")
                            strCategory = "stake";
                        else if(strOp == "COINBASE")
                            strCategory = "generate";

                        if(strCategory != "")
                        {
                            json::json jsonTransaction;

                            /* Add the account */
                            std::string strAccountName = "";
                            if(strOp == "CREDIT" && contract.find("to_name") != contract.end())
                                strAccountName = contract["to_name"].get<std::string>();
                            else if((strOp == "DEBIT" || strOp == "FEE") && contract.find("from_name") != contract.end())
                                strAccountName = contract["from_name"].get<std::string>();
                            else if(strOp == "GENESIS" || strOp == "TRUST")
                                strAccountName = "trust";

                            jsonTransaction["account"] = strAccountName;
                            
                            if(strOp == "CREDIT")
                                jsonTransaction["address"] = contract["to"].get<std::string>(); 
                            else if(strOp == "DEBIT" || strOp == "FEE")
                                jsonTransaction["address"] = contract["from"].get<std::string>();
                            else if(strOp == "GENESIS" || strOp == "TRUST")
                                jsonTransaction["address"] = contract["address"].get<std::string>();
                            
                            jsonTransaction["category"] = strCategory;
                            jsonTransaction["amount"] = contract["amount"];
                            jsonTransaction["confirmations"] = transaction["confirmations"];
                            jsonTransaction["txid"] = transaction["txid"];
                            jsonTransaction["time"] = transaction["timestamp"];
                            
                            ret.push_back(jsonTransaction);
                        }
                    }

                }
                
            }
            else
            {
                // First: get all Legacy::WalletTx into a sorted-by-time multimap.
                typedef std::multimap<uint64_t, const Legacy::WalletTx* > TxItems;
                TxItems txByTime;

                // Note: maintaining indices in the database of (account,time) --> txid and (account, time) --> acentry
                // would make this much faster for applications that do this a lot.
                for(const auto& entry : Legacy::Wallet::GetInstance().mapWallet)
                {
                    const Legacy::WalletTx* wtx = &(entry.second);
                    txByTime.insert(std::make_pair(wtx->GetTxTime(), wtx));
                }

                // iterate backwards until we have nCount items to return:
                for(auto it = txByTime.crbegin(); it != txByTime.crend(); ++it)
                {
                    const Legacy::WalletTx* pwtx = (*it).second;
                    if(pwtx != 0)
                        ListTransactionsJSON(*pwtx, strAccount, 0, true, ret);

                    if(ret.size() >= (nCount + nFrom)) break;
                }

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
            }
            return ret;
        }


        /* listaddresses [max=100]
        Returns list of addresses */
        json::json RPC::ListAddresses(const json::json& params, bool fHelp)
        {
            /* response json */
            json::json list;

            /* Check to see if the caller has specified a token / token_name */
            json::json token;
            if(parse_token(params, token))
            {
                if(fHelp || params.size() > 1)
                    return std::string(
                        "listaddresses <token=<address> or token_name=<name>>"
                        " - Returns list of addresses"
                        "\n - <token> <token_name> the token or token name to list addresses for.");

                /* Map of account names to balances */
                std::map<std::string, double> mapAccountBalances;

                /* Get the tritium accounts */
                json::json jsonAccounts = TAO::API::finance->List(token, false);
                
                /* Iterate through all accounts and add their available balance to the map */
                for(const auto& account : jsonAccounts)
                {
                    /* Get the address */
                    std::string strAddress = account["address"].get<std::string>();

                    /* Get the amount */
                    double dAmount = account["balance"].get<double>();

                    /* If this is a trust account, add on the staked amount */
                    if(account.find("stake") != account.end())
                        dAmount += account["stake"].get<double>();

                    mapAccountBalances[strAddress] = dAmount;
                }

                for(const auto& accountBalance :  mapAccountBalances)
                    list[accountBalance.first] = accountBalance.second;
            }
            else
            {
                if(fHelp || params.size() > 0)
                    return std::string(
                        "listaddresses"
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
                for(std::map<Legacy::NexusAddress, int64_t>::iterator it = mapAddresses.begin(); it != mapAddresses.end() && list.size() < nMax; ++it)
                    list[it->first.ToString()] = Legacy::SatoshisToAmount(it->second);
            }

            return list;
        }

        /* listaccounts
        Returns Object that has account names as keys, account balances as values */
        json::json RPC::ListAccounts(const json::json& params, bool fHelp)
        {
            /* response json */
            json::json ret;

            /* Check to see if the caller has specified a token / token_name */
            json::json token;
            if(parse_token(params, token))
            {
                if(fHelp || params.size() > 1)
                    return std::string(
                        "listaccounts <token=<address> or token_name=<name>>"
                        " - Returns Object that has account names as keys, account balances as values."
                        "\n - <token> <token_name> the token or token name to list the accounts for.");

                /* Map of account names to balances */
                std::map<std::string, double> mapAccountBalances;

                /* Get the tritium accounts */
                json::json jsonAccounts = TAO::API::finance->List(token, false);
                
                /* Iterate through all accounts and add their available balance to the map */
                for(const auto& account : jsonAccounts)
                {
                    /* Get the name */
                    std::string strName = account["name"].get<std::string>();

                    /* If the account has no name then use the address instead */
                    if(strName.empty())
                        strName = account["address"].get<std::string>();

                    /* Get the amount */
                    double dAmount = account["balance"].get<double>();

                    /* If this is a trust account, add on the staked amount */
                    if(account.find("stake") != account.end())
                        dAmount += account["stake"].get<double>();

                    mapAccountBalances[strName] = dAmount;
                }

                for(const auto& accountBalance :  mapAccountBalances)
                    ret[accountBalance.first] = accountBalance.second;
            }
            else
            {
                if(fHelp || params.size() > 0)
                    return std::string(
                        "listaccounts"
                        " - Returns Object that has account names as keys, account balances as values.");

                /* Map of account names to balances */
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

                for(const auto& accountBalance :  mapAccountBalances)
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

        /* RPC Method to bridge limitation of Transaction Lookup from Wallet. Allows lookup from any wallet. */
        json::json RPC::GetGlobalTransaction(const json::json& params, bool fHelp)
        {
            if (fHelp || params.size() != 1)
                return std::string(
                    "getglobaltransaction [txid]\n"
                    "Get detailed information about [txid]");

            /* Build return json object. */
            json::json ret;

            /* Get hash Index. */
            uint512_t hash;
            hash.SetHex(params[0].get<std::string>());

            /* Get confirmations. */
            uint32_t nConfirmations = 0;
            TAO::Ledger::BlockState state;
            if(LLD::Ledger->ReadBlock(hash, state))
                /* Calculate confirmations. */
                nConfirmations = (TAO::Ledger::ChainState::stateBest.load().nHeight - state.nHeight) + 1;

            /* Set block hash. */
            if(!state.IsNull())
                ret["blockhash"] = state.GetHash().GetHex();

            /* Set confirmations. */
            ret["confirmations"] = nConfirmations;

            /* Add TX ID */
            ret["txid"]   = hash.GetHex();

            /* Legacy transaction object */
            Legacy::Transaction txLegacy;

            /* Tritium transaction object */
            TAO::Ledger::Transaction txTritium;

            /* Get the transaction object. */
            if(TAO::Ledger::mempool.Get(hash, txLegacy) || LLD::Legacy->ReadTx(hash, txLegacy))
            {
                /* Fill other relevant data. */
                ret["type"]   = txLegacy.TypeString();
                ret["time"]   = txLegacy.nTime;
                ret["amount"] = Legacy::SatoshisToAmount(txLegacy.GetValueOut());

                /* Get the outputs. */
                json::json vOutputs;
                for(const auto& out : txLegacy.vout)
                {
                    Legacy::NexusAddress address;
                    TAO::Register::Address hashRegister;
                    std::string strAddress;

                    /* Get the Nexus address from the txout public key */
                    if(ExtractAddress(out.scriptPubKey, address))
                        strAddress = address.ToString();
                    else if(ExtractRegister(out.scriptPubKey, hashRegister))
                        strAddress = hashRegister.ToString();
                    else
                        throw APIException(-5, "failed to extract output address");

                    vOutputs.push_back(debug::safe_printstr(strAddress, ":", std::fixed, Legacy::SatoshisToAmount(out.nValue)));
                }

                /* Get the inputs. */
                if(!txLegacy.IsCoinBase())
                {
                    /* Get the number of inputs to the transaction. */
                    uint32_t nSize = static_cast<uint32_t>(txLegacy.vin.size());

                    /* Read all of the inputs. */
                    json::json vInputs;
                    for (uint32_t i = (uint32_t)txLegacy.IsCoinStake(); i < nSize; ++i)
                    {
                        /* Skip inputs that are already found. */
                        Legacy::OutPoint prevout = txLegacy.vin[i].prevout;

                        /* Read the previous transaction. This could be a legacy or tritium transaction*/
                        Legacy::Transaction txPrev;
                        TAO::Ledger::Transaction txPrevTritium;
                        if(LLD::Legacy->ReadTx(prevout.hash, txPrev))
                        {
                            /* Extract the address. */
                            Legacy::NexusAddress address;
                            if(!Legacy::ExtractAddress(txPrev.vout[prevout.n].scriptPubKey, address))
                                throw APIException(-5, "failed to extract input address");

                            /* Add inputs to json. */
                            vInputs.push_back(debug::safe_printstr(address.ToString(), ":", std::fixed, Legacy::SatoshisToAmount(txPrev.vout[prevout.n].nValue)));
                        }
                        else if(LLD::Ledger->ReadTx(prevout.hash, txPrevTritium))
                        {

                            /* Iterate through contracts to fill the outputs with all OP::LEGACY contracts */
                            uint32_t nContracts = txPrevTritium.Size();
                            for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                            {
                                /* Check that the contract is an op legacy */
                                if(TAO::Register::Unpack(txPrevTritium[nContract], TAO::Operation::OP::LEGACY))
                                {
                                    /* The register address of the account that made the OP::LEGACY */
                                    TAO::Register::Address hashRegister;

                                    /* Unpack the register address */
                                    TAO::Register::Unpack(txPrevTritium[nContract], hashRegister);

                                    /* Get the amount */
                                    uint64_t nAmount = 0;
                                    TAO::Register::Unpack(txPrevTritium[nContract], nAmount);

                                    /* Add inputs to json. */
                                    vInputs.push_back(debug::safe_printstr(hashRegister.ToString(), ":", std::fixed, Legacy::SatoshisToAmount(nAmount)));

                                    /* We found our op legacy so break */
                                    break;
                                }
                            }
                        }
                        else
                            throw APIException(-5, debug::safe_printstr("tx ", prevout.hash.SubString(), " not found"));

                    }

                    /* Add to return value. */
                    ret["inputs"] = vInputs;
                }

                /* Add to return value. */
                ret["outputs"] = vOutputs;

            }
            else if(LLD::Ledger->ReadTx(hash, txTritium, TAO::Ledger::FLAGS::MEMPOOL))
            {
                /* Sun of OP::LEGACY contracts for this transaction  */
                uint64_t nTotal = 0;

                /* Get the vOutputs. */
                json::json vOutputs;

                /* Iterate through contracts to fill the outputs with all OP::LEGACY contracts */
                uint32_t nContracts = txTritium.Size();
                for(uint32_t nContract = 0; nContract < nContracts; ++nContract)
                {
                    /* Check that the contract is an op legacy */
                    if(TAO::Register::Unpack(txTritium[nContract], TAO::Operation::OP::LEGACY ))
                    {
                        /* The amount for this op legacy contract */
                        uint64_t nAmount;

                        /* Get the amount */
                        TAO::Register::Unpack(txTritium[nContract], nAmount );

                        /* add to our total */
                        nTotal += nAmount;

                        /* Get the output script from the op legacy */
                        Legacy::Script script;
                        TAO::Register::Unpack(txTritium[nContract], script);

                        /* Get the recipient address from the script*/
                        Legacy::NexusAddress address;
                        Legacy::ExtractAddress(script, address);

                        /* Add this address/amout to the outputs */
                        vOutputs.push_back(debug::safe_printstr(address.ToString(), ":", std::fixed, Legacy::SatoshisToAmount(nAmount)));
                    }

                    /* If we found any op legacy contracts then add the rest of the data */
                    if(vOutputs.size() > 0)
                    {
                        ret["type"]   = txTritium.TypeString();
                        ret["time"]   = txTritium.nTimestamp;
                        ret["amount"] = Legacy::SatoshisToAmount(nTotal);

                        /* Add to return value. */
                        ret["outputs"] = vOutputs;
                    }
                    else
                    {
                        throw APIException(-1, "This is a Tritium transaction.  Please use the Tritium API to retrieve data for this transaction" );
                    }
                }
            }
            else
            {
                throw APIException(-5, "No information available about transaction" );
            }

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

            Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();

            uint512_t hash;
            hash.SetHex(params[0].get<std::string>());

            json::json ret;

            if(!wallet.mapWallet.count(hash))
                throw APIException(-5, "Invalid or non-wallet transaction id");
            const Legacy::WalletTx& wtx = wallet.mapWallet[hash];

            /* Coinstake is "from me" so stake reward ends up in nFee. Need to handle that type of tx separately */
            if(wtx.IsCoinStake())
            {
                int64_t nReward = wtx.GetValueOut() - wtx.GetDebit();
                ret["amount"] = Legacy::SatoshisToAmount(nReward);
            }
            else
            {
                /* Receive tx will have credit for received amount with debit of zero.
                 *
                 * Send tx will have debit for full spent balance with credit for change. Net sent amount is the difference,
                 * which includes the fee paid.
                 *
                 * Fee for sent tx is difference between full value out (amount sent plus change) and the full spent balance.
                 */
                int64_t nCredit = wtx.GetCredit();
                int64_t nDebit = wtx.GetDebit();
                int64_t nNet = nCredit - nDebit;  //includes fee

                int64_t nFee = (wtx.IsFromMe() ? wtx.GetValueOut() - nDebit : 0);

                ret["amount"] = Legacy::SatoshisToAmount(nNet - nFee);
                if(wtx.IsFromMe())
                    ret["fee"] = Legacy::SatoshisToAmount(nFee);
            }

            WalletTxToJSON(wallet.mapWallet[hash], ret);

            json::json details = json::json::array();
            ListTransactionsJSON(wallet.mapWallet[hash], "*", 0, false, details);
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

            /* Extract the address, which will either be a legacy address or a sig chain account address */
            std::string strAddress = params[0].get<std::string>();
            Legacy::NexusAddress address(strAddress);
            TAO::Register::Address hashAccount;

            /* Whether the address is considered valid */
            bool isValid = false;

            /* Flag indicating that this is a legacy (UTXO) address as opposed to a tritium register address */
            bool isLegacy = false;

            /* Decode the address string */
            hashAccount.SetBase58(strAddress);

            /* handle recipient being a register address */
            if(hashAccount.IsValid() && (hashAccount.IsAccount() || hashAccount.IsTrust()))
            {
                isLegacy = false;

                /* Get the account object. */
                TAO::Register::Object account;
                if(LLD::Register->ReadState(hashAccount, account))
                {
                    /* Parse the object register. */
                    if(account.Parse())
                    {
                        /* Get the object standard. */
                        uint8_t nStandard = account.Standard();

                        /* Check the object standard and check the account is a NXS account */
                        if((nStandard == TAO::Register::OBJECTS::ACCOUNT || nStandard == TAO::Register::OBJECTS::TRUST)
                        && (account.get<uint256_t>("token") == 0))
                            isValid = true;
                    }
                }
            }
            else
            {
                isLegacy = true;
                isValid = address.IsValid();
            }

            json::json ret;
            ret["isvalid"] = isValid;
            if(isValid && isLegacy)
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
            else if (isValid)
            {
                ret["address"] = hashAccount.ToString();
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