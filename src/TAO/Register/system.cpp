/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Yesterday I was clever, so I wanted to change the world.
             Today I am wise, so I am changing myself." - Rumi"

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/include/system.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /* Initialize system register values. */
        bool Initialize()
        {
            /* Check if system register exists. */
            Object object;
            if(!LLD::regDB->ReadState(uint256_t(SYSTEM::TRUST), object))
            {
                /* Debug output that the trust system register is initializing. */
                debug::log(0, FUNCTION, "Initializing Trust System Register");

                /* Create the object. */
                object.nVersion   = 1;
                object.nType      = REGISTER::SYSTEM;
                object.hashOwner  = 0; //NOTE: all system register must have owner of zero
                object.nTimestamp = 1409456199; //genesis block timestamp
                object << std::string("trust") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT64_T) << uint64_t(0)
                       << std::string("stake") << uint8_t(TAO::Register::TYPES::MUTABLE) << uint8_t(TAO::Register::TYPES::UINT64_T) << uint64_t(0);

                /* Set the object checksum. */
                object.SetChecksum();

                /* Write the system value. */
                if(!LLD::regDB->WriteState(uint256_t(SYSTEM::TRUST), object))
                    return debug::error(FUNCTION, "failed to write system::trust register");
            }

            return true;
        }
    }
}
