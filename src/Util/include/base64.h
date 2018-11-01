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

    std::string EncodeBase64(const uint8_t* pch, size_t len);

    inline std::string EncodeBase64(const std::string& str);

    std::vector<uint8_t> DecodeBase64(const char* p, bool* pfInvalid = NULL);

    inline std::string DecodeBase64(const std::string& str);

}
#endif
