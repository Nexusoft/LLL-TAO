/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_BASE58_H
#define NEXUS_UTIL_INCLUDE_BASE58_H

#include <string>
#include <vector>

static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// Encode a byte sequence as a base58-encoded string
std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend);


// Encode a byte vector as a base58-encoded string
std::string EncodeBase58(const std::vector<unsigned char>& vch);


// Decode a base58-encoded string psz into byte vector vchRet
// returns true if decoding is succesful
bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet);


// Decode a base58-encoded string str into byte vector vchRet
// returns true if decoding is succesful
bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet);


// Encode a byte vector to a base58-encoded string, including checksum
std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn);


// Decode a base58-encoded string psz that includes a checksum, into byte vector vchRet
// returns true if decoding is succesful
bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet);


// Decode a base58-encoded string str that includes a checksum, into byte vector vchRet
// returns true if decoding is succesful
bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet);


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

    void SetData(int nVersionIn, const void* pdata, size_t nSize);

    void SetData(int nVersionIn, const unsigned char *pbegin, const unsigned char *pend);

public:
    bool SetString(const char* psz);

    bool SetString(const std::string& str);

    std::string ToString() const;

    int CompareTo(const CBase58Data& b58) const;

    bool operator==(const CBase58Data& b58) const { return CompareTo(b58) == 0; }
    bool operator<=(const CBase58Data& b58) const { return CompareTo(b58) <= 0; }
    bool operator>=(const CBase58Data& b58) const { return CompareTo(b58) >= 0; }
    bool operator< (const CBase58Data& b58) const { return CompareTo(b58) <  0; }
    bool operator> (const CBase58Data& b58) const { return CompareTo(b58) >  0; }
};

#endif
