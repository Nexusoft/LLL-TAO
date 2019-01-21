/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

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

#ifndef WIN32
#include "sys/stat.h"
#endif


namespace Legacy
{

    /* Initialize namespace-scope variable */
    bool fDetachDB = false;

    /* Initialization for class static variables */
    DbEnv CDB::dbenv((uint32_t)0);

    bool CDB::fDbEnvInit = false;

    std::map<std::string, uint32_t> CDB::mapFileUseCount; //initializes empty map

    std::map<std::string, Db*> CDB::mapDb;  //initializes empty map

    std::mutex CDB::cs_db; //initializes the mutex


    /* Constructor */
    /* Initializes database environment on first use */
    CDB::CDB(const char *pszFileIn, const char* pszMode) : pdb(nullptr)
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
    CDB::CDB(const std::string strFileIn, const char* pszMode) : pdb(nullptr)
    {
        /* Passing an empty string will initialize a null instance */
        if (strFileIn.empty())
            return;

        Init(strFileIn, pszMode);

        return;
    }


    /* Destructor */
    CDB::~CDB()
    {
        Close();
    }


    /* Performs work of initialization for constructors. */
    void CDB::Init(const std::string strFileIn, const char* pszMode)
    {
        int32_t ret;

        LOCK(CDB::cs_db); // Need to lock before test fDbEnvInit

        if (!CDB::fDbEnvInit)
        {
            /* Need to initialize database environment. This is only done once upon construction of the first CDB instance */
            if (config::fShutdown)
                return;

            std::string pathDataDir(config::GetDataDir());
            std::string pathLogDir(pathDataDir + "/database");
            filesystem::create_directory(pathLogDir);

            std::string pathErrorFile(pathDataDir + "/db.log");
            //debug::log(0, FUNCTION, "dbenv.open LogDir=", pathLogDir, " ErrorFile=",  pathErrorFile);

            uint32_t nDbCache = config::GetArg("-dbcache", 25);

            CDB::dbenv.set_lg_dir(pathLogDir.c_str());
            CDB::dbenv.set_cachesize(nDbCache / 1024, (nDbCache % 1024)*1048576, 1);
            CDB::dbenv.set_lg_bsize(1048576);
            CDB::dbenv.set_lg_max(10485760);
            CDB::dbenv.set_lk_max_locks(10000);
            CDB::dbenv.set_lk_max_objects(10000);
            //CDB::dbenv.set_errfile(fopen(pathErrorFile.c_str(), "a")); /// debug
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
            uint32_t dbMode = S_IRUSR | S_IWUSR;
#else
            uint32_t dbMode = 0;
#endif

            /* Open the Berkely DB environment */
            ret = CDB::dbenv.open(pathDataDir.c_str(), dbFlags, dbMode);

            if (ret > 0)
                throw std::runtime_error(debug::strprintf("CDB() : error %d opening database environment", ret));

            CDB::fDbEnvInit = true;
        }

        /* Initialize current CDB instance */
        strFile = strFileIn;

        /* Usage count will be incremented whether we use pdb from mapDb or open a new one */
        if (CDB::mapFileUseCount.count(strFile) == 0)
            CDB::mapFileUseCount[strFile] = 1;
        else
            ++CDB::mapFileUseCount[strFile];

        /* Extract mode settings */
        bool fCreate = (strchr(pszMode, 'c') != nullptr);
        bool fWrite = (strchr(pszMode, 'w') != nullptr);
        bool fAppend = (strchr(pszMode, '+') != nullptr);

        /* Set database read-only if not write or append mode */
        fReadOnly = !(fWrite || fAppend);

        if (CDB::mapDb.count(strFile) > 0)
        {
            /* mapDb contains entry for strFile, so database is already open */
            pdb = CDB::mapDb[strFile];
        }
        else
        {
            /* Database not already open, so open it now */
            pdb = new Db(&CDB::dbenv, 0);

            /* Opened database will support multi-threaded access */
            uint32_t fWrite = DB_THREAD;

            if (fCreate)
                fWrite |= DB_CREATE; // Add flag to create database file if does not exist

            ret = pdb->open(nullptr,         // Txn pointer
                            strFile.c_str(), // Filename
                            "main",          // Logical db name
                            DB_BTREE,        // Database type
                            fWrite,          // Flags
                            0);

            if (ret == 0)
            {
                /* Database opened successfully. Add database to open database map */
                CDB::mapDb[strFile] = pdb;

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

                --CDB::mapFileUseCount[strFile];

                throw std::runtime_error(debug::strprintf("CDB() : can't open database file %s, error %d", strFile.c_str(), ret));
            }
        }
    }



    /* Open a cursor at the beginning of the database. */
    Dbc* CDB::GetCursor()
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
    int32_t CDB::ReadAtCursor(Dbc* pcursor, DataStream& ssKey, DataStream& ssValue, uint32_t fFlags)
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
    void CDB::CloseCursor(Dbc* pcursor)
    {
        if (pdb == nullptr)
            return;

        pcursor->close();

        return;
    }


    /*  Retrieves the most recently started database transaction. */
    DbTxn* CDB::GetTxn()
    {
        if (!vTxn.empty())
            return vTxn.back();
        else
            return nullptr;
    }


    /* Start a new database transaction and add it to vTxn */
    bool CDB::TxnBegin()
    {
        if (pdb == nullptr)
            return false;

        /* Start a new database transaction. Need to use raw pointer with Berkeley API */
        DbTxn* pTxn = nullptr;

        /* Begin new transaction */
        int32_t ret = CDB::dbenv.txn_begin(GetTxn(), &pTxn, DB_TXN_WRITE_NOSYNC);

        if (pTxn == nullptr || ret != 0)
            return false;

        /* Add new transaction to the end of vTxn */
        vTxn.push_back(pTxn);

        return true;
    }


    /* Commit the transaction most recently added to vTxn */
    bool CDB::TxnCommit()
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
    bool CDB::TxnAbort()
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
    bool CDB::ReadVersion(uint32_t& nVersion)
    {
        nVersion = 0;
        return Read(std::string("version"), nVersion);
    }


    /* Writes a number into the database using the key "version". */
    bool CDB::WriteVersion(uint32_t nVersion)
    {
        return Write(std::string("version"), nVersion);
    }


    /* Close this instance for database access. */
    void CDB::Close()
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

        CDB::dbenv.txn_checkpoint(nMinutes ? config::GetArg("-dblogsize", 100)*1024 : 0, nMinutes, 0);

        {
            LOCK(CDB::cs_db);
            --CDB::mapFileUseCount[strFile];
        }

        delete pdb;
        pdb = nullptr;
        strFile = "";

    }


    /* Closes down the open database handle for a database and removes it from CDB::mapDb */
    void CDB::CloseDb(const std::string& strFile)
    {
        {
            LOCK(CDB::cs_db);

            if (CDB::mapDb.count(strFile) > 0 && CDB::mapDb[strFile] != nullptr)
            {
                /* Close the database handle */
                auto pdb = CDB::mapDb[strFile];
                pdb->close(0);
                delete pdb;

                CDB::mapDb.erase(strFile);
            }
        }
    }


    /* Flushes log file to data file for any database handles with CDB::mapFileUseCount = 0
     * then calls CloseDb on that database.
     */
    void CDB::DBFlush(bool fShutdown)
    {
        /* Flush log data to the actual data file on all files that are not in use */
        debug::log(0, "DBFlush(", fShutdown ? "true" : "false", ")",  CDB::fDbEnvInit ? "" : " db not started");

        if (!CDB::fDbEnvInit)
            return;

        {
            LOCK(CDB::cs_db);

            for (auto mi = CDB::mapFileUseCount.cbegin(); mi != CDB::mapFileUseCount.cend(); /*no increment */)
            {
                const std::string strFile = (*mi).first;
                const uint32_t nRefCount = (*mi).second;

                debug::log(0, strFile, " refcount=", nRefCount);

                if (nRefCount == 0)
                {
                    /* Move log data to the dat file */
                    CloseDb(strFile);

                    debug::log(0, strFile, " checkpoint");
                    CDB::dbenv.txn_checkpoint(0, 0, 0);

                    if (strFile != "addr.dat" || fDetachDB)
                    {
                        debug::log(0, strFile, " detach");
                        CDB::dbenv.lsn_reset(strFile.c_str(), 0);
                    }

                    debug::log(0, strFile, " closed");
                    CDB::mapFileUseCount.erase(mi++);
                }
                else
                    mi++;
            }

            if (config::fShutdown)
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


    /* Rewrites a database file by copying all contents */
    bool CDB::DBRewrite(const std::string& strFile, const char* pszSkip)
    {
        if (!config::fShutdown)
        {
            LOCK(CDB::cs_db);

            if (CDB::mapFileUseCount.count(strFile) == 0 || CDB::mapFileUseCount[strFile] == 0)
            {
                /* Flush log data to the dat file and detach the file */
                CloseDb(strFile);
                CDB::dbenv.txn_checkpoint(0, 0, 0);
                CDB::dbenv.lsn_reset(strFile.c_str(), 0);
                CDB::mapFileUseCount.erase(strFile);

                bool fOpenSuccess = true;
                bool fProcessSuccess = true;
                debug::log(0, "Rewriting ", strFile.c_str(), "...");

                /* Define temporary file name where copy will be written */
                std::string strFileRes = strFile + ".rewrite";

                /* Surround usage of db with extra {} */
                {
                    CDB dbToRewrite(strFile.c_str(), "r");
                    Db* pdbCopy = new Db(&CDB::dbenv, 0);

                    /* Open database handle to temp file */
                    int32_t ret = pdbCopy->open(nullptr,            // Txn pointer
                                                strFileRes.c_str(), // Filename
                                                "main",             // Logical db name
                                                DB_BTREE,           // Database type
                                                DB_CREATE,          // Flags
                                                0);
                    if (ret > 0)
                    {
                        debug::log(0, "Cannot create database file ", strFileRes.c_str());
                        fOpenSuccess = false;
                    }

                    auto pcursor = dbToRewrite.GetCursor();

                    if (pcursor != nullptr)
                        while (fProcessSuccess)
                        {
                            DataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
                            DataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);

                            /* Read next key-value pair to copy */
                            int32_t ret = dbToRewrite.ReadAtCursor(pcursor, ssKey, ssValue, DB_NEXT);

                            if (ret == DB_NOTFOUND)
                            {
                                /* No more data */
                                pcursor->close();
                                break;
                            }
                            else if (ret != 0)
                            {
                                /* Error reading cursor */
                                pcursor->close();
                                fProcessSuccess = false;
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

                            Dbt datKey((char*)ssKey.data(), ssKey.size());
                            Dbt datValue((char*)ssValue.data(), ssValue.size());

                            /* Write the data to temp file */
                            int32_t ret2 = pdbCopy->put(nullptr, &datKey, &datValue, DB_NOOVERWRITE);

                            if (ret2 > 0)
                                fProcessSuccess = false;
                        }

                    if (fOpenSuccess)
                    {
                        dbToRewrite.Close();
                        CloseDb(strFile);

                        if (pdbCopy->close(0) != 0)
                            fProcessSuccess = false;

                        delete pdbCopy;
                    }
                }

                if (fProcessSuccess)
                {
                    /* Remove original database file */
                    Db dbA(&CDB::dbenv, 0);
                    if (dbA.remove(strFile.c_str(), nullptr, 0) != 0)
                        fProcessSuccess = false;
                }

                if (fProcessSuccess)
                {
                    /* Rename temp file to original file name */
                    Db dbB(&CDB::dbenv, 0);
                    if (dbB.rename(strFileRes.c_str(), nullptr, strFile.c_str(), 0) != 0)
                        fProcessSuccess = false;
                }

                if (!fProcessSuccess)
                    debug::log(0, "Rewriting of ", strFile.c_str(), " FAILED!");

                return fProcessSuccess;
            }
        }

        /* Return false if fShutdown */
        return false;
    }


    /* Called to shut down the Berkeley database environment in CDB:dbenv */
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
            debug::log(0, "EnvShutdown exception: ", e.what(), "(", e.get_errno(), ")");
        }

        /* Use of DB_FORCE should not be necessary after calling dbenv.close() but included in case there is an exception */
        CDB::dbenv.remove(config::GetDataDir().c_str(), DB_FORCE);
    }

}
