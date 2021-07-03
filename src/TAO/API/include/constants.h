/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

namespace TAO::API
{
    /** Address for select all. **/
    const uint256_t ADDRESS_ALL = ~uint256_t(0);


    /** Address for select any. **/
    const uint256_t ADDRESS_ANY =  uint256_t(0);


    /** Namespace to hold ticker constants. */
    namespace TOKEN
    {
        /* Hard coded value for NXS token-id. */
        const uint256_t NXS = uint256_t(0);
    }


    /** USER_TYPES
     *
     *  User-defined state register types (non object registers) allowing the data in RAW/APPEND/READONLY registers
     *  to be identified by the leading byte in the register binary data.
     *
     **/
    struct USER_TYPES
    {
        enum
        {
            //for range checks
            RESERVED1   = 0,

            //valid state types
            SUPPLY      = 1,
            INVOICE     = 2,

            //for range checks
            RESERVED2   = 3
        };

        /** Valid
         *
         *  Detect if a type short is within our valid user types range.
         *
         *  @param[in] nType The type we are checking to
         *
         *  @return true if in range, false otherwise.
         *
         **/
        static bool Valid(const uint16_t nType)
        {
            return (nType > RESERVED1 && nType < RESERVED2);
        }
    };
}
