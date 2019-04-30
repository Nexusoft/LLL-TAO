/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_PRIME_H
#define NEXUS_TAO_LEDGER_INCLUDE_PRIME_H

#include <LLC/types/bignum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** SetBits
         *
         *  Convert Double to unsigned int Representative.
         *  Used for encoding / decoding prime difficulty from nBits.
         *
         *  @param[in] nDiff difficulty value
         *
         *  @return Unsigned integer representing double value.
         *
         **/
        uint32_t SetBits(double nDiff);


        /** GetPrimeDifficulty
         *
         *  Determines the difficulty of the Given Prime Number.
         *  Difficulty is represented as so V.X
         *  V is the whole number, or Cluster Size, X is a proportion
         *  of Fermat Remainder from last Composite Number [0 - 1]
         *
         *  @param[in] bnPrime The prime to check.
         *
         *  @return The double value of prime difficulty.
         *
         **/
        double GetPrimeDifficulty(const LLC::CBigNum& bnPrime);


        /** GetPrimeBits
         *
         *  Gets the unsigned int representative of a decimal prime difficulty.
         *
         *  @param[in] bnPrime The prime to get bits for
         *
         *  @return uint32_t representation of prime difficulty.
         *
         **/
        uint32_t GetPrimeBits(const LLC::CBigNum& bnPrime);


        /** GetFractionalDifficulty
         *
         *  Breaks the remainder of last composite in Prime Cluster into an integer.
         *
         *  @param[in] composite The composite number to get remainder of
         *
         *  @return The fractional proportion
         *
         **/
        uint32_t GetFractionalDifficulty(const LLC::CBigNum& bnComposite);


        /** FermatTest
         *
         *  Used after Miller-Rabin and Divisor tests to verify primality.
         *
         *  @param[in] bnPrime The prime to check
         *  @param[in] bnBase The base to check from.
         *
         *  @return The remainder of the fermat test.
         *
         **/
        LLC::CBigNum FermatTest(const LLC::CBigNum& bnPrime, const LLC::CBigNum& bnBase);


        /** MillerRabin
         *
         *  Wrapper for is_prime from OpenSSL
         *
         *  @param[in] bnPrime The prime to test
         *  @param[in] nChecks The times to check the prime.
         *
         *  @return True if bnPrime is prime
         *
         **/
        bool Miller_Rabin(const LLC::CBigNum& bnPrime, uint32_t nChecks);
    }
}

#endif
