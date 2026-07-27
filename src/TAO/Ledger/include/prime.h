/*__________________________________________________________________________________________

			Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

			(c) Copyright The Nexus Developers 2014 - 2026

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_PRIME_H
#define NEXUS_TAO_LEDGER_INCLUDE_PRIME_H

#include <LLC/types/uint1024.h>
#include <TAO/Ledger/include/timelocks.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** PRIME_MILLER_RABIN_MIN_VERSION
         *
         *  Minimum block version required for PrimeCheck() to enforce the
         *  Miller-Rabin primality test added by PR #129. Blocks below this
         *  version (i.e. all chain history mined before the hardening was
         *  activated) are validated using only the original small-divisor +
         *  Fermat rule, since Miller-Rabin can never reject a genuine prime
         *  but a not-yet-detected Fermat pseudoprime already accepted into
         *  history would otherwise wedge re-validation on a fresh sync.
         *
         **/
        constexpr uint32_t PRIME_MILLER_RABIN_MIN_VERSION = 9;


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
         *  @param[in] hashPrime The prime to check.
         *  @param[in] vOffsets Optional offsets for quicker checking.
         *  @param[in] fVerify Whether to verify the offsets are actually prime.
         *  @param[in] nVersion Block version, used to gate the Miller-Rabin
         *             hardening (see PRIME_MILLER_RABIN_MIN_VERSION) so it is
         *             only enforced for blocks mined after activation.
         *
         *  @return The double value of prime difficulty.
         *
         **/
        double GetPrimeDifficulty(const uint1024_t& hashPrime, const std::vector<uint8_t>& vOffsets, const bool fVerify = true,
                                   const uint32_t nVersion = CurrentBlockVersion());


        /** GetOffsets
         *
         *  Return list of offsets for use in optimized prime proof of work calculations.
         *
         *  @param[in] hashPrime The prime to check.
         *  @param[out] vOffsets The list of offsets to return.
         *  @param[in] nVersion Block version, used to gate the Miller-Rabin
         *             hardening (see PRIME_MILLER_RABIN_MIN_VERSION).
         *
         *
         **/
        void GetOffsets(const uint1024_t& hashPrime, std::vector<uint8_t> &vOffsets, const uint32_t nVersion = CurrentBlockVersion());


        /** GetPrimeBits
         *
         *  Gets the unsigned int representative of a decimal prime difficulty.
         *
         *  @param[in] bnPrime The prime to get bits for
         *  @param[in] vOffsets Optional offsets for quicker checking.
         *  @param[in] fVerify Whether to verify the offsets are actually prime.
         *  @param[in] nVersion Block version, used to gate the Miller-Rabin
         *             hardening (see PRIME_MILLER_RABIN_MIN_VERSION).
         *
         *  @return uint32_t representation of prime difficulty.
         *
         **/
        uint32_t GetPrimeBits(const uint1024_t& hashPrime, const std::vector<uint8_t>& vOffsets, const bool fVerify = true,
                              const uint32_t nVersion = CurrentBlockVersion());


        /** GetFractionalDifficulty
         *
         *  Breaks the remainder of last composite in Prime Cluster into an integer.
         *
         *  @param[in] hashComposite The composite number to get remainder of
         *
         *  @return The fractional proportion
         *
         **/
        uint32_t GetFractionalDifficulty(const uint1024_t& hashComposite);


        /** PrimeCheck
         *
         *  Determines if given number is Prime.
         *
         *	@param[in] hashTest The number to test for primality
         *  @param[in] nVersion Block version, used to gate the Miller-Rabin
         *             hardening (see PRIME_MILLER_RABIN_MIN_VERSION) so it is
         *             only enforced for blocks mined after activation.
         *
         *  @return True if number passes prime tests.
         *
         **/
        bool PrimeCheck(const uint1024_t& hashTest, const uint32_t nVersion = CurrentBlockVersion());


        /** FermatTest
         *
         *  Used after Miller-Rabin and Divisor tests to verify primality.
         *
         *  @param[in] hashTest The prime to check
         *
         *  @return The remainder of the fermat test.
         *
         **/
        uint1024_t FermatTest(const uint1024_t& hashTest);


        /** MillerRabin
         *
         *  Wrapper for is_prime from OpenSSL
         *
         *  @param[in] hashTest The prime to test
         *
         *  @return True if bnPrime is prime
         *
         **/
        bool Miller_Rabin(const uint1024_t& hashTest);


        /** SmallDivisors
         *
         *  Determine if the number passes small divisor test up to the first
         *  eleven primes.
         *
         *  @param[in] hashTest The prime to test.
         *
         *  @return Returns True if nPrime passes small divisor tests, false otherwise.
         *
         **/
        bool SmallDivisors(const uint1024_t& hashTest);
    }
}

#endif
