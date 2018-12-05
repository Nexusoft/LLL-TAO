/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEGACY_WALLET_DB_H
#define NEXUS_TAO_LEGACY_WALLET_DB_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <db_cxx.h> /* Berkeley DB header */


namespace Legacy 
{

    extern bool fDetachDB;

    /** @class CDB
     *  
     *  Provides support for accessing Berkeley databases
     *
     **/
    class CDB
    {
    protected:
        /** Mutex for thread concurrency. 
         *  
         *  Used to manage concurrency across CDB databases.
         */
        static std::recursive_mutex cs_db;


        /** The Berkeley database environment.
         *  Initialized on first use of any Berkeley DB access.
         */
        static DbEnv dbenv;


        /** Flag indicating whether or not the database environment
         *  has been initialized.
         */
        static bool fDbEnvInit;


        /** Tracks databases usage
         *
         *  string = file name
         *  int = usage count, value > 0 indicates database in use
         *
         * A usage count >1 indicates that there are multiple CDB instances using the same pdb pointer from mapDb
         */
        static std::map<std::string, int> mapFileUseCount;


        /** Stores pdb copy for each open database handle, keyed by file name.  **/
        static std::map<std::string, std::shared_ptr<Db>> mapDb;


        /* Instance data members */

        /** Pointer to handle for a Berkeley database, for opening/accessing database underlying this CDB instance **/
        std::shared_ptr<Db> pdb;


        /** The file name of the current database **/
        std::string strFile;


        /** Contains all current open (uncommitted) transactions for the database in the order they were begun **/
        std::vector<std::shared_ptr<DbTxn>> vTxn;


        /** Indicates whether or not this database is in read-only mode **/
        bool fReadOnly;


        /** Constructor
         *
         *  Initializes database access for a given file name and access mode.
         *
         *  Access modes: r=read, w=write, +=append, c=create
         *
         *  For modes, read is superfluous, as can always read any open database.
         *  Write and append modes are the same, as append only is not enforced.
         *  Essentially, a database can be opened in read-only mode or read/write mode
         *  using the mode settings. 
         *  
         *  The c (create) mode indicates whether or not to create the database file
         *  if not present. If initialize without using the c mode, and the database file
         *  does not exist, object instantiation will fail with a runtime error.
         *
         *  @param[in] pszFile The database file name
         *
         *  @param[in] pszMode A string containing one or more access mode characters
         *                     defaults to r+ (read and append). An empty or null string is
         *                     equivalent to read only.
         *
         **/
        explicit CDB(const char* pszFile, const char* pszMode="r+");


        /** Destructor
         *
         *  Calls Close() on the database
         *
         **/
        virtual ~CDB();


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
            bool Read(const K& key, T& value);


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
            bool Write(const K& key, const T& value, bool fOverwrite=true);


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
            bool Erase(const K& key);


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
            bool Exists(const K& key);


        /** GetCursor
         *
         *  Open a cursor at the beginning of the database.
         *
         *  @return a pointer to a Berkeley Dbc cursor, nullptr if unable to open cursor
         *
         **/
        std::shared_ptr<Dbc> GetCursor();


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
         *  @param[in] pcursor The cursor used to read from the database
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
        int ReadAtCursor(std::shared_ptr<Dbc> pcursor, CDataStream& ssKey, CDataStream& ssValue, uint32_t fFlags=DB_NEXT);


        /** CloseCursor
         *
         *  Closes and discards a cursor. After calling this method, the cursor is no longer valid for use.
         *
         *  @param[in] pcursor The cursor to close
         *
         **/
        void CloseCursor(std::shared_ptr<Dbc> pcursor);


        /** GetTxn
         *
         *  Retrieves the most recently started database transaction.
         *
         *  @return pointer to most recent database transaction, or nullptr if none started
         *
         **/
        std::shared_ptr<DbTxn> GetTxn();


        /** GetRawTxn
         *
         *  Retrieves the raw pointer for the most recently started database transaction.
         *  Use this where need to pass this pointer to Berkeley API.
         *
         *  @return raw pointer to most recent database transaction, or nullptr if none started
         *
         **/
        DbTxn* GetRawTxn();


    public:
        /** Copy constructor deleted. No copy allowed **/
        CDB(const CDB&) = delete;


        /** Copy assignment operator deleted. No assignment allowed **/
        CDB& operator= (const CDB&) = delete;


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
        bool ReadVersion(int& nVersion);


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
        bool WriteVersion(int nVersion);


        /** Close
         *
         *  Close this instance for database access.
         *  Aborts any open transactions, flushes memory to log file, sets pdb to nullptr,
         *  and sets strFile to an empty string. At this point, the CDB instance can be safely destroyed.
         *
         *  This decrements CDB::mapFileUseCount but does not close the database handle,
         *  which remains stored in CDB::mapDb. The handle will remain open until a call to CloseDb() 
         *  during DBFlush() or shutdown.  Until then, any new instances created for the same data file 
         *  will re-use the open handle.
         *
         **/
        void Close();


        /** @fn CloseDb
         *
         *  Closes down the open database handle for a database and removes it from CDB::mapDb
         *
         *  Should only be called when CDB::mapFileUseCount is 0 (after Close() called on any in-use
         *  instances) or the pdb copy in active instances will become invalid and results of continued 
         *  use are undefined.
         *
         *  @param[in] strFile Database to close 
         *
         **/
        static void CloseDb(const std::string& strFile);


        /** @fn DBFlush
         *
         *  Flushes log file to data file for any database handles with CDB::mapFileUseCount = 0
         *  then calls CloseDb on that database.
         *
         *  @param[in] fShutdown Set true if shutdown in progress, calls EnvShutdown()
         *
         **/
        static void DBFlush(bool fShutdown);


        /** @fn DBRewrite
         *
         *  Rewrites a database file by copying all contents to an new file, then 
         *  replacing the old file with the new one. Does nothing if 
         *  CDB::mapFileUseCount indicates the source file is in use.
         *
         *  @param[in] strFile The database file to rewrite
         *
         *  @param[in] strSkip An optional key type. Any database entries with this key are not copied to the rewritten file
         *
         *  @return true if rewrite was successful
         *
         **/
        static bool DBRewrite(const std::string& strFile, const std::string& strSkip = "");


        /** @fn EnvShutdown
         *
         *  Called to shut down the Berkeley database environment in CDB:dbenv
         *
         *  Should be called on system shutdown. If called at any other time, invalidates
         *  any active database handles resulting in undefined behavior if they are used.
         *  Assuming there are no active database handles, shutting down the environment
         *  when there is no system shutdown requires it to be re-initialized if a new CDB 
         *  instance is later constructed. This is costly and should be avoided.
         *
         **/
        static void EnvShutdown();

    };

}

#endif 
