/*__________________________________________________________________________________________

        Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

        (c) Copyright The Nexus Developers 2014 - 2025

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/prime.h>
#include <LLC/types/bignum.h>
#include <openssl/bn.h>

#include <Util/include/debug.h>
#include <Util/include/softfloat.h>
#include <Util/include/config.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        static const uint16_t nSmallPrimes[11] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31 };

        /* Convert Double to unsigned int Representative. */
        uint32_t SetBits(double nDiff)
        {
            /* Bits are with 10^7 significant figures. */
            uint32_t nBits = 10000000;
            nBits = static_cast<uint32_t>(nBits * nDiff);

            return nBits;
        }


        /* Determines the difficulty of the Given Prime Number. */
        double GetPrimeDifficulty(const uint1024_t& hashPrime, const std::vector<uint8_t>& vOffsets, const bool fVerify)
        {
            /* Return 0 if base is not prime. */
            if(fVerify && !PrimeCheck(hashPrime))
                return 0.0;

            /* Keep track of the cluster size. */
            uint32_t nClusterSize = 1;

            /* Check for optimized tritium version. */
            uint1024_t hashNext = hashPrime;
            if(!vOffsets.empty())
            {
                /* Loop through offsets pattern. */
                uint32_t nSize = vOffsets.size();
                for(uint32_t n = 0; n < nSize - 4; ++n)
                {
                    /* Get the offset. */
                    uint8_t nOffset = vOffsets[n];

                    /* Check for valid offsets. */
                    if(nOffset > 12)
                        return 0.0;

                    /* Set the next offset position. */
                    hashNext += nOffset;

                    /* Check prime at offset. */
                    if(!fVerify || PrimeCheck(hashNext))
                        ++nClusterSize;

                }

                /* Get fractional difficulty. */
                uint32_t nFraction = 0;
                std::copy((uint8_t*)&vOffsets[nSize - 4], (uint8_t*)&vOffsets[nSize - 1], (uint8_t*)&nFraction);

                /* If verifying check the fractional difficulty. */
                if(fVerify && GetFractionalDifficulty(hashNext + 14) != nFraction)
                    return 0.0;

                /* Calculate the rarity of cluster from proportion of fermat remainder of last prime + 2. */
                cv::softdouble nRemainder = cv::softdouble(1000000.0) / cv::softdouble(nFraction);
                if(nRemainder > cv::softdouble(1.0) || nRemainder < cv::softdouble(0.0))
                    nRemainder = cv::softdouble(0.0);

                return double(nClusterSize + nRemainder);
            }
            else
            {
                /* Set temporary variables for the checks. */
                uint1024_t hashLast = hashPrime;

                /* Largest prime gap is +12 for dense clusters. */
                for(hashNext = hashPrime + 2; hashNext <= hashLast + 12; hashNext += 2)
                {
                    /* Check if this interval is prime. */
                    if(PrimeCheck(hashNext))
                    {
                        hashLast = hashNext;
                        ++nClusterSize;
                    }
                }

                /* Calculate the rarity of cluster from proportion of fermat remainder of last prime + 2. */
                cv::softdouble nRemainder = cv::softdouble(1000000.0) / cv::softdouble(GetFractionalDifficulty(hashNext));
                if(nRemainder > cv::softdouble(1.0) || nRemainder < cv::softdouble(0.0))
                    nRemainder = cv::softdouble(0.0);

                return double(nClusterSize + nRemainder);
            }

            return 0.0;
        }


        /* Return list of offsets for use in optimized prime proof of work calculations. */
        /* Gets the offsets of the prime numbers in the cluster. */
        void GetOffsets(const uint1024_t& hashPrime, std::vector<uint8_t> &vOffsets)
        {
            bool fDiagnostic = (config::nVerbose >= 2);
            
            if(fDiagnostic)
            {
                debug::log(2, FUNCTION, "════════════════════════════════════════");
                debug::log(2, FUNCTION, "   GETOFFSETS DIAGNOSTIC");
                debug::log(2, FUNCTION, "════════════════════════════════════════");
                debug::log(2, FUNCTION, "Input hashPrime: ", hashPrime.ToString().substr(0, 64), "...");
            }
            
            /* Check first prime */
            if(fDiagnostic)
                debug::log(2, FUNCTION, "🔍 Validating base prime...");
            
            if(!PrimeCheck(hashPrime))
            {
                if(fDiagnostic)
                {
                    debug::log(2, FUNCTION, "❌ BASE PRIME FAILED PrimeCheck()");
                    debug::log(2, FUNCTION, "   This is why vOffsets is empty!");
                    debug::log(2, FUNCTION, "   The base number is NOT prime");
                    debug::log(2, FUNCTION, "════════════════════════════════════════");
                }
                return;
            }
            
            if(fDiagnostic)
                debug::log(2, FUNCTION, "✅ Base prime is VALID");

            /* Erase offsets if any */
            vOffsets.clear();
            uint8_t nOffset = 2;

            /* Set temporary variables for the checks. */
            uint1024_t hashLast = hashPrime;
            uint32_t nChainLength = 0;
            
            for(uint1024_t hashNext = hashPrime + 2; nOffset <= 12; hashNext += 2, nOffset += 2)
            {
                /* Check if this interval is prime. */
                if(PrimeCheck(hashNext))
                {
                    if(fDiagnostic)
                        debug::log(2, FUNCTION, "   ✅ Offset ", static_cast<int>(nOffset), " → PRIME");
                    
                    hashLast = hashNext;

                    /* Add offset to vector. */
                    vOffsets.push_back(nOffset);
                    nOffset = 0;
                    ++nChainLength;
                }
                else
                {
                    if(fDiagnostic)
                        debug::log(2, FUNCTION, "   ❌ Offset ", static_cast<int>(nOffset), " → not prime (chain breaks)");
                }
            }

            /* Get fractional difficulty. */
            uint32_t nFraction = GetFractionalDifficulty(hashLast + nOffset);
            vOffsets.insert(vOffsets.end(), (uint8_t*)&nFraction, (uint8_t*)&nFraction + 4);
            
            if(fDiagnostic)
            {
                debug::log(2, FUNCTION, "════════════════════════════════════════");
                debug::log(2, FUNCTION, "Result: ", vOffsets.size() - 4, " offsets found");
                debug::log(2, FUNCTION, "Cunningham chain length: ", nChainLength);
                debug::log(2, FUNCTION, "Fractional difficulty: ", nFraction);
                debug::log(2, FUNCTION, "════════════════════════════════════════");
            }
        }


        /* Gets the unsigned int representative of a decimal prime difficulty. */
        uint32_t GetPrimeBits(const uint1024_t& hashPrime, const std::vector<uint8_t>& vOffsets, const bool fVerify)
        {
            return SetBits(GetPrimeDifficulty(hashPrime, vOffsets, fVerify));
        }


        /* Breaks the remainder of last composite in Prime Cluster into an integer. */
        uint32_t GetFractionalDifficulty(const uint1024_t& hashComposite)
    	{
            //LLC::CBigNum a(nComposite);
            //LLC::CBigNum b(FermatTest(nComposite));

            uint1056_t a(hashComposite);
            uint1056_t b(FermatTest(hashComposite));

            return ((a - b << 24) / a).getuint32();
    	}


        /* Determines if given number is Prime. */
        bool PrimeCheck(const uint1024_t& hashTest)
        {
            /* ✅ Only log diagnostics if debug level >= 2 */
            bool fDiagnostic = (config::nVerbose >= 2);
            
            if(fDiagnostic)
            {
                debug::log(2, FUNCTION, "════════════════════════════════════════");
                debug::log(2, FUNCTION, "   PRIMECHECK DIAGNOSTIC");
                debug::log(2, FUNCTION, "════════════════════════════════════════");
                debug::log(2, FUNCTION, "Input prime (first 64 bytes):");
                debug::log(2, FUNCTION, "  ", hashTest.ToString().substr(0, 64), "...");
            }
            
            /* Check A: Small Prime Divisor Tests */
            if(!SmallDivisors(hashTest))
            {
                if(fDiagnostic)
                {
                    debug::log(2, FUNCTION, "❌ FAILED: Small divisor test");
                    debug::log(2, FUNCTION, "   Prime is divisible by 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, or 31");
                }
                return false;
            }
            if(fDiagnostic)
                debug::log(2, FUNCTION, "✅ PASSED: Small divisor test");

            /* Check B: Miller-Rabin Test (OpenSSL probabilistic primality test) */
            if(!Miller_Rabin(hashTest))
            {
                if(fDiagnostic)
                {
                    debug::log(2, FUNCTION, "❌ FAILED: Miller-Rabin test");
                    debug::log(2, FUNCTION, "   Prime failed cryptographic primality test (PR #129)");
                }
                return false;
            }
            if(fDiagnostic)
                debug::log(2, FUNCTION, "✅ PASSED: Miller-Rabin test");

            /* Check C: Fermat Test */
            if(FermatTest(hashTest) != 1)
            {
                if(fDiagnostic)
                {
                    debug::log(2, FUNCTION, "❌ FAILED: Fermat test");
                }
                return false;
            }
            if(fDiagnostic)
            {
                debug::log(2, FUNCTION, "✅ PASSED: Fermat test");
                debug::log(2, FUNCTION, "════════════════════════════════════════");
                debug::log(2, FUNCTION, "✅ PRIME IS VALID");
                debug::log(2, FUNCTION, "════════════════════════════════════════");
            }

            return true;
        }


        /* Used after Miller-Rabin and Divisor tests to verify primality. */
        uint1024_t FermatTest(const uint1024_t& hashTest)
        {
            LLC::CAutoBN_CTX pctx;

            LLC::CBigNum bnPrime(hashTest);
            LLC::CBigNum bnBase(2);
            LLC::CBigNum bnExp = bnPrime - 1;

            LLC::CBigNum bnResult;
            BN_mod_exp(bnResult.getBN(), bnBase.getBN(), bnExp.getBN(), bnPrime.getBN(), pctx);

            return bnResult.getuint1024();
        }


        /* Wrapper for is_prime from OpenSSL */
        bool Miller_Rabin(const uint1024_t& hashTest)
        {
            LLC::CBigNum bnPrime(hashTest);

            return (BN_is_prime_ex(bnPrime.getBN(), 1, nullptr, nullptr) == 1);
        }


        /*  Determine if the number passes small divisor test up to the first
         *  eleven primes. */
        bool SmallDivisors(const uint1024_t& hashTest)
        {
            if(hashTest % nSmallPrimes[0] == 0)
                return false;

            if(hashTest % nSmallPrimes[1] == 0)
                return false;

            if(hashTest % nSmallPrimes[2] == 0)
                return false;

            if(hashTest % nSmallPrimes[3] == 0)
                return false;

            if(hashTest % nSmallPrimes[4] == 0)
                return false;

            if(hashTest % nSmallPrimes[5] == 0)
                return false;

            if(hashTest % nSmallPrimes[6] == 0)
                return false;

            if(hashTest % nSmallPrimes[7] == 0)
                return false;

            if(hashTest % nSmallPrimes[8] == 0)
                return false;

            if(hashTest % nSmallPrimes[9] == 0)
                return false;

            if(hashTest % nSmallPrimes[10] == 0)
                return false;

            return true;
        }
    }
}
