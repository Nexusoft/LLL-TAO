/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/hex.h>
#include <LLC/hash/macro.h>
#include <LLP/include/network.h>


/* buffer for determing hex value of ASCII table */
const signed char phexdigit[256] =
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


/*  Determines if the input string is in all hex encoding or not */
bool IsHex(const std::string& str)
{
    uint64_t s = str.size();
    for(uint64_t i = 0; i < s; ++i)
    {
        if (phexdigit[ (uint8_t) str[i] ] < 0)
            return false;
    }
    return (s > 0) && (s % 2 == 0);
}


/*  Gets a char from a hex string. */
char HexChar(const char* psz)
{
    int8_t c = phexdigit[(uint8_t)*psz++];
    if (c == -1)
        return 0;
    uint8_t n = static_cast<uint8_t>(c << 4);
    c = phexdigit[(uint8_t)*psz++];
    if (c == -1)
        return 0;
    n |= static_cast<uint8_t>(c);

    return n;
}


/*  Parses a string into multiple hex strings. */
std::vector<uint8_t> ParseHex(const char* psz)
{
    // convert hex dump to vector
    std::vector<uint8_t> vch;
    for(;;)
    {
        while (isspace(*psz))
            ++psz;

        int8_t c = phexdigit[(uint8_t)*psz++];

        if (c == -1)
            break;

        uint8_t n = static_cast<uint8_t>(c << 4);
        c = phexdigit[(uint8_t)*psz++];

        if (c == -1)
            break;

        n |= static_cast<uint8_t>(c);
        vch.push_back(n);
    }
    return vch;
}


/*  Parses a string into multiple hex strings. */
std::vector<uint8_t> ParseHex(const std::string& str)
{
    return ParseHex(str.c_str());
}


/*  Builds a hex string from data in a vector. */
std::string HexStr(const std::vector<uint8_t>& vch, bool fSpaces)
{
    return HexStr(vch.begin(), vch.end(), fSpaces);
}


/*  Prints a hex string from data in a character vector. */
void PrintHex(const std::vector<uint8_t>& vch, bool fSpaces)
{
    debug::log(0, HexStr(vch, fSpaces));
}


/* Converts bits to a hex string. */
std::string HexBits(uint32_t nBits)
{
    union
    {
        int32_t nBits;
        char cBits[4];
    } uBits;

    uBits.nBits = htonl((int32_t)nBits);
    return HexStr(BEGIN(uBits.cBits), END(uBits.cBits));
}
