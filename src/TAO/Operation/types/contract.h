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


            /** MEMORY ONLY: the calling public-id. **/
            mutable uint256_t hashCaller;


            /** MEMORY ONLY: the calling timestamp. **/
            mutable uint64_t nTimestamp;


            /** MEMORY ONLY: the calling txid. **/
            mutable uint512_t hashTx;

        public:


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
             **/
            void Bind(const TAO::Ledger::Transaction& tx) const;


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
             *  @return the genesis-id of caller
             *
             **/
            const uint512_t& Hash() const;


            /** Empty
             *
             *  Detect if the stream is empty.
             *
             *  @return true if operation stream is empty.
             *
             **/
            bool Empty() const;


            /** Reset
             *
             *  Reset the internal stream read pointers.
             *
             **/
            void Reset() const;


            /** Clear
             *
             *  Clears all contract data
             *
             **/
            void Clear();


            /** End
             *
             *  End of the internal stream.
             *
             **/
            bool End(const uint8_t nType = 0) const;


            /** Operations
             *
             *  Get the raw operation bytes from the contract.
             *
             *  @return raw byte const reference
             *
             **/
            const std::vector<uint8_t>& Operations() const;


            /** Conditions
             *
             *  Returns whether this contract contains conditions.
             *
             *  @return true if contract contains conditions
             *
             **/
            bool Conditions() const;


            /** Seek
             *
             *  Seek the internal operation stream read pointers.
             *
             *  @param[in] nPos The position to seek to
             *
             **/
            void Seek(const uint32_t nPos) const;


            /** Seek
             *
             *  Seek the internal operation stream read pointers.
             *
             *  @param[in] nPos The position to seek to
             *
             **/
            void Rewind(const uint32_t nPos) const;


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


            /** Operator Overload <=
             *
             *  Serializes data into ssCondition
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
             *  Serializes data into ssRegister
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
