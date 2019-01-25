/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/prime.h>
#include <openssl/bn.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Convert Double to unsigned int Representative. */
        uint32_t SetBits(double nDiff)
        {
            /* Bits are with 10^7 significant figures. */
            uint32_t nBits = 10000000;
            nBits *= nDiff;

            return nBits;
        }


        /* Determines the difficulty of the Given Prime Number. */
        double GetPrimeDifficulty(const LLC::CBigNum& bnPrime, int32_t nChecks)
        {
            /* Return 0 if base is not prime. */
            if(!PrimeCheck(bnPrime, nChecks))
                return 0.0;

            /* Set temporary variables for the checks. */
            LLC::CBigNum bnLast = bnPrime;
            LLC::CBigNum bnNext = bnPrime + 2;

            /* Keep track of the cluster size. */
            uint32_t nClusterSize = 1;

            /* Largest prime gap is +12 for dense clusters. */
            for( ; bnNext <= bnLast + 12; bnNext += 2)
            {
                /* Check if this interval is prime. */
                if(PrimeCheck(bnNext, nChecks))
                {
                    bnLast = bnNext;
                    nClusterSize ++;
                }
            }

            /* Calculate the rarity of cluster from proportion of fermat remainder of last prime + 2. */
            double nRemainder = 1000000.0 / GetFractionalDifficulty(bnNext);
            if(nRemainder > 1.0 || nRemainder < 0.0)
                nRemainder = 0.0;

            return (nClusterSize + nRemainder);
        }


        /* Gets the unsigned int representative of a decimal prime difficulty. */
        uint32_t GetPrimeBits(const LLC::CBigNum& bnPrime)
        {
            return SetBits(GetPrimeDifficulty(bnPrime, 1));
        }


        /* Breaks the remainder of last composite in Prime Cluster into an integer. */
        uint32_t GetFractionalDifficulty(const LLC::CBigNum& bnComposite)
    	{
    		return ((bnComposite - FermatTest(bnComposite, 2) << 24) / bnComposite).getuint32();
    	}


        /* Determines if given number is Prime. */
        bool PrimeCheck(const LLC::CBigNum& bnTest, uint32_t nChecks)
        {
            /* Check A: Small Prime Divisor Tests */
            LLC::CBigNum bnPrimes[11] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31 };
            for(const auto& bnPrime : bnPrimes)
                if(bnTest % bnPrime == 0)
                    return false;

            /* Check B: Miller-Rabin Tests */
            if(!Miller_Rabin(bnTest, nChecks))
                return false;

            /* Check C: Fermat Tests */
            for(LLC::CBigNum bnBase = 2; bnBase < 2 + nChecks; bnBase++)
                if(FermatTest(bnTest, bnBase) != 1)
                    return false;

            return true;
        }


        /* Used after Miller-Rabin and Divisor tests to verify primality. */
        LLC::CBigNum FermatTest(const LLC::CBigNum& bnPrime, const LLC::CBigNum& bnBase)
        {
            LLC::CAutoBN_CTX pctx;
            LLC::CBigNum bnExp = bnPrime - 1;

            LLC::CBigNum bnResult;
            BN_mod_exp(bnResult.getBN(), bnBase.getBN(), bnExp.getBN(), bnPrime.getBN(), pctx);

            return bnResult;
        }


        /* Wrapper for is_prime from OpenSSL */
        bool Miller_Rabin(const LLC::CBigNum& bnPrime, uint32_t nChecks)
        {
            return (BN_is_prime_ex(bnPrime.getBN(), nChecks, nullptr, nullptr) == 1);
        }
    }
}
