/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_STATE_H
#define NEXUS_TAO_REGISTER_INCLUDE_STATE_H


#include <LLC/hash/serialize.h>
#include <Util/include/hex.h>


namespace TAO
{

    namespace Register
    {

        class State
        {
        public:

            /** Flag to determine if register is Read Only. **/
            bool fReadOnly;


            /** The length of the state register. **/
            uint16_t nLength;


            /** The address space of the register. **/
            uint256_t hashAddress;


            /** The owner of the register. **/
            uint256_t hashOwner;


            /** The byte level data of the register. **/
            std::vector<uint8_t> vchState;


            /** The chechsum of the state register for use in pruning. */
            uint64_t hashChecksum;



            IMPLEMENT_SERIALIZE
            (
                READWRITE(fReadOnly);
                READWRITE(nLength);
                READWRITE(vchState);
                READWRITE(hashAddress);
                READWRITE(hashOwner);

                //checksum hash not serialized on gethash
                if(!(nType & SER_GETHASH))
                    READWRITE(hashChecksum);
            )


            State() : fReadOnly(false), nLength(0), hashAddress(0), hashChecksum(0)
            {
                vchState.clear();
            }


            State(std::vector<uint8_t> vchData) : fReadOnly(false), nLength(vchData.size()), vchState(vchData), hashAddress(0), hashChecksum(0)
            {

            }


            State(uint64_t hashChecksumIn) : fReadOnly(false), nLength(0), hashAddress(0), hashChecksum(hashChecksumIn)
            {

            }


            /** Set the State Register into a NULL state. **/
            void SetNull()
            {
                hashAddress  = 0;
                nLength      = 0;
                hashOwner    = 0;
                hashChecksum = 0;

                vchState.clear();
            }


            /** NULL Checking flag for a State Register. **/
            bool IsNull()
            {
                return (hashAddress == 0 && nLength == 0 && vchState.size() == 0 && hashChecksum == 0);
            }


            /** Flag to determine if the state register has been pruned. **/
            bool IsPruned()
            {
                return (fReadOnly == true && nLength == 0 && vchState.size() == 0 && hashChecksum != 0);
            }


            /** Set the Memory Address of this Register's Index. **/
            void SetAddress(uint256_t hashAddressIn)
            {
                hashAddress = hashAddressIn;
            }


            /** Get the hash of the current state. **/
            uint64_t GetHash()
            {
                return LLC::SerializeSK64(*this);
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


            void print()
            {
                printf("State(address=%s, length=%u, owner=%s, checksum=%" PRIu64 ", state=%s)\n", hashAddress.ToString().substr(0, 20).c_str(), nLength, hashOwner.ToString().substr(0, 20).c_str(), hashChecksum, HexStr(vchState.begin(), vchState.end()).c_str());
            }

        };

    }

}

#endif
