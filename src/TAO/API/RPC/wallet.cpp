/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/rpc.h>
#include <Util/include/json.h>

#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>

namespace TAO::API
{

    /* backupwallet <destination>
    *  Safely copies wallet.dat to destination, which can be a directory or a path with filename */
    json::json RPC::BackupWallet(const json::json& params, bool fHelp)
    {
        if (fHelp || params.size() != 1)
        {
            return std::string(
                "backupwallet <destination>"
                " - Safely copies wallet.dat to destination, which can be a directory or a path with filename.");
        }

        std::string strDest = params[0].get<std::string>();
        //Legacy::CWalletDB::BackupWallet(*Legacy::pwalletMain, strDest);

        return nullptr;
    }


    // json::json keypoolrefill(const json::json& params, bool fHelp)
    // {
    //     if (Legacy::CWallet::GetInstance().IsCrypted() && (fHelp || params.size() > 0))
    //         return std::string(
    //             "keypoolrefill"
    //             " - Fills the keypool, requires wallet passphrase to be set.");
    //     if (!Legacy::CWallet::GetInstance().IsCrypted() && (fHelp || params.size() > 0))
    //         return std::string(
    //             "keypoolrefill"
    //             " - Fills the keypool.");

    //     if (Legacy::CWallet::GetInstance().IsLocked())
    //         throw APIException(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    //     Legacy::CWallet::GetInstance().TopUpKeyPool();

    //     if (Legacy::CWallet::GetInstance().GetKeyPoolSize() < GetArg("-keypool", 100))
    //         throw APIException(-4, "Error refreshing keypool.");

    //     return Value::null;
    // }


    // void ThreadTopUpKeyPool(void* parg)
    // {
    //     Legacy::CWallet::GetInstance().TopUpKeyPool();
    // }

    

    /* walletpassphrase <passphrase> [timeout] [mintonly]
    *  Stores the wallet decryption key in memory for [timeout] seconds.
    *  mintonly is optional true/false allowing only block minting. timeout is ignored if mintonly is true / 1*/
    json::json RPC::WalletPassphrase(const json::json& params, bool fHelp)
    {
        if (fHelp || (Legacy::CWallet::GetInstance().IsCrypted() && ( params.size() < 1 || params.size() > 3)))
            return std::string(
                "walletpassphrase <passphrase> [timeout] [mintonly]"
                " - Stores the wallet decryption key in memory for [timeout] seconds."
                " mintonly is optional true/false allowing only block minting. timeout is ignored if mintonly is true / 1");
        
        if (!Legacy::CWallet::GetInstance().IsCrypted())
            throw APIException(-15, "Error: running with an unencrypted wallet, but walletpassphrase was called.");

        if (!Legacy::CWallet::GetInstance().IsLocked())
            throw APIException(-17, "Error: Wallet is already unlocked, use walletlock first if need to change unlock settings.");

        // Note that the walletpassphrase is stored in params[0] which is not mlock()ed
        SecureString strWalletPass;
        strWalletPass.reserve(100);
        
        strWalletPass = params[0];

        if (strWalletPass.length() > 0)
        {
            uint32_t nLockSeconds = 0;
            if (params.size() > 1)
                nLockSeconds = (uint32_t)params[1];

            if (!Legacy::CWallet::GetInstance().Unlock(strWalletPass, nLockSeconds))
                throw APIException(-14, "Error: The wallet passphrase entered was incorrect.");
        }
        else
            return std::string(
                "walletpassphrase <passphrase> [timeout]"
                " - Stores the wallet decryption key in memory for [timeout] seconds.");

        // asynchronously top up the keypool while we have the wallet unlocked.
        std::thread([]()
        {
            Legacy::CWallet::GetInstance().GetKeyPool().TopUpKeyPool();

        }).detach();
        
        // Nexus: if user OS account compromised prevent trivial sendmoney commands
        if (params.size() > 2)
            Legacy::fWalletUnlockMintOnly = (bool)params[2];
        else
            Legacy::fWalletUnlockMintOnly = false;

        return "";
    }

    /* walletpassphrasechange <oldpassphrase> <newpassphrase>
    *  Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>*/
    // json::json walletpassphrasechange(const json::json& params, bool fHelp)
    // {
    //     if (Legacy::CWallet::GetInstance().IsCrypted() && (fHelp || params.size() != 2))
    //         return std::string(
    //             "walletpassphrasechange <oldpassphrase> <newpassphrase>"
    //             " - Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");
    //     if (fHelp)
    //         return true;
    //     if (!Legacy::CWallet::GetInstance().IsCrypted())
    //         throw APIException(-15, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");

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

    //     if (!Legacy::CWallet::GetInstance().ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass))
    //         throw APIException(-14, "Error: The wallet passphrase entered was incorrect.");

    //     return Value::null;
    // }


    /* walletlock"
    *  Removes the wallet encryption key from memory, locking the wallet.
    *  After calling this method, you will need to call walletpassphrase again
    *  before being able to call any methods which require the wallet to be unlocked */
    // json::json walletlock(const json::json& params, bool fHelp)
    // {
    //     if (Legacy::CWallet::GetInstance().IsCrypted() && (fHelp || params.size() != 0))
    //         return std::string(
    //             "walletlock"
    //             " - Removes the wallet encryption key from memory, locking the wallet."
    //             " After calling this method, you will need to call walletpassphrase again"
    //             " before being able to call any methods which require the wallet to be unlocked.");
    //     if (fHelp)
    //         return true;
    //     if (!Legacy::CWallet::GetInstance().IsCrypted())
    //         throw APIException(-15, "Error: running with an unencrypted wallet, but walletlock was called.");

    //     {
    //         LOCK(cs_nWalletUnlockTime);
    //         Legacy::CWallet::GetInstance().Lock();
    //         nWalletUnlockTime = 0;
    //     }

    //     return Value::null;
    // }

    /* encryptwallet <passphrase>
    *  Encrypts the wallet with <passphrase>. */
    json::json RPC::EncryptWallet(const json::json& params, bool fHelp)
    {
        if (fHelp || (!Legacy::CWallet::GetInstance().IsCrypted() && params.size() != 1))
            return std::string(
                "encryptwallet <passphrase>"
                " - Encrypts the wallet with <passphrase>.");

        if (Legacy::CWallet::GetInstance().IsCrypted())
            throw APIException(-15, "Error: running with an encrypted wallet, but encryptwallet was called.");

        SecureString strWalletPass;
        strWalletPass.reserve(100);
        strWalletPass = std::string(params.at(0)).c_str();

        if (strWalletPass.length() < 1)
            return std::string(
                "encryptwallet <passphrase>"
                " - Encrypts the wallet with <passphrase>.");

        if (!Legacy::CWallet::GetInstance().EncryptWallet(strWalletPass))
            throw APIException(-16, "Error: Failed to encrypt the wallet.");

        // BDB seems to have a bad habit of writing old data into
        // slack space in .dat files; that is bad if the old data is
        // unencrypted private keys.  So:
        //StartShutdown();
        //return "wallet encrypted; Nexus server stopping, restart to run with encrypted wallet";
        return "wallet encrypted";
    }

    /* checkwallet
    *  Check wallet for integrity */
    json::json RPC::CheckWallet(const json::json& params, bool fHelp)
    {
        if (fHelp || params.size() > 0)
            return std::string(
                "checkwallet"
                " - Check wallet for integrity.");

    //     int nMismatchSpent;
    //     int64 nBalanceInQuestion;
    //     Legacy::CWallet::GetInstance().FixSpentCoins(nMismatchSpent, nBalanceInQuestion, true);
    //     Object result;
    //     if (nMismatchSpent == 0)
    //         result.push_back(Pair("wallet check passed", true));
    //     else
    //     {
    //         result.push_back(Pair("mismatched spent coins", nMismatchSpent));
    //         result.push_back(Pair("amount in question", ValueFromAmount(nBalanceInQuestion)));
    //     }
    //     return result;
        json::json ret;
        return ret;
    }

    /* listtrustkeys
    *  List all the Trust Keys this Node owns */
    json::json RPC::ListTrustKeys(const json::json& params, bool fHelp)
    {
        if (fHelp || params.size() > 0)
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
        json::json ret;
        return ret;
    }

    /* repairwallet
    *  Repair wallet if checkwallet reports any problem */
    json::json RPC::RepairWallet(const json::json& params, bool fHelp)
    {
        if (fHelp || params.size() > 0)
            return std::string(
                "repairwallet"
                " - Repair wallet if checkwallet reports any problem.");

    //     int nMismatchSpent;
    //     int64 nBalanceInQuestion;
    //     Legacy::CWallet::GetInstance().FixSpentCoins(nMismatchSpent, nBalanceInQuestion);
    //     Object result;
    //     if (nMismatchSpent == 0)
    //         result.push_back(Pair("wallet check passed", true));
    //     else
    //     {
    //         result.push_back(Pair("mismatched spent coins", nMismatchSpent));
    //         result.push_back(Pair("amount affected by repair", ValueFromAmount(nBalanceInQuestion)));
    //     }
    //     return result;
        json::json ret;
        return ret;
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
    json::json RPC::Rescan(const json::json& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            return std::string(
                "rescan"
                " - Rescans the database for relevant wallet transactions.");

    //     Legacy::CWallet::GetInstance().ScanForWalletTransactions(Core::pindexGenesisBlock, true);

    //     return "success";
        json::json ret;
        return ret;
    }

    /* importprivkey <PrivateKey> [label]
    *  Adds a private key (as returned by dumpprivkey) to your wallet */
    json::json RPC::ImportPrivKey(const json::json& params, bool fHelp)
    {
        if (fHelp || params.size() < 1 || params.size() > 2)
            return std::string(
                "importprivkey <PrivateKey> [label]"
                " - Adds a private key (as returned by dumpprivkey) to your wallet.");

    //     string strSecret = params[0].get_str();
    //     string strLabel = "";
    //     if (params.size() > 1)
    //         strLabel = params[1].get_str();
    //     Wallet::NexusSecret vchSecret;
    //     bool fGood = vchSecret.SetString(strSecret);

    //     if (!fGood) throw APIException(-5,"Invalid private key");
    //     if (Legacy::CWallet::GetInstance().IsLocked())
    //         throw APIException(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    //     if (Wallet::fWalletUnlockMintOnly)
    //         throw APIException(-102, "Wallet is unlocked for minting only.");

    //     Wallet::CKey key;
    //     bool fCompressed;
    //     Wallet::CSecret secret = vchSecret.GetSecret(fCompressed);
    //     key.SetSecret(secret, fCompressed);
    //     Wallet::NexusAddress vchAddress = Wallet::NexusAddress(key.GetPubKey());

    //     {
    //         LOCK2(Core::cs_main, Legacy::CWallet::GetInstance().cs_wallet);

    //         Legacy::CWallet::GetInstance().MarkDirty();
    //         Legacy::CWallet::GetInstance().SetAddressBookName(vchAddress, strLabel);

    //         if (!Legacy::CWallet::GetInstance().AddKey(key))
    //             throw APIException(-4,"Error adding key to wallet");
    //     }

    //     MainFrameRepaint();

    //     return Value::null;
        json::json ret;
        return ret;
    }

    /* dumpprivkey <NexusAddress>
    *  Reveals the private key corresponding to <NexusAddress> */
    json::json RPC::DumpPrivKey(const json::json& params, bool fHelp)
    {
        if (fHelp || params.size() != 1)
            return std::string(
                "dumpprivkey <NexusAddress>"
                " - Reveals the private key corresponding to <NexusAddress>.");

    //     string strAddress = params[0].get_str();
    //     Wallet::NexusAddress address;
    //     if (!address.SetString(strAddress))
    //         throw APIException(-5, "Invalid Nexus address");
    //     if (Legacy::CWallet::GetInstance().IsLocked())
    //         throw APIException(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    //     if (Wallet::fWalletUnlockMintOnly) // Nexus: no dumpprivkey in mint-only mode
    //         throw APIException(-102, "Wallet is unlocked for minting only.");
    //     Wallet::CSecret vchSecret;
    //     bool fCompressed;
    //     if (!Legacy::CWallet::GetInstance().GetSecret(address, vchSecret, fCompressed))
    //         throw APIException(-4,"Private key for address " + strAddress + " is not known");
    //     return Wallet::NexusSecret(vchSecret, fCompressed).ToString();
        json::json ret;
        return ret;
    }

    /* importkeys
    *  Import List of Private Keys and return if they import properly or fail with their own code in the output sequence"
    *  You need to list the imported keys in a JSON array of {[account],[privatekey]} */
    json::json RPC::ImportKeys(const json::json& params, bool fHelp)
    {
        if (fHelp || params.size() < 1)
        {
            return std::string(
                "importkeys"
                " - Import List of Private Keys and return if they import properly or fail with their own code in the output sequence"
                " You need to list the imported keys in a JSON array of {[account],[privatekey]}");
        }
            /** Make sure the Wallet is Unlocked fully before proceeding. **/
    //         if (Legacy::CWallet::GetInstance().IsLocked())
    //             throw APIException(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    //         if (Wallet::fWalletUnlockMintOnly)
    //             throw APIException(-102, "Wallet is unlocked for minting only.");

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
    //                 LOCK2(Core::cs_main, Legacy::CWallet::GetInstance().cs_wallet);

    //                 Legacy::CWallet::GetInstance().MarkDirty();
    //                 Legacy::CWallet::GetInstance().SetAddressBookName(vchAddress, entry[nIndex].name_);

    //                 if (!Legacy::CWallet::GetInstance().AddKey(key))
    //                 {
    //                     response.push_back(Pair(entry[nIndex].name_, "Code -4 Error Adding to Wallet"));

    //                     continue;
    //                 }
    //             }

    //             response.push_back(Pair(entry[nIndex].name_, "Successfully Imported"));
    //         }

    //         MainFrameRepaint();

    //         return response;
        json::json ret;
        return ret;
    }

    /* exportkeys
    *  Export the private keys of the current UTXO values.
    *  This will allow the importing and exporting of private keys much easier. */
    json::json RPC::ExportKeys(const json::json& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            return std::string(
                "exportkeys"
                " - Export the private keys of the current UTXO values. "
                " This will allow the importing and exporting of private keys much easier");

    //     /** Disallow the exporting of private keys if the encryption key is not available in the memory. **/
    //     if (Legacy::CWallet::GetInstance().IsLocked())
    //         throw APIException(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    //     if (Wallet::fWalletUnlockMintOnly) // Nexus: no dumpprivkey in mint-only mode
    //         throw APIException(-102, "Wallet is unlocked for minting only.");

    //     /** Compile the list of available Nexus Addresses and their according Balances. **/
    //     map<Wallet::NexusAddress, int64> mapAddresses;
    //     if(!Legacy::CWallet::GetInstance().AvailableAddresses((unsigned int)GetUnifiedTimestamp(), mapAddresses))
    //         throw APIException(-3, "Error Extracting the Addresses from Wallet File. Please Try Again.");

    //     /** Loop all entries of the memory map to compile the list of account names and their addresses.
    //         JSON object format is a reflection of the import and export options. **/
    //     Object entry;
    //     for (map<Wallet::NexusAddress, int64>::iterator it = mapAddresses.begin(); it != mapAddresses.end(); ++it)
    //     {
    //         /** Extract the Secret key from the Wallet. **/
    //         Wallet::CSecret vchSecret;
    //         bool fCompressed;
    //         if (!Legacy::CWallet::GetInstance().GetSecret(it->first, vchSecret, fCompressed))
    //             throw APIException(-4,"Private key for address " + it->first.ToString() + " is not known");

    //         /** Extract the account name from the address book. **/
    //         string strAccount;
    //         if(!Legacy::CWallet::GetInstance().mapAddressBook.count(it->first))
    //             strAccount = "Default";
    //         else
    //             strAccount = Legacy::CWallet::GetInstance().mapAddressBook[it->first];

    //         /** Compile the Secret Key and account information into a listed pair. **/
    //         string strSecret = Wallet::NexusSecret(vchSecret, fCompressed).ToString();
    //         entry.push_back(Pair(strAccount, strSecret));
    //     }

    //     return entry;
        json::json ret;
        return ret;
    }
}
