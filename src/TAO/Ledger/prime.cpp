/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/prime.h>
#include <openssl/bn.h>

#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        static const LLC::CBigNum bnPrimes[2] = { 3, 5 };

        /* Convert Double to unsigned int Representative. */
        uint32_t SetBits(double nDiff)
        {
            /* Bits are with 10^7 significant figures. */
            uint32_t nBits = 10000000;
            nBits = static_cast<uint32_t>(nBits * nDiff);

            return nBits;
        }


        /* Determines the difficulty of the Given Prime Number. */
        double GetPrimeDifficulty(const LLC::CBigNum& bnPrime)
        {
            /* Return 0 if base is not prime. */
            if(FermatTest(bnPrime, 2) != 1)
                return 0.0;

            std::vector<bool> bit_seive(128, 0);

            /* Largest prime gap is +12 for dense clusters. */
            LLC::CBigNum bnNext = bnPrime + 2;


            /* Set temporary variables for the checks. */
            LLC::CBigNum bnLast = bnPrime;


            /* Keep track of the cluster size. */
            uint32_t nClusterSize = 1;

            /* Largest prime gap is +12 for dense clusters. */
            for(uint32_t nTotal = 0; bnNext <= bnLast + 12; bnNext += 2, nTotal += 2)
            {
                if(bit_seive[nTotal])
                    continue;

                if(bnNext % 3 == 0)
                {
                    //debug::log(0, "DIV 3 ", bnPrime.ToString());
                    for(int i = nTotal; i < bit_seive.size(); i += 6)
                        bit_seive[i] = true;

                    continue;
                }

                if(bnNext % 5 == 0)
                {
                    for(int i = nTotal; i < bit_seive.size(); i += 10)
                        bit_seive[i] = true;

                    continue;
                }

                if(bnNext % 7 == 0)
                {
                    for(int i = nTotal; i < bit_seive.size(); i += 14)
                        bit_seive[i] = true;

                    continue;
                }

                if(bnNext % 11 == 0)
                {
                    for(int i = nTotal; i < bit_seive.size(); i += 22)
                        bit_seive[i] = true;

                    continue;
                }

                if(bnNext % 13 == 0)
                {
                    for(int i = nTotal; i < bit_seive.size(); i += 26)
                        bit_seive[i] = true;

                    continue;
                }

                if(bnNext % 17 == 0)
                {
                    for(int i = nTotal; i < bit_seive.size(); i += 34)
                        bit_seive[i] = true;

                    continue;
                }

                if(bnNext % 19 == 0)
                {
                    for(int i = nTotal; i < bit_seive.size(); i += 38)
                        bit_seive[i] = true;

                    continue;
                }

                /* Check if this interval is prime. */
                if(FermatTest(bnNext, 2) == 1)
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
        uint32_t GetPrimeBits(const LLC::CBigNum& bnPrime)
        {
            return SetBits(GetPrimeDifficulty(bnPrime));
        }


        /* Breaks the remainder of last composite in Prime Cluster into an integer. */
        uint32_t GetFractionalDifficulty(const LLC::CBigNum& bnComposite)
    	{
    		return ((bnComposite - FermatTest(bnComposite, 2) << 24) / bnComposite).getuint32();
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
