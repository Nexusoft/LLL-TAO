/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_BASE64_H
#define NEXUS_UTIL_INCLUDE_BASE64_H

#include <string>
#include <vector>

namespace encoding
{

    /** EncodeBase64
     *
     *  Take a raw byte buffer and a encode it into base 64
     *
     *  @param[in] pch Pointer to the character buffer
     *
     *  @param[in] len size of the buffer in bytes
     *
     *  @return The string of the base 64 encoded buffer
     *
     **/
    std::string EncodeBase64(const uint8_t* pch, size_t len);


    /** EncodeBase64
     *
     *  Take a string and a encode it into base 64
     *
     *  @param[in] str the string to encode
     *
     *  @return The string of the base 64 encoded buffer
     *
     **/
    inline std::string EncodeBase64(const std::string& str);


    /** DecodeBase64
     *
     *  Take an encoded base 64 buffer and decode it into it's original message.
     *
     *  @param[in] p Pointer to the encoded buffer
     *
     *  @param[in] pfInvalid Pointer to invalid flag
     *
     *  @return The vector containing the decoded base 64 message.
     *
     **/
    std::vector<uint8_t> DecodeBase64(const char* p, bool* pfInvalid = NULL);

    /** DecodeBase64
     *
     *  Take an encoded base 64 string and decode it into it's original message.
     *
     *  @param[in] str The encoded base 64 string.
     *
     *  @return The string containing the decoded base 64 message.
     *
     **/
    inline std::string DecodeBase64(const std::string& str);

}
#endif
