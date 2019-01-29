/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LEGACY_WALLET_WALLET_H
#define NEXUS_LEGACY_WALLET_WALLET_H

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include <LLC/include/key.h>
#include <LLC/types/uint1024.h>

#include <Legacy/wallet/addressbook.h>
#include <Legacy/wallet/cryptokeystore.h>
#include <Legacy/wallet/keypool.h>
#include <Legacy/wallet/masterkey.h>
#include <Legacy/wallet/walletdb.h>
#include <Legacy/wallet/wallettx.h>

#include <Util/include/allocators.h> /* for SecureString */


/* Global TAO namespace. */
namespace TAO
{
    /* Ledger Layer namespace. */
    namespace Ledger
    {
        /* forward declarations */
        class TritiumBlock;
        class BlockState;
    }
}

namespace Legacy
{

    /* forward declarations */
    class CScript;
    class Transaction;
    class CTxIn;
    class CTxOut;
    class NexusAddress;

    class COutput;
    class CReserveKey;
    class CWalletTx;


    /** Nexus: Setting to unlock wallet for block minting only **/
    extern bool fWalletUnlockMintOnly;


    /** (client) version numbers for particular wallet features.  **/
    enum WalletFeature
    {
        FEATURE_BASE = 10000,
        FEATURE_LATEST = 10000
    };


    /** MasterKeyMap is type alias defining a map for storing master keys by key Id. **/
    using MasterKeyMap = std::map<uint32_t, CMasterKey>;

    /** TransactionMap is type alias defining a map for storing wallet transactions by hash. **/
    using TransactionMap = std::map<uint512_t, CWalletTx>;


    /** @class CWallet
     *
     *  A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
     *  and provides the ability to create new transactions.
     *
     *  Implemented as a Singleton where GetInstance is used wherever the wallet instance is needed.
     *
     *  To use the wallet, first call InitializeWallet followed by LoadWallet. The following
     *  example will use the default wallet database file name:
     *
     *  if (!CWallet::InitializeWallet())
     *      //initialization not successful
     *
     *  if (CWallet::GetInstance().LoadWallet() != Legacy::DB_LOAD_OK)
     *      //load not successful
     *
     *  CWallet& wallet = CWallet::GetInstance();
     *  //use wallet
     *
     **/
    class CWallet : public CCryptoKeyStore
    {
        /** CWalletDB declared friend so it can use private Load methods within LoadWallet **/
        friend class CWalletDB;

    public:
        /** InitializeWallet
         *
         *  Initializes the wallet instance backed by a wallet database file with the provided
         *  file name. Only call this method once (returns false on subsequent calls).
         *
         *  This method only constructs the wallet. It does not call LoadWallet to populate
         *  data from the backing file.
         *
         *  @param[in] strWalletFileIn The wallet database file name that backs the wallet.
         *
         *  @return true if wallet intance initialized, false otherwise
         *
         *  @see CWallet::GetInstance
         *  @see LoadWallet
         *
         **/
        static bool InitializeWallet(const std::string& strWalletFileIn = Legacy::CWalletDB::DEFAULT_WALLET_DB);


        /** GetInstance
         *
         *  Retrieves the wallet.
         *
         *  If the wallet is not yet initialized, this will call InitializeWallet using the
         *  default setting.
         *
         *  @return reference to the CWallet instance
         *
         *  @see InitializeWallet
         *  @see LoadWallet
         *
         **/
        static CWallet& GetInstance();


    private:
        using CCryptoKeyStore::Unlock;


        /** Flag indicating whether or not the wallet instance has been initialized **/
        static bool fWalletInitialized;


        /** The current wallet version: clients below this version are not able to load the wallet **/
        uint32_t nWalletVersion;


        /** The maximum wallet version: memory-only variable that specifies to what version this wallet may be upgraded **/
        uint32_t nWalletMaxVersion;


        /** Flag indicating whether or not wallet is backed by a wallet database. When true, strWalletFile contains database file name. **/
        bool fFileBacked;


        /** Flag indicating whether or not a file backed wallet has been loaded.
         *  Set true after successful call to CWallet::LoadWallet().
         *  Prevents LoadWallet() from executing more than once.
         **/
        bool fLoaded;


        /** File name of database file backing this wallet when fFileBacked is true. **/
        std::string strWalletFile;


        /** Map of master keys. Map has one master key per passphrase. **/
        MasterKeyMap mapMasterKeys;


        /** Current max master key Id (max Id in mapMasterKeys). Incremented for each master key added **/
        uint32_t nMasterKeyMaxID;


        /** The address book contained by this wallet **/
        CAddressBook addressBook;


        /** The key pool contained by this wallet **/
        CKeyPool keyPool;


        /** The default public key value for this wallet.
         *  Must be assigned using SetDefaultKey() to assure value is written to
         *  wallet database for a file backed wallet.
         **/
        std::vector<uint8_t> vchDefaultKey;


        /** The timestamp that this wallet will remain unlocked until **/
        uint64_t nWalletUnlockTime;


        /** Wallet database only used during encryption process to maintain
         *  open database transaction across the process.
         *
         *  This has to be a shared_ptr in C++11 because there is no make_unique function
         *  for assigning it when used. If we move to C++14 or higher standard that function
         *  is added and this can be changed.
         **/
        std::shared_ptr<CWalletDB> pWalletDbEncryption;


        /** Constructor
         *
         *  Initializes a wallet instance for FEATURE_BASE that is not file backed.
         *
         **/
        CWallet()
        : nWalletVersion(FEATURE_BASE)
        , nWalletMaxVersion(FEATURE_BASE)
        , fFileBacked(false)
        , fLoaded(false)
        , strWalletFile("")
        , mapMasterKeys()
        , nMasterKeyMaxID(0)
        , addressBook(CAddressBook(*this))
        , keyPool(CKeyPool(*this))
        , vchDefaultKey()
        , nWalletUnlockTime(0)
        , pWalletDbEncryption(nullptr)
        , cs_wallet()
        , mapWallet()
        , mapRequestCount()
        {

        }


    public:
        /** Mutex for thread concurrency across wallet operations **/
        mutable std::mutex cs_wallet;


        /** Map of wallet transactions contained in this wallet **/
        TransactionMap mapWallet;


        std::map<uint1024_t, uint32_t> mapRequestCount;


    /*----------------------------------------------------------------------------------------*/
    /*  Wallet General                                                                        */
    /*----------------------------------------------------------------------------------------*/

        /** CanSupportFeature
         *
         *  Check whether we are allowed to upgrade (or already support) to the named feature
         *
         *  @param[in] wf The feature to check
         *
         *  @return true if key assigned
         *
         **/
        inline bool CanSupportFeature(const enum Legacy::WalletFeature wf) const
        {
            return nWalletMaxVersion >= wf;
        }


        /** SetMinVersion
         *
         *  Assign the minimum supported version for the wallet. Older database versions will not load.
         *  If the wallet is file backed, any version update will also update the version recorded in the data store.
         *
         *  @param[in] nVersion The new minimum version
         *
         *  @param[in] fForceLatest If true, nVersion higher than current max version will upgrade to FEATURE_LATEST
         *                          If false (default), nVersion higher than current max will assign that version
         *
         *  @return true if version assigned successfully
         *
         */
        bool SetMinVersion(const enum Legacy::WalletFeature nVersion, const bool fForceLatest = false);


        /** SetMaxVersion
         *
         *  Assign the maximum version we're allowed to upgrade to.
         *  That this does not immediately imply upgrading to that format.
         *
         *  @param[in] nVersion The new maximum version
         *
         *  @return true if version assigned successfully
         *
         */
        bool SetMaxVersion(const enum Legacy::WalletFeature nVersion);


        /** GetVersion
         *
         *  Get the current wallet format (the oldest version guaranteed to understand this wallet).
         *
         *  @return current wallet version
         *
         */
        inline uint32_t GetVersion() const { return nWalletVersion; }


        /** IsFileBacked
         *
         *  @return true if wallet backed by a wallet database
         *
         */
        inline bool IsFileBacked() const { return fFileBacked; }


        /** GetAddressBook
         *
         *  Retrieve a reference to the address book for this wallet.
         *
         *  @return this wallet's address book
         *
         */
        inline CAddressBook& GetAddressBook() { return addressBook; }


        /** GetWalletFile
         *
         *  Retrieves the database file name for a file backed wallet.
         *
         *  @return the wallet database file name, or empty string if not file backed
         *
         */
        inline std::string GetWalletFile() const { return strWalletFile; }


        /** LoadWallet
         *
         *  Loads all data for a file backed wallet from the database.
         *
         *  The first time LoadWallet is called the wallet file will not exist and
         *  will be created. It is new, so there is no data to retrieve.
         *  fFirstRunRet will be set true to indicate a new wallet.
         *
         *  When fFirstRunRet is true, LoadWallet() will also set the wallet min/max
         *  versions to the latest, fill the key pool, and assign a default key
         *
         *  @param[out] fFirstRunRet true if new wallet
         *
         *  @return Value from Legacy::DBErrors, DB_LOAD_OK on success
         *
         */
        uint32_t LoadWallet(bool& fFirstRunRet);


        /** Inventory
         *
         *  Tracks requests for transactions contained in this wallet, or the blocks that contain them.
         *
         *  When mapRequestCount contains the given block hash, wallet has one or more
         *  transactions in that block and increments the request count.
         *
         *  @param[in] hash Block hash to track
         *
         */
        void Inventory(const uint1024_t& hash);  //Not really a very intuitive method name


        /** GetWalletUnlockTime
         *
         *  Get the time until which this wallet will remain unlocked
         *
         *  @return the time until which this wallet will remain unlocked
         *
         */
        inline uint64_t GetWalletUnlockTime() const { return nWalletUnlockTime; }


    /*----------------------------------------------------------------------------------------*/
    /*  Keystore Implementation                                                               */
    /*----------------------------------------------------------------------------------------*/

        /** AddCryptedKey
         *
         *  Add a public/encrypted private key pair to the key store.
         *  Key pair must be created from the same master key used to create any other key pairs in the store.
         *  Key store must have encryption active.
         *
         *  Wallet overrides this method to also store the key in the wallet database for file backed wallets.
         *
         *  @param[in] vchPubKey The public key to add
         *
         *  @param[in] vchCryptedSecret The encrypted private key
         *
         *  @return true if key successfully added
         *
         **/
        bool AddCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret) override;


        /** AddKey
         *
         *  Add a key to the key store.
         *  Encrypts the key if encryption is active and key store unlocked.
         *
         *  Wallet overrides this method to also store the key in the wallet database for file backed wallets.
         *
         *  @param[in] key The key to add
         *
         *  @return true if key successfully added
         *
         **/
        bool AddKey(const LLC::ECKey& key) override;


        /** AddCScript
         *
         *  Add a script to the key store.
         *
         *  Wallet overrides this method to also store the script in the wallet database for file backed wallets.
         *
         *  @param[in] redeemScript The script to add
         *
         *  @return true if script was successfully added
         *
         **/
        bool AddCScript(const CScript& redeemScript) override;


    /*----------------------------------------------------------------------------------------*/
    /*  Key, Encryption, and Passphrase                                                       */
    /*----------------------------------------------------------------------------------------*/
        /** GetKeyPool
         *
         *  Retrieve a reference to the key pool for this wallet.
         *
         *  @return this wallet's key pool
         *
         */
        inline CKeyPool& GetKeyPool() { return keyPool; }


        /** GenerateNewKey
         *
         *  Generates a new key and adds it to the key store.
         *
         *  @return the public key value for the newly generated key
         *
         */
        std::vector<uint8_t> GenerateNewKey();


        /** GetDefaultKey
         *
         *  Retrieves the default key for this wallet.
         *
         *  @return the default key value
         *
         */
        inline std::vector<uint8_t> GetDefaultKey() const { return vchDefaultKey; }


        /** SetDefaultKey
         *
         *  Assigns a new default key to this wallet. The key itself
         *  should be obtained from the wallet's key pool or generated new.
         *
         *  Wallet also writes the key in the wallet database for file backed wallets.
         *  This will overwrite any prior default key value.
         *
         *  @param[in] vchPubKey The key to make default
         *
         *  @return true if setting default key successful
         *
         *  @see CKeyPool::GetKeyFromPool
         *  @see GenerateNewKey
         *
         */
        bool SetDefaultKey(const std::vector<uint8_t>& vchPubKey);


        /** EncryptWallet
         *
         *  Encrypts the wallet in both memory and file backing, assigning a passphrase that will be required
         *  to unlock and access the wallet. Will not work if wallet already encrypted.
         *
         *  @param[in] strWalletPassphrase The wallet's passphrase
         *
         *  @return true if encryption successful
         *
         */
        bool EncryptWallet(const SecureString& strWalletPassphrase);


        /** Unlock
         *
         *  Attempt to unlock an encrypted wallet using the passphrase provided.  If the UnlockSeconds parameter
         *  is provided then the wallet will automatically relock when this time has expired
         *  Encrypted wallet cannot be used until unlocked by providing the passphrase used to encrypt it.
         *
         *  @param[in] strWalletPassphrase The wallet's passphrase
         *
         *  @param[in] nUnlockSeconds The number of seconds to remain unlocked, 0 to unlock indefinitely
         *                            This setting is ignored if fWalletUnlockMintOnly=true (always unlocks indefinitely)
         *
         *  @return true if wallet was locked, passphrase matches the one used to encrypt it, and unlock is successful
         *
         */
        bool Unlock(const SecureString& strWalletPassphrase, const uint32_t nUnlockSeconds = 0);


        /** ChangeWalletPassphrase
         *
         *  Replaces the existing wallet passphrase with a new one.
         *
         *  @param[in] strOldWalletPassphrase The old passphrase. Must match or change will be unsuccessful
         *
         *  @param[in] strNewWalletPassphrase The new passphrase to use
         *
         *  @return true if old passphrase matched and passphrase changed successfully
         *
         */
        bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);


    /*----------------------------------------------------------------------------------------*/
    /*  Balance                                                                               */
    /*----------------------------------------------------------------------------------------*/
        /** GetBalance
         *
         *  Retrieves the total wallet balance for all confirmed, mature transactions.
         *
         *  @return The current total wallet balance
         *
         */
        int64_t GetBalance();


        /** GetUnconfirmedBalance
         *
         *  Retrieves the current wallet balance for unconfirmed transactions (non-spendable until confirmed).
         *
         *  @return The current wallet unconfirmed balance
         *
         */
        int64_t GetUnconfirmedBalance();


        /** GetStake
         *
         *  Retrieves the current immature stake balance (non-spendable until maturity).
         *
         *  @return The current wallet stake balance
         *
         */
        int64_t GetStake();


        /** GetNewMint
         *
         *  Retrieves the current immature minted (mined) balance (non-spendable until maturity).
         *
         *  @return The current wallet minted balance
         *
         */
        int64_t GetNewMint();


        /** AvailableCoins
         *
         *  Populate vCoins with vector identifying spendable outputs.
         *
         *  @param[in] nSpendTime Cutoff timestamp for result. Any transactions after this time are filtered
         *
         *  @param[out] vCoins Vector of COutput listing spendable outputs
         *
         *  @param[in] fOnlyConfirmed Set false to include unconfirmed transactions in output
         *
         **/
        void AvailableCoins(const uint32_t nSpendTime, std::vector<COutput>& vCoins, const bool fOnlyConfirmed = true);


    /*----------------------------------------------------------------------------------------*/
    /*  Wallet Transactions                                                                   */
    /*----------------------------------------------------------------------------------------*/
        /** MarkDirty
         *
         *  Mark all transactions in the wallet as "dirty" to force balance recalculation.
         *
         **/
        void MarkDirty();


        /** GetTransaction
         *
         *  Retrieves the transaction for a given transaction hash.
         *
         *  @param[in] hashTx The hash of the requested transaction
         *
         *  @param[out] wtx The wallet transaction matching the requested hash, if found
         *
         *  @return true if transaction found
         *
         **/
        bool GetTransaction(const uint512_t& hashTx, CWalletTx& wtx);


        /** AddToWallet
         *
         *  Adds a wallet transaction to the wallet. If this transaction already exists
         *  in the wallet, the new one is merged into it.
         *
         *  @param[in] wtxIn The wallet transaction to add
         *
         *  @return true if transaction found
         *
         **/
        bool AddToWallet(const CWalletTx& wtxIn);


        /** AddToWalletIfInvolvingMe
         *
         *  Checks whether a transaction has inputs or outputs belonging to this wallet, and adds
         *  it to the wallet when it does.
         *
         *  pblock is optional, but should be provided if the transaction is known to be in a block.
         *  If fUpdate is true, existing transactions will be updated.
         *
         *  @param[in] tx The transaction to check
         *
         *  @param[in] containingBlock The block containing the transaction
         *
         *  @param[in] fUpdate Flag indicating whether or not to update transaction already in wallet
         *
         *  @param[in] fFindBlock No longer used
         *
         *  @param[in] fRescan Set true if processing as part of wallet rescan
         *                     This will set CWalletTx time to tx time if it is added (otherwise uses current timestamp)
         *
         * @return true if the transactions was added/updated
         *
         */
        bool AddToWalletIfInvolvingMe(const Transaction& tx, const TAO::Ledger::TritiumBlock& containingBlock,
                                      bool fUpdate = false, bool fFindBlock = false, bool fRescan = false);


        /** EraseFromWallet
         *
         *  Removes a wallet transaction from the wallet, if present.
         *
         *  @param[in] hash The transaction hash of the wallet transaction to remove
         *
         *  @return true if the transaction was erased
         *
         **/
        bool EraseFromWallet(const uint512_t& hash);


        /** DisableTransaction
         *
         *  When disconnecting a coinstake transaction, this method to marks
         *  any previous outputs from this wallet as unspent.
         *
         *  @param[in] tx The coinstake transaction to disable
         *
         **/
        void DisableTransaction(const Transaction& tx);


        /** ScanForWalletTransactions
         *
         *  Scan the block chain for transactions with UTXOs from or to keys in this wallet.
         *  Add/update the current wallet transactions for anyhat found.
         *
         *  @param[in] startBlock Block state for location in block chain to start the scan.
         *                        If nullptr, will scan full chain
         *
         *  @param[in] fUpdate If true, any transaction found by scan that is already in the
         *                     wallet will be updated
         *
         *  @return The number of transactions added/updated by the scan
         *
         **/
        uint32_t ScanForWalletTransactions(const TAO::Ledger::BlockState* pstartBlock, const bool fUpdate = false);


        /** ResendWalletTransactions
         *
         *  Looks through wallet for transactions that should already have been added to a block, but are
         *  still pending, and re-broadcasts them to then network.
         *
         **/
        void ResendWalletTransactions();


        /** WalletUpdateSpent
         *
         *  Checks a transaction to see if any of its inputs match outputs from wallet transactions
         *  in this wallet. For any it finds, verifies that the outputs are marked as spent, updating
         *  them as needed.
         *
         *  @param[in] tx The transaction to check
         *
         **/
        void WalletUpdateSpent(const Transaction& tx);


        /** FixSpentCoins
         *
         *  Identifies and fixes mismatches of spent coins between the wallet and the index db.
         *
         *  @param[out] nMismatchFound Count of mismatches found
         *
         *  @param[out] nBalanceInQuestion Total balance of mismatches found
         *
         *  @param[in] fCheckOnly Set true to identify mismatches only, but not fix them
         *
         *  @return true process executed successfully
         *
         **/
        void FixSpentCoins(uint32_t& nMismatchFound, int64_t& nBalanceInQuestion, const bool fCheckOnly = false);


    /*----------------------------------------------------------------------------------------*/
    /*  Transaction Ownership                                                                 */
    /*----------------------------------------------------------------------------------------*/
        /** IsMine
         *
         *  Checks whether a transaction contains any outputs belonging to this
         *  wallet.
         *
         *  @param[in] tx The transaction to check
         *
         *  @return true if this wallet receives balance via this transaction
         *
         **/
        bool IsMine(const Transaction& tx);


        /** IsMine
         *
         *  Checks whether a specific transaction input represents a send
         *  from this wallet.
         *
         *  @param[in] txin The transaction input to check
         *
         *  @return true if the txin sends balance from this wallet
         *
         **/
        bool IsMine(const CTxIn& txin);


        /** IsMine
         *
         *  Checks whether a specific transaction output represents balance
         *  received by this wallet.
         *
         *  @param[in] txout The transaction output to check
         *
         *  @return true if this wallet receives balance via this txout
         *
         **/
        bool IsMine(const CTxOut& txout);


        /** IsFromMe
         *
         *  Checks whether a transaction contains any inputs belonging to this
         *  wallet.
         *
         *  @param[in] tx The transaction to check
         *
         *  @return true if this wallet sends balance via this transaction
         *
         **/
        inline bool IsFromMe(const Transaction& tx) { return (GetDebit(tx) > 0); }


    /*----------------------------------------------------------------------------------------*/
    /*  Wallet Accounting                                                                     */
    /*----------------------------------------------------------------------------------------*/
        /** GetDebit
         *
         *  Calculates the total value for all inputs sent from this wallet in a transaction.
         *
         *  @param[in] tx The transaction to process
         *
         *  @return total transaction debit amount
         *
         **/
        int64_t GetDebit(const Transaction& tx);


        /** GetCredit
         *
         *  Calculates the total value for all outputs received by this wallet in a transaction.
         *
         *  Includes any change returned to the wallet.
         *
         *  @param[in] tx The transaction to process
         *
         *  @return total transaction credit amount
         *
         **/
        int64_t GetCredit(const Transaction& tx);


        /** GetChange
         *
         *  Calculates the total amount of change returned to this wallet by a transaction.
         *
         *  @param[in] tx The transaction to process
         *
         *  @return total transaction change amount
         *
         **/
        int64_t GetChange(const Transaction& tx);


        /** GetDebit
         *
         *  Returns the debit amount for this wallet represented by a transaction input.
         *
         *  An input will spend the full amount of its previous output. If that previous
         *  output belongs to this wallet, then its value is the amount of the debit.
         *  If the previous output does not belong to this wallet, debit amount is 0.
         *
         *  @param[in] txin The transaction input to process
         *
         *  @return debit amount to this wallet from the given tx input
         *
         **/
        int64_t GetDebit(const CTxIn& txin);


        /** GetCredit
         *
         *  Returns the credit amount for this wallet represented by a transaction output.
         *
         *  If an output belongs to this wallet, then its value is the credit amount.
         *  Otherwise the credit amount is 0.
         *
         *  @param[in] txout The transaction output to process
         *
         *  @return credit amount to this wallet from the given tx output
         *
         **/
        int64_t GetCredit(const CTxOut& txout);


        /** GetChange
         *
         *  Returns the change amount for this wallet represented by a transaction output.
         *
         *  If an output is a change credit, then its value is the change amount.
         *  Otherwise, the change amount is zero.
         *
         *  @param[in] txout The transaction output to process
         *
         *  @return change amount to this wallet from the given tx output
         *
         **/
        int64_t GetChange(const CTxOut& txout);


        /** IsChange
         *
         *  Checks whether a transaction output belongs to this wallet and
         *  represents change returned to it.
         *
         *  @param[in] txout The transaction output to check
         *
         *  @return true if this is a change output
         *
         **/
        bool IsChange(const CTxOut& txout);


    /*----------------------------------------------------------------------------------------*/
    /*  Transaction Creation                                                                  */
    /*----------------------------------------------------------------------------------------*/
        /** SendToNexusAddress
         *
         *  Generate a transaction to send balance to a given Nexus address.
         *
         *  @param[in] address Nexus address where we are sending balance
         *
         *  @param[in] nValue Amount to send
         *
         *  @param[in,out] wtxNew Wallet transaction, send will populate with transaction data
         *
         *  @param[in] fAskFee For old QT to display fee verification popup, no longer used (setting ignored)
         *
         *  @param[in] nMinDepth Minimum depth required before prior transaction output selected as input to this transaction
         *
         *  @return empty string if successful, otherwise contains a displayable error message
         *
         **/
        std::string SendToNexusAddress(const NexusAddress& address, const int64_t nValue, CWalletTx& wtxNew,
                                       const bool fAskFee = false, const uint32_t nMinDepth = 1);


        /** CreateTransaction
         *
         *  Create and populate a new transaction.
         *
         *  @param[in] vecSend List of scripts set with Nexus Address of recipient paired with amount to send to that recipient.
         *                     Each entry will generate a transaction output.
         *
         *  @param[in,out] wtxNew Wallet transaction, create will populate with transaction data
         *                        Should have strFromAccount populated with account transaction is sent from. If not, uses "default"
         *
         *  @param[in,out] reservekey Key reserved for use by change output, key will be returned if no change output
         *
         *  @param[out] nFeeRet Fee paid to send the created transaction
         *
         *  @param[in] nMinDepth Minimum depth required before prior transaction output selected as input to this transaction
         *
         *  @return true if transaction successfully created
         *
         **/
        bool CreateTransaction(const std::vector<std::pair<CScript, int64_t> >& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey,
                               int64_t& nFeeRet, const uint32_t nMinDepth = 1);


        /** CommitTransaction
         *
         *  Commits a transaction and broadcasts it to the network.
         *
         *  @param[in,out] wtxNew Wallet transaction, commit will relay transaction
         *
         *  @param[in,out] reservekey Key reserved for use by change output, key will be kept on successful commit
         *
         *  @return true if transaction successfully committed
         *
         **/
        bool CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey);


        /** AddCoinstakeInputs
         *
         *  Add inputs to the coinstake txin for a coinstake block
         *
         *  @param[in,out] block Target block to add coinstake inputs
         *
         *  @return true if coinstake inputs successfully added
         *
         **/
        bool AddCoinstakeInputs(TAO::Ledger::TritiumBlock& block);


    private:
    /*----------------------------------------------------------------------------------------*/
    /*  Load Wallet operations - require CWalletDB declared friend                            */
    /*----------------------------------------------------------------------------------------*/
        /** LoadMinVersion
         *
         *  Assigns the minimum supported version without updating the database (for file backed wallet).
         *  For use by LoadWallet.
         *
         *  @param[in] nVersion The new minimum version
         *
         *  @return true if version assigned successfully
         *
         *  @see CWalletDB::LoadWallet
         *
         **/
        bool LoadMinVersion(const uint32_t nVersion);


        /** LoadMasterKey
         *
         *  Loads a master key into the wallet, identified by its key Id.
         *  For use by LoadWallet.
         *
         *  @param[in] nMasterKeyId The key Id of the master key to add
         *
         *  @param[in] kMasterKey The master key to add
         *
         *  @return true if master key was successfully added
         *
         *  @see CWalletDB::LoadWallet
         *
         **/
        bool LoadMasterKey(const uint32_t nMasterKeyId, const CMasterKey& kMasterKey);


        /** LoadCryptedKey
         *
         *  Add a public/encrypted private key pair to the key store without updating the database (for file backed wallet).
         *  For use by LoadWallet.
         *
         *  @param[in] vchPubKey The public key to add
         *
         *  @param[in] vchCryptedSecret The encrypted private key
         *
         *  @return true if key successfully added
         *
         *  @see CWalletDB::LoadWallet
         *
         **/
        bool LoadCryptedKey(const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchCryptedSecret);


        /** LoadKey
         *
         *  Add a key to the key store without updating the database (for file backed wallet).
         *  For use by LoadWallet.
         *
         *  @param[in] key The key to add
         *
         *  @return true if key successfully added
         *
         *  @see CWalletDB::LoadWallet
         *
         **/
        bool LoadKey(const LLC::ECKey& key);


        /** LoadCScript
         *
         *  Add a script to the key store without updating the database (for file backed wallet).
         *  For use by LoadWallet.
         *
         *  @param[in] redeemScript The script to add
         *
         *  @return true if script was successfully added
         *
         *  @see CWalletDB::LoadWallet
         *
         **/
        bool LoadCScript(const CScript& redeemScript);


    /*----------------------------------------------------------------------------------------*/
    /*  Helper Methods                                                                        */
    /*----------------------------------------------------------------------------------------*/
       /** SelectCoins
         *
         *  Selects the unspent transaction outputs to use as inputs when creating a transaction that sends
         *  balance from this wallet.
         *
         *  This method uses SelectCoinsMinConf to perform the actual selection.
         *
         *  @param[in] nTargetValue The amount we are looking to send
         *
         *  @param[in] nSpendTime Time of send. Results only include transactions before this time
         *
         *  @param[in,out] setCoinsRet Set to be populated with selected pairs of transaction and vout index (each identifies selected txout)
         *
         *  @param[out] nValueRet Total value of selected unspent txouts in the result set
         *
         *  @param[in] strAccount (optional) Only include outputs for this account label, "default" for wallet default account
         *
         *  @param[in] nMinDepth (optional) Only include outputs with at least this many confirms
         *
         *  @return true if result set was successfully populated
         *
         **/
        bool SelectCoins(const int64_t nTargetValue, const uint32_t nSpendTime, std::set<std::pair<const CWalletTx*, uint32_t> >& setCoinsRet,
                        int64_t& nValueRet, const std::string& strAccount = "*", const uint32_t nMinDepth = 1);


        /** SelectCoinsMinConf
         *
         *  Selects the unspent outputs to use as inputs when creating a transaction to send
         *  balance from this wallet while requiring a minimum confirmation depth to be included in result.
         *
         *  @param[in] nTargetValue The amount we are looking to send
         *
         *  @param[in] nSpendTime Time of send. Results only include transactions before this time
         *
         *  @param[in] nConfMine Require this number of confirmations if transaction with unspent output was from this wallet
         *                       (eg, spending a change transaction),
         *
         *  @param[in] nConfTheirs Require this number of confirmations if transaction with unspent output was received from elsewhere
         *
         *  @param[in,out] setCoinsRet Set of selected unspent txouts as pairs consisting of transaction and vout index
         *
         *  @param[out] nValueRet Total value of selected unspent txouts in the result set
         *
         *  @param[in] strAccount (optional) Only include outputs for this account label, "default" for wallet default account
         *
         *  @return true if script was successfully added
         *
         **/
        bool SelectCoinsMinConf(const int64_t nTargetValue, const uint32_t nSpendTime, const uint32_t nConfMine, const uint32_t nConfTheirs,
                                std::set<std::pair<const CWalletTx*, uint32_t> >& setCoinsRet,
                                int64_t& nValueRet, const std::string& strAccount = "*");

    };

}

#endif
