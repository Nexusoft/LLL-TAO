/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_PARSE_H
#define NEXUS_UTIL_INCLUDE_PARSE_H

#include <vector>
#include <string>
#include <stdint.h>

#include "debug.h"
#include "convert.h"

#include "../../Core/include/global.h"

#include "../../LLC/types/uint1024.h"


inline void ParseString(const std::string& str, char c, std::vector<std::string>& v)
{
	if (str.empty())
		return;
	
	std::string::size_type i1 = 0;
	std::string::size_type i2;
	{	
		i2 = str.find(c, i1);
		if (i2 == str.npos)
		{
			v.push_back(str.substr(i1));
			
			return;
		}
		v.push_back(str.substr(i1, i2-i1));
		
		i1 = i2+1;
	}
}


inline std::string FormatMoney(int64 n, bool fPlus=false)
{
	// Note: not using straight sprintf here because we do NOT want
	// localized number formatting.
	int64 n_abs = (n > 0 ? n : -n);
	int64 quotient = n_abs / Core::COIN;
	int64 remainder = n_abs % Core::COIN;
	
	std::string str = strprintf("%" PRI64d ".%06" PRI64d "", quotient, remainder);

	// Right-trim excess 0's before the decimal point:
	int nTrim = 0;
	for (int i = str.size()-1; (str[i] == '0' && isdigit(str[i-2])); --i)
		++nTrim;
	
	if (nTrim)
		str.erase(str.size()-nTrim, nTrim);
	
	if (n < 0)
		str.insert((unsigned int)0, 1, '-');
	else if (fPlus && n > 0)
		str.insert((unsigned int)0, 1, '+');
	return str;
}


inline bool ParseMoney(const std::string& str, int64& nRet)
{
	return ParseMoney(str.c_str(), nRet);
}

inline bool ParseMoney(const char* pszIn, int64& nRet)
{
	std::string strWhole;
	int64 nUnits = 0;
	const char* p = pszIn;
	while (isspace(*p))
		p++;
	for (; *p; p++)
	{
		if (*p == '.')
		{
			p++;
			int64 nMult = Core::CENT * 10;
			while (isdigit(*p) && (nMult > 0))
			{
				nUnits += nMult * (*p++ - '0');
				nMult /= 10;
			}
			
			break;
		}
		
		if (isspace(*p))
			break;
		
		if (!isdigit(*p))
			return false;
		
		strWhole.insert(strWhole.end(), *p);
	}
	
	for (; *p; p++)
		if (!isspace(*p))
			return false;
	if (strWhole.size() > 10) // guard against 63 bit overflow
		return false;
	if (nUnits < 0 || nUnits > Core::COIN)
		return false;
	
	int64 nWhole = atoi64(strWhole);
	int64 nValue = nWhole* Core::COIN + nUnits;
	
	nRet = nValue;
	return true;
}




/* Split a string into it's components by delimiter. */
inline std::vector<std::string> Split(const std::string& strInput, char strDelimiter)
{
   std::string::size_type nIndex = 0;
   std::string::size_type nFind  = strInput.find(strDelimiter);

	std::vector<std::string> vData;
   while (nFind != std::string::npos) {
      vData.push_back(strInput.substr(nIndex, nFind - nIndex));
      nIndex = ++ nFind;
      nFind  = strInput.find(strDelimiter, nFind);

      if (nFind == std::string::npos)
         vData.push_back(strInput.substr(nIndex, strInput.length()));
   }
   
   return vData;
}

#endif
