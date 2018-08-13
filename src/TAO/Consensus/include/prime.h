/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"ad vocem populi" - To The Voice of The People
  
____________________________________________________________________________________________*/

#ifndef NEXUS_CORE_INCLUDE_PRIME_H
#define NEXUS_CORE_INCLUDE_PRIME_H

class CBigNum;

namespace Core
{

	/* Set the Bits of the Prime Difficulty into a Unsigned Integer Number by Multiplying and Dividing by 10 ^ Significant Digits. */
	unsigned int SetBits(double nDiff);
	
	
	/* Get the Difficulty of a given prime number by checking its Cluster Size and Fermat Remainder Proportion of last +2 Compositie. */
	double GetPrimeDifficulty(CBigNum prime, int checks);
	
	
	/* Get the Number of Bits in the Prime Number (by its difficulty) to compare with out unsigned integers. */
	unsigned int GetPrimeBits(CBigNum prime);
	
	
	/* Get the Last Composite Fermat Remainder Proportion for calculating the decimal aspect of the Prime Difficulty. */
	unsigned int GetFractionalDifficulty(CBigNum composite);
	
	
	/* Check a Prime Number first by Divisor, then Fermats, then Miller=Rabin to detect if it is an effective prime number. */
	bool PrimeCheck(CBigNum test, int checks);
	
	
	/* Simple Modular Arithmatic Equation in base (n) to detect if a number could potentially be prime without divisor testing. */
	CBigNum FermatTest(CBigNum n, CBigNum a);
	
	
	/* Secondary Test ported rom OpenSSL to double check the fermat and divisor checks for the most accurate prime calculations. */
	bool Miller_Rabin(CBigNum n, int checks);
	
}

#endif
