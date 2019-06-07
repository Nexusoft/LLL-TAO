/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/types/bignum.h>
#include <LLC/include/random.h>
#include <TAO/Ledger/include/prime.h>
#include <unit/catch2/catch.hpp>
#include <openssl/bn.h>


const LLC::CBigNum bnPrimes[11] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31 };


/* Used after Miller-Rabin and Divisor tests to verify primality. */
LLC::CBigNum FermatTest2(const LLC::CBigNum& bnPrime, const LLC::CBigNum& bnBase)
{
    LLC::CAutoBN_CTX pctx;
    LLC::CBigNum bnExp = bnPrime - 1;

    LLC::CBigNum bnResult;
    BN_mod_exp(bnResult.getBN(), bnBase.getBN(), bnExp.getBN(), bnPrime.getBN(), pctx);

    return bnResult;
}


/* Wrapper for is_prime from OpenSSL */
bool Miller_Rabin2(const LLC::CBigNum& bnPrime, uint32_t nChecks)
{
    return (BN_is_prime_ex(bnPrime.getBN(), nChecks, nullptr, nullptr) == 1);
}

/*  Determine if the number passes small divisor test up to the first
 *  eleven primes. */
bool SmallDivisors2(const LLC::CBigNum& bnTest)
{
    for(const auto& bnPrime : bnPrimes)
        if(bnTest % bnPrime == 0)
            return false;

    return true;
}

/* Determines if given number is Prime. */
bool PrimeCheck2(const LLC::CBigNum& bnTest, uint32_t nChecks)
{
    /* Check A: Small Prime Divisor Tests */
    if(!SmallDivisors2(bnTest))
        return false;

    /* Check B: Miller-Rabin Tests */
    if(!Miller_Rabin2(bnTest, nChecks))
        return false;

    /* Check C: Fermat Tests */
    for(LLC::CBigNum bnBase = 2; bnBase < 2 + nChecks; ++bnBase)
        if(FermatTest2(bnTest, bnBase) != 1)
            return false;

    return true;
}


/* Breaks the remainder of last composite in Prime Cluster into an integer. */
uint32_t GetFractionalDifficulty2(const LLC::CBigNum& bnComposite)
{
    return ((bnComposite - FermatTest2(bnComposite, 2) << 24) / bnComposite).getuint32();
}

/* Determines the difficulty of the Given Prime Number. */
double GetPrimeDifficulty2(const LLC::CBigNum& bnPrime, uint32_t nChecks)
{
    /* Return 0 if base is not prime. */
    if(!PrimeCheck2(bnPrime, nChecks))
        return 0.0;

    /* Set temporary variables for the checks. */
    LLC::CBigNum bnLast = bnPrime;
    LLC::CBigNum bnNext = bnPrime + 2;

    /* Keep track of the cluster size. */
    uint32_t nClusterSize = 1;

    /* Largest prime gap is +12 for dense clusters. */
    for(; bnNext <= bnLast + 12; bnNext += 2)
    {
        /* Check if this interval is prime. */
        if(PrimeCheck2(bnNext, nChecks))
        {
            bnLast = bnNext;
            ++nClusterSize;
        }
    }

    /* Calculate the rarity of cluster from proportion of fermat remainder of last prime + 2. */
    double nRemainder = 1000000.0 / GetFractionalDifficulty2(bnNext);
    if(nRemainder > 1.0 || nRemainder < 0.0)
        nRemainder = 0.0;

    return (nClusterSize + nRemainder);
}


/* Gets the unsigned int representative of a decimal prime difficulty. */
uint32_t GetPrimeBits2(const LLC::CBigNum& bnPrime)
{
    return TAO::Ledger::SetBits(GetPrimeDifficulty2(bnPrime, 1));
}


using namespace LLC;


TEST_CASE( "Prime Tests", "[Ledger]")
{
    for(uint32_t i = 0; i < 1000; ++i) 
    {
        uint1024_t nComposite = GetRand1024() |= 1;

        uint1024_t bn1(nComposite);
        CBigNum bn2(nComposite);

        REQUIRE(TAO::Ledger::SmallDivisors(bn1) == SmallDivisors2(bn2));

        REQUIRE(TAO::Ledger::PrimeCheck(bn1) == PrimeCheck2(bn2, 1));

        REQUIRE(TAO::Ledger::GetFractionalDifficulty(bn1) == GetFractionalDifficulty2(bn2));

        REQUIRE(TAO::Ledger::GetPrimeBits(bn1) == GetPrimeBits2(bn2));
    }

}
