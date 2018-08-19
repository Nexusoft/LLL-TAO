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


/** EncodeBase58
 *
 *	Encode into base58 returning a std::string
 *
 *  @param[in] pbegin The begin iterator
 *  @param[in] pend The end iterator
 *
 *	@return Base58 encoded string.
 *
 **/
std::string EncodeBase58(const uint8_t* pbegin, const uint8_t* pend);


/** EncodeBase58
 *
 *	Encode into base58 returning a std::string
 *
 *  @param[in] vch Vector byte char of data
 *
 *	@return Base58 encoded string.
 *
 **/
std::string EncodeBase58(const std::vector<uint8_t>& vch);


/** DecodeBase58
 *
 *	Encode into base58 returning a std::string
 *
 *  @param[in] psz The input string (c style)
 *  @param[out] vchRet The vector char to return
 *
 *	@return true if decoded successfully
 *
 **/
bool DecodeBase58(const char* psz, std::vector<uint8_t>& vchRet);


/** DecodeBase58
 *
 *	Decode base58 string into byte vector
 *
 *  @param[in] str Input string
 *  @param[out] vchRet The byte vector return value
 *
 *	@return true if successful
 *
 **/
bool DecodeBase58(const std::string& str, std::vector<uint8_t>& vchRet);


/** EncodeBase58Check
 *
 *	Encode into base58 including a checksum
 *
 *  @param[in] vchIn The vector char to encode
 *
 *	@return Base58 encoded string with checksum.
 *
 **/
std::string EncodeBase58Check(const std::vector<uint8_t>& vchIn);


/** DecodeBase58Check
 *
 *	Decode into base58 inlucding a checksum
 *
 *  @param[in] psz The c-style input string
 *  @param[out] vchRet The byte vector of return data
 *
 *	@return True if decoding was successful
 *
 **/
bool DecodeBase58Check(const char* psz, std::vector<uint8_t>& vchRet);


/** DecodeBase58Check
 *
 *	Decode into base58 inlucding a checksum
 *
 *  @param[in] str The input string (STL)
 *  @param[out] vchRet The byte vector of return data
 *
 *	@return True if decoding was successful
 *
 **/
bool DecodeBase58Check(const std::string& str, std::vector<uint8_t>& vchRet);


/** Base class for all base58-encoded data */
class CBase58Data
{
protected:

    /** The version byte. **/
    uint8_t nVersion;


    /** The encoded data. **/
    std::vector<uint8_t> vchData;


    /** Default Constructor. **/
    CBase58Data()
    {
        nVersion = 0;
        vchData.clear();
    }


    /** Default Destructor. **/
    ~CBase58Data()
    {
        // zero the memory, as it may contain sensitive data
        if (!vchData.empty())
            memset(&vchData[0], 0, vchData.size()); //TODO: Remove all memset, memcpy functions. They are unsafe and prone to buffer overflows
    }


    /** SetData
     *
     *  Set arbitrary data into Base58 structure
     *
     *  @param[in] nVersionIn The input version to set.
     *  @param[in] pdata The data to set
     *  @param[in] nSize The size of input data set.
     *
     **/
    void SetData(int nVersionIn, const void* pdata, size_t nSize);


    /** SetData
     *
     *  Set arbitrary data into Base58 structure
     *
     *  @param[in] nVersionIn The input version to set.
     *  @param[in] pbegin The begin pointer iterator
     *  @param[in] pend The end pointer iterator
     *
     **/
    void SetData(int nVersionIn, const uint8_t *pbegin, const uint8_t *pend);

public:


    /** SetString
     *
     *  Set arbitrary data from c-style string
     *
     *  @param[in] psz The c-style input string
     *
     *  @return true if string set successfully
     *
     **/
    bool SetString(const char* psz);


    /** SetString
     *
     *  Set arbitrary data from a std::string
     *
     *  @param[in] str The input string to set data
     *
     *  @return true if string set successfully
     *
     **/
    bool SetString(const std::string& str);


    /** SetString
     *
     *  Set arbitrary data from a std::string
     *
     *  @param[in] str The input string to set data
     *
     *  @return true if string set successfully
     *
     **/
    std::string ToString() const;


    /** CompareTo
     *
     *  Compare two Base58 objects
     *
     *  @param[in] b58 The input Base58 object
     *
     *  @return int referencing how they compare
     *
     **/
    int CompareTo(const CBase58Data& b58) const;


    //operator overloads
    bool operator==(const CBase58Data& b58) const { return CompareTo(b58) == 0; }
    bool operator<=(const CBase58Data& b58) const { return CompareTo(b58) <= 0; }
    bool operator>=(const CBase58Data& b58) const { return CompareTo(b58) >= 0; }
    bool operator< (const CBase58Data& b58) const { return CompareTo(b58) <  0; }
    bool operator> (const CBase58Data& b58) const { return CompareTo(b58) >  0; }
};

#endif
