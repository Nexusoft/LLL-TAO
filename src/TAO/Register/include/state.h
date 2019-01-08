/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_STATE_H
#define NEXUS_TAO_REGISTER_INCLUDE_STATE_H

#include <LLC/hash/SK.h>
#include <Util/include/hex.h>
#include <Util/templates/serialize.h>
#include <Util/include/runtime.h>

namespace TAO::Register
{
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
        uint32_t nReadPos;


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


        State()
        : nVersion(1)
        , nType(0)
        , hashOwner(0)
        , nTimestamp(runtime::unifiedtimestamp())
        , vchState()
        , hashChecksum(0)
        , nReadPos(0)
        {
            vchState.clear();
        }


        State(std::vector<uint8_t> vchData)
        : nVersion(1)
        , nType(0)
        , hashOwner(0)
        , nTimestamp(runtime::unifiedtimestamp())
        , vchState(vchData)
        , nReadPos(0)
        {
            SetChecksum();
        }

        State(uint8_t nTypeIn, uint256_t hashAddressIn, uint256_t hashOwnerIn)
        : nVersion(1)
        , nType(nTypeIn)
        , hashOwner(hashOwnerIn)
        , nTimestamp(runtime::unifiedtimestamp())
        , vchState()
        , hashChecksum(0)
        , nReadPos(0)
        {

        }

        State(std::vector<uint8_t> vchData, uint8_t nTypeIn, uint256_t hashAddressIn, uint256_t hashOwnerIn)
        : nVersion(1)
        , nType(nTypeIn)
        , hashOwner(hashOwnerIn)
        , nTimestamp(runtime::unifiedtimestamp())
        , vchState(vchData)
        , nReadPos(0)
        {
            SetChecksum();
        }


        State(uint64_t hashChecksumIn)
        : nVersion(1)
        , nType(0)
        , hashOwner(0)
        , nTimestamp(runtime::unifiedtimestamp())
        , vchState()
        , hashChecksum(hashChecksumIn)
        , nReadPos(0)
        {

        }


        /** Operator overload to check for equivilence. **/
        bool operator==(const State& state)
        {
            return GetHash() == state.GetHash();
        }


        /** Operator overload to check for non-equivilence. **/
        bool operator!=(const State& state)
        {
            return GetHash() != state.GetHash();
        }


        /** Set the State Register into a nullptr state. **/
        void SetNull()
        {
            nVersion     = 0;
            nType        = 0;
            hashOwner    = 0;
            nTimestamp   = 0;
            hashChecksum = 0;

            vchState.clear();
            nReadPos     = 0;
        }


        /** nullptr Checking flag for a State Register. **/
        bool IsNull() const
        {
            return (nVersion == 0 && vchState.size() == 0 && hashChecksum == 0);
        }


        /** Flag to determine if the state register has been pruned. **/
        bool IsPruned()
        {
            return (nVersion == 0 && vchState.size() == 0 && hashChecksum != 0);
        }


        /** Get the hash of the current state. **/
        uint64_t GetHash() const
        {
            DataStream ss(SER_GETHASH, nVersion);
            ss << *this;

            return LLC::SK64(ss.begin(), ss.end());
        }


        /** Set the Checksum of this Register. **/
        void SetChecksum()
        {
            nTimestamp   = runtime::unifiedtimestamp();
            hashChecksum = GetHash();
        }


        /** Check if the register is valid. */
        bool IsValid() const
        {
            /* Check for null state. */
            if(IsNull())
                return debug::error(FUNCTION, "register cannot be null");

            /* Check the checksum. */
            if(GetHash() != hashChecksum)
                return debug::error(FUNCTION, "register checksum (", GetHash(), ") mismatch (", hashChecksum, ")");

            /* Check the timestamp. */
            if(nTimestamp > runtime::unifiedtimestamp() + MAX_UNIFIED_DRIFT)
                return debug::error(FUNCTION, "register timestamp too far in the future");

            return true;
        }


        /** Get the State from the Register. **/
        std::vector<uint8_t> GetState()
        {
            return vchState;
        }


        /** Set the State from Byte Vector. **/
        void SetState(std::vector<uint8_t> vchStateIn)
        {
            vchState = vchStateIn;

            SetChecksum();
        }


        /** Clear a register's state. */
        void ClearState()
        {
            vchState.clear();
            nReadPos = 0;
        }


        /** Detect end of register stream. */
        bool end()
        {
            return nReadPos >= vchState.size();
        }


        /** read
         *
         *  Reads raw data from the stream
         *
         *  @param[in] pch The pointer to beginning of memory to write
         *
         *  @param[in] nSize The total number of bytes to read
         *
         **/
        State& read(char* pch, int nSize)
        {
            /* Check size constraints. */
            if(nReadPos + nSize > vchState.size())
                throw std::runtime_error(debug::strprintf(FUNCTION, "reached end of stream %u", nReadPos));

            /* Copy the bytes into tmp object. */
            std::copy((uint8_t*)&vchState[nReadPos], (uint8_t*)&vchState[nReadPos] + nSize, (uint8_t*)pch);

            /* Iterate the read position. */
            nReadPos += nSize;

            return *this;
        }


        /** write
         *
         *  Writes data into the stream
         *
         *  @param[in] pch The pointer to beginning of memory to write
         *
         *  @param[in] nSize The total number of bytes to copy
         *
         **/
        State& write(const char* pch, int nSize)
        {
            /* Push the obj bytes into the vector. */
            vchState.insert(vchState.end(), (uint8_t*)pch, (uint8_t*)pch + nSize);

            /* Set the length and checksum. */
            SetChecksum();

            return *this;
        }


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
        template<typename Type> State& operator>>(Type& obj)
        {
            /* Unserialize from the stream. */
            ::Unserialize(*this, obj, (uint32_t)SER_REGISTER, nVersion);
            return (*this);
        }


        void print()
        {
            debug::log(0,
                "State(version=", nVersion,
                ", type=", nType,
                ", length=", vchState.size(),
                ", owner=", hashOwner.ToString().substr(0, 20),
                ", checksum=", hashChecksum,
                ", state=)", HexStr(vchState.begin(), vchState.end()));
        }

    };
}

#endif
