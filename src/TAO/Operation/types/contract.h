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


        public:

            /** MEMORY ONLY: Contract timestamp. **/
            mutable uint64_t nTimestamp;


            /** MEMORY ONLY: Caller public-id. **/
            mutable uint256_t hashCaller;


            /** Default Constructor. **/
            Contract()
            : ssOperation()
            , ssCondition()
            , ssRegister()
            , nTimestamp(runtime::unifiedtimestamp())
            , hashCaller(0)
            {
            }


            IMPLEMENT_SERIALIZE
            (
                READWRITE(ssOperation);
                READWRITE(ssCondition);
                READWRITE(ssRegister);
            )


            /** Primitive
             *
             *  Get the primitive operation.
             *
             *  @return The primitive instruction.
             *
             **/
            const uint8_t Primitive() const
            {
                /* Sanity checks. */
                if(ssOperation.size() == 0)
                    throw std::runtime_error(debug::safe_printstr(FUNCTION, "cannot get primitive when empty"));

                /* Return first byte. */
                return ssOperation.get(0);
            }


            /** Empty
             *
             *  Detect if the stream is empty.
             *
             *  @return true if operation stream is empty.
             *
             **/
            const bool Empty() const
            {
                return (ssOperation.size() == 0);
            }


            /** Conditions
             *
             *  Detect if there are any conditions present.
             *
             *  @return false if condition stream is empty
             *
             **/
            const bool Conditions() const
            {
                return (ssCondition.size() == 0);
            }


            /** Reset
             *
             *  Reset the internal stream read pointers.
             *
             **/
            void Reset() const
            {
                /* Set the operation stream to beginning. */
                ssOperation.seek(0, STREAM::BEGIN);

                /* Set the condition stream to beginning. */
                ssCondition.seek(0, STREAM::BEGIN);

                /* Set the register stream to beginning. */
                ssRegister.seek(0, STREAM::BEGIN);
            }


            /** Clear
             *
             *  Clears all contract data
             *
             **/
            void Clear() const
            {
                /* Set the operation stream to beginning. */
                ssOperation.SetNull();

                /* Set the condition stream to beginning. */
                ssCondition.SetNull();

                /* Set the register stream to beginning. */
                ssRegister.SetNull();

                /* Reset the timestamp. */
                nTimestamp = runtime::unifiedtimestamp();

                /* Reset the caller. */
                hashCaller = 0;
            }


            /** End
             *
             *  End of the internal stream.
             *
             **/
            bool End() const
            {
                return ssOperation.end();
            }


            /** Operations
             *
             *  Get the raw operation bytes from the contract.
             *
             *  @return raw byte const reference
             *
             **/
            const std::vector<uint8_t>& Operations() const
            {
                return ssOperation.Bytes();
            }


            /** Conditions
             *
             *  Get the raw condition bytes from the contract.
             *
             *  @return raw byte const reference
             *
             **/
            const std::vector<uint8_t>& Conditions() const
            {
                return ssCondition.Bytes();
            }


            /** Seek
             *
             *  Seek the internal operation stream read pointers.
             *
             *  @param[in] nPos The position to seek to
             *
             **/
            void Seek(const uint32_t nPos) const
            {
                /* Set the operation stream to beginning. */
                ssOperation.seek(nPos, STREAM::BEGIN);

                /* Set the register stream to beginning. */
                ssRegister.seek(0, STREAM::BEGIN);
            }


            /** Seek
             *
             *  Seek the internal operation stream read pointers.
             *
             *  @param[in] nPos The position to seek to
             *
             **/
            void Rewind(const uint32_t nPos) const
            {
                /* Set the operation stream to beginning. */
                ssOperation.seek(int32_t(-1 * nPos));
            }


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
