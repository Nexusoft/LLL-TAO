/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/hash/SK.h>

#include <Util/include/encoding.h>
#include <Util/include/memory.h>

#include <openssl/bn.h>
#include <algorithm>
#include <cstring>

namespace encoding
{
    static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    static const int8_t mapBase58[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
        -1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
        22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
        -1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
        47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    };

    /* Encode into base58 returning a std::string */
    std::string EncodeBase58(const std::vector<uint8_t>& vch)
    {
        /* Make a copy to handle here. */
        std::vector<uint8_t> input = vch;

        // Skip & count leading zeroes.
        int zeroes = 0;
        int length = 0;
        while (input.size() > 0 && input[0] == 0) {
            input.erase(input.begin());
            zeroes++;
        }
        // Allocate enough space in big-endian base58 representation.
        int size = input.size() * 138 / 100 + 1; // log(256) / log(58), rounded up.
        std::vector<unsigned char> b58(size);
        // Process the bytes.
        while (input.size() > 0) {
            int carry = input[0];
            int i = 0;
            // Apply "b58 = b58 * 256 + ch".
            for (std::vector<unsigned char>::reverse_iterator it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
                carry += 256 * (*it);
                *it = carry % 58;
                carry /= 58;
            }

            assert(carry == 0);
            length = i;
            input.erase(input.begin());
        }
        // Skip leading zeroes in base58 result.
        std::vector<unsigned char>::iterator it = b58.begin() + (size - length);
        while (it != b58.end() && *it == 0)
            it++;
        // Translate the result into a string.
        std::string str;
        str.reserve(zeroes + (b58.end() - it));
        str.assign(zeroes, '1');
        while (it != b58.end())
            str += pszBase58[*(it++)];
        return str;
    }


    /* Encode into base58 including a checksum */
    std::string EncodeBase58Check(const std::vector<uint8_t>& vchIn)
    {
        // add 4-byte hash check to the end
        std::vector<unsigned char> vch(vchIn);
        uint256_t hash = LLC::SK256(vch.begin(), vch.end());
        vch.insert(vch.end(), (unsigned char*)&hash, (unsigned char*)&hash + 4);
        return EncodeBase58(vch);
    }


    /* Encode into base58 returning a std::string */
    bool DecodeBase58(const char* psz, std::vector<uint8_t>& vch)
    {
        // Skip leading spaces.
        while (*psz && isspace(*psz))
            psz++;
        // Skip and count leading '1's.
        int zeroes = 0;
        int length = 0;
        while (*psz == '1') {
            zeroes++;
            psz++;
        }
        // Allocate enough space in big-endian base256 representation.
        int size = strlen(psz) * 733 /1000 + 1; // log(58) / log(256), rounded up.
        std::vector<unsigned char> b256(size);
        // Process the characters.
        static_assert(std::size(mapBase58) == 256, "mapBase58.size() should be 256"); // guarantee not out of range
        while (*psz && !isspace(*psz)) {
            // Decode base58 character
            int carry = mapBase58[(uint8_t)*psz];
            if (carry == -1)  // Invalid b58 character
                return false;
            int i = 0;
            for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
                carry += 58 * (*it);
                *it = carry % 256;
                carry /= 256;
            }
            assert(carry == 0);
            length = i;
            psz++;
        }
        // Skip trailing spaces.
        while (isspace(*psz))
            psz++;
        if (*psz != 0)
            return false;
        // Skip leading zeroes in b256.
        std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
        // Copy result into output vector.
        vch.reserve(zeroes + (b256.end() - it));
        vch.assign(zeroes, 0x00);
        while (it != b256.end())
            vch.push_back(*(it++));
        return true;
    }


    /* Decode base58 string into byte vector */
    bool DecodeBase58(const std::string& str, std::vector<uint8_t>& vchRet)
    {
        return DecodeBase58(str.c_str(), vchRet);
    }


    /* Decode into base58 inlucding a checksum */
    bool DecodeBase58Check(const char* psz, std::vector<uint8_t>& vchRet)
    {
        if(!DecodeBase58(psz, vchRet))
            return false;
        if(vchRet.size() < 4)
        {
            vchRet.clear();
            return false;
        }
        uint256_t hash = LLC::SK256(vchRet.begin(), vchRet.end()-4);
        //if(memcmp(&hash, &vchRet.end()[-4], 4) != 0)
        if(memory::compare((uint8_t *)&hash, (uint8_t *)&vchRet.end()[-4], 4) != 0)
        {
            vchRet.clear();
            return false;
        }
        vchRet.resize(vchRet.size()-4);
        return true;
    }


    /* Decode into base58 inlucding a checksum */
    bool DecodeBase58Check(const std::string& str, std::vector<uint8_t>& vchRet)
    {
        return DecodeBase58Check(str.c_str(), vchRet);
    }
}
