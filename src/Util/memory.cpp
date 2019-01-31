/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <Util/include/memory.h>

namespace memory
{

    /**  Compares two byte arrays and determines their signed equivalence byte for
     *   byte.
     **/
    int32_t compare(const uint8_t *a, const uint8_t *b, const uint64_t size)
    {
        for(uint64_t i = 0; i < size; ++i)
        {
            const uint8_t &ai = a[i];
            const uint8_t &bi = b[i];

            if(ai != bi)
                return ai - bi;
        }
        return 0;
    }
}
