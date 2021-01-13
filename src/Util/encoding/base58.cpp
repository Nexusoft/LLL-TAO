/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/bignum.h>
#include <LLC/hash/SK.h>

#include <Util/include/base58.h>
#include <Util/include/encoding.h>
#include <Util/include/memory.h>

#include <openssl/bn.h>
#include <algorithm>
#include <cstring>

namespace encoding
{

    /* Default Constructor. */
    CBase58Data::CBase58Data()
    : nVersion(0)
    , vchData()
    {
    }

    /* Copy Constructor. */
    CBase58Data::CBase58Data(const CBase58Data& data)
    : nVersion (data.nVersion)
    , vchData  (data.vchData)
    {
    }


    /* Move Constructor. */
    CBase58Data::CBase58Data(CBase58Data&& data) noexcept
    : nVersion (std::move(data.nVersion))
    , vchData  (std::move(data.vchData))
    {
    }


    /* Copy assignment. */
    CBase58Data& CBase58Data::operator=(const CBase58Data& data)
    {
        nVersion = data.nVersion;
        vchData  = data.vchData;

        return *this;
    }


    /* Move assignment. */
    CBase58Data& CBase58Data::operator=(CBase58Data&& data) noexcept
    {
        nVersion = std::move(data.nVersion);
        vchData  = std::move(data.vchData);

        return *this;
    }


    /* Default Destructor. */
    CBase58Data::~CBase58Data()
    {
        // zero the memory, as it may contain sensitive data
        if(!vchData.empty())
            memset(&vchData[0], 0, vchData.size());
    }


    /* Set arbitrary data into Base58 structure */
    void CBase58Data::SetData(const uint8_t nVersionIn, const void* pdata, const size_t nSize)
    {
        nVersion = nVersionIn;
        vchData.resize(nSize);
        if(!vchData.empty())
            //memcpy(&vchData[0], pdata, nSize);
            std::copy((uint8_t *)pdata, (uint8_t *)pdata + nSize, &vchData[0]);
    }


    /* Set arbitrary data into Base58 structure */
    void CBase58Data::SetData(const uint8_t nVersionIn, const uint8_t *pbegin, const uint8_t *pend)
    {
        SetData(nVersionIn, (void*)pbegin, pend - pbegin);
    }


    /* Set arbitrary data from c-style string */
    bool CBase58Data::SetString(const char* psz)
    {
        std::vector<uint8_t> vchTemp;
        DecodeBase58Check(psz, vchTemp);
        if(vchTemp.empty())
        {
            vchData.clear();
            nVersion = 0;
            return false;
        }
        nVersion = vchTemp[0];
        vchData.resize(vchTemp.size() - 1);
        if(!vchData.empty())
            //memcpy(&vchData[0], &vchTemp[1], vchData.size());
            std::copy(&vchTemp[1], &vchTemp[1] + vchData.size(), &vchData[0]);
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
        std::vector<uint8_t> vch(1, nVersion);
        vch.insert(vch.end(), vchData.begin(), vchData.end());
        return EncodeBase58Check(vch);
    }


    /* Compare two Base58 objects */
    int CBase58Data::CompareTo(const CBase58Data& b58) const
    {
        if(nVersion < b58.nVersion) return -1;
        if(nVersion > b58.nVersion) return  1;
        if(vchData < b58.vchData)   return -1;
        if(vchData > b58.vchData)   return  1;
        return 0;
    }

    /* operator overload == */
    bool CBase58Data::operator==(const CBase58Data& b58) const
    {
        return CompareTo(b58) == 0;
    }

    /* operator overload <= */
    bool CBase58Data::operator<=(const CBase58Data& b58) const
    {
        return CompareTo(b58) <= 0;
    }

    /* operator overload >= */
    bool CBase58Data::operator>=(const CBase58Data& b58) const
    {
        return CompareTo(b58) >= 0;
    }

    /* operator overload < */
    bool CBase58Data::operator< (const CBase58Data& b58) const
    {
        return CompareTo(b58) <  0;
    }

    /* operator overload > */
    bool CBase58Data::operator> (const CBase58Data& b58) const
    {
        return CompareTo(b58) >  0;
    }

}
