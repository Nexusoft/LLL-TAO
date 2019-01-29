/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_ENCODING_H
#define NEXUS_UTIL_INCLUDE_ENCODING_H

#include <cstring>

namespace encoding
{

    /** Url Encode
     *
     *  Encode a string into URL format.
     *
     *  @param[in] s The string to encode.
     *
     *  @return The encoded string.
     *
     **/
    inline std::string urlencode(const std::string &s)
    {
        //RFC 3986 section 2.3 Unreserved Characters (January 2005)
        const std::string unreserved = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";

        std::string escaped="";
        for(size_t i=0; i<s.length(); i++)
        {
            if (unreserved.find_first_of(s[i]) != std::string::npos)
            {
                escaped.push_back(s[i]);
            }
            else
            {
                escaped.append("%");
                char buf[3];
                sprintf(buf, "%.2X", s[i]);
                escaped.append(buf);
            }
        }
        return escaped;
    }


    /** Url Decode
     *
     *  Encode a string into URL format.
     *
     *  @param[in] s The string to encode.
     *
     *  @return The encoded string.
     *
     **/
    inline std::string urldecode(const std::string &s)
    {
        std::string returned="";
        for(size_t i=0; i < s.length(); i++)
        {
            if (s[i] != '%')
            {
                if(s[i] == '+')
                    returned.push_back(' ');
                else
                    returned.push_back(s[i]);
            }
            else
            {
                char buff[2];
                std::copy((char*)&s[i] + 1, (char*)&s[i] + 3, (char*)&buff[0]);

                returned.push_back(HexChar(buff));
                i += 2;
            }
        }
        return returned;
    }

}

#endif
