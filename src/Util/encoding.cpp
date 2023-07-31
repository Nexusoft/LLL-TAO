/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/bignum.h>
#include <LLC/hash/SK.h>

#include <Util/include/encoding.h>
#include <Util/include/memory.h>

#include <openssl/bn.h>
#include <algorithm>
#include <cstring>

namespace encoding
{
    static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    /* Encode into base58 returning a std::string */
    std::string EncodeBase58(const uint8_t* pbegin, const uint8_t* pend)
    {
        LLC::CAutoBN_CTX pctx;
        LLC::CBigNum bn58 = 58;
        LLC::CBigNum bn0 = 0;

        // Convert big endian data to little endian
        // Extra zero at the end make sure bignum will interpret as a positive number
        std::vector<unsigned char> vchTmp(pend-pbegin+1, 0);
        std::reverse_copy(pbegin, pend, vchTmp.begin());

        // Convert little endian data to bignum
        LLC::CBigNum bn(vchTmp);

        // Convert bignum to std::string
        std::string str;
        // Expected size increase from base58 conversion is approximately 137%
        // use 138% to be safe
        str.reserve((pend - pbegin) * 138 / 100 + 1);
        LLC::CBigNum dv;
        LLC::CBigNum rem;
        while(bn > bn0)
        {
            if(!BN_div(dv.getBN(), rem.getBN(), bn.getBN(), bn58.getBN(), pctx))
                throw LLC::bignum_error("EncodeBase58 : BN_div failed");
            bn = dv;
            unsigned int c = rem.getuint32();
            str += pszBase58[c];
        }

        // Leading zeroes encoded as base58 zeros
        for(const unsigned char* p = pbegin; p < pend && *p == 0; p++)
            str += pszBase58[0];

        // Convert little endian std::string to big endian
        std::reverse(str.begin(), str.end());
        return str;
    }


    /* Encode into base58 returning a std::string */
    std::string EncodeBase58(const std::vector<uint8_t>& vch)
    {
        return EncodeBase58(&vch[0], &vch[0] + vch.size());
    }


    /* Encode into base58 returning a std::string */
    bool DecodeBase58(const char* psz, std::vector<uint8_t>& vchRet)
    {
        LLC::CAutoBN_CTX pctx;
        vchRet.clear();
        LLC::CBigNum bn58 = 58;
        LLC::CBigNum bn = 0;
        LLC::CBigNum bnChar;
        while(isspace(*psz))
            psz++;

        // Convert big endian string to bignum
        for(const char* p = psz; *p; p++)
        {
            const char* p1 = strchr(pszBase58, *p);
            if(p1 == nullptr)
            {
                while(isspace(*p))
                    p++;
                if(*p != '\0')
                    return false;
                break;
            }
            bnChar.setuint32(p1 - pszBase58);
            if(!BN_mul(bn.getBN(), bn.getBN(), bn58.getBN(), pctx))
                throw LLC::bignum_error("DecodeBase58 : BN_mul failed");
            bn += bnChar;
        }

        // Get bignum as little endian data
        std::vector<unsigned char> vchTmp = bn.getvch();

        // Trim off sign byte if present
        if(vchTmp.size() >= 2 && vchTmp.end()[-1] == 0 && vchTmp.end()[-2] >= 0x80)
            vchTmp.erase(vchTmp.end()-1);

        // Restore leading zeros
        int nLeadingZeros = 0;
        for(const char* p = psz; *p == pszBase58[0]; p++)
            nLeadingZeros++;
        vchRet.assign(nLeadingZeros + vchTmp.size(), 0);

        // Convert little endian data to big endian
        std::reverse_copy(vchTmp.begin(), vchTmp.end(), vchRet.end() - vchTmp.size());
        return true;
    }


    /* Decode base58 string into byte vector */
    bool DecodeBase58(const std::string& str, std::vector<uint8_t>& vchRet)
    {
        return DecodeBase58(str.c_str(), vchRet);
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


    /* Decode into base58 including a checksum */
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


    /* Decode into base58 including a checksum */
    bool DecodeBase58Check(const std::string& str, std::vector<uint8_t>& vchRet)
    {
        return DecodeBase58Check(str.c_str(), vchRet);
    }
}
