/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <stdint.h>

#include <Util/include/debug.h>

namespace math
{
    /** pow
     *
     *  Raises one integer to another integer's power.
     *  Uses integer instructions, rather than floating point instructions.
     *
     *  @param[in] nBase The base value to raise to
     *  @param[in] nExp The exponent to raise the base to.
     *
     *  @return the integer value of nBase^nExp
     *
     **/
    __attribute__((const)) inline uint64_t pow(const uint64_t nBase, const uint8_t nExp)
    {
        /* We just do a simple for loop here for repeated multiplication. */
        uint64_t nRet = 1;
        for(uint8_t n = 0; n < nExp; ++n)
        {
            /* Check for an overflow here and throw an exception. */
            if(nRet > (std::numeric_limits<uint64_t>::max() / nBase))
                throw debug::exception(FUNCTION, "computation result greater than 64-bits");

            nRet *= nBase;
        }

        return nRet;
    }


    /** log
     *
     *  Find's the power of a given base, given its size. Opposite of pow
     *  Uses integer instructions, rather than floating point instructions.
     *
     *  @param[in] nBase The log base value
     *  @param[in] nValue The value to perform logarithm on
     *
     *  @return the integer value of log on value
     *
     **/
    __attribute__((const)) inline uint8_t log(const uint64_t nBase, const uint64_t nValue)
    {
        /* Our return value is only 8-bits, since maximum log for 64-bits possible is 64 in base-2. */
        uint8_t nRet = 0;

        /* Determine the logarithm with repeated division, incrementing ret for every operation. */
        uint64_t nCurrent = nValue;
        while(nCurrent >= nBase)
        {
            nCurrent /= nBase;

            ++nRet;
        }

        return nRet;
    }
}
