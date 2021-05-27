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


    /** Struct to hold tiker constants. */
    struct TOKEN
    {
        /* Hard coded value for NXS token-id. */
        const static uint256_t NXS;
    };

    const uint256_t TOKEN::NXS = 0;
}
