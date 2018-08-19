/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

            (c) Copyright The Nexus Developers 2014 - 2017

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_UINT1024_H
#define NEXUS_UINT1024_H

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <stdint.h>

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

    bool operator!() const
    {
        for (int i = 0; i < WIDTH; i++)
            if (pn[i] != 0)
                return false;
        return true;
    }

    const base_uint operator~() const
    {
        base_uint ret;
        for (int i = 0; i < WIDTH; i++)
            ret.pn[i] = ~pn[i];
        return ret;
    }

    const base_uint operator-() const
    {
        base_uint ret;
        for (int i = 0; i < WIDTH; i++)
            ret.pn[i] = ~pn[i];
        ret++;
        return ret;
    }


    base_uint& operator=(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    base_uint& operator^=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] ^= b.pn[i];
        return *this;
    }

    base_uint& operator&=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] &= b.pn[i];
        return *this;
    }

    base_uint& operator|=(const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] |= b.pn[i];
        return *this;
    }

    base_uint& operator^=(uint64_t b)
    {
        pn[0] ^= (uint32_t)b;
        pn[1] ^= (uint32_t)(b >> 32);
        return *this;
    }

    base_uint& operator|=(uint64_t b)
    {
        pn[0] |= (uint32_t)b;
        pn[1] |= (uint32_t)(b >> 32);
        return *this;
    }

    base_uint& operator<<=(uint32_t shift)
    {
        base_uint a(*this);
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
        int k = shift / 32;
        shift = shift % 32;
        for (int i = 0; i < WIDTH; i++)
        {
            if (i+k+1 < WIDTH && shift != 0)
                pn[i+k+1] |= (a.pn[i] >> (32-shift));
            if (i+k < WIDTH)
                pn[i+k] |= (a.pn[i] << shift);
        }
        return *this;
    }

    base_uint& operator>>=(uint32_t shift)
    {
        base_uint a(*this);
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
        int k = shift / 32;
        shift = shift % 32;
        for (int i = 0; i < WIDTH; i++)
        {
            if (i-k-1 >= 0 && shift != 0)
                pn[i-k-1] |= (a.pn[i] << (32-shift));
            if (i-k >= 0)
                pn[i-k] |= (a.pn[i] >> shift);
        }
        return *this;
    }

    base_uint& operator+=(const base_uint& b)
    {
        uint64_t carry = 0;
        for (int i = 0; i < WIDTH; i++)
        {
            uint64_t n = carry + pn[i] + b.pn[i];
            pn[i] = n & 0xffffffff;
            carry = n >> 32;
        }
        return *this;
    }

    base_uint& operator-=(const base_uint& b)
    {
        *this += -b;
        return *this;
    }

    base_uint& operator+=(uint64_t b64)
    {
        base_uint b;
        b = b64;
        *this += b;
        return *this;
    }

    base_uint& operator-=(uint64_t b64)
    {
        base_uint b;
        b = b64;
        *this += -b;
        return *this;
    }


    base_uint& operator++()
    {
        // prefix operator
        int i = 0;
        while (++pn[i] == 0 && i < WIDTH-1)
            i++;
        return *this;
    }

    const base_uint operator++(int)
    {
        // postfix operator
        const base_uint ret = *this;
        ++(*this);
        return ret;
    }

    base_uint& operator--()
    {
        // prefix operator
        int i = 0;
        while (--pn[i] == -1 && i < WIDTH-1)
            i++;
        return *this;
    }

    const base_uint operator--(int)
    {
        // postfix operator
        const base_uint ret = *this;
        --(*this);
        return ret;
    }


    friend inline bool operator<(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] < b.pn[i])
                return true;
            else if (a.pn[i] > b.pn[i])
                return false;
        }
        return false;
    }

    friend inline bool operator<=(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] < b.pn[i])
                return true;
            else if (a.pn[i] > b.pn[i])
                return false;
        }
        return true;
    }

    friend inline bool operator>(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] > b.pn[i])
                return true;
            else if (a.pn[i] < b.pn[i])
                return false;
        }
        return false;
    }

    friend inline bool operator>=(const base_uint& a, const base_uint& b)
    {
        for (int i = base_uint::WIDTH-1; i >= 0; i--)
        {
            if (a.pn[i] > b.pn[i])
                return true;
            else if (a.pn[i] < b.pn[i])
                return false;
        }
        return true;
    }

    friend inline bool operator==(const base_uint& a, const base_uint& b)
    {
        for (int i = 0; i < base_uint::WIDTH; i++)
            if (a.pn[i] != b.pn[i])
                return false;
        return true;
    }

    friend inline bool operator==(const base_uint& a, uint64_t b)
    {
        if (a.pn[0] != (uint32_t)b)
            return false;
        if (a.pn[1] != (uint32_t)(b >> 32))
            return false;
        for (int i = 2; i < base_uint::WIDTH; i++)
            if (a.pn[i] != 0)
                return false;
        return true;
    }

    friend inline bool operator!=(const base_uint& a, const base_uint& b)
    {
        return (!(a == b));
    }

    friend inline bool operator!=(const base_uint& a, uint64_t b)
    {
        return (!(a == b));
    }



    std::string GetHex() const
    {
        char psz[sizeof(pn)*2 + 1];
        for (uint32_t i = 0; i < sizeof(pn); i++)
            sprintf(psz + i*2, "%02x", ((uint8_t*)pn)[sizeof(pn) - i - 1]);
        return std::string(psz, psz + sizeof(pn)*2);
    }

    void SetHex(const char* psz)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;

        while (isspace(*psz))
            psz++;

        if (psz[0] == '0' && tolower(psz[1]) == 'x')
            psz += 2;


        static uint8_t phexdigit[256] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0, 0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0 };
        const char* pbegin = psz;
        while (phexdigit[(uint8_t)*psz] || *psz == '0')
            psz++;
        psz--;
        uint8_t* p1 = (uint8_t*)pn;
        uint8_t* pend = p1 + WIDTH * 4;
        while (psz >= pbegin && p1 < pend)
        {
            *p1 = phexdigit[(uint8_t)*psz--];
            if (psz >= pbegin)
            {
                *p1 |= (phexdigit[(uint8_t)*psz--] << 4);
                p1++;
            }
        }
    }

    void SetHex(const std::string& str)
    {
        SetHex(str.c_str());
    }

    /** Converts the corresponding uint32_teger into bytes.
        Used for serializing in Miner LLP **/
    const std::vector<uint8_t> GetBytes()
    {
        std::vector<uint8_t> DATA;

        for(int index = 0; index < WIDTH; index++)
        {
            std::vector<uint8_t> BYTES(4, 0);
            BYTES[0] = (pn[index] >> 24);
            BYTES[1] = (pn[index] >> 16);
            BYTES[2] = (pn[index] >> 8);
            BYTES[3] =  pn[index];

            DATA.insert(DATA.end(), BYTES.begin(), BYTES.end());
        }

        return DATA;
    }

    /** Creates an uint32_teger from bytes.
        Used for de-serializing in Miner LLP **/
    void SetBytes(const std::vector<uint8_t> DATA)
    {
        for(int index = 0; index < WIDTH; index++)
        {
            std::vector<uint8_t> BYTES(DATA.begin() + (index * 4), DATA.begin() + (index * 4) + 4);
            pn[index] = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + (BYTES[3] );
        }
    }

    std::string ToString() const
    {
        return (GetHex());
    }

    uint8_t* begin()
    {
        return (uint8_t*)&pn[0];
    }

    uint8_t* end()
    {
        return (uint8_t*)&pn[WIDTH];
    }

    uint32_t size()
    {
        return sizeof(pn);
    }

    uint64_t Get64(int n=0) const
    {
        return pn[2*n] | (uint64_t)pn[2*n+1] << 32;
    }

//    uint32_t GetSerializeSize(int nType=0, int nVersion=PROTOCOL_VERSION) const
    uint32_t GetSerializeSize(int nType, int nVersion) const
    {
        return sizeof(pn);
    }

    template<typename Stream>
//    void Serialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION) const
    void Serialize(Stream& s, int nType, int nVersion) const
    {
        s.write((char*)pn, sizeof(pn));
    }

    template<typename Stream>
//    void Unserialize(Stream& s, int nType=0, int nVersion=PROTOCOL_VERSION)
    void Unserialize(Stream& s, int nType, int nVersion)
    {
        s.read((char*)pn, sizeof(pn));
    }


    friend class uint256_t;
    friend class uint512_t;
    friend class uint576_t;
    friend class uint1024_t;
};

typedef base_uint<256> base_uint256;
typedef base_uint<512> base_uint512;
typedef base_uint<576> base_uint576;
typedef base_uint<1024> base_uint1024;


/** 256-bit uint32_teger */
class uint256_t : public base_uint256
{
public:
    typedef base_uint256 basetype;

    uint256_t()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256_t(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint256_t& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint256_t(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint256_t& operator=(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    explicit uint256_t(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint256_t(const std::vector<uint8_t>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }
};

inline bool operator==(const uint256_t& a, uint64_t b)                           { return (base_uint256)a == b; }
inline bool operator!=(const uint256_t& a, uint64_t b)                           { return (base_uint256)a != b; }
inline const uint256_t operator<<(const base_uint256& a, uint32_t shift)   { return uint256_t(a) <<= shift; }
inline const uint256_t operator>>(const base_uint256& a, uint32_t shift)   { return uint256_t(a) >>= shift; }
inline const uint256_t operator<<(const uint256_t& a, uint32_t shift)        { return uint256_t(a) <<= shift; }
inline const uint256_t operator>>(const uint256_t& a, uint32_t shift)        { return uint256_t(a) >>= shift; }

inline const uint256_t operator^(const base_uint256& a, const base_uint256& b) { return uint256_t(a) ^= b; }
inline const uint256_t operator&(const base_uint256& a, const base_uint256& b) { return uint256_t(a) &= b; }
inline const uint256_t operator|(const base_uint256& a, const base_uint256& b) { return uint256_t(a) |= b; }
inline const uint256_t operator+(const base_uint256& a, const base_uint256& b) { return uint256_t(a) += b; }
inline const uint256_t operator-(const base_uint256& a, const base_uint256& b) { return uint256_t(a) -= b; }

inline bool operator<(const base_uint256& a, const uint256_t& b)          { return (base_uint256)a <  (base_uint256)b; }
inline bool operator<=(const base_uint256& a, const uint256_t& b)         { return (base_uint256)a <= (base_uint256)b; }
inline bool operator>(const base_uint256& a, const uint256_t& b)          { return (base_uint256)a >  (base_uint256)b; }
inline bool operator>=(const base_uint256& a, const uint256_t& b)         { return (base_uint256)a >= (base_uint256)b; }
inline bool operator==(const base_uint256& a, const uint256_t& b)         { return (base_uint256)a == (base_uint256)b; }
inline bool operator!=(const base_uint256& a, const uint256_t& b)         { return (base_uint256)a != (base_uint256)b; }
inline const uint256_t operator^(const base_uint256& a, const uint256_t& b) { return (base_uint256)a ^  (base_uint256)b; }
inline const uint256_t operator&(const base_uint256& a, const uint256_t& b) { return (base_uint256)a &  (base_uint256)b; }
inline const uint256_t operator|(const base_uint256& a, const uint256_t& b) { return (base_uint256)a |  (base_uint256)b; }
inline const uint256_t operator+(const base_uint256& a, const uint256_t& b) { return (base_uint256)a +  (base_uint256)b; }
inline const uint256_t operator-(const base_uint256& a, const uint256_t& b) { return (base_uint256)a -  (base_uint256)b; }

inline bool operator<(const uint256_t& a, const base_uint256& b)          { return (base_uint256)a <  (base_uint256)b; }
inline bool operator<=(const uint256_t& a, const base_uint256& b)         { return (base_uint256)a <= (base_uint256)b; }
inline bool operator>(const uint256_t& a, const base_uint256& b)          { return (base_uint256)a >  (base_uint256)b; }
inline bool operator>=(const uint256_t& a, const base_uint256& b)         { return (base_uint256)a >= (base_uint256)b; }
inline bool operator==(const uint256_t& a, const base_uint256& b)         { return (base_uint256)a == (base_uint256)b; }
inline bool operator!=(const uint256_t& a, const base_uint256& b)         { return (base_uint256)a != (base_uint256)b; }
inline const uint256_t operator^(const uint256_t& a, const base_uint256& b) { return (base_uint256)a ^  (base_uint256)b; }
inline const uint256_t operator&(const uint256_t& a, const base_uint256& b) { return (base_uint256)a &  (base_uint256)b; }
inline const uint256_t operator|(const uint256_t& a, const base_uint256& b) { return (base_uint256)a |  (base_uint256)b; }
inline const uint256_t operator+(const uint256_t& a, const base_uint256& b) { return (base_uint256)a +  (base_uint256)b; }
inline const uint256_t operator-(const uint256_t& a, const base_uint256& b) { return (base_uint256)a -  (base_uint256)b; }

inline bool operator<(const uint256_t& a, const uint256_t& b)               { return (base_uint256)a <  (base_uint256)b; }
inline bool operator<=(const uint256_t& a, const uint256_t& b)              { return (base_uint256)a <= (base_uint256)b; }
inline bool operator>(const uint256_t& a, const uint256_t& b)               { return (base_uint256)a >  (base_uint256)b; }
inline bool operator>=(const uint256_t& a, const uint256_t& b)              { return (base_uint256)a >= (base_uint256)b; }
inline bool operator==(const uint256_t& a, const uint256_t& b)              { return (base_uint256)a == (base_uint256)b; }
inline bool operator!=(const uint256_t& a, const uint256_t& b)              { return (base_uint256)a != (base_uint256)b; }
inline const uint256_t operator^(const uint256_t& a, const uint256_t& b)      { return (base_uint256)a ^  (base_uint256)b; }
inline const uint256_t operator&(const uint256_t& a, const uint256_t& b)      { return (base_uint256)a &  (base_uint256)b; }
inline const uint256_t operator|(const uint256_t& a, const uint256_t& b)      { return (base_uint256)a |  (base_uint256)b; }
inline const uint256_t operator+(const uint256_t& a, const uint256_t& b)      { return (base_uint256)a +  (base_uint256)b; }
inline const uint256_t operator-(const uint256_t& a, const uint256_t& b)      { return (base_uint256)a -  (base_uint256)b; }


/** 512-bit uint32_teger */
class uint512_t : public base_uint512
{
public:
    typedef base_uint512 basetype;

    uint512_t()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint512_t(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint512_t& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint512_t(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint512_t& operator=(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    explicit uint512_t(const std::vector<uint8_t> vch)
    {
        SetBytes(vch);
    }

    explicit uint512_t(const std::string& str)
    {
        SetHex(str);
    }

    /*
    explicit uint512_t(const std::vector<uint8_t>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }
    */
};

inline bool operator==(const uint512_t& a, uint64_t b)                           { return (base_uint512)a == b; }
inline bool operator!=(const uint512_t& a, uint64_t b)                           { return (base_uint512)a != b; }
inline const uint512_t operator<<(const base_uint512& a, uint32_t shift)   { return uint512_t(a) <<= shift; }
inline const uint512_t operator>>(const base_uint512& a, uint32_t shift)   { return uint512_t(a) >>= shift; }
inline const uint512_t operator<<(const uint512_t& a, uint32_t shift)        { return uint512_t(a) <<= shift; }
inline const uint512_t operator>>(const uint512_t& a, uint32_t shift)        { return uint512_t(a) >>= shift; }

inline const uint512_t operator^(const base_uint512& a, const base_uint512& b) { return uint512_t(a) ^= b; }
inline const uint512_t operator&(const base_uint512& a, const base_uint512& b) { return uint512_t(a) &= b; }
inline const uint512_t operator|(const base_uint512& a, const base_uint512& b) { return uint512_t(a) |= b; }
inline const uint512_t operator+(const base_uint512& a, const base_uint512& b) { return uint512_t(a) += b; }
inline const uint512_t operator-(const base_uint512& a, const base_uint512& b) { return uint512_t(a) -= b; }

inline bool operator<(const base_uint512& a, const uint512_t& b)          { return (base_uint512)a <  (base_uint512)b; }
inline bool operator<=(const base_uint512& a, const uint512_t& b)         { return (base_uint512)a <= (base_uint512)b; }
inline bool operator>(const base_uint512& a, const uint512_t& b)          { return (base_uint512)a >  (base_uint512)b; }
inline bool operator>=(const base_uint512& a, const uint512_t& b)         { return (base_uint512)a >= (base_uint512)b; }
inline bool operator==(const base_uint512& a, const uint512_t& b)         { return (base_uint512)a == (base_uint512)b; }
inline bool operator!=(const base_uint512& a, const uint512_t& b)         { return (base_uint512)a != (base_uint512)b; }
inline const uint512_t operator^(const base_uint512& a, const uint512_t& b) { return (base_uint512)a ^  (base_uint512)b; }
inline const uint512_t operator&(const base_uint512& a, const uint512_t& b) { return (base_uint512)a &  (base_uint512)b; }
inline const uint512_t operator|(const base_uint512& a, const uint512_t& b) { return (base_uint512)a |  (base_uint512)b; }
inline const uint512_t operator+(const base_uint512& a, const uint512_t& b) { return (base_uint512)a +  (base_uint512)b; }
inline const uint512_t operator-(const base_uint512& a, const uint512_t& b) { return (base_uint512)a -  (base_uint512)b; }

inline bool operator<(const uint512_t& a, const base_uint512& b)          { return (base_uint512)a <  (base_uint512)b; }
inline bool operator<=(const uint512_t& a, const base_uint512& b)         { return (base_uint512)a <= (base_uint512)b; }
inline bool operator>(const uint512_t& a, const base_uint512& b)          { return (base_uint512)a >  (base_uint512)b; }
inline bool operator>=(const uint512_t& a, const base_uint512& b)         { return (base_uint512)a >= (base_uint512)b; }
inline bool operator==(const uint512_t& a, const base_uint512& b)         { return (base_uint512)a == (base_uint512)b; }
inline bool operator!=(const uint512_t& a, const base_uint512& b)         { return (base_uint512)a != (base_uint512)b; }
inline const uint512_t operator^(const uint512_t& a, const base_uint512& b) { return (base_uint512)a ^  (base_uint512)b; }
inline const uint512_t operator&(const uint512_t& a, const base_uint512& b) { return (base_uint512)a &  (base_uint512)b; }
inline const uint512_t operator|(const uint512_t& a, const base_uint512& b) { return (base_uint512)a |  (base_uint512)b; }
inline const uint512_t operator+(const uint512_t& a, const base_uint512& b) { return (base_uint512)a +  (base_uint512)b; }
inline const uint512_t operator-(const uint512_t& a, const base_uint512& b) { return (base_uint512)a -  (base_uint512)b; }

inline bool operator<(const uint512_t& a, const uint512_t& b)               { return (base_uint512)a <  (base_uint512)b; }
inline bool operator<=(const uint512_t& a, const uint512_t& b)              { return (base_uint512)a <= (base_uint512)b; }
inline bool operator>(const uint512_t& a, const uint512_t& b)               { return (base_uint512)a >  (base_uint512)b; }
inline bool operator>=(const uint512_t& a, const uint512_t& b)              { return (base_uint512)a >= (base_uint512)b; }
inline bool operator==(const uint512_t& a, const uint512_t& b)              { return (base_uint512)a == (base_uint512)b; }
inline bool operator!=(const uint512_t& a, const uint512_t& b)              { return (base_uint512)a != (base_uint512)b; }
inline const uint512_t operator^(const uint512_t& a, const uint512_t& b)      { return (base_uint512)a ^  (base_uint512)b; }
inline const uint512_t operator&(const uint512_t& a, const uint512_t& b)      { return (base_uint512)a &  (base_uint512)b; }
inline const uint512_t operator|(const uint512_t& a, const uint512_t& b)      { return (base_uint512)a |  (base_uint512)b; }
inline const uint512_t operator+(const uint512_t& a, const uint512_t& b)      { return (base_uint512)a +  (base_uint512)b; }
inline const uint512_t operator-(const uint512_t& a, const uint512_t& b)      { return (base_uint512)a -  (base_uint512)b; }

/** 576-bit uint32_teger */
class uint576_t : public base_uint576
{
public:
    typedef base_uint576 basetype;

    uint576_t()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint576_t(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint576_t& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint576_t(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint576_t& operator=(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    explicit uint576_t(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint576_t(const std::vector<uint8_t>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }
};

inline bool operator==(const uint576_t& a, uint64_t b)                           { return (base_uint576)a == b; }
inline bool operator!=(const uint576_t& a, uint64_t b)                           { return (base_uint576)a != b; }
inline const uint576_t operator<<(const base_uint576& a, uint32_t shift)   { return uint576_t(a) <<= shift; }
inline const uint576_t operator>>(const base_uint576& a, uint32_t shift)   { return uint576_t(a) >>= shift; }
inline const uint576_t operator<<(const uint576_t& a, uint32_t shift)        { return uint576_t(a) <<= shift; }
inline const uint576_t operator>>(const uint576_t& a, uint32_t shift)        { return uint576_t(a) >>= shift; }

inline const uint576_t operator^(const base_uint576& a, const base_uint576& b) { return uint576_t(a) ^= b; }
inline const uint576_t operator&(const base_uint576& a, const base_uint576& b) { return uint576_t(a) &= b; }
inline const uint576_t operator|(const base_uint576& a, const base_uint576& b) { return uint576_t(a) |= b; }
inline const uint576_t operator+(const base_uint576& a, const base_uint576& b) { return uint576_t(a) += b; }
inline const uint576_t operator-(const base_uint576& a, const base_uint576& b) { return uint576_t(a) -= b; }

inline bool operator<(const base_uint576& a, const uint576_t& b)          { return (base_uint576)a <  (base_uint576)b; }
inline bool operator<=(const base_uint576& a, const uint576_t& b)         { return (base_uint576)a <= (base_uint576)b; }
inline bool operator>(const base_uint576& a, const uint576_t& b)          { return (base_uint576)a >  (base_uint576)b; }
inline bool operator>=(const base_uint576& a, const uint576_t& b)         { return (base_uint576)a >= (base_uint576)b; }
inline bool operator==(const base_uint576& a, const uint576_t& b)         { return (base_uint576)a == (base_uint576)b; }
inline bool operator!=(const base_uint576& a, const uint576_t& b)         { return (base_uint576)a != (base_uint576)b; }
inline const uint576_t operator^(const base_uint576& a, const uint576_t& b) { return (base_uint576)a ^  (base_uint576)b; }
inline const uint576_t operator&(const base_uint576& a, const uint576_t& b) { return (base_uint576)a &  (base_uint576)b; }
inline const uint576_t operator|(const base_uint576& a, const uint576_t& b) { return (base_uint576)a |  (base_uint576)b; }
inline const uint576_t operator+(const base_uint576& a, const uint576_t& b) { return (base_uint576)a +  (base_uint576)b; }
inline const uint576_t operator-(const base_uint576& a, const uint576_t& b) { return (base_uint576)a -  (base_uint576)b; }

inline bool operator<(const uint576_t& a, const base_uint576& b)          { return (base_uint576)a <  (base_uint576)b; }
inline bool operator<=(const uint576_t& a, const base_uint576& b)         { return (base_uint576)a <= (base_uint576)b; }
inline bool operator>(const uint576_t& a, const base_uint576& b)          { return (base_uint576)a >  (base_uint576)b; }
inline bool operator>=(const uint576_t& a, const base_uint576& b)         { return (base_uint576)a >= (base_uint576)b; }
inline bool operator==(const uint576_t& a, const base_uint576& b)         { return (base_uint576)a == (base_uint576)b; }
inline bool operator!=(const uint576_t& a, const base_uint576& b)         { return (base_uint576)a != (base_uint576)b; }
inline const uint576_t operator^(const uint576_t& a, const base_uint576& b) { return (base_uint576)a ^  (base_uint576)b; }
inline const uint576_t operator&(const uint576_t& a, const base_uint576& b) { return (base_uint576)a &  (base_uint576)b; }
inline const uint576_t operator|(const uint576_t& a, const base_uint576& b) { return (base_uint576)a |  (base_uint576)b; }
inline const uint576_t operator+(const uint576_t& a, const base_uint576& b) { return (base_uint576)a +  (base_uint576)b; }
inline const uint576_t operator-(const uint576_t& a, const base_uint576& b) { return (base_uint576)a -  (base_uint576)b; }

inline bool operator<(const uint576_t& a, const uint576_t& b)               { return (base_uint576)a <  (base_uint576)b; }
inline bool operator<=(const uint576_t& a, const uint576_t& b)              { return (base_uint576)a <= (base_uint576)b; }
inline bool operator>(const uint576_t& a, const uint576_t& b)               { return (base_uint576)a >  (base_uint576)b; }
inline bool operator>=(const uint576_t& a, const uint576_t& b)              { return (base_uint576)a >= (base_uint576)b; }
inline bool operator==(const uint576_t& a, const uint576_t& b)              { return (base_uint576)a == (base_uint576)b; }
inline bool operator!=(const uint576_t& a, const uint576_t& b)              { return (base_uint576)a != (base_uint576)b; }
inline const uint576_t operator^(const uint576_t& a, const uint576_t& b)      { return (base_uint576)a ^  (base_uint576)b; }
inline const uint576_t operator&(const uint576_t& a, const uint576_t& b)      { return (base_uint576)a &  (base_uint576)b; }
inline const uint576_t operator|(const uint576_t& a, const uint576_t& b)      { return (base_uint576)a |  (base_uint576)b; }
inline const uint576_t operator+(const uint576_t& a, const uint576_t& b)      { return (base_uint576)a +  (base_uint576)b; }
inline const uint576_t operator-(const uint576_t& a, const uint576_t& b)      { return (base_uint576)a -  (base_uint576)b; }



/** 1024-bit uint32_teger */
class uint1024_t : public base_uint1024
{
public:
    typedef base_uint1024 basetype;

    uint1024_t()
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint1024_t(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
    }

    uint1024_t& operator=(const basetype& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] = b.pn[i];
        return *this;
    }

    uint1024_t(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
    }

    uint1024_t& operator=(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; i++)
            pn[i] = 0;
        return *this;
    }

    uint1024_t(uint256_t b)
    {
        for (int i = 0; i < WIDTH; i++)
            if(i < b.WIDTH)
                pn[i] = b.pn[i];
            else
                pn[i] = 0;
    }

    uint1024_t& operator=(uint256_t b)
    {
        for (int i = 0; i < WIDTH; i++)
            if(i < b.WIDTH)
                pn[i] = b.pn[i];
            else
                pn[i] = 0;

        return *this;
    }

    uint1024_t(uint512_t b)
    {
        for (int i = 0; i < WIDTH; i++)
            if(i < b.WIDTH)
                pn[i] = b.pn[i];
            else
                pn[i] = 0;
    }

    uint1024_t& operator=(uint512_t b)
    {
        for (int i = 0; i < WIDTH; i++)
            if(i < b.WIDTH)
                pn[i] = b.pn[i];
            else
                pn[i] = 0;

        return *this;
    }

    /** This method should only be used to retrieve an uint256_t when stored inside an uint1024_t.
        This is necessary for for ambiguous function declaration. */
    uint256_t getuint256() const
    {
        uint256_t b;
        for (int i = 0; i < b.WIDTH; i++)
            b.pn[i] = pn[i];

        return b;
    }

    /** This method should only be used to retrieve an uint512_t when stored inside an uint1024_t.
        This is necessary for the inventory system to function with both a 1024 bit block
        and 512 bit transaction. */
    uint512_t getuint512() const
    {
        uint512_t b;
        for (int i = 0; i < b.WIDTH; i++)
            b.pn[i] = pn[i];

        return b;
    }

    explicit uint1024_t(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint1024_t(const std::vector<uint8_t>& vch)
    {
        if (vch.size() == sizeof(pn))
            memcpy(pn, &vch[0], sizeof(pn));
        else
            *this = 0;
    }
};

inline bool operator==(const uint1024_t& a, uint64_t b)                           { return (base_uint1024)a == b; }
inline bool operator!=(const uint1024_t& a, uint64_t b)                           { return (base_uint1024)a != b; }
inline const uint1024_t operator<<(const base_uint1024& a, uint32_t shift)   { return uint1024_t(a) <<= shift; }
inline const uint1024_t operator>>(const base_uint1024& a, uint32_t shift)   { return uint1024_t(a) >>= shift; }
inline const uint1024_t operator<<(const uint1024_t& a, uint32_t shift)        { return uint1024_t(a) <<= shift; }
inline const uint1024_t operator>>(const uint1024_t& a, uint32_t shift)        { return uint1024_t(a) >>= shift; }

inline const uint1024_t operator^(const base_uint1024& a, const base_uint1024& b) { return uint1024_t(a) ^= b; }
inline const uint1024_t operator&(const base_uint1024& a, const base_uint1024& b) { return uint1024_t(a) &= b; }
inline const uint1024_t operator|(const base_uint1024& a, const base_uint1024& b) { return uint1024_t(a) |= b; }
inline const uint1024_t operator+(const base_uint1024& a, const base_uint1024& b) { return uint1024_t(a) += b; }
inline const uint1024_t operator-(const base_uint1024& a, const base_uint1024& b) { return uint1024_t(a) -= b; }

inline bool operator<(const base_uint1024& a, const uint1024_t& b)          { return (base_uint1024)a <  (base_uint1024)b; }
inline bool operator<=(const base_uint1024& a, const uint1024_t& b)         { return (base_uint1024)a <= (base_uint1024)b; }
inline bool operator>(const base_uint1024& a, const uint1024_t& b)          { return (base_uint1024)a >  (base_uint1024)b; }
inline bool operator>=(const base_uint1024& a, const uint1024_t& b)         { return (base_uint1024)a >= (base_uint1024)b; }
inline bool operator==(const base_uint1024& a, const uint1024_t& b)         { return (base_uint1024)a == (base_uint1024)b; }
inline bool operator!=(const base_uint1024& a, const uint1024_t& b)         { return (base_uint1024)a != (base_uint1024)b; }
inline const uint1024_t operator^(const base_uint1024& a, const uint1024_t& b) { return (base_uint1024)a ^  (base_uint1024)b; }
inline const uint1024_t operator&(const base_uint1024& a, const uint1024_t& b) { return (base_uint1024)a &  (base_uint1024)b; }
inline const uint1024_t operator|(const base_uint1024& a, const uint1024_t& b) { return (base_uint1024)a |  (base_uint1024)b; }
inline const uint1024_t operator+(const base_uint1024& a, const uint1024_t& b) { return (base_uint1024)a +  (base_uint1024)b; }
inline const uint1024_t operator-(const base_uint1024& a, const uint1024_t& b) { return (base_uint1024)a -  (base_uint1024)b; }

inline bool operator<(const uint1024_t& a, const base_uint1024& b)          { return (base_uint1024)a <  (base_uint1024)b; }
inline bool operator<=(const uint1024_t& a, const base_uint1024& b)         { return (base_uint1024)a <= (base_uint1024)b; }
inline bool operator>(const uint1024_t& a, const base_uint1024& b)          { return (base_uint1024)a >  (base_uint1024)b; }
inline bool operator>=(const uint1024_t& a, const base_uint1024& b)         { return (base_uint1024)a >= (base_uint1024)b; }
inline bool operator==(const uint1024_t& a, const base_uint1024& b)         { return (base_uint1024)a == (base_uint1024)b; }
inline bool operator!=(const uint1024_t& a, const base_uint1024& b)         { return (base_uint1024)a != (base_uint1024)b; }
inline const uint1024_t operator^(const uint1024_t& a, const base_uint1024& b) { return (base_uint1024)a ^  (base_uint1024)b; }
inline const uint1024_t operator&(const uint1024_t& a, const base_uint1024& b) { return (base_uint1024)a &  (base_uint1024)b; }
inline const uint1024_t operator|(const uint1024_t& a, const base_uint1024& b) { return (base_uint1024)a |  (base_uint1024)b; }
inline const uint1024_t operator+(const uint1024_t& a, const base_uint1024& b) { return (base_uint1024)a +  (base_uint1024)b; }
inline const uint1024_t operator-(const uint1024_t& a, const base_uint1024& b) { return (base_uint1024)a -  (base_uint1024)b; }

inline bool operator<(const uint1024_t& a, const uint1024_t& b)               { return (base_uint1024)a <  (base_uint1024)b; }
inline bool operator<=(const uint1024_t& a, const uint1024_t& b)              { return (base_uint1024)a <= (base_uint1024)b; }
inline bool operator>(const uint1024_t& a, const uint1024_t& b)               { return (base_uint1024)a >  (base_uint1024)b; }
inline bool operator>=(const uint1024_t& a, const uint1024_t& b)              { return (base_uint1024)a >= (base_uint1024)b; }
inline bool operator==(const uint1024_t& a, const uint1024_t& b)              { return (base_uint1024)a == (base_uint1024)b; }
inline bool operator!=(const uint1024_t& a, const uint1024_t& b)              { return (base_uint1024)a != (base_uint1024)b; }
inline const uint1024_t operator^(const uint1024_t& a, const uint1024_t& b)      { return (base_uint1024)a ^  (base_uint1024)b; }
inline const uint1024_t operator&(const uint1024_t& a, const uint1024_t& b)      { return (base_uint1024)a &  (base_uint1024)b; }
inline const uint1024_t operator|(const uint1024_t& a, const uint1024_t& b)      { return (base_uint1024)a |  (base_uint1024)b; }
inline const uint1024_t operator+(const uint1024_t& a, const uint1024_t& b)      { return (base_uint1024)a +  (base_uint1024)b; }
inline const uint1024_t operator-(const uint1024_t& a, const uint1024_t& b)      { return (base_uint1024)a -  (base_uint1024)b; }


#endif
