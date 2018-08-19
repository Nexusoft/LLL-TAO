/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

            (c) Copyright The Nexus Developers 2014 - 2017

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_STATE_H
#define NEXUS_TAO_REGISTER_INCLUDE_STATE_H


#include "../../../Util/templates/serialize.h"


namespace TAO
{

    namespace Register
    {

        class CStateRegister
        {
        public:

            /** Flag to determine if register is Read Only. **/
            bool fReadOnly;


            /** The version of the state of the register. **/
            uint16_t nVersion; //may be superfluous


            /** The length of the state register. **/
            uint16_t nLength;


            /** The byte level data of the register. **/
            std::vector<uint8_t> vchState;


            /** The address space of the register. **/
            uint256_t hashAddress;


            /** The owner of the register. **/
            uint256_t hashOwner; //genesis ID


            /** The chechsum of the state register for use in pruning. */
            uint64_t hashChecksum;



            IMPLEMENT_SERIALIZE
            (
                READWRITE(fReadOnly);
                READWRITE(nVersion);
                READWRITE(nLength);
                READWRITE(FLATDATA(vchState));
                READWRITE(hashAddress);
                READWRITE(hashOwner);

                //checksum hash only seriazlied
                //TODO: clean up this logic
                //if(!(nType & SER_REGISTER_PRUNED))
                READWRITE(hashChecksum);
            )


            CStateRegister() : fReadOnly(false), nVersion(1), nLength(0), hashAddress(0), hashChecksum(0)
            {
                vchState.clear();
            }


            CStateRegister(std::vector<uint8_t> vchData) : fReadOnly(false), nVersion(1), nLength(vchData.size()), vchState(vchData), hashAddress(0), hashChecksum(0)
            {

            }


            CStateRegister(uint64_t hashChecksumIn) : fReadOnly(false), nVersion(1), nLength(0), hashAddress(0), hashChecksum(hashChecksumIn)
            {

            }


            /** Set the State Register into a NULL state. **/
            void SetNull()
            {
                nVersion = 1;
                hashAddress = 0;
                nLength   = 0;
                hashChecksum = 0;
            }


            /** NULL Checking flag for a State Register. **/
            bool IsNull()
            {
                return (nVersion == 1 && hashAddress == 0 && nLength == 0 && vchState.size() == 0 && hashChecksum == 0);
            }


            /** Flag to determine if the state register has been pruned. **/
            bool IsPruned()
            {
                return (fReadOnly == true && nVersion == 0 && nLength == 0 && vchState.size() == 0 && hashChecksum != 0);
            }


            /** Set the Memory Address of this Register's Index. **/
            void SetAddress(uint256_t hashAddressIn)
            {
                hashAddress = hashAddressIn;
            }


            /** Set the Checksum of this Register. **/
            void SetChecksum()
            {
                hashChecksum = LLC::SK64(BEGIN(nVersion), END(hashAddress));
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

        };

    }

}

#endif
