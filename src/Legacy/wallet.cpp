/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/include/version.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/tritium.h>
#include <TAO/Ledger/types/transaction.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/money.h>
#include <Legacy/include/signature.h>
#include <Legacy/include/enum.h> // For GMF_SEND
#include <Legacy/types/script.h>

#include <Legacy/wallet/crypter.h>
#include <Legacy/types/output.h>
#include <Legacy/types/reservekey.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>
#include <Util/include/runtime.h>
#include <Util/include/signals.h>
#include <Util/include/string.h>

#include <openssl/rand.h>   // For RAND_bytes

#include <algorithm>
#include <thread>
#include <utility>

namespace Legacy
{
    /** Internal pointer for this class's instance. */
    std::atomic<Wallet*> Wallet::INSTANCE = nullptr;


    /* Track if wallet has been loaded from disk. */
    bool Wallet::fFileBacked = false;
    bool Wallet::fLoaded     = false;


    /* Nexus: Setting indicating wallet unlocked for block minting only */
    bool fWalletUnlockMintOnly = false;


    /* Initialize static variables */
    std::atomic<bool> Wallet::fWalletInitialized(false);


    /* Transaction fee to be used. */
    uint64_t TRANSACTION_FEE = MIN_TX_FEE;


    /* Changes the way wallet accounts for change transactions. */
    uint32_t WALLET_ACCOUNTING_TIMELOCK = 0;


    /** Constructor **/
    Wallet::Wallet()
    : CryptoKeyStore    ( )
    , nWalletVersion    (FEATURE_BASE)
    , nWalletMaxVersion (FEATURE_BASE)
    , mapMasterKeys     ( )
    , nMasterKeyMaxID   (0)
    , addressBook       (AddressBook(*this))
    , keyPool           (KeyPool(*this))
    , vchDefaultKey     ( )
    , vchTrustKey       ( )
    , nWalletUnlockTime (0)
    , cs_wallet         ( )
    , mapWallet         ( )
    {
    }


    /** Destructor **/
    Wallet::~Wallet()
    {
    }


    /* Initializes the wallet instance. */
    bool Wallet::Initialize(const std::string& strWalletFileIn)
    {
        if(Wallet::fWalletInitialized.load())
            return false;
        else
        {
            /* Check for valid pointer and initialization. */
            if(Wallet::INSTANCE.load() == nullptr)
            {
                Wallet::INSTANCE.store(new Wallet());
                Wallet::fWalletInitialized.store(true);
            }

            /* Initialize wallet static values. */
            Wallet::Instance().fFileBacked = true;

            /* Initialize database environment. */
            WalletDB::Initialize(strWalletFileIn);
        }

        return true;
    }


    Wallet& Wallet::Instance()
    {
        /* Check for null accesses. */
        if(Wallet::INSTANCE.load() == nullptr)
            throw debug::exception(FUNCTION, "-> access to nullptr");

        /* Return our instance. */
        return *Wallet::INSTANCE.load();
    }


    /* Shutdown the wallet subsystems. */
    void Wallet::Shutdown()
    {
        /* Check if instance needs to be initialized. */
        if(Wallet::INSTANCE.load() != nullptr)
        {
            /* Cleanup our pointers. */
            delete Wallet::INSTANCE.load();
            Wallet::INSTANCE.store(nullptr);
        }

        /* Shut down wallet database environment. */
        WalletDB::Shutdown();
    }


    /* Assign the minimum supported version for the wallet. */
    bool Wallet::SetMinVersion(const enum Legacy::WalletFeature nVersion, const bool fForceLatest)
    {
        {
            RECURSIVE(cs_wallet);

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
                WalletDB::WriteMinVersion(nWalletVersion);
            }
        }

        return true;
    }


    /* Assign the maximum version we're allowed to upgrade to.  */
    bool Wallet::SetMaxVersion(const enum Legacy::WalletFeature nVersion)
    {
        {
            RECURSIVE(cs_wallet);

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

        {
            /* Lock wallet so WalletDB can load all data into it */
            //RECURSIVE(cs_wallet);

            uint32_t nLoadWalletRet = WalletDB::LoadWallet();
            if(nLoadWalletRet != DB_LOAD_OK)
                return nLoadWalletRet;
        }

        /* New wallet is indicated by an empty default key */
        fFirstRunRet = Wallet::Instance().vchDefaultKey.empty();

        /* On first run, assign min/max version, generate key pool, and generate a default key for this wallet */
        if(fFirstRunRet && !Wallet::Instance().IsLocked())
        {
            /* For a newly created wallet, set the min and max version to the latest */
            debug::log(2, FUNCTION, "Setting wallet min version to ", FEATURE_LATEST);

            Wallet::Instance().SetMinVersion(FEATURE_LATEST);

            std::vector<uint8_t> vchNewDefaultKey;

            /* For a new wallet, may need to generate initial key pool */
            if(Wallet::Instance().GetKeyPool().GetKeyPoolSize() == 0)
                Wallet::Instance().GetKeyPool().NewKeyPool();

            if(Wallet::Instance().GetKeyPool().GetKeyPoolSize() > 0)
            {
                debug::log(2, FUNCTION, "Adding wallet default key");

                if(!Wallet::Instance().GetKeyPool().GetKeyFromPool(vchNewDefaultKey, false))
                {
                    debug::error(FUNCTION, "Error adding wallet default key. Cannot get key from key pool.");
                    return DB_LOAD_FAIL;
                }

                Wallet::Instance().SetDefaultKey(vchNewDefaultKey);

                if(!Wallet::Instance().GetAddressBook().SetAddressBookName(NexusAddress(Wallet::Instance().vchDefaultKey), "default"))
                {
                    debug::error(FUNCTION, "Error adding wallet default key. Unable to add key to address book.");
                    return DB_LOAD_FAIL;
                }
            }
        }
        else if(Wallet::Instance().nWalletVersion == FEATURE_BASE)
        {
            /* Old wallets set min version but it never got recorded because constructor defaulted the value.
             * This assures older wallet files have it stored.
             */
            uint32_t nStoredMinVersion = 0;

            if(!WalletDB::ReadMinVersion(nStoredMinVersion) || nStoredMinVersion == 0)
                Wallet::Instance().SetMinVersion(FEATURE_BASE);
        }

        /* Max allowed version is always the current latest */
        Wallet::Instance().SetMaxVersion(FEATURE_LATEST);

        /* Launch background thread to periodically flush the wallet to the backing database */
        WalletDB::StartFlushThread();

        fLoaded = true;

        return DB_LOAD_OK;
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
            result = WalletDB::WriteCryptedKey(vchPubKey, vchCryptedSecret);

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
            bool result = WalletDB::WriteKey(key.GetPubKey(), key.GetPrivKey());

            return result;
        }


        return true;
    }


    /* Add a script to the key store.  */
    bool Wallet::AddScript(const Script& redeemScript)
    {
        {
            RECURSIVE(cs_wallet);

            /* Call overridden inherited method to add key to key store */
            if(!CryptoKeyStore::AddScript(redeemScript))
                return false;

            if(fFileBacked)
            {
                bool result = WalletDB::WriteScript(LLC::SK256(redeemScript), redeemScript);

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
            RECURSIVE(cs_wallet);

            if(fFileBacked)
            {
                bool result = WalletDB::WriteDefaultKey(vchPubKey);

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
            RECURSIVE(cs_wallet);

            if(fFileBacked)
            {
                bool result = WalletDB::WriteTrustKey(vchPubKey);

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
            RECURSIVE(cs_wallet);

            if(fFileBacked)
            {
                bool result = WalletDB::EraseTrustKey();

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
            RECURSIVE(cs_wallet);

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
                bool fDbEncryptionSuccessful = false;

                fDbEncryptionSuccessful = WalletDB::EncryptDatabase(nNewMasterKeyId, kMasterKey, mapNewEncryptedKeys);

                if(!fDbEncryptionSuccessful)
                {
                    /* Keys encrypted in memory, but not on disk...die to let the user reload their unencrypted wallet. */
                    config::fShutdown.store(true);
                    SHUTDOWN.notify_all();
                    return debug::error(FUNCTION, "Unable to complete encryption for ", WalletDB::GetDatabaseFile(), ". Encryption aborted. Shutting down.");
                }
            }
        }

        /* Lock wallet, then unlock with new passphrase to update key pool */
        Lock();

        /* Unlock wallet only to replace key pool. Don't start stake minter for this unlock */
        Unlock(strWalletPassphrase, 0, false);

        /* Replace key pool with encrypted keys */
        {
            RECURSIVE(cs_wallet);

            Wallet::Instance().GetKeyPool().NewKeyPool();
        }

        /* Lock wallet again before rewrite */
        Lock();

        /* Need to completely rewrite the wallet file; if we don't, bdb might keep
         * bits of the unencrypted private key in slack space in the database file.
         */
        bool rewriteResult = true;
        {
            RECURSIVE(cs_wallet);

            if(fFileBacked)
            {
                rewriteResult = WalletDB::DBRewrite();
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
            return true;

        return false;
    }


    /* Attempt to unlock an encrypted wallet using the passphrase provided. */
    bool Wallet::Unlock(const SecureString& strWalletPassphrase, const uint32_t nUnlockSeconds, const bool fStartStake)
    {
        if(!IsLocked())
            return false;

        Crypter crypter;
        CKeyingMaterial vMasterKey;
        bool fUnlockSuccessful = false;

        {
            RECURSIVE(cs_wallet);

            /* If more than one master key in wallet's map (unusual), this will attempt each one with the passphrase.
             * If any one master key decryption works and unlocks the wallet, then the unlock is successful.
             * Supports a multi-user wallet, where each user has their own passphrase
             */
            for(const auto& pMasterKey : mapMasterKeys)
            {
                /* Set the encryption context using the passphrase provided */
                if(!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                {
                    debug::error(FUNCTION, "Error setting encryption context from passphrase");
                    break;
                }

                /* Attempt to decrypt the master key using the passphrase crypter */
                if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                {
                    debug::error(FUNCTION, "Error unlocking key with passphrase");
                    break;
                }

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

                    fUnlockSuccessful = true;
                    break;
                }
            }
        }

        return fUnlockSuccessful;
    }


    /* Replaces the existing wallet passphrase with a new one. */
    bool Wallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
    {
        /* To change the passphrase, the old passphrase must successfully decrypt the master key and unlock
         * the wallet, thus proving it is the correct one. Then, the master key is re-encrypted using the
         * new passphrase and saved.
         */

        {
            RECURSIVE(cs_wallet);

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
                        WalletDB::WriteMasterKey(pMasterKey.first, pMasterKey.second);
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
        {
            /* Lock wallet only to take a snapshot of current transaction map for calculating balance
             * After unlock, mapWallet can change but it won't affect balance calculation
             */
            RECURSIVE(cs_wallet);

            for (const auto& item : mapWallet)
            {
                const WalletTx& wtx = item.second;

                /* Skip any transaction that isn't final, isn't completely confirmed, or has a future timestamp */
                if (!wtx.IsFinal() || !wtx.IsConfirmed() || wtx.nTime > runtime::unifiedtimestamp())
                    continue;

                nTotalBalance += wtx.GetAvailableCredit();
            }
        }



        return nTotalBalance;
    }



    /* Get the balance for an account associated with a wallet. */
    bool Wallet::BalanceByAccount(std::string strAccount, int64_t& nBalance, const int32_t nMinDepth)
    {
        {
            RECURSIVE(cs_wallet);
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
        {
            /* Lock wallet only to take a snapshot of current transaction map for calculating balance
             * After unlock, mapWallet can change but it won't affect balance calculation
             */
            RECURSIVE(cs_wallet);

            for (const auto& item : mapWallet)
            {
                const WalletTx& wtx = item.second;

                if (wtx.IsFinal() && wtx.IsConfirmed())
                    continue;

                nUnconfirmedBalance += wtx.GetAvailableCredit();
            }
        }



        return nUnconfirmedBalance;
    }


    /* Retrieves the current immature stake balance. */
    int64_t Wallet::GetStake()
    {
        int64_t nTotalStake = 0;

        {
            RECURSIVE(cs_wallet);

            for(const auto& item : mapWallet)
            {
                const WalletTx& wtx = item.second;

                /* Amount currently being staked is that amount in a coinstake tx that is not yet mature but has been added to chain */
                if (wtx.IsCoinStake() && wtx.GetBlocksToMaturity() > 0 && wtx.GetDepthInMainChain() > 0)
                    nTotalStake += GetCredit(wtx);
            }
        }

        return nTotalStake;
    }


    /* Retrieves the current immature minted (mined) balance. */
    int64_t Wallet::GetNewMint()
    {
        int64_t nTotalMint = 0;

        {
            RECURSIVE(cs_wallet);

            for(const auto& item : mapWallet)
            {
                const WalletTx& wtx = item.second;

                if (wtx.IsCoinBase() && wtx.GetBlocksToMaturity() > 0 && wtx.GetDepthInMainChain() > 1)
                    nTotalMint += GetCredit(wtx);
            }
        }

        return nTotalMint;
    }


    /* Populate vCoins with vector identifying spendable outputs. */
    void Wallet::AvailableCoins(const uint32_t nSpendTime, std::vector<Output>& vCoins, const bool fOnlyConfirmed)
    {
        {
            RECURSIVE(cs_wallet);

            vCoins.clear();

            for(const auto& item : mapWallet)
            {
                const WalletTx& wtx = item.second;

                /* Filter transactions not final */
                if (!wtx.IsFinal())
                    continue;

                /* Filter unconfirmed transactions unless want unconfirmed */
                if (fOnlyConfirmed && !wtx.IsConfirmed())
                    continue;

                /* Filter immature minting and staking transactions */
                if ((wtx.IsCoinBase() || wtx.IsCoinStake()) && wtx.GetBlocksToMaturity() > 0)
                    continue;

                for (uint32_t i = 0; i < wtx.vout.size(); i++)
                {
                    /* Filter transactions after requested spend time */
                    if (wtx.nTime > nSpendTime)
                        continue;

                    /* To be included in result, vout must not be spent, must belong to current wallet, and must have positive value */
                    if (!(wtx.IsSpent(i)) && IsMine(wtx.vout[i]) && wtx.vout[i].nValue > 0)
                    {
                        /* Create output from the current vout and add to result */
                        Output txOutput(wtx, i, wtx.GetDepthInMainChain());
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
            RECURSIVE(cs_wallet);

            for(auto& item : mapWallet)
                item.second.MarkDirty();
        }
    }


    /*  Retrieves the transaction for a given transaction hash. */
    bool Wallet::GetTransaction(const uint512_t& hashTx, WalletTx& wtx)
    {
        {
            RECURSIVE(cs_wallet);

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


    /* Adds a wallet transaction to the wallet. */
    bool Wallet::AddToWallet(const WalletTx& wtxIn, uint512_t hash)
    {
        /* Check for explicit declaration of hashes. */
        if(hash == 0)
            hash = wtxIn.GetHash();

        /* Use the returned tx, not wtxIn, in case insert returned an existing transaction */
        std::pair<TransactionMap::iterator, bool> ret;
        {
            RECURSIVE(cs_wallet);

            /* Inserts only if not already there, returns tx inserted or tx found */
            ret = mapWallet.insert(std::make_pair(hash, wtxIn));
        }

        WalletTx& wtx = (*ret.first).second;
        wtx.BindWallet(this);

        bool fInsertedNew = ret.second;
        if (fInsertedNew && wtx.nTimeReceived == 0) // Time will be non-zero if preset by processing (such as rescan)
        {
            /* wtx.nTimeReceive must remain uint32_t for backward compatibility */
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
        debug::log(0, FUNCTION, hash.SubString(10), " ", (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

        /* Write to disk */
        if(fInsertedNew || fUpdated)
            if(!wtx.WriteToDisk(hash))
                return false;

        /* If default receiving address gets used, replace it with a new one */
        Script scriptDefaultKey;
        scriptDefaultKey.SetNexusAddress(vchDefaultKey);

        for(const TxOut& txout : wtx.vout)
        {
            if(txout.scriptPubKey == scriptDefaultKey)
            {
                std::vector<uint8_t> newDefaultKey;

                if(Wallet::Instance().GetKeyPool().GetKeyFromPool(newDefaultKey, false))
                {
                    SetDefaultKey(newDefaultKey);
                    Wallet::Instance().GetAddressBook().SetAddressBookName(NexusAddress(vchDefaultKey), "");
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
    bool Wallet::AddToWalletIfInvolvingMe(const Transaction& tx, TAO::Ledger::BlockState& state,
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

            if(fRescan || TAO::Ledger::ChainState::Synchronizing())
            {
                /* On rescan or initial download, set wtx time to transaction time instead of time tx received.
                 * These are both uint32_t timestamps to support unserialization of legacy data.
                 */
                wtx.nTimeReceived = tx.nTime;
            }

            /* If find is enabled, read the block from LLD. */
            if(fFindBlock)
            {
                /* Read the block state. */
                if(!LLD::Ledger->ReadBlock(hash, state))
                    return debug::error(FUNCTION, "tx ", hash.SubString(), " is an ORPHAN");

                wtx.hashBlock = state.GetHash();
            }
            else if(!state.IsNull())
            {
                /* If have block for transaction, record the hashBlock. */
                wtx.hashBlock = state.GetHash();
            }

            /* AddToWallet preforms merge (update) for transactions already in wallet */
            return AddToWallet(wtx, hash);
        }

        /* Check for spent flags if this transaction was not ours. */
        WalletUpdateSpent(tx);

        return false;
    }


    /*  Checks whether a transaction has inputs or outputs belonging to this wallet, and adds
     *  it to the wallet when it does.
     */
    bool Wallet::AddToWalletIfInvolvingMe(const TAO::Ledger::Transaction& txIn, TAO::Ledger::BlockState& state,
                                          bool fUpdate, bool fFindBlock, bool fRescan)
    {
        uint512_t hash = txIn.GetHash();

        /* Check to see if transaction hash in this wallet */
        bool fExisted = mapWallet.count(hash);

        /* When transaction already in wallet, return unless update is specifically requested */
        if(fExisted && !fUpdate)
            return false;

        /* Check if transaction has outputs (IsMine) or inputs (IsFromMe) belonging to this wallet */
        if(IsMine(txIn))
        {
            /* Build a semi-legacy tx from tritium contracts. */
            Transaction tx(txIn);
            WalletTx wtx(this, tx);

            /* Set the times. */
            if(fRescan || TAO::Ledger::ChainState::Synchronizing())
            {
                /* On rescan or initial download, set wtx time to transaction time instead of time tx received.
                 * These are both uint32_t timestamps to support unserialization of legacy data.
                 */
                wtx.nTimeReceived = tx.nTime;
            }

            /* If find is enabled, read the block from LLD. */
            if(fFindBlock)
            {
                /* Read the block state. */
                if(!LLD::Ledger->ReadBlock(hash, state))
                    return debug::error(FUNCTION, "tx ", hash.SubString(), " is an ORPHAN");

                wtx.hashBlock = state.GetHash();
            }
            else if(!state.IsNull())
            {
                /* If have block for transaction, record the hashBlock. */
                wtx.hashBlock = state.GetHash();
            }

            /* AddToWallet preforms merge (update) for transactions already in wallet.
             * For OP::LEGACY tritium transactions, add the source tritium transaction hash (from txIn), not the wtx hash.
             */
            return AddToWallet(wtx, hash);
        }

        return false;
    }


    /* Removes a wallet transaction from the wallet, if present. */
    bool Wallet::EraseFromWallet(const uint512_t& hash)
    {
        if(!fFileBacked)
            return false;
        {
            RECURSIVE(cs_wallet);

            if(mapWallet.erase(hash))
            {
                WalletDB::EraseTx(hash);
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
            RECURSIVE(cs_wallet);

            for(const TxIn& txin : tx.vin)
            {
                /* Find the previous transaction and mark as unspent the output that corresponds to current txin */
                TransactionMap::iterator mi = mapWallet.find(txin.prevout.hash);

                if(mi != mapWallet.end())
                {
                    WalletTx& txPrev = (*mi).second;

                    if (txin.prevout.n < txPrev.vout.size() && IsMine(txPrev.vout[txin.prevout.n]))
                    {
                        txPrev.MarkUnspent(txin.prevout.n);
                        txPrev.WriteToDisk(tx.GetHash());
                    }
                }
            }
        }
    }


    /* Scan the block chain for transactions from or to keys in this wallet.
     * Add/update the current wallet transactions for any found.
     */
    uint32_t Wallet::ScanForWalletTransactions(const TAO::Ledger::BlockState& stateBegin, const bool fUpdate)
    {
        /* Count the number of transactions process for this wallet to use as return value */
        uint32_t nTransactionCount = 0;
        uint32_t nScannedCount     = 0;

        runtime::timer timer;
        timer.Start();

        debug::log(0, FUNCTION, "Begin rescan");
        /* Check for null. */
        if(!stateBegin)
            return debug::error(FUNCTION, "search from block is invalid");

        uint512_t hashLast = 0;
        TAO::Ledger::BlockState stateStart = stateBegin;
        bool fFirst = true;

        /* Check for genesis. */
        if(stateStart.nHeight == 0)
        {
            /* If beginning from genesis block, move to height 1 */
            stateStart = stateStart.Next();
            if(!stateStart)
                return debug::error(FUNCTION, "next block is null");
        }

        /* Begin by scanning all Legacy transactions from the starting block. */

        /* Starting point for scan is the first Legacy transaction within the starting block. */
        for(auto item : stateStart.vtx)
        {
            if(item.first == TAO::Ledger::TRANSACTION::LEGACY)
            {
                hashLast = item.second;
                break;
            }
        }

        /* If we were passed a starting block without a Legacy transaction, start from beginning */
        if(hashLast == 0)
        {
            /* Read the first Legacy transaction and begin scan from its hash */
            std::vector<Transaction> vtx;

            if(LLD::Legacy->BatchRead("tx", vtx, 1))
                hashLast = vtx[0].GetHash();
        }

        if(hashLast != 0)
        {
            debug::log(0, FUNCTION, "Scanning Legacy from tx ", hashLast.SubString());

            /* Loop until complete. */
            while(!config::fShutdown.load())
            {
                /* Read the next batch of inventory.
                 * The starting hash is only included in results on first iteration when fFirst is true.
                 */
                std::vector<Transaction> vtx;
                if(!LLD::Legacy->BatchRead(std::make_pair(std::string("tx"), hashLast), "tx", vtx, 1000, !fFirst))
                    break;

                /* Loop through found transactions. */
                TAO::Ledger::BlockState state;
                for(const auto& tx : vtx)
                {
                    /* Add to the wallet */
                    if(AddToWalletIfInvolvingMe(tx, state, fUpdate, true, true))
                    {
                        /* Get txid. */
                        uint512_t hash = tx.GetHash();

                        /* Update spent flags. */
                        WalletTx& wtx = mapWallet[hash];
                        for(uint32_t n = 0; n < wtx.vout.size(); ++n)
                        {
                            /* Update spent pointers. */
                            if(LLD::Legacy->IsSpent(hash, n))
                            {
                                wtx.MarkSpent(n);
                                wtx.WriteToDisk(hash);
                            }
                        }

                        ++nTransactionCount;
                    }

                    /* Update the scanned count for meters. */
                    ++nScannedCount;

                    /* Meter for output. */
                    if(nScannedCount % 100000 == 0)
                    {
                        /* Get the time it took to rescan. */
                        uint32_t nElapsedSeconds = timer.Elapsed();
                        debug::log(0, FUNCTION, "Processed ", nTransactionCount,
                            " tx of ", nScannedCount, " in ", nElapsedSeconds, " seconds (",
                            std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");
                    }
                }

                /* Set hash Last. */
                hashLast = vtx.back().GetHash();

                /* Check for end. */
                if(vtx.size() != 1000)
                    break;

                /* Set first flag. */
                fFirst = false;
            }
        }

        /* After completing Legacy scan, also scan Tritium tx. These may contains send-to-legacy contracts to add into wallet */
        hashLast = 0;
        fFirst = true;

        if(stateStart.nVersion < 7)
        {
            /* If started from a Legacy block, read the first Tritium tx to set hashLast */
            std::vector<TAO::Ledger::Transaction> vtx;
            if(LLD::Ledger->BatchRead("tx", vtx, 1))
                hashLast = vtx[0].GetHash();
        }
        else
        {
            /* If started scan from a Tritium block, starting tx hash is first Tritium transaction it contains
             * (it must have at least one, because BlockState stores producer as last vtx entry)
             */
            for(auto item : stateStart.vtx)
            {
                if(item.first == TAO::Ledger::TRANSACTION::TRITIUM)
                {
                    hashLast = item.second;
                    break;
                }
            }
        }

        /* Loop through tritium transactions. */
        if(hashLast != 0)
        {
            debug::log(0, FUNCTION, "Scanning Tritium from tx ", hashLast.SubString());
            while(!config::fShutdown.load())
            {
                /* Read the next batch of inventory. */
                std::vector<TAO::Ledger::Transaction> vtx;
                if(!LLD::Ledger->BatchRead(hashLast, "tx", vtx, 1000, !fFirst))
                    break;

                /* Loop through found transactions. */
                TAO::Ledger::BlockState state;
                for(const auto& tx : vtx)
                {
                    /* Add to the wallet */
                    if(AddToWalletIfInvolvingMe(tx, state, fUpdate, true, true))
                    {
                        /* Get txid. */
                        uint512_t hash = tx.GetHash();

                        /* Update spent flags. */
                        WalletTx& wtx = mapWallet[hash];
                        for(uint32_t n = 0; n < wtx.vout.size(); ++n)
                        {
                            /* Update spent pointers. */
                            if(LLD::Legacy->IsSpent(hash, n))
                            {
                                wtx.MarkSpent(n);
                                wtx.WriteToDisk(hash);
                            }
                        }

                        ++nTransactionCount;
                    }

                    /* Update the scanned count for meters. */
                    ++nScannedCount;

                    /* Meter for output. */
                    if(nScannedCount % 100000 == 0)
                    {
                        /* Get the time it took to rescan. */
                        uint32_t nElapsedSeconds = timer.Elapsed();
                        debug::log(0, FUNCTION, "Processed ", nTransactionCount,
                            " tx of ", nScannedCount, " in ", nElapsedSeconds, " seconds (",
                            std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");
                    }
                }

                /* Set hash Last. */
                hashLast = vtx.back().GetHash();

                /* Check for end. */
                if(vtx.size() != 1000)
                    break;

                /* Set first flag. */
                fFirst = false;
            }
        }

        /* Get the time it took to rescan. */
        uint32_t nElapsedSeconds = timer.Elapsed();
        debug::log(0, FUNCTION, "Rescan complete");
        debug::log(0, FUNCTION, "Processed ", nTransactionCount,
            " tx of ", nScannedCount, " in ", nElapsedSeconds, " seconds (",
            std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");

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
        static std::atomic<uint64_t> snNextTime;

        /* Also keep track of best height on last resend, because no need to process again if has not changed */
        static std::atomic<uint32_t> snLastHeight;


        bool fFirst = (snNextTime == 0);

        /* Always false on first iteration */
        if(runtime::unifiedtimestamp() < snNextTime.load())
            return;

        /* Set a random time until resend is processed */
        snNextTime = runtime::unifiedtimestamp() + LLC::GetRand(5 * 60); //5 minutes

        /* On first iteration, just return. All it does is set snNextTime */
        if(fFirst)
            return;

        /* If no new block, nothing has changed, so just return. */
        if(TAO::Ledger::ChainState::nBestHeight.load() <= snLastHeight.load())
            return;

        /* Record that it is processing resend now */
        snLastHeight = TAO::Ledger::ChainState::nBestHeight.load();
        {
            RECURSIVE(cs_wallet);

            /* Find any sent tx not in block and sort them in chronological order */
            std::multimap<uint64_t, const WalletTx*> mapSorted;
            for(const auto& item : mapWallet)
            {
                const WalletTx& wtx = item.second;
                if(wtx.hashBlock != 0)
                    continue;

                /* Don't put in sorted map for rebroadcast until it's had enough time to be added to a block */
                if(runtime::timestamp() - wtx.nTimeReceived > 1 * 60)
                    mapSorted.insert(std::make_pair(wtx.nTimeReceived, &wtx));
            }

            /* Loop through sorted transactions. */
            for(const auto& item : mapSorted)
            {
                const WalletTx* wtx = item.second;

                /* Validate the transaction, then process rebroadcast on it */
                if(wtx->Check())
                    wtx->RelayWalletTransaction();
            }
        }
    }


    /* Checks a transaction to see if any of its inputs match outputs from wallet transactions
     * in this wallet. For any it finds, verifies that the outputs are marked as spent, updating
     * them as needed.
     */
    void Wallet::WalletUpdateSpent(const Transaction& tx)
    {
        {
            RECURSIVE(cs_wallet);

            /* Loop through and the tx inputs, checking each separately */
            for(const auto& txin : tx.vin)
            {
                /* Check the txin to see if prevout hash maps to a transaction in this wallet */
                TransactionMap::iterator mi = mapWallet.find(txin.prevout.hash);
                if(mi != mapWallet.end())
                {
                    /* When there is a match to the prevout hash, get the previous wallet transaction */
                    WalletTx& wtx = (*mi).second;

                    /* Check if wallet is bound. */
                    if(!wtx.IsBound())
                        wtx.BindWallet(this);

                    /* Outputs in wallet tx will have same index recorded in transaction txin
                     * Check any that are not flagged spent for belonging to this wallet and mark them as spent
                     */
                    if(!wtx.IsSpent(txin.prevout.n) && IsMine(wtx.vout[txin.prevout.n]))
                    {
                        debug::log(2, FUNCTION, "Found spent coin ", FormatMoney(wtx.GetCredit()), " NXS ", wtx.GetHash().SubString());

                        wtx.MarkSpent(txin.prevout.n);
                        wtx.WriteToDisk(txin.prevout.hash);
                    }
                }
            }
        }
    }


    /*  Identifies and fixes mismatches of spent coins between the wallet and the index db.  */
    void Wallet::FixSpentCoins(uint32_t& nMismatchFound, int64_t& nBalanceInQuestion, const bool fCheckOnly)
    {
        RECURSIVE(cs_wallet);

        nMismatchFound = 0;
        nBalanceInQuestion = 0;

        /* Keep track of repaired coins. */
        std::map<uint512_t, WalletTx> mapRepaired;

        {
            RECURSIVE(cs_wallet);

            /* Loop through all values in the wallet map. */
            for(auto& map : mapWallet)
            {
                /* Get a reference of value. */
                WalletTx& wtx = map.second;

                /* Check all the outputs to make sure the flags are all set properly. */
                for (uint32_t n=0; n < wtx.vout.size(); n++)
                {
                    if (!IsMine(wtx.vout[n]))
                        continue;

                    /* Check in LLD keychains and memory pool for spends. */
                    bool fSpentOnChain = LLD::Legacy->IsSpent(map.first, n) || TAO::Ledger::mempool.IsSpent(map.first, n);

                    /* Handle when Transaction on chain records output as unspent but wallet accounting has it as spent */
                    if (wtx.IsSpent(n) && !fSpentOnChain)
                    {
                        debug::log(2, FUNCTION, "Found unspent coin ", FormatMoney(wtx.vout[n].nValue), " NXS ", wtx.GetHash().SubString(),
                            "[", n, "] ", fCheckOnly ? "repair not attempted" : "repairing");

                        ++nMismatchFound;
                        nBalanceInQuestion += wtx.vout[n].nValue;

                        if (!fCheckOnly)
                        {
                            wtx.MarkUnspent(n);
                            wtx.WriteToDisk(map.first);

                            mapRepaired[map.first] = map.second;
                        }
                    }

                    /* Handle when Transaction on chain records output as spent but wallet accounting has it as unspent */
                    else if (!wtx.IsSpent(n) && fSpentOnChain)
                    {
                        debug::log(2, FUNCTION, "Found spent coin ", FormatMoney(wtx.vout[n].nValue), " NXS ", wtx.GetHash().SubString(),
                            "[", n, "] ", fCheckOnly? "repair not attempted" : "repairing");

                        ++nMismatchFound;
                        nBalanceInQuestion += wtx.vout[n].nValue;

                        if (!fCheckOnly)
                        {
                            wtx.MarkSpent(n);
                            wtx.WriteToDisk(map.first);

                            mapRepaired[map.first] = map.second;
                        }
                    }
                }
            }
        }

        /* Update map wallet if mismatched found. */
        if (nMismatchFound > 0 && mapRepaired.size() > 0)
        {
            RECURSIVE(cs_wallet);

            /* Update mapWallet with repaired transactions */
            for (const auto& map : mapRepaired)
                mapWallet[map.first] = map.second;
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


    /* Checks whether a transaction contains any outputs belonging to this wallet. */
    bool Wallet::IsMine(const TAO::Ledger::Transaction& tx)
    {
        /* Loop through the contracts. */
        for(uint32_t n = 0; n < tx.Size(); ++n)
        {
            /* Get legacy converted output.*/
            TxOut txout;
            if(!tx[n].Legacy(txout))
                continue;

            /* Check if is ours. */
            if(IsMine(txout))
                return true;
        }

        return false;
    }


     /* Checks whether a specific transaction input represents a send from this wallet. */
    bool Wallet::IsMine(const TxIn &txin)
    {
        {
            RECURSIVE(cs_wallet);

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
            RECURSIVE(cs_wallet);

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
        /* Check for null (tritium tx outputs will be null if not applicable to legacy). */
        if(txout.IsNull())
            return 0;

        if(!MoneyRange(txout.nValue))
            throw debug::exception(FUNCTION, "value out of range ", txout.nValue);

        return (IsMine(txout) ? txout.nValue : 0);
    }


    /* Returns the change amount for this wallet represented by a transaction output. */
    int64_t Wallet::GetChange(const TxOut& txout)
    {
        /* Check for null (tritium tx outputs will be null if not applicable to legacy). */
        if(txout.IsNull())
            return 0;

        if(!MoneyRange(txout.nValue))
            throw debug::exception(FUNCTION, "value out of range ", txout.nValue);

        return (IsChange(txout) ? txout.nValue : 0);
    }


    /* Checks whether a transaction output belongs to this wallet and
     *  represents change returned to it.
     */
    bool Wallet::IsChange(const TxOut& txout)
    {
        NexusAddress address;

        {
            RECURSIVE(cs_wallet);

            if(ExtractAddress(txout.scriptPubKey, address) && HaveKey(address))
            {
                return true;
            }
        }

        return false;
    }


    /* Generate a transaction to send balance to a given Nexus address. */
    std::string Wallet::SendToNexusAddress(const Script& scriptPubKey, const int64_t nValue, WalletTx& wtxNew,
                                            const bool fAskFee, const uint32_t nMinDepth, const bool fAdjust)
    {
        /* Validate amount */
        if(nValue <= 0)
            return std::string("Invalid amount");

        if(nValue < MIN_TXOUT_AMOUNT)
            return debug::safe_printstr("Send amount less than minimum of ", FormatMoney(MIN_TXOUT_AMOUNT), " NXS");

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

        int64_t nFeeRequired = TRANSACTION_FEE;
        int64_t nAmountSent = nValue;
        std::vector< std::pair<Script, int64_t> > vecSend;
        uint32_t nAttempts = 0;

        /* Key used by CreateTransaction for any change transaction and returned if not used, kept upon CommitTransaction */
        ReserveKey changeKey(*this);

        /* Will attempt up to 2 times, if need to adjust send amount for extra fee */
        while(true)
        {
            ++nAttempts;

            /* When applicable, adjust amount sent by the required fee */
            if(fAdjust)
                nAmountSent = nValue - nFeeRequired;

            /* Validate balance supports value + fees */
            if(nAmountSent + nFeeRequired > GetBalance())
                return std::string("Insufficient funds");

            /* Place the script and output amount into sending vector */
            vecSend.clear();
            vecSend.push_back(make_pair(scriptPubKey, nAmountSent));

            if(!CreateTransaction(vecSend, wtxNew, changeKey, nFeeRequired, nMinDepth))
            {
                /* If failed after a fee increase, try again with reduced amount sent */
                if(nAttempts < 2 && fAdjust && nFeeRequired > TRANSACTION_FEE)
                    continue;

                /* Transaction creation failed */
                std::string strError;
                if(nAmountSent + nFeeRequired > GetBalance())
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

                changeKey.ReturnKey();
                debug::log(0, FUNCTION, strError);

                return strError;
            }

            break;
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
            /* Check for minimum values. */
            if(s.second < MIN_TXOUT_AMOUNT)
                return false;

            nValue += s.second;

            /* Name the change address in the address book using from account  */
            NexusAddress address;
            if(!ExtractAddress(s.first, address) || !address.IsValid())
                continue;

            /* Check for accounts that don't exist to self. */
            if(HaveKey(address) && !Wallet::Instance().GetAddressBook().GetAddressBookMap().count(address))
            {
                /* Update address book entry. */
                Wallet::Instance().GetAddressBook().SetAddressBookName(address, "default");

                debug::log(0, FUNCTION, "Updated ", address.ToString(), " for send to self");
            }
        }

        {
            //RECURSIVE(cs_wallet);

            /* Link wallet to transaction, don't add to wallet yet (will be done when transaction committed) */
            wtxNew.BindWallet(this);

            /* Set fee to minimum for first loop iteration */
            nFeeRet = TRANSACTION_FEE;

            /* This loop is generally executed only once, unless the size of the transaction requires a fee increase.
             * When fee increased, it is possible that selected inputs do not cover it, so repeat the process to
             * assure we have enough value in. It also has to re-do the change calculation and output.
             */
            while(true)
            {
                /* Reset transaction contents */
                wtxNew.vin.clear();
                wtxNew.vout.clear();
                wtxNew.fFromMe = true;

                int64_t nTotalValue = nValue + nFeeRet;

                /* Add transactions outputs to vout */
                for(const auto& s : vecSend)
                    wtxNew.vout.push_back(TxOut(s.second, s.first));

                /* This set will hold txouts (UTXOs) to use as input for this transaction as transaction/vout index pairs */
                std::map<std::pair<uint512_t, uint32_t>, const WalletTx*> mapSelectedCoins;

                /* Initialize total value of all inputs */
                int64_t nValueIn = 0;

                /* Choose coins to use for transaction input */
                bool fSelectCoinsSuccessful = false;

                if(wtxNew.fromAddress.IsValid())
                {
                    /* Transaction sent from a specific address, so select only from that address */
                    fSelectCoinsSuccessful = SelectCoins(nTotalValue, wtxNew.nTime, mapSelectedCoins, nValueIn,
                                                         "*", wtxNew.fromAddress, nMinDepth);
                }
                else if(wtxNew.strFromAccount == "")
                {
                    /* No value for strFromAccount, so use wildcard for SelectCoins */
                    fSelectCoinsSuccessful = SelectCoins(nTotalValue, wtxNew.nTime, mapSelectedCoins, nValueIn,
                                                         "*", NexusAddress(), nMinDepth);
                }
                else
                {
                    /* RPC server calls do use strFromAccount, so pass this value forward */
                    fSelectCoinsSuccessful = SelectCoins(nTotalValue, wtxNew.nTime, mapSelectedCoins, nValueIn,
                                                         wtxNew.strFromAccount, NexusAddress(), nMinDepth);
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

                        /* Old accounting adds the change transaction back to default or send from account. */
                        if(wtxNew.nTime <= WALLET_ACCOUNTING_TIMELOCK)
                        {
                            /* Assign address book name for change key, or use "default" if blank or we received it with wildcard set */
                            if(wtxNew.strFromAccount == "" || wtxNew.strFromAccount == "*")
                                Wallet::Instance().GetAddressBook().SetAddressBookName(address, "default");
                            else
                                Wallet::Instance().GetAddressBook().SetAddressBookName(address, wtxNew.strFromAccount);
                        }
                    }
                    else
                    {
                        /* Change key not used by avatar mode */
                        changeKey.ReturnKey();

                        /* For avatar mode, return change to the last address retrieved by iterating the input set */
                        for(const auto& item : mapSelectedCoins)
                        {
                            /* When done, this will contain scriptPubKey of last transaction in the set */
                            scriptChange = item.second->vout[item.first.second].scriptPubKey;
                        }
                    }

                    /* Insert change output at random position: */
                    auto position = wtxNew.vout.begin() + LLC::GetRandInt(wtxNew.vout.size());
                    wtxNew.vout.insert(position, TxOut(nChange, scriptChange));
                }

                /* Fill vin with selected inputs */
                for(const auto& coin : mapSelectedCoins)
                    wtxNew.vin.push_back(TxIn(coin.first.first, coin.first.second));

                /* Sign inputs to unlock previously unspent outputs */
                uint32_t nIn = 0;
                for(const auto& coin : mapSelectedCoins)
                    if(!SignSignature(*this, (*coin.second), wtxNew, nIn++))
                        return false;

                /* Limit tx size to 20% of max block size */
                uint32_t nBytes = ::GetSerializeSize(*(Transaction*)&wtxNew, SER_NETWORK, LLP::PROTOCOL_VERSION);
                if(nBytes >= TAO::Ledger::MAX_BLOCK_SIZE_GEN/5)
                    return false; // tx size too large

                /* Each multiple of 1000 bytes of tx size multiplies the fee paid */
                int64_t nPayFee = TRANSACTION_FEE * (1 + (int64_t)nBytes / 1000);

                /* Get minimum required fee from transaction */
                int64_t nMinFee = wtxNew.GetMinFee(1, false, Legacy::GMF_SEND);

                /* Check that enough fee is included */
                int64_t nFeeRequired = std::max(nPayFee, nMinFee);
                if(nFeeRet < nFeeRequired)
                {
                    /* More fee required, so increase fee and repeat loop */
                    nFeeRet = nFeeRequired;
                    continue;
                }

                if(nChange == 0)
                {
                    /* No change for this transaction (only return key for no change after possible iteration for increased fee) */
                    changeKey.ReturnKey();
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
            RECURSIVE(cs_wallet);

            /* Mark old coins as spent */
            for (const TxIn& txin : wtxNew.vin)
            {
                WalletTx& txPrev = mapWallet[txin.prevout.hash];

                txPrev.BindWallet(this);
                txPrev.MarkSpent(txin.prevout.n);
                txPrev.WriteToDisk(wtxNew.GetHash()); //Stores to wallet database
            }
        }

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
        RECURSIVE(cs_wallet);

        const uint32_t nMinimumCoinAge = (config::fTestNet ? TAO::Ledger::MINIMUM_GENESIS_COIN_AGE_TESTNET : TAO::Ledger::MINIMUM_GENESIS_COIN_AGE);

        /* Keep a local list of wallet pointers. */
        std::vector<uint512_t> vCoins;

        /* Build a set of wallet transactions from all transaction in the wallet map */
        vCoins.reserve(mapWallet.size());
        for (const auto& item : mapWallet)
            vCoins.push_back(item.first);

        /* Randomly order the transactions as potential inputs */
        LLC::random_shuffle(vCoins.begin(), vCoins.end());

        /* Add Each Input to Transaction. */
        std::vector<const WalletTx*> vInputs;
        for(const uint512_t& hash : vCoins)
        {
            /* Get a reference from wallet map. */
            const WalletTx* wtx = &mapWallet[hash];

            /* Can't spend balance that is unconfirmed or not final */
            if (!wtx->IsFinal() || !wtx->IsConfirmed())
                continue;

            /* Can't spend coinbase or coinstake transactions that are immature */
            if ((wtx->IsCoinBase() || wtx->IsCoinStake()) && wtx->GetBlocksToMaturity() > 0)
                continue;

            /* Do not add coins to Genesis block if age less than trust timestamp */
            if (block.vtx[0].IsGenesis() && (block.vtx[0].nTime - wtx->nTime) < nMinimumCoinAge)
                continue;

            /* Do not add transaction from after Coinstake time */
            if (wtx->nTime > block.vtx[0].nTime)
                continue;

            /* Transaction is ok to use. Now determine which transaction outputs to include in coinstake input */
            for (uint32_t i = 0; i < wtx->vout.size(); i++)
            {
                /* Can't spend outputs that are already spent or not belonging to this wallet */
                if (wtx->IsSpent(i) || !IsMine(wtx->vout[i]))
                    continue;

                /* Stop adding Inputs if has reached Maximum Transaction Size. */
                unsigned int nBytes = ::GetSerializeSize(block.vtx[0], SER_NETWORK, LLP::PROTOCOL_VERSION);
                if(nBytes >= TAO::Ledger::MAX_BLOCK_SIZE_GEN / 5)
                    break;

                /* Add the inputs to coinstake. */
                block.vtx[0].vin.push_back(TxIn(hash, i));
                vInputs.push_back(wtx);

                /** Add the input value to the Coinstake output. **/
                block.vtx[0].vout[0].nValue += wtx->vout[i].nValue;
            }
        }
        if(block.vtx[0].vin.size() == 1)
            return false; // No transactions added

        /* Calculate the staking reward for the Coinstake Transaction. */
        uint64_t nStakeReward = 0;
        if(!block.vtx[0].CoinstakeReward(TAO::Ledger::BlockState(block), nStakeReward))
            return debug::error(FUNCTION, "Failed to calculate staking reward");

        /* Add the staking reward to the Coinstake output */
        block.vtx[0].vout[0].nValue += nStakeReward;

        /* Sign Each Input to Transaction. */
        for(uint32_t nIndex = 0; nIndex < vInputs.size(); nIndex++)
        {
            if (!SignSignature(*this, *vInputs[nIndex], block.vtx[0], nIndex + 1))
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
    bool Wallet::SelectCoins(const int64_t nTargetValue, const uint32_t nSpendTime,
        std::map<std::pair<uint512_t, uint32_t>, const WalletTx*>& mapCoinsRet,
        int64_t& nValueRet, const std::string& strAccount, const NexusAddress fromAddress , uint32_t nMinDepth)
    {
        /* Call detailed select up to 3 times if it fails, using the returns from the first successful call.
         * This allows it to attempt multiple input sets if it doesn't find a workable one on the first try.
         * (example, it chooses an input set with total value exceeding maximum allowed value)
         */
        return (SelectCoinsMinConf(nTargetValue, nSpendTime, nMinDepth, nMinDepth, mapCoinsRet, nValueRet, strAccount, fromAddress) ||
                SelectCoinsMinConf(nTargetValue, nSpendTime, nMinDepth, nMinDepth, mapCoinsRet, nValueRet, strAccount, fromAddress) ||
                SelectCoinsMinConf(nTargetValue, nSpendTime, nMinDepth, nMinDepth, mapCoinsRet, nValueRet, strAccount, fromAddress));
    }


    /* Selects the unspent outputs to use as inputs when creating a transaction to send
     * balance from this wallet while requiring a minimum confirmation depth to be included in result.
     */
    bool Wallet::SelectCoinsMinConf(const int64_t nTargetValue, const uint32_t nSpendTime,
        const uint32_t nConfMine, const uint32_t nConfTheirs,
        std::map<std::pair<uint512_t, uint32_t>, const WalletTx*>& mapCoinsRet,
        int64_t& nValueRet, const std::string& strAccount, const NexusAddress fromAddress)
    {
        /* cs_wallet should already be locked when this is called (CreateTransaction) */
        mapCoinsRet.clear();
        std::vector<uint512_t> vCoins;

        nValueRet = 0;

        if(config::GetBoolArg("-printselectcoin", false))
            debug::log(0, FUNCTION, "Selecting coins for account ", strAccount);

        /* Build a set of wallet transactions from all transaction in the wallet map */
        vCoins.reserve(mapWallet.size());
        for(const auto& item : mapWallet)
            vCoins.push_back(item.first);

        /* Randomly order the transactions as potential inputs */
        LLC::random_shuffle(vCoins.begin(), vCoins.end());

        /* Loop through all transactions, finding and adding available unspent balance to the list of outputs until reach nTargetValue */
        for(const uint512_t& hash : vCoins)
        {
            /* Get the transaction. */
            const WalletTx* wtx = &mapWallet[hash];

            /* Can't spend transaction from after spend time */
            if(wtx->nTime > nSpendTime)
                continue;

            /* Can't spend balance that is unconfirmed or not final */
            if(!wtx->IsFinal() || !wtx->IsConfirmed())
                continue;

            /* Can't spend transaction that has not reached minimum depth setting for mine/theirs */
            if(wtx->GetDepthInMainChain() < (wtx->IsFromMe() ? nConfMine : nConfTheirs))
                continue;

            /* Can't spend coinbase or coinstake transactions that are immature */
            if((wtx->IsCoinBase() || wtx->IsCoinStake()) && wtx->GetBlocksToMaturity() > 0)
                continue;

            /* So far, the transaction itself is available, now have to check each output to see if there are any we can use */
            for(uint32_t i = 0; i < wtx->vout.size(); i++)
            {
                /* Check for null values. */
                if(wtx->vout[i].IsNull())
                    continue;

                /* Can't spend outputs that are already spent or not belonging to this wallet */
                if(wtx->IsSpent(i) || !IsMine(wtx->vout[i]))
                    continue;

                /* Handle send from specific address here. */
                if(fromAddress.IsValid())
                {
                    NexusAddress address;
                    if(!ExtractAddress(wtx->vout[i].scriptPubKey, address) || !address.IsValid())
                        continue;

                    if(address == fromAddress)
                    {
                            /* Address for transaction matches request, include in result */
                            mapCoinsRet[std::make_pair(hash, i)] = wtx;
                            nValueRet += wtx->vout[i].nValue;
                    }
                }

                /* Handle account selection here. */
                else if(strAccount != "*")
                {
                    NexusAddress address;
                    if(!ExtractAddress(wtx->vout[i].scriptPubKey, address) || !address.IsValid())
                        continue;

                    if(Wallet::Instance().GetAddressBook().HasAddress(address))
                    {
                        std::string strEntry = Wallet::Instance().GetAddressBook().GetAddressBookName(address);
                        if(strEntry == "")
                            strEntry = "default";

                        if(strEntry == strAccount)
                        {
                            /* Account label for transaction address matches request, include in result */
                            mapCoinsRet[std::make_pair(hash, i)] = wtx;
                            nValueRet += wtx->vout[i].nValue;
                        }
                    }
                    else if(strAccount == "default")
                    {
                        /* Not in address book (no label), include if default requested */
                        mapCoinsRet[std::make_pair(hash, i)] = wtx;
                        nValueRet += wtx->vout[i].nValue;
                    }
                }

                /* Handle wildcard here, include all transactions. */
                else
                {
                    /* Add transaction with selected vout index to result set */
                    mapCoinsRet[std::make_pair(hash, i)] = wtx;

                    /* Accumulate total value available to spend in result set */
                    nValueRet += wtx->vout[i].nValue;
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
            for(const auto& item : mapCoinsRet)
                item.second->print();

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
