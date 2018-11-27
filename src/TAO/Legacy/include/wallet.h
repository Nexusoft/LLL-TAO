/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_WALLET_H
#define NEXUS_WALLET_H

#include "../core/core.h"

#include "key.h"
#include "keystore.h"
#include "script.h"

namespace Wallet
{
    extern bool fWalletUnlockMintOnly;

    class CWalletTx;
    class CReserveKey;
    class CWalletDB;
    class COutput;

    /** (client) version numbers for particular wallet features.  */
    enum WalletFeature
    {
        FEATURE_BASE = 10000,
        FEATURE_LATEST = 10000
    };


    /** A key pool entry */
    class CKeyPool
    {
    public:
        int64_t nTime;
        std::vector<uint8_t> vchPubKey;

        CKeyPool()
        {
            nTime = GetUnifiedTimestamp();
        }

        CKeyPool(const std::vector<uint8_t>& vchPubKeyIn)
        {
            nTime = GetUnifiedTimestamp();
            vchPubKey = vchPubKeyIn;
        }

        IMPLEMENT_SERIALIZE
        (
            if (!(nSerType & SER_GETHASH))
                READWRITE(nVersion);
            READWRITE(nTime);
            READWRITE(vchPubKey);
        )
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
        mutable CCriticalSection cs_wallet;

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

        std::map<NexusAddress, std::string> mapAddressBook;

        std::vector<uint8_t> vchDefaultKey;

        // check whether we are allowed to upgrade (or already support) to the named feature
        bool CanSupportFeature(enum WalletFeature wf) { return nWalletMaxVersion >= wf; }

        // keystore implementation
        // Generate a new key
        std::vector<uint8_t> GenerateNewKey();
        // Adds a key to the store, and saves it to disk.
        bool AddKey(const ECKey& key);
        // Adds a key to the store, without saving it to disk (used by LoadWallet)
        bool LoadKey(const ECKey& key) { return CCryptoKeyStore::AddKey(key); }

        bool LoadMinVersion(int nVersion) { nWalletVersion = nVersion; nWalletMaxVersion = std::max(nWalletMaxVersion, nVersion); return true; }

        // Adds an encrypted key to the store, and saves it to disk.
        bool AddCryptedKey(const std::vector<uint8_t> &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret);
        // Adds an encrypted key to the store, without saving it to disk (used by LoadWallet)
        bool LoadCryptedKey(const std::vector<uint8_t> &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret) { return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret); }
        bool AddCScript(const CScript& redeemScript);
        bool LoadCScript(const CScript& redeemScript) { return CCryptoKeyStore::AddCScript(redeemScript); }

        bool Unlock(const SecureString& strWalletPassphrase);
        bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);
        bool EncryptWallet(const SecureString& strWalletPassphrase);

        void MarkDirty();
        bool AddToWallet(const CWalletTx& wtxIn);
        bool AddToWalletIfInvolvingMe(const Core::CTransaction& tx, const Core::CBlock* pblock, bool fUpdate = false, bool fFindBlock = false);
        bool EraseFromWallet(uint512_t hash);
        void WalletUpdateSpent(const Core::CTransaction& prevout);
        int ScanForWalletTransactions(Core::CBlockIndex* pindexStart, bool fUpdate = false);
        int ScanForWalletTransaction(const uint512_t& hashTx);
        void ReacceptWalletTransactions();
        void ResendWalletTransactions();
        int64_t GetBalance() const;
        int64_t GetUnconfirmedBalance() const;
        int64_t GetStake() const;
        int64_t GetNewMint() const;

        void AvailableCoins(uint32_t nSpendTime, std::vector<COutput>& vCoins, bool fOnlyConfirmed) const;
        bool AvailableAddresses(uint32_t nSpendTime, std::map<NexusAddress, int64_t>& mapAddresses, bool fOnlyConfirmed = false) const;

        bool CreateTransaction(const std::vector<std::pair<CScript, int64_t> >& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet);
        bool CreateTransaction(CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet);
        bool AddCoinstakeInputs(Core::CTransaction& txNew);
        bool CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey);
        std::string SendMoney(CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, bool fAskFee=false);
        std::string SendToNexusAddress(const NexusAddress& address, int64_t nValue, CWalletTx& wtxNew, bool fAskFee=false);

        bool NewKeyPool();
        bool TopUpKeyPool();
        int64_t AddReserveKey(const CKeyPool& keypool);
        void ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool);
        void KeepKey(int64_t nIndex);
        void ReturnKey(int64_t nIndex);
        bool GetKeyFromPool(std::vector<uint8_t> &key, bool fAllowReuse=true);
        int64_t GetOldestKeyPoolTime();
        void GetAllReserveAddresses(std::set<NexusAddress>& setAddress);

        bool IsMine(const Core::CTxIn& txin) const;
        int64_t GetDebit(const Core::CTxIn& txin) const;
        bool IsMine(const Core::CTxOut& txout) const
        {
            return Wallet::IsMine(*this, txout.scriptPubKey);
        }
        int64_t GetCredit(const Core::CTxOut& txout) const
        {
            if (!Core::MoneyRange(txout.nValue))
                throw std::runtime_error("CWallet::GetCredit() : value out of range");
            return (IsMine(txout) ? txout.nValue : 0);
        }
        bool IsChange(const Core::CTxOut& txout) const;
        int64_t GetChange(const Core::CTxOut& txout) const
        {
            if (!Core::MoneyRange(txout.nValue))
                throw std::runtime_error("CWallet::GetChange() : value out of range");
            return (IsChange(txout) ? txout.nValue : 0);
        }
        bool IsMine(const Core::CTransaction& tx) const
        {
            for(const Core::CTxOut& txout : tx.vout)
                if (IsMine(txout))
                    return true;
            return false;
        }
        bool IsFromMe(const Core::CTransaction& tx) const
        {
            return (GetDebit(tx) > 0);
        }
        int64_t GetDebit(const Core::CTransaction& tx) const
        {
            int64_t nDebit = 0;
            for(const Core::CTxIn& txin : tx.vin)
            {
                nDebit += GetDebit(txin);
                if (!Core::MoneyRange(nDebit))
                    throw std::runtime_error("CWallet::GetDebit() : value out of range");
            }
            return nDebit;
        }
        int64_t GetCredit(const Core::CTransaction& tx) const
        {
            int64_t nCredit = 0;
            for(const Core::CTxOut& txout : tx.vout)
            {
                nCredit += GetCredit(txout);
                if (!Core::MoneyRange(nCredit))
                    throw std::runtime_error("CWallet::GetCredit() : value out of range");
            }
            return nCredit;
        }
        int64_t GetChange(const Core::CTransaction& tx) const
        {
            int64_t nChange = 0;
            for(const Core::CTxOut& txout : tx.vout)
            {
                nChange += GetChange(txout);
                if (!Core::MoneyRange(nChange))
                    throw std::runtime_error("CWallet::GetChange() : value out of range");
            }
            return nChange;
        }
        void SetBestChain(const Core::CBlockLocator& loc);

        int LoadWallet(bool& fFirstRunRet);

        bool SetAddressBookName(const NexusAddress& address, const std::string& strName);

        bool DelAddressBookName(const NexusAddress& address);

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
        void DisableTransaction(const Core::CTransaction &tx);
    };


    /** A key allocated from the key pool. */
    class CReserveKey
    {
    protected:
        CWallet* pwallet;
        int64_t nIndex;
        std::vector<uint8_t> vchPubKey;
    public:
        CReserveKey(CWallet* pwalletIn)
        {
            nIndex = -1;
            pwallet = pwalletIn;
        }

        ~CReserveKey()
        {
            if (!fShutdown)
                ReturnKey();
        }

        void ReturnKey();
        std::vector<uint8_t> GetReservedKey();
        void KeepKey();
    };


    /** A transaction with a bunch of additional info that only the owner cares about.
    * It includes any unrecorded transactions needed to link it back to the block chain.
    */
    class CWalletTx : public Core::CMerkleTx
    {
    private:
        const CWallet* pwallet;

    public:
        std::vector<Core::CMerkleTx> vtxPrev;
        std::map<std::string, std::string> mapValue;
        std::vector<std::pair<std::string, std::string> > vOrderForm;
        uint32_t fTimeReceivedIsTxTime;
        uint32_t nTimeReceived;  // time received by this node
        char fFromMe;
        std::string strFromAccount;
        std::vector<char> vfSpent; // which outputs are already spent

        // memory only
        mutable bool fDebitCached;
        mutable bool fCreditCached;
        mutable bool fAvailableCreditCached;
        mutable bool fChangeCached;
        mutable int64_t nDebitCached;
        mutable int64_t nCreditCached;
        mutable int64_t nAvailableCreditCached;
        mutable int64_t nChangeCached;

        CWalletTx()
        {
            Init(NULL);
        }

        CWalletTx(const CWallet* pwalletIn)
        {
            Init(pwalletIn);
        }

        CWalletTx(const CWallet* pwalletIn, const Core::CMerkleTx& txIn) : Core::CMerkleTx(txIn)
        {
            Init(pwalletIn);
        }

        CWalletTx(const CWallet* pwalletIn, const Core::CTransaction& txIn) : Core::CMerkleTx(txIn)
        {
            Init(pwalletIn);
        }

        void Init(const CWallet* pwalletIn)
        {
            pwallet = pwalletIn;
            vtxPrev.clear();
            mapValue.clear();
            vOrderForm.clear();
            fTimeReceivedIsTxTime = false;
            nTimeReceived = 0;
            fFromMe = false;
            strFromAccount.clear();
            vfSpent.clear();
            fDebitCached = false;
            fCreditCached = false;
            fAvailableCreditCached = false;
            fChangeCached = false;
            nDebitCached = 0;
            nCreditCached = 0;
            nAvailableCreditCached = 0;
            nChangeCached = 0;
        }

        IMPLEMENT_SERIALIZE
        (
            CWalletTx* pthis = const_cast<CWalletTx*>(this);
            if (fRead)
                pthis->Init(NULL);
            char fSpent = false;

            if (!fRead)
            {
                pthis->mapValue["fromaccount"] = pthis->strFromAccount;

                std::string str;
                for(char f : vfSpent)
                {
                    str += (f ? '1' : '0');
                    if (f)
                        fSpent = true;
                }
                pthis->mapValue["spent"] = str;
            }

            nSerSize += SerReadWrite(s, *(CMerkleTx*)this, nSerType, nVersion,ser_action);
            READWRITE(vtxPrev);
            READWRITE(mapValue);
            READWRITE(vOrderForm);
            READWRITE(fTimeReceivedIsTxTime);
            READWRITE(nTimeReceived);
            READWRITE(fFromMe);
            READWRITE(fSpent);

            if (fRead)
            {
                pthis->strFromAccount = pthis->mapValue["fromaccount"];

                if (mapValue.count("spent"))
                    for(char c : pthis->mapValue["spent"])
                        pthis->vfSpent.push_back(c != '0');
                else
                    pthis->vfSpent.assign(vout.size(), fSpent);
            }

            pthis->mapValue.erase("fromaccount");
            pthis->mapValue.erase("version");
            pthis->mapValue.erase("spent");
        )

        // marks certain txout's as spent
        // returns true if any update took place
        bool UpdateSpent(const std::vector<char>& vfNewSpent)
        {
            bool fReturn = false;
            for (uint32_t i = 0; i < vfNewSpent.size(); i++)
            {
                if (i == vfSpent.size())
                    break;

                if (vfNewSpent[i] && !vfSpent[i])
                {
                    vfSpent[i] = true;
                    fReturn = true;
                    fAvailableCreditCached = false;
                }
            }
            return fReturn;
        }

        // make sure balances are recalculated
        void MarkDirty()
        {
            fCreditCached = false;
            fAvailableCreditCached = false;
            fDebitCached = false;
            fChangeCached = false;
        }

        void BindWallet(CWallet *pwalletIn)
        {
            pwallet = pwalletIn;
            MarkDirty();
        }

        void MarkSpent(uint32_t nOut)
        {
            if (nOut >= vout.size())
                throw std::runtime_error("CWalletTx::MarkSpent() : nOut out of range");
            vfSpent.resize(vout.size());
            if (!vfSpent[nOut])
            {
                vfSpent[nOut] = true;
                fAvailableCreditCached = false;
            }
        }

        void MarkUnspent(uint32_t nOut)
        {
            if (nOut >= vout.size())
                throw std::runtime_error("CWalletTx::MarkUnspent() : nOut out of range");
            vfSpent.resize(vout.size());
            if (vfSpent[nOut])
            {
                vfSpent[nOut] = false;
                fAvailableCreditCached = false;
            }
        }

        bool IsSpent(uint32_t nOut) const
        {
            if (nOut >= vout.size())
                throw std::runtime_error("CWalletTx::IsSpent() : nOut out of range");
            if (nOut >= vfSpent.size())
                return false;
            return (!!vfSpent[nOut]);
        }

        int64_t GetDebit() const
        {
            if (vin.empty())
                return 0;
            if (fDebitCached)
                return nDebitCached;
            nDebitCached = pwallet->GetDebit(*this);
            fDebitCached = true;
            return nDebitCached;
        }

        int64_t GetCredit(bool fUseCache=true) const
        {
            // Must wait until coinbase / coinstake is safely deep enough in the chain before valuing it
            if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
                return 0;

            // GetBalance can assume transactions in mapWallet won't change
            if (fUseCache && fCreditCached)
                return nCreditCached;

            nCreditCached = pwallet->GetCredit(*this);
            fCreditCached = true;
            return nCreditCached;
        }

        int64_t GetAvailableCredit(bool fUseCache=true) const
        {
            // Must wait until coinbase is safely deep enough in the chain before valuing it
            if ((IsCoinBase() || IsCoinStake()) && GetBlocksToMaturity() > 0)
                return 0;

            //if (fUseCache && fAvailableCreditCached)
            //return nAvailableCreditCached;

            int64_t nCredit = 0;
            for (uint32_t i = 0; i < vout.size(); i++)
            {
                if (!IsSpent(i) && pwallet->IsMine(vout[i]) && vout[i].nValue > 0)
                {
                    const Core::CTxOut &txout = vout[i];
                    nCredit += pwallet->GetCredit(txout);
                    if (!Core::MoneyRange(nCredit))
                        throw std::runtime_error("CWalletTx::GetAvailableCredit() : value out of range");
                }
            }

            nAvailableCreditCached = nCredit;
            fAvailableCreditCached = true;
            return nCredit;
        }


        int64_t GetChange() const
        {
            if (fChangeCached)
                return nChangeCached;
            nChangeCached = pwallet->GetChange(*this);
            fChangeCached = true;
            return nChangeCached;
        }

        void GetAmounts(int64_t& nGeneratedImmature, int64_t& nGeneratedMature, std::list<std::pair<NexusAddress, int64_t> >& listReceived,
                        std::list<std::pair<NexusAddress, int64_t> >& listSent, int64_t& nFee, std::string& strSentAccount) const;

        void GetAccountAmounts(const std::string& strAccount, int64_t& nGenerated, int64_t& nReceived,
                            int64_t& nSent, int64_t& nFee) const;

        bool IsFromMe() const
        {
            return (GetDebit() > 0);
        }

        bool IsConfirmed() const
        {
            // Quick answer in most cases
            if (!IsFinal())
                return false;
            if (GetDepthInMainChain() >= 1)
                return true;
            if (!IsFromMe()) // using wtx's cached debit
                return false;

            // If no confirmations but it's from us, we can still
            // consider it confirmed if all dependencies are confirmed
            std::map<uint512_t, const Core::CMerkleTx*> mapPrev;
            std::vector<const Core::CMerkleTx*> vWorkQueue;
            vWorkQueue.reserve(vtxPrev.size()+1);
            vWorkQueue.push_back(this);

            for (uint32_t i = 0; i < vWorkQueue.size(); i++)
            {
                const CMerkleTx* ptx = vWorkQueue[i];

                if (!ptx->IsFinal())
                    return false;
                if (ptx->GetDepthInMainChain() >= 1)
                    continue;
                if (!pwallet->IsFromMe(*ptx))
                    return false;

                if (mapPrev.empty())
                {
                    for(const Core::CMerkleTx& tx : vtxPrev)
                        mapPrev[tx.GetHash()] = &tx;
                }

                for(const Core::CTxIn& txin : ptx->vin)
                {
                    if (!mapPrev.count(txin.prevout.hash))
                        return false;
                    vWorkQueue.push_back(mapPrev[txin.prevout.hash]);
                }
            }
            return true;
        }

        bool WriteToDisk();

        int64_t GetTxTime() const;
        int GetRequestCount() const;

        void AddSupportingTransactions(LLD::CIndexDB& indexdb);

        bool AcceptWalletTransaction(LLD::CIndexDB& indexdb, bool fCheckInputs=true);
        bool AcceptWalletTransaction();

        void RelayWalletTransaction(LLD::CIndexDB& indexdb);
        void RelayWalletTransaction();
    };


    /** Class to hold Private key binary data. */
    class CWalletKey
    {
    public:
        CPrivKey vchPrivKey;
        int64_t nTimeCreated;
        int64_t nTimeExpires;
        std::string strComment;
        //// todo: add something to note what created it (user, getnewaddress, change)
        ////   maybe should have a map<string, string> property map

        CWalletKey(int64_t nExpires=0)
        {
            nTimeCreated = (nExpires ? GetUnifiedTimestamp() : 0);
            nTimeExpires = nExpires;
        }

        IMPLEMENT_SERIALIZE
        (
            if (!(nSerType & SER_GETHASH))
                READWRITE(nVersion);
            READWRITE(vchPrivKey);
            READWRITE(nTimeCreated);
            READWRITE(nTimeExpires);
            READWRITE(strComment);
        )
    };



    /** Class to determine the value and depth of a specific transaction.
        Used for the Available Coins Method located in Wallet.cpp mainly.
        To be used for further purpose in the future. **/
    class COutput
    {
    public:
        const CWalletTx *tx;
        int i;
        int nDepth;

        COutput(const CWalletTx *txIn, int iIn, int nDepthIn)
        {
            tx = txIn; i = iIn; nDepth = nDepthIn;
        }

        std::string ToString() const
        {
            return strprintf("COutput(%s, %d, %d) [%s]", tx->GetHash().ToString().substr(0,10).c_str(), i, nDepth, FormatMoney(tx->vout[i].nValue).c_str());
        }

        void print() const
        {
            debug::log("%s\n", ToString().c_str());
        }
    };






    /** Account information.
    * Stored in wallet with key "acc"+string account name.
    */
    class CAccount
    {
    public:
        std::vector<uint8_t> vchPubKey;

        CAccount()
        {
            SetNull();
        }

        void SetNull()
        {
            vchPubKey.clear();
        }

        IMPLEMENT_SERIALIZE
        (
            if (!(nSerType & SER_GETHASH))
                READWRITE(nVersion);
            READWRITE(vchPubKey);
        )
    };



    /** Internal transfers.
    * Database key is acentry<account><counter>.
    */
    class CAccountingEntry
    {
    public:
        std::string strAccount;
        int64_t nCreditDebit;
        int64_t nTime;
        std::string strOtherAccount;
        std::string strComment;

        CAccountingEntry()
        {
            SetNull();
        }

        void SetNull()
        {
            nCreditDebit = 0;
            nTime = 0;
            strAccount.clear();
            strOtherAccount.clear();
            strComment.clear();
        }

        IMPLEMENT_SERIALIZE
        (
            if (!(nSerType & SER_GETHASH))
                READWRITE(nVersion);
            // Note: strAccount is serialized as part of the key, not here.
            READWRITE(nCreditDebit);
            READWRITE(nTime);
            READWRITE(strOtherAccount);
            READWRITE(strComment);
        )
    };

    bool GetWalletFile(CWallet* pwallet, std::string &strWalletFileOut);

}

#endif
