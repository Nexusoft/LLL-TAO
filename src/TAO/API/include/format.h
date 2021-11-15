/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <TAO/API/include/constants.h>

namespace TAO::API
{
    /** FormatBalance
     *
     *  Outputs the correct balance in terms of a double based on decimals input.
     *
     *  @param[in] nBalance The balance to encode for output.
     *  @param[in] nDecimals The number of decimals to format for
     *
     *  @return a double representation of the whole formatting.
     *
     **/
    __attribute__((const)) double FormatBalance(const uint64_t nBalance, const uint8_t nDecimals); //we don't pass by reference for <= 8 bytes


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
    __attribute__((pure)) double FormatBalance(const uint64_t nBalance, const uint256_t& hashToken = TOKEN::NXS);


    /** FormatMint
     *
     *  Outputs the correct balance in terms of a double that can be formatted for output.
     *
     *  @param[in] nBalance The balance to encode for output.
     *
     *  @return a double representation of the whole formatting.
     *
     **/
    __attribute__((pure)) double FormatMint(const int32_t nBalance);



    /** FormatStake
     *
     *  Outputs the correct stake change in terms of a double that can be formatted for output.
     *
     *  @param[in] nBalance The stake change to encode for output.
     *
     *  @return a double representation of the whole formatting.
     *
     **/
    __attribute__((const)) double FormatStake(const int64_t nStake);



    /** FormatStakeRate
     *
     *  Outputs the correct stake rate in terms of a double that can be formatted for output.
     *
     *  @param[in] nBalance The stake change to encode for output.
     *
     *  @return a double representation of the whole formatting.
     *
     **/
    __attribute__((const)) double FormatStakeRate(const uint64_t nTrust);


}
