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
    /** FormatBalance
     *
     *  Outputs the correct balance in terms of a double that can be formatted for output.
     *
     *  @param[in] nBalance The balance to encode for output.
     *  @param[in] hashToken The token identifier we are formatting for
     *
     *  @return a double representation of the whole formatting.
     *
     **/
    double FormatBalance(const uint64_t nBalance, const uint256_t& hashToken);
}
