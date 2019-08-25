/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_OPERATION_TYPES_CONTRACT_H
#define NEXUS_TAO_OPERATION_TYPES_CONTRACT_H

#include <TAO/Operation/types/stream.h>

#include <TAO/Register/types/stream.h>

/* Forward declarations. */
namespace Legacy
{
    class TxOut;
}

/* Global TAO namespace. */
namespace TAO
{
    /* Ledger layer namespace. */
    namespace Ledger
    {
        /* Forward declarations. */
        class Transaction;
    }


    /* Operation Layer namespace. */
    namespace Operation
    {

        /** Contract
         *
         *  Contains the operation stream, and register pre-states and post states.
         *
         **/
        class Contract
        {
            /** Contract operation stream. **/
            TAO::Operation::Stream ssOperation;


            /** Contract conditional stream. */
            TAO::Operation::Stream ssCondition;


            /** Contract register stream. **/
            TAO::Register::Stream  ssRegister;

            /* MEMORY ONLY: The cached cost of this contract */
            mutable uint64_t nCost;

            /** MEMORY ONLY: the calling public-id. **/
            mutable uint256_t hashCaller;


            /** MEMORY ONLY: the calling timestamp. **/
            mutable uint64_t nTimestamp;


            /** MEMORY ONLY: the calling txid. **/
            mutable uint512_t hashTx;


        public:

            /** Enumeration to handle setting aspects of the contract. */
            enum
            {
                OPERATIONS = (1 << 1),
                CONDITIONS = (1 << 2),
                REGISTERS  = (1 << 3),

                ALL        = OPERATIONS | CONDITIONS | REGISTERS
            };


            /** Default Constructor. **/
            Contract();


            /** Copy Constructor. **/
            Contract(const Contract& contract);


            /** Move Constructor. **/
            Contract(const Contract&& contract);


            /** Assignment Operator **/
            Contract& operator=(const Contract& contract);


            IMPLEMENT_SERIALIZE
            (
                READWRITE(ssOperation);
                READWRITE(ssCondition);
                READWRITE(ssRegister);
            )


            /** Bind
             *
             *  Bind the contract to a transaction.
             *
             *  @param[in] tx The transaction to bind the contract to.
             *
             **/
            void Bind(const TAO::Ledger::Transaction* tx) const;


            /** AddCost
             *
             *  Adds the costs to the contract.
             *
             *  @param[in] nFee The total cost to add to the contract.
             *
             **/
            void AddCost(const uint64_t nCost) const;


            /** Cost
             *
             *  Get the cost of this contract.
             *
             *  @return The cost of this contract.
             *
             **/
            uint64_t Cost() const;


            /** Primitive
             *
             *  Get the primitive operation.
             *
             *  @return The primitive instruction.
             *
             **/
            uint8_t Primitive() const;


            /** Timestamp
             *
             *  Get this contract's execution time.
             *
             *  @return the timestamp contract was executed.
             *
             **/
            const uint64_t& Timestamp() const;


            /** Caller
             *
             *  Get this contract's caller
             *
             *  @return the genesis-id of caller
             *
             **/
            const uint256_t& Caller() const;


            /** Hash
             *
             *  Get the hash of calling tx
             *
             *  @return Returns the txid of calling tx
             *
             **/
            const uint512_t& Hash() const;


            /** Value
             *
             *  Get the value of the contract if valid
             *
             *  @param[out] nValue The value to return.
             *
             *  @return True if there is value in the contract.
             *
             **/
            bool Value(uint64_t &nValue) const;


            /** Legacy
             *
             *  Get the legacy converted output of the contract if valid
             *
             *  @param[out] txout The legacy output.
             *
             *  @return True if there is value in the contract.
             *
             **/
            bool Legacy(Legacy::TxOut& txout) const;


            /** Empty
             *
             *  Detect if the stream is empty.
             *
             *  @param[in] nFlags The flags to determine which streams to check.
             *
             *  @return true if stream(s) is empty.
             *
             **/
            bool Empty(const uint8_t nFlags = OPERATIONS) const;


            /** Reset
             *
             *  Reset the internal stream read pointers.
             *
             *  @param[in] nFlags The flags to determine which streams to reset.
             *
             **/
            void Reset(const uint8_t nFlags = ALL) const;


            /** Clear
             *
             *  Clears all contract data
             *
             *  @param[in] nFlags The flags to determine which streams to clear.
             *
             **/
            void Clear(const uint8_t nFlags = ALL);


            /** ReadCompactSize
             *
             *  Get's a size from internal stream.
             *
             *  @param[in] nFlags The flags to determine which streams to check.
             *
             **/
            uint64_t ReadCompactSize(const uint8_t nFlags = OPERATIONS) const;


            /** End
             *
             *  End of the internal stream.
             *
             *  @param[in] nFlags The flags to determine which streams to check.
             *
             **/
            bool End(const uint8_t nFlags = OPERATIONS) const;


            /** Operations
             *
             *  Get the raw operation bytes from the contract.
             *
             *  @return raw byte const reference
             *
             **/
            const std::vector<uint8_t>& Operations() const;


            /** Seek
             *
             *  Seek the internal operation stream read pointers.
             *
             *  @param[in] nPos The position to seek to
             *  @param[in] nFlags The flags to determine which streams to seek.
             *  @param[in] nType The type to seek (begin, cursor, end).
             *
             **/
            void Seek(const uint32_t nPos, const uint8_t nFlags = OPERATIONS, const uint8_t nType = STREAM::CURSOR) const;


            /** Seek
             *
             *  Seek the internal operation stream read pointers.
             *
             *  @param[in] nPos The position to seek to
             *
             **/
            void Rewind(const uint32_t nPos, const uint8_t nFlags = CONDITIONS) const;


            /** Operator Overload <<
             *
             *  Serializes data into ssOperation
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type>
            Contract& operator<<(const Type& obj)
            {
                /* Serialize to the stream. */
                ssOperation << obj;

                return (*this);
            }


            /** Operator Overload <<
             *
             *  Serializes data into ssOperation
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type>
            const Contract& operator>>(Type& obj) const
            {
                /* Serialize to the stream. */
                ssOperation >> obj;

                return (*this);
            }


            /** Operator Overload <=
             *
             *  Serializes data into ssCondition
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type>
            Contract& operator<=(const Type& obj)
            {
                /* Serialize to the stream. */
                ssCondition << obj;

                return (*this);
            }


            /** Operator Overload >=
             *
             *  Serializes data from ssCondition
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type>
            const Contract& operator>=(Type& obj) const
            {
                /* Serialize to the stream. */
                ssCondition >> obj;

                return (*this);
            }


            /** Operator Overload <<=
             *
             *  Serializes data into ssRegister
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type>
            Contract& operator<<=(const Type& obj)
            {
                /* Serialize to the stream. */
                ssRegister << obj;

                return (*this);
            }


            /** Operator Overload >>=
             *
             *  Serializes data from ssRegister
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type>
            const Contract& operator>>=(Type& obj) const
            {
                /* Serialize to the stream. */
                ssRegister >> obj;

                return (*this);
            }
        };
    }
}

#endif
