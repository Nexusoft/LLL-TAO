/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_RESERVED_H
#define NEXUS_TAO_REGISTER_INCLUDE_RESERVED_H

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/address.h>

#include <TAO/Ledger/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** Hold list of reserved object register values. **/
        const std::vector<std::string> RESERVED =
        {
            /* Used to hold token blance. */
            "balance",

            /* Used to identify a specific token. */
            "token",

            /* Used to set validation script in object register. */
            "require",

            /* Used for token domain parameters for max supply. */
            "supply",

            /* Used to determine decimals of a token. */
            "decimals",

            /* Used to determine collective trust in a register. */
            "trust",

            /* Used to determine total stake for a register. */
            "stake",

            /* Used for internal object register system memory. */
            "system",

            /* Used for object register names. Reserved to never be changed. */
            "name"
        };


        /** Reserved
         *
         *  Object register keys that are reserved
         *
         *  @param[in] strValue The value to check.
         *
         *  @return True if value is object register reserved value.
         *
         **/
        inline bool Reserved(const std::string& strValue)
        {
            return std::find(RESERVED.begin(), RESERVED.end(), strValue) != RESERVED.end();
        }


        /** Reserved
         *
         *  System reserved values for system registers.
         *
         *  @param[in] hashAddress The register address to check.
         *
         *  @return True if value is system reserved value.
         *
         **/
        inline bool Reserved(const Address& hashAddress)
        {
            /* Check for valid address values. */
            if(!hashAddress.IsValid())
                return true;

            return hashAddress >= uint8_t(SYSTEM::RESERVED) && hashAddress <= uint8_t(SYSTEM::LIMIT);
        }


        /** Range
         *
         *  Check if a register type is using a valid range
         *
         *  @param[in] nType The type byte to check
         *
         *  @return True if value is in acceptable range.
         *
         **/
        inline bool Range(const uint8_t nType)
        {
            return (nType > REGISTER::RESERVED && nType < REGISTER::SYSTEM);
        }
    }
}

#endif
