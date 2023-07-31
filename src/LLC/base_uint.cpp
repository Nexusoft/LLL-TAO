/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/
#include <LLC/types/base_uint.h>
#include <limits>
#include <stdexcept>

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


/* Default Constructor. */
template<uint32_t BITS>
base_uint<BITS>::base_uint()
{
    for(uint8_t i = 0; i < WIDTH; ++i)
        pn[i] = 0;
}


/* Copy Constructor. */
template<uint32_t BITS>
base_uint<BITS>::base_uint(const base_uint<BITS>& n)
{
    for(uint8_t i = 0; i < WIDTH; ++i)
        pn[i] = n.pn[i];
}


/** Move Constructor. **/
template<uint32_t BITS>
base_uint<BITS>::base_uint(base_uint&& n) noexcept
{
    for(uint8_t i = 0; i < WIDTH; ++i)
        pn[i] = std::move(n.pn[i]);
}


/** Copy Assignment. **/
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator=(const base_uint& n)
{
    for(uint8_t i = 0; i < WIDTH; ++i)
        pn[i] = n.pn[i];

    return *this;
}


/** Move Assignment. **/
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator=(base_uint&& n) noexcept
{
    for(uint8_t i = 0; i < WIDTH; ++i)
        pn[i] = std::move(n.pn[i]);

    return *this;
}


/* Assignment operator. (64-bit) */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator=(const uint64_t n)
{
    pn[0] = (uint32_t) n;
    pn[1] = (uint32_t)(n >> 32);

    for(uint8_t i = 2; i < WIDTH; ++i)
        pn[i] = 0;

    return *this;
}


/* Destructor. */
template<uint32_t BITS>
base_uint<BITS>::~base_uint()
{
}


/* Constructor. (from string) */
template<uint32_t BITS>
base_uint<BITS>::base_uint(const std::string& str)
{
    SetHex(str);
}


/* Constructor. (from vector) */
template<uint32_t BITS>
base_uint<BITS>::base_uint(const std::vector<uint8_t>& vch)
{
    if(vch.size() == sizeof(pn))
        std::copy(&vch[0], &vch[0] + sizeof(pn), (uint8_t *)pn);
    else
        *this = 0u;
}


/* Constructor. (64-bit) */
template<uint32_t BITS>
base_uint<BITS>::base_uint(uint64_t b)
{
    pn[0] = (uint32_t)b;
    pn[1] = (uint32_t)(b >> 32);

    for(uint8_t i = 2; i < WIDTH; ++i)
        pn[i] = 0;
}


/* Logical NOT unary operator. */
template<uint32_t BITS>
bool base_uint<BITS>::operator!() const
{
    for(uint8_t i = 0; i < WIDTH; ++i)
    {
        if(pn[i] != 0)
            return false;
    }

    return true;
}


/* Bitwise inversion unary operator. */
template<uint32_t BITS>
const base_uint<BITS> base_uint<BITS>::operator~() const
{
    base_uint<BITS> ret;

    for(uint8_t i = 0; i < WIDTH; ++i)
        ret.pn[i] = ~pn[i];

    return ret;
}


/* One's complement (negation) bitwise unary operator */
template<uint32_t BITS>
const base_uint<BITS> base_uint<BITS>::operator-() const
{
    base_uint<BITS> ret;

    for(uint8_t i = 0; i < WIDTH; ++i)
        ret.pn[i] = ~pn[i];

    ++ret;

    return ret;
}


/* XOR assignment operator. */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator^=(const base_uint<BITS>& b)
{
    for(uint8_t i = 0; i < WIDTH; ++i)
        pn[i] ^= b.pn[i];

    return *this;
}


/* XOR assignment operator. (64-bit) */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator^=(uint64_t b)
{
    pn[0] ^= (uint32_t)b;
    pn[1] ^= (uint32_t)(b >> 32);
    return *this;
}



/* OR assignment operator. */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator|=(const base_uint<BITS>& b)
{
    for(uint8_t i = 0; i < WIDTH; ++i)
        pn[i] |= b.pn[i];

    return *this;
}


/* OR assignment operator. (64-bit) */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator|=(uint64_t b)
{
    pn[0] |= (uint32_t)b;
    pn[1] |= (uint32_t)(b >> 32);

    return *this;
}


/* AND assignment operator. */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator&=(const base_uint<BITS>& b)
{
    for(uint8_t i = 0; i < WIDTH; ++i)
        pn[i] &= b.pn[i];

    return *this;
}


/* Left shift assignment operator. */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator<<=(uint32_t shift)
{
    base_uint<BITS> a(*this);
    for(int32_t i = 0; i < WIDTH; ++i)
        pn[i] = 0;

    int32_t k = shift / 32;
    shift = shift % 32;
    for(int32_t i = 0; i < WIDTH; ++i)
    {
        if(i+k+1 < WIDTH && shift != 0)
            pn[i+k+1] |= (a.pn[i] >> (32-shift));
        if(i+k < WIDTH)
            pn[i+k] |= (a.pn[i] << shift);
    }

    return *this;
}



/* Right shift assignment operator. */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator>>=(uint32_t shift)
{
    base_uint<BITS> a(*this);

    for(int32_t i = 0; i < WIDTH; ++i)
        pn[i] = 0;

    int32_t k = shift / 32;
    shift = shift % 32;
    for(int32_t i = 0; i < WIDTH; ++i)
    {
        if(i-k-1 >= 0 && shift != 0)
            pn[i-k-1] |= (a.pn[i] << (32-shift));
        if(i-k >= 0)
            pn[i-k] |= (a.pn[i] >> shift);
    }

    return *this;
}


/* Addition assignment operator. */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator+=(const base_uint<BITS>& b)
{
    uint64_t carry = 0;
    for(uint8_t i = 0; i < WIDTH; ++i)
    {
        uint64_t n = carry + pn[i] + b.pn[i];
        pn[i] = n & 0xffffffff;
        carry = n >> 32;
    }

    return *this;
}


/* Addition assignment operator. (64-bit) */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator+=(uint64_t b64)
{
    base_uint<BITS> b;
    b = b64;
    *this += b;

    return *this;
}


/* Subtraction assignment operator. */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator-=(const base_uint<BITS>& b)
{
    *this += -b;

    return *this;
}


/* Subtraction assignment operator. (64-bit) */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator-=(uint64_t b64)
{
    base_uint<BITS> b;
    b = b64;
    *this += -b;

    return *this;
}


/* Multiply assignment operator. */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator*=(const base_uint<BITS>& b)
{
    base_uint<BITS> a;
    a = 0u;

    for(int j = 0; j < WIDTH; j++)
    {
        uint64_t carry = 0;
        for(int i = 0; i + j < WIDTH; i++)
        {
            uint64_t n = carry + a.pn[i + j] + (uint64_t)pn[j] * b.pn[i];
            a.pn[i + j] = n & 0xffffffff;
            carry = n >> 32;
        }
    }
    *this = a;

    return *this;
}


/* Multiply assignment operator. (64-bit) */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator*=(uint64_t n)
{
    base_uint<BITS> a;
    a = 0u;

    base_uint<BITS> b;
    b = n;

    for(int j = 0; j < WIDTH; j++)
    {
        uint64_t carry = 0;
        for(int i = 0; i + j < WIDTH; i++)
        {
            uint64_t n = carry + a.pn[i + j] + (uint64_t)pn[j] * b.pn[i];
            a.pn[i + j] = n & 0xffffffff;
            carry = n >> 32;
        }
    }
    *this = a;

    return *this;
}



/* Divide assignment operator. */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator/=(const base_uint<BITS>& b)
{
    base_uint<BITS> div = b;     // make a copy, so we can shift.
    base_uint<BITS> num = *this; // make a copy, so we can subtract.
    *this = 0;                   // the quotient.

    int num_bits = num.bits();
    int div_bits = div.bits();

    if(div_bits == 0)
        throw std::domain_error("Division by zero");

    if(div_bits > num_bits) // the result is certainly 0.
        return *this;

    int shift = num_bits - div_bits;

    div <<= shift; // shift so that div and num align.
    while(shift >= 0)
    {
        if(num >= div)
        {
            num -= div;
            pn[shift >> 5] |= (1 << (shift & 31)); // set a bit of the result.
        }

        div >>= 1; // shift back.
        --shift;
    }

    // num now contains the remainder of the division.
    return *this;
}


/* Divide assignment operator. (64-bit) */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator/=(uint64_t b)
{
    *this /= base_uint<BITS>(b);

    // num now contains the remainder of the division.
    return *this;
}



/* Prefix increment operator. */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator++()
{
    // prefix operator
    int i = 0;
    while(++pn[i] == 0 && i < WIDTH-1)
        ++i;

    return *this;
}


/* Postfix increment operator. */
template<uint32_t BITS>
const base_uint<BITS> base_uint<BITS>::operator++(int)
{
    // postfix operator
    const base_uint<BITS> ret = *this;
    ++(*this);

    return ret;
}


/* Prefix decrement operator. */
template<uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::operator--()
{
    // prefix operator
    int i = 0;
    while(--pn[i] == -1 && i < WIDTH-1)
        ++i;

    return *this;
}


/* Postfix decrement operator. */
template<uint32_t BITS>
const base_uint<BITS> base_uint<BITS>::operator--(int)
{
    // postfix operator
    const base_uint<BITS> ret = *this;
    --(*this);

    return ret;
}


/* Relational less-than operator. */
template<uint32_t BITS>
bool base_uint<BITS>::operator<(const base_uint<BITS>& n) const
{
    for(int8_t i = WIDTH-1; i >= 0; --i)
    {
        if(this->pn[i] < n.pn[i])
            return true;
        else if(this->pn[i] > n.pn[i])
            return false;
    }

    return false;
}


/* Relational less-than-or-equal operator. */
template<uint32_t BITS>
bool base_uint<BITS>::operator<=(const base_uint<BITS>& n) const
{
    for(int8_t i = WIDTH-1; i >= 0; --i)
    {
        if(this->pn[i] < n.pn[i])
            return true;
        else if(this->pn[i] > n.pn[i])
            return false;
    }

    return true;
}


/* Relational greater-than operator. */
template<uint32_t BITS>
bool base_uint<BITS>::operator>(const base_uint<BITS>& n) const
{
    for(int8_t i = WIDTH-1; i >= 0; --i)
    {
        if(this->pn[i] > n.pn[i])
            return true;
        else if(this->pn[i] < n.pn[i])
            return false;
    }

    return false;
}


/* Relational greater-than-or-equal operator. */
template<uint32_t BITS>
bool base_uint<BITS>::operator>=(const base_uint<BITS>& n) const
{
    for(int8_t i = WIDTH-1; i >= 0; --i)
    {
        if(this->pn[i] > n.pn[i])
            return true;
        else if(this->pn[i] < n.pn[i])
            return false;
    }

    return true;
}


/* Relational equivalence operator. */
template<uint32_t BITS>
bool base_uint<BITS>::operator==(const base_uint<BITS>& n) const
{
    for(uint8_t i = 0; i < WIDTH; ++i)
        if(this->pn[i] != n.pn[i])
            return false;

    return true;
}


/* Relational equivalence operator. (64-bit) */
template<uint32_t BITS>
bool base_uint<BITS>::operator==(uint64_t n) const
{
    if(this->pn[0] != (uint32_t)n)
        return false;
    if(this->pn[1] != (uint32_t)(n >> 32))
        return false;

    for(uint8_t i = 2; i < WIDTH; ++i)
        if(this->pn[i] != 0)
            return false;

    return true;
}


/* Relational inequivalence operator. */
template<uint32_t BITS>
bool base_uint<BITS>::operator!=(const base_uint<BITS>& n) const
{
    return (!(*this == n));
}


/* Relational inequivalence operator. (64-bit) */
template<uint32_t BITS>
bool base_uint<BITS>::operator!=(uint64_t n) const
{
    return (!(*this == n));
}


/*  Translates the base_uint object to a hex string representation. */
template<uint32_t BITS>
std::string base_uint<BITS>::GetHex() const
{
    char psz[sizeof(pn)*2 + 1];
    for(uint32_t i = 0; i < sizeof(pn); ++i)
        sprintf(psz + i*2, "%02x", ((uint8_t*)pn)[sizeof(pn) - i - 1]);

    return std::string(psz, psz + sizeof(pn)*2);
}


/*  Sets the base_uint object with data parsed from a C-string pointer. */
template<uint32_t BITS>
void base_uint<BITS>::SetHex(const char* psz)
{
    for(int i = 0; i < WIDTH; ++i)
        pn[i] = 0;

    while(isspace(*psz))
        ++psz;

    if(psz[0] == '0' && tolower(psz[1]) == 'x')
        psz += 2;

    const char* pbegin = psz;
    while(phexdigit[(uint8_t)*psz] || *psz == '0')
        ++psz;
    --psz;
    uint8_t* p1 = (uint8_t*)pn;
    uint8_t* pend = p1 + WIDTH * 4;
    while(psz >= pbegin && p1 < pend)
    {
        *p1 = phexdigit[(uint8_t)*psz--];
        if(psz >= pbegin)
        {
            uint32_t tmp = (phexdigit[(uint8_t)*psz--] << 4);
            *p1 = *p1 | static_cast<uint8_t>(tmp);
            ++p1;
        }
    }
}


/*  Sets the base_uint object with data parsed from a standard string object. */
template<uint32_t BITS>
void base_uint<BITS>::SetHex(const std::string& str)
{
    SetHex(str.c_str());
}


/* Set the type byte into base_uint.*/
template<uint32_t BITS>
void base_uint<BITS>::SetType(const uint8_t nType)
{
    /* Mask off most significant byte (little endian). */
    pn[WIDTH -1] = (pn[WIDTH - 1] & 0x00ffffff) + (nType << 24);
}


/*  Get the type byte from base_uint.*/
template<uint32_t BITS>
uint8_t base_uint<BITS>::GetType() const
{
    /* Get type from hash. */
    uint8_t nType = (pn[WIDTH -1] >> 24);

    return nType;
}


/*  Converts the corresponding 32-bit radix integer into bytes.
 *  Used for serializing in Miner LLP */
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


/*  Creates 32-bit radix integer from bytes. Used for de-serializing in Miner LLP */
template<uint32_t BITS>
void base_uint<BITS>::SetBytes(const std::vector<uint8_t> DATA)
{
    for(int index = 0; index < WIDTH; ++index)
    {
        std::vector<uint8_t> BYTES(DATA.begin() + (index * 4), DATA.begin() + (index * 4) + 4);
        pn[index] = (BYTES[0] << 24) + (BYTES[1] << 16) + (BYTES[2] << 8) + (BYTES[3]);
    }
}


/* Computes and returns the count of the highest order bit set. */
template<uint32_t BITS>
uint32_t base_uint<BITS>::BitCount() const
{
    uint32_t i = (WIDTH << 5) - 1;

    for(; i >= 0; --i)
    {
        if(pn[i >> 5] & (1 << (i & 31)))
            break;
    }

    /* Any number will have at least 1 bit. */
    return i + 1;
}


/*  Returns a string representation of the base_uint object. */
template<uint32_t BITS>
std::string base_uint<BITS>::ToString() const
{
    return (GetHex());
}


/*  Returns a string representation of the base_uint object. */
template<uint32_t BITS>
std::string base_uint<BITS>::SubString(const uint32_t nSize) const
{
    return (GetHex().substr(0, nSize));
}



/*  Returns a byte pointer to the begin of the base_uint object. */
template<uint32_t BITS>
uint8_t* base_uint<BITS>::begin() const
{
    return (uint8_t*)&pn[0];
}


/*  Returns a byte pointer to the end of the base_uint object. */
template<uint32_t BITS>
uint8_t* base_uint<BITS>::end() const
{
    return (uint8_t*)&pn[WIDTH];
}


/*  Gets a 32-bit word from the base_uint array at the given index. */
template<uint32_t BITS>
uint32_t base_uint<BITS>::get(uint32_t n) const
{
    return pn[n];
}


/*  Gets a 32-bit word from the base_uint array. If the base_uint cannot be
 *  represented as 32-bit bit word. Returns all-bits set (i.e 0xFFFFFFFF) */
template<uint32_t BITS>
uint32_t base_uint<BITS>::getuint32() const
{
    if(bits() > 64)
        return std::numeric_limits<uint32_t>::max();

    return static_cast<uint32_t>(Get64(0));
}


/*  Sets the base_uint object from a vector of data. */
template<uint32_t BITS>
void base_uint<BITS>::set(const std::vector<uint32_t>& data)
{
    for(int i = 0; i < WIDTH; ++i)
        pn[i] = data[i];
}


/*  Returns the size in bytes of the base_uint object. */
template<uint32_t BITS>
uint32_t base_uint<BITS>::size()
{
    return sizeof(pn);
}


/*  Gets a 64-bit integer comprised of low and high 32-bit words derived from
 *  the number array at the given 64-bit index. */
template<uint32_t BITS>
uint64_t base_uint<BITS>::Get64(uint32_t n) const
{
    return pn[2*n] | (uint64_t)pn[2*n+1] << 32;
}


/*  A helper function used to inspect the most significant word. This can be
  *  used to determine the size in base 2 up to 992 bits. */
template<uint32_t BITS>
uint32_t base_uint<BITS>::high_bits(uint32_t nMask)
{
    return pn[WIDTH-1] & nMask;
}


/*  Returns the number of bits represented in the integer. */
template <uint32_t BITS>
uint32_t base_uint<BITS>::bits() const
{
    for(int32_t nPos = WIDTH - 1; nPos >= 0; --nPos)
    {
        if(pn[nPos])
        {
            for(int32_t nbits = 31; nbits > 0; --nbits)
            {
                if(pn[nPos] & 1U << nbits)
                    return 32 * nPos + nbits + 1;
            }
            return 32 * nPos + 1;
        }
    }

    return 0;
}



template <uint32_t BITS>
base_uint<BITS>& base_uint<BITS>::SetCompact(uint32_t nCompact)
{
    int nSize = nCompact >> 24;
    uint32_t nWord = nCompact & 0x007fffff;

    if(nSize <= 3)
    {
        nWord >>= 8 * (3 - nSize);
        *this = nWord;
    }
    else
    {
        *this = nWord;
        *this <<= 8 * (nSize - 3);
    }

    return *this;
}


template <uint32_t BITS>
uint32_t base_uint<BITS>::GetCompact() const
{
    uint32_t nBits = bits();
    uint32_t nSize = ((nBits + 7) / 8) + ((nBits & 0x07) == 0);

    uint32_t nCompact = 0;
    if(nSize <= 3)
    {
        nCompact = Get64() << 8 * (3 - nSize);
    }
    else
    {
        base_uint<BITS> bn = *this >> 8 * (nSize - 3);
        nCompact = bn.Get64();
    }

    nCompact |= nSize << 24;

    return nCompact;
}


/* Explicitly instantiate all template instances needed for compiler. */
template class base_uint<128>;
template class base_uint<256>;
template class base_uint<512>;
template class base_uint<576>;
template class base_uint<1024>;
template class base_uint<1056>;
template class base_uint<1088>;
