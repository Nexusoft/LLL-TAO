/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#ifndef NEXUS_LLD_TEMPLATES_KEYCHAIN_H
#define NEXUS_LLD_TEMPLATES_KEYCHAIN_H

#include "key.h"

#include "../../Util/include/args.h"

namespace LLD
{
	/** Handle the Registry of Shared Keychain Pointer Objects. **/
	extern std::map<std::string, KeyDatabase*> mapKeychainRegistry;
	
	/** Handle the Key Registry. **/
	extern Mutex_t REGISTRY_MUTEX;
	
	/** Handle the Registrying of Keychains for LLD Sectors. **/
	void RegisterKeychain(std::string strRegistryName, std::string strBaseName);
	
	/** Return the Keychain Pointer Object. **/
	KeyDatabase* GetKeychain(std::string strRegistryName);
}

#endif
