/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <exception>

#include <Legacy/wallet/db.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/filesystem.h>
#include <Util/templates/serialize.h>

#include <sys/stat.h>


namespace Legacy
{

    /* Initialize namespace-scope variable */
    bool fDetachDB = false;

    /* Initialization for class static variables */
    DbEnv BerkeleyDB::dbenv((uint32_t)0);

    bool BerkeleyDB::fDbEnvInit = false;

    std::map<std::string, uint32_t> BerkeleyDB::mapFileUseCount; //initializes empty map

    std::map<std::string, Db*> BerkeleyDB::mapDb;  //initializes empty map

    std::mutex BerkeleyDB::cs_db; //initializes the mutex


    /* Constructor */
    /* Initializes database environment on first use */
    BerkeleyDB::BerkeleyDB(const char *pszFileIn, const char* pszMode)
    : pdb(nullptr)
    , strFile()
    , vTxn()
    , fReadOnly(true)
    {
        /* Passing a null string will initialize a null instance */
        if (pszFileIn == nullptr)
            return;

        std::string strFileIn(pszFileIn);

        Init(strFileIn, pszMode);

        return;
    }


    /* Constructor */
    /* Initializes database environment on first use */
    BerkeleyDB::BerkeleyDB(const std::string& strFileIn, const char* pszMode)
    : pdb(nullptr)
    , strFile()
    , vTxn()
    , fReadOnly(true)
    {
        /* Passing an empty string will initialize a null instance */
        if (strFileIn.empty())
            return;

        Init(strFileIn, pszMode);

        return;
    }


    /* Destructor */
    BerkeleyDB::~BerkeleyDB()
    {
        Close();
    }


    /* Performs work of initialization for constructors. */
    void BerkeleyDB::Init(const std::string& strFileIn, const char* pszMode)
    {
        int32_t ret;

        LOCK(BerkeleyDB::cs_db); // Need to lock before test fDbEnvInit

        if (!BerkeleyDB::fDbEnvInit)
        {
            /* Need to initialize database environment. This is only done once upon construction of the first BerkeleyDB instance */
            if (config::fShutdown)
                return;

            std::string pathDataDir(config::GetDataDir());
            std::string pathLogDir(pathDataDir + "database");
            filesystem::create_directory(pathLogDir);

            std::string pathErrorFile(pathDataDir + "db.log");
            //debug::log(0, FUNCTION, "dbenv.open LogDir=", pathLogDir, " ErrorFile=",  pathErrorFile);

            uint32_t nDbCache = config::GetArg("-dbcache", 25);

            BerkeleyDB::dbenv.set_lg_dir(pathLogDir.c_str());
            BerkeleyDB::dbenv.set_cachesize(nDbCache / 1024, (nDbCache % 1024)*1048576, 1);
            BerkeleyDB::dbenv.set_lg_bsize(1048576);
            BerkeleyDB::dbenv.set_lg_max(10485760);
            BerkeleyDB::dbenv.set_lk_max_locks(10000);
            BerkeleyDB::dbenv.set_lk_max_objects(10000);
            //BerkeleyDB::dbenv.set_errfile(fopen(pathErrorFile.c_str(), "a")); /// debug
            BerkeleyDB::dbenv.set_flags(DB_TXN_WRITE_NOSYNC, 1);
            BerkeleyDB::dbenv.set_flags(DB_AUTO_COMMIT, 1);
            BerkeleyDB::dbenv.log_set_config(DB_LOG_AUTO_REMOVE, 1);

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
            uint32_t dbMode = S_IRUSR | S_IWUSR;
#else
            uint32_t dbMode = 0;
#endif

            /* Open the Berkely DB environment */
            ret = BerkeleyDB::dbenv.open(pathDataDir.c_str(), dbFlags, dbMode);

            if (ret > 0)
                throw std::runtime_error(debug::strprintf(FUNCTION, "Error %d initializing Berkeley database environment", ret));

            BerkeleyDB::fDbEnvInit = true;

            debug::log(0, FUNCTION, "Initialized Legacy Berkeley database environment");
        }

        /* Initialize current BerkeleyDB instance */
        strFile = strFileIn;

        /* Usage count will be incremented whether we use pdb from mapDb or open a new one */
        if (BerkeleyDB::mapFileUseCount.count(strFile) == 0)
            BerkeleyDB::mapFileUseCount[strFile] = 1;
        else
            ++BerkeleyDB::mapFileUseCount[strFile];

        /* Extract mode settings */
        bool fCreate = (strchr(pszMode, 'c') != nullptr);
        bool fWrite = (strchr(pszMode, 'w') != nullptr);
        bool fAppend = (strchr(pszMode, '+') != nullptr);

        /* Set database read-only if not write or append mode */
        fReadOnly = !(fWrite || fAppend);

        if (BerkeleyDB::mapDb.count(strFile) > 0)
        {
            /* mapDb contains entry for strFile, so database is already open */
            pdb = BerkeleyDB::mapDb[strFile];
        }
        else
        {
            /* Database not already open, so open it now */
            pdb = new Db(&BerkeleyDB::dbenv, 0);

            /* Opened database will support multi-threaded access */
            uint32_t fOpenFlags = DB_THREAD;

            if (fCreate)
                fOpenFlags |= DB_CREATE; // Add flag to create database file if does not exist

            ret = pdb->open(nullptr,         // Txn pointer
                            strFile.c_str(), // Filename
                            "main",          // Logical db name
                            DB_BTREE,        // Database type
                            fOpenFlags,      // Flags
                            0);

            if (ret == 0)
            {
                /* Database opened successfully. Add database to open database map */
                BerkeleyDB::mapDb[strFile] = pdb;

                if (fCreate && !Exists(std::string("version")))
                {
                    /* For new database file, write the database version as the first entry
                     * This is written even if the new database was created as read-only
                     */
                    bool fTmp = fReadOnly;
                    fReadOnly = false;
                    WriteVersion(LLD::DATABASE_VERSION);
                    fReadOnly = fTmp;
                }
            }
            else
            {
                /* Error opening db, reset db */
                delete pdb;
                pdb = nullptr;
                strFile = "";

                --BerkeleyDB::mapFileUseCount[strFile];

                throw std::runtime_error(debug::strprintf(FUNCTION, "Cannot open database file %s, error %d", strFile.c_str(), ret));
            }
        }
    }



    /* Open a cursor at the beginning of the database. */
    Dbc* BerkeleyDB::GetCursor()
    {
        if (pdb == nullptr)
            return nullptr;

        Dbc* pcursor = nullptr;

        int32_t ret = pdb->cursor(nullptr, &pcursor, 0);

        if (ret != 0)
            return nullptr;

        return pcursor;
    }


    /* Read a database key-value pair from the current cursor location. */
    int32_t BerkeleyDB::ReadAtCursor(Dbc* pcursor, DataStream& ssKey, DataStream& ssValue, uint32_t fFlags)
    {
        /* Key - Initialize with argument data for flag settings that need it */
        Dbt datKey;
        if (fFlags == DB_SET || fFlags == DB_SET_RANGE || fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
        {
            datKey.set_data((char*)ssKey.data());
            datKey.set_size(ssKey.size());
        }

        /* Value - Initialize with argument data for flag settings that need it */
        Dbt datValue;
        if (fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
        {
            datValue.set_data((char*)ssValue.data());
            datValue.set_size(ssValue.size());
        }

        /* Berkeley will allocate memory for returned data, will need to free before return */
        datKey.set_flags(DB_DBT_MALLOC);
        datValue.set_flags(DB_DBT_MALLOC);

        /* Execute cursor operation */
        int32_t ret = pcursor->get(&datKey, &datValue, fFlags);

        if (ret != 0)
            return ret;

        else if (datKey.get_data() == nullptr || datValue.get_data() == nullptr)
            return 99999;

        /* Convert to streams */
        ssKey.SetType(SER_DISK);
        ssKey.clear();
        ssKey.write((char*)datKey.get_data(), datKey.get_size());

        ssValue.SetType(SER_DISK);
        ssValue.clear();
        ssValue.write((char*)datValue.get_data(), datValue.get_size());

        /* Clear and free memory */
        memset(datKey.get_data(), 0, datKey.get_size());
        memset(datValue.get_data(), 0, datValue.get_size());

        free(datKey.get_data());
        free(datValue.get_data());

        return 0;
    }


    /* Closes and discards a cursor. After calling this method, the cursor is no longer valid for use. */
    void BerkeleyDB::CloseCursor(Dbc* pcursor)
    {
        if (pdb == nullptr)
            return;

        pcursor->close();

        return;
    }


    /*  Retrieves the most recently started database transaction. */
    DbTxn* BerkeleyDB::GetTxn()
    {
        if (!vTxn.empty())
            return vTxn.back();
        else
            return nullptr;
    }


    /* Start a new database transaction and add it to vTxn */
    bool BerkeleyDB::TxnBegin()
    {
        if (pdb == nullptr)
            return false;

        /* Start a new database transaction. Need to use raw pointer with Berkeley API */
        DbTxn* pTxn = nullptr;

        /* Begin new transaction */
        int32_t ret = BerkeleyDB::dbenv.txn_begin(GetTxn(), &pTxn, DB_TXN_WRITE_NOSYNC);

        if (pTxn == nullptr || ret != 0)
            return false;

        /* Add new transaction to the end of vTxn */
        vTxn.push_back(pTxn);

        return true;
    }


    /* Commit the transaction most recently added to vTxn */
    bool BerkeleyDB::TxnCommit()
    {
        if (pdb == nullptr)
            return false;

        auto pTxn = GetTxn();

        if (pTxn == nullptr)
            return false;

        int32_t ret = pTxn->commit(0);

        vTxn.pop_back();

        return (ret == 0);
    }


    /* Abort the transaction most recently added to vTxn, reversing any updates performed. */
    bool BerkeleyDB::TxnAbort()
    {
        if (pdb == nullptr)
            return false;

        auto pTxn = GetTxn();

        if (pTxn == nullptr)
            return false;

        int32_t ret = pTxn->abort();

        vTxn.pop_back();

        return (ret == 0);
    }


    /* Read the current value for key "version" from the database */
    bool BerkeleyDB::ReadVersion(uint32_t& nVersion)
    {
        nVersion = 0;
        return Read(std::string("version"), nVersion);
    }


    /* Writes a number into the database using the key "version". */
    bool BerkeleyDB::WriteVersion(const uint32_t nVersion)
    {
        return Write(std::string("version"), nVersion);
    }


    /* Close this instance for database access. */
    void BerkeleyDB::Close()
    {
        if (pdb == nullptr)
            return;

        /* Abort any in-progress transactions on this database */
        if (!vTxn.empty())
            vTxn.front()->abort();

        vTxn.clear();

        /* Flush database activity from memory pool to disk log */
        uint32_t nMinutes = 0;
        if (fReadOnly)
            nMinutes = 1;

        if (strFile == "addr.dat")
            nMinutes = 2;

        BerkeleyDB::dbenv.txn_checkpoint(nMinutes ? config::GetArg("-dblogsize", 100)*1024 : 0, nMinutes, 0);

        {
            LOCK(BerkeleyDB::cs_db);
            --BerkeleyDB::mapFileUseCount[strFile];
        }

        //delete pdb; //Don't delete here, pointer is stored in mapDb so handle can be reused. It is deleted in CloseDb()
        pdb = nullptr;
        strFile = "";

    }


    /* Closes down the open database handle for a database and removes it from BerkeleyDB::mapDb */
    void BerkeleyDB::CloseDb(const std::string& strFile)
    {
        /* Does not LOCK(BerkeleyDB::cd_db) -- this lock must be previously obtained before calling this methods */

        if (BerkeleyDB::mapDb.count(strFile) > 0 && BerkeleyDB::mapDb[strFile] != nullptr)
        {
            /* Close the database handle */
            auto pdb = BerkeleyDB::mapDb[strFile];
            pdb->close(0);
            delete pdb;

            BerkeleyDB::mapDb.erase(strFile);
        }
    }


    /* Flushes log file to data file for any database handles with BerkeleyDB::mapFileUseCount = 0
     * then calls CloseDb on that database.
     */
    void BerkeleyDB::DBFlush(bool fShutdown)
    {
        /* Flush log data to the actual data file on all files that are not in use */
        debug::log(0, FUNCTION, "Shutdown ", fShutdown ? "true" : "false", ")",  BerkeleyDB::fDbEnvInit ? "" : " db not started");

        {
            LOCK(BerkeleyDB::cs_db);

            if (!BerkeleyDB::fDbEnvInit)
                return;

            /* Copy mapFileUseCount so can erase without invalidating iterator */
            std::map<std::string, uint32_t> mapTempUseCount = BerkeleyDB::mapFileUseCount;

            for (auto mi = mapTempUseCount.cbegin(); mi != mapTempUseCount.cend(); mi++)
            {
                const std::string strFile = (*mi).first;
                const uint32_t nRefCount = (*mi).second;

                debug::log(2, FUNCTION, strFile, " refcount=", nRefCount);

                if (nRefCount == 0)
                {
                    /* We have lock on BerkeleyDB::cs_db so can call CloseDb safely */
                    CloseDb(strFile);

                    /* Flush log data to the dat file and detach the file */
                    debug::log(2, FUNCTION, strFile, " checkpoint");
                    BerkeleyDB::dbenv.txn_checkpoint(0, 0, 0);

                    debug::log(2, FUNCTION, strFile, " detach");
                    BerkeleyDB::dbenv.lsn_reset(strFile.c_str(), 0);

                    debug::log(2, FUNCTION, strFile, " closed");
                    BerkeleyDB::mapFileUseCount.erase(strFile);
                }
            }

            if (fShutdown)
            {
                char** listp;

                if (BerkeleyDB::mapFileUseCount.empty())
                    BerkeleyDB::dbenv.log_archive(&listp, DB_ARCH_REMOVE);
            }
        }

        if (fShutdown)
        {
            /* Need to call this outside of lock scope */
            EnvShutdown();
        }
    }


    /* Rewrites a database file by copying all contents */
    bool BerkeleyDB::DBRewrite(const std::string& strFile, const char* pszSkip)
    {
        if (config::fShutdown)
            return false;

        bool fProcessSuccess = true;
        int32_t dbReturn = 0;

        /* Define temporary file name where copy will be written */
        std::string strFileRewrite = strFile + ".rewrite";

        {
            /* Lock database access for full rewrite process.
             * This process does not use a BerkeleyDB instance and manually calls Berkeley methods
             * to avoid use of locks within BerkeleyDB itself. Adds work to do ReadAtCursor(), but
             * allows a proper lock scope.
             */
            LOCK(BerkeleyDB::cs_db);

            if (BerkeleyDB::mapFileUseCount.count(strFile) != 0 && BerkeleyDB::mapFileUseCount[strFile] != 0)
            {
                /* Database file in use. Cannot rewrite */
                return false;
            }
            else
            {
                /* We have lock on BerkeleyDB::cs_db so can call CloseDb safely */
                CloseDb(strFile);

                /* Flush log data to the dat file and detach the file */
                BerkeleyDB::dbenv.txn_checkpoint(0, 0, 0);
                BerkeleyDB::dbenv.lsn_reset(strFile.c_str(), 0);
                BerkeleyDB::mapFileUseCount.erase(strFile);
            }

            debug::log(0, FUNCTION, "Rewriting ", strFile.c_str(), "...");

            Db* pdbSource = new Db(&BerkeleyDB::dbenv, 0);
            Db* pdbCopy = new Db(&BerkeleyDB::dbenv, 0);

            /* Open database handle to temp file */
            dbReturn = pdbSource->open(nullptr,              // Txn pointer
                                       strFile.c_str(),      // Filename
                                       "main",               // Logical db name
                                       DB_BTREE,             // Database type
                                       DB_RDONLY,            // Flags
                                       0);

            if (dbReturn != 0)
            {
                debug::log(0, FUNCTION, "Cannot open source database file ", strFile.c_str());
                return false;
            }

            /* Open database handle to temp file */
            dbReturn = pdbCopy->open(nullptr,                // Txn pointer
                                     strFileRewrite.c_str(), // Filename
                                     "main",                 // Logical db name
                                     DB_BTREE,               // Database type
                                     DB_CREATE,              // Flags
                                     0);

            if (dbReturn != 0)
            {
                debug::log(0, FUNCTION, "Cannot create target database file ", strFileRewrite.c_str());
                return false;
            }

            /* Get cursor to read source file */
            Dbc* pcursor = nullptr;
            dbReturn = pdbSource->cursor(nullptr, &pcursor, 0);

            if (dbReturn != 0 || pcursor == nullptr)
            {
                debug::log(0, FUNCTION, "Failure opening cursor on source database file ", strFile.c_str());
                return false;
            }

            while (fProcessSuccess)
            {
                /* This section duplicates the process in BerkeleyDB::ReadAtCursor without the need for a BerkeleyDB instance
                 * Code logic from BerkeleyDB::ReadAtCursor that is not needed for this specific process is left out.
                 */
                DataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
                DataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);
                Dbt datKey;
                Dbt datValue;

                /* Berkeley will allocate memory for returned data, will need to free before return */
                datKey.set_flags(DB_DBT_MALLOC);
                datValue.set_flags(DB_DBT_MALLOC);

                /* Execute cursor operation */
                dbReturn = pcursor->get(&datKey, &datValue, DB_NEXT);

                if (dbReturn == DB_NOTFOUND)
                {
                    /* No more data */
                    pcursor->close();
                    fProcessSuccess = true;
                    break;
                }
                else if (datKey.get_data() == nullptr || datValue.get_data() == nullptr)
                {
                    /* No data reading cursor */
                    pcursor->close();
                    fProcessSuccess = false;
                    debug::log(0, FUNCTION, "No data reading cursor on database file ", strFile.c_str());
                    break;
                }
                else if (dbReturn == 0)
                {
                    /* Convert to streams */
                    ssKey.SetType(SER_DISK);
                    ssKey.clear();
                    ssKey.write((char*)datKey.get_data(), datKey.get_size());

                    ssValue.SetType(SER_DISK);
                    ssValue.clear();
                    ssValue.write((char*)datValue.get_data(), datValue.get_size());

                    /* Clear and free memory */
                    memset(datKey.get_data(), 0, datKey.get_size());
                    memset(datValue.get_data(), 0, datValue.get_size());

                    free(datKey.get_data());
                    free(datValue.get_data());
                }
                else if (dbReturn != 0)
                {
                    /* Error reading cursor */
                    pcursor->close();
                    fProcessSuccess = false;
                    debug::log(0, FUNCTION, "Failure reading cursor on database file ", strFile.c_str());
                    break;
                }

                /* Skip any key value defined by pszSkip argument */
                if (pszSkip != nullptr && strncmp((char*)ssKey.data(), pszSkip, std::min(ssKey.size(), strlen(pszSkip))) == 0)
                    continue;

                /* Don't copy the version, instead use latest version */
                if (strncmp((char*)ssKey.data(), "version", 7) == 0)
                {
                    /* Update version */
                    ssValue.clear();
                    ssValue << LLD::DATABASE_VERSION;
                }

                Dbt writeDatKey((char*)ssKey.data(), ssKey.size());
                Dbt writeDatValue((char*)ssValue.data(), ssValue.size());

                /* Write the data to temp file */
                dbReturn = pdbCopy->put(nullptr, &writeDatKey, &writeDatValue, DB_NOOVERWRITE);

                if (dbReturn != 0)
                {
                    pcursor->close();
                    fProcessSuccess = false;
                    debug::log(0, FUNCTION, "Failure writing target database file ", strFile.c_str());
                    break;
                }
            }

            /* Rewrite complete. Close the databases */
            pdbSource->close(0);
            delete pdbSource;

            pdbCopy->close(0);
            delete pdbCopy;

            /* Flush log data to the dat files */
            BerkeleyDB::dbenv.txn_checkpoint(0, 0, 0);
            BerkeleyDB::dbenv.lsn_reset(strFile.c_str(), 0);
            BerkeleyDB::dbenv.lsn_reset(strFileRewrite.c_str(), 0);

            if (fProcessSuccess)
            {
                /* Remove original database file */
                Db dbOld(&BerkeleyDB::dbenv, 0);
                if (dbOld.remove(strFile.c_str(), nullptr, 0) != 0)
                {
                    debug::log(0, FUNCTION, "Unable to remove old database file ", strFile.c_str());
                    fProcessSuccess = false;
                }
            }

            if (fProcessSuccess)
            {
                /* Rename temp file to original file name */
                Db dbNew(&BerkeleyDB::dbenv, 0);
                if (dbNew.rename(strFileRewrite.c_str(), nullptr, strFile.c_str(), 0) != 0)
                {
                    debug::log(0, FUNCTION, "Unable to rename database file ", strFileRewrite.c_str(), " to ", strFile.c_str());
                    fProcessSuccess = false;
                }
            }

            if (!fProcessSuccess)
                debug::log(0, FUNCTION, "Rewriting of ", strFile.c_str(), " FAILED!");

        } //End lock scope

        return fProcessSuccess;
    }


    /* Called to shut down the Berkeley database environment in BerkeleyDB:dbenv */
    void BerkeleyDB::EnvShutdown()
    {
        {
            /* Lock database access before closing databases and shutting down environment */
            LOCK(BerkeleyDB::cs_db);

            if (!BerkeleyDB::fDbEnvInit)
                return;

            debug::log(0, FUNCTION, "Shutting down Legacy Berkeley database environment");

            /* Ensure all open db handles are closed and pointers deleted.
             * CloseDb calls erase() on mapDb so build list of open files first
             * thus avoiding any problems with erase invalidating iterators.
             */
            std::vector<std::string> dbFilesToClose;
            for (auto& mapEntry : mapDb)
            {
                if (mapEntry.second != nullptr)
                    dbFilesToClose.push_back(mapEntry.first);
            }

            for (auto& dbFile : dbFilesToClose)
                BerkeleyDB::CloseDb(dbFile);

            /* Shut down the database environment */
            BerkeleyDB::fDbEnvInit = false;

            try
            {
                BerkeleyDB::dbenv.close(0);
            }
            catch(const DbException& e)
            {
                debug::log(0, FUNCTION, "Exception: ", e.what(), "(", e.get_errno(), ")");
            }

        } //End lock scope
    }

}
