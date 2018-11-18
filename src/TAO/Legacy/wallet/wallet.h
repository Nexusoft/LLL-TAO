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
    
    /* forward declarations */    
    namespace Types
    {
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

        /** (client) version numbers for particular wallet features.  */
        enum WalletFeature
        {
            FEATURE_BASE = 10000,
            FEATURE_LATEST = 10000
        };

        /** A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
        * and provides the ability to create new transactions.
        */
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

        public:
            mutable std::recursive_mutex cs_wallet;

            bool fFileBacked;
            std::string strWalletFile;

            std::set<int64_t> setKeyPool;


            typedef std::map<uint32_t, CMasterKey> MasterKeyMap;
            MasterKeyMap mapMasterKeys;
            uint32_t nMasterKeyMaxID;

            CWallet()
            {
                nWalletVersion = FEATURE_BASE;
                nWalletMaxVersion = FEATURE_BASE;
                fFileBacked = false;
                nMasterKeyMaxID = 0;
                pwalletdbEncryption = NULL;
            }
            CWallet(std::string strWalletFileIn)
            {
                nWalletVersion = FEATURE_BASE;
                nWalletMaxVersion = FEATURE_BASE;
                strWalletFile = strWalletFileIn;
                fFileBacked = true;
                nMasterKeyMaxID = 0;
                pwalletdbEncryption = NULL;
            }

            std::map<uint512_t, CWalletTx> mapWallet;
            std::vector<uint512_t> vWalletUpdated;

            std::map<uint1024_t, int> mapRequestCount;

            std::map<Legacy::Types::NexusAddress, std::string> mapAddressBook;

            std::vector<uint8_t> vchDefaultKey;

            // check whether we are allowed to upgrade (or already support) to the named feature
            bool CanSupportFeature(enum WalletFeature wf) { return nWalletMaxVersion >= wf; }

            // keystore implementation
            // Generate a new key
            std::vector<uint8_t> GenerateNewKey();
            // Adds a key to the store, and saves it to disk.
            bool AddKey(const LLC::ECKey& key);
            // Adds a key to the store, without saving it to disk (used by LoadWallet)
            bool LoadKey(const LLC::ECKey& key) { return CCryptoKeyStore::AddKey(key); }

            bool LoadMinVersion(int nVersion) { nWalletVersion = nVersion; nWalletMaxVersion = std::max(nWalletMaxVersion, nVersion); return true; }

            // Adds an encrypted key to the store, and saves it to disk.
            bool AddCryptedKey(const std::vector<uint8_t> &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret);
            // Adds an encrypted key to the store, without saving it to disk (used by LoadWallet)
            bool LoadCryptedKey(const std::vector<uint8_t> &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret) { return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret); }
            bool AddCScript(const Legacy::Types::CScript& redeemScript);
            bool LoadCScript(const Legacy::Types::CScript& redeemScript) { return CCryptoKeyStore::AddCScript(redeemScript); }

            bool Unlock(const SecureString& strWalletPassphrase);
            bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);
            bool EncryptWallet(const SecureString& strWalletPassphrase);

            void MarkDirty();
            bool AddToWallet(const CWalletTx& wtxIn);
            bool AddToWalletIfInvolvingMe(const Legacy::Types::CTransaction& tx, const Core::CBlock* pblock, bool fUpdate = false, bool fFindBlock = false);
            bool EraseFromWallet(uint512_t hash);
            void WalletUpdateSpent(const Legacy::Types::CTransaction& prevout);
            int ScanForWalletTransactions(Core::CBlockIndex* pindexStart, bool fUpdate = false);
            int ScanForWalletTransaction(const uint512_t& hashTx);
            void ReacceptWalletTransactions();
            void ResendWalletTransactions();
            int64_t GetBalance() const;
            int64_t GetUnconfirmedBalance() const;
            int64_t GetStake() const;
            int64_t GetNewMint() const;

            void AvailableCoins(uint32_t nSpendTime, std::vector<COutput>& vCoins, bool fOnlyConfirmed) const;
            bool AvailableAddresses(uint32_t nSpendTime, std::map<Legacy::Types::NexusAddress, int64_t>& mapAddresses, bool fOnlyConfirmed = false) const;

            bool CreateTransaction(const std::vector<std::pair<Legacy::CScript, int64_t> >& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet);
            bool CreateTransaction(Legacy::Types::CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet);
            bool AddCoinstakeInputs(Legacy::Types::CTransaction& txNew);
            bool CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey);
            std::string SendMoney(Legacy::Types::CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, bool fAskFee=false);
            std::string SendToNexusAddress(const Legacy::Types::NexusAddress& address, int64_t nValue, CWalletTx& wtxNew, bool fAskFee=false);

            bool NewKeyPool();
            bool TopUpKeyPool();
            int64_t AddReserveKey(const CKeyPool& keypool);
            void ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool);
            void KeepKey(int64_t nIndex);
            void ReturnKey(int64_t nIndex);
            bool GetKeyFromPool(std::vector<uint8_t> &key, bool fAllowReuse=true);
            int64_t GetOldestKeyPoolTime();
            void GetAllReserveAddresses(std::set<Legacy::Types::NexusAddress>& setAddress);

            bool IsMine(const Legacy::Types::CTxIn& txin) const;
            int64_t GetDebit(const Legacy::Types::CTxIn& txin) const;
            bool IsMine(const Legacy::Types::CTxOut& txout) const
            {
                return Wallet::IsMine(*this, txout.scriptPubKey);
            }
            int64_t GetCredit(const Legacy::Types::CTxOut& txout) const
            {
                if (!Core::MoneyRange(txout.nValue))
                    throw std::runtime_error("CWallet::GetCredit() : value out of range");
                return (IsMine(txout) ? txout.nValue : 0);
            }
            bool IsChange(const Legacy::Types::CTxOut& txout) const;
            int64_t GetChange(const Legacy::Types::CTxOut& txout) const
            {
                if (!Core::MoneyRange(txout.nValue))
                    throw std::runtime_error("CWallet::GetChange() : value out of range");
                return (IsChange(txout) ? txout.nValue : 0);
            }
            bool IsMine(const Legacy::Types::CTransaction& tx) const
            {
                for(const Legacy::Types::CTxOut& txout : tx.vout)
                    if (IsMine(txout))
                        return true;
                return false;
            }
            bool IsFromMe(const Legacy::Types::CTransaction& tx) const
            {
                return (GetDebit(tx) > 0);
            }
            int64_t GetDebit(const Legacy::Types::CTransaction& tx) const
            {
                int64_t nDebit = 0;
                for(const Legacy::Types::CTxIn& txin : tx.vin)
                {
                    nDebit += GetDebit(txin);
                    if (!Core::MoneyRange(nDebit))
                        throw std::runtime_error("CWallet::GetDebit() : value out of range");
                }
                return nDebit;
            }
            int64_t GetCredit(const Legacy::Types::CTransaction& tx) const
            {
                int64_t nCredit = 0;
                for(const Legacy::Types::CTxOut& txout : tx.vout)
                {
                    nCredit += GetCredit(txout);
                    if (!Core::MoneyRange(nCredit))
                        throw std::runtime_error("CWallet::GetCredit() : value out of range");
                }
                return nCredit;
            }
            int64_t GetChange(const Legacy::Types::CTransaction& tx) const
            {
                int64_t nChange = 0;
                for(const Legacy::Types::CTxOut& txout : tx.vout)
                {
                    nChange += GetChange(txout);
                    if (!Core::MoneyRange(nChange))
                        throw std::runtime_error("CWallet::GetChange() : value out of range");
                }
                return nChange;
            }
            void SetBestChain(const Core::CBlockLocator& loc);

            int LoadWallet(bool& fFirstRunRet);

            bool SetAddressBookName(const Legacy::Types::NexusAddress& address, const std::string& strName);

            bool DelAddressBookName(const Legacy::Types::NexusAddress& address);

            void UpdatedTransaction(const uint512_t &hashTx)
            {
                {
                    LOCK(cs_wallet);
                    vWalletUpdated.push_back(hashTx);
                }
            }

            void PrintWallet(const Core::CBlock& block);

            void Inventory(const uint1024_t &hash)
            {
                {
                    LOCK(cs_wallet);
                    std::map<uint1024_t, int>::iterator mi = mapRequestCount.find(hash);
                    if (mi != mapRequestCount.end())
                        (*mi).second++;
                }
            }

            int GetKeyPoolSize()
            {
                return setKeyPool.size();
            }

            bool GetTransaction(const uint512_t &hashTx, CWalletTx& wtx);

            bool SetDefaultKey(const std::vector<uint8_t> &vchPubKey);

            // signify that a particular wallet feature is now used. this may change nWalletVersion and nWalletMaxVersion if those are lower
            bool SetMinVersion(enum WalletFeature, CWalletDB* pwalletdbIn = NULL, bool fExplicit = false);

            // change which version we're allowed to upgrade to (note that this does not immediately imply upgrading to that format)
            bool SetMaxVersion(int nVersion);

            // get the current wallet format (the oldest client version guaranteed to understand this wallet)
            int GetVersion() { return nWalletVersion; }

            void FixSpentCoins(int& nMismatchSpent, int64_t& nBalanceInQuestion, bool fCheckOnly = false);
            void DisableTransaction(const Legacy::Types::CTransaction &tx);
        };


        bool GetWalletFile(CWallet* pwallet, std::string &strWalletFileOut);

    }
}

#endif
