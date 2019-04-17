/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <LLC/types/base_uint.h>

namespace
{
    uint8_t phexdigit[256] =
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
        0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0
    };
}


template<uint32_t BITS>
base_uint<BITS>::base_uint()
{
    for(uint8_t i = 0; i < WIDTH; ++i)
        pn[i] = 0;
}


template<uint32_t BITS>
base_uint<BITS>::~base_uint()
{
}


template<uint32_t BITS>
base_uint<BITS>::base_uint(const base_uint<BITS>& b)
{
    for(uint8_t i = 0; i < WIDTH; ++i)
        pn[i] = b.pn[i];
}

template<uint32_t BITS>
base_uint<BITS>::base_uint(uint64_t b)
{
    pn[0] = (uint32_t)b;
    pn[1] = (uint32_t)(b >> 32);

    for (uint8_t i = 2; i < WIDTH; ++i)
        pn[i] = 0;
}


template<uint32_t BITS>
base_uint<BITS>::base_uint(const std::string& str)
{
    SetHex(str);
}


template<uint32_t BITS>
base_uint<BITS>::base_uint(const std::vector<uint8_t>& vch)
{
    if (vch.size() == sizeof(pn))
        std::copy(&vch[0], &vch[0] + sizeof(pn), (uint8_t *)pn);
    else
        *this = 0;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator=(const base_uint<BITS>& b)
{
    for (uint8_t i = 0; i < WIDTH; ++i)
        pn[i] = b.pn[i];

    return *this;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator=(uint64_t b)
{
    pn[0] = (uint32_t)b;
    pn[1] = (uint32_t)(b >> 32);

    for (uint8_t i = 2; i < WIDTH; ++i)
        pn[i] = 0;

    return *this;
}


template<uint32_t BITS>
bool base_uint<BITS>::operator!() const
{
    for (uint8_t i = 0; i < WIDTH; ++i)
    {
        if (pn[i] != 0)
            return false;
    }

    return true;
}


template<uint32_t BITS>
const base_uint<BITS> base_uint<BITS>::operator~() const
{
    base_uint<BITS> ret;

    for (uint8_t i = 0; i < WIDTH; ++i)
        ret.pn[i] = ~pn[i];

    return ret;
}


template<uint32_t BITS>
const base_uint<BITS> base_uint<BITS>::operator-() const
{
    base_uint<BITS> ret;

    for (uint8_t i = 0; i < WIDTH; ++i)
        ret.pn[i] = ~pn[i];

    ret++;

    return ret;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator^=(const base_uint<BITS>& b)
{
    for (uint8_t i = 0; i < WIDTH; ++i)
        pn[i] ^= b.pn[i];
    return *this;
}


template<uint32_t BITS>
base_uint<BITS> &base_uint<BITS>::operator^(const base_uint<BITS> &rhs)
{
    return base_uint<BITS>(*this) ^= rhs;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator^=(uint64_t b)
{
    pn[0] ^= (uint32_t)b;
    pn[1] ^= (uint32_t)(b >> 32);
    return *this;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator&=(const base_uint<BITS>& b)
{
    for (uint8_t i = 0; i < WIDTH; ++i)
        pn[i] &= b.pn[i];
    return *this;
}


template<uint32_t BITS>
base_uint<BITS> &base_uint<BITS>::operator&(const base_uint<BITS> &rhs)
{
    return base_uint<BITS>(*this) &= rhs;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator|=(const base_uint<BITS>& b)
{
    for (uint8_t i = 0; i < WIDTH; ++i)
        pn[i] |= b.pn[i];
    return *this;
}



template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator|=(uint64_t b)
{
    pn[0] |= (uint32_t)b;
    pn[1] |= (uint32_t)(b >> 32);
    return *this;
}

template<uint32_t BITS>
base_uint<BITS> &base_uint<BITS>::operator|(const base_uint<BITS> &rhs)
{
    return base_uint<BITS>(*this) |= rhs;
}




template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator<<=(uint32_t shift)
{
    base_uint<BITS> a(*this);
    for (int32_t i = 0; i < WIDTH; ++i)
        pn[i] = 0;

    int32_t k = shift / 32;
    shift = shift % 32;
    for (int32_t i = 0; i < WIDTH; ++i)
    {
        if (i+k+1 < WIDTH && shift != 0)
            pn[i+k+1] |= (a.pn[i] >> (32-shift));
        if (i+k < WIDTH)
            pn[i+k] |= (a.pn[i] << shift);
    }
    return *this;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator<<(uint32_t shift) const
{
    return base_uint<BITS>(*this) <<= shift;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator>>=(uint32_t shift)
{
    base_uint<BITS> a(*this);

    for (int32_t i = 0; i < WIDTH; ++i)
        pn[i] = 0;

    int32_t k = shift / 32;
    shift = shift % 32;
    for (int32_t i = 0; i < WIDTH; ++i)
    {
        if (i-k-1 >= 0 && shift != 0)
            pn[i-k-1] |= (a.pn[i] << (32-shift));
        if (i-k >= 0)
            pn[i-k] |= (a.pn[i] >> shift);
    }
    return *this;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator>>(uint32_t shift) const
{
    return base_uint<BITS>(*this) >>= shift;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator+=(const base_uint<BITS>& b)
{
    uint64_t carry = 0;
    for (uint8_t i = 0; i < WIDTH; ++i)
    {
        uint64_t n = carry + pn[i] + b.pn[i];
        pn[i] = n & 0xffffffff;
        carry = n >> 32;
    }
    return *this;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator+=(uint64_t b64)
{
    base_uint<BITS> b;
    b = b64;
    *this += b;
    return *this;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator+(const base_uint<BITS>& rhs) const
{
    return base_uint<BITS>(*this) += rhs;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator-=(const base_uint<BITS>& b)
{
    *this += -b;
    return *this;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator-=(uint64_t b64)
{
    base_uint<BITS> b;
    b = b64;
    *this += -b;
    return *this;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator-(const base_uint<BITS>& rhs) const
{
    return base_uint<BITS>(*this) -= rhs;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator++()
{
    // prefix operator
    int i = 0;
    while (++pn[i] == 0 && i < WIDTH-1)
        ++i;
    return *this;
}


template<uint32_t BITS>
const base_uint<BITS> base_uint<BITS>::operator++(int)
{
    // postfix operator
    const base_uint<BITS> ret = *this;
    ++(*this);
    return ret;
}


template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator--()
{
    // prefix operator
    int i = 0;
    while (--pn[i] == -1 && i < WIDTH-1)
        ++i;
    return *this;
}


template<uint32_t BITS>
const base_uint<BITS> base_uint<BITS>::operator--(int)
{
    // postfix operator
    const base_uint<BITS> ret = *this;
    --(*this);
    return ret;
}


template<uint32_t BITS>
bool base_uint<BITS>::operator<(const base_uint<BITS>& rhs) const
{
    for (int8_t i = WIDTH-1; i >= 0; --i)
    {
        if (this->pn[i] < rhs.pn[i])
            return true;
        else if (this->pn[i] > rhs.pn[i])
            return false;
    }
    return false;
}


template<uint32_t BITS>
bool base_uint<BITS>::operator<=(const base_uint<BITS>& rhs) const
{
    for (int8_t i = WIDTH-1; i >= 0; --i)
    {
        if (this->pn[i] < rhs.pn[i])
            return true;
        else if (this->pn[i] > rhs.pn[i])
            return false;
    }
    return true;
}


template<uint32_t BITS>
bool base_uint<BITS>::operator>(const base_uint<BITS>& rhs) const
{
    for (int8_t i = WIDTH-1; i >= 0; --i)
    {
        if (this->pn[i] > rhs.pn[i])
            return true;
        else if (this->pn[i] < rhs.pn[i])
            return false;
    }
    return false;
}


template<uint32_t BITS>
bool base_uint<BITS>::operator>=(const base_uint<BITS>& rhs) const
{
    for (int8_t i = WIDTH-1; i >= 0; --i)
    {
        if (this->pn[i] > rhs.pn[i])
            return true;
        else if (this->pn[i] < rhs.pn[i])
            return false;
    }
    return true;
}


template<uint32_t BITS>
bool base_uint<BITS>::operator==(const base_uint<BITS>& rhs) const
{
    for (uint8_t i = 0; i < WIDTH; ++i)
        if (this->pn[i] != rhs.pn[i])
            return false;

    return true;
}


template<uint32_t BITS>
bool base_uint<BITS>::operator==(uint64_t rhs) const
{
    if (this->pn[0] != (uint32_t)rhs)
        return false;
    if (this->pn[1] != (uint32_t)(rhs >> 32))
        return false;

    for (uint8_t i = 2; i < WIDTH; ++i)
        if (this->pn[i] != 0)
            return false;

    return true;
}


template<uint32_t BITS>
bool base_uint<BITS>::operator!=(const base_uint<BITS>& rhs) const
{
    return (!(*this == rhs));
}


template<uint32_t BITS>
bool base_uint<BITS>::operator!=(uint64_t rhs) const
{
    return (!(*this == rhs));
}

template<uint32_t BITS>
std::string base_uint<BITS>::GetHex() const
{
    char psz[sizeof(pn)*2 + 1];
    for (uint32_t i = 0; i < sizeof(pn); ++i)
        sprintf(psz + i*2, "%02x", ((uint8_t*)pn)[sizeof(pn) - i - 1]);
    return std::string(psz, psz + sizeof(pn)*2);
}


template<uint32_t BITS>
void base_uint<BITS>::SetHex(const char* psz)
{
    for (int i = 0; i < WIDTH; ++i)
        pn[i] = 0;

    while (isspace(*psz))
        ++psz;

    if (psz[0] == '0' && tolower(psz[1]) == 'x')
        psz += 2;

    const char* pbegin = psz;
    while (phexdigit[(uint8_t)*psz] || *psz == '0')
        ++psz;
    --psz;
    uint8_t* p1 = (uint8_t*)pn;
    uint8_t* pend = p1 + WIDTH * 4;
    while (psz >= pbegin && p1 < pend)
    {
        *p1 = phexdigit[(uint8_t)*psz--];
        if (psz >= pbegin)
        {
            uint32_t tmp = (phexdigit[(uint8_t)*psz--] << 4);
            *p1 = *p1 | static_cast<uint8_t>(tmp);
            ++p1;
        }
    }
}


template<uint32_t BITS>
void base_uint<BITS>::SetHex(const std::string& str)
{
    SetHex(str.c_str());
}


/** Converts the corresponding uint32_teger into bytes.
    Used for serializing in Miner LLP **/
template<uint32_t BITS>
const std::vector<uint8_t> base_uint<BITS>::GetBytes() const
{
    std::vector<uint8_t> DATA;

    for(int index = 0; index < WIDTH; ++index)
    {
        std::vector<uint8_t> BYTES(4, 0);
        BYTES[0] = static_cast<uint8_t>(pn[index] >> 24);
        BYTES[1] = static_cast<uint8_t>(pn[index] >> 16);
        BYTES[2] = static_cast<uint8_t>(pn[index] >> 8);
        BYTES[3] = static_cast<uint8_t>(pn[index]);

        DATA.insert(DATA.end(), BYTES.begin(), BYTES.end());
    }

    return DATA;
}



/** Creates an uint32_teger from bytes.
    Used for de-serializing in Miner LLP **/
template<uint32_t BITS>
void base_uint<BITS>::SetBytes(const std::vector<uint8_t> DATA)
{
    for(int index = 0; index < WIDTH; ++index)
    {
        std::vector<uint8_t> BYTES(DATA.begin() + (index * 4), DATA.begin() + (index * 4) + 4);
        pn[index] = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + (BYTES[3] );
    }
}


/* Computes the count of the highest order bit set. */
template<uint32_t BITS>
uint32_t base_uint<BITS>::BitCount() const
{
    uint32_t i = WIDTH << 5;
    for(; i > 0; --i)
    {
        if(pn[i >> 5] & (1 << (i & 31)))
            break;
    }

    /* Any number will have at least 1 bit. */
    return i + 1;
}


template<uint32_t BITS>
std::string base_uint<BITS>::ToString() const
{
    return (GetHex());
}


template<uint32_t BITS>
uint8_t* base_uint<BITS>::begin()
{
    return (uint8_t*)&pn[0];
}


template<uint32_t BITS>
uint8_t* base_uint<BITS>::end()
{
    return (uint8_t*)&pn[WIDTH];
}


template<uint32_t BITS>
uint32_t base_uint<BITS>::get(uint32_t n)
{
    return pn[n];
}


template<uint32_t BITS>
void base_uint<BITS>::set(const std::vector<uint32_t>& data)
{
    for(int i = 0; i < WIDTH; ++i)
        pn[i] = data.size();
}


template<uint32_t BITS>
uint32_t base_uint<BITS>::size()
{
    return sizeof(pn);
}


template<uint32_t BITS>
uint64_t base_uint<BITS>::Get64(uint32_t n) const
{
    return pn[2*n] | (uint64_t)pn[2*n+1] << 32;
}

template<uint32_t BITS>
uint32_t base_uint<BITS>::high_bits(uint32_t mask)
{
    return pn[WIDTH-1] & mask;
}


/* Explicity instantiate all template instances needed for compiler. */
template class base_uint<256>;
template class base_uint<512>;
template class base_uint<576>;
template class base_uint<1024>;
