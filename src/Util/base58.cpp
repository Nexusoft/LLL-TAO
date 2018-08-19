/*******************************************************************************************

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

[Learn and Create] Viz. http://www.opensource.org/licenses/mit-license.php

*******************************************************************************************/

#ifndef NEXUS_UTIL_INCLUDE_BASE58_H
#define NEXUS_UTIL_INCLUDE_BASE58_H

#include <string>
#include <vector>
#include "../util/bignum.h"
#include "key.h"

    static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    // Encode a byte sequence as a base58-encoded string
    inline std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
    {
        CAutoBN_CTX pctx;
        CBigNum bn58 = 58;
        CBigNum bn0 = 0;

        // Convert big endian data to little endian
        // Extra zero at the end make sure bignum will interpret as a positive number
        std::vector<unsigned char> vchTmp(pend-pbegin+1, 0);
        reverse_copy(pbegin, pend, vchTmp.begin());

        // Convert little endian data to bignum
        CBigNum bn(vchTmp);

        // Convert bignum to std::string
        std::string str;
        // Expected size increase from base58 conversion is approximately 137%
        // use 138% to be safe
        str.reserve((pend - pbegin) * 138 / 100 + 1);
        CBigNum dv;
        CBigNum rem;
        while (bn > bn0)
        {
            if (!BN_div(dv.getBN(), rem.getBN(), bn.getBN(), bn58.getBN(), pctx))
                throw bignum_error("EncodeBase58 : BN_div failed");
            bn = dv;
            unsigned int c = rem.getulong();
            str += pszBase58[c];
        }

        // Leading zeroes encoded as base58 zeros
        for (const unsigned char* p = pbegin; p < pend && *p == 0; p++)
            str += pszBase58[0];

        // Convert little endian std::string to big endian
        reverse(str.begin(), str.end());
        return str;
    }

    // Encode a byte vector as a base58-encoded string
    inline std::string EncodeBase58(const std::vector<unsigned char>& vch)
    {
        return EncodeBase58(&vch[0], &vch[0] + vch.size());
    }

    // Decode a base58-encoded string psz into byte vector vchRet
    // returns true if decoding is succesful
    inline bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet)
    {
        CAutoBN_CTX pctx;
        vchRet.clear();
        CBigNum bn58 = 58;
        CBigNum bn = 0;
        CBigNum bnChar;
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
                throw bignum_error("DecodeBase58 : BN_mul failed");
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
        reverse_copy(vchTmp.begin(), vchTmp.end(), vchRet.end() - vchTmp.size());
        return true;
    }

    // Decode a base58-encoded string str into byte vector vchRet
    // returns true if decoding is succesful
    inline bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet)
    {
        return DecodeBase58(str.c_str(), vchRet);
    }




    // Encode a byte vector to a base58-encoded string, including checksum
    inline std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn)
    {
        // add 4-byte hash check to the end
        std::vector<unsigned char> vch(vchIn);
        uint256 hash = SK256(vch.begin(), vch.end());
        vch.insert(vch.end(), (unsigned char*)&hash, (unsigned char*)&hash + 4);
        return EncodeBase58(vch);
    }

    // Decode a base58-encoded string psz that includes a checksum, into byte vector vchRet
    // returns true if decoding is succesful
    inline bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet)
    {
        if (!DecodeBase58(psz, vchRet))
            return false;
        if (vchRet.size() < 4)
        {
            vchRet.clear();
            return false;
        }
        uint256 hash = SK256(vchRet.begin(), vchRet.end()-4);
        if (memcmp(&hash, &vchRet.end()[-4], 4) != 0)
        {
            vchRet.clear();
            return false;
        }
        vchRet.resize(vchRet.size()-4);
        return true;
    }

    // Decode a base58-encoded string str that includes a checksum, into byte vector vchRet
    // returns true if decoding is succesful
    inline bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet)
    {
        return DecodeBase58Check(str.c_str(), vchRet);
    }





    /** Base class for all base58-encoded data */
    class CBase58Data
    {
    protected:
        // the version byte
        unsigned char nVersion;

        // the actually encoded data
        std::vector<unsigned char> vchData;

        CBase58Data()
        {
            nVersion = 0;
            vchData.clear();
        }

        ~CBase58Data()
        {
            // zero the memory, as it may contain sensitive data
            if (!vchData.empty())
                memset(&vchData[0], 0, vchData.size());
        }

        void SetData(int nVersionIn, const void* pdata, size_t nSize)
        {
            nVersion = nVersionIn;
            vchData.resize(nSize);
            if (!vchData.empty())
                memcpy(&vchData[0], pdata, nSize);
        }

        void SetData(int nVersionIn, const unsigned char *pbegin, const unsigned char *pend)
        {
            SetData(nVersionIn, (void*)pbegin, pend - pbegin);
        }

    public:
        bool SetString(const char* psz)
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
                memcpy(&vchData[0], &vchTemp[1], vchData.size());
            memset(&vchTemp[0], 0, vchTemp.size());
            return true;
        }

        bool SetString(const std::string& str)
        {
            return SetString(str.c_str());
        }

        std::string ToString() const
        {
            std::vector<unsigned char> vch(1, nVersion);
            vch.insert(vch.end(), vchData.begin(), vchData.end());
            return EncodeBase58Check(vch);
        }

        int CompareTo(const CBase58Data& b58) const
        {
            if (nVersion < b58.nVersion) return -1;
            if (nVersion > b58.nVersion) return  1;
            if (vchData < b58.vchData)   return -1;
            if (vchData > b58.vchData)   return  1;
            return 0;
        }

        bool operator==(const CBase58Data& b58) const { return CompareTo(b58) == 0; }
        bool operator<=(const CBase58Data& b58) const { return CompareTo(b58) <= 0; }
        bool operator>=(const CBase58Data& b58) const { return CompareTo(b58) >= 0; }
        bool operator< (const CBase58Data& b58) const { return CompareTo(b58) <  0; }
        bool operator> (const CBase58Data& b58) const { return CompareTo(b58) >  0; }
    };
}

#endif
