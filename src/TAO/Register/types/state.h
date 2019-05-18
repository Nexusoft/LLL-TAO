/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_STATE_H
#define NEXUS_TAO_REGISTER_INCLUDE_STATE_H

#include <LLC/hash/SK.h>
#include <Util/include/hex.h>
#include <Util/templates/serialize.h>
#include <Util/include/runtime.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** State
         *
         *  A State Register.
         *  Handles the holding of state of a register.
         *
         **/
        class State
        {
        public:
            /** The version of the state register. */
            uint16_t nVersion;


            /** The type of state recorded. */
            uint8_t  nType;


            /** The owner of the register. **/
            uint256_t hashOwner;


            /** The timestamp of the register. **/
            uint64_t nTimestamp;


            /** The byte level data of the register. **/
            std::vector<uint8_t> vchState;


            /** The chechsum of the state register for use in pruning. */
            uint64_t hashChecksum;


            //memory only read position
            mutable uint32_t nReadPos;


            /** Serialization **/
            IMPLEMENT_SERIALIZE
            (
                READWRITE(nVersion);
                READWRITE(nType);
                READWRITE(hashOwner);
                READWRITE(nTimestamp);
                READWRITE(vchState);

                //checksum hash not serialized on gethash
                if(!(nSerType & SER_GETHASH))
                    READWRITE(hashChecksum);
            )


            /** Default Constructor **/
            State();


            /** Basic Type Constructor **/
            State(uint8_t nTypeIn);


            /** Default Constructor **/
            State(const std::vector<uint8_t>& vchData);

            /** Default Constructor **/
            State(uint8_t nTypeIn, const uint256_t& hashOwnerIn);


            /** Default Constructor **/
            State(std::vector<uint8_t> vchData, uint8_t nTypeIn, const uint256_t& hashOwnerIn);


            /** Default Constructor **/
            State(uint64_t hashChecksumIn);


            /** Default Destructor **/
            ~State();


            /** Operator overload to check for equivilence. **/
            bool operator==(const State& state) const;


            /** Operator overload to check for non-equivilence. **/
            bool operator!=(const State& state) const;

            /** SetNull
             *
             *  Set the State Register into a nullptr state.
             *
             **/
            void SetNull();


            /** IsNull
             *
             *  nullptr Checking flag for a State Register.
             *
             **/
            bool IsNull() const;


            /** IsPruned
             *
             *  Flag to determine if the state register has been pruned.
             *
             **/
            bool IsPruned() const;


            /** GetHash
             *
             *  Get the hash of the current state.
             *
             **/
            uint64_t GetHash() const;


            /** SetChecksum
             *
             *  Set the Checksum of this Register.
             *
             **/
            void SetChecksum();


            /** IsValid
             *
             *  Check if the register is valid.
             *
             **/
            bool IsValid() const;


            /** GetState
             *
             *  Get the State from the Register.
             *
             **/
            const std::vector<uint8_t>& GetState() const;


            /** SetState
             *
             *  Set the State from Byte Vector.
             *
             **/
            void SetState(const std::vector<uint8_t>& vchStateIn);


            /** ClearState
             *
             *  Clear a register's state.
             *
             **/
            void ClearState();


            /** end
             *
             *  Detect end of register stream.
             *
             **/
            bool end() const;


            /** read
             *
             *  Reads raw data from the stream.
             *
             *  @param[in] pch The pointer to beginning of memory to write.
             *  @param[in] nSize The total number of bytes to read.
             *
             **/
            const State& read(char* pch, int nSize) const;


            /** write
             *
             *  Writes data into the stream.
             *
             *  @param[in] pch The pointer to beginning of memory to write.
             *  @param[in] nSize The total number of bytes to copy.
             *
             **/
            State& write(const char* pch, int nSize);


            /** Operator Overload <<
             *
             *  Serializes data into vchOperations
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type> State& operator<<(const Type& obj)
            {
                /* Serialize to the stream. */
                ::Serialize(*this, obj, (uint32_t)SER_REGISTER, nVersion); //temp versinos for now

                return (*this);
            }


            /** Operator Overload >>
             *
             *  Serializes data into vchOperations
             *
             *  @param[out] obj The object to de-serialize from ledger data
             *
             **/
            template<typename Type> const State& operator>>(Type& obj) const
            {
                /* Unserialize from the stream. */
                ::Unserialize(*this, obj, (uint32_t)SER_REGISTER, nVersion);
                return (*this);
            }


            /** print
             *
             *  Print debug information to the console and log file.
             *
             **/
            void print() const;

        };
    }
}

#endif
