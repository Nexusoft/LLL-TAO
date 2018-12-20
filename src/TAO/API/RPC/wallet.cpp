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

    /* backupwallet <destination>
    *  Safely copies wallet.dat to destination, which can be a directory or a path with filename */
    json::json RPC::BackupWallet(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 1)
            return std::string(
                "backupwallet <destination>"
                " - Safely copies wallet.dat to destination, which can be a directory or a path with filename.");

    //     string strDest = params[0].get_str();
    //     BackupWallet(*pwalletMain, strDest);

    //     return Value::null;
    }


    // json::json keypoolrefill(const json::json& jsonParams, bool fHelp)
    // {
    //     if (pwalletMain->IsCrypted() && (fHelp || jsonParams.size() > 0))
    //         return std::string(
    //             "keypoolrefill"
    //             " - Fills the keypool, requires wallet passphrase to be set.");
    //     if (!pwalletMain->IsCrypted() && (fHelp || jsonParams.size() > 0))
    //         return std::string(
    //             "keypoolrefill"
    //             " - Fills the keypool.");

    //     if (pwalletMain->IsLocked())
    //         throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    //     pwalletMain->TopUpKeyPool();

    //     if (pwalletMain->GetKeyPoolSize() < GetArg("-keypool", 100))
    //         throw JSONRPCError(-4, "Error refreshing keypool.");

    //     return Value::null;
    // }


    // void ThreadTopUpKeyPool(void* parg)
    // {
    //     pwalletMain->TopUpKeyPool();
    // }

    // void ThreadCleanWalletPassphrase(void* parg)
    // {
    //     int64 nMyWakeTime = GetTimeMillis() + *((int64*)parg) * 1000;

    //     ENTER_CRITICAL_SECTION(cs_nWalletUnlockTime);

    //     if (nWalletUnlockTime == 0)
    //     {
    //         nWalletUnlockTime = nMyWakeTime;

    //         do
    //         {
    //             if (nWalletUnlockTime==0)
    //                 break;
    //             int64 nToSleep = nWalletUnlockTime - GetTimeMillis();
    //             if (nToSleep <= 0)
    //                 break;

    //             LEAVE_CRITICAL_SECTION(cs_nWalletUnlockTime);
    //             Sleep(nToSleep);
    //             ENTER_CRITICAL_SECTION(cs_nWalletUnlockTime);

    //         } while(1);

    //         if (nWalletUnlockTime)
    //         {
    //             nWalletUnlockTime = 0;
    //             pwalletMain->Lock();
    //         }
    //     }
    //     else
    //     {
    //         if (nWalletUnlockTime < nMyWakeTime)
    //             nWalletUnlockTime = nMyWakeTime;
    //     }

    //     LEAVE_CRITICAL_SECTION(cs_nWalletUnlockTime);

    //     delete (int64*)parg;
    // }

    /* walletpassphrase <passphrase> <timeout> [mintonly]
    *  Stores the wallet decryption key in memory for <timeout> seconds.
    *  mintonly is optional true/false allowing only block minting*/
    // json::json walletpassphrase(const json::json& jsonParams, bool fHelp)
    // {
    //     if (pwalletMain->IsCrypted() && (fHelp || jsonParams.size() < 2 || jsonParams.size() > 3))
    //         return std::string(
    //             "walletpassphrase <passphrase> <timeout> [mintonly]"
    //             " - Stores the wallet decryption key in memory for <timeout> seconds."
    //             " mintonly is optional true/false allowing only block minting.");
    //     if (fHelp)
    //         return true;
    //     if (!pwalletMain->IsCrypted())
    //         throw JSONRPCError(-15, "Error: running with an unencrypted wallet, but walletpassphrase was called.");

    //     if (!pwalletMain->IsLocked())
    //         throw JSONRPCError(-17, "Error: Wallet is already unlocked, use walletlock first if need to change unlock settings.");

    //     // Note that the walletpassphrase is stored in params[0] which is not mlock()ed
    //     SecureString strWalletPass;
    //     strWalletPass.reserve(100);
    //     // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    //     // Alternately, find a way to make params[0] mlock()'d to begin with.
    //     strWalletPass = params[0].get_str().c_str();

    //     if (strWalletPass.length() > 0)
    //     {
    //         if (!pwalletMain->Unlock(strWalletPass))
    //             throw JSONRPCError(-14, "Error: The wallet passphrase entered was incorrect.");
    //     }
    //     else
    //         return std::string(
    //             "walletpassphrase <passphrase> <timeout>"
    //             " - Stores the wallet decryption key in memory for <timeout> seconds.");

    //     CreateThread(ThreadTopUpKeyPool, NULL);
    //     int64* pnSleepTime = new int64(params[1].get_int64());
    //     CreateThread(ThreadCleanWalletPassphrase, pnSleepTime);

    //     // Nexus: if user OS account compromised prevent trivial sendmoney commands
    //     if (jsonParams.size() > 2)
    //         Wallet::fWalletUnlockMintOnly = params[2].get_bool();
    //     else
    //         Wallet::fWalletUnlockMintOnly = false;

    //     return Value::null;
    // }

    /* walletpassphrasechange <oldpassphrase> <newpassphrase>
    *  Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>*/
    // json::json walletpassphrasechange(const json::json& jsonParams, bool fHelp)
    // {
    //     if (pwalletMain->IsCrypted() && (fHelp || jsonParams.size() != 2))
    //         return std::string(
    //             "walletpassphrasechange <oldpassphrase> <newpassphrase>"
    //             " - Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");
    //     if (fHelp)
    //         return true;
    //     if (!pwalletMain->IsCrypted())
    //         throw JSONRPCError(-15, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");

    //     // TODO: get rid of these .c_str() calls by implementing SecureString::operator=(std::string)
    //     // Alternately, find a way to make params[0] mlock()'d to begin with.
    //     SecureString strOldWalletPass;
    //     strOldWalletPass.reserve(100);
    //     strOldWalletPass = params[0].get_str().c_str();

    //     SecureString strNewWalletPass;
    //     strNewWalletPass.reserve(100);
    //     strNewWalletPass = params[1].get_str().c_str();

    //     if (strOldWalletPass.length() < 1 || strNewWalletPass.length() < 1)
    //         return std::string(
    //             "walletpassphrasechange <oldpassphrase> <newpassphrase>"
    //             " - Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");

    //     if (!pwalletMain->ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass))
    //         throw JSONRPCError(-14, "Error: The wallet passphrase entered was incorrect.");

    //     return Value::null;
    // }


    /* walletlock"
    *  Removes the wallet encryption key from memory, locking the wallet.
    *  After calling this method, you will need to call walletpassphrase again
    *  before being able to call any methods which require the wallet to be unlocked */
    // json::json walletlock(const json::json& jsonParams, bool fHelp)
    // {
    //     if (pwalletMain->IsCrypted() && (fHelp || jsonParams.size() != 0))
    //         return std::string(
    //             "walletlock"
    //             " - Removes the wallet encryption key from memory, locking the wallet."
    //             " After calling this method, you will need to call walletpassphrase again"
    //             " before being able to call any methods which require the wallet to be unlocked.");
    //     if (fHelp)
    //         return true;
    //     if (!pwalletMain->IsCrypted())
    //         throw JSONRPCError(-15, "Error: running with an unencrypted wallet, but walletlock was called.");

    //     {
    //         LOCK(cs_nWalletUnlockTime);
    //         pwalletMain->Lock();
    //         nWalletUnlockTime = 0;
    //     }

    //     return Value::null;
    // }

    /* encryptwallet <passphrase>
    *  Encrypts the wallet with <passphrase>. */
    // json::json encryptwallet(const json::json& jsonParams, bool fHelp)
    // {
    //     if (!pwalletMain->IsCrypted() && (fHelp || jsonParams.size() != 1))
    //         return std::string(
    //             "encryptwallet <passphrase>"
    //             " - Encrypts the wallet with <passphrase>.");
    //     if (fHelp)
    //         return true;
    //     if (pwalletMain->IsCrypted())
    //         throw JSONRPCError(-15, "Error: running with an encrypted wallet, but encryptwallet was called.");

    //     // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    //     // Alternately, find a way to make params[0] mlock()'d to begin with.
    //     SecureString strWalletPass;
    //     strWalletPass.reserve(100);
    //     strWalletPass = params[0].get_str().c_str();

    //     if (strWalletPass.length() < 1)
    //         return std::string(
    //             "encryptwallet <passphrase>"
    //             " - Encrypts the wallet with <passphrase>.");

    //     if (!pwalletMain->EncryptWallet(strWalletPass))
    //         throw JSONRPCError(-16, "Error: Failed to encrypt the wallet.");

    //     // BDB seems to have a bad habit of writing old data into
    //     // slack space in .dat files; that is bad if the old data is
    //     // unencrypted private keys.  So:
    //     StartShutdown();
    //     return "wallet encrypted; Nexus server stopping, restart to run with encrypted wallet";
    // }

    /* checkwallet
    *  Check wallet for integrity */
    json::json RPC::CheckWallet(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 0)
            return std::string(
                "checkwallet"
                " - Check wallet for integrity.");

    //     int nMismatchSpent;
    //     int64 nBalanceInQuestion;
    //     pwalletMain->FixSpentCoins(nMismatchSpent, nBalanceInQuestion, true);
    //     Object result;
    //     if (nMismatchSpent == 0)
    //         result.push_back(Pair("wallet check passed", true));
    //     else
    //     {
    //         result.push_back(Pair("mismatched spent coins", nMismatchSpent));
    //         result.push_back(Pair("amount in question", ValueFromAmount(nBalanceInQuestion)));
    //     }
    //     return result;
    }

    /* listtrustkeys
    *  List all the Trust Keys this Node owns */
    json::json RPC::ListTrustKeys(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 0)
            return std::string(
                "listtrustkeys"
                " - List all the Trust Keys this Node owns.");

    //     Object result;
    //     LLD::CTrustDB trustdb("cr");
    //     LLD::CIndexDB indexdb("cr");

    //     Core::CTrustKey trustKey;
    //     if(trustdb.ReadMyKey(trustKey))
    //     {

    //         /* Set the wallet address. */
    //         Wallet::NexusAddress address;
    //         address.SetPubKey(trustKey.vchPubKey);
    //         result.push_back(Pair(address.ToString(), Core::dInterestRate));
        // }


    //     return result;
    }

    /* repairwallet
    *  Repair wallet if checkwallet reports any problem */
    json::json RPC::RepairWallet(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() > 0)
            return std::string(
                "repairwallet"
                " - Repair wallet if checkwallet reports any problem.");

    //     int nMismatchSpent;
    //     int64 nBalanceInQuestion;
    //     pwalletMain->FixSpentCoins(nMismatchSpent, nBalanceInQuestion);
    //     Object result;
    //     if (nMismatchSpent == 0)
    //         result.push_back(Pair("wallet check passed", true));
    //     else
    //     {
    //         result.push_back(Pair("mismatched spent coins", nMismatchSpent));
    //         result.push_back(Pair("amount affected by repair", ValueFromAmount(nBalanceInQuestion)));
    //     }
    //     return result;
    }

    // class CTxDump
    // {
    // public:
    //     Core::CBlockIndex *pindex;
    //     int64 nValue;
    //     bool fSpent;
    //     Wallet::CWalletTx* ptx;
    //     int nOut;
    //     CTxDump(Wallet::CWalletTx* ptx = NULL, int nOut = -1)
    //     {
    //         pindex = NULL;
    //         nValue = 0;
    //         fSpent = false;
    //         this->ptx = ptx;
    //         this->nOut = nOut;
    //     }
    // };

    /* rescan
    *  Rescans the database for relevant wallet transactions */
    json::json RPC::Rescan(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 0)
            return std::string(
                "rescan"
                " - Rescans the database for relevant wallet transactions.");

    //     pwalletMain->ScanForWalletTransactions(Core::pindexGenesisBlock, true);

    //     return "success";
    }

    /* importprivkey <PrivateKey> [label]
    *  Adds a private key (as returned by dumpprivkey) to your wallet */
    json::json RPC::ImportPrivKey(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() < 1 || jsonParams.size() > 2)
            return std::string(
                "importprivkey <PrivateKey> [label]"
                " - Adds a private key (as returned by dumpprivkey) to your wallet.");

    //     string strSecret = params[0].get_str();
    //     string strLabel = "";
    //     if (jsonParams.size() > 1)
    //         strLabel = params[1].get_str();
    //     Wallet::NexusSecret vchSecret;
    //     bool fGood = vchSecret.SetString(strSecret);

    //     if (!fGood) throw JSONRPCError(-5,"Invalid private key");
    //     if (pwalletMain->IsLocked())
    //         throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    //     if (Wallet::fWalletUnlockMintOnly)
    //         throw JSONRPCError(-102, "Wallet is unlocked for minting only.");

    //     Wallet::CKey key;
    //     bool fCompressed;
    //     Wallet::CSecret secret = vchSecret.GetSecret(fCompressed);
    //     key.SetSecret(secret, fCompressed);
    //     Wallet::NexusAddress vchAddress = Wallet::NexusAddress(key.GetPubKey());

    //     {
    //         LOCK2(Core::cs_main, pwalletMain->cs_wallet);

    //         pwalletMain->MarkDirty();
    //         pwalletMain->SetAddressBookName(vchAddress, strLabel);

    //         if (!pwalletMain->AddKey(key))
    //             throw JSONRPCError(-4,"Error adding key to wallet");
    //     }

    //     MainFrameRepaint();

    //     return Value::null;
    }

    /* dumpprivkey <NexusAddress>
    *  Reveals the private key corresponding to <NexusAddress> */
    json::json RPC::DumpPrivKey(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 1)
            return std::string(
                "dumpprivkey <NexusAddress>"
                " - Reveals the private key corresponding to <NexusAddress>.");

    //     string strAddress = params[0].get_str();
    //     Wallet::NexusAddress address;
    //     if (!address.SetString(strAddress))
    //         throw JSONRPCError(-5, "Invalid Nexus address");
    //     if (pwalletMain->IsLocked())
    //         throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    //     if (Wallet::fWalletUnlockMintOnly) // Nexus: no dumpprivkey in mint-only mode
    //         throw JSONRPCError(-102, "Wallet is unlocked for minting only.");
    //     Wallet::CSecret vchSecret;
    //     bool fCompressed;
    //     if (!pwalletMain->GetSecret(address, vchSecret, fCompressed))
    //         throw JSONRPCError(-4,"Private key for address " + strAddress + " is not known");
    //     return Wallet::NexusSecret(vchSecret, fCompressed).ToString();
    }

    /* importkeys
    *  Import List of Private Keys and return if they import properly or fail with their own code in the output sequence" 
    *  You need to list the imported keys in a JSON array of {[account],[privatekey]} */
    json::json RPC::ImportKeys(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() < 1)
        {
            return std::string(
                "importkeys"
                " - Import List of Private Keys and return if they import properly or fail with their own code in the output sequence" 
                " You need to list the imported keys in a JSON array of {[account],[privatekey]}");
        }
            /** Make sure the Wallet is Unlocked fully before proceeding. **/
    //         if (pwalletMain->IsLocked())
    //             throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    //         if (Wallet::fWalletUnlockMintOnly)
    //             throw JSONRPCError(-102, "Wallet is unlocked for minting only.");

    //         /** Establish the JSON Object from the Parameters. **/
    //         Object entry = params[0].get_obj();

    //         /** Establish a return value JSON object for Error Reporting. **/
    //         Object response;

    //         /** Import the Keys one by One. **/
    //         for(int nIndex = 0; nIndex < entry.size(); nIndex++) {

    //             Wallet::NexusSecret vchSecret;
    //             if(!vchSecret.SetString(entry[nIndex].value_.get_str())){
    //                 response.push_back(Pair(entry[nIndex].name_, "Code -5 Invalid Key"));

    //                 continue;
    //             }

    //             Wallet::CKey key;
    //             bool fCompressed;
    //             Wallet::CSecret secret = vchSecret.GetSecret(fCompressed);
    //             key.SetSecret(secret, fCompressed);
    //             Wallet::NexusAddress vchAddress = Wallet::NexusAddress(key.GetPubKey());

    //             {
    //                 LOCK2(Core::cs_main, pwalletMain->cs_wallet);

    //                 pwalletMain->MarkDirty();
    //                 pwalletMain->SetAddressBookName(vchAddress, entry[nIndex].name_);

    //                 if (!pwalletMain->AddKey(key))
    //                 {
    //                     response.push_back(Pair(entry[nIndex].name_, "Code -4 Error Adding to Wallet"));

    //                     continue;
    //                 }
    //             }

    //             response.push_back(Pair(entry[nIndex].name_, "Successfully Imported"));
    //         }

    //         MainFrameRepaint();

    //         return response;
    }

    /* exportkeys
    *  Export the private keys of the current UTXO values.
    *  This will allow the importing and exporting of private keys much easier. */
    json::json RPC::ExportKeys(const json::json& jsonParams, bool fHelp)
    {
        if (fHelp || jsonParams.size() != 0)
            return std::string(
                "exportkeys"
                " - Export the private keys of the current UTXO values. "
                " This will allow the importing and exporting of private keys much easier");

    //     /** Disallow the exporting of private keys if the encryption key is not available in the memory. **/
    //     if (pwalletMain->IsLocked())
    //         throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    //     if (Wallet::fWalletUnlockMintOnly) // Nexus: no dumpprivkey in mint-only mode
    //         throw JSONRPCError(-102, "Wallet is unlocked for minting only.");

    //     /** Compile the list of available Nexus Addresses and their according Balances. **/
    //     map<Wallet::NexusAddress, int64> mapAddresses;
    //     if(!pwalletMain->AvailableAddresses((unsigned int)GetUnifiedTimestamp(), mapAddresses))
    //         throw JSONRPCError(-3, "Error Extracting the Addresses from Wallet File. Please Try Again.");

    //     /** Loop all entries of the memory map to compile the list of account names and their addresses.
    //         JSON object format is a reflection of the import and export options. **/
    //     Object entry;
    //     for (map<Wallet::NexusAddress, int64>::iterator it = mapAddresses.begin(); it != mapAddresses.end(); ++it)
    //     {
    //         /** Extract the Secret key from the Wallet. **/
    //         Wallet::CSecret vchSecret;
    //         bool fCompressed;
    //         if (!pwalletMain->GetSecret(it->first, vchSecret, fCompressed))
    //             throw JSONRPCError(-4,"Private key for address " + it->first.ToString() + " is not known");

    //         /** Extract the account name from the address book. **/
    //         string strAccount;
    //         if(!pwalletMain->mapAddressBook.count(it->first))
    //             strAccount = "Default";
    //         else
    //             strAccount = pwalletMain->mapAddressBook[it->first];

    //         /** Compile the Secret Key and account information into a listed pair. **/
    //         string strSecret = Wallet::NexusSecret(vchSecret, fCompressed).ToString();
    //         entry.push_back(Pair(strAccount, strSecret));
    //     }

    //     return entry;
    }

}