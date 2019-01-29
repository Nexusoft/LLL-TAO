/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <utility>

#include <LLD/include/version.h>

#include <Legacy/types/script.h>
#include <Legacy/wallet/accountingentry.h>
#include <Legacy/wallet/keypoolentry.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/wallet/walletaccount.h>
#include <Legacy/wallet/walletdb.h>
#include <Legacy/wallet/walletkey.h>
#include <Legacy/wallet/wallettx.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/convert.h>
#include <Util/include/filesystem.h>
#include <Util/include/runtime.h>


namespace Legacy
{

    /* Initialize static values */
    const std::string CWalletDB::DEFAULT_WALLET_DB("wallet.dat");

    uint32_t CWalletDB::nWalletDBUpdated = 0;

    uint64_t CWalletDB::nAccountingEntryNumber = 0;

    std::mutex CWalletDB::cs_walletdb;


    /* Stores an encrypted master key into the database. */
    bool CWalletDB::WriteMasterKey(const uint32_t nMasterKeyId, const CMasterKey& kMasterKey)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Write(std::make_pair(std::string("mkey"), nMasterKeyId), kMasterKey, true);
    }


    /* Reads the minimum database version supported by this wallet database. */
    bool CWalletDB::ReadMinVersion(uint32_t& nVersion)
    {
        LOCK(CWalletDB::cs_walletdb);
        return Read(std::string("minversion"), nVersion);
    }


    /* Stores the minimum database version supported by this wallet database. */
    bool CWalletDB::WriteMinVersion(const uint32_t nVersion)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Write(std::string("minversion"), nVersion, true);
    }


    /* Reads the wallet account data associated with an account (Nexus address). */
    bool CWalletDB::ReadAccount(const std::string& strAccount, CAccount& account)
    {
        LOCK(CWalletDB::cs_walletdb);
        account.SetNull();
        return Read(std::make_pair(std::string("acc"), strAccount), account);
    }


    /* Stores the wallet account data for an address in the database. */
    bool CWalletDB::WriteAccount(const std::string& strAccount, const CAccount& account)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Write(std::make_pair(std::string("acc"), strAccount), account);
    }


    /* Reads a logical name (label) for an address into the database. */
    bool CWalletDB::ReadName(const std::string& strAddress, std::string& strName)
    {
        LOCK(CWalletDB::cs_walletdb);
        strName = "";
        return Read(std::make_pair(std::string("name"), strAddress), strName);
    }


    /* Stores a logical name (label) for an address in the database. */
    bool CWalletDB::WriteName(const std::string& strAddress, const std::string& strName)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Write(std::make_pair(std::string("name"), strAddress), strName);
    }


    /* Removes the name entry associated with an address. */
    bool CWalletDB::EraseName(const std::string& strAddress)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Erase(std::make_pair(std::string("name"), strAddress));
    }


    /* Reads the default public key from the wallet database. */
    bool CWalletDB::ReadDefaultKey(std::vector<uint8_t>& vchPubKey)
    {
        LOCK(CWalletDB::cs_walletdb);
        vchPubKey.clear();
        return Read(std::string("defaultkey"), vchPubKey);
    }


    /* Stores the default public key to the wallet database. */
    bool CWalletDB::WriteDefaultKey(const std::vector<uint8_t>& vchPubKey)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Write(std::string("defaultkey"), vchPubKey);
    }


    /* Reads the public key for the wallet's trust key from the wallet database. */
    bool CWalletDB::ReadTrustKey(std::vector<uint8_t>& vchPubKey)
    {
        LOCK(CWalletDB::cs_walletdb);
        vchPubKey.clear();
        return Read(std::string("trustkey"), vchPubKey);
    }


    /* Stores the public key for the wallet's trust key to the wallet database. */
    bool CWalletDB::WriteTrustKey(const std::vector<uint8_t>& vchPubKey)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Write(std::string("trustkey"), vchPubKey);
    }


    /* Removes the trust key entry. */
    bool CWalletDB::EraseTrustKey()
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Erase("trustkey");
    }


    /* Reads the unencrypted private key associated with a public key */
    bool CWalletDB::ReadKey(const std::vector<uint8_t>& vchPubKey, LLC::CPrivKey& vchPrivKey)
    {
        LOCK(CWalletDB::cs_walletdb);
        vchPrivKey.clear();
        return Read(std::make_pair(std::string("key"), vchPubKey), vchPrivKey);
    }


    /* Stores an unencrypted private key using the corresponding public key. */
    bool CWalletDB::WriteKey(const std::vector<uint8_t>& vchPubKey, const LLC::CPrivKey& vchPrivKey)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Write(std::make_pair(std::string("key"), vchPubKey), vchPrivKey, false);
    }


    /* Stores an encrypted private key using the corresponding public key. */
    bool CWalletDB::WriteCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret, bool fEraseUnencryptedKey)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        if (!Write(std::make_pair(std::string("ckey"), vchPubKey), vchCryptedSecret, false))
            return false;

        if (fEraseUnencryptedKey)
        {
            Erase(std::make_pair(std::string("key"), vchPubKey));
            Erase(std::make_pair(std::string("wkey"), vchPubKey));
        }
        return true;
    }


    /* Reads the wallet transaction for a given transaction hash. */
    bool CWalletDB::ReadTx(const uint512_t& hash, CWalletTx& wtx)
    {
        LOCK(CWalletDB::cs_walletdb);
        return Read(std::make_pair(std::string("tx"), hash), wtx);
    }


    /* Stores a wallet transaction using its transaction hash. */
    bool CWalletDB::WriteTx(const uint512_t& hash, const CWalletTx& wtx)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Write(std::make_pair(std::string("tx"), hash), wtx);
    }


    /* Removes the wallet transaction associated with a transaction hash. */
    bool CWalletDB::EraseTx(const uint512_t& hash)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Erase(std::make_pair(std::string("tx"), hash));
    }


    /* Reads the script for a given script hash. */
    bool CWalletDB::ReadCScript(const uint256_t& hash, CScript& redeemScript)
    {
        LOCK(CWalletDB::cs_walletdb);
        redeemScript.clear();
        return Read(std::make_pair(std::string("cscript"), hash), redeemScript);
    }


    /* Stores a redeem script using its script hash. */
    bool CWalletDB::WriteCScript(const uint256_t& hash, const CScript& redeemScript)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Write(std::make_pair(std::string("cscript"), hash), redeemScript, false);
    }


    /* Reads a key pool entry from the database. */
    bool CWalletDB::ReadPool(const uint64_t nPool, CKeyPoolEntry& keypoolEntry)
    {
        LOCK(CWalletDB::cs_walletdb);
        return Read(std::make_pair(std::string("pool"), nPool), keypoolEntry);
    }


    /* Stores a key pool entry using its pool entry number (ID value). */
    bool CWalletDB::WritePool(const uint64_t nPool, const CKeyPoolEntry& keypoolEntry)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Write(std::make_pair(std::string("pool"), nPool), keypoolEntry);
    }


    /* Removes a key pool entry associated with a pool entry number. */
    bool CWalletDB::ErasePool(const uint64_t nPool)
    {
        LOCK(CWalletDB::cs_walletdb);
        CWalletDB::nWalletDBUpdated++;
        return Erase(std::make_pair(std::string("pool"), nPool));
    }


    /* Stores an accounting entry in the wallet database. */
    bool CWalletDB::WriteAccountingEntry(const CAccountingEntry& acentry)
    {
        LOCK(CWalletDB::cs_walletdb);
        return Write(std::make_tuple(std::string("acentry"), acentry.strAccount, ++CWalletDB::nAccountingEntryNumber), acentry);
    }


    /* Retrieves the net total of all accounting entries for an account (Nexus address). */
    int64_t CWalletDB::GetAccountCreditDebit(const std::string& strAccount)
    {
        std::list<CAccountingEntry> entries;
        ListAccountCreditDebit(strAccount, entries);

        int64_t nCreditDebitTotal = 0;
        for(CAccountingEntry& entry : entries)
            nCreditDebitTotal += entry.nCreditDebit;

        return nCreditDebitTotal;
    }


    /* Retrieves a list of individual accounting entries for an account (Nexus address) */
    void CWalletDB::ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& entries)
    {
        LOCK(CWalletDB::cs_walletdb);
        bool fAllAccounts = (strAccount == "*");

        auto pcursor = GetCursor();

        if (pcursor == nullptr)
            throw std::runtime_error("CWalletDB::ListAccountCreditDebit : Cannot create DB cursor");

        uint32_t fFlags = DB_SET_RANGE;

        while(true) {
            /* Read next record */
            DataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
            if (fFlags == DB_SET_RANGE)
            {
                /* Set key input to acentry<account><0> when set range flag is set (first iteration)
                 * This will return all entries beginning with ID 0, with DB_NEXT reading the next entry each time
                 * Read until key does not begin with acentry, does not match requested account, or DB_NOTFOUND (end of database)
                 */
                ssKey << std::make_tuple(std::string("acentry"), (fAllAccounts? std::string("") : strAccount), uint64_t(0));
            }

            DataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);
            int32_t ret = ReadAtCursor(pcursor, ssKey, ssValue, fFlags);

            /* After initial read, change flag setting to DB_NEXT so additional reads just get the next database entry */
            fFlags = DB_NEXT;
            if (ret == DB_NOTFOUND)
            {
                /* End of database reached, no further entries to read */
                break;
            }
            else if (ret != 0)
            {
                /* Error retrieving accounting entries */
                pcursor->close();
                throw std::runtime_error("CWalletDB::ListAccountCreditDebit : Error scanning DB");
            }

            /* Unserialize */
            std::string strType;
            ssKey >> strType;
            if (strType != "acentry")
                break; // Read an entry with a different key type (finished with read)

            CAccountingEntry acentry;
            ssKey >> acentry.strAccount;
            if (!fAllAccounts && acentry.strAccount != strAccount)
                break; // Read an entry for a different account (finished with read)

            /* Database entry read matches both acentry key type and requested account, add to list */
            ssValue >> acentry;
            entries.push_back(acentry);
        }

        pcursor->close();
    }


    /* Initializes a wallet instance from the data in this wallet database. */
    uint32_t CWalletDB::LoadWallet(CWallet& wallet)
    {
        uint32_t nFileVersion = 0;
        std::vector<uint512_t> vWalletRemove;

        uint64_t startTimestamp = runtime::timestamp(true);

        bool fIsEncrypted = false;

        {
            LOCK(wallet.cs_wallet);

            /* Reset default key into wallet to clear any current value. (done now so it stays empty if none loaded) */
            wallet.vchDefaultKey.clear();

            /* Read and validate minversion required by database file */
            uint32_t nMinVersion = 0;
            if (ReadMinVersion(nMinVersion))
            {
                if (nMinVersion > FEATURE_LATEST)
                    return DB_TOO_NEW;

                wallet.LoadMinVersion(nMinVersion);
            }

            /* Get cursor */
            auto pcursor = GetCursor();
            if (pcursor == nullptr)
            {
                debug::error(FUNCTION, "Error getting wallet database cursor");
                return DB_CORRUPT;
            }

            /* Loop to read all entries from wallet database */
            while(true)
            {
                /* Read next record */
                DataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
                DataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);

                int32_t ret = ReadAtCursor(pcursor, ssKey, ssValue);

                if (ret == DB_NOTFOUND)
                {
                    /* End of database, no more entries to read */
                    break;
                }
                else if (ret != 0)
                {
                    debug::error(FUNCTION, "Error reading next record from wallet database");
                    return DB_CORRUPT;
                }

                /* Unserialize
                 * Taking advantage of the fact that pair serialization
                 * is just the two items serialized one after the other
                 */
                std::string strType;
                ssKey >> strType;

                if (strType == "name")
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

                else if (strType == "tx")
                {
                    /* Wallet transaction */
                    uint512_t hash;
                    ssKey >> hash;
                    CWalletTx& wtx = wallet.mapWallet[hash];
                    ssValue >> wtx;

                    if(config::GetBoolArg("-walletclean", false)) {
                        /* Add all transactions to remove list if -walletclean argument is set */
                        vWalletRemove.push_back(hash);

                    }
                    else if (wtx.GetHash() != hash) {
                        debug::error(FUNCTION, "Error in wallet.dat, hash mismatch. Removing Transaction from wallet map. Run the rescan command to restore.");

                        /* Add mismatched transaction to list of transactions to remove from database */
                        vWalletRemove.push_back(hash);
                    }
                    else
                        wtx.BindWallet(wallet);

                }

                else if (strType == "defaultkey")
                {
                    /* Wallet default key */
                    ssValue >> wallet.vchDefaultKey;

                }

                else if (strType == "trustkey")
                {
                    /* Wallet trust key public key */
                    ssValue >> wallet.vchTrustKey;

                }

                else if (strType == "mkey")
                {
                    /* Wallet master key */
                    uint32_t nMasterKeyId;
                    ssKey >> nMasterKeyId;
                    CMasterKey kMasterKey;
                    ssValue >> kMasterKey;

                    /* Load the master key into the wallet */
                    if (!wallet.LoadMasterKey(nMasterKeyId, kMasterKey))
                    {
                        debug::error(FUNCTION, "Error reading wallet database: duplicate CMasterKey id ", nMasterKeyId);
                        return DB_CORRUPT;
                    }

                }

                else if (strType == "key" || strType == "wkey")
                {
                    /* Unencrypted key */
                    std::vector<uint8_t> vchPubKey;
                    ssKey >> vchPubKey;

                    LLC::ECKey key;
                    if (strType == "key")
                    {
                        /* key entry stores unencrypted private key */
                        LLC::CPrivKey privateKey;
                        ssValue >> privateKey;
                        key.SetPubKey(vchPubKey);
                        key.SetPrivKey(privateKey);

                        /* Validate the key data */
                        if (key.GetPubKey() != vchPubKey)
                        {
                            debug::error(FUNCTION, "Error reading wallet database: CPrivKey pubkey inconsistency");
                            return DB_CORRUPT;
                        }

                        if (!key.IsValid())
                        {
                            debug::error(FUNCTION, "Error reading wallet database: invalid CPrivKey");
                            return DB_CORRUPT;
                        }
                    }
                    else
                    {
                        /* wkey entry stores CWalletKey
                         * These are no longer written to database, but support to read them is included for backward compatability
                         * Loaded into wallet key, same as key entry
                         */
                        CWalletKey wkey;
                        ssValue >> wkey;
                        key.SetPubKey(vchPubKey);
                        key.SetPrivKey(wkey.vchPrivKey);

                        /* Validate the key data  */
                        if (key.GetPubKey() != vchPubKey)
                        {
                            debug::error(FUNCTION, "Error reading wallet database: CWalletKey pubkey inconsistency");
                            return DB_CORRUPT;
                        }

                        if (!key.IsValid())
                        {
                            debug::error(FUNCTION, "Error reading wallet database: invalid CWalletKey");
                            return DB_CORRUPT;
                        }
                    }

                    /* Load the key into the wallet */
                    if (!wallet.LoadKey(key))
                    {
                        debug::error(FUNCTION, "Error reading wallet database: LoadKey failed");
                        return DB_CORRUPT;
                    }

                }

                else if (strType == "ckey")
                {
                    /* Encrypted key */
                    std::vector<uint8_t> vchPubKey;
                    ssKey >> vchPubKey;
                    std::vector<uint8_t> vchPrivKey;
                    ssValue >> vchPrivKey;

                    if (!wallet.LoadCryptedKey(vchPubKey, vchPrivKey))
                    {
                        debug::error(FUNCTION, "Error reading wallet database: LoadCryptedKey failed");
                        return DB_CORRUPT;
                    }

                    /* If any one key entry is encrypted, treat the entire wallet as encrypted */
                    fIsEncrypted = true;

                }

                else if (strType == "pool")
                {
                    /* Key pool entry */
                    uint64_t nPoolIndex;
                    ssKey >> nPoolIndex;

                    /* Only the pool index is stored in the key pool */
                    /* Key pool entry will be read from the database as needed */
                    wallet.GetKeyPool().setKeyPool.insert(nPoolIndex);

                }

                else if (strType == "version")
                {
                    /* Wallet database file version */
                    ssValue >> nFileVersion;

                }

                else if (strType == "cscript")
                {
                    /* Script */
                    uint256_t hash;
                    ssKey >> hash;
                    CScript script;
                    ssValue >> script;

                    if (!wallet.LoadCScript(script))
                    {
                        debug::error(FUNCTION, "Error reading wallet database: LoadCScript failed");
                        return DB_CORRUPT;
                    }

                }

                else if (strType == "acentry")
                {
                    /* Accounting entry */
                    std::string strAccount;
                    ssKey >> strAccount;
                    uint64_t nNumber;
                    ssKey >> nNumber;

                    /* After load, nAccountingEntryNumber will contain the maximum accounting entry number currently stored in the database */
                    if (nNumber > CWalletDB::nAccountingEntryNumber)
                        CWalletDB::nAccountingEntryNumber = nNumber;

                }

                else
                {
                    /* All other keys are no longer supported and can be ignored/not loaded */
                }
            }

            CloseCursor(pcursor);

        } /* End lock scope */

        /* Remove transactions flagged for removal */
        if(vWalletRemove.size() > 0)
        {
            for(const auto& hash : vWalletRemove)
            {
                EraseTx(hash);
                wallet.mapWallet.erase(hash);

                debug::log(0, FUNCTION, "Erasing Transaction at hash ", hash.ToString());
            }
        }

        /* Update file version to latest version */
        if (nFileVersion < LLD::DATABASE_VERSION)
            WriteVersion(LLD::DATABASE_VERSION);

        /* Assign maximum upgrade version for wallet */
        wallet.SetMaxVersion(FEATURE_LATEST);

        uint64_t elapsedTime = runtime::timestamp(true) - startTimestamp;

        debug::log(0, FUNCTION, "", fIsEncrypted ? "Encrypted Wallet" : "Wallet", " Loaded in ", elapsedTime, " ms file version = ", nFileVersion);

        return DB_LOAD_OK;
    }


    /* Function that loops until shutdown and periodically flushes a wallet db */
    void CWalletDB::ThreadFlushWalletDB(const std::string& strWalletFile)
    {
        if (!config::GetBoolArg("-flushwallet", true))
            return;

        /* CWalletDB::nWalletDBUpdated is incremented each time wallet database data is updated
         * Generally, we want to flush the database any time this value has changed since the last iteration
         * However, we don't want to do that if it is too soon after the update (to allow for possible series of multiple updates)
         * Therefore, each time the CWalletDB::nWalletDBUpdated has been updated, we record that in nLastSeen along with an updated timestamp
         * Then, whenever CWalletDB::nWalletDBUpdated no longer equals nLastFlushed AND enough time has passed since seeing the change, flush is performed
         * In this manner, if there is a series of (possibly related) updates in a short timespan, they will all be flushed together
         */
        const int64_t minTimeSinceLastUpdate = 2;
        uint32_t nLastSeen = CWalletDB::nWalletDBUpdated;
        uint32_t nLastFlushed = CWalletDB::nWalletDBUpdated;
        uint64_t nLastWalletUpdate = runtime::unifiedtimestamp();

        debug::log(1, FUNCTION, "Wallet flush thread started");

        while (!config::fShutdown)
        {
            runtime::sleep(1000);

            if (nLastSeen != CWalletDB::nWalletDBUpdated)
            {
                /* Database is updated. Record time update recognized */
                nLastSeen = CWalletDB::nWalletDBUpdated;
                nLastWalletUpdate = runtime::unifiedtimestamp();
            }

            /* Perform flush if any wallet database updated, and the minimum required time has passed since recognizing the update */
            if (nLastFlushed != CWalletDB::nWalletDBUpdated && (runtime::unifiedtimestamp() - nLastWalletUpdate) >= minTimeSinceLastUpdate)
            {
                /* Try to lock but don't wait for it. Skip this iteration if fail to get lock. */
                if (CDB::cs_db.try_lock())
                {
                    /* Check ref count and skip flush attempt if any databases are in use (have an open file handle indicated by usage map count > 0) */
                    uint32_t nRefCount = 0;
                    auto mi = CDB::mapFileUseCount.cbegin();

                    while (mi != CDB::mapFileUseCount.cend())
                    {
                        /* Calculate total of all ref counts in map. This will be zero if no databases in use, non-zero if any are. */
                        nRefCount += (*mi).second;
                        mi++;
                    }

                    if (nRefCount == 0 && !config::fShutdown)
                    {
                        /* If strWalletFile has not been opened since startup, no need to flush even if nWalletDBUpdated count has changed.
                         * An entry in mapFileUseCount verifies that this particular wallet file has been used at some point, so it will be flushed.
                         * Should also never have an entry in mapFileUseCount if dbenv is not initialized, but it is checked to be sure.
                         */
                        auto mi = CDB::mapFileUseCount.find(strWalletFile);
                        if (CDB::fDbEnvInit && mi != CDB::mapFileUseCount.end())
                        {
                            debug::log(0, FUNCTION, DateTimeStrFormat(runtime::unifiedtimestamp()), " Flushing ", strWalletFile);
                            nLastFlushed = CWalletDB::nWalletDBUpdated;
                            int64_t nStart = runtime::timestamp(true);

                            /* Flush wallet file so it's self contained */
                            CDB::CloseDb(strWalletFile);
                            CDB::dbenv.txn_checkpoint(0, 0, 0);
                            CDB::dbenv.lsn_reset(strWalletFile.c_str(), 0);

                            CDB::mapFileUseCount.erase(mi++);
                            debug::log(0, FUNCTION, "Flushed ", strWalletFile, " in ", runtime::timestamp(true) - nStart, " ms");
                        }
                    }

                    CDB::cs_db.unlock();
                }
            }
        }

        debug::log(1, FUNCTION, "Shutting down wallet flush thread");

        /* Should be shutdown if get here, so this thread can shutdown database environment */
        if (config::fShutdown)
            CDB::EnvShutdown();
    }


    /* Writes a backup copy of a wallet to a designated backup file */
    bool CWalletDB::BackupWallet(const CWallet& wallet, const std::string& strDest)
    {
        if (!wallet.IsFileBacked())
            return false;

        /* Validate the length of strDest. This assures pathDest.size() cast to uint32_t is always valid (nobody can pass a ridiculously long string) */
#ifndef WIN32
        if (strDest.size() > 4096) {
            debug::log(0, FUNCTION, "Error: Invalid destination path. Path size exceeds maximum limit");
            return false;
        }
#else
        if (strDest.size() > 260) {
            debug::log(0, FUNCTION, "Error: Invalid destination path. Path size exceeds maximum limit");
            return false;
        }
#endif

        while (!config::fShutdown)
        {
            {
                LOCK(CDB::cs_db);

                std::string strSource = wallet.GetWalletFile();

                /* If wallet database is in use, will wait and repeat loop until it becomes available */
                if (CDB::mapFileUseCount.count(strSource) == 0 || CDB::mapFileUseCount[strSource] == 0)
                {
                    /* Flush log data to the dat file */
                    CDB::CloseDb(strSource);
                    CDB::dbenv.txn_checkpoint(0, 0, 0);
                    CDB::dbenv.lsn_reset(strSource.c_str(), 0);
                    CDB::mapFileUseCount.erase(strSource);

                    std::string pathSource(config::GetDataDir() + strSource);
                    std::string pathDest(strDest);

                    /* Create any missing directories in the destination path.
                     * If final character in pathDest is not / then the last entry in path is not created (could be a file name)
                     */
                    filesystem::create_directories(pathDest);

                    /* If destination is a folder, append source file name to use as dest file name */
                    if (filesystem::is_directory(pathDest))
                    {
                        uint32_t s = static_cast<uint32_t>(pathDest.size());

                        if (pathDest[s-1] != '/')
                            pathDest += '/';

                        pathDest = pathDest + strSource;

                        /* After appending to pathDest, need to validate again */
#ifndef WIN32
                        if (strDest.size() > 4096) {
                            debug::log(0, FUNCTION, "Error: Invalid destination path. Path size exceeds maximum limit");
                            return false;
                        }
#else
                        if (strDest.size() > 260) {
                            debug::log(0, FUNCTION, "Error: Invalid destination path. Path size exceeds maximum limit");
                            return false;
                        }
#endif
                    }

                    /* Copy wallet.dat (this method is a bit slow, but is simple and should be ok for an occasional copy) */
                    if (filesystem::copy_file(pathSource, pathDest))
                    {
                        debug::log(0, FUNCTION, "Copied ", strSource, " to ", pathDest);
                        return true;
                    }

                    else
                    {
                        debug::log(0, FUNCTION, "Error copying ", strSource, " to ", pathDest);
                        return false;
                    }
                }
            }

            /* Wait for usage to end when database is in use */
            runtime::sleep(100);
        }

        return false;
    }

}
