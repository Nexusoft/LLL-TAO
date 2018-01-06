/*__________________________________________________________________________________________
 
			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
			
			(c) Copyright The Nexus Developers 2014 - 2017
			
			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.
			
			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers
  
____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_SORTING_H
#define NEXUS_UTIL_INCLUDE_SORTING_H

#include <utility>
#include <map>

// Align by increasing pointer, must have extra space at end of buffer
template <size_t nBytes, typename T>
T* alignup(T* p)
{
    union
    {
        T* ptr;
        size_t n;
    } u;
    u.ptr = p;
    u.n = (u.n + (nBytes-1)) & ~(nBytes-1);
    return u.ptr;
}

/** Create a sorted Multimap for rich lists. **/
template <typename A, typename B> std::multimap<B, A> flip_map(std::map<A,B> & src) 
{
	std::multimap<B,A> dst;
	for(typename std::map<A, B>::const_iterator it = src.begin(); it != src.end(); ++it)
		dst.insert(std::pair<B, A>(it -> second, it -> first));

    return dst;
}

#endif
