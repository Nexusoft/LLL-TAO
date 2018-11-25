/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <iostream>
#include <fstream>
#include <map>
#include <utility>

#include <db_cxx.h> /* Berkeley DB header */

#include <LLC/include/key.h>

#include <LLD/include/version.h>

#include <TAO/Legacy/wallet/wallet.h>
#include <TAO/Legacy/wallet/walletdb.h>

#include <Util/include/filesystem.h>
#include <Util/templates/serialize.h>


namespace Legacy
{

    namespace Wallet
    {

        /* WriteMasterKey */
        bool WriteMasterKey(const uint32_t nID, const CMasterKey& kMasterKey)
        {
            CWalletDB::nWalletDBUpdated++;
            return Write(std::make_pair(std::string("mkey"), nID), kMasterKey, true);
        }


        /* WriteMinVersion */
        bool WriteMinVersion(const int nVersion)
        {
            return Write(std::string("minversion"), nVersion);
        }


        /* ReadAccount */
        bool CWalletDB::ReadAccount(const std::string& strAccount, CAccount& account)
        {
            account.SetNull();
            return Read(std::make_pair(std::string("acc"), strAccount), account);
        }


        /* WriteAccount */
        bool CWalletDB::WriteAccount(const std::string& strAccount, const CAccount& account)
        {
            return Write(std::make_pair(std::string("acc"), strAccount), account);
        }


        /* ReadName */
        bool ReadName(const std::string& strAddress, std::string& strName)
        {
            strName = "";
            return Read(std::make_pair(std::string("name"), strAddress), strName);
        }


        /* WriteName */
        bool CWalletDB::WriteName(const std::string& strAddress, const std::string& strName)
        {
            CWalletDB::nWalletDBUpdated++;
            return Write(std::make_pair(std::string("name"), strAddress), strName);
        }


        /* EraseNamse */
        bool CWalletDB::EraseName(const std::string& strAddress)
        {
            CWalletDB::nWalletDBUpdated++;
            return Erase(std::make_pair(std::string("name"), strAddress));
        }


        /* ReadDefaultKey */
        bool ReadDefaultKey(std::vector<uint8_t>& vchPubKey)
        {
            vchPubKey.clear();
            return Read(std::string("defaultkey"), vchPubKey);
        }


        /* WriteDefaultkey */
        bool WriteDefaultKey(const std::vector<uint8_t>& vchPubKey)
        {
            CWalletDB::nWalletDBUpdated++;
            return Write(std::string("defaultkey"), vchPubKey);
        }


        /* ReadKey */
        bool ReadKey(const std::vector<uint8_t>& vchPubKey, LLC::CPrivKey& vchPrivKey)
        {
            vchPrivKey.clear();
            return Read(std::make_pair(std::string("key"), vchPubKey), vchPrivKey);
        }


        /* WriteKey */
        bool WriteKey(const std::vector<uint8_t>& vchPubKey, const LLC::CPrivKey& vchPrivKey)
        {
            CWalletDB::nWalletDBUpdated++;
            return Write(std::make_pair(std::string("key"), vchPubKey), vchPrivKey, false);
        }


        /* WriteCryptedKey */
        bool WriteCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret, bool fEraseUnencryptedKey = true)
        {
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


        /* ReadTx */
        bool ReadTx(const uint512_t hash, CWalletTx& wtx)
        {
            return Read(std::make_pair(std::string("tx"), hash), wtx);
        }


        /* WriteTx */
        bool WriteTx(const uint512_t hash, const CWalletTx& wtx)
        {
            CWalletDB::nWalletDBUpdated++;
            return Write(std::make_pair(std::string("tx"), hash), wtx);
        }


        /* EraseTx */
        bool EraseTx(const uint512_t hash)
        {
            CWalletDB::nWalletDBUpdated++;
            return Erase(std::make_pair(std::string("tx"), hash));
        }


        /* ReadCScript */
        bool ReadCScript(const uint256_t &hash, Legacy::Types::CScript& redeemScript)
        {
            redeemScript.clear();
            return Read(std::make_pair(std::string("cscript"), hash), redeemScript);
        }


        /* WriteCScript */
        bool WriteCScript(const uint256_t& hash, const Legacy::Types::CScript& redeemScript)
        {
            CWalletDB::nWalletDBUpdated++;
            return Write(std::make_pair(std::string("cscript"), hash), redeemScript, false);
        }


        /* ReadBestBlock */
        bool ReadBestBlock(Core::CBlockLocator& locator)
        {
            return Read(std::string("bestblock"), locator);
        }


        /* WriteBestBlock */
        bool WriteBestBlock(const Core::CBlockLocator& locator)
        {
            CWalletDB::nWalletDBUpdated++;
            return Write(std::string("bestblock"), locator);
        }


        /* ReadPool */
        bool ReadPool(const int64_t nPool, CKeyPool& keypool)
        {
            return Read(std::make_pair(std::string("pool"), nPool), keypool);
        }


        /* WritePool */
        bool WritePool(const int64_t nPool, const CKeyPool& keypool)
        {
            CWalletDB::nWalletDBUpdated++;
            return Write(std::make_pair(std::string("pool"), nPool), keypool);
        }


        /* ErasePool */
        bool ErasePool(const int64_t nPool)
        {
            CWalletDB::nWalletDBUpdated++;
            return Erase(std::make_pair(std::string("pool"), nPool));
        }


        /* WriteAccountingEntry */
        bool CWalletDB::WriteAccountingEntry(const CAccountingEntry& acentry)
        {
            return Write(std::make_tuple(std::string("acentry"), acentry.strAccount, ++CWalletDB::nAccountingEntryNumber), acentry);
        }


        /* GetAccountCreditDebit */
        int64_t CWalletDB::GetAccountCreditDebit(const std::string& strAccount)
        {
            std::list<CAccountingEntry> entries;
            ListAccountCreditDebit(strAccount, entries);

            int64_t nCreditDebitTotal = 0;
            for(CAccountingEntry& entry : entries)
                nCreditDebitTotal += entry.nCreditDebit;

            return nCreditDebitTotal;
        }


        /* ListAccountCreditDebit */
        void CWalletDB::ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& entries)
        {
            bool fAllAccounts = (strAccount == "*");

            auto pcursor = GetCursor();

            if (pcursor == nullptr)
                throw runtime_error("CWalletDB::ListAccountCreditDebit() : cannot create DB cursor");

            uint32_t fFlags = DB_SET_RANGE;

            loop() {
                // Read next record
                CDataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
                if (fFlags == DB_SET_RANGE)
                {
                    // Set key input to acentry<account><0> when set range flag is set (first iteration)
                    // This will return all entries beginning with ID 0, with DB_NEXT reading the next entry each time
                    // Read until key does not begin with acentry, does not match requested account, or DB_NOTFOUND (end of database)
                    ssKey << std::::make_tuple(std::string("acentry"), (fAllAccounts? std::string("") : strAccount), uint64_t(0));
                }

                CDataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);
                int ret = ReadAtCursor(pcursor, ssKey, ssValue, fFlags);

                // After initial read, change flag setting to DB_NEXT so additional reads just get the next database entry
                fFlags = DB_NEXT;
                if (ret == DB_NOTFOUND)
                {
                    // End of database reached, no further entries to read
                    break;
                }
                else if (ret != 0)
                {
                    // Error retrieving accounting entries
                    pcursor->close();
                    throw runtime_error("CWalletDB::ListAccountCreditDebit() : error scanning DB");
                }

                // Unserialize
                std::string strType;
                ssKey >> strType;
                if (strType != "acentry")
                    break; // Read an entry with a different key type (finished with read)

                CAccountingEntry acentry;
                ssKey >> acentry.strAccount;
                if (!fAllAccounts && acentry.strAccount != strAccount)
                    break; // Read an entry for a different account (finished with read)

                // Database entry read matches both acentry key type and requested account, add to list
                ssValue >> acentry;
                entries.push_back(acentry);
            }

            pcursor->close();
        }


        /* LoadWallet */
        int CWalletDB::LoadWallet(std::shared_ptr<CWallet> pwallet)
        {
            pwallet->vchDefaultKey.clear();
            int nFileVersion = 0;
            std::vector<uint512_t> vWalletUpgrade;
            std::vector<uint512_t> vWalletRemove;

            bool fIsEncrypted = false;

            { //Begin lock scope 
                LOCK(pwallet->cs_wallet);

                // Read and validate minversion required by database file
                int nMinVersion = 0;
                if (Read(std::string("minversion"), nMinVersion))
                {
                    if (nMinVersion > LLD::DATABASE_VERSION)
                        return DB_TOO_NEW;

                    pwallet->LoadMinVersion(nMinVersion);
                }

                // Get cursor
                auto pcursor = GetCursor();
                if (pcursor == nullptr)
                {
                    printf("Error getting wallet database cursor\n");
                    return DB_CORRUPT;
                }

                // Loop to read all entries from wallet database
                loop() {
                    // Read next record
                    CDataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
                    CDataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);
                    int ret = ReadAtCursor(pcursor, ssKey, ssValue);

                    if (ret == DB_NOTFOUND)
                    {
                        // End of database, no more entries to read
                        break;
                    }
                    else if (ret != 0)
                    {
                        printf("Error reading next record from wallet database\n");
                        return DB_CORRUPT;
                    }

                    // Unserialize
                    // Taking advantage of the fact that pair serialization
                    // is just the two items serialized one after the other
                    std::string strType;
                    ssKey >> strType;
                    if (strType == "name")
                    {
                        std::string strAddress;
                        ssKey >> strAddress;
                        ssValue >> pwallet->mapAddressBook[strAddress];

                    }
                    else if (strType == "tx")
                    {
                        uint512_t hash;
                        ssKey >> hash;
                        CWalletTx& wtx = pwallet->mapWallet[hash];
                        ssValue >> wtx;

                        if(GetBoolArg("-walletclean", false)) {
                            // Add all transactions to remove list if -walletclean argument is set
                            vWalletRemove.push_back(hash);
                        }
                        else if (wtx.GetHash() != hash) {
                            printf("Error in wallet.dat, hash mismatch. Removing Transaction from wallet map. Run the rescan command to restore.\n");

                            // Add mismatched transaction to list of transactions to remove from database
                            vWalletRemove.push_back(hash);
                        }
                        else
                            wtx.BindWallet(pwallet);

                        // Undo serialize changes in 31600
                        if (31404 <= wtx.fTimeReceivedIsTxTime && wtx.fTimeReceivedIsTxTime <= 31703)
                        {
                            if (!ssValue.empty())
                            {
                                char fTmp;
                                char fUnused;
                                ssValue >> fTmp >> fUnused >> wtx.strFromAccount;
                                printf("LoadWallet() upgrading tx ver=%d %d '%s' %s\n", wtx.fTimeReceivedIsTxTime, fTmp, wtx.strFromAccount.c_str(), hash.ToString().c_str());
                                wtx.fTimeReceivedIsTxTime = fTmp;
                            }
                            else
                            {
                                printf("LoadWallet() repairing tx ver=%d %s\n", wtx.fTimeReceivedIsTxTime, hash.ToString().c_str());
                                wtx.fTimeReceivedIsTxTime = 0;
                            }

                            // Add to list of transactions to be re-written
                            vWalletUpgrade.push_back(hash);
                        }

                    }
                    else if (strType == "acentry")
                    {
                        std::string strAccount;
                        ssKey >> strAccount;
                        uint64_t nNumber;
                        ssKey >> nNumber;

                        // After load, nAccountingEntryNumber will contain the maximum accounting entry number currently stored in the database
                        if (nNumber > CWalletDB::nAccountingEntryNumber)
                            CWalletDB::nAccountingEntryNumber = nNumber;

                    }
                    else if (strType == "key" || strType == "wkey")
                    {
                        std::vector<uint8_t> vchPubKey;
                        ssKey >> vchPubKey;

                        LLC::ECKey key;
                        if (strType == "key")
                        {
                            // key entry stores unencrypted private key
                            CPrivKey privateKey;
                            ssValue >> privateKey;
                            key.SetPubKey(vchPubKey);
                            key.SetPrivKey(privateKey);

                            // Validate the key data 
                            if (key.GetPubKey() != vchPubKey)
                            {
                                printf("Error reading wallet database: CPrivKey pubkey inconsistency\n");
                                return DB_CORRUPT;
                            }

                            if (!key.IsValid())
                            {
                                printf("Error reading wallet database: invalid CPrivKey\n");
                                return DB_CORRUPT;
                            }
                        }
                        else
                        {
                            // wkey entry stores CWalletKey
                            // These are no longer written to database, but support to read them is included for backward compatability
                            // Loaded into wallet key, same as key entry
                            CWalletKey wkey;
                            ssValue >> wkey;
                            key.SetPubKey(vchPubKey);
                            key.SetPrivKey(wkey.vchPrivKey);

                            // Validate the key data 
                            if (key.GetPubKey() != vchPubKey)
                            {
                                printf("Error reading wallet database: CWalletKey pubkey inconsistency\n");
                                return DB_CORRUPT;
                            }

                            if (!key.IsValid())
                            {
                                printf("Error reading wallet database: invalid CWalletKey\n");
                                return DB_CORRUPT;
                            }
                        }

                        // Load the key into the wallet
                        if (!pwallet->LoadKey(key))
                        {
                            printf("Error reading wallet database: LoadKey failed\n");
                            return DB_CORRUPT;
                        }

                    }
                    else if (strType == "mkey")
                    {
                        uint32_t nID;
                        ssKey >> nID;
                        CMasterKey kMasterKey;
                        ssValue >> kMasterKey;

                        // Validate the master key data
                        if(pwallet->mapMasterKeys.count(nID) != 0)
                        {
                            printf("Error reading wallet database: duplicate CMasterKey id %u\n", nID);
                            return DB_CORRUPT;
                        }

                        pwallet->mapMasterKeys[nID] = kMasterKey;

                        // After load, wallet nMasterKeyMaxID will contain the maximum key ID currently stored in the database
                        if (pwallet->nMasterKeyMaxID < nID)
                            pwallet->nMasterKeyMaxID = nID;

                    }
                    else if (strType == "ckey")
                    {
                        std::vector<uint8_t> vchPubKey;
                        ssKey >> vchPubKey;
                        std::vector<uint8_t> vchPrivKey;
                        ssValue >> vchPrivKey;

                        if (!pwallet->LoadCryptedKey(vchPubKey, vchPrivKey))
                        {
                            printf("Error reading wallet database: LoadCryptedKey failed\n");
                            return DB_CORRUPT;
                        }

                        // If any one key entry is encrypted, treat the entire wallet as encrypted
                        fIsEncrypted = true;

                    }
                    else if (strType == "defaultkey")
                    {
                        ssValue >> pwallet->vchDefaultKey;

                    }
                    else if (strType == "pool")
                    {
                        int64_t nIndex;
                        ssKey >> nIndex;

                        // Only the key ID (index) is added to the wallet now
                        // Key pool values will be read from the database as needed
                        pwallet->setKeyPool.insert(nIndex);

                    }
                    else if (strType == "version")
                    {
                        ssValue >> nFileVersion;

                        if (nFileVersion == 10300)
                            nFileVersion = 300;

                    }
                    else if (strType == "cscript")
                    {
                        uint256_t hash;
                        ssKey >> hash;
                        CScript script;
                        ssValue >> script;

                        if (!pwallet->LoadCScript(script))
                        {
                            printf("Error reading wallet database: LoadCScript failed\n");
                            return DB_CORRUPT;
                        }

                    }
                }

                pcursor->close();

            } //End lock scope

            // Rewrite transactions flagged for upgrade
            for(auto hash : vWalletUpgrade)
                WriteTx(hash, pwallet->mapWallet[hash]);


            // Remove transactions flagged for removal
            if(vWalletRemove.size() > 0) {
                for(auto hash : vWalletRemove) {
                    EraseTx(hash);
                    pwallet->mapWallet.erase(hash);

                    printf("Erasing Transaction at hash %s\n", hash.ToString().c_str());
                }
            }

            printf("nFileVersion = %d\n", nFileVersion);


            // Update file version to latest version
            if (nFileVersion < LLD::DATABASE_VERSION) 
                WriteVersion(LLD::DATABASE_VERSION);

            return DB_LOAD_OK;
        }


        /* ThreadFlushWalletDB */
        void ThreadFlushWalletDB(const std::string strWalletFile)
        {
            if (!GetBoolArg("-flushwallet", true))
                return;

            // CWalletDB::nWalletDBUpdated is incremented each time wallet database data is updated
            // Generally, we want to flush the database any time this value has changed since the last iteration
            // However, we don't want to do that if it is too soon after the update (to allow for possible series of multiple updates)
            // Therefore, each time the CWalletDB::nWalletDBUpdated has been updated, we record that in nLastSeen along with an updated timestamp
            // Then, whenever CWalletDB::nWalletDBUpdated no longer equals nLastFlushed AND enough time has passed since seeing the change, flush is performed
            // In this manner, if there is a series of (possibly related) updates in a short timespan, they will all be flushed together
            const int64_t minTimeSinceLastUpdate = 2;
            uint32_t nLastSeen = CWalletDB::nWalletDBUpdated;
            uint32_t nLastFlushed = CWalletDB::nWalletDBUpdated;
            int64_t nLastWalletUpdate = GetUnifiedTimestamp();

            while (!fShutdown)
            {
                Sleep(500);

                if (nLastSeen != CWalletDB::nWalletDBUpdated)
                {
                    // Database is update. Record time update recognized
                    nLastSeen = CWalletDB::nWalletDBUpdated;
                    nLastWalletUpdate = GetUnifiedTimestamp();
                }

                // Perform flush if database updated, and the minimum required time has passed since recognizing the update
                if (nLastFlushed != CWalletDB::nWalletDBUpdated && (GetUnifiedTimestamp() - nLastWalletUpdate) >= minTimeSinceLastUpdate)
                {
                    // Try to lock but don't wait for it. Skip this iteration fail to get lock.
                    if (CDB::cs_db.try_lock())
                    {
                        // Check ref count and skip flush attempt if any databases are in use (have an open file handle indicated by usage map count > 0)
                        int nRefCount = 0;
                        auto mi = CDB::mapFileUseCount.cbegin();

                        while (mi != CDB::mapFileUseCount.cend())
                        {
                            // Calculate total of all ref counts in map. This will be zero if no databases in use, non-zero if any are.
                            nRefCount += (*mi).second;
                            mi++;
                        }

                        if (nRefCount == 0 && !fShutdown)
                        {
                            auto mi = CDB::mapFileUseCount.find(CWalletDB::WALLET_DB);
                            if (mi != CDB::mapFileUseCount.end())
                            {
                                printf("%s ", DateTimeStrFormat(GetUnifiedTimestamp()).c_str());
                                printf("Flushing wallet.dat\n");
                                nLastFlushed = CWalletDB::nWalletDBUpdated;
                                int64_t nStart = GetTimeMillis();

                                // Flush wallet.dat so it's self contained
                                CloseDb(strFile);
                                dbenv.txn_checkpoint(0, 0, 0);
                                dbenv.lsn_reset(strFile.c_str(), 0);

                                mapFileUseCount.erase(mi++);
                                printf("Flushed wallet.dat %" PRI64d "ms\n", GetTimeMillis() - nStart);
                            }
                        }

                        CDB::cs_db.unlock();
                    }
                }
            }
        }


        /* BackupWallet */
        bool BackupWallet(const CWallet& wallet, const std::string& strDest)
        {
            if (!wallet.fFileBacked)
                return false;

            while (!fShutdown)
            {
                {
                    std::lock_guard<std::mutex> dbLock(CDB::cs_db); 

                    // If wallet database is in use, will wait and repeat loop until it becomes available
                    if (CDB::mapFileUseCount.count(CWalletDB::WALLET_DB) == 0 || CDB::mapFileUseCount[CWalletDB::WALLET_DB] == 0)
                    {
                        // Flush log data to the dat file
                        CloseDb(wallet.strWalletFile);
                        dbenv.txn_checkpoint(0, 0, 0);
                        dbenv.lsn_reset(wallet.strWalletFile.c_str(), 0);
                        CDB::mapFileUseCount.erase(CWalletDB::WALLET_DB);

                        std::string pathSrc(GetDataDir() + "/" + CWalletDB::WALLET_DB);
                        std::string pathDest(strDest);

                        // If destination is a folder, use wallet database name
                        if (filesystem::is_directory(pathDest))
                            pathDest = pathDest + "/" + CWalletDB::WALLET_DB;

                        // If destination file exists, remove it (ie, we overwrite the file)
                        if (filesystem::exists(pathDest))
                            filesystem::remove(pathDest);

                        // Copy wallet.dat (this method is a bit slow, but is simple and should be ok for an occasional copy)
                        if filesystem::copy_file(pathSource, pathDest)
                        {
                            printf("Copied wallet.dat to %s\n", pathDest.c_str());
                            return true;
                        }

                        else
                        {
                            printf("Error copying wallet.dat to %s\n", pathDest.c_str());
                            return false;
                        }
                    }
                }

                // Wait for usage to end when database is in use
                Sleep(100);
            }

            return false;
        }

    }
}
