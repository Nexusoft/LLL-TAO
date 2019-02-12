/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_CONVERT_H
#define NEXUS_UTIL_INCLUDE_CONVERT_H

#include <cstdlib>
#include <cstdint>
#include <string>
#include <iostream>
#include <Util/include/debug.h>
#include <Util/include/json.h>


namespace convert
{
    /** i64tostr
     *
     *  Converts a 64-bit signed integer into a string.
     *
     *  @param[in] n The 64-bit signed integer
     *
     *  @return the 64-bit integer string
     *
     **/
    inline std::string i64tostr(int64_t n)
    {
        return debug::safe_printstr(n);
    }


    /** itostr
     *
     *  Converts a 32-bit signed integer into a string.
     *
     *  @param[in] n The 32-bit signed integer
     *
     *  @return the integer string
     *
     **/
    inline std::string i32tostr(int n)
    {
        return debug::safe_printstr(n);
    }


    /** atoi64
     *
     *  The ASCII to integer wrapper function
     *
     *  @param[in] psz The input str
     *
     *  @return the 64-bit integer value of the string
     *
     **/
    inline int64_t atoi64(const char* psz)
    {
    #ifdef _MSC_VER
        return _atoi64(psz);
    #else
        return strtoll(psz, nullptr, 10);
    #endif
    }


    /** atoi64
     *
     *  The ASCII to 64-bit integer wrapper for the standard string
     *
     *  @param[in] str The input str
     *
     *  @return the 64-bit integer value of the string
     *
     **/
    inline int64_t atoi64(const std::string& str)
    {
    #ifdef _MSC_VER
        return _atoi64(str.c_str());
    #else
        return strtoll(str.c_str(), nullptr, 10);
    #endif
    }


    /** atoi32
     *
     *  The ASCII to integer wrapper for the standard string
     *
     *  @param[in] str The input str
     *
     *  @return the integer value of the string
     *
     **/
    inline int32_t atoi32(const std::string& str)
    {
        return atoi(str.c_str());
    }


    /** roundint
     *
     *  Rounds the decimal value to the nearest 32-bit integer
     *
     *  @param[in] d the value to round
     *
     *  @return the rounded 32-bit integer value
     *
     **/
    inline int roundint(double d)
    {
        return (int)(d > 0 ? d + 0.5 : d - 0.5);
    }


    /** roundint64
     *
     *  Rounds the decimal value to the nearest 64-bit integer
     *
     *  @param[in] d the value to round
     *
     *  @return the rounded 64-bit integer value
     *
     **/
    inline int64_t roundint64(double d)
    {
        return (int64_t)(d > 0 ? d + 0.5 : d - 0.5);
    }


    /** abs64
     *
     *  Returns the absolute value of the signed 64-bit integer
     *
     *  @param[in] n the input value
     *
     *  @return the 64-bit absolute value
     *
     **/
    inline int64_t abs64(int64_t n)
    {
        return (n >= 0 ? n : -n);
    }


    /** DateTimeStrFormat
     *
     *  Converts a 64-bit time value into a formatted date/time string.
     *
     *  @param[in] pszFormat The date/time format specifier string
     *
     *  @param[in] nTime The time value to format date and time with
     *
     *  @return the formatted date/time string
     *
     **/
    inline std::string DateTimeStrFormat(const char* pszFormat, int64_t nTime)
    {
        time_t n = nTime;
        struct tm* ptmTime = gmtime(&n);
        char pszTime[200];
        strftime(pszTime, sizeof(pszTime), pszFormat, ptmTime);
        return pszTime;
    }


    static const std::string strTimestampFormat = "%Y-%m-%d %H:%M:%S UTC";

    /** DateTimeStrFormat
     *
     *  Converts a 64-bit time value into a formatted date/time string.
     *
     *  @param[in] nTime The time value to format date and time with
     *
     *  @return the formatted date/time string
     *
     **/
    inline std::string DateTimeStrFormat(int64_t nTime)
    {
        return DateTimeStrFormat(strTimestampFormat.c_str(), nTime);
    }


    /** skipspaces
     *
     *  Increments a container iterator while there are spaces to skip
     *
     *  @param[in] it the container iterator to increment
     *
     **/
    template<typename T>
    void skipspaces(T& it)
    {
        while (isspace(*it))
            ++it;
    }


    /** IsSwitchChar
     *
     *  determine if the char is a command line switch token
     *
     *  @param[in] c the character to check
     *
     *  @return True, if the character is a command line switch, false otherwise
     *
     **/
    inline bool IsSwitchChar(char c)
    {
    #ifdef WIN32
        return c == '-' || c == '/';
    #else
        return c == '-';
    #endif
    }


    /** ip_string
     *
     *  Converts a byte vector containing ip into an ip string with dots
     *
     *  @param[in] ip the ip address element vector
     *
     *  @return the ip string
     *
     **/
    inline std::string ip_string(std::vector<uint8_t> ip)
    {
        return debug::safe_printstr(ip[0], ".", ip[1], ".", ip[2], ".", ip[3]);
    }


    /** parse_ip
     *
     *  Parse an IP Address into a Byte Vector from Std::String.
     *
     *  @param[in] ip The string containing the ip address
     *
     *  @return the byte vector containing the 4 elements of the IP address
     *
     **/
    inline std::vector<uint8_t> parse_ip(std::string ip)
    {
        std::vector<uint8_t> bytes(4, 0);
        sscanf(ip.c_str(), "%hhu.%hhu.%hhu.%hhu", &bytes[0], &bytes[1], &bytes[2], &bytes[3]);

        return bytes;
    }


    /** uint2bytes
     *
     * Convert a 32 bit Unsigned Integer to Byte Vector using Bitwise Shifts.
     *
     *  @param[in] UINT The 32-bit unsigned integer
     *
     *  @return the converted byte vector
     *
     **/
    inline std::vector<uint8_t> uint2bytes(uint32_t UINT)
    {
        std::vector<uint8_t> BYTES(4, 0);
        BYTES[0] = UINT >> 24;
        BYTES[1] = UINT >> 16;
        BYTES[2] = UINT >> 8;
        BYTES[3] = UINT;

        return BYTES;
    }


    /** bytes2int
     *
     *  Convert a byte stream into a signed integer 32 bit
     *
     *  @param[in] BYTES The byte vector
     *  @param[in] nOffset The offset into the byte vector
     *
     *  @return the converted 32-bit signed integer
     *
     **/
    inline int bytes2int(std::vector<uint8_t> BYTES, int nOffset = 0)
    {
        return (BYTES[0 + nOffset] << 24) + (BYTES[1 + nOffset] << 16) +
               (BYTES[2 + nOffset] << 8) + BYTES[3 + nOffset];
    }


    /** int2bytes
     *
     *  Convert a 32 bit signed integer to byte vector using bitwise shifts.
     *
     *  @param[in] INT The signed integer
     *
     *  @return the converted byte vector
     *
     **/
    inline std::vector<uint8_t> int2bytes(int INT)
    {
        std::vector<uint8_t> BYTES(4, 0);
        BYTES[0] = INT >> 24;
        BYTES[1] = INT >> 16;
        BYTES[2] = INT >> 8;
        BYTES[3] = INT;

        return BYTES;
    }


    /** bytes2uint
     *
     *  Convert a byte stream into a uint32_t
     *
     *  @param[in] BYTES The byte vector
     *  @param[in] nOffset The offset into the byte vector
     *
     *  @return the converted unsigned integer
     *
     **/
    inline uint32_t bytes2uint(std::vector<uint8_t> BYTES, int nOffset = 0)
    {
        return (BYTES[0 + nOffset] << 24) + (BYTES[1 + nOffset] << 16) + (BYTES[2 + nOffset] << 8) + BYTES[3 + nOffset];
    }


    /** uint2bytes64
     *
     *  Convert a 64-bit Unsigned Integer to Byte Vector using Bitwise Shifts.
     *
     *  @param[in] UINT 64-bit unsigned integer
     *
     *  @return the converted byte vector
     *
     **/
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


    /** bytes2uint64
     *
     *  Converts a byte Vector into a uint64_t.
     *
     *  @param[in] BYTES The byte vector
     *  @param[in] nOffset The offset into the byte vector
     *
     *  @return the converted uint64_t
     *
     **/
    inline uint64_t bytes2uint64(std::vector<uint8_t> BYTES, int nOffset = 0)
    {
        return (bytes2uint(BYTES, nOffset) | ((uint64_t)bytes2uint(BYTES, nOffset + 4) << 32));
    }


    /** string2bytes
     *
     *  Converts a Standard String into a Byte Vector.
     *
     *  @param[in] STRING The standard string
     *
     *  @return the converted byte vector
     *
     **/
    inline std::vector<uint8_t> string2bytes(std::string STRING)
    {
        std::vector<uint8_t> BYTES(STRING.begin(), STRING.end());
        return BYTES;
    }


    /** bytes2string
     *
     *  Converts a Byte Vector into a Standard String.
     *
     *  @param[in] BYTES The byte vector
     *
     *  @return the converted string
     *
     **/
    inline std::string bytes2string(std::vector<uint8_t> BYTES)
    {
        std::string STRING(BYTES.begin(), BYTES.end());
        return STRING;
    }

    /** StringValueTo
    *
    *  Template function for converting a string value to the given template type.
    *  If the value is not a string then no conversion takes place
    *
    *  @param[in] value - a reference to the value to be converted.
    *
    *  @return void.
    *
    **/
    template<typename T>
    void StringValueTo(json::json::value_type& value)
    {
        if(value.is_string())
        {
            std::string strValue = value.get<std::string>();
            std::istringstream ss(strValue);

            T tValueOut = T();
            bool bValueOut = false;
            // if the caller has requested conversion to a bool then test for boolean true|false value first
            // as istringsteam won't convert this implicitly
            if( std::is_same<T, bool>::value && (ss >> std::boolalpha >> bValueOut))
                tValueOut = bValueOut;
            else
                ss >> tValueOut;

            value = tValueOut;
        }
    }
}

#endif
