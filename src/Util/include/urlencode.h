/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_ENCODING_H
#define NEXUS_UTIL_INCLUDE_ENCODING_H

#include <cstring>
#include <Util/include/hex.h>

namespace encoding
{

    /** urlencode
     *
     *  Encode a string into URL format.
     *
     *  @param[in] strValue The string to encode.
     *
     *  @return The encoded string.
     *
     **/
    __attribute__((const)) inline std::string urlencode(const std::string& strValue)
    {
        //RFC 3986 section 2.3 Unreserved Characters (January 2005)
        const std::string strUnreserved = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";

        /* Loop through our values and append to return string. */
        std::string strRet = "";
        for(size_t i=0; i < strValue.length(); i++)
        {
            /* Push regular letter that's not hex value. */
            if(strUnreserved.find_first_of(strValue[i]) != std::string::npos)
                strRet.push_back(strValue[i]);
            else
                strRet.append(debug::safe_printstr("%", std::uppercase, std::hex, uint32_t(strValue[i])));
        }

        return strRet;
    }


    /** urldecode
     *
     *  Encode a string into URL format.
     *
     *  @param[in] strValue The string to encode.
     *
     *  @return The encoded string.
     *
     **/
    __attribute__((const)) inline std::string urldecode(const std::string& strValue)
    {
        /* Loop and convert each hex value to given character. */
        std::string strRet = "";
        for(size_t i = 0; i < strValue.length(); i++)
        {
            /* Check for special characters. */
            if(strValue[i] != '%')
            {
                if(strValue[i] == '+')
                    strRet.push_back(' ');
                else
                    strRet.push_back(strValue[i]);
            }

            /* Handle the hexadecimal encoding. */
            else
            {
                /* Copy into our buffer. */
                char chBuffer[2];
                std::copy((char*)&strValue[i] + 1, (char*)&strValue[i] + 3, (char*)&chBuffer[0]);

                /* Add to our return value. */
                strRet.push_back(HexChar(chBuffer));
                i += 2;
            }
        }

        return strRet;
    }

}

#endif
