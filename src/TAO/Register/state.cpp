/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <TAO/Register/include/state.h>

#include <Util/templates/datastream.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Set the State Register into a nullptr state. */
        void State::SetNull()
        {
            nVersion     = 0;
            nType        = 0;
            hashOwner    = 0;
            nTimestamp   = 0;
            hashChecksum = 0;

            vchState.clear();
            nReadPos     = 0;
        }


        /* Null Checking flag for a State Register. */
        bool State::IsNull() const
        {
            return (nVersion == 0 && vchState.size() == 0 && hashChecksum == 0);
        }


        /* Flag to determine if the state register has been pruned. */
        bool State::IsPruned() const
        {
            return (nVersion == 0 && vchState.size() == 0 && hashChecksum != 0);
        }


        /* Get the hash of the current state. */
        uint64_t State::GetHash() const
        {
            DataStream ss(SER_GETHASH, nVersion);
            ss << *this;

            return LLC::SK64(ss.begin(), ss.end());
        }


        /* Set the Checksum of this Register. */
        void State::SetChecksum()
        {
            hashChecksum = GetHash();
        }


        /* Check if the register is valid. */
        bool State::IsValid() const
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


        /* Get the State from the Register. */
        const std::vector<uint8_t>& State::GetState() const
        {
            return vchState;
        }


        /* Set the State from Byte Vector. */
        void State::SetState(const std::vector<uint8_t>& vchStateIn)
        {
            vchState = vchStateIn;

            SetChecksum();
        }


        /* Clear a register's state. */
        void State::ClearState()
        {
            vchState.clear();
            nReadPos = 0;
        }


        /* Detect end of register stream. */
        bool State::end() const
        {
            return nReadPos >= vchState.size();
        }


        /* Reads raw data from the stream. */
        const State& State::read(char* pch, int nSize) const
        {
            /* Check size constraints. */
            if(nReadPos + nSize > vchState.size())
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "reached end of stream ", nReadPos));

            /* Copy the bytes into tmp object. */
            std::copy((uint8_t*)&vchState[nReadPos], (uint8_t*)&vchState[nReadPos] + nSize, (uint8_t*)pch);

            /* Iterate the read position. */
            nReadPos += nSize;

            return *this;
        }


        /*  Writes data into the stream. */
        State& State::write(const char* pch, int nSize)
        {
            /* Push the obj bytes into the vector. */
            vchState.insert(vchState.end(), (uint8_t*)pch, (uint8_t*)pch + nSize);

            /* Set the length and checksum. */
            SetChecksum();

            return *this;
        }


        /* Print debug information to the console and log file. */
        void State::print() const
        {
            debug::log(0,
                "State(version=", nVersion,
                ", type=", (uint32_t)nType,
                ", length=", vchState.size(),
                ", owner=", hashOwner.ToString().substr(0, 20),
                ", checksum=", hashChecksum,
                ", state=", HexStr(vchState.begin(), vchState.end()));
        }
    }
}
