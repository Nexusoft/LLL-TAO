/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_WALLET_DB_H
#define NEXUS_LEGACY_WALLET_DB_H

#include <LLD/include/version.h>

#include <Util/templates/datastream.h>

#include <db_cxx.h> /* Berkeley DB header */

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>


namespace Legacy
{

    /** @class BerkeleyDB
     *
     *  Provides implementation for Berkeley database operations.
     *
     *  BerkeleyDB provides one instance per database file used. Each is retrieved
     *  via the GetInstance(strFileIn) method.  An instance initializes and
     *  maintains its own database environment and is independent of others.
     *
     *  Within the instance, all database operations are single-threaded.
     *  Nexus does not require high scalability for Berkeley operations, thus
     *  this approach assures data integrity and avoids race conditions
     *  without impacting system performance.
     *
     *  To assure there can never be two copies of the database instance that may access
     *  the same file concurrenlty, the BerkeleyDB is not copyable. GetInstance() returns
     *  a reference, and only the reference can be passed around.
     *
     **/
    class BerkeleyDB final
    {

    private:

        /** Flag indicating whether or not the wallet instance has been initialized **/
        static std::atomic<bool> fDbInitialized;


        /** Mutex for thread concurrency.
         *
         *  Used to manage concurrency of operations within this database instance.
         */
        std::mutex cs_db;


        /** The Berkeley database environment. Initialized by constructor. */
        DbEnv* dbenv;


        /** Pointer to handle for a Berkeley database,
         *  for opening/accessing database underlying this BerkeleyDB instance
         *
         *  This handle must be closed whenever the database is flushed, then
         *  must be reopened by the next call.
         **/
        Db* pdb;


        /** The file name of the current database **/
        std::string strDbFile;


        /** Contains all current open (uncommitted) transactions for the database in the order they were begun **/
        std::vector<DbTxn*> vTxn;


        /** Constructor
         *
         *  Initializes database access for a given file name.
         *
         *  All BerkeleyDB instances are initialized in read/write/create mode.
         *
         *  @param[in] strFileIn The database file name
         *
         **/
        explicit BerkeleyDB(const std::string& strFileIn);


        /** Init
         *
         *  Performs work of initialization for constructor.
         *
         **/
        void Init();


        /** OpenHandle
         *
         *  Initializes the handle for the current database instance, if not currently open.
         *  Access to handle is then available using pdb instance variable
         *
         *  This method does not lock the cs_db mutex and should be called within lock scope.
         *
         **/
        void OpenHandle();


        /** CloseHandle
         *
         *  Closes the handle for the current database instance, if currently open.
         *  Aborts any open transactions, flushes memory to log file, sets pdb to nullptr.
         *
         *  Call this before any database operations that must close database access (such as flush).
         *  The next database operation must then open a new database handle.
         *
         *  This method does not lock the cs_db mutex and should be called within lock scope.
         *
         **/
        void CloseHandle();


        /** GetTxn
         *
         *  Retrieves the most recently started database transaction.
         *
         *  This method does not lock the cs_db mutex and should be called within lock scope.
         *
         *  @return pointer to most recent database transaction, or nullptr if none started
         *
         **/
        DbTxn* GetTxn();


    public:
        

        /** Default constructor **/
        BerkeleyDB()                                   = delete;


        /** Copy Constructor. **/
        BerkeleyDB(const BerkeleyDB& store)            = delete;


        /** Move Constructor. **/
        BerkeleyDB(BerkeleyDB&& store)                 = delete;


        /** Copy Assignment. **/
        BerkeleyDB& operator=(const BerkeleyDB& store) = delete;


        /** Move Assignment. **/
        BerkeleyDB& operator=(BerkeleyDB&& store)      = delete;


        /** Destructor
         *
         *  Calls Close() on the database
         *
         **/
        ~BerkeleyDB();


        /** GetDatabaseFile
         *
         *  Retrieve the file name backing this database file.
         *
         *  @return the database file name
         *
         **/
        inline std::string GetDatabaseFile() const
        {
            return strDbFile;
        }


        /** Read
         *
         *  Read a value from a database key-value pair.
         *
         *  @param[in] key The key entry of the value to read
         *
         *  @param[out] value The resulting value read from the database
         *                    Can read object data that is serialized into database and supports >> operator for extraction
         *
         *  @return true if the key exists and the value was successfully read
         *
         **/
        template<typename K, typename T>
        bool Read(const K& key, T& value)
        {
            LOCK(cs_db);

            if(pdb == nullptr)
                OpenHandle();

            /* Key */
            DataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(ssKey.data(), ssKey.size());

            /* Value */
            Dbt datValue;
            datValue.set_flags(DB_DBT_MALLOC); // Berkeley will allocate memory for returned value, will need to free before return

            /* Read */
            int32_t ret = pdb->get(GetTxn(), &datKey, &datValue, 0);

            /*  Clear the key memory */
            memset(datKey.get_data(), 0, datKey.get_size());

            if(datValue.get_data() == nullptr)
                return false; // Key not found or no value present, can safely return without free() because there is nothing to free

            /* Unserialize value */
            try
            {
                /* get_data returns a void* and currently uses a C-style cast to load into DataStream */
                DataStream ssValue((char*)datValue.get_data(), (char*)datValue.get_data() + datValue.get_size(), SER_DISK, LLD::DATABASE_VERSION);
                ssValue >> value;
            }
            catch(std::exception &e)
            {
                /* Still need to free any memory allocated for datValue, so do not return here. Just set ret so it returns false */
                ret = -1;
            }

            /* Clear and free value memory */
            memset(datValue.get_data(), 0, datValue.get_size());
            free(datValue.get_data());

            return (ret == 0);
        }


        /** Write
         *
         *  Write a key-value pair to the database.
         *
         *  Cannot write to database opened as read-only.
         *  If the provided key does not exist in the database, the key-value will be appended.
         *  If it does exist, the corresponding value will be overwritten if fOverwrite is set.
         *  Otherwise, this method will return false (write not successful)
         *
         *  @param[in] key The key entry to write
         *
         *  @param[in] value The value to write for the provided key value.
         *                   Can write any value/object that is serializable
         *
         *  @param[in] fOverwrite If true (default), overwrites value for given key if it already exists
         *
         *  @return true if value was successfully written
         *
         **/
        template<typename K, typename T>
        bool Write(const K& key, const T& value, bool fOverwrite=true)
        {
            LOCK(cs_db);

            if(pdb == nullptr)
                OpenHandle();

            /* Key */
            DataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(ssKey.data(), ssKey.size());

            /* Value */
            DataStream ssValue(SER_DISK, LLD::DATABASE_VERSION);
            ssValue.reserve(10000);
            ssValue << value;
            Dbt datValue(ssValue.data(), ssValue.size());

            /* Write */
            int32_t ret = pdb->put(GetTxn(), &datKey, &datValue, (fOverwrite ? 0 : DB_NOOVERWRITE));

            /* Clear memory in case it was a private key */
            memset(datKey.get_data(), 0, datKey.get_size());
            memset(datValue.get_data(), 0, datValue.get_size());

            return (ret == 0);
        }


        /** Erase
         *
         *  Remove a key-value pair from the database
         *
         *  Cannot erase from a database opened as read-only.
         *
         *  @param[in] key The key value of the database entry to erase
         *
         *  @return true if value was successfully removed
         *
         **/
        template<typename K>
        inline bool Erase(const K& key)
        {
            LOCK(cs_db);

            if(pdb == nullptr)
                OpenHandle();

            /* Key */
            DataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(ssKey.data(), ssKey.size());

            /* Erase */
            int32_t ret = pdb->del(GetTxn(), &datKey, 0);

            /* Clear memory */
            memset(datKey.get_data(), 0, datKey.get_size());

            return (ret == 0 || ret == DB_NOTFOUND);
        }


        /** Exists
         *
         *  Tests whether an entry exists in the database for a given key
         *
         *  @param[in] key The key value to test
         *
         *  @return true if the database contains an entry for the given key
         *
         **/
        template<typename K>
        inline bool Exists(const K& key)
        {
            LOCK(cs_db);

            if(pdb == nullptr)
                OpenHandle();

            /* Key */
            DataStream ssKey(SER_DISK, LLD::DATABASE_VERSION);
            ssKey.reserve(1000);
            ssKey << key;
            Dbt datKey(ssKey.data(), ssKey.size());

            /* Exists */
            int32_t ret = pdb->exists(GetTxn(), &datKey, 0);

            /* Clear memory */
            memset(datKey.get_data(), 0, datKey.get_size());
            return (ret == 0);
        }


        /** GetCursor
         *
         *  Open a cursor at the beginning of the database.
         *
         *  @return a pointer to a Berkeley Dbc cursor, nullptr if unable to open cursor
         *
         **/
        Dbc* GetCursor();


        /** ReadAtCursor
         *
         *  Read a database key-value pair from the current cursor location.
         *
         *  The fFlags setting impacts how ssKey and ssValue are used as input. ReadAtCursor() supports:
         *     DB_SET            - Move cursor to key specified by ssKey. Key must match exactly.
         *     DB_SET_RANGE      - Move cursor to key specified by ssKey, or first key >= if no exact match
         *     DB_GET_BOTH       - Move cursor to key/value specified by ssKey and ssValue. Both must match exactly.
         *     DB_GET_BOTH_RANGE - Move cursor to key/value specified by ssKey and ssValue. Key must match exactly,
         *                         first value >= if no exact match.
         *
         *  ReadAtCursor() also supports flags that do not require explicit ssKey/ssValue settings, including
         *     DB_FIRST   - Move to and read the first entry in the database.
         *     DB_NEXT    - Move to and read the next entry in the database.
         *                  Same as DB_FIRST when cursor initially created. Returns DB_NOTFOUND if cursor already on DB_LAST.
         *     DB_CURRENT - Read the current entry in the database. Allows same entry to be read multiple times without moving cursor.
         *     DB_LAST    - Move to and read the last entry in the database.
         *
         *  @param[in,out] pcursor The cursor used to read from the database
         *
         *  @param[in,out] ssKey A stream containing the serialized key read. May also contain input to cursor
         *                       operation for certain flag settings
         *
         *  @param[in,out] ssValue A stream containing the serialized value read. May also contain input to cursor
         *                       operation for certain flag settings
         *
         *  @param[in] fFlags Berkeley database flags for getting data from cursor, defaults to DB_NEXT (read next record)
         *
         *  @return true if value was successfully read
         *
         **/
        int32_t ReadAtCursor(Dbc* pcursor, DataStream& ssKey, DataStream& ssValue, uint32_t fFlags=DB_NEXT);


        /** CloseCursor
         *
         *  Closes and discards a cursor. After calling this method, the cursor is no longer valid for use.
         *
         *  @param[in,out] pcursor The cursor to close
         *
         **/
        void CloseCursor(Dbc* pcursor);


        /** TxnBegin
         *
         *  Start a new database transaction and add it to vTxn
         *
         *  @return true if transaction successfully started
         *
         **/
        bool TxnBegin();


        /** TxnCommit
         *
         *  Commit the transaction most recently added to vTxn
         *
         *  @return true if transaction successfully committed
         *
         **/
        bool TxnCommit();


        /** TxnAbort
         *
         *  Abort the transaction most recently added to vTxn,
         *  reversing any updates performed.
         *
         *  @return true if transaction was successfully aborted
         *
         **/
        bool TxnAbort();


        /** ReadVersion
         *
         *  Read the current value for key "version" from the database
         *
         *  @param[out] nVersion The value read
         *
         *  @return true if a version entry exists in the database and its value was successfully read
         *
         **/
        bool ReadVersion(uint32_t& nVersion);


        /** WriteVersion
         *
         *  Writes a number into the database using the key "version".
         *  Overwrites any previous version entry.
         *
         *  @param[in] nVersion The version number to write
         *
         *  @return true if the value was successfully written
         *
         **/
        bool WriteVersion(const uint32_t nVersion);


        /** DBFlush
         *
         *  Flushes data to the data file for the current database.
         *
         **/
        void DBFlush();


       /** DBRewrite
         *
         *  Rewrites the database file by copying all contents to a new file, then
         *  replaces the original file with the new one.
         *
         *  @return true if rewrite was successful
         *
         **/
        bool DBRewrite();


        /** Shutdown
         *
         *  Shut down the Berkeley database environment for this instance.
         *
         *  Call this on system shutdown to flush, checkpoint, and detach the backing database file.
         *
         **/
        void Shutdown();


        /* Static Methods */

        /** GetInstance
         *
         *  Retrieves the BerkeleyDB instance that corresponds to a given database file.
         *
         *  Database setup will be executed the first time this method is called, and
         *  the database file name must be passed. This will initialize the database environment
         *  for that file. After this initialization call, the database file should not be passed.
         *
         *  Initial call requires a file name, and throws an error if not present.
         *
         *  All subsequent calls should not pass a file name. If one is, and it matches the one
         *  currently in use, it is ignored. If a different file name is passed, it throws an error.
         *
         *  On the first call, this method opens the database environment for the database file.
         *  The returned instance allows all read/write operations. If the file does not
         *  exist, it will be created the first time it is accessed.
         *
         *  @param strFileIn[in] The database file name
         *
         *  @return Berkeley db reference for operating on the database file
         *
         **/
        static BerkeleyDB& GetInstance(const std::string& strFileIn = std::string(""));

    };

}

#endif
