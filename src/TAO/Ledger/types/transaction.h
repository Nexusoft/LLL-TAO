/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#include <TAO/Operation/types/contract.h>

#include <TAO/Ledger/include/enum.h>

#include <vector>

namespace TAO::API { class Transaction; }

/* Global TAO namespace. */
namespace TAO::Ledger
{
    class BlockState;
    class MerkleTx;


    /** Transaction
     *
     *  A Tritium Transaction.
     *  Stores state of a tritium specific transaction.
     *
     *  transaction header size is 144 bytes
     *
     **/
    class Transaction
    {
        /** Class friends. **/
        friend class MerkleTx;
        friend class TAO::API::Transaction;

    protected:

        /** For disk indexing on contract. **/
        std::vector<TAO::Operation::Contract> vContracts;

    public:

        /** The transaction version. **/
        uint32_t nVersion;


        /** The sequence identifier. **/
        uint32_t nSequence;


        /** The transaction timestamp. **/
        uint64_t nTimestamp;


        /** The nextHash which can claim the signature chain. */
        uint256_t hashNext;


        /** The recovery hash which can be changed only when in recovery mode. */
        uint256_t hashRecovery;


        /** The genesis ID hash. **/
        uint256_t hashGenesis;


        /** The previous transaction. **/
        uint512_t hashPrevTx;


        /** The key type. **/
        uint8_t nKeyType;


        /** The next key type. **/
        uint8_t nNextType;


        /* Memory only, to be disposed once fully locked into the chain behind a checkpoint
         * this is for the segregated keys from transaction data. */
        std::vector<uint8_t> vchPubKey;
        std::vector<uint8_t> vchSig;


        /** MEMORY ONLY: cache of the txid. **/
        mutable uint512_t hashCache;


        /* serialization macros */
        IMPLEMENT_SERIALIZE
        (
            /* Operations layers. */
            READWRITE(vContracts);

            /* Ledger layer */
            READWRITE(nVersion);
            READWRITE(nSequence);
            READWRITE(nTimestamp);
            READWRITE(hashNext);
            READWRITE(hashRecovery);
            READWRITE(hashGenesis);
            READWRITE(hashPrevTx);
            READWRITE(nKeyType);
            READWRITE(nNextType);

            /* Check for skipping public key. */
            if(!(nSerType & SER_GETHASH) && !(nSerType & SER_SKIPPUB))
                READWRITE(vchPubKey);

            /* Handle for when not getting hash or skipsig. */
            if(!(nSerType & SER_GETHASH) && !(nSerType & SER_SKIPSIG))
                READWRITE(vchSig);

            /* Reset our cache if deserializing. */
            if(!(nSerType & SER_GETHASH) && fRead)
                hashCache = 0;
        )


        /** Default Constructor. **/
        Transaction();


        /** Constructor to set txid cache. **/
        Transaction(const uint512_t& hashCacheIn);


        /** Copy constructor. **/
        Transaction(const Transaction& tx);


        /** Move constructor. **/
        Transaction(Transaction&& tx) noexcept;


        /** Copy constructor. **/
        Transaction(const MerkleTx& tx);


        /** Move constructor. **/
        Transaction(MerkleTx&& tx) noexcept;


        /** Copy assignment. **/
        Transaction& operator=(const Transaction& tx);


        /** Move assignment. **/
        Transaction& operator=(Transaction&& tx) noexcept;


        /** Copy assignment. **/
        Transaction& operator=(const MerkleTx& tx);


        /** Move assignment. **/
        Transaction& operator=(MerkleTx&& tx) noexcept;


        /** Default Destructor. **/
        ~Transaction();


        /** Operator Overload <<
         *
         *  Add contracts to the internal vector.
         *
         **/
        Transaction& operator<<(const TAO::Operation::Contract& rContract);


        /** Operator Overload >
         *
         *  Used for sorting transactions by sequence.
         *
         **/
        bool operator>(const Transaction& tx) const;


         /** Operator Overload <
          *
          *  Used for sorting transactions by sequence.
          *
          **/
        bool operator<(const Transaction& tx) const;


        /** Operator Overload []
         *
         *  Access for the contract operator overload. This is for read-only objects.
         *
         **/
        const TAO::Operation::Contract& operator[](const uint32_t n) const;


        /** Operator Overload []
         *
         *  Write access fot the contract operator overload. This handles writes to create new contracts.
         *
         **/
        TAO::Operation::Contract& operator[](const uint32_t n);


        /** Contracts
         *
         *  Gets the list of contracts internal to transaction.
         *
         *  @return a reference of internal contracts list.
         *
         **/
        const std::vector<TAO::Operation::Contract>& Contracts() const;


        /** Size
         *
         *  Get the total contracts in transaction.
         *
         **/
        uint32_t Size() const;


        /** Check
         *
         *  Determines if the transaction is a valid transaction and passes ledger level checks.
         *
         *  @return true if transaction is valid.
         *
         **/
        bool Check(const uint8_t nFlags = 0) const;


        /** Verify
         *
         *  Verify a transaction contracts.
         *
         *  @return true if transaction is valid.
         *
         **/
        bool Verify(const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK) const;


        /** CheckTrust
         *
         *  Check that the claimed trust score and stake reward are correct.
         *
         *  @param[in] pblock Pointer to the block that is connecting
         *
         *  @return true if trust score and reward correct.
         *
         **/
        bool CheckTrust(BlockState* pblock) const;


        /** Cost
         *
         *  Get the total cost of this transaction.
         *
         *  @return the cost of this transaction (in viz).
         *
         **/
        uint64_t Cost();


        /** Build
         *
         *  Build the transaction contracts.
         *
         *  @return true if transaction is valid.
         *
         **/
        bool Build();


        /** Connect
         *
         *  Connect a transaction object to the main chain.
         *
         *  @param[in] nFlags Flag to tell whether transaction is a mempool check.
         *
         *  @return true if transaction is valid.
         *
         **/
        bool Connect(const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK, const BlockState* pblock = nullptr) const;


        /** Disconnect
         *
         *  Disconnect a transaction object to the main chain.
         *
         *  @param[in] nFlags Flag to tell whether transaction is a mempool check.
         *
         *  @return true if transaction is valid.
         *
         **/
        bool Disconnect(const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);


        /** IsCoinBase
         *
         *  Determines if the transaction is a coinbase transaction.
         *
         *  @return true if transaction is a coinbase.
         *
         **/
        bool IsCoinBase() const;


        /** IsCoinStake
         *
         *  Determines if the transaction is a coinstake (trust or genesis) transaction.
         *
         *  @return true if transaction is a coinstake.
         *
         **/
        bool IsCoinStake() const;


        /** IsHybrid
         *
         *  Determines if the transaction is for a private block.
         *
         *  @return true if transaction is a coinbase.
         *
         **/
        bool IsHybrid() const;


        /** IsTrust
         *
         *  Determines if the transaction is a solo staking trust transaction.
         *
         *  @return true if transaction is a coinbase.
         *
         **/
        bool IsTrust() const;


        /** IsGenesis
         *
         *  Determines if the transaction is a solo staking genesis transaction
         *
         *  @return true if transaction is genesis
         *
         **/
        bool IsGenesis() const;


        /** IsFirst
         *
         *  Determines if the transaction is the first transaction
         *  in a signature chain
         *
         *  @return true if transaction is first
         *
         **/
        bool IsFirst() const;


        /** IsConfirmed
         *
         *  Determines if the transaction has been confirmed in the main chain.
         *
         *  @return true if transaction has been confirmed by the network.
         *
         **/
        bool IsConfirmed() const;


        /** GetTrustInfo
         *
         *  Gets the total trust and stake of pre-state.
         *
         *  @param[out] nBalance The balance in the trust account.
         *  @param[out] nTrust The total trust in object.
         *  @param[out] nStake The total stake in object.
         *
         *  @return true if succeeded
         *
         **/
        bool GetTrustInfo(uint64_t &nBalance, uint64_t &nTrust, uint64_t &nStake) const;


        /** GetHash
         *
         *  Gets the hash of the transaction object.
         *
         *  @param[in] fCacheOverride Flag to tell if we want to override our cache
         *
         *  @return 512-bit unsigned integer of hash.
         *
         **/
        uint512_t GetHash(const bool fCacheOverride = false) const;


        /** ProofHash
         *
         *  Gets a proof hash of the transaction object.
         *
         *  @return 512-bit unsigned integer of hash.
         *
         **/
        uint512_t ProofHash() const;


        /** NextHash
         *
         *  Sets the Next Hash from the key
         *
         *  @param[in] hashSecret The secret phrase to generate the keys.
         *
         **/
        void NextHash(const uint512_t& hashSecret);


        /** NextHash
         *
         *  Calculates a next-hash from given secret key.
         *
         *  @param[in] hashSecret The secret phrase to generate the keys.
         *  @param[in] nType The type of key to create with.
         *
         *  @return the calculated next-hash
         *
         **/
        static uint256_t NextHash(const uint512_t& hashSecret, const uint8_t nType);


        /** PrevHash
         *
         *  Gets the nextHash from the previous transaction
         *
         *  @return 256-bit hash of previous transaction
         *
         **/
        uint256_t PrevHash() const;


        /** Sign
         *
         *  Signs the transaction with the private key and sets the public key
         *
         *  @param[in] hashSecret The secret phrase to generate the keys.
         *
         **/
         bool Sign(const uint512_t& hashSecret);


         /** print
          *
          *  Prints the object to the console.
          *
          **/
         void print() const;


         /** ToString
          *
          *  Create a transaction string
          *
          *  @return The string value to return;
          *
          **/
        std::string ToString() const;


         /** ToStringShort
          *
          *  Short form of the debug output.
          *
          *  @return The string value to return;
          *
          **/
        std::string ToStringShort() const;


        /** TypeString
         *
         *  User readable description of the transaction type.
         *
         *  @return User readable description of the transaction type;
         *
         **/
        std::string TypeString() const;


        /** Fees
         *
         *  Calculates and returns the total fee included in this transaction
         *
         *  @return The sum of all OP::FEE contracts in the transaction
         *
         **/
        uint64_t Fees() const;

    };
}
