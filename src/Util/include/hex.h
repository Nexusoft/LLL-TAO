/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_HEX_H
#define NEXUS_UTIL_INCLUDE_HEX_H

#include <string>
#include <vector>

#include <Util/include/debug.h>


const char hexmap[16] =
{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};


/** IsHex
 *
 *  Determines if the input string is in all hex encoding or not
 *
 *  @param[in] str The input string for hex scan
 *
 *  @return True if the string is all hex, false otherwise
 *
 **/
bool IsHex(const std::string& str);


/** HexChar
 *
 *  Gets a char from a hex string.
 *
 *  @param[in] psz The input string pointer
 *
 *  @return The char to return.
 *
 **/
char HexChar(const char* psz);


/** ParseHex
 *
 *  Parses a string into multiple hex strings.
 *
 *  @param[in] psz The input string pointer
 *
 *  @return The vector of hex strings
 *
 **/
std::vector<uint8_t> ParseHex(const char* psz);


/** ParseHex
 *
 *  Parses a string into multiple hex strings.
 *
 *  @param[in] str The input string
 *
 *  @return The vector of hex strings
 *
 **/
std::vector<uint8_t> ParseHex(const std::string& str);


/** HexStr
 *
 *  Builds a hex string from data in a container class.
 *
 *  @param[in] itbegin The iterator container begin.
 *  @param[in] itend The iterator container end.
 *  @param[in] fSpaces The flag for if there should be spaces.
 *
 *  @return The newly created hex string.
 *
 **/
template<typename T>
std::string HexStr(const T itbegin, const T itend, bool fSpaces = false)
{
    std::vector<char> rv;

    rv.reserve((itend-itbegin)*3);

    int nTotal = 0;
    for(T it = itbegin; it < itend; ++it, ++nTotal)
    {
        uint8_t val = (uint8_t)(*it);
        if(fSpaces && it != itbegin && nTotal % 32 == 0)
            rv.push_back('\n');
        else if(fSpaces && it != itbegin && it != itend && nTotal % 4 == 0)
            rv.push_back(' ');
        rv.push_back(hexmap[val>>4]);
        rv.push_back(hexmap[val&15]);
    }

    return std::string(rv.begin(), rv.end());
}


/** HexStr
 *
 *  Builds a hex string from data in a vector.
 *
 *  @param[in] vch The character vector.
 *  @param[in] fSpaces The flag for if there should be spaces.
 *
 *  @return The newly created hex string
 *
 **/
std::string HexStr(const std::vector<uint8_t>& vch, bool fSpaces = false);


/** PrintHex
 *
 *  Prints a hex string from data in a container class.
 *
 *  @param[in] itbegin The iterator container begin.
 *  @param[in] itend The iterator container end.
 *  @param[in] fSpaces The flag for if there should be spaces.
 *
 **/
template<typename T>
inline void PrintHex(const T pbegin, const T pend, bool fSpaces = true)
{
    debug::log(0, HexStr(pbegin, pend, fSpaces));
}


/** PrintHex
 *
 *  Prints a hex string from data in a character vector.
 *
 *  @param[in] vch The character vector.
 *  @param[in] pszFormat The format specifier string for formatted output.
 *  @param[in] fSpaces The flag for if there should be spaces.
 *
 **/
void PrintHex(const std::vector<uint8_t>& vch, bool fSpaces = true);


/** HexBits
 *
 *  Converts bits to a hex string.
 *
 *  @param[in] nBits The bits to convert
 *
 * @return The newly created hex string
 **/
std::string HexBits(uint32_t nBits);

#endif
