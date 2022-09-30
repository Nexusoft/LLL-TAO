/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO::API
{

    /** @class Transaction
     *
     *  API level class of transaction for keeping double linked list indexes.
     *
     **/
    class Transaction : public TAO::Ledger::Transaction
    {
        /** Track the last time this object was modified. **/
        uint64_t nModified;


        /*& Track our current status. **/
        uint8_t nStatus;

    public:

        /* Enum to track status. */
        enum : uint8_t
        {
            PENDING   = 0x01, //peending means it was broadcast or created
            ACCEPTED  = 0x02,
        };

        /** Track the next transaction in the chain. **/
        uint512_t hashNextTx;


        /** The default constructor. Sets block state to Null. **/
        Transaction();


        /** Copy constructor. **/
        Transaction(const Transaction& tx);


        /** Move constructor. **/
        Transaction(Transaction&& tx) noexcept;


        /** Copy constructor. **/
        Transaction(const TAO::Ledger::Transaction& tx);


        /** Move constructor. **/
        Transaction(TAO::Ledger::Transaction&& tx) noexcept;


        /** Copy assignment. **/
        Transaction& operator=(const Transaction& tx);


        /** Move assignment. **/
        Transaction& operator=(Transaction&& tx) noexcept;


        /** Copy assignment. **/
        Transaction& operator=(const TAO::Ledger::Transaction& tx);


        /** Move assignment. **/
        Transaction& operator=(TAO::Ledger::Transaction&& tx) noexcept;


        /** Default Destructor **/
        virtual ~Transaction();


        /* serialization macros */
        IMPLEMENT_SERIALIZE
        (
            /* Contracts layers. */
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

            /* We don't want merkle information for gethash. */
            if(!(nSerType & SER_GETHASH))
            {
                READWRITE(nModified);
                READWRITE(nStatus);
                READWRITE(hashNextTx);

                /* Reset our cache if deserializing. */
                hashCache = 0;
            }
        )


        /** Confirmed
         *
         *  Get if transaction is in a confirmed status.
         *
         **/
        bool Confirmed() const;


        /** Spent
         *
         *  Check if a specific contract has been spent already.
         *
         *  @param[in] hash The txid to check spends by.
         *  @param[in] nContract The contract-id to check proofs for.
         *
         *  @return true if the contract has been spent.
         *
         **/
        bool Spent(const uint512_t& hash, const uint32_t nContract) const;


        /** Broadcast
         *
         *  Broadcast the transaction to all available nodes and update status.
         *
         **/
        void Broadcast();


        /** Index
         *
         *  Index a last transaction into the logical database.
         *
         *  @param[in] hash The txid to index the transaction by
         *
         *  @return true if this was indexed as last tx
         *
         **/
        bool Index(const uint512_t& hash);


        /** Delete
         *
         *  Delete this transaction from the logical database.
         *
         *  @param[in] hash The txid to delete the transaction index
         *
         *  @return true if this index was deleted
         *
         **/
        bool Delete(const uint512_t& hash);


        /** IsLast
         *
         *  Check if transaction is last in sigchain.
         *
         *  @return true if transaction is last in sigchain.
         *
         **/
        bool IsLast() const;


    private:

        /** index_registers
         *
         *  Index registers for logged in sessions.
         *
         *  @param[in] hash The transaction-id we are working on.
         *
         **/
        void index_registers(const uint512_t& hash);


        /** deindex_registers
         *
         *  Delete index of registers for logged in sessions.
         *
         *  @param[in] hash The transaction-id we are working on.
         *
         **/
        void deindex_registers(const uint512_t& hash);
    };
}
