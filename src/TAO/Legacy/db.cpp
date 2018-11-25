/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Legacy/wallet/db.h>

#include <LLD/include/version.h>

#include <Util/include/filesystem.h>
#include <Util/templates/serialize.h>

#ifndef WIN32
#include "sys/stat.h"
#endif


namespace Legacy
{
    
    namespace Wallet
    {

        /* Initialize namespace-scope variable */
        bool fDetachDB = false;

        /* Initialization for class static variables */
        DbEnv CDB::dbenv = DbEnv((uint32_t)0);

        bool CDB::fDbEnvInit = false;

        std::map<std::string, int> CDB::mapFileUseCount; //initializes empty map

        std::map<std::string, std::shared_ptr<Db>> CDB::mapDb;  //initializes empty map


        /* Constructor */
        /* Initializes database environment on first use */
        CDB::CDB(const char *pszFile, const char* pszMode) : pdb(nullptr)
        {
            int ret;

            //Passing a null string will initialize a null instance
            if (pszFile == nullptr)
                return;

            { //Begin lock scope
                std::lock_guard<std::mutex> dbLock(CDB::cs_db); //Need to lock before test fDbEnvIniti

                if (!CDB::fDbEnvInit)
                {
                    //Need to initialize database environment. This is only done once upon construction of the first CDB instance
                    if (fShutdown)
                        return;
 
                    std::string pathDataDir(GetDataDir());
                    std::string pathLogDir(pathDataDir + "/database");
                    filesystem::create_directory(pathLogDir);
 
                    std::string pathErrorFile(pathDataDir + "/db.log");
                    printf("dbenv.open LogDir=%s ErrorFile=%s\n", pathLogDir.c_str(), pathErrorFile.c_str());

                    int nDbCache = GetArg("-dbcache", 25);

                    CDB::dbenv.set_lg_dir(pathLogDir.c_str());
                    CDB::dbenv.set_cachesize(nDbCache / 1024, (nDbCache % 1024)*1048576, 1);
                    CDB::dbenv.set_lg_bsize(1048576);
                    CDB::dbenv.set_lg_max(10485760);
                    CDB::dbenv.set_lk_max_locks(10000);
                    CDB::dbenv.set_lk_max_objects(10000);
                    CDB::dbenv.set_errfile(fopen(pathErrorFile.c_str(), "a")); /// debug
                    CDB::dbenv.set_flags(DB_TXN_WRITE_NOSYNC, 1);
                    CDB::dbenv.set_flags(DB_AUTO_COMMIT, 1);
                    CDB::dbenv.log_set_config(DB_LOG_AUTO_REMOVE, 1);

                    /* Flags to enable dbenv subsystems
                     * DB_CREATE     - Create underlying files, as needed (required when DB_RECOVER present)
                     * DB_INIT_LOCK  - Enable locking to allow multithreaded read/write
                     * DB_INIT_LOG   - Enable use of recovery logs (required when DB_INIT_TXN present)
                     * DB_INIT_MPOOL - Enable shared memory pool
                     * DB_INIT_TXN   - Enable transaction management and recovery
                     * DB_THREAD     - Enable multithreaded access
                     * DB_RECOVER    - Run recovery before opening environment, if necessary 
                     */
                    uint32_t dbFlags =  DB_CREATE     |
                                        DB_INIT_LOCK  |
                                        DB_INIT_LOG   |
                                        DB_INIT_MPOOL |
                                        DB_INIT_TXN   |
                                        DB_THREAD     |
                                        DB_RECOVER;

                    /* Mode specifies permission settings for all Berkeley-created files on UNIX systems
                     * as defined in the system file sys/stat.h
                     *
                     *   S_IRUSR - Readable by owner
                     *   S_IWUSR - Writable by owner
                     *
                     * This setting overrides default of readable/writable by both owner and group
                     */
#ifndef WIN32
                    int dbMode = S_IRUSR | S_IWUSR;
#else
                    int dbMode = 0;
#endif

                    //Open the Berkely DB environment
                    ret = CDB::dbenv.open(pathDataDir.c_str(), dbFlags, dbMode);

                    if (ret > 0)
                        throw runtime_error(strprintf("CDB() : error %d opening database environment", ret));

                    CDB::fDbEnvInit = true;
                }

                //Initialize current CDB instance
                strFile = pszFile;

                //Usage count will be incremented whether we use pdb from mapDb or open a new one
                if (CDB::mapFileUseCount.count(strFile) == 0)
                    CDB::mapFileUseCount.insert(strFile, 1);
                else
                    ++CDB::mapFileUseCount[strFile];

                //Extract mode settings
                bool fCreate = (strchr(pszMode, 'c') != nullptr);
                bool fWrite = (strchr(pszMode, 'w') != nullptr);
                bool fAppend = (strchr(pszMode, '+') != nullptr);

                //Set database read-only if not write or append mode
                fReadOnly = !(fWrite || fAppend);

                if (CDB::mapDb.count(strFile) > 0)
                {
                    //mapDb contains entry for strFile, so database is already open
                    pdb = CDB::mapDb[strFile];
                }
                else
                {
                    //Database not already open, so open it now
                    pdb = std::make_shared<Db>(Db(&CDB::dbenv, 0));

                    //Opened database will support multi-threaded access
                    uint32_t nFlags = DB_THREAD;

                    if (fCreate)
                        nFlags |= DB_CREATE; //Add flag to create database file if does not exist

                    ret = pdb->open(nullptr,   // Txn pointer
                                    pszFile,   // Filename
                                    "main",    // Logical db name
                                    DB_BTREE,  // Database type
                                    nFlags,    // Flags
                                    0);

                    if (ret == 0)
                    {
                        //Database opened successfully. Add database to open database map
                        CDB::mapDb[strFile] = pdb;

                        if (fCreate && !Exists(std::string("version")))
                        {
                            //For new database file, write the database version as the first entry
                            //This is written even if the new database was created as read-only
                            bool fTmp = fReadOnly;
                            fReadOnly = false;
                            WriteVersion(LLD::DATABASE_VERSION);
                            fReadOnly = fTmp;
                        }
                    }
                    else
                    {
                        //Error opening db, reset db (no need to delete the shared pointer)
                        pdb = nullptr;
                        strFile = "";

                        --CDB::mapFileUseCount[strFile];

                        throw runtime_error(strprintf("CDB() : can't open database file %s, error %d", pszFile, ret));
                    }
                }
            } //End lock scope

        }


        /* Destructor */
        CDB::~CDB() 
        { 
            Close(); 
        }


        /* Read */
        template<typename K, typename T>
        bool CDB::Read(const K& key, T& value)
        {
            if (pdb == nullptr)
                return false;

            // Key
            CDataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(&ssKey[0], ssKey.size());

            // Value
            Dbt datValue;
            datValue.set_flags(DB_DBT_MALLOC); // Berkeley will allocate memory for returned value, will need to free before return

            // Read
            int ret = pdb->get(GetRawTxn(), &datKey, &datValue, 0);

            // Clear the key memory
            memset(datKey.get_data(), 0, datKey.get_size());

            if (datValue.get_data() == nullptr)
                return false; // Key not found or no value present, can safely return without free() because there is nothing to free

            // Unserialize value
            try {
                // get_data returns a void* and currently uses a C-style cast to load into CDataStream
                CDataStream ssValue((char*)datValue.get_data(), (char*)datValue.get_data() + datValue.get_size(), SER_DISK, LLD::DATABASE_VERSION);
                ssValue >> value;
            }
            catch (std::exception &e) {
                // Still need to free any memory allocated for datValue, so do not return here. Just set ret so it returns false
                ret = -1;
            }

            // Clear and free value memory
            memset(datValue.get_data(), 0, datValue.get_size());
            free(datValue.get_data());

            return (ret == 0);
        }


        /* Write */
        template<typename K, typename T>
        bool CDB::Write(const K& key, const T& value, bool fOverwrite=true)
        {
            if (pdb == nullptr)
                return false;

            if (fReadOnly)
                assert(!"Write called on database in read-only mode");

            // Key
            CDataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(&ssKey[0], ssKey.size());

            // Value
            CDataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);
            ssValue.reserve(10000);
            ssValue << value;
            Dbt datValue(&ssValue[0], ssValue.size());

            // Write
            int ret = pdb->put(GetRawTxn(), &datKey, &datValue, (fOverwrite ? 0 : DB_NOOVERWRITE));

            // Clear memory in case it was a private key
            memset(datKey.get_data(), 0, datKey.get_size());
            memset(datValue.get_data(), 0, datValue.get_size());

            return (ret == 0);
        }


        /* Erase */
        template<typename K>
        bool CDB::Erase(const K& key)
        {
            if (pdb == nullptr)
                return false;

            if (fReadOnly)
                assert(!"Erase called on database in read-only mode");

            // Key
            CDataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(&ssKey[0], ssKey.size());

            // Erase
            int ret = pdb->del(GetRawTxn(), &datKey, 0);

            // Clear memory
            memset(datKey.get_data(), 0, datKey.get_size());

            return (ret == 0 || ret == DB_NOTFOUND);
        }


        /* Exists */
        template<typename K>
        bool CDB::Exists(const K& key)
        {
            if (pdb == nullptr)
                return false;

            // Key
            CDataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(&ssKey[0], ssKey.size());

            // Exists
            int ret = pdb->exists(GetRawTxn(), &datKey, 0);

            // Clear memory
            memset(datKey.get_data(), 0, datKey.get_size());
            return (ret == 0);
        }


        /* GetCursor */
        std::shared_ptr<Dbc> CDB::GetCursor()
        {
            if (pdb == nullptr)
                return nullptr;

            // To play nicely with Berkeley API, cannot define as shared_ptr for API call. 
            // Instead, need to construct shared_ptr from the returned Dbc*
            Dbc* dbcursor = nullptr;

            int ret = pdb->cursor(nullptr, &dbcursor, 0);

            if (ret != 0)
                return nullptr;

            std::make_shared<Dbc> pcursor(dbcursor);

            return pcursor; //dereference to pass Dbc to make-shared
        }


        /* ReadAtCursor */
        int CDB::ReadAtCursor(std::shared_ptr<Dbc> pcursor, CDataStream& ssKey, CDataStream& ssValue, uint32_t fFlags)
        {
            // Key - Initialize with argument data for flag settings that need it
            Dbt datKey;
            if (fFlags == DB_SET || fFlags == DB_SET_RANGE || fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
            {
                datKey.set_data(&ssKey[0]);
                datKey.set_size(ssKey.size());
            }

            // Value - Initialize with argument data for flag settings that need it
            Dbt datValue;
            if (fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
            {
                datValue.set_data(&ssValue[0]);
                datValue.set_size(ssValue.size());
            }

            // Berkeley will allocate memory for returned data, will need to free before return
            datKey.set_flags(DB_DBT_MALLOC);
            datValue.set_flags(DB_DBT_MALLOC);

            // Execute cursor operation
            int ret = pcursor->get(&datKey, &datValue, fFlags);

            if (ret != 0)
                return ret;

            else if (datKey.get_data() == nullptr || datValue.get_data() == nullptr)
                return 99999;

            // Convert to streams
            ssKey.SetType(SER_DISK);
            ssKey.clear();
            ssKey.write((char*)datKey.get_data(), datKey.get_size());

            ssValue.SetType(SER_DISK);
            ssValue.clear();
            ssValue.write((char*)datValue.get_data(), datValue.get_size());

            // Clear and free memory
            memset(datKey.get_data(), 0, datKey.get_size());
            memset(datValue.get_data(), 0, datValue.get_size());

            free(datKey.get_data());
            free(datValue.get_data());

            return 0;
        }


        /* CloseCursor */
        CDB::CloseCursor(std::shared_ptr<Dbc> pcursor)
        {
            if (pdb == nullptr)
                return;

            int ret = pcursor->close();

            return;
        }


        /* GetTxn */
        std::shared_ptr<DbTxn> CDB::GetTxn()
        {
            if (!vTxn.empty())
                return vTxn.back();
            else
                return nullptr;
        }


        /* GetRawTxn */
        DbTxn* CDB::GetRawTxn()
        {
            DbTxn* dbTxn = nullptr; 
            auto pTxn = GetTxn();

            if (pTxn != nullptr)
                dbTxn = pTxn.get(); 

            return dbTxn;
        }


        /* TxnBegin */
        bool CDB::TxnBegin()
        {
            if (pdb == nullptr)
                return false;

            // Start a new database transaction. Need to use raw pointer with Berkeley API
            DbTxn* dbTxn = nullptr;

            // Begin new transaction
            int ret = CDB::dbenv.txn_begin(getRawTxn(), &dbTxn, DB_TXN_WRITE_NOSYNC);

            if (dbTxn == nullptr || ret != 0)
                return false;

            // Add new transaction to the end of vTxn
            std::shared_ptr<DbTxn> pTxn(dbTxn);
            vTxn.push_back(pTxn);

            return true;
        }


        /* TxnCommit */
        bool CDB::TxnCommit()
        {
            if (pdb == nullptr)
                return false;

            auto pTxn = GetTxn();

            if (pTxn == nullptr)
                return false;

            int ret = pTxn->commit(0);

            vTxn.pop_back();

            return (ret == 0);
        }


        /* TxnAbort */
        bool CDB::TxnAbort()
        {
            if (pdb == nullptr)
                return false;

            auto pTxn = GetTxn();

            if (pTxn == nullptr)
                return false;

            int ret = pTxn->abort();

            vTxn.pop_back();

            return (ret == 0);
        }


        /* ReadVersion */
        bool CDB::ReadVersion(int& nVersion)
        {
            nVersion = 0;
            return Read(std::string("version"), nVersion);
        }


        /* WriteVersion */
        bool CDB::WriteVersion(int nVersion)
        {
            return Write(std::string("version"), nVersion);
        }


        /* Close */
        void CDB::Close()
        {
            if (pdb == nullptr)
                return;

            // Abort any in-progress transactions on this database
            if (!vTxn.empty())
                vTxn.front()->abort();

            vTxn.clear();

            // Flush database activity from memory pool to disk log
            uint32_t nMinutes = 0;
            if (fReadOnly)
                nMinutes = 1;

            if (strFile == "addr.dat")
                nMinutes = 2;

            CDB::dbenv.txn_checkpoint(nMinutes ? GetArg("-dblogsize", 100)*1024 : 0, nMinutes, 0);

            {
                std::lock_guard<std::mutex> dbLock(CDB::cs_db); 
                --CDB::mapFileUseCount[strFile];
            }

            pdb = nullptr;
            strFile = "";

        }


        /* CloseDb */
        void CDB::CloseDb(const std::string& strFile)
        {
            {
                std::lock_guard<std::mutex> dbLock(CDB::cs_db); 

                if (CDB::mapDb.count(strFile) > 0)
                {
                    // Close the database handle
                    auto pdb = CDB::mapDb[strFile];
                    pdb->close(0);

                    CDB::mapDb.erase(strFile);
                }
            }
        }


        /* DBFlush */
        void CDB::DBFlush(bool fShutdown)
        {
            // Flush log data to the actual data file on all files that are not in use
            printf("DBFlush(%s)%s\n", fShutdown ? "true" : "false", CDB::fDbEnvInit ? "" : " db not started");

            if (!CDB::fDbEnvInit)
                return;

            {
                std::lock_guard<std::mutex> dbLock(CDB::cs_db); 

                for (auto mi = CDB::mapFileUseCount.cbegin(); mi != CDB::mapFileUseCount.cend(); /*no increment */)
                {
                    const std::string strFile = (*mi).first;
                    const int nRefCount = (*mi).second;
                    
                    printf("%s refcount=%d\n", strFile.c_str(), nRefCount);

                    if (nRefCount == 0)
                    {
                        // Move log data to the dat file
                        CloseDb(strFile);

                        printf("%s checkpoint\n", strFile.c_str());
                        CDB::dbenv.txn_checkpoint(0, 0, 0);

                        if (strFile != "addr.dat" || fDetachDB) 
                        {
                            printf("%s detach\n", strFile.c_str());
                            CDB::dbenv.lsn_reset(strFile.c_str(), 0);
                        }

                        printf("%s closed\n", strFile.c_str());
                        CDB::mapFileUseCount.erase(mi++);
                    }
                    else
                        mi++;
                }

                if (fShutdown)
                {
                    char** listp;
                    if (CDB::mapFileUseCount.empty())
                    {
                        CDB::dbenv.log_archive(&listp, DB_ARCH_REMOVE);
                        EnvShutdown();
                    }
                }
            }
        }


        /* Rewwrite */
        bool CDB::DBRewrite(const std::string& strFile, const char* pszSkip)
        {
            if (!fShutdown)
            {
                { // Begin lock scope
                    std::lock_guard<std::mutex> dbLock(CDB::cs_db); 

                    if (CDB::mapFileUseCount.count(strFile) == 0 || CDB::mapFileUseCount[strFile] == 0)
                    {
                        // Flush log data to the dat file and detach the file
                        CloseDb(strFile);
                        CDB::dbenv.txn_checkpoint(0, 0, 0);
                        CDB::dbenv.lsn_reset(strFile.c_str(), 0);
                        CDB::mapFileUseCount.erase(strFile);

                        bool fSuccess = true;
                        printf("Rewriting %s...\n", strFile.c_str());

                        //Define temporary file name where copy will be written
                        std::string strFileRes = strFile + ".rewrite";

                        { // surround usage of db with extra {}
                            CDB dbToRewrite(strFile.c_str(), "r");
                            std::unique_ptr<Db> pdbCopy(Db(&CDB::dbenv, 0));

                            // Open database handle to temp file 
                            int ret = pdbCopy->open(NULL,               // Txn pointer
                                                    strFileRes.c_str(), // Filename
                                                    "main",             // Logical db name
                                                    DB_BTREE,           // Database type
                                                    DB_CREATE,          // Flags
                                                    0);
                            if (ret > 0)
                            {
                                printf("Cannot create database file %s\n", strFileRes.c_str());
                                fSuccess = false;
                            }

                            auto pcursor = dbToRewrite.GetCursor();

                            if (pcursor != nullptr)
                                while (fSuccess)
                                {
                                    CDataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
                                    CDataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);

                                    // Read next key-value pair to copy
                                    int ret = dbToRewrite.ReadAtCursor(pcursor, ssKey, ssValue, DB_NEXT);

                                    if (ret == DB_NOTFOUND)
                                    {
                                        // No more data
                                        pcursor->close();
                                        break;
                                    }
                                    else if (ret != 0)
                                    {
                                        // Error reading cursor
                                        pcursor->close();
                                        fSuccess = false;
                                        break;
                                    }

                                    // Skip any key value defined by pszSkip argument
                                    if (pszSkip && strncmp(&ssKey[0], pszSkip, std::min(ssKey.size(), strlen(pszSkip))) == 0)
                                        continue;

                                    // Don't copy the version, instead use latest version
                                    if (strncmp(&ssKey[0], "\x07version", 8) == 0)
                                    {
                                        // Update version:
                                        ssValue.clear();
                                        ssValue << LLD::DATABASE_VERSION;
                                    }

                                    Dbt datKey(&ssKey[0], ssKey.size());
                                    Dbt datValue(&ssValue[0], ssValue.size());

                                    // Write the data to temp file 
                                    int ret2 = pdbCopy->put(nullptr, &datKey, &datValue, DB_NOOVERWRITE);

                                    if (ret2 > 0)
                                        fSuccess = false;
                                }

                            if (fSuccess)
                            {
                                dbToRewrite.Close();
                                CloseDb(strFile);

                                if (pdbCopy->close(0) != 0)
                                    fSuccess = false;
                            }
                        }

                        if (fSuccess)
                        {
                            // Remove original database file
                            Db dbA(&CDB::dbenv, 0);
                            if (dbA.remove(strFile.c_str(), nullptr, 0) != 0)
                                fSuccess = false;
                        }

                        if (fSuccess)
                        {
                            // Rename temp file to original file name
                            Db dbB(&CDB::dbenv, 0);
                            if (dbB.rename(strFileRes.c_str(), nullptr, strFile.c_str(), 0) != 0)
                                fSuccess = false;
                        }

                        if (!fSuccess)
                            printf("Rewriting of %s FAILED!\n", strFile.c_str());

                        return fSuccess;
                    }
                } // End lock scope
            }

            return false; // Return false if fShutdown
        }


        /* EnvShutdown */
        void CDB::EnvShutdown()
        {
            if (!CDB::fDbEnvInit)
                return;

            CDB::fDbEnvInit = false;
            try
            {
                CDB::dbenv.close(0);
            }
            catch (const DbException& e)
            {
                printf("EnvShutdown exception: %s (%d)\n", e.what(), e.get_errno());
            }
 
            // Use of DB_FORCE should not be necessary after calling dbenv.close() but included in case there is an exception
            CDB::dbenv.remove(GetDataDir().c_str(), DB_FORCE);
        }

    }
}
