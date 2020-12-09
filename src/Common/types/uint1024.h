/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_COMMON_TYPES_UINT1024_H
#define NEXUS_COMMON_TYPES_UINT1024_H

#include <Common/types/base_uint.h>

namespace
{

    using uint128_t = base_uint<128>;
    using uint256_t = base_uint<256>;
    using uint512_t = base_uint<512>;
    using uint576_t = base_uint<576>;
    using uint1024_t = base_uint<1024>;
    using uint1056_t = base_uint<1056>;

}

#endif
