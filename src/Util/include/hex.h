/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_HEX_H
#define NEXUS_UTIL_INCLUDE_HEX_H

#include <string>
#include <vector>

#include <Util/include/debug.h>

/* buffer for determing hex value of ASCII table */
static signed char phexdigit[256] =
{
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
    -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
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
inline bool IsHex(const std::string& str)
{
    for(int i = 0; i < str.size(); i++)
    {
        if (phexdigit[ (uint8_t) str[i] ] < 0)
            return false;
    }
    return (str.size() > 0) && (str.size()%2 == 0);
}

/** ParseHex
 *
 *  Parses a string into multiple hex strings.
 *
 *  @param[in] psz The input string pointer
 *
 *  @return The vector of hex strings
 *
 **/
inline std::vector<uint8_t> ParseHex(const char* psz)
{
    // convert hex dump to vector
    std::vector<uint8_t> vch;
    for(;;)
    {
        while (isspace(*psz))
            psz++;
        signed char c = phexdigit[(uint8_t)*psz++];
        if (c == (signed char)-1)
            break;
        uint8_t n = (c << 4);
        c = phexdigit[(uint8_t)*psz++];
        if (c == (signed char)-1)
            break;
        n |= c;
        vch.push_back(n);
    }
    return vch;
}

/** ParseHex
 *
 *  Parses a string into multiple hex strings.
 *
 *  @param[in] str The input string
 *
 *  @return The vector of hex strings
 *
 **/
inline std::vector<uint8_t> ParseHex(const std::string& str)
{
    return ParseHex(str.c_str());
}

/** HexStr
 *
 *  Builds a hex string from data in a container class.
 *
 *  @param[in] itbegin The iterator container begin
 *
 *  @param[in] itend The iterator container end
 *
 *  @param[in] fSpaces The flag for if there should be spaces
 *
 *  @return The newly created hex string
 *
 **/
template<typename T>
std::string HexStr(const T itbegin, const T itend, bool fSpaces=false)
{
    std::vector<char> rv;
    static char hexmap[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    rv.reserve((itend-itbegin)*3);

    int nTotal = 0;
    for(T it = itbegin; it < itend; ++it, nTotal++)
    {
        uint8_t val = (uint8_t)(*it);
        if(fSpaces && it != itbegin && it != itend && nTotal % 4 == 0)
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
 *  @param[in] vch The character vector
 *
 *  @param[in] fSpaces The flag for if there should be spaces
 *
 *  @return The newly created hex string
 *
 **/
inline std::string HexStr(const std::vector<uint8_t>& vch, bool fSpaces=false)
{
    return HexStr(vch.begin(), vch.end(), fSpaces);
}

/** PrintHex
 *
 *  Prints a hex string from data in a container class.
 *
 *  @param[in] itbegin The iterator container begin
 *
 *  @param[in] itend The iterator container end
 *
 *  @param[in] pszFormat The format specifier string for formatted output
 *
 *  @param[in] fSpaces The flag for if there should be spaces
 *
 **/
template<typename T>
inline void PrintHex(const T pbegin, const T pend, const char* pszFormat="%s\n", bool fSpaces=true)
{
    debug::log(pszFormat, HexStr(pbegin, pend, fSpaces).c_str());
}

/** PrintHex
 *
 *  Prints a hex string from data in a character vector.
 *
 *  @param[in] vch The character vector
 *
 *  @param[in] pszFormat The format specifier string for formatted output
 *
 *  @param[in] fSpaces The flag for if there should be spaces
 *
 **/
inline void PrintHex(const std::vector<uint8_t>& vch, const char* pszFormat="%s\n", bool fSpaces=true)
{
    debug::log(pszFormat, HexStr(vch, fSpaces).c_str());
}

#endif
