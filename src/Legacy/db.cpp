/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/wallet/db.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/filesystem.h>

#include <sys/stat.h>

#include <cmath>
#include <exception>


namespace Legacy
{

    /* Static variable initialization */
    std::atomic<bool> BerkeleyDB::fDbInitialized(false);


    /* Constructor */
    /* Initializes database environment on first use */
    BerkeleyDB::BerkeleyDB(const std::string& strFileIn)
    : cs_db()
    , dbenv(nullptr)
    , pdb(nullptr)
    , strDbFile(strFileIn)
    , vTxn()
    {
        /* Passing an empty string is invalid */
        if(strFileIn.empty())
        {
            debug::error(FUNCTION, "Invalid empty string for Berkeley database file name");

            throw std::runtime_error(debug::safe_printstr(FUNCTION, "Invalid empty string for Berkeley database file name"));
        }

        Init();

        return;
    }


    /* Destructor */
    BerkeleyDB::~BerkeleyDB()
    {
        /* Destructor of instance should only be called after pdb and dbenv already invalidated, but check it here as a precaution */
        LOCK(cs_db);

        if(pdb != nullptr)
            CloseHandle();

        if(dbenv != nullptr)
        {
             /* Flush log data to the dat file and detach the file */
            dbenv->txn_checkpoint(0, 0, 0);
            dbenv->lsn_reset(strDbFile.c_str(), 0);

            /* Remove log files */
            char** listp;
            dbenv->log_archive(&listp, DB_ARCH_REMOVE);

            /* Shut down the database environment */
            try
            {
                dbenv->close(0);
            }
            catch(const DbException& e)
            {
                debug::log(0, FUNCTION, "Exception: ", e.what(), "(", e.get_errno(), ")");
            }

            delete dbenv;
            dbenv = nullptr;
        }
    }


    /* Private Methods */

    /* Performs work of initialization for constructor. */
    void BerkeleyDB::Init()
    {
        int32_t ret = 0;

        {
            LOCK(cs_db);

            if(dbenv == nullptr)  //Should be true when this is called
            {
                if(config::fShutdown.load())
                    return;

                std::string pathDataDir(config::GetDataDir());
                std::string pathLogDir(pathDataDir + "database");
                filesystem::create_directory(pathLogDir);

                std::string pathErrorFile(pathDataDir + "db.log");

                uint32_t nDbCache = config::GetArg("-dbcache", 25);

                dbenv = new DbEnv((uint32_t)0);

                dbenv->set_lg_dir(pathLogDir.c_str());
                dbenv->set_cachesize(nDbCache / 1024, (nDbCache % 1024)*1048576, 1);
                dbenv->set_lg_bsize(1048576);
                dbenv->set_lg_max(10485760);
                //dbenv->set_lk_max_locks(10000);
                //dbenv->set_lk_max_objects(10000);
                //dbenv->set_errfile(fopen(pathErrorFile.c_str(), "a")); /// debug
                dbenv->set_flags(DB_TXN_WRITE_NOSYNC, 1);
                dbenv->set_flags(DB_AUTO_COMMIT, 1);
                dbenv->log_set_config(DB_LOG_AUTO_REMOVE, 1);

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
                                    // DB_INIT_LOCK  |
                                    DB_INIT_LOG   |
                                    DB_INIT_MPOOL |
                                    DB_INIT_TXN   |
                                   // DB_THREAD     |
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
                try
                {
                    ret = dbenv->open(pathDataDir.c_str(), dbFlags, dbMode);
                }
                catch (const std::exception& e)
                {
                    delete dbenv;
                    dbenv = nullptr;

                    debug::error(FUNCTION, "Exception initializing Berkeley database environment for ", strDbFile, ": ", e.what());

                    throw std::runtime_error(
                            debug::safe_printstr(FUNCTION, "Exception initializing Berkeley database environment for ", strDbFile, ": ", e.what()));

                }

                if(ret != 0)
                {
                    delete dbenv;
                    dbenv = nullptr;

                    debug::error(FUNCTION, "Error ", ret, " initializing Berkeley database environment for ", strDbFile);

                    throw std::runtime_error(
                            debug::safe_printstr(FUNCTION, "Error ", ret, " initializing Berkeley database environment for ", strDbFile));
                }

                debug::log(0, FUNCTION, "Initialized Legacy Berkeley database environment for ", strDbFile);
            }


            /* Pre-open the database and initialize handle for its use. Creates file if it doesn't exist. */
            OpenHandle();

        } //end cs_db lock


        /* Successfully opening db for the first time will create the database file.
         * When file is new, it will not yet contain version. Write the database version as the first entry.
         *
         * These methods lock, so call must be outside lock scope
         */
        if(!Exists(std::string("version")))
            WriteVersion(LLD::DATABASE_VERSION);

    }


    /* Initializes the handle for the current database instance, if not currently open. */
    void BerkeleyDB::OpenHandle()
    {
        if(pdb != nullptr)
            return;

        int32_t ret = 0;

        /* Database not open, so open it now */
        pdb = new Db(dbenv, 0);

        /* Database will support multi-threaded access and create database file if does not exist*/
        //uint32_t fOpenFlags = DB_THREAD | DB_CREATE;

        ret = pdb->open(nullptr,           // Txn pointer
                        strDbFile.c_str(), // Filename
                        "main",            // Logical db name
                        DB_BTREE,          // Database type
                        DB_CREATE,        // Flags
                        0);

        if(ret != 0)
        {
            /* Error opening db, reset db */
            delete pdb;
            pdb = nullptr;

            debug::error(FUNCTION, "Cannot open database file ", strDbFile, ", error ", ret);

            throw std::runtime_error(debug::safe_printstr(FUNCTION, "Cannot open database file ", strDbFile, ", error ", ret));
        }
    }


    /* Close this instance for database access. */
    void BerkeleyDB::CloseHandle()
    {
        if(pdb == nullptr)
            return;

        /* Abort any in-progress transactions on this database */
        if(!vTxn.empty())
            vTxn.front()->abort();

        vTxn.clear();

        /* Flush database activity from memory pool to disk log */
        uint32_t nMinutes = 0;

        dbenv->txn_checkpoint(nMinutes ? config::GetArg("-dblogsize", 100)*1024 : 0, nMinutes, 0);

        pdb->close(0);

        /* Current pdb no longer valid. Remove it */
        delete pdb;
        pdb = nullptr;
    }


    /*  Retrieves the most recently started database transaction. */
    DbTxn* BerkeleyDB::GetTxn()
    {
        if(!vTxn.empty())
            return vTxn.back();
        else
            return nullptr;
    }


    /* Public Methods */

    /* Open a cursor at the beginning of the database. */
    Dbc* BerkeleyDB::GetCursor()
    {
        LOCK(cs_db);

        if(pdb == nullptr)
            OpenHandle();

        Dbc* pcursor = nullptr;

        int32_t ret = pdb->cursor(nullptr, &pcursor, 0);

        if(ret != 0)
            return nullptr;

        return pcursor;
    }


    /* Read a database key-value pair from the current cursor location. */
    int32_t BerkeleyDB::ReadAtCursor(Dbc* pcursor, DataStream& ssKey, DataStream& ssValue, uint32_t fFlags)
    {
        LOCK(cs_db);

        if(pcursor == nullptr)
            return 99998;

        /* Key - Initialize with argument data for flag settings that need it */
        Dbt datKey;
        if(fFlags == DB_SET || fFlags == DB_SET_RANGE || fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
        {
            datKey.set_data((char*)ssKey.data());
            datKey.set_size(ssKey.size());
        }

        /* Value - Initialize with argument data for flag settings that need it */
        Dbt datValue;
        if(fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
        {
            datValue.set_data((char*)ssValue.data());
            datValue.set_size(ssValue.size());
        }

        /* Berkeley will allocate memory for returned data, will need to free before return */
        datKey.set_flags(DB_DBT_MALLOC);
        datValue.set_flags(DB_DBT_MALLOC);

        /* Execute cursor operation */
        int32_t ret = pcursor->get(&datKey, &datValue, fFlags);

        if(ret != 0)
            return ret;

        else if(datKey.get_data() == nullptr || datValue.get_data() == nullptr)
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
        LOCK(cs_db);

        if(pcursor == nullptr)
            return;

        pcursor->close();

        return;
    }


    /* Start a new database transaction and add it to vTxn */
    bool BerkeleyDB::TxnBegin()
    {
        LOCK(cs_db);

        if(pdb == nullptr)
            OpenHandle();

        /* Start a new database transaction. Need to use raw pointer with Berkeley API */
        DbTxn* pTxn = nullptr;

        /* Begin new transaction */
        int32_t ret = dbenv->txn_begin(GetTxn(), &pTxn, DB_TXN_WRITE_NOSYNC);

        if(pTxn == nullptr || ret != 0)
            return false;

        /* Add new transaction to the end of vTxn */
        vTxn.push_back(pTxn);

        return true;
    }


    /* Commit the transaction most recently added to vTxn */
    bool BerkeleyDB::TxnCommit()
    {
        LOCK(cs_db);

        auto pTxn = GetTxn();

        if(pTxn == nullptr)
            return false;

        int32_t ret = pTxn->commit(0);

        vTxn.pop_back();

        return (ret == 0);
    }


    /* Abort the transaction most recently added to vTxn, reversing any updates performed. */
    bool BerkeleyDB::TxnAbort()
    {
        LOCK(cs_db);

        auto pTxn = GetTxn();

        if(pTxn == nullptr)
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


    /* Flushes data to the data file for the current database. */
    void BerkeleyDB::DBFlush()
    {
        LOCK(cs_db);

        if(dbenv == nullptr)
            return;

        /* Close any open database activity. Handle will need to be reopened after flush */
        CloseHandle();

        debug::log(0, FUNCTION, "Flushing ", strDbFile);
        int64_t nStart = runtime::timestamp(true);

        /* Flush wallet file so it's self contained */
        dbenv->txn_checkpoint(0, 0, 0);
        dbenv->lsn_reset(strDbFile.c_str(), 0);

        debug::log(0, FUNCTION, "Flushed ", strDbFile, " in ", runtime::timestamp(true) - nStart, " ms");

    }


    /* Rewrites the database file by copying all contents to a new file. */
    bool BerkeleyDB::DBRewrite()
    {
        if(config::fShutdown)
            return false; // Don't start rewrite if shutdown in progress

        bool fProcessSuccess = true;
        int32_t dbReturn = 0;

        /* Define temporary file name where copy will be written */
        std::string strDbFileRewrite = strDbFile + ".rewrite";

        {
            /* Lock database access for full rewrite process. */
            LOCK(cs_db);

            CloseHandle();

            /* Flush log data to the dat file and detach the file */
            dbenv->txn_checkpoint(0, 0, 0);
            dbenv->lsn_reset(strDbFile.c_str(), 0);

            debug::log(0, FUNCTION, "Rewriting ", strDbFile, "...");

            Db* pdbSource = new Db(dbenv, 0);
            Db* pdbCopy = new Db(dbenv, 0);

            /* Open database handle to temp file */
            dbReturn = pdbSource->open(nullptr,              // Txn pointer
                                       strDbFile.c_str(),    // Filename
                                       "main",               // Logical db name
                                       DB_BTREE,             // Database type
                                       DB_RDONLY,            // Flags
                                       0);

            if(dbReturn != 0)
            {
                debug::log(0, FUNCTION, "Cannot open source database file ", strDbFile.c_str());

                delete pdbSource;
                delete pdbCopy;
                return false;
            }

            /* Open database handle to temp file */
            dbReturn = pdbCopy->open(nullptr,                  // Txn pointer
                                     strDbFileRewrite.c_str(), // Filename
                                     "main",                   // Logical db name
                                     DB_BTREE,                 // Database type
                                     DB_CREATE,                // Flags
                                     0);

            if(dbReturn != 0)
            {
                debug::log(0, FUNCTION, "Cannot create target database file ", strDbFileRewrite.c_str());

                pdbSource->close(0);
                delete pdbSource;
                delete pdbCopy;
                return false;
            }

            /* Get cursor to read source file */
            Dbc* pcursor = nullptr;
            dbReturn = pdbSource->cursor(nullptr, &pcursor, 0);

            if(dbReturn != 0 || pcursor == nullptr)
            {
                debug::error(FUNCTION, "Failure opening cursor on source database file ", strDbFile.c_str());

                pdbSource->close(0);
                delete pdbSource;
                pdbCopy->close(0);
                delete pdbCopy;
                return false;
            }

            while(fProcessSuccess)
            {
                /* This section directly codes all cursor, read, and write operations without using their corresponding
                 * methods, allowing it to keep hold of the cs_db mutex the entire time without lock conflicts.
                 */
                DataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
                DataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);
                Dbt datKey;
                Dbt datValue;

                /* Berkeley will allocate memory for returned data, will need to free before return */
                datKey.set_flags(DB_DBT_MALLOC);
                datValue.set_flags(DB_DBT_MALLOC);

                /* Read next entry from source file */
                dbReturn = pcursor->get(&datKey, &datValue, DB_NEXT);

                if(dbReturn == DB_NOTFOUND)
                {
                    /* No more data */
                    fProcessSuccess = true;
                    break;
                }
                else if(datKey.get_data() == nullptr || datValue.get_data() == nullptr)
                {
                    /* No data reading cursor */
                    fProcessSuccess = false;
                    debug::error(FUNCTION, "No data reading cursor on database file ", strDbFile.c_str());
                    break;
                }
                else if(dbReturn == 0)
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
                else if(dbReturn != 0)
                {
                    /* Error reading cursor */
                    fProcessSuccess = false;
                    debug::error(FUNCTION, "Failure reading cursor on database file ", strDbFile);
                    break;
                }

                /* Don't copy the version, instead use latest version */
                if(strncmp((char*)ssKey.data(), "version", 7) == 0)
                {
                    /* Update version */
                    ssValue.clear();
                    ssValue << LLD::DATABASE_VERSION;
                }

                Dbt writeDatKey((char*)ssKey.data(), ssKey.size());
                Dbt writeDatValue((char*)ssValue.data(), ssValue.size());

                /* Write the data to temp file */
                dbReturn = pdbCopy->put(nullptr, &writeDatKey, &writeDatValue, DB_NOOVERWRITE);

                if(dbReturn != 0)
                {
                    fProcessSuccess = false;
                    debug::error(FUNCTION, "Failure writing target database file ", strDbFile);
                    break;
                }
            }

            pcursor->close();

            /* Rewrite complete. Close the databases */
            pdbSource->close(0);
            delete pdbSource;

            pdbCopy->close(0);
            delete pdbCopy;

            /* Flush the data files */
            dbenv->txn_checkpoint(0, 0, 0);
            dbenv->lsn_reset(strDbFile.c_str(), 0);
            dbenv->lsn_reset(strDbFileRewrite.c_str(), 0);

            if(fProcessSuccess)
            {
                /* Remove original database file */
                Db dbOld(dbenv, 0);
                if(dbOld.remove(strDbFile.c_str(), nullptr, 0) != 0)
                {
                    debug::error(FUNCTION, "Unable to remove old database file ", strDbFile);
                    fProcessSuccess = false;
                }
            }

            if(fProcessSuccess)
            {
                /* Rename temp file to original file name */
                Db dbNew(dbenv, 0);
                if(dbNew.rename(strDbFileRewrite.c_str(), nullptr, strDbFile.c_str(), 0) != 0)
                {
                    debug::error(FUNCTION, "Unable to rename database file ", strDbFileRewrite.c_str(), " to ", strDbFile);
                    fProcessSuccess = false;
                }
            }

            if(!fProcessSuccess)
                debug::log(0, FUNCTION, "Rewriting of ", strDbFile, " failed");

        } //End lock scope

        return fProcessSuccess;
    }


    /* Shut down the Berkeley database environment for this instance. */
    void BerkeleyDB::Shutdown()
    {
        debug::log(0, FUNCTION, "Shutting down ", strDbFile);

        LOCK(cs_db);

        CloseHandle();

        if(dbenv != nullptr)
        {
             /* Flush log data to the dat file and detach the file */
            debug::log(2, FUNCTION, strDbFile, " checkpoint");
            dbenv->txn_checkpoint(0, 0, 0);

            debug::log(2, FUNCTION, strDbFile, " detach");
            dbenv->lsn_reset(strDbFile.c_str(), 0);

            debug::log(2, FUNCTION, strDbFile, " closed");

            // /* Remove log files */
            char** listp;
            dbenv->log_archive(&listp, DB_ARCH_REMOVE);

            /* Shut down the database environment */
            try
            {
                dbenv->close(0);
            }
            catch(const DbException& e)
            {
                debug::log(0, FUNCTION, "Exception: ", e.what(), "(", e.get_errno(), ")");
            }

            delete dbenv;
            dbenv = nullptr;
        }
    }


    /* Static methods */

    /* Retrieves the BerkeleyDB instance that corresponds to a given database file. */
    BerkeleyDB& BerkeleyDB::GetInstance(const std::string& strFileIn)
    {
        if(!BerkeleyDB::fDbInitialized.load() && strFileIn.empty())
        {
            debug::error(FUNCTION, "Berkeley database initialization missing file name");
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "Berkeley database initialization missing file name"));
        }

        /* This will create an initialized, database instance on first call to GetInstance() */
        static BerkeleyDB dbInstance(strFileIn);

        if(BerkeleyDB::fDbInitialized.load() && !strFileIn.empty() && dbInstance.strDbFile.compare(strFileIn) != 0)
        {
            debug::error(FUNCTION, "Invalid attempt to change Berkeley database file name");
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "Invalid attempt to change Berkeley database file name"));
        }

        BerkeleyDB::fDbInitialized.store(true);

        return dbInstance;
    }

}
