/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <openssl/rand.h>   // For RAND_bytes

#include <algorithm>
#include <thread>
#include <utility>

#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <LLD/include/legacy.h>
#include <LLD/include/global.h>

#include <LLP/include/version.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/tritium.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/money.h>
#include <Legacy/include/signature.h>
#include <Legacy/include/enum.h> // For GMF_SEND
#include <Legacy/types/legacy_minter.h>
#include <Legacy/types/script.h>

#include <Legacy/wallet/crypter.h>
#include <Legacy/wallet/output.h>
#include <Legacy/wallet/reservekey.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/signals.h>

namespace Legacy
{

    /* Nexus: Setting indicating wallet unlocked for block minting only */
    bool fWalletUnlockMintOnly = false;


    /* Initialize static variables */
    std::atomic<bool> Wallet::fWalletInitialized(false);


    /** Constructor **/
    Wallet::Wallet()
    : CryptoKeyStore()
    , nWalletVersion(FEATURE_BASE)
    , nWalletMaxVersion(FEATURE_BASE)
    , fFileBacked(false)
    , fLoaded(false)
    , strWalletFile("")
    , mapMasterKeys()
    , nMasterKeyMaxID(0)
    , addressBook(AddressBook(*this))
    , keyPool(KeyPool(*this))
    , vchDefaultKey()
    , vchTrustKey()
    , nWalletUnlockTime(0)
    , cs_wallet()
    , mapWallet()
    , mapRequestCount()
    {
    }


    /** Destructor **/
    Wallet::~Wallet()
    {
    }


    /* Implement static methods */

    /* Initializes the wallet instance. */
    bool Wallet::InitializeWallet(const std::string& strWalletFileIn)
    {
        if(Wallet::fWalletInitialized.load())
            return false;
        else
        {
            Wallet& wallet = Wallet::GetInstance();
            wallet.strWalletFile = strWalletFileIn;
            wallet.fFileBacked = true;

            WalletDB walletdb(strWalletFileIn);
            walletdb.InitializeDatabase();
        }

        return true;
    }


    Wallet& Wallet::GetInstance()
    {
        /* This will create a default initialized, memory only wallet file on first call (lazy instantiation) */
        static Wallet wallet;

        if(!Wallet::fWalletInitialized.load())
            Wallet::fWalletInitialized.store(true);

        return wallet;
    }


    /* Assign the minimum supported version for the wallet. */
    bool Wallet::SetMinVersion(const enum Legacy::WalletFeature nVersion, const bool fForceLatest)
    {
        {
            LOCK(cs_wallet);

            /* Allows potential to override nVersion with FEATURE_LATEST */
            Legacy::WalletFeature nVersionToSet = nVersion;

            /* When force, if we pass a value greater than the max version currently supported, upgrade all the way to latest */
            if(fForceLatest && nVersionToSet > nWalletMaxVersion)
                    nVersionToSet = FEATURE_LATEST;

            /* Ignore new setting if current setting is higher version
             * Will still process if they are equal, because nWalletVersion defaults to FEATURE_BASE and it needs to call WriteMinVersin for new wallet */
            if(nWalletVersion > nVersionToSet)
                return true;

            nWalletVersion = nVersionToSet;

            /* If new min version exceeds old max version, update the max version */
            if(nVersionToSet > nWalletMaxVersion)
                nWalletMaxVersion = nVersionToSet;

            if(fFileBacked)
            {
                /* Store new version to database (overwrites old) */
                WalletDB walletdb(strWalletFile);
                walletdb.WriteMinVersion(nWalletVersion);
            }
        }

        return true;
    }


    /* Assign the maximum version we're allowed to upgrade to.  */
    bool Wallet::SetMaxVersion(const enum Legacy::WalletFeature nVersion)
    {
        {
            LOCK(cs_wallet);

            /* Cannot downgrade below current version */
            if(nWalletVersion > nVersion)
                return false;

            nWalletMaxVersion = nVersion;
        }

        return true;
    }


    /* Loads all data for a file backed wallet from the database. */
    uint32_t Wallet::LoadWallet(bool& fFirstRunRet)
    {
        if(!fFileBacked)
            return false;

        /* If wallet was already loaded, just return */
        if(fLoaded)
            return DB_LOAD_OK;

        fFirstRunRet = false;

        WalletDB walletdb(strWalletFile);

        {
            /* Lock wallet so WalletDB can load all data into it */
            //LOCK(cs_wallet);

            uint32_t nLoadWalletRet = walletdb.LoadWallet(*this);

            if(nLoadWalletRet != DB_LOAD_OK)
                return nLoadWalletRet;
        }

        /* New wallet is indicated by an empty default key */
        fFirstRunRet = vchDefaultKey.empty();

        /* On first run, assign min/max version, generate key pool, and generate a default key for this wallet */
        if(fFirstRunRet && !IsLocked())
        {
            /* For a newly created wallet, set the min and max version to the latest */
            debug::log(2, FUNCTION, "Setting wallet min version to ", FEATURE_LATEST);

            SetMinVersion(FEATURE_LATEST);

            std::vector<uint8_t> vchNewDefaultKey;

            /* For a new wallet, may need to generate initial key pool */
            if(keyPool.GetKeyPoolSize() == 0)
                keyPool.NewKeyPool();

            if(keyPool.GetKeyPoolSize() > 0)
            {
                debug::log(2, FUNCTION, "Adding wallet default key");

                if(!keyPool.GetKeyFromPool(vchNewDefaultKey, false))
                {
                    debug::error(FUNCTION, "Error adding wallet default key. Cannot get key from key pool.");
                    return DB_LOAD_FAIL;
                }

                SetDefaultKey(vchNewDefaultKey);

                if(!addressBook.SetAddressBookName(NexusAddress(vchDefaultKey), "default"))
                {
                    debug::error(FUNCTION, "Error adding wallet default key. Unable to add key to address book.");
                    return DB_LOAD_FAIL;
                }
            }
        }
        else if(nWalletVersion == FEATURE_BASE)
        {
            /* Old wallets set min version but it never got recorded because constructor defaulted the value.
             * This assures older wallet files have it stored.
             */
            uint32_t nStoredMinVersion = 0;

            if(!walletdb.ReadMinVersion(nStoredMinVersion) || nStoredMinVersion == 0)
                SetMinVersion(FEATURE_BASE);
        }

        /* Max allowed version is always the current latest */
        SetMaxVersion(FEATURE_LATEST);

        /* Launch background thread to periodically flush the wallet to the backing database */
        WalletDB::StartFlushThread(strWalletFile);

        fLoaded = true;

        return DB_LOAD_OK;
    }


    /*  Tracks requests for transactions contained in this wallet, or the blocks that contain them. */
    void Wallet::Inventory(const uint1024_t& hash)
    {
        {
            LOCK(cs_wallet);

            auto mi = mapRequestCount.find(hash);

            if(mi != mapRequestCount.end())
            {
                /* Map contains an entry for the given block hash (we have one or more transactions in that block) */
                (*mi).second++;
            }
        }
    }


    /* Add a public/encrypted private key pair to the key store. */
    bool Wallet::AddCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret)
    {
        /* Call overridden inherited method to add key to key store */
        if(!CryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
            return false;

        if(fFileBacked)
        {
            bool result = false;

            WalletDB walletdb(strWalletFile);
            result = walletdb.WriteCryptedKey(vchPubKey, vchCryptedSecret);

            return result;
        }

        return true;
    }


    /* Add a key to the key store. */
    bool Wallet::AddKey(const LLC::ECKey& key)
    {
        /*
         * This works in a convoluted manner for encrypted wallets.
         *   1. This method calls CryptoKeyStore::AddKey()
         *   2. If wallet is not encrypted, that method adds key to key store, but not to database (as expected)
         *   3. If wallet is encrypted (and unlocked), CryptoKeyStore::AddKey encrypts the key and calls AddCryptedKey()
         *   4. Because this is a Wallet instance, that call will actually call Wallet::AddCryptedKey() and
         *      not the more obvious CryptoKeyStore::AddCryptedKey()
         *
         * In other words, the call to AddCryptedKey() within CryptoKeyStore::AddKey is actually performing a
         * polymorphic call to this->AddCryptedKey() to execute the method in the derived Wallet class.
         *
         * The Wallet version of AddCryptedKey() handles adding the encrypted key to both key store and database.
         * The result: only need to write to database here if wallet is not encrypted
         *
         * Would be better to have a more intuitive way for code to handle encrypted key, but this way does work.
         * It violates encapsulation, though, because we should not have to rely on how CryptoKeyStore implements AddKey
         */


        /* Call overridden method to add key to key store */
        /* For encrypted wallet, this adds to both key store and wallet database (as described above) */
        if(!CryptoKeyStore::AddKey(key))
            return false;

        if(fFileBacked && !IsCrypted())
        {
            /* Only if wallet is not encrypted */
            WalletDB walletdb(strWalletFile);
            bool result = walletdb.WriteKey(key.GetPubKey(), key.GetPrivKey());

            return result;
        }


        return true;
    }


    /* Add a script to the key store.  */
    bool Wallet::AddScript(const Script& redeemScript)
    {
        {
            LOCK(cs_wallet);

            /* Call overridden inherited method to add key to key store */
            if(!CryptoKeyStore::AddScript(redeemScript))
                return false;

            if(fFileBacked)
            {
                WalletDB walletdb(strWalletFile);
                bool result = walletdb.WriteScript(LLC::SK256(redeemScript), redeemScript);

                return result;
            }
        }

        return true;
    }


    /* Generates a new key and adds it to the key store. */
    std::vector<uint8_t> Wallet::GenerateNewKey()
    {
        bool fCompressed = true;

        LLC::RandAddSeedPerfmon();
        LLC::ECKey key;
        key.MakeNewKey(fCompressed);

        /* AddKey adds to key store, encrypting it first if wallet is encrypted, and writes key to database if file backed */
        /* AddKey also performs wallet locking, so no lock guard needed here */
        if(!AddKey(key))
            throw std::runtime_error("Wallet::GenerateNewKey : AddKey failed");

        return key.GetPubKey();
    }


    /* Assigns a new default key to this wallet. */
    bool Wallet::SetDefaultKey(const std::vector<uint8_t>& vchPubKey)
    {
        {
            LOCK(cs_wallet);

            if(fFileBacked)
            {
                WalletDB walletdb(strWalletFile);
                bool result = walletdb.WriteDefaultKey(vchPubKey);

                if(!result)
                    return false;
            }

            vchDefaultKey = vchPubKey;
        }

        return true;
    }


    /* Stores the public key for the wallet's trust key. */
    bool Wallet::SetTrustKey(const std::vector<uint8_t>& vchPubKey)
    {
        {
            LOCK(cs_wallet);

            if(fFileBacked)
            {
                WalletDB walletdb(strWalletFile);
                bool result = walletdb.WriteTrustKey(vchPubKey);

                if(!result)
                    return false;
            }

            vchTrustKey = vchPubKey;
        }

        return true;
    }


    /* Removes a wallet transaction from the wallet, if present. */
    bool Wallet::RemoveTrustKey()
    {
        {
            LOCK(cs_wallet);

            if(fFileBacked)
            {
                WalletDB walletdb(strWalletFile);
                bool result = walletdb.EraseTrustKey();

                if(!result)
                    return false;
            }

            vchTrustKey.clear();
        }

        return true;
    }


    /* Encrypts the wallet in both memory and file backing, assigning a passphrase that will be required
     * to unlock and access the wallet.
     */
    bool Wallet::EncryptWallet(const SecureString& strWalletPassphrase)
    {
        if(IsCrypted())
            return false;

        Crypter crypter;
        MasterKey kMasterKey;
        uint32_t nNewMasterKeyId = 0;

        CKeyingMaterial vMasterKey;
        LLC::RandAddSeedPerfmon();

        /* If it is running, stop stake minter before encrypting wallet */
        LegacyMinter::GetInstance().StopStakeMinter();

        /* Fill keying material (unencrypted key value) and new master key salt with random data using OpenSSL RAND_bytes */
        vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
        RAND_bytes(&vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

        LLC::RandAddSeedPerfmon();
        kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
        RAND_bytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

        /* Use time to process 2 calls to SetKeyFromPassphrase to create a nDeriveIterations value for master key */
        int64_t nStartTime = runtime::timestamp(true);
        crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
        kMasterKey.nDeriveIterations = 2500000 / ((double)(runtime::timestamp(true) - nStartTime));

        nStartTime = runtime::timestamp(true);
        crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
        kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + kMasterKey.nDeriveIterations * 100 / ((double)(runtime::timestamp(true) - nStartTime))) / 2;

        /* Assure a minimum value */
        if(kMasterKey.nDeriveIterations < 25000)
            kMasterKey.nDeriveIterations = 25000;

        debug::log(0, FUNCTION, "Encrypting Wallet with nDeriveIterations of ", kMasterKey.nDeriveIterations);

        /* Encrypt the master key value using the new passphrase */
        if(!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
            return false;

        if(!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey))
            return false;

        /* kMasterKey now contains the master key encrypted by the provided passphrase. Ready to perform wallet encryption. */
        {
            /* Lock for writing master key and converting keys */
            LOCK(cs_wallet);

            nNewMasterKeyId = ++nMasterKeyMaxID;
            mapMasterKeys[nNewMasterKeyId] = kMasterKey;


            /* EncryptKeys() in CryptoKeyStore will encrypt every public key/private key pair in the BasicKeyStore, including those in the
             * key pool. Then it will clear the BasickeyStore, write the encrypted keys into CryptoKeyStore, and return them in mapNewEncryptedKeys.
             */
            CryptedKeyMap mapNewEncryptedKeys;
            if(!EncryptKeys(vMasterKey, mapNewEncryptedKeys))
            {
                /* The encryption failed, but we have not updated the key store or the database, yet, so just return false */
                return debug::error(FUNCTION, "Error encrypting wallet. Encryption aborted.");;
            }

            /* Update the backing database. This will store the master key and encrypted keys, and remove old unencrypted keys */
            if(fFileBacked)
            {
                WalletDB walletdb(strWalletFile);
                bool fDbEncryptionSuccessful = false;

                fDbEncryptionSuccessful = walletdb.EncryptDatabase(nNewMasterKeyId, kMasterKey, mapNewEncryptedKeys);

                if(!fDbEncryptionSuccessful)
                {
                    /* Keys encrypted in memory, but not on disk...die to let the user reload their unencrypted wallet. */
                    config::fShutdown.store(true);
                    SHUTDOWN.notify_all();
                    return debug::error(FUNCTION, "Unable to complete encryption for ", strWalletFile, ". Encryption aborted. Shutting down.");
                }
            }
        }

        /* Lock wallet, then unlock with new passphrase to update key pool */
        Lock();
        Unlock(strWalletPassphrase);

        /* Replace key pool with encrypted keys */
        {
            LOCK(cs_wallet);

            keyPool.NewKeyPool();
        }

        /* Lock wallet again before rewrite */
        Lock();

        /* Need to completely rewrite the wallet file; if we don't, bdb might keep
         * bits of the unencrypted private key in slack space in the database file.
         */
        bool rewriteResult = true;
        {
            LOCK(cs_wallet);

            if(fFileBacked)
            {
                WalletDB walletdb(strWalletFile);
                rewriteResult = walletdb.DBRewrite();
            }
        }

        if(rewriteResult)
            debug::log(0, FUNCTION, "Wallet encryption completed successfully");

        return rewriteResult;
    }


    /*  Attempt to lock an encrypted wallet. */
    bool Wallet::Lock()
    {
        /* Cannot lock unencrypted key store. This will enable encryption if not enabled already. */
        if(IsCrypted() && CryptoKeyStore::Lock())
        {
            /* Upon successful lock, stop the stake minter if it is running */
            LegacyMinter::GetInstance().StopStakeMinter();

            return true;
        }

        return false;
    }


    /* Attempt to unlock an encrypted wallet using the passphrase provided. */
    bool Wallet::Unlock(const SecureString& strWalletPassphrase, const uint32_t nUnlockSeconds)
    {
        if(!IsLocked())
            return false;

        Crypter crypter;
        CKeyingMaterial vMasterKey;

        {
            LOCK(cs_wallet);

            /* If more than one master key in wallet's map (unusual), this will attempt each one with the passphrase.
             * If any one master key decryption works and unlocks the wallet, then the unlock is successful.
             * Supports a multi-user wallet, where each user has their own passphrase
             */
            for(const auto& pMasterKey : mapMasterKeys)
            {
                /* Set the encryption context using the passphrase provided */
                if(!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;

                /* Attempt to decrypt the master key using the passphrase crypter */
                if(!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                    return false;

                /* Attempt to unlock the wallet using the decrypted value for the master key */
                if(CryptoKeyStore::Unlock(vMasterKey))
                {
                    /* If the caller has provided an nUnlockSeconds value then initiate a thread to lock
                     * the wallet once this time has expired.  NOTE: the fWalletUnlockMintOnly flag overrides this timeout
                     * as we unlock indefinitely if fWalletUnlockMintOnly is true */
                    if(nUnlockSeconds > 0 && !Legacy::fWalletUnlockMintOnly)
                    {
                        nWalletUnlockTime = runtime::timestamp() + nUnlockSeconds;

                        /* use C++ lambda to creating a threaded callback to lock the wallet */
                        std::thread([=]()
                        {
                            // check the time every second until the unlock time is surpassed or the wallet is manually locked
                            while(runtime::timestamp() < nWalletUnlockTime && !this->IsLocked())
                                std::this_thread::sleep_for(std::chrono::seconds(1));

                            Lock();
                            nWalletUnlockTime = 0;
                        }).detach();

                    }

                    /* Whether unlocked fully or for minting only, start the stake minter if configured */
                    LegacyMinter::GetInstance().StartStakeMinter();

                    return true;
                }
            }
        }

        return false;
    }


    /* Replaces the existing wallet passphrase with a new one. */
    bool Wallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
    {
        /* To change the passphrase, the old passphrase must successfully decrypt the master key and unlock
         * the wallet, thus proving it is the correct one. Then, the master key is re-encrypted using the
         * new passphrase and saved.
         */

        {
            LOCK(cs_wallet);

            /* Save current lock state so it can be reset when done */
            bool fWasLocked = IsLocked();

            /* Lock the wallet so we can use unlock to verify old passphrase */
            Lock();

            Crypter crypter;
            CKeyingMaterial vMasterKey;

            /* If more than one master key in wallet's map (unusual), have to find the one that corresponds to old passphrase.
             * Do this by attempting to use each to unlock with old passphrase until find a match.
             */
            for(auto& pMasterKey : mapMasterKeys)
            {
                /* Attempt to decrypt the current master key and unlock the wallet with it */
                if(!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;

                if(!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                    return false;

                if(CryptoKeyStore::Unlock(vMasterKey))
                {
                    /* Successfully unlocked, so pMasterKey is the map entry that corresponds to the old passphrase
                     * Now change that passphrase by re-encrypting master key with new one.
                     */

                    /* Use time to process 2 calls to SetKeyFromPassphrase to create a new nDeriveIterations value for master key */
                    int64_t nStartTime = runtime::timestamp(true);
                    crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                    pMasterKey.second.nDeriveIterations = pMasterKey.second.nDeriveIterations * (100 / ((double)(runtime::timestamp(true) - nStartTime)));

                    nStartTime = runtime::timestamp(true);
                    crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                    pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + pMasterKey.second.nDeriveIterations * 100 / ((double)(runtime::timestamp(true) - nStartTime))) / 2;

                    /* Assure a minimum value */
                    if(pMasterKey.second.nDeriveIterations < 25000)
                        pMasterKey.second.nDeriveIterations = 25000;

                    debug::log(0, FUNCTION, "Wallet passphrase changed to use nDeriveIterations of ", pMasterKey.second.nDeriveIterations);

                    /* Re-encrypt the master key using the new passphrase */
                    if(!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                        return false;

                    if(!crypter.Encrypt(vMasterKey, pMasterKey.second.vchCryptedKey))
                        return false;

                    if(fFileBacked)
                    {
                        /* Store new master key encryption to the wallet database (overwrites old value) */
                        WalletDB walletdb(strWalletFile);
                        walletdb.WriteMasterKey(pMasterKey.first, pMasterKey.second);
                    }

                    /* Relock file if it was locked when we started */
                    if(fWasLocked)
                        Lock();

                    return true;
                }
            }
        }

        return false;
    }


    /* Retrieves the total wallet balance for all confirmed, mature transactions. */
    int64_t Wallet::GetBalance()
    {
        int64_t nTotalBalance = 0;

        TransactionMap mapWalletCopy;

        {
            /* Lock wallet only to take a snapshot of current transaction map for calculating balance
             * After unlock, mapWallet can change but it won't affect balance calculation
             */
            LOCK(cs_wallet);

            mapWalletCopy = mapWallet;
        }

        for(const auto& item : mapWalletCopy)
        {
            const WalletTx& walletTx = item.second;

            /* Skip any transaction that isn't final, isn't completely confirmed, or has a future timestamp */
            if(!walletTx.IsFinal() || !walletTx.IsConfirmed() || walletTx.nTime > runtime::unifiedtimestamp())
                continue;

            nTotalBalance += walletTx.GetAvailableCredit();
        }

        return nTotalBalance;
    }



    /* Get the available addresses that have a balance associated with a wallet. */
    bool Wallet::BalanceByAccount(std::string strAccount, int64_t& nBalance, int32_t nMinDepth)
    {
        {
            LOCK(cs_wallet);
            nBalance = 0;
            for(auto it = mapWallet.begin(); it != mapWallet.end(); ++it)
            {
                const WalletTx* pcoin = &(*it).second;
                if(!pcoin->IsFinal())
                    continue;

                if(pcoin->GetDepthInMainChain() < nMinDepth)
                    continue;

                if((pcoin->IsCoinBase() || pcoin->IsCoinStake()) && pcoin->GetBlocksToMaturity() > 0)
                    continue;

                for(int i = 0; i < pcoin->vout.size(); i++)
                {

                    if(!pcoin->IsSpent(i) && IsMine(pcoin->vout[i]) && pcoin->vout[i].nValue > 0)
                    {
                        if(strAccount == "*")
                        {
                            nBalance += pcoin->vout[i].nValue;

                            continue;
                        }

                        NexusAddress address;
                        if(!ExtractAddress(pcoin->vout[i].scriptPubKey, address) || !address.IsValid())
                            return false;

                        if(GetAddressBook().GetAddressBookMap().count(address))
                        {
                            std::string strEntry = GetAddressBook().GetAddressBookMap().at(address);
                            if(strEntry == "" && strAccount == "default")
                                strEntry = "default";

                            if(strEntry == "default" && strAccount == "")
                                strAccount = "default";

                            if(strEntry == strAccount)
                                nBalance += pcoin->vout[i].nValue;
                        }
                        else if(strAccount == "default" || strAccount == "")
                            nBalance += pcoin->vout[i].nValue;

                    }
                }
            }
        }

        return true;
    }


    /* Retrieves the current wallet balance for unconfirmed transactions. */
    int64_t Wallet::GetUnconfirmedBalance()
    {
        int64_t nUnconfirmedBalance = 0;

        TransactionMap mapWalletCopy;

        {
            /* Lock wallet only to take a snapshot of current transaction map for calculating balance
             * After unlock, mapWallet can change but it won't affect balance calculation
             */
            LOCK(cs_wallet);

            mapWalletCopy = mapWallet;
        }

        for(const auto& item : mapWalletCopy)
        {
            const WalletTx& walletTx = item.second;

            if(walletTx.IsFinal() && walletTx.IsConfirmed())
                continue;

            nUnconfirmedBalance += walletTx.GetAvailableCredit();
        }

        return nUnconfirmedBalance;
    }


    /* Retrieves the current immature stake balance. */
    int64_t Wallet::GetStake()
    {
        int64_t nTotalStake = 0;

        {
            LOCK(cs_wallet);

            for(const auto& item : mapWallet)
            {
                const WalletTx& walletTx = item.second;

                if(walletTx.IsCoinStake() && walletTx.GetBlocksToMaturity() > 0 && walletTx.GetDepthInMainChain() > 1)
                    nTotalStake += GetCredit(walletTx);
            }
        }

        return nTotalStake;
    }


    /* Retrieves the current immature minted (mined) balance. */
    int64_t Wallet::GetNewMint()
    {
        int64_t nTotalMint = 0;

        {
            LOCK(cs_wallet);

            for(const auto& item : mapWallet)
            {
                const WalletTx& walletTx = item.second;

                if(walletTx.IsCoinBase() && walletTx.GetBlocksToMaturity() > 0 && walletTx.GetDepthInMainChain() > 1)
                    nTotalMint += GetCredit(walletTx);
            }
        }

        return nTotalMint;
    }


    /* Populate vCoins with vector identifying spendable outputs. */
    void Wallet::AvailableCoins(const uint32_t nSpendTime, std::vector<Output>& vCoins, const bool fOnlyConfirmed)
    {
        {
            LOCK(cs_wallet);

            vCoins.clear();

            for(const auto& item : mapWallet)
            {
                const WalletTx& walletTx = item.second;

                /* Filter transactions not final */
                if(!walletTx.IsFinal())
                    continue;

                /* Filter unconfirmed transactions unless want unconfirmed */
                if(fOnlyConfirmed && !walletTx.IsConfirmed())
                    continue;

                /* Filter immature minting and staking transactions */
                if((walletTx.IsCoinBase() || walletTx.IsCoinStake()) && walletTx.GetBlocksToMaturity() > 0)
                    continue;

                for(uint32_t i = 0; i < walletTx.vout.size(); i++)
                {
                    /* Filter transactions after requested spend time */
                    if(walletTx.nTime > nSpendTime)
                        continue;

                    /* To be included in result, vout must not be spent, must belong to current wallet, and must have positive value */
                    if(!(walletTx.IsSpent(i)) && IsMine(walletTx.vout[i]) && walletTx.vout[i].nValue > 0)
                    {
                        /* Create output from the current vout and add to result */
                        Output txOutput(walletTx, i, walletTx.GetDepthInMainChain());
                        vCoins.push_back(txOutput);
                    }
                }
            }
        }
    }


    /* Mark all transactions in the wallet as "dirty" to force balance recalculation. */
    void Wallet::MarkDirty()
    {
        {
            LOCK(cs_wallet);

            for(auto& item : mapWallet)
                item.second.MarkDirty();
        }
    }


    /*  Retrieves the transaction for a given transaction hash. */
    bool Wallet::GetTransaction(const uint512_t& hashTx, WalletTx& wtx)
    {
        {
            LOCK(cs_wallet);

            /* Find the requested transaction in the wallet */
            TransactionMap::iterator mi = mapWallet.find(hashTx);

            if(mi != mapWallet.end())
            {
                wtx = (*mi).second;
                return true;
            }
        }

        return false;
    }


    int32_t Wallet::GetRequestCount(const WalletTx& wtx) const
    {
        /* Returns -1 if it wasn't being tracked */
        int32_t nRequests = -1;

        if(wtx.IsCoinBase() || wtx.IsCoinStake())
        {
            LOCK(cs_wallet);

            /* If this is a block generated via minting, use request count entry for the block */
            /* Blocks created via staking/mining have block hash added to request tracking */
            if(wtx.hashBlock != 0)
            {
                auto mi = mapRequestCount.find(wtx.hashBlock);

                if(mi != mapRequestCount.end())
                    nRequests = (*mi).second;
            }
        }
        else
        {
            LOCK(cs_wallet);

            /* Did anyone request this transaction? */
            auto mi = mapRequestCount.find(wtx.GetHash());
            if(mi != mapRequestCount.end())
            {
                nRequests = (*mi).second;

                /* If no request specifically for the transaction hash, check the count for the block containing it */
                if(nRequests == 0 && wtx.hashBlock != 0)
                {
                    auto mi = mapRequestCount.find(wtx.hashBlock);
                    if(mi != mapRequestCount.end())
                        nRequests = (*mi).second;
                }
            }
        }

        return nRequests;
    }


    /* Adds a wallet transaction to the wallet. */
    bool Wallet::AddToWallet(const WalletTx& wtxIn)
    {
        uint512_t hash = wtxIn.GetHash();

        /* Use the returned tx, not wtxIn, in case insert returned an existing transaction */
        std::pair<TransactionMap::iterator, bool> ret;
        {
            LOCK(cs_wallet);

            /* Inserts only if not already there, returns tx inserted or tx found */
            ret = mapWallet.insert(std::make_pair(hash, wtxIn));
        }

        WalletTx& wtx = (*ret.first).second;
        bool fInsertedNew = ret.second;
        wtx.BindWallet(this);

        if(fInsertedNew)
        {
            /* wtx.nTimeReceive must remain uint32_t for backward compatability */
            wtx.nTimeReceived = (uint32_t)runtime::unifiedtimestamp();
        }

        bool fUpdated = false;
        if(!fInsertedNew)
        {
            /* If found an existing transaction, merge the new one into it */
            if(wtxIn.hashBlock != 0 && wtxIn.hashBlock != wtx.hashBlock)
            {
                wtx.hashBlock = wtxIn.hashBlock;
                fUpdated = true;
            }

            /* nIndex and vMerkleBranch are deprecated so nIndex will be -1 for all legacy transactions created in Tritium
             * Code here is only relevant for processing old transactions previously stored in wallet.dat
             */
            if(wtxIn.nIndex != -1 && (wtxIn.vMerkleBranch != wtx.vMerkleBranch || wtxIn.nIndex != wtx.nIndex))
            {
                wtx.vMerkleBranch = wtxIn.vMerkleBranch;
                wtx.nIndex = wtxIn.nIndex;
                fUpdated = true;
            }

            if(wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
            {
                wtx.fFromMe = wtxIn.fFromMe;
                fUpdated = true;
            }

            /* Merge spent flags */
            fUpdated |= wtx.UpdateSpent(wtxIn.vfSpent);
        }

        /* debug print */
        debug::log(0, FUNCTION, wtxIn.GetHash().ToString().substr(0,10), " ", (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

        /* Write to disk */
        if(fInsertedNew || fUpdated)
            if(!wtx.WriteToDisk())
                return false;

        /* If default receiving address gets used, replace it with a new one */
        Script scriptDefaultKey;
        scriptDefaultKey.SetNexusAddress(vchDefaultKey);

        for(const TxOut& txout : wtx.vout)
        {
            if(txout.scriptPubKey == scriptDefaultKey)
            {
                std::vector<uint8_t> newDefaultKey;

                if(keyPool.GetKeyFromPool(newDefaultKey, false))
                {
                    SetDefaultKey(newDefaultKey);
                    addressBook.SetAddressBookName(NexusAddress(vchDefaultKey), "");
                }
            }
        }

        /* since AddToWallet is called directly for self-originating transactions, check for consumption of own coins */
        WalletUpdateSpent(wtx);

        return true;
    }


    /*  Checks whether a transaction has inputs or outputs belonging to this wallet, and adds
     *  it to the wallet when it does.
     */
    bool Wallet::AddToWalletIfInvolvingMe(const Transaction& tx, const TAO::Ledger::BlockState& containingBlock,
                                           bool fUpdate, bool fFindBlock, bool fRescan)
    {
        uint512_t hash = tx.GetHash();

        /* Check to see if transaction hash in this wallet */
        bool fExisted = mapWallet.count(hash);

        /* When transaction already in wallet, return unless update is specifically requested */
        if(fExisted && !fUpdate)
            return false;

        /* Check if transaction has outputs (IsMine) or inputs (IsFromMe) belonging to this wallet */
        if(IsMine(tx) || IsFromMe(tx))
        {
            WalletTx wtx(this, tx);
            if(fRescan) {
                /* On rescan or initial download, set wtx time to transaction time instead of time tx received.
                 * These are both uint32_t timestamps to support unserialization of legacy data.
                 */
                wtx.nTimeReceived = tx.nTime;
            }

            if(!containingBlock.IsNull())
                wtx.hashBlock = containingBlock.GetHash();

            /* AddToWallet preforms merge (update) for transactions already in wallet */
            return AddToWallet(wtx);
        }
        else
            WalletUpdateSpent(tx);

        return false;
    }


    /* Removes a wallet transaction from the wallet, if present. */
    bool Wallet::EraseFromWallet(const uint512_t& hash)
    {
        if(!fFileBacked)
            return false;
        {
            LOCK(cs_wallet);

            if(mapWallet.erase(hash))
            {
                WalletDB walletdb(strWalletFile);
                walletdb.EraseTx(hash);
            }
        }

        return true;
    }


    /* When disconnecting a coinstake transaction, this method marks
     * any previous outputs from this wallet as unspent.
     */
    void Wallet::DisableTransaction(const Transaction& tx)
    {
        /* If transaction is not coinstake or not from this wallet, nothing to process */
        if(!tx.IsCoinStake() || !IsFromMe(tx))
            return;

        {
            /* Disconnecting coinstake requires marking input unspent */
            LOCK(cs_wallet);

            for(const TxIn& txin : tx.vin)
            {
                /* Find the previous transaction and mark as unspent the output that corresponds to current txin */
                TransactionMap::iterator mi = mapWallet.find(txin.prevout.hash);

                if(mi != mapWallet.end())
                {
                    WalletTx& prevTx = (*mi).second;

                    if(txin.prevout.n < prevTx.vout.size() && IsMine(prevTx.vout[txin.prevout.n]))
                    {
                        prevTx.MarkUnspent(txin.prevout.n);
                        prevTx.WriteToDisk();
                    }
                }
            }
        }
    }


    /* Scan the block chain for transactions from or to keys in this wallet.
     * Add/update the current wallet transactions for any found.
     */
    uint32_t Wallet::ScanForWalletTransactions(const TAO::Ledger::BlockState* pstartBlock, const bool fUpdate)
    {
        /* Count the number of transactions process for this wallet to use as return value */
        uint32_t nTransactionCount = 0;
        uint32_t nScannedCount     = 0;
        uint32_t nScannedBlocks    = 0;
        TAO::Ledger::BlockState block;
        if(pstartBlock == nullptr)
            block = TAO::Ledger::ChainState::stateGenesis;
        else
            block = *pstartBlock;

        runtime::timer timer;
        timer.Start();
        Legacy::Transaction tx;
        while(!config::fShutdown.load())
        {
            /* Output for the debugger. */
            ++nScannedBlocks;

            /* Meter to know the progress. */
            if(nScannedBlocks % 10000 == 0)
                debug::log(0, FUNCTION, nScannedBlocks, " blocks processed");

            /* Scan each transaction in the block and process those related to this wallet */
            for(const auto& item : block.vtx)
            {
                if(item.first == TAO::Ledger::LEGACY_TX)
                {
                    /* Read transaction from database */
                    if(!LLD::Legacy->ReadTx(item.second, tx))
                        continue;

                    /* Add to the wallet */
                    if(AddToWalletIfInvolvingMe(tx, block, fUpdate, false, true))
                        ++nTransactionCount;

                    /* Update the scanned count for meters. */
                    ++nScannedCount;
                }
            }

            /* Move to next block. Will return false when reach end of chain, ending the while loop */
            block = block.Next();
            if(!block)
                break;
        }

        int32_t nElapsedSeconds = timer.Elapsed();
        debug::log(0, FUNCTION, "Scanned ", nTransactionCount,
            " transactions, ", nScannedBlocks, " blocks  in ", nElapsedSeconds, " seconds (", std::fixed, nScannedCount > 0 ? (double)(nScannedCount / nElapsedSeconds > 0 ? nElapsedSeconds : 1) : 1, " tx/s)");

        return nTransactionCount;
    }


    /* Looks through wallet for transactions that should already have been added to a block, but are
     * still pending, and re-broadcasts them to then network.
     */
    void Wallet::ResendWalletTransactions()
    {
        /* Do this infrequently and randomly to avoid giving away that these are our transactions.
         *
         * Uses static snNextTime for this purpose. On first call, sets a random value (up to 30 minutes)
         * and returns. Any subsequent calls will only process resend if at least that much time
         * has passed.
         */
        static std::atomic<int64_t> snNextTime;

        /* Also keep track of best height on last resend, because no need to process again if has not changed */
        static std::atomic<int32_t> snLastHeight;


        bool fFirst = (snNextTime == 0);

        /* Always false on first iteration */
        if(runtime::unifiedtimestamp() < snNextTime.load())
            return;

        /* Set a random time until resend is processed */
        snNextTime = runtime::unifiedtimestamp() + LLC::GetRand(30 * 5);

        /* On first iteration, just return. All it does is set snNextTime */
        if(fFirst)
            return;

        /* If no new block, nothing has changed, so just return. */
        if(TAO::Ledger::ChainState::nBestHeight.load() <= snLastHeight.load())
            return;

        /* Record that it is processing resend now */
        snLastHeight = TAO::Ledger::ChainState::nBestHeight.load();
        {
            LOCK(cs_wallet);

            /* Find any sent tx not in block and sort them in chronological order */
            std::multimap<uint64_t, WalletTx> mapSorted;
            for(const auto& item : mapWallet)
            {
                const WalletTx& wtx = item.second;

                /* Don't put in sorted map for rebroadcast until it's had enough time to be added to a block */
                if(runtime::timestamp() - wtx.nTimeReceived > 1 * 60)
                    mapSorted.insert(std::make_pair(wtx.nTimeReceived, wtx));
            }

            for(const auto& item : mapSorted)
            {
                const WalletTx& wtx = item.second;

                /* Validate the transaction, then process rebroadcast on it */
                if(wtx.CheckTransaction())
                    wtx.RelayWalletTransaction();
                else
                    debug::log(0, FUNCTION, "CheckTransaction failed for transaction ", wtx.GetHash().ToString());
            }
        }
    }


    /* Checks a transaction to see if any of its inputs match outputs from wallet transactions
     * in this wallet. For any it finds, verifies that the outputs are marked as spent, updating
     * them as needed.
     */
    void Wallet::WalletUpdateSpent(const Transaction &tx)
    {
        {
            LOCK(cs_wallet);

            /* Loop through and the tx inputs, checking each separately */
            for(const auto& txin : tx.vin)
            {
                /* Check the txin to see if prevout hash maps to a transaction in this wallet */
                TransactionMap::iterator mi = mapWallet.find(txin.prevout.hash);

                if(mi != mapWallet.end())
                {
                    /* When there is a match to the prevout hash, get the previous wallet transaction */
                    WalletTx& prevTx = (*mi).second;

                    /* Outputs in wallet tx will have same index recorded in transaction txin
                     * Check any that are not flagged spent for belonging to this wallet and mark them as spent
                     */
                    if(!prevTx.IsSpent(txin.prevout.n) && IsMine(prevTx.vout[txin.prevout.n]))
                    {
                        debug::log(2, FUNCTION, "Found spent coin ", FormatMoney(prevTx.GetCredit()), " NXS ", prevTx.GetHash().ToString().substr(0, 20));

                        prevTx.MarkSpent(txin.prevout.n);
                        prevTx.WriteToDisk();
                    }
                }
            }
        }
    }


    /*  Identifies and fixes mismatches of spent coins between the wallet and the index db.  */
    void Wallet::FixSpentCoins(uint32_t& nMismatchFound, int64_t& nBalanceInQuestion, const bool fCheckOnly)
    {
        nMismatchFound = 0;
        nBalanceInQuestion = 0;

        std::vector<WalletTx> transactionsInWallet;

        {
            LOCK(cs_wallet);

            transactionsInWallet.reserve(mapWallet.size());
            for(auto& item : mapWallet)
                transactionsInWallet.push_back(item.second);

        }

        for(WalletTx& walletTx : transactionsInWallet)
        {
            /* Verify transaction is in the tx db */
            Legacy::Transaction txTemp;

            /* Check all the outputs to make sure the flags are all set properly. */
            for(uint32_t n=0; n < walletTx.vout.size(); n++)
            {
                bool isSpentOnChain = LLD::Legacy->IsSpent(walletTx.GetHash(), n);

                /* Handle when Transaction on chain records output as unspent but wallet accounting has it as spent */
                if(IsMine(walletTx.vout[n]) && walletTx.IsSpent(n) && !isSpentOnChain)
                {
                    debug::log(0, FUNCTION, "Found unspent coin ", FormatMoney(walletTx.vout[n].nValue), " NXS ", walletTx.GetHash().ToString().substr(0, 20), "[", n, "] ", fCheckOnly ? "repair not attempted" : "repairing");

                    ++nMismatchFound;
                    nBalanceInQuestion += walletTx.vout[n].nValue;

                    if(!fCheckOnly)
                    {
                        walletTx.MarkUnspent(n);
                        walletTx.WriteToDisk();
                    }
                }

                /* Handle when Transaction on chain records output as spent but wallet accounting has it as unspent */
                else if(IsMine(walletTx.vout[n]) && !walletTx.IsSpent(n) && isSpentOnChain)
                {
                    debug::log(0, FUNCTION, "Found spent coin ", FormatMoney(walletTx.vout[n].nValue), " NXS ", walletTx.GetHash().ToString().substr(0, 20),
                        "[", n, "] ", fCheckOnly? "repair not attempted" : "repairing");

                    ++nMismatchFound;

                    nBalanceInQuestion += walletTx.vout[n].nValue;

                    if(!fCheckOnly)
                    {
                        walletTx.MarkSpent(n);
                        walletTx.WriteToDisk();
                    }
                }
            }
        }
    }


    /* Checks whether a transaction contains any outputs belonging to this wallet. */
    bool Wallet::IsMine(const Transaction& tx)
    {
        for(const TxOut& txout : tx.vout)
        {
            if(IsMine(txout))
                return true;
        }

        return false;
    }


     /* Checks whether a specific transaction input represents a send from this wallet. */
    bool Wallet::IsMine(const TxIn &txin)
    {
        {
            LOCK(cs_wallet);

            /* Any input from this wallet will have a corresponding UTXO in the previous transaction
             * Thus, if the wallet doesn't contain the previous transaction, the input is not from this wallet.
             * If it does contain the previous tx, must still check that the specific output matching
             * this input belongs to it.
             */
            auto mi = mapWallet.find(txin.prevout.hash);

            if(mi != mapWallet.end())
            {
                const WalletTx& prev = (*mi).second;

                if(txin.prevout.n < prev.vout.size())
                {
                    /* If the matching txout in the previous tx is from this wallet, then this txin is from this wallet */
                    if(IsMine(prev.vout[txin.prevout.n]))
                        return true;
                }
            }
        }
        return false;
    }


    /* Checks whether a specific transaction output represents balance received by this wallet. */
    bool Wallet::IsMine(const TxOut& txout)
    {
        /* Output belongs to this wallet if it has a key matching the output script */
        return Legacy::IsMine(*this, txout.scriptPubKey);
    }


    /* Calculates the total value for all inputs sent from this wallet by a transaction. */
    int64_t Wallet::GetDebit(const Transaction& tx)
    {
        int64_t nDebit = 0;

        for(const auto& txin : tx.vin)
        {
            nDebit += GetDebit(txin);

            if(!MoneyRange(nDebit))
                throw std::runtime_error("Wallet::GetDebit() : value out of range");
        }

        return nDebit;
    }


    /* Calculates the total value for all outputs received by this wallet in a transaction. */
    int64_t Wallet::GetCredit(const Transaction& tx)
    {
        int64_t nCredit = 0;

        for(const auto& txout : tx.vout)
        {
            nCredit += GetCredit(txout);

            if(!MoneyRange(nCredit))
                throw std::runtime_error("Wallet::GetCredit() : value out of range");
        }

        return nCredit;
    }


    /* Calculates the total change amount returned to this wallet by a transaction. */
    int64_t Wallet::GetChange(const Transaction& tx)
    {
        int64_t nChange = 0;

        for(const TxOut& txout : tx.vout)
        {
            nChange += GetChange(txout);

            if(!MoneyRange(nChange))
                throw std::runtime_error("Wallet::GetChange() : value out of range");
        }

        return nChange;
    }


    /* Returns the debit amount for this wallet represented by a transaction input. */
    int64_t Wallet::GetDebit(const TxIn &txin)
    {
        if(txin.prevout.IsNull())
            return 0;

        {
            LOCK(cs_wallet);

            /* A debit spends the txout value from a previous output
             * so begin by finding the previous transaction in the wallet
             */
            auto mi = mapWallet.find(txin.prevout.hash);

            if(mi != mapWallet.end())
            {
                const WalletTx& prev = (*mi).second;

                if(txin.prevout.n < prev.vout.size())
                {
                    /* If the previous txout belongs to this wallet, then debit is from this wallet */
                    if(IsMine(prev.vout[txin.prevout.n]))
                        return prev.vout[txin.prevout.n].nValue;
                }
            }
        }

        return 0;
    }


    /* Returns the credit amount for this wallet represented by a transaction output. */
    int64_t Wallet::GetCredit(const TxOut& txout)
    {
        if(!MoneyRange(txout.nValue))
            throw std::runtime_error("Wallet::GetCredit() : value out of range");

        return (IsMine(txout) ? txout.nValue : 0);
    }


    /* Returns the change amount for this wallet represented by a transaction output. */
    int64_t Wallet::GetChange(const TxOut& txout)
    {
        if(!MoneyRange(txout.nValue))
            throw std::runtime_error("Wallet::GetChange() : value out of range");

        return (IsChange(txout) ? txout.nValue : 0);
    }


    /* Checks whether a transaction output belongs to this wallet and
     *  represents change returned to it.
     */
    bool Wallet::IsChange(const TxOut& txout)
    {
        NexusAddress address;

        {
            LOCK(cs_wallet);

            if(ExtractAddress(txout.scriptPubKey, address) && HaveKey(address))
            {
                return true;
            }
        }

        return false;
    }


    /* Generate a transaction to send balance to a given Nexus address. */
    std::string Wallet::SendToNexusAddress(const NexusAddress& address, const int64_t nValue, WalletTx& wtxNew,
                                            const bool fAskFee, const uint32_t nMinDepth)
    {
        /* Validate amount */
        if(nValue <= 0)
            return std::string("Invalid amount");

        if(nValue < MIN_TXOUT_AMOUNT)
            return debug::safe_printstr("Send amount less than minimum of ", FormatMoney(MIN_TXOUT_AMOUNT), " NXS");

        /* Validate balance supports value + fees */
        if(nValue + MIN_TX_FEE > GetBalance())
            return std::string("Insufficient funds");

        /* Parse Nexus address */
        Script scriptPubKey;
        scriptPubKey.SetNexusAddress(address);

        /* Place the script and amount into sending vector */
        std::vector< std::pair<Script, int64_t> > vecSend;
        vecSend.push_back(make_pair(scriptPubKey, nValue));

        if(IsLocked())
        {
            /* Cannot create transaction when wallet locked */
            std::string strError = std::string("Wallet locked, unable to create transaction.");
            debug::log(0, FUNCTION, strError);
            return strError;
        }

        if(fWalletUnlockMintOnly)
        {
            /* Cannot create transaction if unlocked for mint only */
            std::string strError = std::string("Wallet unlocked for block minting only, unable to create transaction.");
            debug::log(0, FUNCTION, strError);
            return strError;
        }

        int64_t nFeeRequired;

        /* Key will be reserved by CreateTransaction for any change transaction, kept/returned on commit */
        ReserveKey changeKey(*this);

        if(!CreateTransaction(vecSend, wtxNew, changeKey, nFeeRequired, nMinDepth))
        {
            /* Transaction creation failed */
            std::string strError;
            if(nValue + nFeeRequired > GetBalance())
            {
                /* Failure resulted because required fee caused transaction amount to exceed available balance.
                 * Really should not get this because of initial check at start of function. Could only happen
                 * if it calculates an additional fee such that nFeeRequired > MIN_TX_FEE
                 */
                strError = debug::safe_printstr(
                    "This transaction requires a transaction fee of at least ", FormatMoney(nFeeRequired), " because of its amount, complexity, or use of recently received funds  ");
            }
            else
            {
                /* Other transaction creation failure */
                strError = std::string("Transaction creation failed");
            }

            debug::log(0, FUNCTION, strError);

            return strError;
        }

        /* In Tritium, with QT interface removed, we no longer display the fee confirmation here.
         * Successful transaction creation will be committed automatically
         */
        if(!CommitTransaction(wtxNew, changeKey))
            return std::string("The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");

        return "";
    }


    /* Create and populate a new transaction. */
    bool Wallet::CreateTransaction(const std::vector<std::pair<Script, int64_t> >& vecSend, WalletTx& wtxNew, ReserveKey& changeKey,
                                    int64_t& nFeeRet, const uint32_t nMinDepth)
    {
        int64_t nValue = 0;

        /* Cannot create transaction if nothing to send */
        if(vecSend.empty())
            return false;

        /* Calculate total send amount */
        for(const auto& s : vecSend)
        {
            if(s.second < MIN_TXOUT_AMOUNT)
                return false;

            nValue += s.second;
        }

        {
            //LOCK(cs_wallet);

            /* Link wallet to transaction, don't add to wallet yet (will be done when transaction committed) */
            wtxNew.BindWallet(this);

            /* Set fee to minimum for first loop iteration */
            nFeeRet = MIN_TX_FEE;

            /* This loop is generally executed only once, unless the size of the transaction requires a fee increase.
             * When fee increased, it is possible that selected inputs do not cover it, so repeat the process to
             * assure we have enough value in. It also has to re-do the change calculation and output.
             */
            while(true) {
                /* Reset transaction contents */
                wtxNew.vin.clear();
                wtxNew.vout.clear();
                wtxNew.fFromMe = true;

                int64_t nTotalValue = nValue + nFeeRet;

                /* Add transactions outputs to vout */
                for(const auto& s : vecSend)
                    wtxNew.vout.push_back(TxOut(s.second, s.first));

                /* This set will hold txouts (UTXOs) to use as input for this transaction as transaction/vout index pairs */
                std::set<std::pair<const WalletTx*,uint32_t> > setSelectedCoins;

                /* Initialize total value of all inputs */
                int64_t nValueIn = 0;

                /* Choose coins to use for transaction input */
                bool fSelectCoinsSuccessful = false;

                if(wtxNew.strFromAccount == "")
                {
                    /* No value for strFromAccount, so use default for SelectCoins */
                    fSelectCoinsSuccessful = SelectCoins(nTotalValue, wtxNew.nTime, setSelectedCoins, nValueIn, "*", nMinDepth);
                }
                else
                {
                    /* RPC server calls do use strFromAccount, so pass this value forward */
                    fSelectCoinsSuccessful = SelectCoins(nTotalValue, wtxNew.nTime, setSelectedCoins, nValueIn, wtxNew.strFromAccount, nMinDepth);
                }

                if(!fSelectCoinsSuccessful)
                    return false;

                /* Amount of change needed is total of inputs - (total sent + fee) */
                int64_t nChange = nValueIn - nTotalValue;

                /* Any change below minimum tx amount is moved to fee */
                if(nChange > 0 && nChange < MIN_TXOUT_AMOUNT)
                {
                    nFeeRet += nChange;
                    nChange = 0;
                }

                /* When change needed, create a txOut for it and insert into transaction outputs */
                if(nChange > 0)
                {
                    Script scriptChange;

                    if(!config::GetBoolArg("-avatar", true)) //Avatar enabled by default
                    {
                        /* When not avatar mode, use a new key pair from key pool for change */
                        std::vector<uint8_t> vchPubKeyChange = changeKey.GetReservedKey();

                        /* Fill a vout to return change */
                        scriptChange.SetNexusAddress(vchPubKeyChange);

                        /* Name the change address in the address book using from account  */
                        NexusAddress address;
                        if(!ExtractAddress(scriptChange, address) || !address.IsValid())
                            return false;

                        /* Assign address book name for change key, or use "default" if blank or we received it with wildcard set */
                        if(wtxNew.strFromAccount == "" || wtxNew.strFromAccount == "*")
                            addressBook.SetAddressBookName(address, "default");
                        else
                            addressBook.SetAddressBookName(address, wtxNew.strFromAccount);

                    }
                    else
                    {
                        /* Change key not used by avatar mode */
                        changeKey.ReturnKey();

                        /* For avatar mode, return change to the last address retrieved by iterating the input set */
                        for(const auto& item : setSelectedCoins)
                        {
                            const WalletTx& selectedTransaction = *(item.first);

                            /* When done, this will contain scriptPubKey of last transaction in the set */
                            scriptChange = selectedTransaction.vout[item.second].scriptPubKey;
                        }
                    }

                    /* Insert change output at random position: */
                    auto position = wtxNew.vout.begin() + LLC::GetRandInt(wtxNew.vout.size());
                    wtxNew.vout.insert(position, TxOut(nChange, scriptChange));
                }
                else
                {
                    /* No change for this transaction */
                    changeKey.ReturnKey();
                }

                /* Fill vin with selected inputs */
                for(const auto coin : setSelectedCoins)
                    wtxNew.vin.push_back(TxIn(coin.first->GetHash(), coin.second));

                /* Sign inputs to unlock previously unspent outputs */
                uint32_t nIn = 0;
                for(const auto coin : setSelectedCoins)
                    if(!SignSignature(*this, *(coin.first), wtxNew, nIn++))
                        return false;

                /* Limit tx size to 20% of max block size */
                uint32_t nBytes = ::GetSerializeSize(*(Transaction*)&wtxNew, SER_NETWORK, LLP::PROTOCOL_VERSION);
                if(nBytes >= TAO::Ledger::MAX_BLOCK_SIZE_GEN/5)
                    return false; // tx size too large

                /* Each multiple of 1000 bytes of tx size multiplies the fee paid */
                int64_t nPayFee = MIN_TX_FEE * (1 + (int64_t)nBytes / 1000);

                /* Get minimum required fee from transaction */
                int64_t nMinFee = wtxNew.GetMinFee(1, false, Legacy::GMF_SEND);

                /* Check that enough fee is included */
                if(nFeeRet < std::max(nPayFee, nMinFee))
                {
                    /* More fee required, so increase fee and repeat loop */
                    nFeeRet = std::max(nPayFee, nMinFee);
                    continue;
                }

                /* Fill vtxPrev by copying vtxPrev from previous transactions */
                wtxNew.AddSupportingTransactions();

                wtxNew.fTimeReceivedIsTxTime = true; //wtxNew time received set when AddToWallet

                break;
            }
        }
        return true;
    }


    /* Commits a transaction and broadcasts it to the network. */
    bool Wallet::CommitTransaction(WalletTx& wtxNew, ReserveKey& changeKey)
    {
        debug::log(0, FUNCTION, wtxNew.ToString());

        /* Add tx to wallet, because if it has change it's also ours, otherwise just for transaction history.
         * This will update to wallet database
         */
        AddToWallet(wtxNew);

        /* If transaction used the change key, this will remove key pair from key pool so it won't be used again.
         * If key was previously returned, this does nothing.
         */
        changeKey.KeepKey();

        {
            LOCK(cs_wallet);

            /* Mark old coins as spent */
            std::set<WalletTx*> setCoins;
            for(const TxIn& txin : wtxNew.vin)
            {
                WalletTx& prevTx = mapWallet[txin.prevout.hash];
                prevTx.BindWallet(this);
                prevTx.MarkSpent(txin.prevout.n);
                prevTx.WriteToDisk(); //Stores to wallet database
            }
        }

        /* Add to tracking for how many getdata requests our transaction gets */
        /* This entry tracks the transaction hash (not block hash) */
        mapRequestCount[wtxNew.GetHash()] = 0;

        /* Broadcast transaction to network */
        if(!TAO::Ledger::mempool.Accept(wtxNew))
        {
            /* This must not fail. The transaction has already been signed and recorded. */
            debug::log(0, FUNCTION, "Error: Transaction not valid");
            return false;
        }

        wtxNew.RelayWalletTransaction();

        return true;
    }


    /* Add inputs (and output amount with reward) to the coinstake txin for a coinstake block */
    bool Wallet::AddCoinstakeInputs(LegacyBlock& block)
    {
        /* Add Each Input to Transaction. */
        std::vector<WalletTx> vAllWalletTx;
        std::vector<WalletTx> vInputWalletTx;

        block.vtx[0].vout[0].nValue = 0;

        {
            LOCK(cs_wallet);

            vAllWalletTx.reserve(mapWallet.size());

            for(const auto item : mapWallet)
                vAllWalletTx.push_back(item.second);
        }

        std::random_shuffle(vAllWalletTx.begin(), vAllWalletTx.end(), LLC::GetRandInt);

        for(const auto& walletTx : vAllWalletTx)
        {
            /* Can't spend balance that is unconfirmed or not final */
            if(!walletTx.IsFinal() || !walletTx.IsConfirmed())
                continue;

            /* Can't spend coinbase or coinstake transactions that are immature */
            if((walletTx.IsCoinBase() || walletTx.IsCoinStake()) && walletTx.GetBlocksToMaturity() > 0)
                continue;

            /* Do not add coins to Genesis block if age less than trust timestamp */
            uint32_t nMinimumCoinAge = (config::fTestNet.load() ? TAO::Ledger::TRUST_KEY_TIMESPAN_TESTNET : TAO::Ledger::TRUST_KEY_TIMESPAN);
            if(block.vtx[0].IsGenesis() && (block.vtx[0].nTime - walletTx.nTime) < nMinimumCoinAge)
                continue;

            /* Do not add transaction from after Coinstake time */
            if(walletTx.nTime > block.vtx[0].nTime)
                continue;

            /* Transaction is ok to use. Now determine which transaction outputs to include in coinstake input */
            for(uint32_t i = 0; i < walletTx.vout.size(); i++)
            {
                /* Can't spend outputs that are already spent or not belonging to this wallet */
                if(walletTx.IsSpent(i) || !IsMine(walletTx.vout[i]))
                    continue;

                /* Stop adding Inputs if has reached Maximum Transaction Size. */
                unsigned int nBytes = ::GetSerializeSize(block.vtx[0], SER_NETWORK, LLP::PROTOCOL_VERSION);
                if(nBytes >= TAO::Ledger::MAX_BLOCK_SIZE_GEN / 5)
                    break;

                block.vtx[0].vin.push_back(TxIn(walletTx.GetHash(), i));
                vInputWalletTx.push_back(walletTx);

                /** Add the input value to the Coinstake output. **/
                block.vtx[0].vout[0].nValue += walletTx.vout[i].nValue;
            }
        }

        if(block.vtx[0].vin.size() == 1)
            return false; // No transactions added

        /* Calculate the staking reward for the Coinstake Transaction. */
        uint64_t nStakeReward;
        TAO::Ledger::BlockState blockState(block);

        if(!block.vtx[0].CoinstakeReward(blockState, nStakeReward))
            return debug::error(FUNCTION, "Failed to calculate staking reward");

        /* Add the staking reward to the Coinstake output */
        block.vtx[0].vout[0].nValue += nStakeReward;

        /* Sign Each Input to Transaction. */
        for(uint32_t nIndex = 0; nIndex < vInputWalletTx.size(); nIndex++)
        {
            if(!SignSignature(*this, vInputWalletTx[nIndex], block.vtx[0], nIndex + 1))
                return debug::error(FUNCTION, "Unable to sign Coinstake Transaction Input.");

        }

        return true;
    }


    /*
     *  Private load operations are accessible from WalletDB via friend declaration.
     *  Everyone else uses corresponding set/add operation.
     */

    /* Load the minimum supported version without updating the database */
    bool Wallet::LoadMinVersion(const uint32_t nVersion)
    {
        nWalletVersion = nVersion;
        nWalletMaxVersion = std::max(nWalletMaxVersion, nVersion);
        return true;
    }


    /* Loads a master key into the wallet, identified by its key Id. */
    bool Wallet::LoadMasterKey(const uint32_t nMasterKeyId, const MasterKey& kMasterKey)
    {
        if(mapMasterKeys.count(nMasterKeyId) != 0)
            return false;

        mapMasterKeys[nMasterKeyId] = kMasterKey;

        /* After load, wallet nMasterKeyMaxID will contain the maximum key ID currently stored in the database */
        if(nMasterKeyMaxID < nMasterKeyId)
            nMasterKeyMaxID = nMasterKeyId;

        return true;
    }


    /* Load a public/encrypted private key pair to the key store without updating the database. */
    bool Wallet::LoadCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret)
    {
        return CryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret);
    }


    /* Load a key to the key store without updating the database. */
    bool Wallet::LoadKey(const LLC::ECKey& key)
    {
        return CryptoKeyStore::AddKey(key);
    }


    /* Load a script to the key store without updating the database. */
    bool Wallet::LoadScript(const Script& redeemScript)
    {
        return CryptoKeyStore::AddScript(redeemScript);
    }


    /* Selects the unspent transaction outputs to use as inputs when creating a transaction that sends balance from this wallet. */
    bool Wallet::SelectCoins(const int64_t nTargetValue, const uint32_t nSpendTime, std::set<std::pair<const WalletTx*, uint32_t> >& setCoinsRet,
                              int64_t& nValueRet, const std::string& strAccount, uint32_t nMinDepth)
    {
        /* Call detailed select up to 3 times if it fails, using the returns from the first successful call.
         * This allows it to attempt multiple input sets if it doesn't find a workable one on the first try.
         * (example, it chooses an input set with total value exceeding maximum allowed value)
         */
        return (SelectCoinsMinConf(nTargetValue, nSpendTime, nMinDepth, nMinDepth, setCoinsRet, nValueRet, strAccount) ||
                SelectCoinsMinConf(nTargetValue, nSpendTime, nMinDepth, nMinDepth, setCoinsRet, nValueRet, strAccount) ||
                SelectCoinsMinConf(nTargetValue, nSpendTime, nMinDepth, nMinDepth, setCoinsRet, nValueRet, strAccount));
    }


    /* Selects the unspent outputs to use as inputs when creating a transaction to send
     * balance from this wallet while requiring a minimum confirmation depth to be included in result.
     */
    bool Wallet::SelectCoinsMinConf(const int64_t nTargetValue, const uint32_t nSpendTime, const uint32_t nConfMine, const uint32_t nConfTheirs,
                                std::set<std::pair<const WalletTx*, uint32_t> >& setCoinsRet, int64_t& nValueRet, const std::string& strAccount)
    {
        /* cs_wallet should already be locked when this is called (CreateTransaction) */
        setCoinsRet.clear();
        std::vector<const WalletTx*> vCoins;

        nValueRet = 0;

        if(config::GetBoolArg("-printselectcoin", false))
            debug::log(0, FUNCTION, "Selecting coins for account ", strAccount);

        /* Build a set of wallet transactions from all transaction in the wallet map */
        vCoins.reserve(mapWallet.size());
        for(const auto& item : mapWallet)
            vCoins.push_back(&item.second);

        /* Randomly order the transactions as potential inputs */
        std::random_shuffle(vCoins.begin(), vCoins.end(), LLC::GetRandInt);

        /* Loop through all transactions, finding and adding available unspent balance to the list of outputs until reach nTargetValue */
        for(const WalletTx* walletTx : vCoins)
        {
            /* Can't spend transaction from after spend time */
            if(walletTx->nTime > nSpendTime)
                continue;

            /* Can't spend balance that is unconfirmed or not final */
            if(!walletTx->IsFinal() || !walletTx->IsConfirmed())
                continue;

            /* Can't spend transaction that has not reached minimum depth setting for mine/theirs */
            if(walletTx->GetDepthInMainChain() < (walletTx->IsFromMe() ? nConfMine : nConfTheirs))
                continue;

            /* Can't spend coinbase or coinstake transactions that are immature */
            if((walletTx->IsCoinBase() || walletTx->IsCoinStake()) && walletTx->GetBlocksToMaturity() > 0)
                continue;

            /* So far, the transaction itself is available, now have to check each output to see if there are any we can use */
            for(uint32_t i = 0; i < walletTx->vout.size(); i++)
            {
                /* Can't spend outputs that are already spent or not belonging to this wallet */
                if(walletTx->IsSpent(i) || !IsMine(walletTx->vout[i]))
                    continue;

                /* Handle account selection here. */
                if(strAccount != "*")
                {
                    NexusAddress address;
                    if(!ExtractAddress(walletTx->vout[i].scriptPubKey, address) || !address.IsValid())
                        continue;

                    if(addressBook.HasAddress(address))
                    {
                        std::string strEntry = addressBook.GetAddressBookName(address);
                        if(strEntry == "")
                            strEntry = "default";

                        if(strEntry == strAccount)
                        {
                            /* Account label for transaction address matches request, include in result */
                            setCoinsRet.insert(std::make_pair(walletTx, i));
                            nValueRet += walletTx->vout[i].nValue;
                        }
                    }
                    else if(strAccount == "default")
                    {
                        /* Not in address book (no label), include if default requested */
                        setCoinsRet.insert(std::make_pair(walletTx, i));
                        nValueRet += walletTx->vout[i].nValue;
                    }
                }

                /* Handle wildcard here, include all transactions. */
                else
                {
                    /* Add transaction with selected vout index to result set */
                    setCoinsRet.insert(std::make_pair(walletTx, i));

                    /* Accumulate total value available to spend in result set */
                    nValueRet += walletTx->vout[i].nValue;
                }

                /* If value available to spend in result set exceeds target value, we are done */
                if(nValueRet >= nTargetValue)
                    break;
            }

            /* This is tested twice to break out of both loops */
            if(nValueRet >= nTargetValue)
                break;

        }

        /* Print result set when argument set */
        if(config::GetBoolArg("-printselectcoin", false))
        {
            debug::log(0, FUNCTION, "Coins selected: ");
            for(const auto& item : setCoinsRet)
                item.first->print();

            debug::log(0, FUNCTION, "Total ", FormatMoney(nValueRet));
        }

        /* Ensure input total value does not exceed maximum allowed */
        if(!MoneyRange(nValueRet))
            return debug::error(FUNCTION, "Input total over TX limit. Total: ", nValueRet, " Limit ",  MaxTxOut());

        /* Ensure balance is sufficient to cover transaction */
        if(nValueRet < nTargetValue)
            return debug::error(FUNCTION, "Insufficient Balance. Target: ", nTargetValue, " Actual ",  nValueRet);

        return true;
    }

}
