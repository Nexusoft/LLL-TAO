/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_SYSTEM_H
#define NEXUS_TAO_REGISTER_INCLUDE_SYSTEM_H

#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** Reserved
         *
         *  System reserved values for system registers.
         *
         *  @param[in] hashAddress The register address to check.
         *
         *  @return True if value is system reserved value.
         *
         **/
        inline bool Reserved(const uint256_t& hashAddress)
        {
            return hashAddress >= uint8_t(SYSTEM::RESERVED) && hashAddress <= uint8_t(SYSTEM::LIMIT);
        }
    }
}

#endif
