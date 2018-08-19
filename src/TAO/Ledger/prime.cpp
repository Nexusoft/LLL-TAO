/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++
			
			(c) Copyright The Nexus Developers 2014 - 2018
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#include "include/prime.h"
#include "../LLC/types/bignum.h"

using namespace std;

namespace Core
{

	/* Convert Double to uint32_t Representative. Used for encoding / decoding prime difficulty from nBits. */
	uint32_t SetBits(double nDiff)
	{
		uint32_t nBits = 10000000;
		nBits *= nDiff;
		
		return nBits;
	}

	
	/* Determines the difficulty of the Given Prime Number.
	 * Difficulty is represented as so V.X
	 * V is the whole number, or Cluster Size, X is a proportion
	 * of Fermat Remainder from last Composite Number [0 - 1] */
	double GetPrimeDifficulty(CBigNum prime, int checks)
	{
		if(!PrimeCheck(prime, checks))
			return 0.0; ///difficulty of a composite number
			
		CBigNum lastPrime = prime;
		CBigNum next = prime + 2;
		uint32_t clusterSize = 1;
		
		/* Largest prime gap in cluster can be + 12
		 * this was determined by previously found clusters 
		 * up to 17 primes */
		for( ; next <= lastPrime + 12 ; next += 2)
		{
			if(PrimeCheck(next, checks))
			{
				lastPrime = next;
				++clusterSize;
			}
		}
		
		/* Calulate the rarety of cluster from proportion of fermat remainder of last prime + 2
		 * keep fractional remainder in bounds of [0, 1] */
		double fractionalRemainder = 1000000.0 / GetFractionalDifficulty(next);
		if(fractionalRemainder > 1.0 || fractionalRemainder < 0.0)
			fractionalRemainder = 0.0;
		
		return (clusterSize + fractionalRemainder);
	}

	
	/* Gets the uint32_t representative of a decimal prime difficulty */
	uint32_t GetPrimeBits(CBigNum prime)
	{
		return SetBits(GetPrimeDifficulty(prime, 1));
	}

	
	/* Breaks the remainder of last composite in Prime Cluster into an integer. 
	 * Larger numbers are more rare to find, so a proportion can be determined 
	 * to give decimal difficulty between whole number increases. */
	uint32_t GetFractionalDifficulty(CBigNum composite)
	{
		/** Break the remainder of Fermat test to calculate fractional difficulty [Thanks Sunny] **/
		return ((composite - FermatTest(composite, 2) << 24) / composite).getuint();
	}

	
	/* Determines if given number is Prime. Accuracy can be determined by "checks". 
	 * The default checks the Nexus Network uses is 2 */
	bool PrimeCheck(CBigNum test, int checks)
	{
		/* Check A: Small Prime Divisor Tests */
		CBigNum primes[11] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31 };
		for(int index = 0; index < 11; index++)
			if(test % primes[index] == 0)
				return false;

		/* Check B: Miller-Rabin Tests */
		bool millerRabin = Miller_Rabin(test, checks);
		if(!millerRabin)
			return false;
			
		/* Check C: Fermat Tests */
		for(CBigNum n = 2; n < 2 + checks; n++)
			if(FermatTest(test, n) != 1)
				return false;
		
		return true;
	}

	
	/* Simple Modular Exponential Equation a^(n - 1) % n == 1 or notated in Modular Arithmetic a^(n - 1) = 1 [mod n]. 
	 * a = Base or 2... 2 + checks, n is the Prime Test. Used after Miller-Rabin and Divisor tests to verify primality. */
	CBigNum FermatTest(CBigNum n, CBigNum a)
	{
		CAutoBN_CTX pctx;
		CBigNum e = n - 1;
		CBigNum r;
		BN_mod_exp(&r, &a, &e, &n, pctx);
		
		return r;
	}

	
	/* Miller-Rabin Primality Test from the OpenSSL BN Library. */
	bool Miller_Rabin(CBigNum n, int checks)
	{
		return (BN_is_prime(&n, checks, NULL, NULL, NULL) == 1);
	}

}

