/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_WALLETDB_H
#define NEXUS_LEGACY_WALLET_WALLETDB_H

#include <list>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <LLC/types/uint1024.h>

#include <TAO/Legacy/wallet/db.h>


namespace LLC 
{
    /* forward declaration */    
    class CPrivKey;
}

namespace Legacy
{
    
    namespace Types
    {
        /* forward declaration */    
        class CScript;
    }

    namespace Wallet
    {

        /* forward declarations */
        class CAccount;
        class CAccountingEntry;
        class CKeyPoolEntry;
        class CMasterKey;
        class CWallet;
        class CWalletTx;

        /** Error statuses for the wallet database **/
        enum DBErrors
        {
            DB_LOAD_OK,
            DB_CORRUPT,
            DB_TOO_NEW,
            DB_LOAD_FAIL,
            DB_NEED_REWRITE
        };


        /** @class CWalletDB
         *
         *  Access to the wallet database (wallet.dat).
         *
         *  This class implements operations for reading and writing different types
         *  of entries (keys) into the wallet database.
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
        class CWalletDB : public CDB
        {
        protected:
            /** Defines name of wallet database file **/
            static const std::string DEFAULT_WALLET_DB("wallet.dat");


            /**
             *  Value indicating how many updates have been written to any wallet database since startup.
             *  There may be multiple instances of CWalletDB accessing a database, so this
             *  is a static value stored across instances.
             *
             *  Used by the flush wallet thread to track if there have been updates since 
             *  its last iteration that need to be flushed to disk.
             *
             **/
            static uint32_t nWalletDBUpdated = 0;


            /**
             *  An internal counter for accounting entries. 
             *  At load time, this value is calculated and assigned when the wallet is loaded,
             *  then incremented each time a new accounting entry is written to the database.
             *  The resulting entry number is used as part of the database key.
             *
             *  Supports multiple accounting entries for the same account with each having a 
             *  unique database key.
             **/
            static uint64_t nAccountingEntryNumber = 0;


        public:
            /** Constructor
             *
             *  Initializes database access to wallet database using CWalletDB::DEFAULT_WALLET_DB
             *  for the file name.
             *  
             *
             *  @param[in] pszMode A string containing one or more access mode characters
             *                     defaults to r+ (read and append). An empty or null string is
             *                     equivalent to read only.
             *
             *  @see CDB for modes
             *
             **/
            CWalletDB(const char* pszMode="r+") : 
                CDB(CWalletDB::DEFAULT_WALLET_DB, pszMode)
            { }


            /** Constructor 
             *
             *  Initializes database access for a given file name and access mode.
             *
             *  @param[in] strFilename The database file name. Should be file name only. Database
             *                     will put the file in data directory automatically.
             *
             *  @param[in] pszMode A string containing one or more access mode characters
             *                     defaults to r+ (read and append). An empty or null string is
             *                     equivalent to read only.
             *
             *  @see CDB for modes
             *
             **/
            CWalletDB(std::string strFilename, const char* pszMode="r+") : 
                CDB(strFileName, pszMode)
            { }


            /** Copy constructor deleted. No copy allowed **/
            CWalletDB(const CWalletDB&) = delete;


            /** Copy assignment operator deleted. No assignment allowed **/
            CWalletDB& operator=(const CWalletDB&) = delete;


            /** WriteMasterKey
             *
             *  Stores an encrypted master key into the database. Used to lock/unlock access when
             *  wallet database content has been encrypted. Any old entry for the same key Id is overwritten.
             *
             *  CWalletDB supports multiple master key entries, identified by nID. This supports
             *  the potential for the wallet database to have multiple passphrases.
             *
             *  After wallet database content is encrypted, this encryption cannot be reversed. Therefore, 
             *  the master key entry is *required* and cannot be removed, only overwritten with an
             *  updated value (after changing the passphrase, for example).
             *
             *  Master key settings should be populated with encryption settings and encrypted key value
             *  before calling this method. The general process looks like this:
             * 
             *    - Create CCrypter
             *    - Create CMasterKey
             *    - Populate CMasterKey values for salt, derivation method, number of iterations
             *    - Call CCrypter::SetKeyFromPassphrase to configure encryption context in the crypter
             *    - Call CCrypter::Encrypt to encrypt the master key value into the CMasterKey vchCryptedKey
             *    - Call this method to write the encrypted key into the wallet database
             *
             *  @see CCrypter::SetKeyFromPassphrase
             *  @see CCrypter::Encrypt
             *  @see CMasterKey
             *
             *  @param[in] nMasterKeyId The key Id to identify a particuler master key entry. 
             *
             *  @param[in] kMasterKey Encrypted key value along with the encryption settings used to encrypt it
             *
             *  @return true if master key successfully written to wallet database
             *
             **/
            bool WriteMasterKey(const uint32_t nMasterKeyId, const CMasterKey& kMasterKey);


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
            bool WriteMinVersion(const int nVersion);


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
            bool ReadAccount(const std::string& strAccount, CAccount& account);


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
            bool WriteAccount(const std::string& strAccount, const CAccount& account);


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
             *  CWalletDB::LoadWallet
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
            bool WriteCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret, const bool fEraseUnencryptedKey = true);


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
            bool ReadTx(const uint512_t hash, CWalletTx& wtx);


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
            bool WriteTx(const uint512_t hash, const CWalletTx& wtx);


            /** EraseTx
             *
             *  Removes the wallet transaction associated with a transaction hash.
             *
             *  @param[in] hash The transaction hash of the wallet transaction to remove
             *
             *  @return true if database entry successfully removed
             *
             **/
            bool EraseTx(const uint512_t hash);


            /** ReadCScript
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
            bool ReadCScript(const uint256_t &hash, Legacy::Types::CScript& redeemScript);


            /** WriteCScript
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
            bool WriteCScript(const uint256_t& hash, const Legacy::Types::CScript& redeemScript);


            /** ReadBestBlock
             *
             *  Reads the stored CBlockLocator of the last recorded best block.
             *
             *  @param[out] locator Block locator of the best block as recorded in wallet database
             *
             *  @return true if best block entry present in database and successfully read
             *
             **/
            bool ReadBestBlock(Core::CBlockLocator& locator);


            /** WriteBestBlock
             *
             *  Stores a CBlockLocator to record current best block.
             *
             *  @param[in] locator The block locator to store
             *
             *  @param[out] wtx The retrieved wallet transaction
             *
             *  @return true if database entry successfully written
             *
             **/
            bool WriteBestBlock(const Core::CBlockLocator& locator);


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
            bool ReadPool(const int64_t nPool, CKeyPoolEntry& keypoolEntry);


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
            bool WritePool(const int64_t nPool, const CKeyPoolEntry& keypoolEntry);


            /** ErasePool
             *
             *  Removes a key pool entry associated with a pool entry number.
             *
             *  @param[in] nPool The pool entry ID value associated with the key pool entry
             *
             *  @return true if database entry successfully removed
             *
             **/
            bool ErasePool(const int64_t nPool);


            /** WriteAccountingEntry
             *
             *  Stores an accounting entry in the wallet database.
             *
             *  @param[in] acentry The accounting entry to store
             *
             *  @return true if database entry successfully written
             *
             **/
            bool WriteAccountingEntry(const CAccountingEntry& acentry);


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
            void ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& acentries);


            /** LoadWallet
             *
             *  Initializes a wallet instance from the data in this wallet database.
             *
             *  @param[in] pwallet The wallet instance to initialize
             *
             *  @return Value from Legacy::Wallet::DBErrors, DB_LOAD_OK on success
             *
             **/
            int LoadWallet(CWallet& wallet);
        };


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
         *  @param[in] strWalletFile The wallet database file to flush
         *
         **/
        void ThreadFlushWalletDB(const std::string strWalletFile);


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
        bool BackupWallet(const CWallet& wallet, const std::string& strDest);

    }
}

#endif
