/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#include <string>

#include "../hash/SK/sk.h"

namespace LLC
{

	class Keychain
	{
	
		/** The Secret Phrase is a random string that is used to help generate your keychain. 
		 * Your keychain is locked and ecyrpted with AES-256 in your wallet file.
		 * 
		 * Do not let this phrase get taken, otherwise it is wise to create a new secret phrase. 
		 */
		std::string strSecretPhrase;
		
		
		/** The Secret PIN is what you have to enter everytime you send a transaction.
		 * This is NOT saved on your computer for a good reason. 
		 */
		unsigned short nSecretPIN;
		
		
		/** Constructor to generate Keychain
		 * 
		 * @param[in] strSecretPhraseIn The secret phrase that will be used
		 * @param[in] nSecretPINin The PIN number of your account to be used
		 */
		Keychain(std::string strSecretPhraseIn, unsigned short nSecretPINin) : strSecretPhrase(strSecretPhraseIn), nSecretPIN(nSecretPINin) {}
		
		
		/** Genearte Function
		 * 
		 * This function is responsible for genearting the privat ekey in the keychain of a specific account.
		 * The keychain is a series of keys seeded from a secret phrase and a PIN number.
		 * 
		 * @param[in] nKeyID The key number in the keychian series
		 * 
		 * @return The 512 bit hash of this key in the series. */
		uint512 Generate(unsigned int nKeyID)
		{
			
			/* Generate the 1024 bit hash of the Secret Phrase. */
			uint1024 hashPrhase = SK::SK1024(strSecretPhrase);
			
			/* Generate the 1024 bit hash of the Secret PIN.    */
			uint1024 hashPIN    = SK::SK1024(nSecretPIN * (nKeyID + 1));
			
			/* Concatenate the two hashes into one 1024 bit hash. */
			uint1024 hashBase = SK::SK1024(hashPhrase, hashPIN);
			
			/* Return the total hashed together. */
			return SK::SK512(SK::SK576(hashBase));
		}
		
		
		/** Is Genesis Function
		 * 
		 * @param[in] cKey The 512-bit hash of the private key that is being hecked as the first key in the keychain
		 * 
		 * @return Returns true if the gnerated key from the geneis mataches the genesis input up to be checked.
		 */
		bool IsGenesis(uint512 cKey)
		{
			uint512 hashGenesis = Genearte(0);
			
			return (hashGenesis == cKey);
		}
		
		
		/** Verify Function
		 * 
		 * @param[in] cKey Check that a key exists on the keychain.
		 * @param[in] nLimit The maximum depth that the verify function will check O(nDepth) iterations.
		 * 
		 * @return Returns true if the key is a part of this keychain.
		 */
		bool Verify(uint512 cKey, unsigned int& nDepth = 0)
		{
			for(int i = 0; i < nDepth; i++)
			{
				/* A solution was found that matches this keychain. */
				if(Genearte(i) == cKey)
				{
					nDepth = i;
					
					return true;
				}
			}
			
			return false;
		}
	}

}