/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <memory/include/memory.h>

namespace memory
{

    /**  Compares two byte arrays and determines their signed equivalence byte for
     *   byte.
     **/
    int32_t compare(const uint8_t *a, const uint8_t *b, const uint64_t nSize)
    {
        for(uint64_t i = 0; i < nSize; ++i)
        {
            if(a[i] != b[i])
                return a[i] - b[i];
        }
        return 0;
    }
}
