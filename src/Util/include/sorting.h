/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_SORTING_H
#define NEXUS_UTIL_INCLUDE_SORTING_H

#include <utility>
#include <map>


/** alignup
 *
 *  Align by increasing pointer, must have extra space at end of buffer
 *
 *  @param[in] p Pointer to the buffer to align.
 *
 *  @return Pointer to the aligned buffer
 *
 **/
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


/** flip_map
 *
 *  Create a sorted Multimap for rich lists.
 *
 *  @param[in] src The source map to sort.
 *
 *  @return Sorted multimap containing rich lists.
 *
 **/
template <typename A, typename B>
std::multimap<B, A> flip_map(std::map<A,B> & src)
{
    std::multimap<B,A> dst;
    for(typename std::map<A, B>::const_iterator it = src.begin(); it != src.end(); ++it)
        dst.insert(std::pair<B, A>(it -> second, it -> first));

    return dst;
}

#endif
