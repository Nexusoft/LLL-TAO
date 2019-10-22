/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/prime.h>
#include <LLC/types/bignum.h>
#include <openssl/bn.h>

#include <Util/include/debug.h>
#include <Util/include/softfloat.h>


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
                for(const auto& nOffset : vOffsets)
                {
                    /* Check for valid offsets. */
                    if(nOffset > 12)
                        return 0.0;

                    /* Set the next offset position. */
                    hashNext += nOffset;

                    /* Check prime at offset. */
                    if(!fVerify || PrimeCheck(hashNext))
                        ++nClusterSize;
                }

                /* If offsets all passed, get composite offset. */
                hashNext += 14;
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
            }

            /* Calculate the rarity of cluster from proportion of fermat remainder of last prime + 2. */
            cv::softdouble nRemainder = cv::softdouble(1000000.0) / cv::softdouble(GetFractionalDifficulty(hashNext));
            if(nRemainder > cv::softdouble(1.0) || nRemainder < cv::softdouble(0.0))
                nRemainder = cv::softdouble(0.0);

            return double(nClusterSize + nRemainder);
        }


        /* Return list of offsets for use in optimized prime proof of work calculations. */
        void GetOffsets(const uint1024_t& hashPrime, std::vector<uint8_t> &vOffsets)
        {
            /* Check first prime. */
            if(!PrimeCheck(hashPrime))
                return;

            /* Erase offsets if any */
            vOffsets.clear();
            uint8_t nOffset = 2;

            /* Set temporary variables for the checks. */
            uint1024_t hashLast = hashPrime;
            for(uint1024_t hashNext = hashPrime + 2; nOffset <= 12; hashNext += 2, nOffset += 2)
            {
                /* Check if this interval is prime. */
                if(PrimeCheck(hashNext))
                {
                    hashLast = hashNext;

                    /* Add offset to vector. */
                    vOffsets.push_back(nOffset);
                    nOffset = 0;
                }
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
            /* Small Prime Divisor Tests */
            if(!SmallDivisors(hashTest))
                return false;

            /* Fermat Test */
            if(FermatTest(hashTest) != 1)
                return false;

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
