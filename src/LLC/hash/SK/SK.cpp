/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

namespace LLC
{
    /* Implementation of SK function caches */
    LLD::TemplateLRU<std::vector<uint8_t>, uint64_t> cache64(16);
    LLD::TemplateLRU<std::vector<uint8_t>, uint256_t> cache256(16);
    LLD::TemplateLRU<std::vector<uint8_t>, uint512_t> cache512(16);
    LLD::TemplateLRU<std::vector<uint8_t>, uint1024_t> cache1024(9);

}