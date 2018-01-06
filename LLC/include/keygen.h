/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#ifndef NEXUS_LLC_INCLUDE_KEYGEN_H
#define NEXUS_LLC_INCLUCE_KEYGEN_H

//TODO: Integrate the key.cpp from wallet/key.cpp into the keygen class for extended use with ECC hash based key suites and signature chains
namespace LLC 
{
    
    /* The base key of a keychain / sigchain is the entry location for the tritium key suites to genearte your "login" into the cryptography of the network. */
    class GenesisKey
    {
    private:
        
        /** This is the main seed for your key that should be kept on paper. It is used in part with your PIN to create your signature chain from your basekey. **/
        std::string strBaseSeed;
        
        /** This is the PIN that's used to generate your basekey and multipled as the multiplier concatenated with the base seed. **/
        unsigned int nPIN;
        
        
        
    };
    
    //NOTE: Do not use a signature key more than once... Even under classical computing circumstances this can result in the key being stolen by reversing on a side-channel attack with as few as 200 signatures

}
