/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_WALLET_WALLETDB_H
#define NEXUS_LEGACY_WALLET_WALLETDB_H

#include <atomic>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <LLC/include/eckey.h>
#include <LLC/types/uint1024.h>

#include <Legacy/wallet/cryptokeystore.h> //for CryptedKeyMap typedef
#include <Legacy/wallet/db.h>


namespace Legacy
{

    /* forward declarations */
    class Script;
    class Account;
    class AccountingEntry;
    class KeyPoolEntry;
    class MasterKey;
    class Wallet;
    class WalletTx;

    /** Error statuses for the wallet database **/
    enum DBErrors
    {
        DB_LOAD_OK,
        DB_CORRUPT,
        DB_TOO_NEW,
        DB_LOAD_FAIL
    };


    /** @class WalletDB
     *
     *  Access to the wallet database (wallet.dat).
     *
     *  This class implements operations for reading and writing different types
     *  of entries (keys) into the wallet database as a proxy for interfacing
     *  with the actual database environment (BerkeleyDB).
     *
     *  The wallet database, through its supported operations, stores values
     *  for multiple types of entries (keys) in the database, including:
     *
     *    - "mkey"<ID> = Master key for unlocking/descrypting encrypted database entries
     *    - "name"<account> = Logical name (label) for an account/Nexus address
     *    - "defaultkey" = Default public key value
     *    - "key"<public key> = unencrypted private key
     *    - "ckey"<public key> = encrypted private key
     *    - "tx"<tx hash> = serialized wallet transaction
     *    - "cscript"<script hash> = serialized redeem script
     *    - "bestblock" = block locator
     *    - "pool"<ID> = serialized key pool
     *    - "acc"<account> = public key associated with an account/Nexus address
     *    - "acentry"<account><counter> = accounting entry (credit/debit) associated with an account/Nexus address
     *
     **/
    class WalletDB
    {
    public:
        /** Defines default name of wallet database file **/
        static const std::string DEFAULT_WALLET_DB;


        /**
         *  Value indicating how many updates have been written to any wallet database since startup.
         *  There may be multiple instances of WalletDB accessing a database, so this
         *  is a static value stored across instances.
         *
         *  Used by the flush wallet thread to track if there have been updates since
         *  its last iteration that need to be flushed to disk.
         *
         **/
        static std::atomic<uint32_t> nWalletDBUpdated;


        /**
         *  An internal counter for accounting entries.
         *  At load time, this value is calculated and assigned when the wallet is loaded,
         *  then incremented each time a new accounting entry is written to the database.
         *  The resulting entry number is used as part of the database key.
         *
         *  Supports multiple accounting entries for the same account with each having a
         *  unique database key.
         **/
        static uint64_t nAccountingEntryNumber;


        /** The file name of the wallet database file **/
        std::string strWalletFile;


        /** Constructor
         *
         *  Initializes database access to wallet database using WalletDB::DEFAULT_WALLET_DB
         *  for the file name.
         *
         **/
        WalletDB()
        : strWalletFile(WalletDB::DEFAULT_WALLET_DB)
        { }


        /** Constructor
         *
         *  Initializes database access for a given file name and access mode.
         *
         *  @param[in] strFileName The database file name. Should be file name only. Database
         *                     will put the file in data directory automatically.
         *
         **/
        WalletDB(const std::string& strFileName)
        : strWalletFile(strFileName)
        { }


        /** Default Destructor **/
        virtual ~WalletDB()
        {
        }


        /** Copy constructor deleted. No copy allowed **/
        WalletDB(const WalletDB&) = delete;


        /** Copy assignment operator deleted. No assignment allowed **/
        WalletDB& operator=(const WalletDB&) = delete;


        /** WriteMasterKey
         *
         *  Stores an encrypted master key into the database. Used to lock/unlock access when
         *  wallet database content has been encrypted. Any old entry for the same key Id is overwritten.
         *
         *  WalletDB supports multiple master key entries, identified by nID. This supports
         *  the potential for the wallet database to have multiple passphrases.
         *
         *  After wallet database content is encrypted, this encryption cannot be reversed. Therefore,
         *  the master key entry is *required* and cannot be removed, only overwritten with an
         *  updated value (after changing the passphrase, for example).
         *
         *  Master key settings should be populated with encryption settings and encrypted key value
         *  before calling this method. The general process looks like this:
         *
         *    - Create Crypter
         *    - Create MasterKey
         *    - Populate MasterKey values for salt, derivation method, number of iterations
         *    - Call Crypter::SetKeyFromPassphrase to configure encryption context in the crypter
         *    - Call Crypter::Encrypt to encrypt the master key value into the MasterKey vchCryptedKey
         *    - Call this method to write the encrypted key into the wallet database
         *
         *  @see Crypter::SetKeyFromPassphrase
         *  @see Crypter::Encrypt
         *  @see MasterKey
         *
         *  @param[in] nMasterKeyId The key Id to identify a particuler master key entry.
         *
         *  @param[in] kMasterKey Encrypted key value along with the encryption settings used to encrypt it
         *
         *  @return true if master key successfully written to wallet database
         *
         **/
        bool WriteMasterKey(const uint32_t nMasterKeyId, const MasterKey& kMasterKey);


        /** ReadMinVersion
         *
         *  Reads the minimum database version supported by this wallet database.
         *
         *  @param[out] nVersion Vesion number to store
         *
         *  @return true if min version is present in the database entry and read successfully
         *
         **/
        bool ReadMinVersion(uint32_t& nVersion);


        /** WriteMinVersion
         *
         *  Stores the minimum database version supported by this wallet database.
         *  Overwrites any previous value.
         *
         *  @param[in] nVersion Vesion number to store
         *
         *  @return true if database entry successfully written
         *
         **/
        bool WriteMinVersion(const uint32_t nVersion);


        /** ReadAccount
         *
         *  Reads the wallet account data associated with an account (Nexus address).
         *  This data includes the public key. This key value can then be used to
         *  retrieve the corresponding private key as needed.
         *
         *  @param[in] strAccount Nexus address in string form of account to read
         *
         *  @param[out] account The wallet account data
         *
         *  @return true if account is present in the database and read successfully
         *
         **/
        bool ReadAccount(const std::string& strAccount, Account& account);


        /** WriteAccount
         *
         *  Stores the wallet account data for an address in the database.
         *
         *  @param[in] strAccount Nexus address in string form of account to write
         *
         *  @param[in] account The wallet account data
         *
         *  @return true if database entry successfully written
         *
         **/
        bool WriteAccount(const std::string& strAccount, const Account& account);


        /** ReadName
         *
         *  Reads a logical name (label) for an address into the database.
         *
         *  @param[in] strAddress Nexus address in string form of name to read
         *
         *  @param[out] strName The value of the logical name, or an empty string if address not in database
         *
         *  @return true if name is present in the database and read successfully
         *
         **/
        bool ReadName(const std::string& strAddress, std::string& strName);


        /** WriteName
         *
         *  Stores a logical name (label) for an address in the database.
         *
         *  @param[in] strAddress Nexus address in string form of name to write
         *
         *  @param[in] strName Logical name to write
         *
         *  @return true if database entry successfully written
         *
         **/
        bool WriteName(const std::string& strAddress, const std::string& strName);


        /** EraseName
         *
         *  Removes the name entry associated with an address.
         *
         *  @param[in] strAddress Nexus address in string form of name to erase
         *
         *  @return true if database entry successfully removed
         *
         **/
        bool EraseName(const std::string& strAddress);


        /** ReadDefaultKey
         *
         *  Reads the default public key from the wallet database.
         *
         *  @param[out] vchPubKey The value of the default public key
         *
         *  @return true if default key is present in the database and read successfully
         *
         **/
        bool ReadDefaultKey(std::vector<uint8_t>& vchPubKey);


        /** WriteDefaultKey
         *
         *  Stores the default public key to the wallet database.
         *
         *  @param[in] vchPubKey The key to write as default public key
         *
         *  @return true if database entry successfully written
         *
         **/
        bool WriteDefaultKey(const std::vector<uint8_t>& vchPubKey);


        /** ReadTrustKey
         *
         *  Reads the public key for the wallet's trust key from the wallet database.
         *
         *  @param[out] vchPubKey The value of the trust key public key
         *
         *  @return true if trust key is present in the database and read successfully
         *
         **/
        bool ReadTrustKey(std::vector<uint8_t>& vchPubKey);


        /** WriteTrustKey
         *
         *  Stores the public key for the wallet's trust key to the wallet database.
         *
         *  @param[in] vchPubKey The public key to save as trust key
         *
         *  @return true if database entry successfully written
         *
         **/
        bool WriteTrustKey(const std::vector<uint8_t>& vchPubKey);


        /** EraseTrustKey
         *
         *  Removes the trust key entry.
         *
         *  @return true if database entry successfully removed
         *
         **/
        bool EraseTrustKey();


        /** ReadKey
         *
         *  Reads the unencrypted private key associated with a public key
         *
         *  @param[in] vchPubKey The public key value of the private key to read
         *
         *  @param[out] vchPrivKey The value of the private key
         *
         *  @return true if unencrypted key is present in the database and read successfully
         *
         **/
        bool ReadKey(const std::vector<uint8_t>& vchPubKey, LLC::CPrivKey& vchPrivKey);


        /** WriteKey
         *
         *  Stores an unencrypted private key using the corresponding public key.
         *
         *  @param[in] vchPubKey The public key value of the private key to write
         *
         *  @param[in] vchPrivKey The value of the private key
         *
         *  @return true if database entry successfully written
         *
         **/
        bool WriteKey(const std::vector<uint8_t>& vchPubKey, const LLC::CPrivKey& vchPrivKey);


        /** WriteCryptedKey
         *
         *  Stores an encrypted private key using the corresponding public key. There is no
         *  complementary read operation for encrypted keys. They are read at startup using
         *  WalletDB::LoadWallet
         *
         *  @param[in] vchPubKey The public key value of the private key to write
         *
         *  @param[in] vchCryptedSecret The encrypted value of the private key
         *
         *  @param[in] fEraseUnencryptedKey Set true (default) to remove any unencrypted entries for the same public key
         *
         *  @return true if database entry successfully written
         *
         **/
        bool WriteCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret,
                             const bool fEraseUnencryptedKey = true);


        /** ReadTx
         *
         *  Reads the wallet transaction for a given transaction hash.
         *
         *  @param[in] hash The transaction hash of the wallet transaction to retrieve
         *
         *  @param[out] wtx The retrieved wallet transaction
         *
         *  @return true if the transaction is present in the database and read successfully
         *
         **/
        bool ReadTx(const uint512_t& hash, WalletTx& wtx);


        /** WriteTx
         *
         *  Stores a wallet transaction using its transaction hash.
         *
         *  @param[in] hash The transaction hash of the wallet transaction to store
         *
         *  @param[in] wtx The wallet transaction to store
         *
         *  @return true if database entry successfully written
         *
         **/
        bool WriteTx(const uint512_t& hash, const WalletTx& wtx);


        /** EraseTx
         *
         *  Removes the wallet transaction associated with a transaction hash.
         *
         *  @param[in] hash The transaction hash of the wallet transaction to remove
         *
         *  @return true if database entry successfully removed
         *
         **/
        bool EraseTx(const uint512_t& hash);


        /** ReadScript
         *
         *  Reads the script for a given script hash.
         *
         *  @param[in] hash The script hash of the script to retrieve
         *
         *  @param[out] redeemScript The retrieved script
         *
         *  @return true if the script is present in the database and read successfully
         *
         **/
        bool ReadScript(const uint256_t& hash, Script& redeemScript);


        /** WriteScript
         *
         *  Stores a redeem script using its script hash.
         *
         *  @param[in] hash The script hash of the script to store
         *
         *  @param[in] redeemScript The script to store
         *
         *  @return true if database entry successfully written
         *
         **/
        bool WriteScript(const uint256_t& hash, const Script& redeemScript);


        /** ReadPool
         *
         *  Reads a key pool entry from the database.
         *
         *  @param[in] nPool The pool entry ID value associated with the key pool entry
         *
         *  @param[out] keypoolEntry The retrieved key pool entry
         *
         *  @return true if the key pool entry is present in the database and read successfully
         *
         **/
        bool ReadPool(const uint64_t nPool, KeyPoolEntry& keypoolEntry);


        /** WritePool
         *
         *  Stores a key pool entry using its pool entry number (ID value).
         *
         *  @param[in] nPool The pool entry ID value associated with the key pool entry
         *
         *  @param[in] keypoolEntry The key pool entry to store
         *
         *  @return true if database entry successfully written
         *
         **/
        bool WritePool(const uint64_t nPool, const KeyPoolEntry& keypoolEntry);


        /** ErasePool
         *
         *  Removes a key pool entry associated with a pool entry number.
         *
         *  @param[in] nPool The pool entry ID value associated with the key pool entry
         *
         *  @return true if database entry successfully removed
         *
         **/
        bool ErasePool(const uint64_t nPool);


        /** WriteAccountingEntry
         *
         *  Stores an accounting entry in the wallet database.
         *
         *  @param[in] acentry The accounting entry to store
         *
         *  @return true if database entry successfully written
         *
         **/
        bool WriteAccountingEntry(const AccountingEntry& acentry);


        /** GetAccountCreditDebit
         *
         *  Retrieves the net total of all accounting entries for an account (Nexus address).
         *
         *  This method calls ListAccountCreditDebit() so passing * for the account will
         *  retrieve the net total of all accounting entries in the database, but because all
         *  entries should be created as credit/debit pairs this net total should always be zero
         *  and isn't of much use.
         *
         *  @param[in] strAccount Nexus address in string form of accounting entries to read
         *
         *  @return net credit or debit of all accounting entries for the provided account
         *
         **/
        int64_t GetAccountCreditDebit(const std::string& strAccount);


        /** ListAccountCreditDebit
         *
         *  Retrieves a list of individual accounting entries for an account (Nexus address)
         *
         *  @param[in] strAccount Nexus address in string form of accounting entries to read, * lists entries for all accounts
         *
         *  @param[out] acentries Accounting entries for the given account will be appended to this list
         *
         **/
        void ListAccountCreditDebit(const std::string& strAccount, std::list<AccountingEntry>& acentries);


        /** InitializeDatabase
         *
         *  Initializes WalletDB for database access to the database file.
         *
         *  Should be called once on initialization. Any subsequent calls will be ignored.
         *
         **/
        void InitializeDatabase();


        /** LoadWallet
         *
         *  Initializes a wallet instance from the data in this wallet database.
         *
         *  @param[in,out] pwallet The wallet instance to initialize
         *
         *  @return Value from Legacy::DBErrors, DB_LOAD_OK on success
         *
         **/
        uint32_t LoadWallet(Wallet& wallet);


        /** EncryptDatabase
         *
         *  Converts the database from unencrypted to encrypted.
         *
         *  This method will write the master key and encrypted key map into the database. It will also remove any unencrypted
         *  keys that correspond to the new encrypted ones. It will suspend the flush thread and perform all database
         *  updates as a single transaction that is committed on success. 
         *
         *  @param[in] nNewMasterKeyId The index (Id) of the master key to store
         *
         *  @param[in] kMasterKey Master key used to encrypt the new encrypted keys
         *
         *  @param[in] mapNewEncryptedKeys The encrypted keys to store in the database
         * 
         *  @return true on success, false otherwise (transaction aborted, database not updated)
         *
         **/
        bool EncryptDatabase(const uint32_t nNewMasterKeyId, const MasterKey& kMasterKey, const CryptedKeyMap& mapNewEncryptedKeys);


        /** DBRewrite
         *
         *  Rewrites the backing database by copying all contents. 
         *
         *  This method provides a proxy to the underlying database process, keeping all database specifics encapsulated 
         *  within WalletDB. Wallet class, or other use points, should always call this method instead of the
         *  underlying method, allowing flexibilty to the underlying implementation.
         * 
         *  @return true on success, false otherwise 
         *
         **/
        bool DBRewrite();


        /** @fn ThreadFlushWalletDB
         *
         *  Start the wallet flush thread for a given wallet file.
         *  If -flushwallet argument is false, this method will not start the thread and simply return.
         *  This method only supports one flush thread for one wallet file, and will return false ir
         *  attempt to start a second.
         *
         *  @param strFile The wallet file to periodically flush
         *
         *  @return true if the flush thread was started, false otherwise
         *
         **/
        static bool StartFlushThread(const std::string& strFile);


        /** @fn ThreadFlushWalletDB
         *
         *  Signals the wallet flush thread to shut down.
         *
         **/
        static void ShutdownFlushThread();


        /** @fn ThreadFlushWalletDB
         *
         *  Function that loops until shutdown and periodically flushes a wallet db
         *  to disk as needed to ensure all data updates are properly persisted. Execute
         *  this function in a background thread to handle wallet flush.
         *
         *  The actual flush is only performed after any open database handle on the wallet database
         *  file is closed by calling CloseDb()
         *
         *  This operation can be disabled by setting the startup option -flushwallet to false
         *
         **/
        static void ThreadFlushWalletDB();


        /** @fn BackupWallet
         *
         *  Writes a backup copy of a wallet to a designated backup file
         *
         *  @param[in] wallet Wallet to back up
         *
         *  @param[in] strDest String containing wallet backup file name or directory
         *                     If a directory, it must exist and same file name as wallet is used
         *
         *  @return true if backup file successfully written
         *
         **/
        static bool BackupWallet(const Wallet& wallet, const std::string& strDest);


    private:
        /**  Thread to perform wallet flush. Used to execute WalletDB::ThreadFlushWalletDB **/
        static std::thread flushThread;


        /**  Flag to record that the flush thread has been started **/
        static std::atomic<bool> fFlushThreadStarted;


        /**  Flag to signal flushThread to shut down **/
        static std::atomic<bool> fShutdownFlushThread;


        /**  Flag to tell flushThread there is a multi-step database operation (cursor, transction, etc.)
         *   currently in progress and it should wait to flush.
         **/
        static std::atomic<bool> fDbInProgress;

    };

}

#endif
