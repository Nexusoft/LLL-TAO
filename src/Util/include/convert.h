/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++
            
            (c) Copyright The Nexus Developers 2014 - 2017
            
            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.
            
            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_CONVERT_H
#define NEXUS_UTIL_INCLUDE_CONVERT_H

#include <cstdlib>
#include <stdint.h>
#include <string>

#include "debug.h"

inline std::string i64tostr(int64_t n)
{
    return strprintf("%" PRI64d, n);
}


inline std::string itostr(int n)
{
    return strprintf("%d", n);
}


inline int64_t atoi64(const char* psz)
{
#ifdef _MSC_VER
    return _atoi64(psz);
#else
    return strtoll(psz, NULL, 10);
#endif
}


inline int64_t atoi64(const std::string& str)
{
#ifdef _MSC_VER
    return _atoi64(str.c_str());
#else
    return strtoll(str.c_str(), NULL, 10);
#endif
}


inline int atoi(const std::string& str)
{
    return atoi(str.c_str());
}


inline int roundint(double d)
{
    return (int)(d > 0 ? d + 0.5 : d - 0.5);
}


inline int64_t roundint64(double d)
{
    return (int64_t)(d > 0 ? d + 0.5 : d - 0.5);
}


inline int64_t abs64(int64_t n)
{
    return (n >= 0 ? n : -n);
}


inline std::string DateTimeStrFormat(const char* pszFormat, int64_t nTime)
{
    time_t n = nTime;
    struct tm* ptmTime = gmtime(&n);
    char pszTime[200];
    strftime(pszTime, sizeof(pszTime), pszFormat, ptmTime);
    return pszTime;
}


static const std::string strTimestampFormat = "%Y-%m-%d %H:%M:%S UTC";
inline std::string DateTimeStrFormat(int64_t nTime)
{
    return DateTimeStrFormat(strTimestampFormat.c_str(), nTime);
}


template<typename T>
void skipspaces(T& it)
{
    while (isspace(*it))
        ++it;
}


inline bool IsSwitchChar(char c)
{
#ifdef WIN32
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}


inline std::string ip_string(std::vector<uint8_t> ip) { return strprintf("%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]); }


/* Parse an IP Address into a Byte Vector from Std::String. */
inline std::vector<uint8_t> parse_ip(std::string ip)
{
    std::vector<uint8_t> bytes(4, 0);
    sscanf(ip.c_str(), "%hhu.%hhu.%hhu.%hhu", &bytes[0], &bytes[1], &bytes[2], &bytes[3]);
    
    return bytes;
}


/* Convert a 32 bit Unsigned Integer to Byte Vector using Bitwise Shifts. */
inline std::vector<uint8_t> uint2bytes(uint32_t UINT)
{
    std::vector<uint8_t> BYTES(4, 0);
    BYTES[0] = UINT >> 24;
    BYTES[1] = UINT >> 16;
    BYTES[2] = UINT >> 8;
    BYTES[3] = UINT;
                
    return BYTES;
}


/* Convert a byte stream into a signed integer 32 bit. */	
inline int bytes2int(std::vector<uint8_t> BYTES, int nOffset = 0) { return (BYTES[0 + nOffset] << 24) + (BYTES[1 + nOffset] << 16) + (BYTES[2 + nOffset] << 8) + BYTES[3 + nOffset]; }
        

/* Convert a 32 bit signed Integer to Byte Vector using Bitwise Shifts. */
inline std::vector<uint8_t> int2bytes(int INT)
{
    std::vector<uint8_t> BYTES(4, 0);
    BYTES[0] = INT >> 24;
    BYTES[1] = INT >> 16;
    BYTES[2] = INT >> 8;
    BYTES[3] = INT;
                
    return BYTES;
}
            
            
/* Convert a byte stream into uint32_teger 32 bit. */	
inline uint32_t bytes2uint(std::vector<uint8_t> BYTES, int nOffset = 0) { return (BYTES[0 + nOffset] << 24) + (BYTES[1 + nOffset] << 16) + (BYTES[2 + nOffset] << 8) + BYTES[3 + nOffset]; }		
            
            
/* Convert a 64 bit Unsigned Integer to Byte Vector using Bitwise Shifts. */
inline std::vector<uint8_t> uint2bytes64(uint64_t UINT)
{
    std::vector<uint8_t> INTS[2];
    INTS[0] = uint2bytes((uint32_t) UINT);
    INTS[1] = uint2bytes((uint32_t) (UINT >> 32));
                
    std::vector<uint8_t> BYTES;
    BYTES.insert(BYTES.end(), INTS[0].begin(), INTS[0].end());
    BYTES.insert(BYTES.end(), INTS[1].begin(), INTS[1].end());
                
    return BYTES;
}

            
/* Convert a byte Vector into uint32_teger 64 bit. */
inline uint64_t bytes2uint64(std::vector<uint8_t> BYTES, int nOffset = 0) { return (bytes2uint(BYTES, nOffset) | ((uint64_t)bytes2uint(BYTES, nOffset + 4) << 32)); }


/* Convert Standard String into Byte Vector. */
inline std::vector<uint8_t> string2bytes(std::string STRING)
{
    std::vector<uint8_t> BYTES(STRING.begin(), STRING.end());
    return BYTES;
}


/* Convert Byte Vector into Standard String. */
inline std::string bytes2string(std::vector<uint8_t> BYTES)
{
    std::string STRING(BYTES.begin(), BYTES.end());
    return STRING;
}

#endif
