/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_MEMORY_H
#define NEXUS_UTIL_INCLUDE_MEMORY_H

#include <cinttypes>

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
    int32_t compare(const uint8_t *a, const uint8_t *b, const uint64_t size);
}

#endif
