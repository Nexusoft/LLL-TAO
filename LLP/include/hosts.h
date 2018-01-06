/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#ifndef NEXUS_LLP_INCLUDE_HOSTS_H
#define NEXUS_LLP_INCLUDE_HOSTS_H

#include <string>
#include <vector>

namespace LLP
{
	
	class CAddress;
	class CNetAddr;
	class CService;
	
	/** These addresses are the first point of contact on the P2P network
	 * They are established and maintained by the owners of each domain.
	 */
	extern std::vector<std::string> DNS_SeedNodes;
	extern std::vector<std::string> DNS_SeedNodes_Testnet;
	
	
	/* The DNS Lookup Routine to find the Nodes that are set as DNS seeds. */
	std::vector<CAddress> DNS_Lookup(std::vector<std::string> DNS_Seed);
	
	
	/* Standard Wrapper Function to Interact with cstdlib DNS functions. */
	bool LookupHost(const char *pszName, std::vector<CNetAddr>& vIP, unsigned int nMaxSolutions = 0, bool fAllowLookup = true);
	bool LookupHostNumeric(const char *pszName, std::vector<CNetAddr>& vIP, unsigned int nMaxSolutions = 0);
	bool Lookup(const char *pszName, CService& addr, int portDefault = 0, bool fAllowLookup = true);
	bool Lookup(const char *pszName, std::vector<CService>& vAddr, int portDefault = 0, bool fAllowLookup = true, unsigned int nMaxSolutions = 0);
	bool LookupNumeric(const char *pszName, CService& addr, int portDefault = 0);
}

#endif
