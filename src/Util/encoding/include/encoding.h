/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_ENCODING_H
#define NEXUS_UTIL_INCLUDE_ENCODING_H

#include <string>
#include <vector>

namespace encoding
{

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

}
#endif
