/*******************************************************************************************

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

 [Learn and Create] Viz. http://www.opensource.org/licenses/mit-license.php

*******************************************************************************************/

#include "../main.h" // for pwalletMain
#include "../net/rpcserver.h"
#include "../util/ui_interface.h"

#include <boost/lexical_cast.hpp>

#include "../json/json_spirit_reader_template.h"
#include "../json/json_spirit_writer_template.h"
#include "../json/json_spirit_utils.h"

#define printf OutputDebugStringF

// using namespace boost::asio;
using namespace json_spirit;
using namespace std;

namespace Net
{
    extern Object JSONRPCError(int code, const string& message);

    class CTxDump
    {
    public:
        Core::CBlockIndex *pindex;
        int64_t nValue;
        bool fSpent;
        Wallet::CWalletTx* ptx;
        int nOut;
        CTxDump(Wallet::CWalletTx* ptx = NULL, int nOut = -1)
        {
            pindex = NULL;
            nValue = 0;
            fSpent = false;
            this->ptx = ptx;
            this->nOut = nOut;
        }
    };

    Value rescan(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "rescan\n"
                "Rescans the database for relevant wallet transactions.");

        pwalletMain->ScanForWalletTransactions(Core::pindexGenesisBlock, true);

        return "Wallet Rescanning Complete";
    }

    Value importprivkey(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 1 || params.size() > 2)
            throw runtime_error(
                "importprivkey <PrivateKey> [label]\n"
                "Adds a private key (as returned by dumpprivkey) to your wallet.");

        string strSecret = params[0].get_str();
        string strLabel = "";
        if (params.size() > 1)
            strLabel = params[1].get_str();
        Wallet::NexusSecret vchSecret;
        bool fGood = vchSecret.SetString(strSecret);

        if (!fGood) throw JSONRPCError(-5,"Invalid private key");
        if (pwalletMain->IsLocked())
            throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
        if (Wallet::fWalletUnlockMintOnly)
            throw JSONRPCError(-102, "Wallet is unlocked for minting only.");

        Wallet::CKey key;
        bool fCompressed;
        Wallet::CSecret secret = vchSecret.GetSecret(fCompressed);
        key.SetSecret(secret, fCompressed);
        Wallet::NexusAddress vchAddress = Wallet::NexusAddress(key.GetPubKey());

        {
            LOCK2(Core::cs_main, pwalletMain->cs_wallet);

            pwalletMain->MarkDirty();
            pwalletMain->SetAddressBookName(vchAddress, strLabel);

            if (!pwalletMain->AddKey(key))
                throw JSONRPCError(-4,"Error adding key to wallet");
        }

        MainFrameRepaint();

        return Value::null;
    }

    Value dumpprivkey(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 1)
            throw runtime_error(
                "dumpprivkey <NexusAddress>\n"
                "Reveals the private key corresponding to <NexusAddress>.");

        string strAddress = params[0].get_str();
        Wallet::NexusAddress address;
        if (!address.SetString(strAddress))
            throw JSONRPCError(-5, "Invalid Nexus address");
        if (pwalletMain->IsLocked())
            throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
        if (Wallet::fWalletUnlockMintOnly) // Nexus: no dumpprivkey in mint-only mode
            throw JSONRPCError(-102, "Wallet is unlocked for minting only.");
        Wallet::CSecret vchSecret;
        bool fCompressed;
        if (!pwalletMain->GetSecret(address, vchSecret, fCompressed))
            throw JSONRPCError(-4,"Private key for address " + strAddress + " is not known");
        return Wallet::NexusSecret(vchSecret, fCompressed).ToString();
    }

    /** Import List of Private Keys amd if they import properly or fail with their own code int he output sequenc **/
    Value importkeys(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 1)
        {
            throw runtime_error(
                "importkeys\n"
                "The account and keypair need to \n"
                "You need to list the imported keys in a JSON array of {[account],[privatekey]}\n");
        }
            /** Make sure the Wallet is Unlocked fully before proceeding. **/
            if (pwalletMain->IsLocked())
                throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
            if (Wallet::fWalletUnlockMintOnly)
                throw JSONRPCError(-102, "Wallet is unlocked for minting only.");

            /** Establish the JSON Object from the Parameters. **/
            Object entry = params[0].get_obj();

            /** Establish a return value JSON object for Error Reporting. **/
            Object response;

            /** Import the Keys one by One. **/
            for(int nIndex = 0; nIndex < entry.size(); nIndex++) {

                Wallet::NexusSecret vchSecret;
                if(!vchSecret.SetString(entry[nIndex].value_.get_str())){
                    response.push_back(Pair(entry[nIndex].name_, "Code -5 Invalid Key"));

                    continue;
                }

                Wallet::CKey key;
                bool fCompressed;
                Wallet::CSecret secret = vchSecret.GetSecret(fCompressed);
                key.SetSecret(secret, fCompressed);
                Wallet::NexusAddress vchAddress = Wallet::NexusAddress(key.GetPubKey());

                {
                    LOCK2(Core::cs_main, pwalletMain->cs_wallet);

                    pwalletMain->MarkDirty();
                    pwalletMain->SetAddressBookName(vchAddress, entry[nIndex].name_);

                    if (!pwalletMain->AddKey(key))
                    {
                        response.push_back(Pair(entry[nIndex].name_, "Code -4 Error Adding to Wallet"));

                        continue;
                    }
                }

                response.push_back(Pair(entry[nIndex].name_, "Successfully Imported"));
            }

            MainFrameRepaint();

            return response;
    }

    /** Export the private keys of the current UTXO values.
        This will allow the importing and exporting of private keys much easier. **/
    Value exportkeys(const Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 0)
            throw runtime_error(
                "exportkeys\n"
                "This command dumps the private keys and account names of all unspent outputs.\n"
                "This allows the easy dumping and importing of all private keys on the system\n");

        /** Disallow the exporting of private keys if the encryption key is not available in the memory. **/
        if (pwalletMain->IsLocked())
            throw JSONRPCError(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.");
        if (Wallet::fWalletUnlockMintOnly) // Nexus: no dumpprivkey in mint-only mode
            throw JSONRPCError(-102, "Wallet is unlocked for minting only.");

        /** Compile the list of available Nexus Addresses and their according Balances. **/
        map<Wallet::NexusAddress, int64_t> mapAddresses;
        if(!pwalletMain->AvailableAddresses((unsigned int)GetUnifiedTimestamp(), mapAddresses))
            throw JSONRPCError(-3, "Error Extracting the Addresses from Wallet File. Please Try Again.");

        /** Loop all entries of the memory map to compile the list of account names and their addresses.
            JSON object format is a reflection of the import and export options. **/
        Object entry;
        for (map<Wallet::NexusAddress, int64_t>::iterator it = mapAddresses.begin(); it != mapAddresses.end(); ++it)
        {
            /** Extract the Secret key from the Wallet. **/
            Wallet::CSecret vchSecret;
            bool fCompressed;
            if (!pwalletMain->GetSecret(it->first, vchSecret, fCompressed))
                throw JSONRPCError(-4,"Private key for address " + it->first.ToString() + " is not known");

            /** Extract the account name from the address book. **/
            string strAccount;
            if(!pwalletMain->mapAddressBook.count(it->first))
                strAccount = "Default";
            else
                strAccount = pwalletMain->mapAddressBook[it->first];

            /** Compile the Secret Key and account information into a listed pair. **/
            string strSecret = Wallet::NexusSecret(vchSecret, fCompressed).ToString();
            entry.push_back(Pair(strAccount, strSecret));
        }

        return entry;
    }

}
