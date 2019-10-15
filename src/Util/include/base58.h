/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_INCLUDE_BASE58_H
#define NEXUS_UTIL_INCLUDE_BASE58_H

#include <string>
#include <vector>

namespace encoding
{

    /** Base class for all base58-encoded data */
    class CBase58Data
    {
    protected:

        /** The version byte. **/
        uint8_t nVersion;


        /** The encoded data. **/
        std::vector<uint8_t> vchData;


        /** SetData
         *
         *  Set arbitrary data into Base58 structure
         *
         *  @param[in] nVersionIn The input version to set.
         *  @param[in] pdata The data to set
         *  @param[in] nSize The size of input data set.
         *
         **/
        void SetData(const uint8_t nVersionIn, const void* pdata, const size_t nSize);


        /** SetData
         *
         *  Set arbitrary data into Base58 structure
         *
         *  @param[in] nVersionIn The input version to set.
         *  @param[in] pbegin The begin pointer iterator
         *  @param[in] pend The end pointer iterator
         *
         **/
        void SetData(const uint8_t nVersionIn, const uint8_t *pbegin, const uint8_t *pend);

    public:


        /** Default Constructor. **/
        CBase58Data();


        /** Copy Constructor. **/
        CBase58Data(const CBase58Data& data);


        /** Move Constructor. **/
        CBase58Data(CBase58Data&& data) noexcept;


        /** Copy assignment. **/
        CBase58Data& operator=(const CBase58Data& data);


        /** Move assignment. **/
        CBase58Data& operator=(CBase58Data&& data) noexcept;


        /** Default Destructor. **/
        virtual ~CBase58Data();


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


        /* operator overloads */
        bool operator==(const CBase58Data& b58) const;
        bool operator<=(const CBase58Data& b58) const;
        bool operator>=(const CBase58Data& b58) const;
        bool operator< (const CBase58Data& b58) const;
        bool operator> (const CBase58Data& b58) const;
    };

}
#endif
