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

namespace TAO::Register
{
    class State
    {
    public:
        /** The version of the state register. */
        uint16_t nVersion;


        /** The type of state recorded. */
        uint8_t  nType;


        /** The length of the state register. **/
        uint16_t nLength;


        /** The owner of the register. **/
        uint256_t hashOwner;


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
            READWRITE(nLength);
            READWRITE(vchState);
            READWRITE(hashOwner);

            //checksum hash not serialized on gethash
            if(!(nSerType & SER_GETHASH))
                READWRITE(hashChecksum);
        )


        State() : nVersion(1), nType(0), nLength(0), hashOwner(0), hashChecksum(0), nReadPos(0)
        {
            vchState.clear();
        }


        State(std::vector<uint8_t> vchData) : nVersion(1), nType(0), nLength(vchData.size()), vchState(vchData), nReadPos(0)
        {
            SetChecksum();
        }

        State(uint8_t nTypeIn, uint256_t hashAddressIn, uint256_t hashOwnerIn) : nVersion(1), nType(nTypeIn), nLength(0), hashOwner(hashOwnerIn), nReadPos(0)
        {

        }

        State(std::vector<uint8_t> vchData, uint8_t nTypeIn, uint256_t hashAddressIn, uint256_t hashOwnerIn) : nVersion(1), nType(nTypeIn), nLength(vchData.size()), vchState(vchData), hashOwner(hashOwnerIn), nReadPos(0)
        {
            SetChecksum();
        }


        State(uint64_t hashChecksumIn) : nVersion(1), nLength(0), hashChecksum(hashChecksumIn), nReadPos(0)
        {

        }


        /** Set the State Register into a nullptr state. **/
        void SetNull()
        {
            nVersion     = 0;
            nType        = 0;
            nLength      = 0;
            hashOwner    = 0;
            hashChecksum = 0;

            vchState.clear();
        }


        /** nullptr Checking flag for a State Register. **/
        bool IsNull()
        {
            return (nVersion == 0 && nLength == 0 && vchState.size() == 0 && hashChecksum == 0);
        }


        /** Flag to determine if the state register has been pruned. **/
        bool IsPruned()
        {
            return (nVersion == 0 && nLength == 0 && vchState.size() == 0 && hashChecksum != 0);
        }


        /** Get the hash of the current state. **/
        uint64_t GetHash()
        {
            CDataStream ss(SER_GETHASH, nVersion);
            ss << *this;

            return LLC::SK64(ss.begin(), ss.end());
        }

        /** Set the Checksum of this Register. **/
        void SetChecksum()
        {
            hashChecksum = GetHash();
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
            nLength  = vchStateIn.size();

            SetChecksum();
        }

        void ClearState()
        {
            vchState.clear();
            nLength  = 0;
            nReadPos = 0;
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchLedgerData
         *
         *  @param[in] obj The object to serialize into ledger data
         *
         **/
        template<typename Type> State& operator<<(const Type& obj)
        {
            /* Push the size byte */
            vchState.push_back((uint8_t)sizeof(obj));

            /* Push the data */
            vchState.insert(vchState.end(), (uint8_t*)&obj, (uint8_t*)&obj + sizeof(obj));

            /* Set the new length. */
            nLength += sizeof(obj);

            /* Set the checksum */
            SetChecksum();

            return *this;
        }


        /** Operator Overload >>
         *
         *  Serializes data into vchLedgerData
         *
         *  @param[out] obj The object to de-serialize from ledger data
         *
         **/
        template<typename Type> State& operator>>(Type& obj)
        {
            /* Create temporary object to prevent double free in std::copy. */
            Type tmp;

            /* Get the size byte. */
            uint8_t nSize = vchState[0];

            /* Copy the data into tmp. */
            std::copy((uint8_t*)&vchState[nReadPos + 1], (uint8_t*)&vchState[nReadPos + 1] + nSize, (uint8_t*)&tmp);
            nReadPos += nSize + 1;

            /* Set return value. */
            obj = tmp;

            return *this;
        }


        void print()
        {
            debug::log(0, "State(version=%u, type=%u, length=%u, owner=%s, checksum=%" PRIu64 ", state=%s)\n", nVersion, nType, nLength, hashOwner.ToString().substr(0, 20).c_str(), hashChecksum, HexStr(vchState.begin(), vchState.end()).c_str());
        }

    };
}

#endif
