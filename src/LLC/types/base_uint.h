/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_TYPES_BASE_UINT_H
#define NEXUS_LLC_TYPES_BASE_UINT_H

#include <cstdint>
#include <string>
#include <vector>


/** Base class without constructors for uint256_t, uint512_t, uint576_t, uint1024_t.
* This makes the compiler let u use it in a union.
*/
template<uint32_t BITS>
class base_uint
{
protected:
    enum { WIDTH=BITS/32 };
    uint32_t pn[WIDTH];
public:

    base_uint();
    ~base_uint();
    base_uint(const base_uint& b);
    base_uint(uint64_t b);

    explicit base_uint(const std::string& str);
    explicit base_uint(const std::vector<uint8_t>& vch);

    base_uint& operator=(const base_uint& b);
    base_uint& operator=(uint64_t b);

    bool operator!() const;

    const base_uint operator~() const;

    const base_uint operator-() const;




    base_uint& operator^=(const base_uint& b);

    base_uint& operator^(const base_uint &b);

    base_uint& operator^=(uint64_t b);

    base_uint& operator|=(const base_uint& b);

    base_uint& operator|=(uint64_t b);

    base_uint& operator|(const base_uint &b);

    base_uint& operator&=(const base_uint& b);

    base_uint& operator&(const base_uint &b);

    base_uint& operator<<=(uint32_t shift);

    base_uint& operator<<(uint32_t shift) const;

    base_uint& operator>>=(uint32_t shift);

    base_uint& operator>>(uint32_t shift) const;

    base_uint& operator+=(const base_uint& b);

    base_uint& operator+=(uint64_t b64);

    base_uint& operator+(const base_uint& rhs) const;

    base_uint& operator-=(const base_uint& b);

    base_uint& operator-=(uint64_t b64);

    base_uint& operator-(const base_uint& rhs) const;

    base_uint& operator++();

    const base_uint operator++(int);

    base_uint& operator--();

    const base_uint operator--(int);

    bool operator<(const base_uint& rhs) const;

    bool operator<=(const base_uint& rhs) const;

    bool operator>(const base_uint& rhs) const;

    bool operator>=(const base_uint& rhs) const;

    bool operator==(const base_uint& rhs) const;

    bool operator==(uint64_t rhs) const;

    bool operator!=(const base_uint& rhs) const;

    bool operator!=(uint64_t rhs) const;



    std::string GetHex() const;

    void SetHex(const char* psz);

    void SetHex(const std::string& str);

    /** Converts the corresponding uint32_teger into bytes.
        Used for serializing in Miner LLP **/
    const std::vector<uint8_t> GetBytes() const;


    /** Creates an uint32_teger from bytes.
        Used for de-serializing in Miner LLP **/
    void SetBytes(const std::vector<uint8_t> DATA);


    /* Computes the count of the highest order bit set. */
    uint32_t BitCount() const;

    std::string ToString() const;

    uint8_t* begin();

    uint8_t* end();

    uint32_t get(uint32_t n);

    void set(const std::vector<uint32_t>& data);

    uint32_t size();

    uint64_t Get64(uint32_t n=0) const;

    uint32_t GetSerializeSize(int nSerType, int nVersion) const
    {
        return sizeof(pn);
    }

    template<typename Stream>
    void Serialize(Stream& s, int nSerType, int nVersion) const
    {
        s.write((char*)pn, sizeof(pn));
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nSerType, int nVersion)
    {
        s.read((char*)pn, sizeof(pn));
    }

    uint32_t high_bits(uint32_t mask);


    friend class uint1024_t;
    friend class uint576_t;
    friend class uint512_t;
    friend class uint256_t;

};

#endif
