/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <Legacy/types/script.h>
#include <Legacy/types/transaction.h>

#include <Legacy/types/keypoolentry.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletdb.h>
#include <Legacy/types/walletkey.h>

#include <Legacy/types/account.h>
#include <Legacy/types/wallettx.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/convert.h>
#include <Util/include/filesystem.h>
#include <Util/include/runtime.h>

#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <utility>


namespace Legacy
{

    /* Initialize static values */
    const std::string WalletDB::DEFAULT_WALLET_DB("wallet.dat");


    /* Last time database was flushed. */
    std::atomic<uint32_t> WalletDB::nWalletDBUpdated(0);


    /* Background thread to handle flushing. */
    std::thread WalletDB::flushThread;


    /* Flag to set if flushing is started. */
    std::atomic<bool> WalletDB::fFlushThreadStarted(false);


    /* Flag for shutdown sequence. */
    std::atomic<bool> WalletDB::fShutdownFlushThread(false);


    /* Flag if flush thread is currently writing. */
    std::atomic<bool> WalletDB::fDbInProgress(false);


    /* Constructor */
    WalletDB::WalletDB()
    : strWalletFile(WalletDB::DEFAULT_WALLET_DB)
    {
    }


    /* Initializes database access for a given file name and access mode. */
    WalletDB::WalletDB(const std::string& strFileName)
    : strWalletFile(strFileName)
    {
    }


    /* Default Destructor */
    WalletDB::~WalletDB()
    {
    }


    /* Stores an encrypted master key into the database. */
    bool WalletDB::WriteMasterKey(const uint32_t nMasterKeyId, const MasterKey& kMasterKey)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Write(std::make_pair(std::string("mkey"), nMasterKeyId), kMasterKey, true);
    }


    /* Reads the minimum database version supported by this wallet database. */
    bool WalletDB::ReadMinVersion(uint32_t& nVersion)
    {
        return BerkeleyDB::GetInstance().Read(std::string("minversion"), nVersion);
    }


    /* Stores the minimum database version supported by this wallet database. */
    bool WalletDB::WriteMinVersion(const uint32_t nVersion)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Write(std::string("minversion"), nVersion, true);
    }


    /* Reads the update time for utxo account name changing */
    bool WalletDB::ReadUpdateTime(uint32_t &nTime)
    {
        return BerkeleyDB::GetInstance().Read(std::string("updated"), nTime);
    }


    /* Writes the updated time for utxo account name changing */
    bool WalletDB::WriteUpdateTime(const uint32_t nTime)
    {
        return BerkeleyDB::GetInstance().Write(std::string("updated"), nTime);
    }


    /* Reads the wallet account data associated with an account (Nexus address). */
    bool WalletDB::ReadAccount(const std::string& strAccount, Account& account)
    {
        account.SetNull();
        return BerkeleyDB::GetInstance().Read(std::make_pair(std::string("acc"), strAccount), account);
    }


    /* Stores the wallet account data for an address in the database. */
    bool WalletDB::WriteAccount(const std::string& strAccount, const Account& account)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Write(std::make_pair(std::string("acc"), strAccount), account);
    }


    /* Reads a logical name (label) for an address into the database. */
    bool WalletDB::ReadName(const std::string& strAddress, std::string& strName)
    {
        strName = "";
        return BerkeleyDB::GetInstance().Read(std::make_pair(std::string("name"), strAddress), strName);
    }


    /* Stores a logical name (label) for an address in the database. */
    bool WalletDB::WriteName(const std::string& strAddress, const std::string& strName)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Write(std::make_pair(std::string("name"), strAddress), strName);
    }


    /* Removes the name entry associated with an address. */
    bool WalletDB::EraseName(const std::string& strAddress)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Erase(std::make_pair(std::string("name"), strAddress));
    }


    /* Reads the default public key from the wallet database. */
    bool WalletDB::ReadDefaultKey(std::vector<uint8_t>& vchPubKey)
    {
        vchPubKey.clear();
        return BerkeleyDB::GetInstance().Read(std::string("defaultkey"), vchPubKey);
    }


    /* Stores the default public key to the wallet database. */
    bool WalletDB::WriteDefaultKey(const std::vector<uint8_t>& vchPubKey)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Write(std::string("defaultkey"), vchPubKey);
    }


    /* Reads the public key for the wallet's trust key from the wallet database. */
    bool WalletDB::ReadTrustKey(std::vector<uint8_t>& vchPubKey)
    {
        vchPubKey.clear();
        return BerkeleyDB::GetInstance().Read(std::string("trustkey"), vchPubKey);
    }


    /* Stores the public key for the wallet's trust key to the wallet database. */
    bool WalletDB::WriteTrustKey(const std::vector<uint8_t>& vchPubKey)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Write(std::string("trustkey"), vchPubKey);
    }


    /* Removes the trust key entry. */
    bool WalletDB::EraseTrustKey()
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Erase(std::string("trustkey"));
    }


    /* Reads the unencrypted private key associated with a public key */
    bool WalletDB::ReadKey(const std::vector<uint8_t>& vchPubKey, LLC::CPrivKey& vchPrivKey)
    {
        vchPrivKey.clear();
        return BerkeleyDB::GetInstance().Read(std::make_pair(std::string("key"), vchPubKey), vchPrivKey);
    }


    /* Stores an unencrypted private key using the corresponding public key. */
    bool WalletDB::WriteKey(const std::vector<uint8_t>& vchPubKey, const LLC::CPrivKey& vchPrivKey)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Write(std::make_pair(std::string("key"), vchPubKey), vchPrivKey, false);
    }


    /* Stores an encrypted private key using the corresponding public key. */
    bool WalletDB::WriteCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret, bool fEraseUnencryptedKey)
    {
        ++WalletDB::nWalletDBUpdated;
        BerkeleyDB& db = BerkeleyDB::GetInstance();

        if(!db.Write(std::make_pair(std::string("ckey"), vchPubKey), vchCryptedSecret, false))
            return false;

        if(fEraseUnencryptedKey)
        {
            db.Erase(std::make_pair(std::string("key"), vchPubKey));
            db.Erase(std::make_pair(std::string("wkey"), vchPubKey));
        }
        return true;
    }


    /* Reads the wallet transaction for a given transaction hash. */
    bool WalletDB::ReadTx(const uint512_t& hash, WalletTx& wtx)
    {
        return BerkeleyDB::GetInstance().Read(std::make_pair(std::string("tx"), hash), wtx);
    }


    /* Stores a wallet transaction using its transaction hash. */
    bool WalletDB::WriteTx(const uint512_t& hash, const WalletTx& wtx)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Write(std::make_pair(std::string("tx"), hash), wtx);
    }


    /* Removes the wallet transaction associated with a transaction hash. */
    bool WalletDB::EraseTx(const uint512_t& hash)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Erase(std::make_pair(std::string("tx"), hash));
    }


    /* Reads the script for a given script hash. */
    bool WalletDB::ReadScript(const uint256_t& hash, Script& redeemScript)
    {
        redeemScript.clear();
        return BerkeleyDB::GetInstance().Read(std::make_pair(std::string("cscript"), hash), redeemScript);
    }


    /* Stores a redeem script using its script hash. */
    bool WalletDB::WriteScript(const uint256_t& hash, const Script& redeemScript)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Write(std::make_pair(std::string("cscript"), hash), redeemScript, false);
    }


    /* Reads a key pool entry from the database. */
    bool WalletDB::ReadPool(const uint64_t nPool, KeyPoolEntry& keypoolEntry)
    {
        return BerkeleyDB::GetInstance().Read(std::make_pair(std::string("pool"), nPool), keypoolEntry);
    }


    /* Stores a key pool entry using its pool entry number (ID value). */
    bool WalletDB::WritePool(const uint64_t nPool, const KeyPoolEntry& keypoolEntry)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Write(std::make_pair(std::string("pool"), nPool), keypoolEntry);
    }


    /* Removes a key pool entry associated with a pool entry number. */
    bool WalletDB::ErasePool(const uint64_t nPool)
    {
        ++WalletDB::nWalletDBUpdated;
        return BerkeleyDB::GetInstance().Erase(std::make_pair(std::string("pool"), nPool));
    }


    /* Initializes WalletDB for database access to the database file. */
    void WalletDB::InitializeDatabase()
    {
        BerkeleyDB::GetInstance(strWalletFile);
    }


    /* Initializes a wallet instance from the data in this wallet database. */
    uint32_t WalletDB::LoadWallet(Wallet& wallet)
    {
        uint32_t nFileVersion = 0;
        uint64_t nStartTimestamp = runtime::timestamp(true);
        bool fIsEncrypted = false;
        uint32_t nRet = 0;

        std::vector<uint512_t> vRemove;

        /* Flush thread shouldn't be running while we load, but ensure it doesn't try to flush during load if it is.
         * This will atomically set flag to true if it is currently false as expected, otherwise it returns false and wait loop executes.
         *
         * Although the primary purpose is to suspend flush thread operations, this flag also acts as a "soft lock" token for any method
         * that performs database operations requiring multiple steps. BerkeleyDB locks during each individual database call, but not
         * across calls that use cursors or db transactions. Holding this flag forces other multi-step operations to wait
         * for it to become available.
         *
         * compare_exchange_weak expects us to pass references that match the atomic type, not literals, so we have to define them.
         * If the actual value is not the expected value (ie, true instead of false), it stores the actual in the expected before it
         * returns, so loop must set it false again before next check.
         *
         * Weak form is used because it is faster, and its possibility of a spurious fail is ok. We are already checking in a loop.
         */
        bool fExpectedValue = false;
        bool fDesiredValue = true;
        while (!WalletDB::fDbInProgress.compare_exchange_weak(fExpectedValue, fDesiredValue))
        {
            runtime::sleep(100);
            fExpectedValue = false;
        }

        BerkeleyDB& db = BerkeleyDB::GetInstance();

        /* Reset default key into wallet to clear any current value. (done now so it stays empty if none loaded) */
        wallet.vchDefaultKey.clear();

        /* Read and validate minversion required by database file */
        uint32_t nMinVersion = 0;
        if(ReadMinVersion(nMinVersion))
        {
            if(nMinVersion > FEATURE_LATEST)
            {
                WalletDB::fDbInProgress.store(false); //reset before return
                return DB_TOO_NEW;
            }

            wallet.LoadMinVersion(nMinVersion);
        }

        /* Handle accounting timelock. */
        if(!ReadUpdateTime(WALLET_ACCOUNTING_TIMELOCK))
        {
            /* Set the accounting time-lock. */
            WALLET_ACCOUNTING_TIMELOCK = (runtime::timestamp() - 299);
            WriteUpdateTime(WALLET_ACCOUNTING_TIMELOCK);

            debug::log(0, FUNCTION, "Time-Lock set for accounting upgrade at ", WALLET_ACCOUNTING_TIMELOCK);
        }
        else
        {
            debug::log(0, FUNCTION, "Wallet accounting set at ", WALLET_ACCOUNTING_TIMELOCK);
        }


        /* Upgrade wallet transactions. */
        bool fUpgrade = (nMinVersion != FEATURE_BASE);
        if(fUpgrade) //this is forcing upgrade with FEATURE_BASE
        {
            /* Upgrade the versions. */
            nMinVersion = FEATURE_BASE;
            WriteMinVersion(nMinVersion);
            wallet.LoadMinVersion(nMinVersion);

            debug::log(0, FUNCTION, "Upgrading Minimum Database Version to ", nMinVersion);
        }


        /* Debug output for walletcheck. */
        else if(config::GetBoolArg("-walletcheck", true))
            debug::log(0, FUNCTION, "Checking transactions for consistency");

        /* Get cursor */
        auto pcursor = db.GetCursor();
        if(pcursor == nullptr)
        {
            debug::error(FUNCTION, "Error opening wallet database cursor");
            WalletDB::fDbInProgress.store(false); //reset before return
            return DB_CORRUPT;
        }

        /* Loop to read all entries from wallet database */
        while(true)
        {
            /* Read next record */
            DataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
            DataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);

            nRet = db.ReadAtCursor(pcursor, ssKey, ssValue);

            if(nRet == DB_NOTFOUND)
            {
                /* End of database, no more entries to read */
                break;
            }
            else if(nRet != 0)
            {
                debug::error(FUNCTION, "Error reading next record from wallet database");
                nRet = DB_CORRUPT;
                break;
            }

            /* Unserialize
             * Taking advantage of the fact that pair serialization
             * is just the two items serialized one after the other
             */
            std::string strType;
            ssKey >> strType;

            if(strType == "name")
            {
                /* Address book entry */
                std::string strNexusAddress;
                std::string strAddressLabel;

                /* Extract the Nexus address from the name key */
                ssKey >> strNexusAddress;
                NexusAddress address(strNexusAddress);

                /* Value is the address label */
                ssValue >> strAddressLabel;

                wallet.GetAddressBook().mapAddressBook[address] = strAddressLabel;

            }

            else if(strType == "tx")
            {
                /* Wallet transaction */
                uint512_t hash;
                ssKey >> hash;

                /* Upgrade wallet. */
                if(fUpgrade || config::GetBoolArg("-walletclean", false))
                {
                    vRemove.push_back(hash);
                    continue;
                }

                /* If no upgrade deserialize tx's like normal. */
                WalletTx& wtx = wallet.mapWallet[hash];
                ssValue >> wtx;

                /* flag to indicate we are ok to bind */
                bool fBind = true;

                /* Check transactions for consistency. */
                if(config::GetBoolArg("-walletcheck", true))
                {
                    /* Flag to track if transaction switched to tritium. */
                    bool fTritium = false;

                    /* Check for tritium hash type, but fall through to legacy if this fails. */
                    if(hash.GetType() == TAO::Ledger::TRITIUM)
                    {
                        /* Read the transaction from ledger db. */
                        TAO::Ledger::Transaction tx;
                        if(LLD::Ledger->ReadTx(hash, tx))
                        {
                            /* Convert the disk transaction into WalletTx. Wallet db stores OP::LEGACY wtx using tritium hash,
                             * but calling wtx.GetHash() will return legacy hash, so have to convert first to do a valid check.
                             */
                            Transaction ltx(tx);
                            WalletTx wtx2(&wallet, ltx);

                            /* Check converted hash from tritium disk tx to current wallet cache. */
                            if(wtx.GetHash() != wtx2.GetHash())
                            {
                                /* Add mismatched transaction to list of transactions to remove from database */
                                debug::error(FUNCTION, "Error in ", strWalletFile, ", Tritium hash mismatch, resolving");
                                vRemove.push_back(hash);

                                continue;
                            }

                            /* Set tritium flag. */
                            fTritium = true;
                        }
                    }

                    /* Check wtx hash against stored hash for corruption. */
                    if(!fTritium && LLD::Legacy->HasTx(hash) && wtx.GetHash() != hash)
                    {
                        /* Add mismatched transaction to list of transactions to remove from database */
                        debug::error(FUNCTION, "Error in ", strWalletFile, ", Legacy hash mismatch, resolving");
                        vRemove.push_back(hash);

                        continue;
                    }
                }

                /* Bind to wallet if valid. */
                if(fBind)
                    wtx.BindWallet(wallet);
            }

            else if(strType == "defaultkey")
            {
                /* Wallet default key */
                ssValue >> wallet.vchDefaultKey;

            }

            else if(strType == "trustkey")
            {
                /* Wallet trust key public key */
                ssValue >> wallet.vchTrustKey;

            }

            else if(strType == "mkey")
            {
                /* Wallet master key */
                uint32_t nMasterKeyId;
                ssKey >> nMasterKeyId;
                MasterKey kMasterKey;
                ssValue >> kMasterKey;

                /* Load the master key into the wallet */
                if(!wallet.LoadMasterKey(nMasterKeyId, kMasterKey))
                {
                    debug::error(FUNCTION, "Error reading wallet database: duplicate MasterKey id ", nMasterKeyId);
                    nRet = DB_CORRUPT;
                    break;
                }

            }

            else if(strType == "key" || strType == "wkey")
            {
                /* Unencrypted key */
                std::vector<uint8_t> vchPubKey;
                ssKey >> vchPubKey;

                LLC::ECKey key;
                if(strType == "key")
                {
                    /* key entry stores unencrypted private key */
                    LLC::CPrivKey privateKey;
                    ssValue >> privateKey;
                    key.SetPubKey(vchPubKey);
                    key.SetPrivKey(privateKey);

                    /* Validate the key data */
                    if(key.GetPubKey() != vchPubKey)
                    {
                        debug::error(FUNCTION, "Error reading wallet database: CPrivKey pubkey inconsistency");
                        nRet = DB_CORRUPT;
                        break;
                    }

                    if(!key.IsValid())
                    {
                        debug::error(FUNCTION, "Error reading wallet database: invalid CPrivKey");
                        nRet = DB_CORRUPT;
                        break;
                    }
                }
                else
                {
                    /* wkey entry stores WalletKey
                     * These are no longer written to database, but support to read them is included for backward compatability
                     * Loaded into wallet key, same as key entry
                     */
                    WalletKey wkey;
                    ssValue >> wkey;
                    key.SetPubKey(vchPubKey);
                    key.SetPrivKey(wkey.vchPrivKey);

                    /* Validate the key data  */
                    if(key.GetPubKey() != vchPubKey)
                    {
                        debug::error(FUNCTION, "Error reading wallet database: WalletKey pubkey inconsistency");
                        nRet = DB_CORRUPT;
                        break;
                    }

                    if(!key.IsValid())
                    {
                        debug::error(FUNCTION, "Error reading wallet database: invalid WalletKey");
                        nRet = DB_CORRUPT;
                        break;
                    }
                }

                /* Load the key into the wallet */
                if(!wallet.LoadKey(key))
                {
                    debug::error(FUNCTION, "Error reading wallet database: LoadKey failed");
                    nRet = DB_CORRUPT;
                    break;
                }

            }

            else if(strType == "ckey")
            {
                /* Encrypted key */
                std::vector<uint8_t> vchPubKey;
                ssKey >> vchPubKey;
                std::vector<uint8_t> vchPrivKey;
                ssValue >> vchPrivKey;

                if(!wallet.LoadCryptedKey(vchPubKey, vchPrivKey))
                {
                    debug::error(FUNCTION, "Error reading wallet database: LoadCryptedKey failed");
                    nRet = DB_CORRUPT;
                    break;
                }

                /* If any one key entry is encrypted, treat the entire wallet as encrypted */
                fIsEncrypted = true;

            }

            else if(strType == "pool")
            {
                /* Key pool entry */
                uint64_t nPoolIndex;
                ssKey >> nPoolIndex;

                /* Only the pool index is stored in the key pool */
                /* Key pool entry will be read from the database as needed */
                wallet.GetKeyPool().setKeyPool.insert(nPoolIndex);

            }

            else if(strType == "version")
            {
                /* Wallet database file version */
                ssValue >> nFileVersion;

            }

            else if(strType == "cscript")
            {
                /* Script */
                uint256_t hash;
                ssKey >> hash;
                Script script;
                ssValue >> script;

                if(!wallet.LoadScript(script))
                {
                    debug::error(FUNCTION, "Error reading wallet database: LoadScript failed");
                    nRet =  DB_CORRUPT;
                    break;
                }

            }

            else if(strType == "acentry")
            {
                /* Accounting entry */
                std::string strAccount;
                ssKey >> strAccount;
                uint64_t nNumber;
                ssKey >> nNumber;

            }

            else
            {
                /* All other keys are no longer supported and can be ignored/not loaded */
            }
        }

        db.CloseCursor(pcursor);

        /* If load has completed normally, nRet will equal DB_NOT_FOUND because it read all records to the end.
         * Any other value is an error code. Skip remaining processing and return the error.
         */
        if(nRet == DB_NOTFOUND)
        {
            /* Remove transactions flagged for removal */
            if(vRemove.size() > 0)
            {
                /* Debug output. */
                debug::log(0, FUNCTION, "Erasing ", vRemove.size(), " Transactions from WalletDB");

                /* Erase the flagged transactions. */
                for(const auto& hash : vRemove)
                {
                    EraseTx(hash);
                    wallet.mapWallet.erase(hash);
                    ++nWalletDBUpdated;
                }

                nRet = DB_NEEDS_RESCAN; // Will return this on successful completion
            }
            else
                nRet = DB_LOAD_OK; // Will return this on successful completion

            /* Update file version to latest version */
            if(nFileVersion < LLD::DATABASE_VERSION)
                db.WriteVersion(LLD::DATABASE_VERSION);

            uint64_t nElapsed = runtime::timestamp(true) - nStartTimestamp;
            debug::log(0, FUNCTION, "", fIsEncrypted ? "Encrypted Wallet" : "Wallet", " Loaded in ", nElapsed, " ms file version = ", nFileVersion);
        }

        /* Ok to flush again */
        WalletDB::fDbInProgress.store(false);

        return nRet;
    }


    /* Converts the database from unencrypted to encrypted. */
    bool WalletDB::EncryptDatabase(const uint32_t nNewMasterKeyId, const MasterKey& kMasterKey, const CryptedKeyMap& mapNewEncryptedKeys)
    {
        bool fSuccessful = true;

        /* Don't flush during encryption transaction. See LoadWallet() for discussion of this code. */
        bool fExpectedValue = false;
        bool fDesiredValue = true;
        while (!WalletDB::fDbInProgress.compare_exchange_weak(fExpectedValue, fDesiredValue))
        {
            runtime::sleep(100);
            fExpectedValue = false;
        }

        BerkeleyDB& db = BerkeleyDB::GetInstance();

        if(!db.TxnBegin())
        {
            debug::error(FUNCTION, "Unable to begin database transaction for writing encrypted keys to ", strWalletFile);
            return false;
        }

            /* Start encryption transaction by writing the master key */
        if(!WriteMasterKey(nNewMasterKeyId, kMasterKey))
        {
            debug::error(FUNCTION, "Error writing master key to ", strWalletFile);
            fSuccessful = false;
        }

        if(fSuccessful)
        {
            for(const auto& mKey : mapNewEncryptedKeys)
            {
                const std::vector<uint8_t>& vchPubKey = mKey.second.first;
                const std::vector<uint8_t>& vchCryptedSecret = mKey.second.second;

                if(!WriteCryptedKey(vchPubKey, vchCryptedSecret, true))
                {
                    debug::error(FUNCTION, "Error writing encrypted key to ", strWalletFile);
                    fSuccessful = false;
                }
            }
        }

        if(fSuccessful)
        {
            if(!db.TxnCommit())
            {
                debug::error(FUNCTION, "Error committing encryption updates to ", strWalletFile);
                fSuccessful = false;
            }
        }

        if(fSuccessful)
            db.DBFlush();
        else
            db.TxnAbort();

        /* Ok to flush again. */
        WalletDB::fDbInProgress.store(false);

        return fSuccessful;
    }


    /* Rewrites the backing database by copying all contents. */
    bool WalletDB::DBRewrite()
    {
        return BerkeleyDB::GetInstance().DBRewrite();
    }


    /* Start the wallet flush thread for a given wallet file. */
    bool WalletDB::StartFlushThread(const std::string& strFile)
    {
        if(WalletDB::fFlushThreadStarted.load())
        {
            debug::error(FUNCTION, "Attempt to start a second wallet flush thread for file ", strFile);
            return false;
        }

        if(config::GetBoolArg("-flushwallet", true))
        {
            WalletDB::flushThread = std::thread(WalletDB::ThreadFlushWalletDB);
            WalletDB::fFlushThreadStarted.store(true);
        }

        return true;
    }


    /* Signals the wallet flush thread to shut down. */
    void WalletDB::ShutdownFlushThread()
    {
        if(config::GetBoolArg("-flushwallet", true))
        {
            WalletDB::fShutdownFlushThread.store(true);

            if(WalletDB::flushThread.joinable())
                WalletDB::flushThread.join();
        }
    }


    /* Function that loops until shutdown and periodically flushes a wallet db */
    void WalletDB::ThreadFlushWalletDB()
    {
        if(!config::GetBoolArg("-flushwallet", true))
            return;

        /* WalletDB::nWalletDBUpdated is incremented each time wallet database data is updated'
         * Generally, we want to flush the database any time this value has changed since the last iteration'
         * However, we don't want to do that if it is too soon after the update (to allow for possible series of updates).
         *
         * Therefore, each time the WalletDB::nWalletDBUpdated has been updated, we record the timestamp and nLastSeen.
         * Then, when WalletDB::nWalletDBUpdated no longer equals nLastFlushed AND enough time has passed, flush is performed.
         * In this manner, if there is a series of (possibly related) updates in a short timespan, they will all be flushed together.
         */
        const int64_t minTimeSinceLastUpdate = 2;

        uint32_t nLastSeen = WalletDB::nWalletDBUpdated.load();
        uint32_t nLastFlushed = 0;
        uint64_t nLastWalletUpdate = runtime::unifiedtimestamp();

        debug::log(1, FUNCTION, "Wallet flush thread started");

        while(!WalletDB::fShutdownFlushThread.load())
        {
            runtime::sleep(1000);

            /* Check for shutdown. */
            if(WalletDB::fShutdownFlushThread.load() && nLastFlushed == nLastSeen)
                break;

            /* Skip flush if multi-step DB operation (cursor, transaction, etc.) in progress. Wait for it to complete */
            if(WalletDB::fDbInProgress.load())
                continue;

            /* Copy the atomic value to a local variable so we can use it without any chance that it will change */
            const uint32_t nWalletUpdatedCount = WalletDB::nWalletDBUpdated.load();
            if(nLastSeen != nWalletUpdatedCount)
            {
                /* Database is updated since last checked. Record time update recognized */
                nLastSeen = nWalletUpdatedCount;
                nLastWalletUpdate = runtime::unifiedtimestamp();
            }

            /* Perform flush if database updated, and the minimum required time has passed since recognizing the update */
            if(nLastFlushed != nLastSeen && (runtime::unifiedtimestamp() - nLastWalletUpdate) >= minTimeSinceLastUpdate)
            {

                /* If fDbInProgress currently false, atomically set it to true and performs flush.
                 * Otherwise, value has changed since the check above. Skip flush this iteration.
                 * Use strong compare here instead of weak to avoid possible spurious fail that would require an unnecessary loop iteration.
                 */
                bool fExpectedValue = false;
                bool fDesiredValue = true;
                if (WalletDB::fDbInProgress.compare_exchange_strong(fExpectedValue, fDesiredValue))
                {
                    BerkeleyDB::GetInstance().DBFlush();
                    nLastFlushed = nLastSeen;
                    WalletDB::fDbInProgress.store(false);
                }
            }
        }

        debug::log(0, FUNCTION, "Shutting down wallet flush thread");

        /* Reset flags */
        WalletDB::fShutdownFlushThread.store(false);
        WalletDB::fFlushThreadStarted.store(false);
    }


    /* Writes a backup copy of a given wallet to a designated backup file */
    bool WalletDB::BackupWallet(const Wallet& wallet, const std::string& strDest)
    {
        if(!wallet.IsFileBacked())
            return false;

        /* Validate the length of strDest. This assures pathDest.size() cast to uint32_t is always valid (nobody can pass a ridiculously long string) */
        if(strDest.size() > MAX_PATH)
        {
            debug::error(FUNCTION, "Error: Invalid destination path. Path size exceeds maximum limit");
            return false;
        }

        bool fBackupSuccessful = false;

        while(!config::fShutdown.load()) //Loop used so we can easily break to the end on error. Only iterates once.
        {
            /* Tell flush thread not to flush during backup (backup will flush below). See LoadWallet() for discussion of this code. */
            bool fExpectedValue = false;
            bool fDesiredValue = true;
            while (!WalletDB::fDbInProgress.compare_exchange_weak(fExpectedValue, fDesiredValue))
            {
                runtime::sleep(100);
                fExpectedValue = false;
            }

            if(config::fShutdown.load()) //Just in case we had to wait and it changed
                break;

            std::string strSource = wallet.GetWalletFile();

            std::string pathSource(config::GetDataDir() + strSource);
            std::string pathDest(strDest);

            /* Create any missing directories in the destination path.
             * If final character in pathDest is not / then the last entry in path is not created (could be a file name)
             */
            filesystem::create_directories(pathDest);

                /* If destination is a folder, append source file name to use as dest file name */
            if(filesystem::is_directory(pathDest))
            {
                uint32_t s = static_cast<uint32_t>(pathDest.size());

                if(pathDest[s-1] != '/')
                    pathDest += '/';

                pathDest = pathDest + strSource;

                /* After appending to pathDest, need to validate again */
                if(strDest.size() > MAX_PATH) {
                    debug::error(FUNCTION, "Error: Invalid destination path. Path size exceeds maximum limit");
                    fBackupSuccessful =  false;
                    break;
                }
            }

            /* Flush and detach the source file before we copy it */
            BerkeleyDB::GetInstance().DBFlush();

            /* Copy wallet file from source to dest */
            if(filesystem::copy_file(pathSource, pathDest))
            {
                debug::log(0, FUNCTION, "Successfully backed up ", strSource, " to ", pathDest);
                fBackupSuccessful = true;
                break;
            }
            else
            {
                debug::error(FUNCTION, "Error copying ", strSource, " to ", pathDest);
                fBackupSuccessful = false;
                break;
            }

        }

        /* Ok to flush again */
        WalletDB::fDbInProgress.store(false);

        return fBackupSuccessful;
    }

}
