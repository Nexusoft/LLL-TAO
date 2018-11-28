/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_WALLET_H
#define NEXUS_LEGACY_WALLET_WALLET_H

#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include <LLC/include/key.h>
#include <LLC/types/uint1024.h>

#include <TAO/Ledger/include/global.h>

#include <TAO/Legacy/wallet/cryptokeystore.h>

#include <Util/include/allocators.h> /* for SecureString */

/******************************************/
/*                                        */
/* Needs updates for CBlock, CBlockIndex, */
/* CBlockLocator, MoneyRange()            */
/*                                        */
/******************************************/

namespace Legacy
{
    
    namespace Types
    {
        /* forward declarations */    
        class CScript;
        class CTransaction;
        class CTxIn;
        class CTxOut;
        class NexusAddress;
    }

    namespace Wallet
    {
        /* forward declarations */
        class CKeyPool;
        class COutput;
        class CReserveKey;
        class CWalletDB;
        class CWalletTx;

        extern bool fWalletUnlockMintOnly;


        /** (client) version numbers for particular wallet features.  **/
        enum WalletFeature
        {
            FEATURE_BASE = 10000,
            FEATURE_LATEST = 10000
        };


        /** MasterKeyMap is type alias defining a map for storing master keys by key Id. **/
        using MasterKeyMap = std::map<uint32_t, CMasterKey>;


        /** @class CWallet
         *
         *  A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
         * and provides the ability to create new transactions.
         **/
        class CWallet : public CCryptoKeyStore
        {
        private:
            bool SelectCoinsMinConf(int64_t nTargetValue, uint32_t nSpendTime, int nConfMine, int nConfTheirs, std::set<std::pair<const CWalletTx*,uint32_t> >& setCoinsRet, int64_t& nValueRet) const;
            bool SelectCoins(int64_t nTargetValue, uint32_t nSpendTime, std::set<std::pair<const CWalletTx*,uint32_t> >& setCoinsRet, int64_t& nValueRet) const;

            CWalletDB *pwalletdbEncryption;

            // the current wallet version: clients below this version are not able to load the wallet
            int nWalletVersion;

            // the maxmimum wallet format version: memory-only variable that specifies to what version this wallet may be upgraded
            int nWalletMaxVersion;

            MasterKeyMap mapMasterKeys;
            uint32_t nMasterKeyMaxID;

        public:
            /** Mutex for thread concurrency across wallet operations **/
            mutable std::recursive_mutex cs_wallet;

            /** Flag indicating whether or not wallet is backed by a wallet database. When true, strWalletFile contains database file name. **/
            bool fFileBacked;

            /** File name of database file backing wallet when fFileBacked is true. **/
            std::string strWalletFile;

            /** The key pool contained by this wallet **/
            CKeyPool keyPool;

            /** This default public key value for this wallet. **/
            std::vector<uint8_t> vchDefaultKey;

            std::map<uint512_t, CWalletTx> mapWallet;
            std::vector<uint512_t> vWalletUpdated;

            std::map<uint1024_t, int> mapRequestCount;

            std::map<Legacy::Types::NexusAddress, std::string> mapAddressBook;


            /** Constructor
             *
             *  Initializes a wallet instance for FEATURE_BASE that is not file backed.
             *
             **/
            CWallet() :
                nWalletVersion(FEATURE_BASE),
                nWalletMaxVersion(FEATURE_BASE),
                fFileBacked(false),
                nMasterKeyMaxID(0),
                pwalletdbEncryption(nullptr);
            { }


            /** Constructor
             *
             *  Initializes a wallet instance for FEATURE_BASE that is file backed. 
             *
             *  This constructor only initializes the wallet and does not load it from the 
             *  data store.
             *
             *  @param[in] strWalletFileIn The wallet database file name that backs this wallet.
             *
             *  @see LoadWallet()
             *
             **/
            CWallet(std::string strWalletFileIn) :
                nWalletVersion(FEATURE_BASE),
                nWalletMaxVersion(FEATURE_BASE),
                strWalletFile(strWalletFileIn),
                fFileBacked(true),
                nMasterKeyMaxID(0),
                pwalletdbEncryption(nullptr);
            { }


        /*----------------------------------------------------------------------------------------*/
        /*  Wallet General                                                                        */
        /*----------------------------------------------------------------------------------------*/

            /** CanSupportFeature
             *
             *  Check whether we are allowed to upgrade (or already support) to the named feature
             *
             *  @param[in] wf The feature to check
             *
             *  @return true if key assigned
             *
             **/
            inline bool CanSupportFeature(const enum Legacy::Wallet::WalletFeature wf) const 
            { 
                return nWalletMaxVersion >= wf; 
            }


            /** SetMinVersion
             *
             *  Assign the minimum supported version for the wallet. Older database versions will not load.
             *  If the wallet is file backed, any version update will also update the version recorded in the data store.
             *
             *  @param[in] nVersion The new minimum version
             *
             *  @param[in] fForceLatest If true, nVersion higher than current max version will upgrade to FEATURE_LATEST
             *                          If false (default), nVersion higher than current max will assign that version
             *
             *  @return true if version assigned successfully
             *
             */
            bool SetMinVersion(const enum Legacy::Wallet::WalletFeature nVersion, const bool fForceLatest = false);


            /** SetMaxVersion
             *
             *  Assign the maximum version we're allowed to upgrade to. 
             *  That this does not immediately imply upgrading to that format.
             *
             *  @param[in] nVersion The new maximum version
             *
             *  @return true if version assigned successfully
             *
             */
            bool SetMaxVersion(const int nVersion);


            /** GetVersion
             *
             *  Get the current wallet format (the oldest version guaranteed to understand this wallet).
             *
             *  @return current wallet version
             *
             */
            inline int GetVersion() const { return nWalletVersion; }


            /** IsFileBacked
             *
             *  @return true if wallet backed by a wallet database
             *
             */
            inline bool IsFileBacked() const { return fFileBacked; }


            /** LoadWallet
             *
             *  Loads all data for a file backed wallet from the database.
             *
             *  The first time LoadWallet is called the wallet file will not exist and
             *  will be created. It is new, so there is no data to retrieve. The 
             *  wallet will not have a default key (vchDefaultKey is empty) and one
             *  must be assigned. It also will not have values for min version or max version. 
             *  In this case fFirstRunRet will be set true to indicate a new wallet.
             * 
             *  @param[out] fFirstRunRet true if new wallet that needs a default key
             *
             *  @return Value from Legacy::Wallet::DBErrors, DB_LOAD_OK on success
             *
             */
            int LoadWallet(bool& fFirstRunRet);


        /*----------------------------------------------------------------------------------------*/
        /*  Keystore Implementation                                                               */
        /*----------------------------------------------------------------------------------------*/

            /** AddCryptedKey
             *
             *  Add a public/encrypted private key pair to the key store. 
             *  Key pair must be created from the same master key used to create any other key pairs in the store.
             *  Key store must have encryption active.
             *
             *  Wallet overrides this method to also store the key in the wallet database for file backed wallets.
             *
             *  @param[in] vchPubKey The public key to add
             *
             *  @param[in] vchCryptedSecret The encrypted private key
             *
             *  @return true if key successfully added
             *
             **/
            bool AddCryptedKey(const std::vector<uint8_t> &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret) override;


            /** AddKey
             *
             *  Add a key to the key store. 
             *
             *  Wallet overrides this method to only add the key if wallet is unencrypted (returns false if encrypted wallet).
             *  Also stores the key in the wallet database for file backed wallets.
             *
             *  @param[in] key The key to add
             *
             *  @return true if key successfully added
             *
             **/
            bool AddKey(const LLC::ECKey& key) override;


            /** AddCScript
             *
             *  Add a script to the key store. 
             *
             *  Wallet overrides this method to also store the script in the wallet database for file backed wallets.
             *
             *  @param[in] redeemScript The script to add
             *
             *  @return true if script was successfully added
             *
             **/
            bool AddCScript(const Legacy::Types::CScript& redeemScript) override;


        /*----------------------------------------------------------------------------------------*/
        /*  Load Wallet                                                                           */
        /*----------------------------------------------------------------------------------------*/
            /** LoadMinVersion
             *
             *  Assigns the minimum supported version without updating the data store (for file backed wallet).
             *  For use by LoadWallet.
             *
             *  @param[in] nVersion The new minimum version
             *
             *  @return true if version assigned successfully
             *
             *  @see CWalletDB::LoadWallet
             *
             **/
            bool LoadMinVersion(const int nVersion);


            /** LoadMasterKey
             *
             *  Loads a master key into the wallet, identified by its key Id.
             *  For use by LoadWallet.
             *
             *  @param[in] nMasterKeyId The key Id of the master key to add
             *
             *  @param[in] kMasterKey The master key to add
             *
             *  @return true if master key was successfully added
             *
             *  @see CWalletDB::LoadWallet
             *
             **/
            bool LoadMasterKey(const uint32_t nMasterKeyId, const CMasterKey& kMasterKey);


            /** LoadCryptedKey
             *
             *  Add a public/encrypted private key pair to the key store without updating the data store (for file backed wallet). 
             *  For use by LoadWallet.
             *
             *  @param[in] vchPubKey The public key to add
             *
             *  @param[in] vchCryptedSecret The encrypted private key
             *
             *  @return true if key successfully added
             *
             *  @see CWalletDB::LoadWallet
             *
             **/
            bool LoadCryptedKey(const std::vector<uint8_t> &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret);


            /** LoadKey
             *
             *  Add a key to the key store without updating the data store (for file backed wallet).
             *  For use by LoadWallet.
             *
             *  @param[in] key The key to add
             *
             *  @return true if key successfully added
             *
             *  @see CWalletDB::LoadWallet
             *
             **/
            bool LoadKey(const LLC::ECKey& key);


            /** LoadCScript
             *
             *  Add a script to the key store without updating the data store (for file backed wallet).
             *  For use by LoadWallet.
             *
             *  @param[in] redeemScript The script to add
             *
             *  @return true if script was successfully added
             *
             *  @see CWalletDB::LoadWallet
             *
             **/
            bool LoadCScript(const Legacy::Types::CScript& redeemScript);


        /*----------------------------------------------------------------------------------------*/
        /*  Key Operations                                                                        */
        /*----------------------------------------------------------------------------------------*/
            /** GenerateNewKey
             *
             *  Generates a new key and adds it to the key store.
             *
             *  @return the public key value for the newly generated key
             *
             */
            std::vector<uint8_t> GenerateNewKey();


            /** SetDefaultKey
             *
             *  Assigns a new default key to this wallet. The key itself
             *  should already have been added to the wallet.
             *
             *  @param[in] vchPubKey The key to make default
             *
             *  @return true if setting default key successful
             *
             */
            bool SetDefaultKey(const std::vector<uint8_t> &vchPubKey);


            /** GetKeyPool
             *
             *  Retrieve a reference to the key pool for this wallet.
             *
             *  @return this wallet's key pool
             *
             */
            inline CKeyPool& GetKeyPool() const { return keyPool; }


        /*----------------------------------------------------------------------------------------*/
        /*  Encryption and Passphrase                                                             */
        /*----------------------------------------------------------------------------------------*/
            /** Unlock
             *
             *  Attempt to unlock an encrypted wallet using the passphrase provided. 
             *  Encrypted wallet cannot be used until unlocked by providing the passphrase used to encrypt it.
             *
             *  @param[in] strWalletPassphrase The wallet's passphrase
             *
             *  @return true if wallet was locked, passphrase matches the one used to encrypt it, and unlock is successful
             *
             */
            bool Unlock(const SecureString& strWalletPassphrase);


            /** EncryptWallet
             *
             *  Encrypts the wallet in both memory and file backing, assigning a passphrase that will be required 
             *  to unlock and access the wallet. Will not work if wallet already encrypted.
             *
             *  @param[in] strWalletPassphrase The wallet's passphrase
             *
             *  @return true if encryption successful
             *
             */
            bool EncryptWallet(const SecureString& strWalletPassphrase);


            /** ChangeWalletPassphrase
             *
             *  Replaces the existing wallet passphrase with a new one.
             *
             *  @param[in] strOldWalletPassphrase The old passphrase. Must match or change will be unsuccessful
             *
             *  @param[in] strNewWalletPassphrase The new passphrase to use
             *
             *  @return true if old passphrase matched and passphrase changed successfully
             *
             */
            bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);


        /*----------------------------------------------------------------------------------------*/
        /*  Balance                                                                               */
        /*----------------------------------------------------------------------------------------*/
            int64_t GetBalance() const;
            int64_t GetUnconfirmedBalance() const;
            int64_t GetStake() const;
            int64_t GetNewMint() const;

            void AvailableCoins(uint32_t nSpendTime, std::vector<COutput>& vCoins, bool fOnlyConfirmed) const;

        /*----------------------------------------------------------------------------------------*/
        /*  Wallet Transactions                                                                   */
        /*----------------------------------------------------------------------------------------*/
            void MarkDirty();
            bool AddToWallet(const CWalletTx& wtxIn);
            bool AddToWalletIfInvolvingMe(const Legacy::Types::CTransaction& tx, const Core::CBlock* pblock, bool fUpdate = false, bool fFindBlock = false);
            bool EraseFromWallet(uint512_t hash);
            void WalletUpdateSpent(const Legacy::Types::CTransaction& prevout);
            int ScanForWalletTransactions(Core::CBlockIndex* pindexStart, bool fUpdate = false);
            int ScanForWalletTransaction(const uint512_t& hashTx);
            void ReacceptWalletTransactions();
            void ResendWalletTransactions();

            bool CreateTransaction(const std::vector<std::pair<Legacy::CScript, int64_t> >& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet);
            bool CreateTransaction(Legacy::Types::CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet);
            bool AddCoinstakeInputs(Legacy::Types::CTransaction& txNew);
            bool CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey);
            std::string SendMoney(Legacy::Types::CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, bool fAskFee=false);
            std::string SendToNexusAddress(const Legacy::Types::NexusAddress& address, int64_t nValue, CWalletTx& wtxNew, bool fAskFee=false);



        /*----------------------------------------------------------------------------------------*/
        /*  Blockchain Transactions                                                               */
        /*----------------------------------------------------------------------------------------*/
            bool IsMine(const Legacy::Types::CTxIn& txin) const;
            int64_t GetDebit(const Legacy::Types::CTxIn& txin) const;
            bool IsMine(const Legacy::Types::CTxOut& txout) const;
            int64_t GetCredit(const Legacy::Types::CTxOut& txout) const;
            bool IsChange(const Legacy::Types::CTxOut& txout) const;
            int64_t GetChange(const Legacy::Types::CTxOut& txout) const;
            bool IsMine(const Legacy::Types::CTransaction& tx) const;
            bool IsFromMe(const Legacy::Types::CTransaction& tx) const;
            int64_t GetDebit(const Legacy::Types::CTransaction& tx) const;
            int64_t GetCredit(const Legacy::Types::CTransaction& tx) const;
            int64_t GetChange(const Legacy::Types::CTransaction& tx) const;
            bool GetTransaction(const uint512_t &hashTx, CWalletTx& wtx);
            void UpdatedTransaction(const uint512_t &hashTx);

            void SetBestChain(const Core::CBlockLocator& loc);


        /*----------------------------------------------------------------------------------------*/
        /*  Address Book                                                                          */
        /*----------------------------------------------------------------------------------------*/
            bool AvailableAddresses(uint32_t nSpendTime, std::map<Legacy::Types::NexusAddress, int64_t>& mapAddresses, bool fOnlyConfirmed = false) const;
            bool SetAddressBookName(const Legacy::Types::NexusAddress& address, const std::string& strName);
            bool DelAddressBookName(const Legacy::Types::NexusAddress& address);

            void Inventory(const uint1024_t &hash)
            {
                {
                    LOCK(cs_wallet);
                    std::map<uint1024_t, int>::iterator mi = mapRequestCount.find(hash);
                    if (mi != mapRequestCount.end())
                        (*mi).second++;
                }
            }

            void FixSpentCoins(int& nMismatchSpent, int64_t& nBalanceInQuestion, bool fCheckOnly = false);
            void DisableTransaction(const Legacy::Types::CTransaction &tx);
        };


        bool GetWalletFile(CWallet* pwallet, std::string &strWalletFileOut);

    }
}

#endif
