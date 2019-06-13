/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <TAO/Register/include/address.h>
#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {
        /* Get's a register address with properly appended enum byte. */
        uint256_t GetAddress()
        {
            /* Get a raw register address. */
            uint256_t hashAddress = LLC::GetRand256();

            /* Set the leading byte to raw enum. */
            uint8_t nType = 0;

            /* Copy into address. */
            std::copy((uint8_t*)&nType, (uint8_t*)&nType + 1, (uint8_t*)&hashAddress + 31);

            return hashAddress;
        }
    }
}
