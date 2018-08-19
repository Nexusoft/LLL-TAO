/*******************************************************************************************

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

[Learn and Create] Viz. http://www.opensource.org/licenses/mit-license.php

*******************************************************************************************/

#include "wallet.h"
#include "walletdb.h"
#include "crypter.h"
#include "../util/ui_interface.h"

#include "../LLD/index.h"

using namespace std;

namespace Wallet
{

    std::vector<unsigned char> CWallet::GenerateNewKey()
    {
        bool fCompressed = true;

        RandAddSeedPerfmon();
        CKey key;
        key.MakeNewKey(fCompressed);

        if (!AddKey(key))
            throw std::runtime_error("CWallet::GenerateNewKey() : AddKey failed");
        return key.GetPubKey();
    }

    bool CWallet::AddKey(const CKey& key)
    {
        if (!CCryptoKeyStore::AddKey(key))
            return false;
        if (!fFileBacked)
            return true;
        if (!IsCrypted())
            return CWalletDB(strWalletFile).WriteKey(key.GetPubKey(), key.GetPrivKey());
        return true;
    }

    bool CWallet::AddCryptedKey(const vector<unsigned char> &vchPubKey, const vector<unsigned char> &vchCryptedSecret)
    {
        if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
            return false;
        if (!fFileBacked)
            return true;
        {
            LOCK(cs_wallet);
            if (pwalletdbEncryption)
                return pwalletdbEncryption->WriteCryptedKey(vchPubKey, vchCryptedSecret);
            else
                return CWalletDB(strWalletFile).WriteCryptedKey(vchPubKey, vchCryptedSecret);
        }
        return false;
    }

    bool CWallet::AddCScript(const CScript& redeemScript)
    {
        if (!CCryptoKeyStore::AddCScript(redeemScript))
            return false;
        if (!fFileBacked)
            return true;
        return CWalletDB(strWalletFile).WriteCScript(SK256(redeemScript), redeemScript);
    }

    // Nexus: optional setting to unlock wallet for block minting only;
    //         serves to disable the trivial sendmoney when OS account compromised
    bool fWalletUnlockMintOnly = false;

    bool CWallet::Unlock(const SecureString& strWalletPassphrase)
    {
        if (!IsLocked())
            return false;

        CCrypter crypter;
        CKeyingMaterial vMasterKey;

        {
            LOCK(cs_wallet);
            BOOST_FOREACH(const MasterKeyMap::value_type& pMasterKey, mapMasterKeys)
            {
                if(!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                    return false;
                if (CCryptoKeyStore::Unlock(vMasterKey))
                    return true;
            }
        }
        return false;
    }

    bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
    {
        bool fWasLocked = IsLocked();

        {
            LOCK(cs_wallet);
            Lock();

            CCrypter crypter;
            CKeyingMaterial vMasterKey;
            BOOST_FOREACH(MasterKeyMap::value_type& pMasterKey, mapMasterKeys)
            {
                if(!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                    return false;
                if (CCryptoKeyStore::Unlock(vMasterKey))
                {
                    int64_t nStartTime = GetTimeMillis();
                    crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                    pMasterKey.second.nDeriveIterations = pMasterKey.second.nDeriveIterations * (100 / ((double)(GetTimeMillis() - nStartTime)));

                    nStartTime = GetTimeMillis();
                    crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                    pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + pMasterKey.second.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

                    if (pMasterKey.second.nDeriveIterations < 25000)
                        pMasterKey.second.nDeriveIterations = 25000;

                    printf("Wallet passphrase changed to an nDeriveIterations of %i\n", pMasterKey.second.nDeriveIterations);

                    if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                        return false;
                    if (!crypter.Encrypt(vMasterKey, pMasterKey.second.vchCryptedKey))
                        return false;
                    CWalletDB(strWalletFile).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                    if (fWasLocked)
                        Lock();
                    return true;
                }
            }
        }

        return false;
    }

    void CWallet::SetBestChain(const Core::CBlockLocator& loc)
    {
        CWalletDB walletdb(strWalletFile);
        walletdb.WriteBestBlock(loc);
    }

    // This class implements an addrIncoming entry that causes pre-0.4
    // clients to crash on startup if reading a private-key-encrypted wallet.
    class CCorruptAddress
    {
    public:
        IMPLEMENT_SERIALIZE
        (
            if (nType & SER_DISK)
                READWRITE(nVersion);
        )
    };

    bool CWallet::SetMinVersion(enum WalletFeature nVersion, CWalletDB* pwalletdbIn, bool fExplicit)
    {
        if (nWalletVersion >= nVersion)
            return true;

        // when doing an explicit upgrade, if we pass the max version permitted, upgrade all the way
        if (fExplicit && nVersion > nWalletMaxVersion)
                nVersion = FEATURE_LATEST;

        nWalletVersion = nVersion;

        if (nVersion > nWalletMaxVersion)
            nWalletMaxVersion = nVersion;

        if (fFileBacked)
        {
            CWalletDB* pwalletdb = pwalletdbIn ? pwalletdbIn : new CWalletDB(strWalletFile);
            pwalletdb->WriteMinVersion(nWalletVersion);

            if (!pwalletdbIn)
                delete pwalletdb;
        }

        return true;
    }

    bool CWallet::SetMaxVersion(int nVersion)
    {
        // cannot downgrade below current version
        if (nWalletVersion > nVersion)
            return false;

        nWalletMaxVersion = nVersion;

        return true;
    }

    bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
    {
        if (IsCrypted())
            return false;

        CKeyingMaterial vMasterKey;
        RandAddSeedPerfmon();

        vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
        RAND_bytes(&vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

        CMasterKey kMasterKey;

        RandAddSeedPerfmon();
        kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
        RAND_bytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

        CCrypter crypter;
        int64_t nStartTime = GetTimeMillis();
        crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
        kMasterKey.nDeriveIterations = 2500000 / ((double)(GetTimeMillis() - nStartTime));

        nStartTime = GetTimeMillis();
        crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
        kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

        if (kMasterKey.nDeriveIterations < 25000)
            kMasterKey.nDeriveIterations = 25000;

        printf("Encrypting Wallet with an nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);

        if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
            return false;
        if (!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey))
            return false;

        {
            LOCK(cs_wallet);
            mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
            if (fFileBacked)
            {
                pwalletdbEncryption = new CWalletDB(strWalletFile);
                if (!pwalletdbEncryption->TxnBegin())
                    return false;
                pwalletdbEncryption->WriteMasterKey(nMasterKeyMaxID, kMasterKey);
            }

            if (!EncryptKeys(vMasterKey))
            {
                if (fFileBacked)
                    pwalletdbEncryption->TxnAbort();
                exit(1); //We now probably have half of our keys encrypted in memory, and half not...die and let the user reload their unencrypted wallet.
            }

            if (fFileBacked)
            {
                if (!pwalletdbEncryption->TxnCommit())
                    exit(1); //We now have keys encrypted in memory, but no on disk...die to avoid confusion and let the user reload their unencrypted wallet.

                delete pwalletdbEncryption;
                pwalletdbEncryption = NULL;
            }

            Lock();
            Unlock(strWalletPassphrase);
            NewKeyPool();
            Lock();

            // Need to completely rewrite the wallet file; if we don't, bdb might keep
            // bits of the unencrypted private key in slack space in the database file.
            CDB::Rewrite(strWalletFile);
        }

        return true;
    }

    void CWallet::WalletUpdateSpent(const Core::CTransaction &tx)
    {
        // Anytime a signature is successfully verified, it's proof the outpoint is spent.
        // Update the wallet spent flag if it doesn't know due to wallet.dat being
        // restored from backup or the user making copies of wallet.dat.
        {
            LOCK(cs_wallet);
            BOOST_FOREACH(const Core::CTxIn& txin, tx.vin)
            {
                map<uint512, CWalletTx>::iterator mi = mapWallet.find(txin.prevout.hash);
                if (mi != mapWallet.end())
                {
                    CWalletTx& wtx = (*mi).second;
                    if (!wtx.IsSpent(txin.prevout.n) && IsMine(wtx.vout[txin.prevout.n]))
                    {
                        printf("WalletUpdateSpent found spent coin %s Nexus %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                        wtx.MarkSpent(txin.prevout.n);
                        wtx.WriteToDisk();
                        vWalletUpdated.push_back(txin.prevout.hash);
                    }
                }
            }
        }
    }

    void CWallet::MarkDirty()
    {
        {
            LOCK(cs_wallet);
            BOOST_FOREACH(PAIRTYPE(const uint512, CWalletTx)& item, mapWallet)
                item.second.MarkDirty();
        }
    }

    bool CWallet::AddToWallet(const CWalletTx& wtxIn)
    {
        uint512 hash = wtxIn.GetHash();
        {
            LOCK(cs_wallet);

            // Inserts only if not already there, returns tx inserted or tx found
            pair<map<uint512, CWalletTx>::iterator, bool> ret = mapWallet.insert(make_pair(hash, wtxIn));
            CWalletTx& wtx = (*ret.first).second;
            wtx.BindWallet(this);
            bool fInsertedNew = ret.second;
            if (fInsertedNew)
                wtx.nTimeReceived = GetUnifiedTimestamp();

            bool fUpdated = false;
            if (!fInsertedNew)
            {
                // Merge
                if (wtxIn.hashBlock != 0 && wtxIn.hashBlock != wtx.hashBlock)
                {
                    wtx.hashBlock = wtxIn.hashBlock;
                    fUpdated = true;
                }
                if (wtxIn.nIndex != -1 && (wtxIn.vMerkleBranch != wtx.vMerkleBranch || wtxIn.nIndex != wtx.nIndex))
                {
                    wtx.vMerkleBranch = wtxIn.vMerkleBranch;
                    wtx.nIndex = wtxIn.nIndex;
                    fUpdated = true;
                }
                if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
                {
                    wtx.fFromMe = wtxIn.fFromMe;
                    fUpdated = true;
                }
                fUpdated |= wtx.UpdateSpent(wtxIn.vfSpent);
            }

            //// debug print
            printf("AddToWallet %s  %s%s\n", wtxIn.GetHash().ToString().substr(0,10).c_str(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

            // Write to disk
            if (fInsertedNew || fUpdated)
                if (!wtx.WriteToDisk())
                    return false;
    #ifndef QT_GUI
            // If default receiving address gets used, replace it with a new one
            CScript scriptDefaultKey;
            scriptDefaultKey.SetNexusAddress(vchDefaultKey);
            BOOST_FOREACH(const Core::CTxOut& txout, wtx.vout)
            {
                if (txout.scriptPubKey == scriptDefaultKey)
                {
                    std::vector<unsigned char> newDefaultKey;
                    if (GetKeyFromPool(newDefaultKey, false))
                    {
                        SetDefaultKey(newDefaultKey);
                        SetAddressBookName(NexusAddress(vchDefaultKey), "");
                    }
                }
            }
    #endif
            // Notify UI
            vWalletUpdated.push_back(hash);

            // since AddToWallet is called directly for self-originating transactions, check for consumption of own coins
            WalletUpdateSpent(wtx);
        }

        // Refresh UI
        MainFrameRepaint();
        return true;
    }

    // Add a transaction to the wallet, or update it.
    // pblock is optional, but should be provided if the transaction is known to be in a block.
    // If fUpdate is true, existing transactions will be updated.
    bool CWallet::AddToWalletIfInvolvingMe(const Core::CTransaction& tx, const Core::CBlock* pblock, bool fUpdate, bool fFindBlock)
    {
        uint512 hash = tx.GetHash();
        {
            LOCK(cs_wallet);
            bool fExisted = mapWallet.count(hash);
            if (fExisted && !fUpdate) return false;
            if (IsMine(tx) || IsFromMe(tx))
            {
                CWalletTx wtx(this,tx);
                // Get merkle branch if transaction was found in a block
                if (pblock)
                    wtx.SetMerkleBranch(pblock);

                return AddToWallet(wtx);
            }
            else
                WalletUpdateSpent(tx);
        }
        return false;
    }

    bool CWallet::EraseFromWallet(uint512 hash)
    {
        if (!fFileBacked)
            return false;
        {
            LOCK(cs_wallet);
            if (mapWallet.erase(hash))
                CWalletDB(strWalletFile).EraseTx(hash);
        }
        return true;
    }


    bool CWallet::IsMine(const Core::CTxIn &txin) const
    {
        {
            LOCK(cs_wallet);
            map<uint512, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
            if (mi != mapWallet.end())
            {
                const CWalletTx& prev = (*mi).second;
                if (txin.prevout.n < prev.vout.size())
                    if (IsMine(prev.vout[txin.prevout.n]))
                        return true;
            }
        }
        return false;
    }

    int64_t CWallet::GetDebit(const Core::CTxIn &txin) const
    {
        if(txin.prevout.IsNull())
            return 0;

        {
            LOCK(cs_wallet);
            map<uint512, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
            if (mi != mapWallet.end())
            {
                const CWalletTx& prev = (*mi).second;
                if (txin.prevout.n < prev.vout.size())
                    if (IsMine(prev.vout[txin.prevout.n]))
                        return prev.vout[txin.prevout.n].nValue;
            }
        }
        return 0;
    }

    bool CWallet::IsChange(const Core::CTxOut& txout) const
    {
        NexusAddress address;

        // TODO: fix handling of 'change' outputs. The assumption is that any
        // payment to a TX_PUBKEYHASH that is mine but isn't in the address book
        // is change. That assumption is likely to break when we implement multisignature
        // wallets that return change back into a multi-signature-protected address;
        // a better way of identifying which outputs are 'the send' and which are
        // 'the change' will need to be implemented (maybe extend CWalletTx to remember
        // which output, if any, was change).
        if (ExtractAddress(txout.scriptPubKey, address) && HaveKey(address))
        {
            LOCK(cs_wallet);
            if (!mapAddressBook.count(address))
                return true;
        }
        return false;
    }

    int64_t CWalletTx::GetTxTime() const
    {
        return nTimeReceived;
    }

    int CWalletTx::GetRequestCount() const
    {
        // Returns -1 if it wasn't being tracked
        int nRequests = -1;
        {
            LOCK(pwallet->cs_wallet);
            if (IsCoinBase() || IsCoinStake())
            {
                // Generated block
                if (hashBlock != 0)
                {
                    map<uint1024, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                    if (mi != pwallet->mapRequestCount.end())
                        nRequests = (*mi).second;
                }
            }
            else
            {
                // Did anyone request this transaction?
                map<uint1024, int>::const_iterator mi = pwallet->mapRequestCount.find(GetHash());
                if (mi != pwallet->mapRequestCount.end())
                {
                    nRequests = (*mi).second;

                    // How about the block it's in?
                    if (nRequests == 0 && hashBlock != 0)
                    {
                        map<uint1024, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                        if (mi != pwallet->mapRequestCount.end())
                            nRequests = (*mi).second;
                        else
                            nRequests = 1; // If it's in someone else's block it must have got out
                    }
                }
            }
        }
        return nRequests;
    }

    void CWalletTx::GetAmounts(int64_t& nGeneratedImmature, int64_t& nGeneratedMature, list<pair<NexusAddress, int64_t> >& listReceived,
                            list<pair<NexusAddress, int64_t> >& listSent, int64_t& nFee, string& strSentAccount) const
    {
        nGeneratedImmature = nGeneratedMature = nFee = 0;
        listReceived.clear();
        listSent.clear();
        strSentAccount = strFromAccount;

        if (IsCoinBase() || IsCoinStake())
        {
            if (GetBlocksToMaturity() > 0)
                nGeneratedImmature = pwallet->GetCredit(*this);
            else
                nGeneratedMature = GetCredit();
            return;
        }

        // Compute fee:
        int64_t nDebit = GetDebit();
        if (nDebit > 0) // debit>0 means we signed/sent this transaction
        {
            int64_t nValueOut = GetValueOut();
            nFee = nDebit - nValueOut;
        }

        // Sent/received.
        BOOST_FOREACH(const Core::CTxOut& txout, vout)
        {
            NexusAddress address;
            vector<unsigned char> vchPubKey;
            if (!ExtractAddress(txout.scriptPubKey, address))
            {
                printf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
                    this->GetHash().ToString().c_str());
                address = " unknown ";
            }

            // Don't report 'change' txouts
            if (nDebit > 0 && pwallet->IsChange(txout))
                continue;

            if (nDebit > 0)
                listSent.push_back(make_pair(address, txout.nValue));

            if (pwallet->IsMine(txout))
                listReceived.push_back(make_pair(address, txout.nValue));
        }

    }

    void CWalletTx::GetAccountAmounts(const string& strAccount, int64_t& nGenerated, int64_t& nReceived,
                                    int64_t& nSent, int64_t& nFee) const
    {
        nGenerated = nReceived = nSent = nFee = 0;

        int64_t allGeneratedImmature, allGeneratedMature, allFee;
        allGeneratedImmature = allGeneratedMature = allFee = 0;
        string strSentAccount;
        list<pair<NexusAddress, int64_t> > listReceived;
        list<pair<NexusAddress, int64_t> > listSent;
        GetAmounts(allGeneratedImmature, allGeneratedMature, listReceived, listSent, allFee, strSentAccount);

        if (strAccount == "")
            nGenerated = allGeneratedMature;
        if (strAccount == strSentAccount)
        {
            BOOST_FOREACH(const PAIRTYPE(NexusAddress,int64_t)& s, listSent)
                nSent += s.second;
            nFee = allFee;
        }
        {
            LOCK(pwallet->cs_wallet);
            BOOST_FOREACH(const PAIRTYPE(NexusAddress,int64_t)& r, listReceived)
            {
                if (pwallet->mapAddressBook.count(r.first))
                {
                    map<NexusAddress, string>::const_iterator mi = pwallet->mapAddressBook.find(r.first);
                    if (mi != pwallet->mapAddressBook.end() && (*mi).second == strAccount)
                        nReceived += r.second;
                }
                else if (strAccount.empty())
                {
                    nReceived += r.second;
                }
            }
        }
    }

    void CWalletTx::AddSupportingTransactions(LLD::CIndexDB& indexdb)
    {
        vtxPrev.clear();

        const int COPY_DEPTH = 3;
        if (SetMerkleBranch() < COPY_DEPTH)
        {
            vector<uint512> vWorkQueue;
            BOOST_FOREACH(const Core::CTxIn& txin, vin)
                vWorkQueue.push_back(txin.prevout.hash);

            // This critsect is OK because indexdb is already open
            {
                LOCK(pwallet->cs_wallet);
                map<uint512, const CMerkleTx*> mapWalletPrev;
                set<uint512> setAlreadyDone;
                for (unsigned int i = 0; i < vWorkQueue.size(); i++)
                {
                    uint512 hash = vWorkQueue[i];
                    if (setAlreadyDone.count(hash))
                        continue;
                    setAlreadyDone.insert(hash);

                    Core::CMerkleTx tx;
                    map<uint512, CWalletTx>::const_iterator mi = pwallet->mapWallet.find(hash);
                    if (mi != pwallet->mapWallet.end())
                    {
                        tx = (*mi).second;
                        BOOST_FOREACH(const Core::CMerkleTx& txWalletPrev, (*mi).second.vtxPrev)
                            mapWalletPrev[txWalletPrev.GetHash()] = &txWalletPrev;
                    }
                    else if (mapWalletPrev.count(hash))
                    {
                        tx = *mapWalletPrev[hash];
                    }
                    else if (!Net::fClient && indexdb.ReadDiskTx(hash, tx))
                    {

                    }
                    else
                    {
                        printf("ERROR: AddSupportingTransactions() : unsupported transaction\n");
                        continue;
                    }

                    int nDepth = tx.SetMerkleBranch();
                    vtxPrev.push_back(tx);

                    if (nDepth < COPY_DEPTH)
                    {
                        BOOST_FOREACH(const Core::CTxIn& txin, tx.vin)
                            vWorkQueue.push_back(txin.prevout.hash);
                    }
                }
            }
        }

        reverse(vtxPrev.begin(), vtxPrev.end());
    }

    bool CWalletTx::WriteToDisk()
    {
        return CWalletDB(pwallet->strWalletFile).WriteTx(GetHash(), *this);
    }

    // Scan the block chain (starting in pindexStart) for transactions
    // from or to us. If fUpdate is true, found transactions that already
    // exist in the wallet will be updated.
    int CWallet::ScanForWalletTransactions(Core::CBlockIndex* pindexStart, bool fUpdate)
    {
        int ret = 0;

        Core::CBlockIndex* pindex = pindexStart;
        {
            LOCK(cs_wallet);
            while (pindex)
            {
                Core::CBlock block;
                block.ReadFromDisk(pindex, true);
                BOOST_FOREACH(Core::CTransaction& tx, block.vtx)
                {
                    if (AddToWalletIfInvolvingMe(tx, &block, fUpdate))
                        ret++;
                }
                pindex = pindex->pnext;
            }
        }
        return ret;
    }

    int CWallet::ScanForWalletTransaction(const uint512& hashTx)
    {
        Core::CTransaction tx;
        tx.ReadFromDisk(Core::COutPoint(hashTx, 0));
        if (AddToWalletIfInvolvingMe(tx, NULL, true, true))
            return 1;
        return 0;
    }

    void CWallet::ReacceptWalletTransactions()
    {
        LLD::CIndexDB indexdb("r");
        bool fRepeat = true;
        while (fRepeat)
        {
            LOCK(cs_wallet);
            fRepeat = false;
            vector<Core::CDiskTxPos> vMissingTx;
            BOOST_FOREACH(PAIRTYPE(const uint512, CWalletTx)& item, mapWallet)
            {
                CWalletTx& wtx = item.second;
                if ((wtx.IsCoinBase() && wtx.IsSpent(0)) || (wtx.IsCoinStake() && wtx.IsSpent(0)))
                    continue;

                Core::CTxIndex txindex;
                bool fUpdated = false;
                if (indexdb.ReadTxIndex(wtx.GetHash(), txindex))
                {
                    // Update fSpent if a tx got spent somewhere else by a copy of wallet.dat
                    if (txindex.vSpent.size() != wtx.vout.size())
                    {
                        printf("ERROR: ReacceptWalletTransactions() : txindex.vSpent.size() %d != wtx.vout.size() %d\n", txindex.vSpent.size(), wtx.vout.size());
                        continue;
                    }
                    for (unsigned int i = 0; i < txindex.vSpent.size(); i++)
                    {
                        if (wtx.IsSpent(i))
                            continue;
                        if (!txindex.vSpent[i].IsNull() && IsMine(wtx.vout[i]))
                        {
                            wtx.MarkSpent(i);
                            fUpdated = true;
                            vMissingTx.push_back(txindex.vSpent[i]);
                        }
                    }
                    if (fUpdated)
                    {
                        printf("ReacceptWalletTransactions found spent coin %s Nexus %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                        wtx.MarkDirty();
                        wtx.WriteToDisk();
                    }
                }
                else
                {
                    // Reaccept any txes of ours that aren't already in a block
                    if (!(wtx.IsCoinBase() || wtx.IsCoinStake()))
                        wtx.AcceptWalletTransaction(indexdb, false);
                }
            }
            if (!vMissingTx.empty())
            {
                // TODO: optimize this to scan just part of the block chain?
                if (ScanForWalletTransactions(Core::pindexGenesisBlock))
                    fRepeat = true;  // Found missing transactions: re-do Reaccept.
            }
        }
    }

    void CWalletTx::RelayWalletTransaction(LLD::CIndexDB& indexdb)
    {
        BOOST_FOREACH(const Core::CMerkleTx& tx, vtxPrev)
        {
            if (!(tx.IsCoinBase() || tx.IsCoinStake()))
            {
                uint512 hash = tx.GetHash();
                if (!indexdb.ContainsTx(hash))
                    RelayMessage(Net::CInv(Net::MSG_TX, hash), (Core::CTransaction)tx);
            }
        }
        if (!(IsCoinBase() || IsCoinStake()))
        {
            uint512 hash = GetHash();
            if (!indexdb.ContainsTx(hash))
            {
                printf("Relaying wtx %s\n", hash.ToString().substr(0,10).c_str());
                RelayMessage(Net::CInv(Net::MSG_TX, hash), (Core::CTransaction)*this);
            }
        }
    }

    void CWalletTx::RelayWalletTransaction()
    {
    LLD::CIndexDB indexdb("r");
    RelayWalletTransaction(indexdb);
    }

    void CWallet::ResendWalletTransactions()
    {
        // Do this infrequently and randomly to avoid giving away
        // that these are our transactions.
        static int64_t nNextTime;
        if (GetUnifiedTimestamp() < nNextTime)
            return;
        bool fFirst = (nNextTime == 0);
        nNextTime = GetUnifiedTimestamp() + GetRand(30 * 60);
        if (fFirst)
            return;

        // Only do it if there's been a new block since last time
        static int64_t nLastTime;
        if (Core::nTimeBestReceived < nLastTime)
            return;
        nLastTime = GetUnifiedTimestamp();

        // Rebroadcast any of our txes that aren't in a block yet
        printf("ResendWalletTransactions()\n");
        LLD::CIndexDB indexdb("r");
        {
            LOCK(cs_wallet);
            // Sort them in chronological order
            multimap<unsigned int, CWalletTx*> mapSorted;
            BOOST_FOREACH(PAIRTYPE(const uint512, CWalletTx)& item, mapWallet)
            {
                CWalletTx& wtx = item.second;
                // Don't rebroadcast until it's had plenty of time that
                // it should have gotten in already by now.
                if (Core::nTimeBestReceived - (int64_t)wtx.nTimeReceived > 5 * 60)
                    mapSorted.insert(make_pair(wtx.nTimeReceived, &wtx));
            }
            BOOST_FOREACH(PAIRTYPE(const unsigned int, CWalletTx*)& item, mapSorted)
            {
                CWalletTx& wtx = *item.second;
                if (wtx.CheckTransaction())
                    wtx.RelayWalletTransaction(indexdb);
                else
                    printf("ResendWalletTransactions() : CheckTransaction failed for transaction %s\n", wtx.GetHash().ToString().c_str());
            }
        }
    }






    //////////////////////////////////////////////////////////////////////////////
    //
    // Actions
    //


    int64_t CWallet::GetBalance() const
    {
        int64_t nTotal = 0;
        {
            LOCK(cs_wallet);
            for (map<uint512, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            {
                const CWalletTx* pcoin = &(*it).second;
                if (!pcoin->IsFinal() || !pcoin->IsConfirmed() || pcoin->nTime > GetUnifiedTimestamp())
                    continue;

                nTotal += pcoin->GetAvailableCredit();
            }
        }

        return nTotal;
    }


    int64_t CWallet::GetUnconfirmedBalance() const
    {
        int64_t nTotal = 0;
        {
            LOCK(cs_wallet);
            for (map<uint512, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            {
                const CWalletTx* pcoin = &(*it).second;
                if (pcoin->IsFinal() && pcoin->IsConfirmed())
                    continue;
                nTotal += pcoin->GetAvailableCredit();
            }
        }
        return nTotal;
    }

    // Nexus: total coins staked (non-spendable until maturity)
    int64_t CWallet::GetStake() const
    {
        int64_t nTotal = 0;
        LOCK(cs_wallet);
        for (map<uint512, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsCoinStake() && pcoin->GetBlocksToMaturity() > 0 && pcoin->GetDepthInMainChain() > 1)
                nTotal += CWallet::GetCredit(*pcoin);
        }
        return nTotal;
    }

    int64_t CWallet::GetNewMint() const
    {
        int64_t nTotal = 0;
        LOCK(cs_wallet);
        for (map<uint512, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0 && pcoin->GetDepthInMainChain() > 1)
                nTotal += CWallet::GetCredit(*pcoin);
        }
        return nTotal;
    }

    // populate vCoins with vector of spendable (age, (value, (transaction, output_number))) outputs
    void CWallet::AvailableCoins(unsigned int nSpendTime, vector<COutput>& vCoins, bool fOnlyConfirmed) const
    {
        vCoins.clear();

        {
            LOCK(cs_wallet);
            for (map<uint512, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            {
                const CWalletTx* pcoin = &(*it).second;

                if (!pcoin->IsFinal())
                    continue;

                if (fOnlyConfirmed && !pcoin->IsConfirmed())
                    continue;

                if ((pcoin->IsCoinBase() || pcoin->IsCoinStake()) && pcoin->GetBlocksToMaturity() > 0)
                    continue;

                for (int i = 0; i < pcoin->vout.size(); i++)
                {
                    if (pcoin->nTime > nSpendTime)
                        continue;  // ppcoin: timestamp must not exceed spend time

                    if (!(pcoin->IsSpent(i)) && IsMine(pcoin->vout[i]) && pcoin->vout[i].nValue > 0)
                        vCoins.push_back(COutput(pcoin, i, pcoin->GetDepthInMainChain()));
                }
            }
        }
    }

    /** Get the available addresses that have a balance associated with a wallet. **/
    bool CWallet::AvailableAddresses(unsigned int nSpendTime, map<NexusAddress, int64_t>& mapAddresses, bool fOnlyConfirmed) const
    {
        mapAddresses.clear();
        {
            LOCK(cs_wallet);
            for (map<uint512, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            {
                const CWalletTx* pcoin = &(*it).second;

                if (!pcoin->IsFinal())
                    continue;

                if (fOnlyConfirmed && !pcoin->IsConfirmed())
                    continue;

                if (fOnlyConfirmed && pcoin->GetBlocksToMaturity() > 0)
                    continue;

                for (int i = 0; i < pcoin->vout.size(); i++)
                {
                    if (pcoin->nTime > nSpendTime)
                        continue;  // ppcoin: timestamp must not exceed spend time

                    if (!(pcoin->IsSpent(i)) && IsMine(pcoin->vout[i]) && pcoin->vout[i].nValue > 0) {
                        NexusAddress cAddress;
                        if(!ExtractAddress(pcoin->vout[i].scriptPubKey, cAddress) || !cAddress.IsValid())
                            return false;

                        if(mapAddresses.count(cAddress))
                            mapAddresses[cAddress] = pcoin->vout[i].nValue;
                        else
                            mapAddresses[cAddress] += pcoin->vout[i].nValue;

                    }
                }
            }
        }

        return true;
    }


    bool CWallet::SelectCoinsMinConf(int64_t nTargetValue, unsigned int nSpendTime, int nConfMine, int nConfTheirs, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const
    {
        /* Add Each Input to Transaction. */
        setCoinsRet.clear();
        vector<const CWalletTx*> vCoins;

        nValueRet = 0;

        {
        LOCK(cs_wallet);

        vCoins.reserve(mapWallet.size());
        for (map<uint512, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            vCoins.push_back(&(*it).second);
        }

        random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);
        BOOST_FOREACH(const CWalletTx* pcoin, vCoins)
        {
            if (!pcoin->IsFinal() || !pcoin->IsConfirmed())
                continue;

            if ((pcoin->IsCoinBase() || pcoin->IsCoinStake()) && pcoin->GetBlocksToMaturity() > 0)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                if (pcoin->IsSpent(i) || !IsMine(pcoin->vout[i]))
                    continue;

                int nDepth = pcoin->GetDepthInMainChain();
                if (nDepth < (pcoin->IsFromMe() ? nConfMine : nConfTheirs))
                    continue;

                if (pcoin->nTime > nSpendTime)
                    continue;

                if(nValueRet >= nTargetValue)
                    break;

                setCoinsRet.insert(make_pair(pcoin, i));
                nValueRet += pcoin->vout[i].nValue;
            }
        }

        //// debug print
        if (GetBoolArg("-printselectcoin"))
        {
            printf("SelectCoins() selected: ");
            BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoinsRet)
                pcoin.first->print();

            printf("total %s\n", FormatMoney(nValueRet).c_str());
        }

        //Ensure total inputs does not exceed maximum
        if(!Core::MoneyRange(nValueRet))
            return error("CWallet::SelectCoins() : Input total over TX limit Total: %" PRI64d " Limit %" PRI64d, nValueRet, Core::MAX_TXOUT_AMOUNT);

        //Ensure balance is sufficient to cover transaction
        if(nValueRet < nTargetValue)
            return error("CWallet::SelectCoins() : Insufficient Balance Target: %" PRI64d " Actual %" PRI64d, nTargetValue, nValueRet);

        return true;
    }


    bool CWallet::SelectCoins(int64_t nTargetValue, unsigned int nSpendTime, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const
    {
        return (SelectCoinsMinConf(nTargetValue, nSpendTime, 3, 3, setCoinsRet, nValueRet) ||
                SelectCoinsMinConf(nTargetValue, nSpendTime, 3, 3, setCoinsRet, nValueRet) ||
                SelectCoinsMinConf(nTargetValue, nSpendTime, 3, 3, setCoinsRet, nValueRet));
    }




    bool CWallet::CreateTransaction(const vector<pair<CScript, int64_t> >& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet)
    {
        int64_t nValue = 0;
        BOOST_FOREACH (const PAIRTYPE(CScript, int64_t)& s, vecSend)
        {
            if (nValue < 0)
                return false;
            nValue += s.second;
        }
        if (vecSend.empty() || nValue < 0)
            return false;

        wtxNew.BindWallet(this);

        {
            LOCK2(Core::cs_main, cs_wallet);

            // indexdb must be opened before the mapWallet lock
            LLD::CIndexDB indexdb("r");
            {
                nFeeRet = Core::nTransactionFee;
                loop() {
                    wtxNew.vin.clear();
                    wtxNew.vout.clear();
                    wtxNew.fFromMe = true;

                    int64_t nTotalValue = nValue + nFeeRet;
                    double dPriority = 0;
                    // vouts to the payees
                    BOOST_FOREACH (const PAIRTYPE(CScript, int64_t)& s, vecSend)
                        wtxNew.vout.push_back(Core::CTxOut(s.second, s.first));

                    // Choose coins to use
                    set<pair<const CWalletTx*,unsigned int> > setCoins;
                    int64_t nValueIn = 0;
                    if (!SelectCoins(nTotalValue, wtxNew.nTime, setCoins, nValueIn))
                        return false;

                    CScript scriptChange;
                    BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
                    {
                        int64_t nCredit = pcoin.first->vout[pcoin.second].nValue;
                        dPriority += (double)nCredit * pcoin.first->GetDepthInMainChain();
                        scriptChange = pcoin.first->vout[pcoin.second].scriptPubKey;
                    }

                    int64_t nChange = nValueIn - nValue - nFeeRet;

                    // if sub-cent change is required, the fee must be raised to at least MIN_TX_FEE
                    // or until nChange becomes zero
                    // NOTE: this depends on the exact behaviour of GetMinFee
                    if (nFeeRet < Core::MIN_TX_FEE && nChange > 0 && nChange < CENT)
                    {
                        int64_t nMoveToFee = min(nChange, Core::MIN_TX_FEE - nFeeRet);
                        nChange -= nMoveToFee;
                        nFeeRet += nMoveToFee;
                    }

                    // Nexus: sub-cent change is moved to fee
                    if (nChange > 0 && nChange < Core::MIN_TXOUT_AMOUNT)
                    {
                        nFeeRet += nChange;
                        nChange = 0;
                    }

                    if (nChange > 0)
                    {
                        // Note: We use a new key here to keep it from being obvious which side is the change.
                        //  The drawback is that by not reusing a previous key, the change may be lost if a
                        //  backup is restored, if the backup doesn't have the new private key for the change.
                        //  If we reused the old key, it would be possible to add code to look for and
                        //  rediscover unknown transactions that were written with keys of ours to recover
                        //  post-backup change.

                        if (!GetBoolArg("-avatar")) // Nexus: not avatar mode
                        {
                            // Reserve a new key pair from key pool
                            vector<unsigned char> vchPubKey = reservekey.GetReservedKey();
                            // assert(mapKeys.count(vchPubKey));

                            // Fill a vout to ourself
                            // TODO: pass in scriptChange instead of reservekey so
                            // change transaction isn't always pay-to-nexus-address
                            scriptChange.SetNexusAddress(vchPubKey);
                        }

                        // Insert change txn at random position:
                        vector<Core::CTxOut>::iterator position = wtxNew.vout.begin()+GetRandInt(wtxNew.vout.size());
                        wtxNew.vout.insert(position, Core::CTxOut(nChange, scriptChange));
                    }
                    else
                        reservekey.ReturnKey();

                    // Fill vin
                    BOOST_FOREACH(const PAIRTYPE(const CWalletTx*,unsigned int)& coin, setCoins)
                        wtxNew.vin.push_back(Core::CTxIn(coin.first->GetHash(),coin.second));

                    // Sign
                    int nIn = 0;
                    BOOST_FOREACH(const PAIRTYPE(const CWalletTx*,unsigned int)& coin, setCoins)
                        if (!SignSignature(*this, *coin.first, wtxNew, nIn++))
                            return false;

                    // Limit size
                    unsigned int nBytes = ::GetSerializeSize(*(Core::CTransaction*)&wtxNew, SER_NETWORK, PROTOCOL_VERSION);
                    if (nBytes >= Core::MAX_BLOCK_SIZE_GEN/5)
                        return false;
                    dPriority /= nBytes;

                    // Check that enough fee is included
                    int64_t nPayFee = Core::nTransactionFee * (1 + (int64_t)nBytes / 1000);
                    int64_t nMinFee = wtxNew.GetMinFee(1, false, Core::GMF_SEND);
                    if (nFeeRet < max(nPayFee, nMinFee))
                    {
                        nFeeRet = max(nPayFee, nMinFee);
                        continue;
                    }

                    // Fill vtxPrev by copying from previous transactions vtxPrev
                    wtxNew.AddSupportingTransactions(indexdb);
                    wtxNew.fTimeReceivedIsTxTime = true;

                    break;
                }
            }
        }
        return true;
    }

    bool CWallet::CreateTransaction(CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet)
    {
        vector< pair<CScript, int64_t> > vecSend;
        vecSend.push_back(make_pair(scriptPubKey, nValue));
        return CreateTransaction(vecSend, wtxNew, reservekey, nFeeRet);
    }

    // Call after CreateTransaction unless you want to abort
    bool CWallet::CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey)
    {
        {
            LOCK2(Core::cs_main, cs_wallet);
            printf("CommitTransaction:\n%s", wtxNew.ToString().c_str());
            {
                // This is only to keep the database open to defeat the auto-flush for the
                // duration of this scope.  This is the only place where this optimization
                // maybe makes sense; please don't do it anywhere else.
                CWalletDB* pwalletdb = fFileBacked ? new CWalletDB(strWalletFile,"r") : NULL;

                // Take key pair from key pool so it won't be used again
                reservekey.KeepKey();

                // Add tx to wallet, because if it has change it's also ours,
                // otherwise just for transaction history.
                AddToWallet(wtxNew);

                // Mark old coins as spent
                set<CWalletTx*> setCoins;
                BOOST_FOREACH(const Core::CTxIn& txin, wtxNew.vin)
                {
                    CWalletTx &coin = mapWallet[txin.prevout.hash];
                    coin.BindWallet(this);
                    coin.MarkSpent(txin.prevout.n);
                    coin.WriteToDisk();
                    vWalletUpdated.push_back(coin.GetHash());
                }

                if (fFileBacked)
                    delete pwalletdb;
            }

            // Track how many getdata requests our transaction gets
            mapRequestCount[wtxNew.GetHash()] = 0;

            // Broadcast
            if (!wtxNew.AcceptToMemoryPool())
            {
                // This must not fail. The transaction has already been signed and recorded.
                printf("CommitTransaction() : Error: Transaction not valid");
                return false;
            }
            wtxNew.RelayWalletTransaction();
        }
        MainFrameRepaint();
        return true;
    }




    string CWallet::SendMoney(CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, bool fAskFee)
    {
        CReserveKey reservekey(this);
        int64_t nFeeRequired;

        if (IsLocked())
        {
            string strError = _("Error: Wallet locked, unable to create transaction  ");
            printf("SendMoney() : %s", strError.c_str());
            return strError;
        }
        if (fWalletUnlockMintOnly)
        {
            string strError = _("Error: Wallet unlocked for block minting only, unable to create transaction.");
            printf("SendMoney() : %s", strError.c_str());
            return strError;
        }
        if (!CreateTransaction(scriptPubKey, nValue, wtxNew, reservekey, nFeeRequired))
        {
            string strError;
            if (nValue + nFeeRequired > GetBalance())
                strError = strprintf(_("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds  "), FormatMoney(nFeeRequired).c_str());
            else
                strError = _("Error: Transaction creation failed  ");
            printf("SendMoney() : %s", strError.c_str());
            return strError;
        }

        if (fAskFee && !ThreadSafeAskFee(nFeeRequired, _("Sending...")))
            return "ABORTED";

        if (!CommitTransaction(wtxNew, reservekey))
            return _("Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");

        MainFrameRepaint();
        return "";
    }



    string CWallet::SendToNexusAddress(const NexusAddress& address, int64_t nValue, CWalletTx& wtxNew, bool fAskFee)
    {
        // Check amount
        if (nValue <= 0)
            return _("Invalid amount");
        if (nValue + Core::nTransactionFee > GetBalance())
            return _("Insufficient funds");

        // Parse nexus address
        CScript scriptPubKey;
        scriptPubKey.SetNexusAddress(address);

        return SendMoney(scriptPubKey, nValue, wtxNew, fAskFee);
    }




    int CWallet::LoadWallet(bool& fFirstRunRet)
    {
        if (!fFileBacked)
            return false;
        fFirstRunRet = false;
        int nLoadWalletRet = CWalletDB(strWalletFile,"cr+").LoadWallet(this);
        if (nLoadWalletRet == DB_NEED_REWRITE)
        {
            if (CDB::Rewrite(strWalletFile, "\x04pool"))
            {
                setKeyPool.clear();
                // Note: can't top-up keypool here, because wallet is locked.
                // User will be prompted to unlock wallet the next operation
                // the requires a new key.
            }
            nLoadWalletRet = DB_NEED_REWRITE;
        }

        if (nLoadWalletRet != DB_LOAD_OK)
            return nLoadWalletRet;
        fFirstRunRet = vchDefaultKey.empty();

        CreateThread(ThreadFlushWalletDB, &strWalletFile);
        return DB_LOAD_OK;
    }


    bool CWallet::SetAddressBookName(const NexusAddress& address, const string& strName)
    {
        mapAddressBook[address] = strName;
        AddressBookRepaint();
        if (!fFileBacked)
            return false;
        return CWalletDB(strWalletFile).WriteName(address.ToString(), strName);
    }

    bool CWallet::DelAddressBookName(const NexusAddress& address)
    {
        mapAddressBook.erase(address);
        AddressBookRepaint();
        if (!fFileBacked)
            return false;
        return CWalletDB(strWalletFile).EraseName(address.ToString());
    }


    void CWallet::PrintWallet(const Core::CBlock& block)
    {
        {
            LOCK(cs_wallet);
            if (block.IsProofOfWork() && mapWallet.count(block.vtx[0].GetHash()))
            {
                CWalletTx& wtx = mapWallet[block.vtx[0].GetHash()];
                printf("    mine:  %d  %d  %s", wtx.GetDepthInMainChain(), wtx.GetBlocksToMaturity(), FormatMoney(wtx.GetCredit()).c_str());
            }
            if (block.IsProofOfStake() && mapWallet.count(block.vtx[1].GetHash()))
            {
                CWalletTx& wtx = mapWallet[block.vtx[1].GetHash()];
                printf("    stake: %d  %d  %s", wtx.GetDepthInMainChain(), wtx.GetBlocksToMaturity(), FormatMoney(wtx.GetCredit()).c_str());
            }
        }
        printf("\n");
    }

    bool CWallet::GetTransaction(const uint512 &hashTx, CWalletTx& wtx)
    {
        {
            LOCK(cs_wallet);
            map<uint512, CWalletTx>::iterator mi = mapWallet.find(hashTx);
            if (mi != mapWallet.end())
            {
                wtx = (*mi).second;
                return true;
            }
        }
        return false;
    }

    bool CWallet::SetDefaultKey(const std::vector<unsigned char> &vchPubKey)
    {
        if (fFileBacked)
        {
            if (!CWalletDB(strWalletFile).WriteDefaultKey(vchPubKey))
                return false;
        }
        vchDefaultKey = vchPubKey;
        return true;
    }

    bool GetWalletFile(CWallet* pwallet, string &strWalletFileOut)
    {
        if (!pwallet->fFileBacked)
            return false;
        strWalletFileOut = pwallet->strWalletFile;
        return true;
    }

    //
    // Mark old keypool keys as used,
    // and generate all new keys
    //
    bool CWallet::NewKeyPool()
    {
        {
            LOCK(cs_wallet);
            CWalletDB walletdb(strWalletFile);
            BOOST_FOREACH(int64_t nIndex, setKeyPool)
                walletdb.ErasePool(nIndex);
            setKeyPool.clear();

            if (IsLocked())
                return false;

            int64_t nKeys = max(GetArg("-keypool", 100), (int64_t)0);
            for (int i = 0; i < nKeys; i++)
            {
                int64_t nIndex = i+1;
                walletdb.WritePool(nIndex, CKeyPool(GenerateNewKey()));
                setKeyPool.insert(nIndex);
            }
            printf("CWallet::NewKeyPool wrote %" PRI64d " new keys\n", nKeys);
        }
        return true;
    }

    bool CWallet::TopUpKeyPool()
    {
        {
            LOCK(cs_wallet);

            if (IsLocked())
                return false;

            CWalletDB walletdb(strWalletFile);

            // Top up key pool
            unsigned int nTargetSize = max(GetArg("-keypool", 100), 0LL);
            while (setKeyPool.size() < (nTargetSize + 1))
            {
                int64_t nEnd = 1;
                if (!setKeyPool.empty())
                    nEnd = *(--setKeyPool.end()) + 1;
                if (!walletdb.WritePool(nEnd, CKeyPool(GenerateNewKey())))
                    throw runtime_error("TopUpKeyPool() : writing generated key failed");
                setKeyPool.insert(nEnd);
                printf("keypool added key %" PRI64d ", size=%d\n", nEnd, setKeyPool.size());
            }
        }
        return true;
    }

    void CWallet::ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool)
    {
        nIndex = -1;
        keypool.vchPubKey.clear();
        {
            LOCK(cs_wallet);

            if (!IsLocked())
                TopUpKeyPool();

            // Get the oldest key
            if(setKeyPool.empty())
                return;

            CWalletDB walletdb(strWalletFile);

            nIndex = *(setKeyPool.begin());
            setKeyPool.erase(setKeyPool.begin());
            if (!walletdb.ReadPool(nIndex, keypool))
                throw runtime_error("ReserveKeyFromKeyPool() : read failed");

            if (!HaveKey(SK256(keypool.vchPubKey)))
                throw runtime_error("ReserveKeyFromKeyPool() : unknown key in key pool");

            assert(!keypool.vchPubKey.empty());
            if (fDebug && GetBoolArg("-printkeypool"))
                printf("keypool reserve %" PRI64d "\n", nIndex);
        }
    }

    int64_t CWallet::AddReserveKey(const CKeyPool& keypool)
    {
        {
            LOCK2(Core::cs_main, cs_wallet);
            CWalletDB walletdb(strWalletFile);

            int64_t nIndex = 1 + *(--setKeyPool.end());
            if (!walletdb.WritePool(nIndex, keypool))
                throw runtime_error("AddReserveKey() : writing added key failed");
            setKeyPool.insert(nIndex);
            return nIndex;
        }
        return -1;
    }

    void CWallet::KeepKey(int64_t nIndex)
    {
        // Remove from key pool
        if (fFileBacked)
        {
            CWalletDB walletdb(strWalletFile);
            walletdb.ErasePool(nIndex);
        }
        printf("keypool keep %" PRI64d "\n", nIndex);
    }

    void CWallet::ReturnKey(int64_t nIndex)
    {
        // Return to key pool
        {
            LOCK(cs_wallet);
            setKeyPool.insert(nIndex);
        }
        if (fDebug && GetBoolArg("-printkeypool"))
            printf("keypool return %" PRI64d "\n", nIndex);
    }

    bool CWallet::GetKeyFromPool(vector<unsigned char>& result, bool fAllowReuse)
    {
        int64_t nIndex = 0;
        CKeyPool keypool;
        {
            LOCK(cs_wallet);
            ReserveKeyFromKeyPool(nIndex, keypool);
            if (nIndex == -1)
            {
                if (fAllowReuse && !vchDefaultKey.empty())
                {
                    result = vchDefaultKey;
                    return true;
                }
                if (IsLocked()) return false;
                result = GenerateNewKey();
                return true;
            }
            KeepKey(nIndex);
            result = keypool.vchPubKey;
        }
        return true;
    }

    int64_t CWallet::GetOldestKeyPoolTime()
    {
        int64_t nIndex = 0;
        CKeyPool keypool;
        ReserveKeyFromKeyPool(nIndex, keypool);
        if (nIndex == -1)
            return GetUnifiedTimestamp();
        ReturnKey(nIndex);
        return keypool.nTime;
    }

    // Nexus: check 'spent' consistency between wallet and txindex
    // Nexus: fix wallet spent state according to txindex
    void CWallet::FixSpentCoins(int& nMismatchFound, int64_t& nBalanceInQuestion, bool fCheckOnly)
    {
        nMismatchFound = 0;
        nBalanceInQuestion = 0;

        LOCK(cs_wallet);
        vector<CWalletTx*> vCoins;
        vCoins.reserve(mapWallet.size());
        for (map<uint512, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            vCoins.push_back(&(*it).second);

        LLD::CIndexDB indexdb("r");
        BOOST_FOREACH(CWalletTx* pcoin, vCoins)
        {
            // Find the corresponding transaction index
            Core::CTxIndex txindex;
            if(!indexdb.ReadTxIndex(pcoin->GetHash(), txindex))
                continue;

            /* Check all the outputs to make sure the flags are all set properly. */
            for (int n=0; n < pcoin->vout.size(); n++)
            {
                /* Handle the Index on Disk for Transaction being inconsistent from the Wallet's accounting to the UTXO. */
                if (IsMine(pcoin->vout[n]) && pcoin->IsSpent(n) && (txindex.vSpent.size() <= n || txindex.vSpent[n].IsNull()))
                {
                    printf("FixSpentCoins found lost coin %s Nexus %s[%d], %s\n",
                        FormatMoney(pcoin->vout[n].nValue).c_str(), pcoin->GetHash().ToString().c_str(), n, fCheckOnly? "repair not attempted" : "repairing");
                    nMismatchFound++;
                    nBalanceInQuestion += pcoin->vout[n].nValue;
                    if (!fCheckOnly)
                    {
                        pcoin->MarkUnspent(n);
                        pcoin->WriteToDisk();
                    }
                }

                /* Handle the wallet missing a spend that was updated in the indexes. The index is updated on connect inputs. */
                else if (IsMine(pcoin->vout[n]) && !pcoin->IsSpent(n) && (txindex.vSpent.size() > n && !txindex.vSpent[n].IsNull()))
                {
                    printf("FixSpentCoins found spent coin %s Nexus %s[%d], %s\n",
                        FormatMoney(pcoin->vout[n].nValue).c_str(), pcoin->GetHash().ToString().c_str(), n, fCheckOnly? "repair not attempted" : "repairing");
                    nMismatchFound++;
                    nBalanceInQuestion += pcoin->vout[n].nValue;
                    if (!fCheckOnly)
                    {
                        pcoin->MarkSpent(n);
                        pcoin->WriteToDisk();
                    }
                }
            }
        }
    }

    // Nexus: disable transaction (only for coinstake)
    void CWallet::DisableTransaction(const Core::CTransaction &tx)
    {
        if (!tx.IsCoinStake() || !IsFromMe(tx))
            return; // only disconnecting coinstake requires marking input unspent

        LOCK(cs_wallet);
        BOOST_FOREACH(const Core::CTxIn& txin, tx.vin)
        {
            map<uint512, CWalletTx>::iterator mi = mapWallet.find(txin.prevout.hash);
            if (mi != mapWallet.end())
            {
                CWalletTx& prev = (*mi).second;
                if (txin.prevout.n < prev.vout.size() && IsMine(prev.vout[txin.prevout.n]))
                {
                    prev.MarkUnspent(txin.prevout.n);
                    prev.WriteToDisk();
                }
            }
        }
    }

    vector<unsigned char> CReserveKey::GetReservedKey()
    {
        if (nIndex == -1)
        {
            CKeyPool keypool;
            pwallet->ReserveKeyFromKeyPool(nIndex, keypool);
            if (nIndex != -1)
                vchPubKey = keypool.vchPubKey;
            else
            {
                printf("CReserveKey::GetReservedKey(): Warning: using default key instead of a new key, top up your keypool.");
                vchPubKey = pwallet->vchDefaultKey;
            }
        }
        assert(!vchPubKey.empty());
        return vchPubKey;
    }

    void CReserveKey::KeepKey()
    {
        if (nIndex != -1)
            pwallet->KeepKey(nIndex);
        nIndex = -1;
        vchPubKey.clear();
    }

    void CReserveKey::ReturnKey()
    {
        if (nIndex != -1)
            pwallet->ReturnKey(nIndex);
        nIndex = -1;
        vchPubKey.clear();
    }

    void CWallet::GetAllReserveAddresses(set<NexusAddress>& setAddress)
    {
        setAddress.clear();

        CWalletDB walletdb(strWalletFile);

        LOCK2(Core::cs_main, cs_wallet);
        BOOST_FOREACH(const int64_t& id, setKeyPool)
        {
            CKeyPool keypool;
            if (!walletdb.ReadPool(id, keypool))
                throw runtime_error("GetAllReserveKeyHashes() : read failed");
            NexusAddress address(keypool.vchPubKey);
            assert(!keypool.vchPubKey.empty());
            if (!HaveKey(address))
                throw runtime_error("GetAllReserveKeyHashes() : unknown key in key pool");
            setAddress.insert(address);
        }
    }

}
