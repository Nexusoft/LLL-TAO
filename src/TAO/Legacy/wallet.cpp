/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <openssl/rand.h>   // For RAND_bytes

#include <thread>

#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <TAO/Ledger/include/global.h>

#include <TAO/Legacy/types/enum.h> // For GMF_SEND
#include <TAO/Legacy/wallet/crypter.h>
#include <TAO/Legacy/wallet/output.h>
#include <TAO/Legacy/wallet/script.h>
#include <TAO/Legacy/wallet/wallet.h>
#include <TAO/Legacy/wallet/walletdb.h>

#include <Util/include/args.h>
#include <Util/include/runtime.h>

namespace Legacy
{

    /* Nexus: Setting to unlock wallet for block minting only */
    bool fWalletUnlockMintOnly = false;


    /* Assign the minimum supported version for the wallet. */
    bool CWallet::SetMinVersion(const enum Legacy::WalletFeature nVersion, const bool fForceLatest)
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Ignore new setting if current setting is higher version */
            if (nWalletVersion >= nVersion)
                return true; 

            /* When force, if we pass the max version currently supported, use latest */
            if (fForceLatest && nVersion > nWalletMaxVersion)
                    nVersion = FEATURE_LATEST;

            nWalletVersion = nVersion;

            /* If new min version exceeds old max version, update the max version */
            if (nVersion > nWalletMaxVersion)
                nWalletMaxVersion = nVersion;

            if (fFileBacked)
            {
                /* Store new version to database (overwrites old) */
                CWalletDB walletdb(strWalletFile);
                walletdb.WriteMinVersion(nWalletVersion);
                walletdb.close();
            }
        }

        return true;
    }


    /* Assign the maximum version we're allowed to upgrade to.  */
    bool CWallet::SetMaxVersion(const int nVersion)
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Cannot downgrade below current version */
            if (nWalletVersion > nVersion)
                return false;

            nWalletMaxVersion = nVersion;
        }

        return true;
    }


    /* Loads all data for a file backed wallet from the database. */
    int CWallet::LoadWallet(bool& fFirstRunRet)
    {
        if (!fFileBacked)
            return false;

        /* If wallet was already loaded, just return */
        if (fLoaded)
            return DB_LOAD_OK;

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            fFirstRunRet = false;

            CWalletDB walletdb(strWalletFile,"cr+");
            int nLoadWalletRet = walletdb.LoadWallet(*this);
            walletdb.Close();

            if (nLoadWalletRet == DB_NEED_REWRITE)
                CDB::DBRewrite(strWalletFile);

            if (nLoadWalletRet != DB_LOAD_OK)
                return nLoadWalletRet;

            /* New wallet is indicated by an empty default key */
            fFirstRunRet = vchDefaultKey.empty();
        }

        /* Launch background thread to periodically flush the wallet to the backing database */
        std::thread flushThread(Legacy::ThreadFlushWalletDB, std::string(strWalletFile));
        flushThread.detach();

        fLoaded = true;

        return DB_LOAD_OK;
    }


    /*  Tracks requests for transactions contained in this wallet, or the blocks that contain them. */
    void CWallet::Inventory(const uint1024_t &hash)
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            auto mi = mapRequestCount.find(hash);

            if (mi != mapRequestCount.end())
            {
                /* Map contains an entry for the given block hash (we have one or more transactions in that block) */
                (*mi).second++;
            }
        }
    }


    /* Add a public/encrypted private key pair to the key store. */
    bool CWallet::AddCryptedKey(const vector<uint8_t> &vchPubKey, const vector<uint8_t> &vchCryptedSecret)
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Call overridden inherited method to add key to key store */
            if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
                return false;

            if (fFileBacked) 
            {
                CWalletDB walletdb(strWalletFile);
                bool result = walletdb.WriteCryptedKey(vchPubKey, vchCryptedSecret);
                walletdb.Close();

                return result;
            }
        }

        return true;
    }


    /* Add a key to the key store. */
    bool CWallet::AddKey(const ECKey& key)
    {
        /*
         * This works in a convoluted manner for encrypted wallets.
         *   1. This method calls CCryptoKeyStore::AddKey()
         *   2. If wallet is not encrypted, that method adds key to key store, but not to database (as expected)
         *   3. If wallet is encrypted (and unlocked), CCryptoKeyStore::AddKey encrypts the key and calls AddCryptedKey()
         *   4. Because this is a CWallet instance, that call will actually call CWallet::AddCryptedKey() and 
         *      not the more obvious CCryptoKeyStore::AddCryptedKey()
         *
         * In other words, the call to AddCryptedKey() within CCryptoKeyStore::AddKey is actually performing a
         * polymorphic call to this->AddCryptedKey() to execute the method in the derived CWallet class.
         *
         * The CWallet version of AddCryptedKey() handles adding the encrypted key to both key store and database.
         * The result: only need to write to database here if wallet is not encrypted
         *
         * Would be better to have a more intuitive way for code to handle encrypted key, but this way does work.
         * It violates encapsulation, though, because we should not have to rely on how CCryptoKeyStore implements AddKey
         */
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Call overridden method to add key to key store */
            /* For encrypted wallet, this adds to both key store and wallet database (as described above) */
            if (!CCryptoKeyStore::AddKey(key))
                return false;

            if (fFileBacked && !IsCrypted()) 
            {
                /* Only if wallet is not encrypted */
                CWalletDB walletdb(strWalletFile);
                bool result = walletdb.WriteKey(key.GetPubKey(), key.GetPrivKey());
                walletdb.Close();

                return result;
            }
        }

        return false;
    }


    /* Add a script to the key store.  */
    bool CWallet::AddCScript(const CScript& redeemScript)
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Call overridden inherited method to add key to key store */
            if (!CCryptoKeyStore::AddCScript(redeemScript))
                return false;

            if (fFileBacked) 
            {
                CWalletDB walletdb(strWalletFile);
                bool result = walletdb.WriteCScript(SK256(redeemScript), redeemScript);
                walletdb.Close();

                return result;
            }
        }

        return true;
    }


    /* Generates a new key and adds it to the key store. */
    std::vector<uint8_t> CWallet::GenerateNewKey()
    {
        bool fCompressed = true;

        RandAddSeedPerfmon();
        ECKey key;
        key.MakeNewKey(fCompressed);

        /* AddKey adds to key store, encrypting it first if wallet is encrypted, and writes key to database if file backed */
        /* AddKey also performs wallet locking, so no lock guard needed here */
        if (!AddKey(key))
            throw std::runtime_error("CWallet::GenerateNewKey() : AddKey failed");

        return key.GetPubKey();
    }


    /* Assigns a new default key to this wallet. */
    bool CWallet::SetDefaultKey(const std::vector<uint8_t> &vchPubKey)
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            if (fFileBacked) 
            {
                CWalletDB walletdb(strWalletFile);
                bool result = WriteDefaultKey(vchPubKey);
                walletdb.Close();

                if (!result)
                    return false;
            }

            vchDefaultKey = vchPubKey;
        }

        return true;
    }


    /* Encrypts the wallet in both memory and file backing, assigning a passphrase that will be required 
     * to unlock and access the wallet. 
     */
    bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
    {
        if (IsCrypted())
            return false;

        CCrypter crypter;
        CMasterKey kMasterKey;

        CKeyingMaterial vMasterKey;
        RandAddSeedPerfmon();

        /* Fill keying material (unencrypted key value) and new master key salt with random data using OpenSSL RAND_bytes */
        vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
        RAND_bytes(&vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

        RandAddSeedPerfmon();
        kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
        RAND_bytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

        /* Use time to process 2 calls to SetKeyFromPassphrase to create a nDeriveIterations value for master key */
        int64_t nStartTime = Timestamp(true);
        crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
        kMasterKey.nDeriveIterations = 2500000 / ((double)(Timestamp(true) - nStartTime));

        nStartTime = Timestamp(true);
        crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
        kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + kMasterKey.nDeriveIterations * 100 / ((double)(Timestamp(true) - nStartTime))) / 2;

        /* Assure a minimum value */
        if (kMasterKey.nDeriveIterations < 25000)
            kMasterKey.nDeriveIterations = 25000;

        printf("Encrypting Wallet with nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);

        /* Encrypt the master key value using the new passphrase */
        if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
            return false;

        if (!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey))
            return false;

        /* kMasterKey now contains the master key encrypted by the provided passphrase. Ready to perform wallet encryption. */
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;

            if (fFileBacked)
            {
                CWalletDB walletdb(strWalletFile);

                if (!walletdb->TxnBegin())
                    return false;

                walletdb.WriteMasterKey(nMasterKeyMaxID, kMasterKey);
            }

            /* EncryptKeys() in CCryptoKeyStore will encrypt every public key/private key pair in the key store, including those that 
             * are part of the key pool. It calls CCryptoKeyStore::AddCryptedKey() to add each to the key store, which will polymorphically 
             * call CWallet::AddCryptedKey and also write them to the database.
             *
             * See CWallet::AddKey() for more discussion on how this works
             *
             * When it writes the encrypted key to the database, it will also remove any unencrypted entry for the same public key
             */
            if (!EncryptKeys(vMasterKey))
            {
                if (fFileBacked)
                    walletdb.TxnAbort();

                /* We now probably have half of our keys encrypted in memory, 
                 * and half not...die to let the user reload their unencrypted wallet. 
                 */
                exit(1); 
            }

            if (fFileBacked)
            {
                if (walletdb.TxnCommit())
                {
                    /* Keys encrypted in memory, but not on disk...die to let the user reload their unencrypted wallet. */
                    exit(1); 
                }

                walletdb.Close();
            }

            Lock();
            Unlock(strWalletPassphrase);
            keyPool.NewKeyPool();
            Lock();

            /* Need to completely rewrite the wallet file; if we don't, bdb might keep
             * bits of the unencrypted private key in slack space in the database file.
             */
            CDB::DBRewrite(strWalletFile);
        }

        return true;
    }


    /* Attempt to unlock an encrypted wallet using the passphrase provided. */
    bool CWallet::Unlock(const SecureString& strWalletPassphrase)
    {
        if (!IsLocked())
            return false;

        CCrypter crypter;
        CKeyingMaterial vMasterKey;

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* If more than one master key in wallet's map (unusual), this will attempt each one with the passphrase. 
             * If any one master key decryption works and unlocks the wallet, then the unlock is successful.
             * Supports a multi-user wallet, where each user has their own passphrase
             */
            for(auto pMasterKey : mapMasterKeys)
            {
                /* Set the encryption context using the passphrase provided */
                if(!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;

                /* Attempt to decrypt the master key using the passphrase crypter */
                if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                    return false;

                /* Attempt to unlock the wallet using the decrypted value for the master key */
                if (CCryptoKeyStore::Unlock(vMasterKey))
                    return true;
            }
        }

        return false;
    }


    /* Replaces the existing wallet passphrase with a new one. */
    bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
    {
        /* To change the passphrase, the old passphrase must successfully decrypt the master key and unlock
         * the wallet, thus proving it is the correct one. Then, the master key is re-encrypted using the 
         * new passphrase and saved.
         */

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Save current lock state so it can be reset when done */
            bool fWasLocked = IsLocked();

            /* Lock the wallet so we can use unlock to verify old passphrase */
            Lock();

            CCrypter crypter;
            CKeyingMaterial vMasterKey;

            /* If more than one master key in wallet's map (unusual), have to find the one that corresponds to old passphrase. 
             * Do this by attempting to use each to unlock with old passphrase until find a match.
             */
            for(auto& pMasterKey : mapMasterKeys)
            {
                /* Attempt to decrypt the current master key and unlock the wallet with it */
                if(!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;

                if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                    return false;

                if (CCryptoKeyStore::Unlock(vMasterKey))
                {
                    /* Successfully unlocked, so pMasterKey is the map entry that corresponds to the old passphrase
                     * Now change that passphrase by re-encrypting master key with new one.
                     */

                    /* Use time to process 2 calls to SetKeyFromPassphrase to create a new nDeriveIterations value for master key */
                    int64_t nStartTime = Timestamp(true);
                    crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                    pMasterKey.second.nDeriveIterations = pMasterKey.second.nDeriveIterations * (100 / ((double)(Timestamp(true) - nStartTime)));

                    nStartTime = Timestamp(true);
                    crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                    pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + pMasterKey.second.nDeriveIterations * 100 / ((double)(Timestamp(true) - nStartTime))) / 2;

                    /* Assure a minimum value */
                    if (pMasterKey.second.nDeriveIterations < 25000)
                        pMasterKey.second.nDeriveIterations = 25000;

                    printf("Wallet passphrase changed to use nDeriveIterations of %i\n", pMasterKey.second.nDeriveIterations);

                    /* Re-encrypt the master key using the new passphrase */
                    if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                        return false;

                    if (!crypter.Encrypt(vMasterKey, pMasterKey.second.vchCryptedKey))
                        return false;

                    if (fFileBacked)
                    {
                        /* Store new master key encryption to the wallet database (overwrites old value)*/
                        CWalletDB walletdb(strWalletFile);
                        walletdb.WriteMasterKey(pMasterKey.first, pMasterKey.second);
                        walletdb.Close();
                    }

                    /* Relock file if it was locked when we started */
                    if (fWasLocked)
                        Lock();

                    return true;
                }
            }
        }

        return false;
    }


    /* Retrieves the total wallet balance for all confirmed, mature transactions. */
    int64_t CWallet::GetBalance() const
    {
        int64_t nTotalBalance = 0;

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            for (const auto& item : mapWallet)
            {
                const CWalletTx& walletTx = item.second;

                /* Skip any transaction that isn't final, isn't completely confirmed, or has a future timestamp */
                if (!walletTx.IsFinal() || !walletTx.IsConfirmed() || walletTx.nTime > UnifiedTimestamp())
                    continue;

                nTotalBalance += walletTx.GetAvailableCredit();
            }
        }

        return nTotalBalance;
    }


    /* Retrieves the current wallet balance for unconfirmed transactions. */
    int64_t CWallet::GetUnconfirmedBalance() const
    {
        int64_t nUnconfirmedBalance = 0;

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            for (const auto& item : mapWallet)
            {
                const CWalletTx& walletTx = item.second;

                if (walletTx.IsFinal() && walletTx.IsConfirmed())
                    continue;

                nUnconfirmedBalance += walletTx.GetAvailableCredit();
            }
        }

        return nUnconfirmedBalance;
    }


    /* Retrieves the current immature stake balance. */
    int64_t CWallet::GetStake() const
    {
        int64_t nTotalStake = 0;

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            for (const auto& item : mapWallet)
            {
                const CWalletTx& walletTx = item.second;

                if (walletTx.IsCoinStake() && walletTx.GetBlocksToMaturity() > 0 && walletTx.GetDepthInMainChain() > 1)
                    nTotalStake += GetCredit(walletTx);
            }
        }

        return nTotalStake;
    }


    /* Retrieves the current immature minted (mined) balance. */
    int64_t CWallet::GetNewMint() const
    {
        int64_t nTotalMint = 0;

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            for (const auto& item : mapWallet)
            {
                const CWalletTx& walletTx = item.second;

                if (walletTx.IsCoinBase() && walletTx.GetBlocksToMaturity() > 0 && walletTx.GetDepthInMainChain() > 1)
                    nTotalMint += GetCredit(walletTx);
            }
        }

        return nTotalMint;
    }


    /* Populate vCoins with vector identifying spendable outputs. */
    void CWallet::AvailableCoins(const uint32_t nSpendTime, vector<COutput>& vCoins, const bool fOnlyConfirmed) const
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            vCoins.clear();

            for (const auto& item : mapWallet)
            {
                const CWalletTx& walletTx = item.second;

                /* Filter transactions not final */
                if (!walletTx.IsFinal())
                    continue;

                /* Filter unconfirmed transactions unless want unconfirmed */
                if (fOnlyConfirmed && !walletTx.IsConfirmed())
                    continue;

                /* Filter immature minting and staking transactions */
                if ((walletTx.IsCoinBase() || walletTx.IsCoinStake()) && walletTx.GetBlocksToMaturity() > 0)
                    continue;

                for (int i = 0; i < walletTx.vout.size(); i++)
                {
                    /* Filter transactions after requested spend time */
                    if (walletTx.nTime > nSpendTime)
                        continue;  

                    /* To be included in result, vout must not be spent, must belong to current wallet, and must have positive value */
                    if (!(walletTx.IsSpent(i)) && IsMine(walletTx.vout[i]) && walletTx.vout[i].nValue > 0)
                    {
                        /* Create output from the current vout and add to result */
                        COutput txOutput(walletTx, i, walletTx.GetDepthInMainChain());
                        vCoins.push_back(txOutput);
                    }
                }
            }
        }
    }


    /* Mark all transactions in the wallet as "dirty" to force balance recalculation. */
    void CWallet::MarkDirty()
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            for(auto& item : mapWallet)
                item.second.MarkDirty();
        }
    }


    /*  Retrieves the transaction for a given transaction hash. */
    bool CWallet::GetTransaction(const uint512_t &hashTx, CWalletTx& wtx)
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Find the requested transaction in the wallet */
            TransactionMap::iterator mi = mapWallet.find(hashTx);

            if (mi != mapWallet.end())
            {
                wtx = (*mi).second;
                return true;
            }
        }

        return false;
    }


    /* Adds a wallet transaction to the wallet. */
    bool CWallet::AddToWallet(const CWalletTx& wtxIn)
    {
        uint512_t hash = wtxIn.GetHash();

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Inserts only if not already there, returns tx inserted or tx found */
            pair<TransactionMap::iterator, bool> ret = mapWallet.insert(std::make_pair(hash, wtxIn));

            /* Use the returned tx, not wtxIn, in case insert returned an existing transaction */
            CWalletTx& wtx = (*ret.first).second;
            wtx.BindWallet(*this);

            bool fInsertedNew = ret.second;
            if (fInsertedNew)
                wtx.nTimeReceived = UnifiedTimestamp();

            bool fUpdated = false;
            if (!fInsertedNew)
            {
                /* If found an existing transaction, merge the new one into it */
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

                /* Merge spent flags */
                fUpdated |= wtx.UpdateSpent(wtxIn.vfSpent);
            }

            /* debug print */
            printf("AddToWallet %s  %s%s\n", wtxIn.GetHash().ToString().substr(0,10).c_str(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

            /* Write to disk */
            if (fInsertedNew || fUpdated)
                if (!wtx.WriteToDisk())
                    return false;

    /* Update for new interface? Is this needed? */
    #ifndef QT_GUI
            /* If default receiving address gets used, replace it with a new one */
            CScript scriptDefaultKey;
            scriptDefaultKey.SetNexusAddress(vchDefaultKey);

            for(const CTxOut& txout : wtx.vout)
            {
                if (txout.scriptPubKey == scriptDefaultKey)
                {
                    std::vector<uint8_t> newDefaultKey;

                    if (keyPool.GetKeyFromPool(newDefaultKey, false))
                    {
                        SetDefaultKey(newDefaultKey);
                        SetAddressBookName(NexusAddress(vchDefaultKey), "");
                    }
                }
            }
    #endif

            /* since AddToWallet is called directly for self-originating transactions, check for consumption of own coins */
            WalletUpdateSpent(wtx);
        }

        return true;
    }


    /*  Checks whether a transaction has inputs or outputs belonging to this wallet, and adds 
     *  it to the wallet when it does.
     */
    bool CWallet::AddToWalletIfInvolvingMe(const Core::CTransaction& tx, const Core::CBlock* pblock, bool fUpdate, bool fFindBlock)
    {
        uint512_t hash = tx.GetHash();

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Check to see if transaction hash in this wallet */
            bool fExisted = mapWallet.count(hash);

            /* When transaction already in wallet, return unless update is specifically requested */
            if (fExisted && !fUpdate) 
                return false;

            /* Check if transaction has outputs (IsMine) or inputs (IsFromMe) belonging to this wallet */
            if (IsMine(tx) || IsFromMe(tx))
            {
                CWalletTx wtx(this,tx);

                /* Get merkle branch if transaction was found in a block */
                if (pblock)
                    wtx.SetMerkleBranch(pblock);

                /* AddToWallet preforms merge (update) for transactions already in wallet */
                return AddToWallet(wtx);
            }
            else
                WalletUpdateSpent(tx);
        }

        return false;
    }


    /* Removes a wallet transaction from the wallet, if present. */
    bool CWallet::EraseFromWallet(const uint512_t hash)
    {
        if (!fFileBacked)
            return false;
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            if (mapWallet.erase(hash))
            {
                CWalletDB(strWalletFile) walletdb;
                walletdb.EraseTx(hash);
                walletdb.Close();
            }
        }

        return true;
    }


    /* When disconnecting a coinstake transaction, this method to marks
     *  any previous outputs from this wallet as unspent.
     */
    void CWallet::DisableTransaction(const CTransaction &tx)
    {
        /* If transaction is not coinstake or not from this wallet, nothing to process */
        if (!tx.IsCoinStake() || !IsFromMe(tx))
            return; 

        {
            /* Disconnecting coinstake requires marking input unspent */
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            for(const CTxIn& txin : tx.vin)
            {
                /* Find the previous transaction and mark as unspent the output that corresponds to current txin */
                TransactionMap::iterator mi = mapWallet.find(txin.prevout.hash);

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
    }


    /* Scan the block chain for transactions from or to keys in this wallet.
     * Add/update the current wallet transactions for any found.
     */
    int CWallet::ScanForWalletTransactions(Core::CBlockIndex* pindexStart, const bool fUpdate)
    {
        /* Count the number of transactions process for this wallet to use as return value */
        int nTransactionCount = 0;

        Core::CBlockIndex* pindex = pindexStart;
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            while (pindex)
            {
                /* Get next block in the chain */
                Core::CBlock block;
                block.ReadFromDisk(pindex, true);

                /* Scan each transaction in the block and process those related to this wallet */
                for(Core::CTransaction& tx : block.vtx)
                {
                    if (AddToWalletIfInvolvingMe(tx, &block, fUpdate))
                        nTransactionCount++;
                }

                /* Move to next block. Will be nullptr when reach end of chain, ending the while loop */
                pindex = pindex->pnext;
            }
        }

        return nTransactionCount;
    }


    /* Looks through wallet for transactions that should already have been added to a block, but are
     * still pending, and re-broadcasts them to then network.
     */
    void CWallet::ResendWalletTransactions()
    {
        /* Do this infrequently and randomly to avoid giving away that these are our transactions.
         * 
         * Uses static snNextTime for this purpose. On first call, sets a random value (up to 30 minutes)
         * and returns. Any subsequent calls will only process resend if at least that much time
         * has passed. 
         */
        static int64_t snNextTime;
        static int64_t snLastTime;
        bool fFirst = (snNextTime == 0);

        /* Always false on first iteration */
        if (UnifiedTimestamp() < snNextTime)
            return;

        /* Set a random time until resend is processed */
        snNextTime = UnifiedTimestamp() + GetRand(30 * 60);

        /* On first iteration, just return. All it does is set snNextTime */
        if (fFirst)
            return;

        /* static snLastTime is just to check if there is new block since last time we processed resend.
         * If no new block, nothing has changed, so just returns. Would only be true if random snNextTime
         * interval were less than the block interval.
         */
        if (Core::nTimeBestReceived < snLastTime)
            return;

        /* Record that it is processing resend now */
        snLastTime = UnifiedTimestamp();

        /* Rebroadcast any of our tx that aren't in a block yet */
        printf("ResendWalletTransactions()\n");
        LLD::CIndexDB indexdb("r");

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Find any sent tx not in block and sort them in chronological order */
            multimap<uint64_t, CWalletTx&> mapSorted;
            for(auto item& : mapWallet)
            {
                CWalletTx& wtx = item.second;

                /* Don't put in sorted map for rebroadcast until it's had enough time to be added to a block */
                if (Core::nTimeBestReceived - wtx.nTimeReceived > 5 * 60)
                    mapSorted.insert(std::make_pair(wtx.nTimeReceived, wtx));
            }

            for(auto item& : mapSorted)
            {
                CWalletTx& wtx = item.second;

                /* Validate the transaction, then process rebroadcast on it */
                if (wtx.CheckTransaction())
                    wtx.RelayWalletTransaction(indexdb);
                else
                    printf("ResendWalletTransactions() : CheckTransaction failed for transaction %s\n", wtx.GetHash().ToString().c_str());
            }
        }
    }


    /* Checks a transaction to see if any of its inputs match outputs from wallet transactions 
     * in this wallet. For any it finds, verifies that the outputs are marked as spent, updating 
     * them as needed.
     */
    void CWallet::WalletUpdateSpent(const CTransaction &tx)
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Loop through and the tx inputs, checking each separately */
            for(const auto& txin : tx.vin)
            {
                /* Check the txin to see if prevout hash maps to a transaction in this wallet */
                TransactionMap::iterator mi = mapWallet.find(txin.prevout.hash);

                if (mi != mapWallet.end())
                {
                    /* When there is a match to the prevout hash, get the wallet transaction */
                    CWalletTx& wtx = (*mi).second;

                    /* Outputs in wallet tx will have same index recorded in transaction txin
                     * Check for belonging to this wallet any that are not flagged spent and mark them as spent
                     */
                    if (!wtx.IsSpent(txin.prevout.n) && IsMine(wtx.vout[txin.prevout.n]))
                    {
                        printf("WalletUpdateSpent found spent coin %s Nexus %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                        wtx.MarkSpent(txin.prevout.n);
                        wtx.WriteToDisk();
                    }
                }
            }
        }
    }


    /*  Identifies and fixes mismatches of spent coins between the wallet and the index db.  */
    void CWallet::FixSpentCoins(int& nMismatchFound, int64_t& nBalanceInQuestion, const bool fCheckOnly)
    {
        nMismatchFound = 0;
        nBalanceInQuestion = 0;

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            vector<CWalletTx&> vCoins;
            vCoins.reserve(mapWallet.size());

            for (auto& item : mapWallet)
                vCoins.push_back(item.second);

            LLD::CIndexDB indexdb("r");
            for(CWalletTx& walletTx : vCoins)
            {
                /* Find the corresponding transaction index */
                Core::CTxIndex txindex;
                if(!indexdb.ReadTxIndex(walletTx.GetHash(), txindex))
                    continue;

                /* Check all the outputs to make sure the flags are all set properly. */
                for (int n=0; n < walletTx.vout.size(); n++)
                {
                    /* Handle the Index on Disk for Transaction being inconsistent from the Wallet's accounting to the UTXO. */
                    if (IsMine(walletTx.vout[n]) && walletTx.IsSpent(n) && (txindex.vSpent.size() <= n || txindex.vSpent[n].IsNull()))
                    {
                        printf("FixSpentCoins found lost coin %s Nexus %s[%d], %s\n",
                            FormatMoney(walletTx.vout[n].nValue).c_str(), walletTx.GetHash().ToString().c_str(), n, fCheckOnly? "repair not attempted" : "repairing");

                        nMismatchFound++;

                        nBalanceInQuestion += walletTx.vout[n].nValue;

                        if (!fCheckOnly)
                        {
                            walletTx.MarkUnspent(n);
                            walletTx.WriteToDisk();
                        }
                    }

                    /* Handle the wallet missing a spend that was updated in the indexes. The index is updated on connect inputs. */
                    else if (IsMine(walletTx.vout[n]) && !walletTx.IsSpent(n) && (txindex.vSpent.size() > n && !txindex.vSpent[n].IsNull()))
                    {
                        printf("FixSpentCoins found spent coin %s Nexus %s[%d], %s\n",
                            FormatMoney(walletTx.vout[n].nValue).c_str(), walletTx.GetHash().ToString().c_str(), n, fCheckOnly? "repair not attempted" : "repairing");

                        nMismatchFound++;

                        nBalanceInQuestion += walletTx.vout[n].nValue;

                        if (!fCheckOnly)
                        {
                            walletTx.MarkSpent(n);
                            walletTx.WriteToDisk();
                        }
                    }
                }
            }
        }
    }


    /* Checks whether a transaction contains any outputs belonging to this wallet. */
    bool CWallet::IsMine(const CTransaction& tx) const
    {
        for(const CTxOut& txout : tx.vout)
        {
            if (IsMine(txout))
                return true;
        }

        return false;
    }


     /* Checks whether a specific transaction input represents a send from this wallet. */
    bool CWallet::IsMine(const Core::CTxIn &txin) const
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* Any input from this wallet will have a corresponding UTXO in the previous transaction 
             * Thus, if the wallet doesn't contain the previous transaction, the input is not from this wallet.
             * If it does contain the previous tx, must still check that the specific output matching
             * this input belongs to it.
             */
            auto mi = mapWallet.find(txin.prevout.hash);

            if (mi != mapWallet.end())
            {
                const CWalletTx& prev = (*mi).second;

                if (txin.prevout.n < prev.vout.size())
                {
                    /* If the matching txout in the previous tx is from this wallet, then this txin is from this wallet */
                    if (IsMine(prev.vout[txin.prevout.n]))
                        return true;
                }
            }
        }
        return false;
    }


    /* Checks whether a specific transaction output represents balance received by this wallet. */
    bool CWallet::IsMine(const CTxOut& txout) const
    {
        /* Output belongs to this wallet if it has a key matching the output script */
        return IsMine(*this, txout.scriptPubKey);
    }


    /* Calculates the total value for all inputs sent from this wallet by a transaction. */
    int64_t CWallet::GetDebit(const CTransaction& tx) const
    {
        int64_t nDebit = 0;

        for(const auto& txin : tx.vin)
        {
            nDebit += GetDebit(txin);

            if (!Core::MoneyRange(nDebit))
                throw std::runtime_error("CWallet::GetDebit() : value out of range");
        }

        return nDebit;
    }


    /* Calculates the total value for all outputs received by this wallet in a transaction. */
    int64_t CWallet::GetCredit(const CTransaction& tx) const
    {
        int64_t nCredit = 0;

        for(const auto& txout : tx.vout)
        {
            nCredit += GetCredit(txout);

            if (!Core::MoneyRange(nCredit))
                throw std::runtime_error("CWallet::GetCredit() : value out of range");
        }

        return nCredit;
    }


    /* Calculates the total change amount returned to this wallet by a transaction. */
    int64_t CWallet::GetChange(const CTransaction& tx) const
    {
        int64_t nChange = 0;

        for(const CTxOut& txout : tx.vout)
        {
            nChange += GetChange(txout);

            if (!Core::MoneyRange(nChange))
                throw std::runtime_error("CWallet::GetChange() : value out of range");
        }

        return nChange;
    }


    /* Returns the debit amount for this wallet represented by a transaction input. */
    int64_t CWallet::GetDebit(const Core::CTxIn &txin) const
    {
        if(txin.prevout.IsNull())
            return 0;

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            /* A debit spends the txout value from a previous output 
             * so begin by finding the previous transaction in the wallet
             */
            auto mi = mapWallet.find(txin.prevout.hash);

            if (mi != mapWallet.end())
            {
                const CWalletTx& prev = (*mi).second;

                if (txin.prevout.n < prev.vout.size())
                {
                    /* If the previous txout belongs to this wallet, then debit is from this wallet */
                    if (IsMine(prev.vout[txin.prevout.n]))
                        return prev.vout[txin.prevout.n].nValue;
                }
            }
        }

        return 0;
    }


    /* Returns the credit amount for this wallet represented by a transaction output. */
    int64_t CWallet::GetCredit(const CTxOut& txout) const
    {
        if (!Core::MoneyRange(txout.nValue))
            throw std::runtime_error("CWallet::GetCredit() : value out of range");

        return (IsMine(txout) ? txout.nValue : 0);
    }


    /* Returns the change amount for this wallet represented by a transaction output. */
    int64_t CWallet::GetChange(const CTxOut& txout) const
    {
        if (!Core::MoneyRange(txout.nValue))
            throw std::runtime_error("CWallet::GetChange() : value out of range");

        return (IsChange(txout) ? txout.nValue : 0);
    }


    /* Checks whether a transaction output belongs to this wallet and
     *  represents change returned to it. 
     */
    bool CWallet::IsChange(const Core::CTxOut& txout) const
    {
        NexusAddress address;

        /* TODO: fix handling of 'change' outputs. The assumption is that any
         * payment to a TX_PUBKEYHASH that is mine but isn't in the address book
         * is change. That assumption is likely to break when we implement multisignature
         * wallets that return change back into a multi-signature-protected address;
         * a better way of identifying which outputs are 'the send' and which are
         * 'the change' will need to be implemented (maybe extend CWalletTx to remember
         * which output, if any, was change).
         */
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            if (ExtractAddress(txout.scriptPubKey, address) && HaveKey(address))
            {
                if (!mapAddressBook.count(address))
                    return true;
            }
        }

        return false;
    }


    /* Generate a transaction to send balance to a given Nexus address. */
    std::string CWallet::SendToNexusAddress(const NexusAddress& address, int64_t nValue, CWalletTx& wtxNew, bool fAskFee)
    {
        /* Validate amount */
        if (nValue <= 0)
            return _("Invalid amount");

        /* Validate balance supports value + fees */
        if (nValue + Core::nTransactionFee > GetBalance())
            return _("Insufficient funds");

        /* Parse nexus address */
        CScript scriptPubKey;
        scriptPubKey.SetNexusAddress(address);

        /* Place the script and amount into sending vector */
        vector< pair<CScript, int64_t> > vecSend;
        vecSend.push_back(make_pair(scriptPubKey, nValue));

        /* Key will be reserved for any change transaction, kept on commit */
        CReserveKey reservekey(*this); 

        int64_t nFeeRequired;

        if (IsLocked())
        {
            /* Cannot create transaction when wallet locked */
            string strError = _("Error: Wallet locked, unable to create transaction  ");
            printf("SendMoney() : %s", strError.c_str());
            return strError;
        }

        if (fWalletUnlockMintOnly)
        {
            /* Cannot create transaction if unlocked for mint only */
            string strError = _("Error: Wallet unlocked for block minting only, unable to create transaction.");
            printf("SendMoney() : %s", strError.c_str());
            return strError;
        }

        if (!CreateTransaction(vecSend, wtxNew, reservekey, nFeeRequired))
        {
            /* Transaction creation failed */
            string strError;
            if (nValue + nFeeRequired > GetBalance())
            {
                /* Failure resulted because required fee caused transaction amount to exceed available balance. 
                 * Really should not get this because of initial check at start of function. Could only happen
                 * if nFeeRequired > Core::nTransactionFee
                 */
                strError = strprintf(_("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds  "), FormatMoney(nFeeRequired).c_str());
            }
            else
            {
                /* Other transaction creation failure */
                strError = _("Error: Transaction creation failed  ");
            }

            printf("SendMoney() : %s", strError.c_str());

            return strError;
        }

        /* With QT interface removed, we no longer display the fee confirmation here. Successful transaction creation will be committed automatically */
        if (!CommitTransaction(wtxNew, reservekey))
            return _("Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");

        return "";
    }


    /* Create and populate a new transaction. */
    bool CWallet::CreateTransaction(const vector<pair<CScript, int64_t> >& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet)
    {
        int64_t nValue = 0;

        /* Calculate total send amount */
        for (const auto& s : vecSend)
        {
            if (nValue < 0)
                return false; // Negative value invalid

            nValue += s.second;
        }

        /* Cannot create transaction if nothing to send */
        if (vecSend.empty() || nValue < 0)
            return false;

        /* Link transaction to wallet, don't add it yet (will be done when transaction committed) */
        wtxNew.BindWallet(this);

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            LLD::CIndexDB indexdb("r");

            nFeeRet = Core::nTransactionFee;

            /* This loop is generally executed only once, unless the size of the transaction requires a fee increase.
             * When fee increased, it is possible that selected inputs do not cover it, so repeat the process to 
             * assure we have enough value in. It also has to re-do the change calculation and output.
             */
            loop() {
                /* Reset transaction contents */
                wtxNew.vin.clear();
                wtxNew.vout.clear();
                wtxNew.fFromMe = true;

                int64_t nTotalValue = nValue + nFeeRet;

                /* Add transactions outputs to vout */
                for (auto s& : vecSend)
                    wtxNew.vout.push_back(CTxOut(s.second, s.first));

                /* This set will hold txouts to use as input for this transaction as transaction/vout index pairs */
                set<pair<const CWalletTx&,uint32_t> > setSelectedCoins;

                /* Initialize total value of all inputs */
                int64_t nValueIn = 0;

                /* Choose coins to use for transaction input */
                if (!SelectCoins(nTotalValue, wtxNew.nTime, setSelectedCoins, nValueIn))
                    return false;

                /* Process selected coins to get scriptChange */
                CScript scriptChange;
                for(auto item : setSelectedCoins)
                {
                    CWalletTx& selectedTransaction = = item.first;

                    /* When done, this will contain scriptPubKey of last transaction in the set */
                    scriptChange = selectedTransaction.vout[item.second].scriptPubKey;
                }

                /* Amount of change needed is total of inputs - (total sent + fee) */
                int64_t nChange = nValueIn - nTotalValue;

                if (nChange > 0)
                {
                    /* Reserve a new key pair from key pool to use for change */
                    vector<uint8_t> vchPubKeyChange = reservekey.GetReservedKey();

                    /* Fill a vout to return change */
                    scriptChange.SetNexusAddress(vchPubKeyChange);

                    /* Insert change output at random position: */
                    auto position = wtxNew.vout.begin() + GetRandInt(wtxNew.vout.size());
                    wtxNew.vout.insert(position, Core::CTxOut(nChange, scriptChange));

                }
                else
                    reservekey.ReturnKey();

                /* Fill vin with selected inputs */
                for(const auto coin : setCoins)
                    wtxNew.vin.push_back(Core::CTxIn(coin.first->GetHash(),coin.second));

                /* Sign inputs to unlock previously unspent outputs */
                int nIn = 0;
                for(const auto coin& : setSelectedCoins)
                    if (!SignSignature(*this, coin.first, wtxNew, nIn++))
                        return false;

                /* Limit tx size to 20% of max block size */
                uint32_t nBytes = ::GetSerializeSize(*(CTransaction*)&wtxNew, SER_NETWORK, LLP::PROTOCOL_VERSION);
                if (nBytes >= Core::MAX_BLOCK_SIZE_GEN/5)
                    return false; // tx size too large

                /* Each multiple of 1000 bytes of tx size multiplies the fee paid */
                int64_t nPayFee = Core::nTransactionFee * (1 + (int64_t)nBytes / 1000);

                /* Get minimum required fee from transaction */
                int64_t nMinFee = wtxNew.GetMinFee(1, false, Legacy::GMF_SEND);

                /* Check that enough fee is included */
                if (nFeeRet < max(nPayFee, nMinFee))
                {
                    /* More fee required, so increase fee and repeat loop */
                    nFeeRet = max(nPayFee, nMinFee);
                    continue;
                }

                /* Fill vtxPrev by copying from previous transactions vtxPrev */
                wtxNew.AddSupportingTransactions(indexdb);

                wtxNew.fTimeReceivedIsTxTime = true;

                break;
            }
        }
        return true;
    }


    /* Commits a transaction and broadcasts it to the network. */
    bool CWallet::CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey)
    {
        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            printf("CommitTransaction:\n%s", wtxNew.ToString().c_str());

            /* This is only to keep the database open to defeat the auto-flush for the
             * duration of this scope.  This is the only place where this optimization
             * maybe makes sense; please don't do it anywhere else.
             */
            CWalletDB& walletdb;
            if (fFileBacked)
                walletdb = CWalletDB(strWalletFile,"r");

            /* Take key pair from key pool so it won't be used again */
            reservekey.KeepKey();

            /* Add tx to wallet, because if it has change it's also ours, otherwise just for transaction history. */
            AddToWallet(wtxNew);

            /* Mark old coins as spent */
            set<CWalletTx*> setCoins;
            for(const Core::CTxIn& txin : wtxNew.vin)
            {
                CWalletTx& prevTx = mapWallet[txin.prevout.hash];
                prevTx.BindWallet(*this);
                prevTx.MarkSpent(txin.prevout.n);
                prevTx.WriteToDisk();
            }

            if (fFileBacked)
                delete pwalletdb;

            /* Add to tracking for how many getdata requests our transaction gets */
            /* This entry track the transaction hash (not block hash) */
            mapRequestCount[wtxNew.GetHash()] = 0;

            /* Broadcast transaction to network */
            if (!wtxNew.AcceptToMemoryPool())
            {
                /* This must not fail. The transaction has already been signed and recorded. */
                printf("CommitTransaction() : Error: Transaction not valid");
                return false;
            }

            wtxNew.RelayWalletTransaction();

            if (fFileBacked)
                walletdb.Close();
        }

        return true;
    }


    bool CWallet::AddCoinstakeInputs(Core::CBlock& block)
    {
        /* Add Each Input to Transaction. */
        vector<const CWalletTx&> vInputs;
        vector<const CWalletTx&> vCoins;

        block.vtx[0].vout[0].nValue = 0;

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            vCoins.reserve(mapWallet.size());

            for (auto& item : mapWallet)
                vCoins.push_back(item.second);
        }

        random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);

        for(auto walletTx : vCoins)
        {
            /* Can't spend balance that is unconfirmed or not final */
            if (!walletTx.IsFinal() || !walletTx.IsConfirmed())
                continue;

            /* Can't spend coinbase or coinstake transactions that are immature */
            if ((walletTx.IsCoinBase() || walletTx.IsCoinStake()) && walletTx.GetBlocksToMaturity() > 0)
                continue;

            /* Do not add coins to Genesis block if age less than trust timestamp */
            if (block.vtx[0].IsGenesis() && (block.vtx[0].nTime - walletTx.nTime) < (fTestNet ? Core::TRUST_KEY_TIMESPAN_TESTNET : Core::TRUST_KEY_TIMESPAN))
                continue;

            /* Can't spend transaction from after block time */
            if (walletTx.nTime > block.vtx[0].nTime)
                continue;

            for (unsigned int i = 0; i < walletTx.vout.size(); i++)
            {
                /* Can't spend outputs that are already spent or not belonging to this wallet */
                if (walletTx.IsSpent(i) || !IsMine(walletTx.vout[i]))
                    continue;

                /* Stop adding Inputs if has reached Maximum Transaction Size. */
                unsigned int nBytes = ::GetSerializeSize(block.vtx[0], SER_NETWORK, PROTOCOL_VERSION);
                if (nBytes >= Core::MAX_BLOCK_SIZE_GEN / 5)
                    break;

                block.vtx[0].vin.push_back(CTxIn(walletTx.GetHash(), i));
                vInputs.push_back(walletTx);

                /** Add the value to the first Output for Coinstake. **/
                block.vtx[0].vout[0].nValue += walletTx.vout[i].nValue;
            }
        }

        if(block.vtx[0].vin.size() == 1)
            return false; // No transactions added

        /* Calculate the Interest for the Coinstake Transaction. */
        int64 nInterest;
        LLD::CIndexDB indexdb("cr");
        if(!block.vtx[0].GetCoinstakeInterest(block, indexdb, nInterest))
            return error("AddCoinstakeInputs() : Failed to Get Interest");

        block.vtx[0].vout[0].nValue += nInterest;

        /* Sign Each Input to Transaction. */
        for(int nIndex = 0; nIndex < vInputs.size(); nIndex++)
        {
            if (!SignSignature(*this, vInputs[nIndex], block.vtx[0], nIndex + 1))
                return error("AddCoinstakeInputs() : Unable to sign Coinstake Transaction Input.");

        }

        return true;
    }


    /*
     *  Private load operations are accessible from CWalletDB via friend declaration.
     *  Everyone else uses corresponding set/add operation.
     */

    /* Load the minimum supported version without updating the database */
    bool LoadMinVersion(const int nVersion) 
    { 
        nWalletVersion = nVersion; 
        nWalletMaxVersion = std::max(nWalletMaxVersion, nVersion); 
        return true; 
    }

    
    /* Loads a master key into the wallet, identified by its key Id. */
    bool LoadMasterKey(const uint32_t nMasterKeyId, const CMasterKey& kMasterKey) 
    { 
        if (mapMasterKeys.count(nMasterKeyId) != 0)
            return false;

        mapMasterKeys[nMasterKeyId] = kMasterKey; 

        /* After load, wallet nMasterKeyMaxID will contain the maximum key ID currently stored in the database */
        if (nMasterKeyMaxID < nMasterKeyId)
            nMasterKeyMaxID = nMasterKeyId;

        return true; 
    }


    /* Load a public/encrypted private key pair to the key store without updating the database. */
    bool LoadCryptedKey(const std::vector<uint8_t> &vchPubKey, const std::vector<uint8_t> &vchCryptedSecret) 
    { 
        return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret); 
    }


    /* Load a key to the key store without updating the database. */
    bool LoadKey(const LLC::ECKey& key) 
    { 
        return CCryptoKeyStore::AddKey(key); 
    }

    
    /* Load a script to the key store without updating the database. */
    bool LoadCScript(const CScript& redeemScript) 
    { 
        return CCryptoKeyStore::AddCScript(redeemScript); 
    }


    /* Selects the unspent transaction outputs to use as inputs when creating a transaction that sends balance from this wallet. */
    bool CWallet::SelectCoins(const int64_t nTargetValue, const uint32_t nSpendTime, set<pair<const CWalletTx,uint32_t> >& setCoinsRet, int64_t& nValueRet) const
    {
        /* Call detailed select up to 3 times if it fails, using the returns from the first successful call.
         * This allows it to attempt multiple input sets if it doesn't find a workable one on the first try.
         * (example, it chooses an input set with total value exceeding maximum allowed value)
         */
        return (SelectCoinsMinConf(nTargetValue, nSpendTime, 3, 3, setCoinsRet, nValueRet) ||
                SelectCoinsMinConf(nTargetValue, nSpendTime, 3, 3, setCoinsRet, nValueRet) ||
                SelectCoinsMinConf(nTargetValue, nSpendTime, 3, 3, setCoinsRet, nValueRet));
    }


    /* Selects the unspent outputs to use as inputs when creating a transaction to send
     * balance from this wallet while requiring a minimum confirmation depth to be included in result.
     */
    bool CWallet::SelectCoinsMinConf(const int64_t nTargetValue, const uint32_t nSpendTime, const int nConfMine, const int nConfTheirs, 
                                     set<pair<const CWalletTx&,uint32_t> >& setCoinsRet, int64_t& nValueRet) const
    {
        /* Add Each Input to Transaction. */
        setCoinsRet.clear();
        vector<const CWalletTx&> vallWalletTx;

        nValueRet = 0;

        {
            std::lock_guard<std::mutex> walletLock(cs_wallet); 

            vallWalletTx.reserve(mapWallet.size());

            for (auto item : mapWallet)
                vallWalletTx.push_back(item.second);

        }

        random_shuffle(vallWalletTx.begin(), vallWalletTx.end(), GetRandInt);

        for(const CWalletTx& walletTx : vallWalletTx)
        {
            /* Can't spend balance that is unconfirmed or not final */
            if (!walletTx.IsFinal() || !walletTx.IsConfirmed())
                continue;

            /* Can't spend coinbase or coinstake transactions that are immature */
            if ((walletTx.IsCoinBase() || walletTx.IsCoinStake()) && walletTx.GetBlocksToMaturity() > 0)
                continue;

            /* Can't spend transaction that has not reached minimum depth setting for mine/theirs */
            int nDepth = walletTx.GetDepthInMainChain();
            if (nDepth < (walletTx.IsFromMe() ? nConfMine : nConfTheirs))
                continue;

            /* Can't spend transaction from after spend time */
            if (walletTx.nTime > nSpendTime)
                continue;

            for (uint32_t i = 0; i < walletTx.vout.size(); i++)
            {
                /* Can't spend outputs that are already spent or not belonging to this wallet */
                if (walletTx.IsSpent(i) || !IsMine(walletTx.vout[i]))
                    continue;

                /* Add reference to transaction with selected vout index to result set */
                setCoinsRet.insert(make_pair(walletTx, i));

                /* Accumulate total value available to spend in result set */
                nValueRet += walletTx.vout[i].nValue;
            }

            /* If value available to spend in result set exceeds target value, we are done */
            if(nValueRet >= nTargetValue)
                break;

        }

        /* Print result set when argument set */
        if (GetBoolArg("-printselectcoin", false))
        {
            printf("SelectCoins() selected: ");
            for(auto item : setCoinsRet)
                item.first->print();

            printf("total %s\n", FormatMoney(nValueRet).c_str());
        }

        /* Ensure input total value does not exceed maximum allowed */
        if(!Core::MoneyRange(nValueRet))
            return error("CWallet::SelectCoins() : Input total over TX limit Total: %" PRI64d " Limit %" PRI64d, nValueRet, Core::MaxTxOut());

        /* Ensure balance is sufficient to cover transaction */
        if(nValueRet < nTargetValue)
            return error("CWallet::SelectCoins() : Insufficient Balance Target: %" PRI64d " Actual %" PRI64d, nTargetValue, nValueRet);

        return true;
    }

}
