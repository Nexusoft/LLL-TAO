/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2020

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_COMMON_INCLUDE_MEMORY_H
#define NEXUS_COMMON_INCLUDE_MEMORY_H

#include <cstdint>

namespace memory
{
    /** compare
     *
     *  Compares two byte arrays and determines their signed equivalence byte for
     *   byte.
     *
     *  @param[in] a The first byte array to compare.
     *  @param[in] b The second byte array to compare.
     *  @param[in] size The number of bytes to compare.
     *
     *  @return Returns the signed difference of the first different byte value.
     *
     **/
    inline std::int32_t compare(const std::uint8_t *a, const std::uint8_t *b, const std::uint64_t size)
    {
        for(auto i = 0; i < size; ++i)
        {
            const std::uint8_t &ai = a[i];
            const std::uint8_t &bi = b[i];

            if(ai != bi)
                return ai - bi;
        }
        return 0;
    }
}

#endif
