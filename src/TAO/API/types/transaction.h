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
    public:

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
                READWRITE(hashNextTx);
            }
        )


        /** BuildIndexes
         *
         *  Builds the indexes for the previous transaction based on this new one.
         *
         **/
        void BuildIndexes() const;
    };
}
