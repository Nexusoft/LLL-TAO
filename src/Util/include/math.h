/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
    __attribute__((const)) inline uint64_t pow(const uint64_t nBase, const uint64_t nExp) //XXX: maybe use uint8_t for nExp? 64 is maximum size
    {
        /* We just do a simple for loop here for repeated multiplication. */
        uint64_t nRet = 1;
        for(uint32_t n = 0; n < nExp; ++n)
            nRet *= nBase; //XXX: maybe check for overflows and throw exception here?

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
    __attribute__((const)) inline uint64_t log(const uint64_t nBase, const uint64_t nValue)
    {
        /* We just do a simple for loop here for repeated multiplication. */
        uint64_t nRet = 0, nCurrent = nValue;
        while(nCurrent >= nBase)
        {
            nCurrent /= nBase;
            ++nRet;
        }

        return nRet;
    }
}
