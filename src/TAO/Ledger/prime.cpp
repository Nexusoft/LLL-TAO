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
        double GetPrimeDifficulty(const uint1024_t& bnPrime)
        {
            /* Return 0 if base is not prime. */
            if(!PrimeCheck(bnPrime))
                return 0.0;

            /* Set temporary variables for the checks. */
            uint1024_t bnLast = bnPrime;
            uint1024_t bnNext = bnPrime + 2;

            /* Keep track of the cluster size. */
            uint32_t nClusterSize = 1;

            /* Largest prime gap is +12 for dense clusters. */
            for( ; bnNext <= bnLast + 12; bnNext += 2)
            {
                /* Check if this interval is prime. */
                if(PrimeCheck(bnNext))
                {
                    bnLast = bnNext;
                    ++nClusterSize;
                }
            }

            /* Calculate the rarity of cluster from proportion of fermat remainder of last prime + 2. */
            double nRemainder = 1000000.0 / GetFractionalDifficulty(bnNext);
            if(nRemainder > 1.0 || nRemainder < 0.0)
                nRemainder = 0.0;

            return (nClusterSize + nRemainder);
        }


        /* Gets the unsigned int representative of a decimal prime difficulty. */
        uint32_t GetPrimeBits(const uint1024_t& bnPrime)
        {
            return SetBits(GetPrimeDifficulty(bnPrime));
        }


        /* Breaks the remainder of last composite in Prime Cluster into an integer. */
        uint32_t GetFractionalDifficulty(const uint1024_t& nComposite)
    	{
            //LLC::CBigNum a(nComposite);
            //LLC::CBigNum b(FermatTest(nComposite));

            uint1056_t a(nComposite);
            uint1056_t b(FermatTest(nComposite));

            return ( (a - b << 24) / a).getuint32();
    	}


        /* Determines if given number is Prime. */
        bool PrimeCheck(const uint1024_t& nTest)
        {

            /* Check A: Small Prime Divisor Tests */
            if(!SmallDivisors(nTest))
                return false;

            /* Check B: Miller-Rabin Tests */
            //if(!Miller_Rabin(nTest))
            //    return false;

            /* Check C: Fermat Tests */
            if(FermatTest(nTest) != 1)
                return false;

            return true;
        }


        /* Used after Miller-Rabin and Divisor tests to verify primality. */
        uint1024_t FermatTest(const uint1024_t& nPrime)
        {
            LLC::CAutoBN_CTX pctx;

            LLC::CBigNum bnPrime(nPrime);
            LLC::CBigNum bnBase(2);
            LLC::CBigNum bnExp = bnPrime - 1;

            LLC::CBigNum bnResult;
            BN_mod_exp(bnResult.getBN(), bnBase.getBN(), bnExp.getBN(), bnPrime.getBN(), pctx);

            return bnResult.getuint1024();
        }


        /* Wrapper for is_prime from OpenSSL */
        bool Miller_Rabin(const uint1024_t& nPrime)
        {
            LLC::CBigNum bnPrime(nPrime);

            return (BN_is_prime_ex(bnPrime.getBN(), 1, nullptr, nullptr) == 1);
        }


        /*  Determine if the number passes small divisor test up to the first
         *  eleven primes. */
        bool SmallDivisors(const uint1024_t& nTest)
        {
            if(nTest % nSmallPrimes[0] == 0)
                return false;

            if(nTest % nSmallPrimes[1] == 0)
                return false;

            if(nTest % nSmallPrimes[2] == 0)
                return false;

            if(nTest % nSmallPrimes[3] == 0)
                return false;

            if(nTest % nSmallPrimes[4] == 0)
                return false;

            if(nTest % nSmallPrimes[5] == 0)
                return false;

            if(nTest % nSmallPrimes[6] == 0)
                return false;

            if(nTest % nSmallPrimes[7] == 0)
                return false;

            if(nTest % nSmallPrimes[8] == 0)
                return false;

            if(nTest % nSmallPrimes[9] == 0)
                return false;

            if(nTest % nSmallPrimes[10] == 0)
                return false;

            return true;
        }
    }
}
