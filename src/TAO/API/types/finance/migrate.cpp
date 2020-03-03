/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/finance.h>

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

#include <TAO/Ledger/include/constants.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/state.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/create.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <Legacy/include/money.h>
#include <Legacy/types/address.h>
#include <Legacy/types/script.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/types/wallettx.h>

#include <map>
#include <string>
#include <vector>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Migrate all Legacy wallet accounts to corresponding accounts in the signature chain */
        json::json Finance::MigrateAccounts(const json::json& params, bool fHelp)
        {
            /* Return value array */
            json::json ret = json::json::array();

            Legacy::Wallet& wallet = Legacy::Wallet::GetInstance();

            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            uint256_t nSession = users->GetSession(params);

            /* Get the user signature chain. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
            if(!user)
                throw APIException(-10, "Invalid session ID");

            /* Check for walletpassphrase parameter. */
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if(params.find("walletpassphrase") != params.end())
                strWalletPass = params["walletpassphrase"].get<std::string>().c_str();

            /* Check to see if the caller has specified NOT to create a name (we do by default) */
            bool fCreateName = params.find("createname") == params.end()
                    || (params["createname"].get<std::string>() != "0" && params["createname"].get<std::string>() != "false");

            /* Save the current lock state of wallet */
            bool fLocked = wallet.IsLocked();
            bool fMintOnly = Legacy::fWalletUnlockMintOnly;

            /* Must provide passphrase to send if wallet locked or unlocked for minting only */
            if(wallet.IsCrypted() && (fLocked || fMintOnly))
            {
                if(strWalletPass.length() == 0)
                    throw APIException(-179, "Legacy wallet is locked. walletpassphrase required");

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
                    throw APIException(-180, "Incorrect walletpassphrase for Legacy wallet");
            }

            /* Get a map of all account balances from the legacy wallet */
            std::map<std::string, int64_t> mapAccountBalances;
            for(const auto& entry : Legacy::Wallet::GetInstance().GetAddressBook().GetAddressBookMap())
            {
                if(Legacy::Wallet::GetInstance().HaveKey(entry.first)) // This address belongs to me
                {
                    if(entry.second == "" || entry.second == "default")
                        mapAccountBalances["default"] = 0;
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

                    /* Make sure to map blank legacy account to default */
                    if(strAccount == "")
                        strAccount = "default";

                    mapAccountBalances[strAccount] += entry.second;
                }
                else
                {
                    mapAccountBalances["default"] += entry.second;
                }
            }


            /* map of legacy account names to tritium account register addresses */
            std::map<std::string, TAO::Register::Address> mapAccountRegisters;

            /* Lock the signature chain. */
            LOCK(users->CREATE_MUTEX);

            /* Create the transaction. */
            TAO::Ledger::Transaction tx;
            if(!Users::CreateTransaction(user, strPIN, tx))
                throw APIException(-17, "Failed to create transaction");

            /* tracks how many contracts we have added to the current transaction */
            uint8_t nContracts = 0;

            /* Iterate the legacy accounts */
            for(const auto& accountBalance :  mapAccountBalances)
            {
                /* The name of the legacy account */
                std::string strAccount = accountBalance.first;

                /* The new account address */
                TAO::Register::Address hashAccount;

                /* First check to see if an account exists with this name */
                hashAccount = Names::ResolveAddress(params, strAccount, false);

                /* If one does not exist then check to see if one exists with a matching data field, from a previous migration */
                if(!hashAccount.IsValid())
                {
                    std::vector<TAO::Register::Address> vAccounts;
                    if(ListAccounts(user->Genesis(), vAccounts, false, false))
                    {
                        for(const auto& hashRegister : vAccounts)
                        {
                            /* Retrieve the account */
                            TAO::Register::Object object;
                            if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                                throw TAO::API::APIException(-13, "Account not found");

                            /* Parse the object register. */
                            if(!object.Parse())
                                throw TAO::API::APIException(-14, "Object failed to parse");

                            /* Check to see if it is a NXS account the data matches the account name */
                            if(object.get<uint256_t>("token") == 0
                                && object.CheckName("data") && object.get<std::string>("data") == strAccount)
                            {
                                hashAccount = hashRegister;
                                break;
                            }
                        }
                    }
                }

                /* If we still haven't found an account then create a new one */
                if(!hashAccount.IsValid())
                {
                    /* Make sure we have enough room in the current TX for this account and name. If not then submit this transaction
                       and create a new one.  NOTE we add a maximum of 99 to leave room for the fee  */
                    if(nContracts +(fCreateName ? 1 : 2) >= 99)
                    {
                        /* Add the fee */
                        AddFee(tx);

                        /* Execute the operations layer. */
                        if(!tx.Build())
                            throw APIException(-44, "Transaction failed to build");

                        /* Sign the transaction. */
                        if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                            throw APIException(-31, "Ledger failed to sign transaction");

                        /* Execute the operations layer. */
                        if(!TAO::Ledger::mempool.Accept(tx))
                            throw APIException(-32, "Failed to accept");

                        /* Create the next transaction and reset the counter */
                        tx = TAO::Ledger::Transaction();
                        if(!Users::CreateTransaction(user, strPIN, tx))
                            throw APIException(-17, "Failed to create transaction");


                        nContracts = 0;
                    }

                    /* Generate a random hash for this objects register address */
                    hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

                    /* Create an account object register for NXS (identifier 0). */
                    TAO::Register::Object account = TAO::Register::CreateAccount(0);

                    /* Store the legacy account name in the data field of the account register */
                    account << std::string("data") << uint8_t(TAO::Register::TYPES::STRING) << strAccount;

                    /* Submit the payload object. */
                    tx[nContracts++] << uint8_t(TAO::Operation::OP::CREATE) << hashAccount << uint8_t(TAO::Register::REGISTER::OBJECT) << account.GetState();

                    /* If user has not explicitly indicated not to create a name then create a Name Object register for it. */
                    if(fCreateName)
                        tx[nContracts++] = Names::CreateName(user->Genesis(), strAccount, "", hashAccount);
                }

                /* Add this to the map */
                mapAccountRegisters[strAccount] = hashAccount;
            }

            /* If there are accounts to create then submit the transaction */
            if(nContracts > 0)
            {
                /* Add the fee */
                AddFee(tx);

                /* Execute the operations layer. */
                if(!tx.Build())
                    throw APIException(-44, "Transaction failed to build");

                /* Sign the transaction. */
                if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, nSession)))
                    throw APIException(-31, "Ledger failed to sign transaction");

                /* Execute the operations layer. */
                if(!TAO::Ledger::mempool.Accept(tx))
                    throw APIException(-32, "Failed to accept");
            }

            /* Once the accounts have been created transfer the balance from the legacy account to the new ones */
            for(const auto& accountBalance :  mapAccountBalances)
            {
                /* Check that there is enough balance to send */
                if(accountBalance.second <= Legacy::TRANSACTION_FEE)
                    continue;

                /* The account to send from */
                std::string strAccount = accountBalance.first;

                /* The account address to send to */
                TAO::Register::Address hashAccount = mapAccountRegisters[strAccount];

                /* The amount to send */
                int64_t nAmount = accountBalance.second;

                /* The script to contain the recipient */
                Legacy::Script scriptPubKey;
                scriptPubKey.SetRegisterAddress(hashAccount);

                /* Legacy wallet transaction  */
                Legacy::WalletTx wtx;

                /* Set the from account */
                wtx.strFromAccount = strAccount;

                /* Create the legacy transaction */
                std::string strException = wallet.SendToNexusAddress(scriptPubKey, nAmount, wtx, false, 1, true);

                json::json entry;
                entry["account"] = strAccount;
                entry["address"] = hashAccount.ToString();
                entry["amount"]  = (nAmount / (double)TAO::Ledger::NXS_COIN);

                if(!strException.empty())
                    entry["error"] = strException;
                else
                    entry["txid"] = wtx.GetHash().GetHex();

                ret.push_back(entry);
            }

            /* If used walletpassphrase to temporarily unlock wallet, re-lock the wallet
             * This does not return unlocked for minting state, because we are migrating from the trust key and
             * the minter should not be re-started.
             */
            if(wallet.IsCrypted() && (fLocked || fMintOnly))
                wallet.Lock();

            return ret;
        }
    }
}
