/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/bignum.h>
#include <LLC/hash/SK.h>
#include <Util/include/base58.h>
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
        while (bn > bn0)
        {
            if (!BN_div(dv.getBN(), rem.getBN(), bn.getBN(), bn58.getBN(), pctx))
                throw LLC::bignum_error("EncodeBase58 : BN_div failed");
            bn = dv;
            unsigned int c = rem.getulong();
            str += pszBase58[c];
        }

        // Leading zeroes encoded as base58 zeros
        for (const unsigned char* p = pbegin; p < pend && *p == 0; p++)
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
        while (isspace(*psz))
            psz++;

        // Convert big endian string to bignum
        for (const char* p = psz; *p; p++)
        {
            const char* p1 = strchr(pszBase58, *p);
            if (p1 == NULL)
            {
                while (isspace(*p))
                    p++;
                if (*p != '\0')
                    return false;
                break;
            }
            bnChar.setulong(p1 - pszBase58);
            if (!BN_mul(bn.getBN(), bn.getBN(), bn58.getBN(), pctx))
                throw LLC::bignum_error("DecodeBase58 : BN_mul failed");
            bn += bnChar;
        }

        // Get bignum as little endian data
        std::vector<unsigned char> vchTmp = bn.getvch();

        // Trim off sign byte if present
        if (vchTmp.size() >= 2 && vchTmp.end()[-1] == 0 && vchTmp.end()[-2] >= 0x80)
            vchTmp.erase(vchTmp.end()-1);

        // Restore leading zeros
        int nLeadingZeros = 0;
        for (const char* p = psz; *p == pszBase58[0]; p++)
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


    /* Decode into base58 inlucding a checksum */
    bool DecodeBase58Check(const char* psz, std::vector<uint8_t>& vchRet)
    {
        if (!DecodeBase58(psz, vchRet))
            return false;
        if (vchRet.size() < 4)
        {
            vchRet.clear();
            return false;
        }
        uint256_t hash = LLC::SK256(vchRet.begin(), vchRet.end()-4);
        if (memcmp(&hash, &vchRet.end()[-4], 4) != 0)
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

    /** Default Constructor. **/
    CBase58Data::CBase58Data()
    {
        nVersion = 0;
        vchData.clear();
    }

    /** Default Destructor. **/
    CBase58Data::~CBase58Data()
    {
        // zero the memory, as it may contain sensitive data
        if (!vchData.empty())
            memset(&vchData[0], 0, vchData.size()); //TODO: Remove all memset, memcpy functions. They are unsafe and prone to buffer overflows
    }


    /* Set arbitrary data into Base58 structure */
    void CBase58Data::SetData(int nVersionIn, const void* pdata, size_t nSize)
    {
        nVersion = nVersionIn;
        vchData.resize(nSize);
        if (!vchData.empty())
            memcpy(&vchData[0], pdata, nSize); //TODO: remove all instances of memcpy
    }


    /* Set arbitrary data into Base58 structure */
    void CBase58Data::SetData(int nVersionIn, const uint8_t *pbegin, const uint8_t *pend)
    {
        SetData(nVersionIn, (void*)pbegin, pend - pbegin);
    }


    /* Set arbitrary data from c-style string */
    bool CBase58Data::SetString(const char* psz)
    {
        std::vector<unsigned char> vchTemp;
        DecodeBase58Check(psz, vchTemp);
        if (vchTemp.empty())
        {
            vchData.clear();
            nVersion = 0;
            return false;
        }
        nVersion = vchTemp[0];
        vchData.resize(vchTemp.size() - 1);
        if (!vchData.empty())
            //std::copy(vchData.begin(), vchData.end(), vchTemp.begin() + 1);
            memcpy(&vchData[0], &vchTemp[1], vchData.size()); //TODO: remove all instances of memcpy for a safer alternative

        memset(&vchTemp[0], 0, vchTemp.size());
        return true;
    }


    /* Set arbitrary data from a std::string */
    bool CBase58Data::SetString(const std::string& str)
    {
        return SetString(str.c_str());
    }


    /* Set arbitrary data from a std::string */
    std::string CBase58Data::ToString() const
    {
        std::vector<unsigned char> vch(1, nVersion);
        vch.insert(vch.end(), vchData.begin(), vchData.end());
        return EncodeBase58Check(vch);
    }


    /* Compare two Base58 objects */
    int CBase58Data::CompareTo(const CBase58Data& b58) const
    {
        if (nVersion < b58.nVersion) return -1;
        if (nVersion > b58.nVersion) return  1;
        if (vchData < b58.vchData)   return -1;
        if (vchData > b58.vchData)   return  1;
        return 0;
    }

        //operator overloads
    bool CBase58Data::operator==(const CBase58Data& b58) const
    {
        return CompareTo(b58) == 0;
    }

    bool CBase58Data::operator<=(const CBase58Data& b58) const
    {
        return CompareTo(b58) <= 0;
    }

    bool CBase58Data::operator>=(const CBase58Data& b58) const
    {
        return CompareTo(b58) >= 0;
    }

    bool CBase58Data::operator< (const CBase58Data& b58) const
    {
        return CompareTo(b58) <  0;
    }

    bool CBase58Data::operator> (const CBase58Data& b58) const
    {
        return CompareTo(b58) >  0;
    }

}
