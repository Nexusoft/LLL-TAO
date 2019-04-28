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


#include <string>
#include <vector>
#include <cstdint>
#include <cmath>


/** base_uint
 *
 *  The base_uint template class is designed to handle big number arithmetic of
 *  arbitrary sizes. It is intended to be used as a base class that can be further
 *  extended in a derived class that implement additional functionality. This class
 *  maintains the general core math operations and is the foundation for big numbers
 *  that are used for encryption.
 *
 **/
template<uint32_t BITS>
class base_uint
{

protected:

    /* Determine the width in number of words. */
    enum { WIDTH=BITS/32 };

    /* The 32-bit integer bignum array. */
    uint32_t pn[WIDTH];


public:

    /** Default Constructor. **/
    base_uint();


    /** Destructor. **/
    ~base_uint();


    /** Copy Constructor. **/
    base_uint(const base_uint& rhs);


    /** Copy Constructor. **/
    template<uint32_t BITS2>
    base_uint(const base_uint<BITS2>& rhs)
    {
        uint32_t nMinWidth = std::min((uint32_t)WIDTH, (uint32_t)rhs.WIDTH);

        for(uint8_t i = 0; i < nMinWidth; ++i)
            pn[i] = rhs.pn[i];
    }


    /** Constructor. (64-bit) **/
    base_uint(uint64_t rhs64);


    /** Assignment operator. **/
    base_uint& operator=(const base_uint& rhs);


    /** Assignment operator. **/
    template<uint32_t BITS2>
    base_uint& operator=(const base_uint<BITS2>& rhs)
    {
        uint32_t nMinWidth = std::min((uint32_t)WIDTH, (uint32_t)rhs.WIDTH);

        for (uint8_t i = 0; i < nMinWidth; ++i)
            pn[i] = rhs.pn[i];

        return *this;
    }


    /** Assignment operator. (64-bit) **/
    base_uint& operator=(uint64_t rhs64);


    /** Constructor. (from string) **/
    explicit base_uint(const std::string& str);


    /** Constructor. (from vector) **/
    explicit base_uint(const std::vector<uint8_t>& vch);


    /** Logical NOT unary operator. **/
    bool operator!() const;


    /** Bitwise inversion unary operator. **/
    const base_uint operator~() const;


    /** One's complement (negation) bitwise unary operator **/
    const base_uint operator-() const;


    /** XOR assignment operator. **/
    base_uint& operator^=(const base_uint& rhs);


    /** XOR assignment operator. (64-bit) **/
    base_uint& operator^=(uint64_t rhs64);


    /** OR assignment operator. **/
    base_uint& operator|=(const base_uint& rhs);


    /** OR assignment operator. (64-bit) **/
    base_uint& operator|=(uint64_t rhs64);


    /** AND assignment operator. **/
    base_uint& operator&=(const base_uint& rhs);


    /** Left shift assignment operator. **/
    base_uint& operator<<=(uint32_t shift);


    /** Right shift assignment operator. **/
    base_uint& operator>>=(uint32_t shift);


    /** Addition assignment operator. **/
    base_uint& operator+=(const base_uint& rhs);


    /** Addition assignment operator. (64-bit) **/
    base_uint& operator+=(uint64_t rhs64);


    /** Subtraction assignment operator. **/
    base_uint& operator-=(const base_uint& rhs);


    /** Subtraction assignment operator. (64-bit) **/
    base_uint& operator-=(uint64_t rhs64);


    /** Multiply assignment operator. **/
    base_uint& operator*=(const base_uint& rhs);


    /** Multiply assignment operator. (64-bit) **/
    base_uint& operator*=(uint64_t rhs64);


    /** Divide assignment operator. **/
    base_uint& operator/=(const base_uint& rhs);


    /** Divide assignment operator. (64-bit) **/
    base_uint& operator/=(uint64_t rhs);


    /** Prefix increment operator. **/
    base_uint& operator++();


    /** Postfix increment operator. **/
    const base_uint operator++(int);


    /** Prefix decrement operator. **/
    base_uint& operator--();


    /** Postfix decrement operator. **/
    const base_uint operator--(int);


    /** Relational less-than operator. **/
    bool operator<(const base_uint& rhs) const;


    /** Relational less-than-or-equal operator. **/
    bool operator<=(const base_uint& rhs) const;


    /** Relational greater-than operator. **/
    bool operator>(const base_uint& rhs) const;


    /** Relational greater-than-or-equal operator. **/
    bool operator>=(const base_uint& rhs) const;


    /** Relational equivalence operator. **/
    bool operator==(const base_uint& rhs) const;


    /** Relational equivalence operator. (64-bit) **/
    bool operator==(uint64_t rhs) const;


    /** Relational inequivalence operator. **/
    bool operator!=(const base_uint& rhs) const;


    /** Relational inequivalence operator. (64-bit) **/
    bool operator!=(uint64_t rhs) const;


    /** GetHex
     *
     *  Translates the base_uint object to a hex string representation.
     *
     *  @return Returns the hex string representation of the base_uint object.
     *
     **/
    std::string GetHex() const;


    /** SetHex
     *
     *  Sets the base_uint object with data parsed from a C-string pointer.
     *
     *  @param[in] str The string to interpret the base_uint from.
     *
     **/
    void SetHex(const char* psz);


    /** SetHex
     *
     *  Sets the base_uint object with data parsed from a standard string object.
     *
     *  @param[in] str The string to interpret the base_uint from.
     *
     **/
    void SetHex(const std::string& str);

    /** GetBytes
     *
     *  Converts the corresponding 32-bit radix integer into bytes.
     *  Used for serializing in Miner LLP
     *
     *  @return Returns the serialized byte array of the base_uint object.
     *
     **/
    const std::vector<uint8_t> GetBytes() const;


    /** SetBytes
     *
     *  Creates 32-bit radix integer from bytes. Used for de-serializing in Miner LLP
     *
     *  @param[in] DATA The data to set the bytes with.
     *
     **/
    void SetBytes(const std::vector<uint8_t> DATA);


    /** BitCount
     *
     * Computes and returns the count of the highest order bit set.
     *
     **/
    uint32_t BitCount() const;


    /** ToString
     *
     *  Returns a string representation of the base_uint object.
     *
     **/
    std::string ToString() const;


    /** begin
     *
     *  Returns a byte pointer to the begin of the base_uint object.
     *
     **/
    uint8_t* begin();


    /** end
     *
     *  Returns a byte pointer to the end of the base_uint object.
     *
     **/
    uint8_t* end();


    /** get
     *
     *  Gets a 32-bit word from the base_uint array at the given index.
     *
     *  @param[in] n The index to retrieve a word from.
     *
     *  @return Returns the 32-bit word at the given index.
     *
     **/
    uint32_t get(uint32_t n);


    /** set
     *
     *  Sets the base_uint object from a vector of data.
     *
     *  @param[in] data The data to set the base_uint array with.
     *
     **/
    void set(const std::vector<uint32_t>& data);


    /** size
     *
     *  Returns the size in bytes of the base_uint object.
     *
     **/
    uint32_t size();


    /** GetCompact
     *
     *  Returns the 32-bit compact representation of the base_uint.
     *
     **/
    uint32_t GetCompact() const;


    /** SetCompact
     *
     *  Sets the base_uint from the compact 32-bit representation.
     *
     *  @param[in] nCompact The compact representation to set.
     *
     *  @return Returns the compact representation in a base_uint class.
     *
     **/
    base_uint& SetCompact(uint32_t nCompact);


    /** Get64
     *
     *  Gets a 64-bit integer comprised of low and high 32-bit words derived from
     *  the number array at the given 64-bit index.
     *
     *  @param[in] n The 64-bit index into a 32-bit array.
     *
     *  @return Returns the 64-bit composed integer from 32-bit word array.
     *
     **/
    uint64_t Get64(uint32_t n=0) const;


    /** GetSerializeSize
     *
     *  Gets the serialize size in bytes of the base_uint object.
     *
     *  @param[in] nSerType The serialization type.
     *  @param[in] nVersion The serialization version.
     *
     *  @return Returns the size of the base_uint object in bytes.
     *
     **/
    uint32_t GetSerializeSize(int nSerType, int nVersion) const
    {
        return sizeof(pn);
    }


    /** Serialize
     *
     *  Takes a templated stream and serializes (writes) the base_uint object.
     *
     *  @param[in/out] s The template stream to serialize with, potentially
     *                   modifying it's internal state.
     *  @param[in] nSertype The serialization type.
     *  @param[in] nVersion The serialization version.
     *
     **/
    template<typename Stream>
    void Serialize(Stream& s, int nSerType, int nVersion) const
    {
        s.write((char*)pn, sizeof(pn));
    }


    /** Unserialize
     *
     *  Takes a templated stream and unserializes (reads) the base_uint object.
     *
     *  @param[in/out] s The template stream to serialize with, potentially
     *                   modifying it's internal state.
     *  @param[in] nSertype The serialization type.
     *  @param[in] nVersion The serialization version.
     *
     **/
    template<typename Stream>
    void Unserialize(Stream& s, int nSerType, int nVersion)
    {
        s.read((char*)pn, sizeof(pn));
    }


    /** high_bits
     *
     *  A helper function used to inspect the most significant word. This can be
     *  used to determine the size in base 2 up to 992 bits.
     *
     *  @param[in] mask The bitmask used to compare against the high word.
     *
     *  @return Returns the mask of the high word.
     *
     **/
    uint32_t high_bits(uint32_t mask);


    /** bits
     *
     *  Returns the number of bits represented in the integer.
     *
     **/
    uint32_t bits() const;


    /* Needed for specialized copy and assignment constructors. */
    friend class base_uint<1024>;
    friend class base_uint<576>;
    friend class base_uint<512>;
    friend class base_uint<256>;
    friend class base_uint<192>;

};


/** Relational equivalence binary operator (64-bit). **/
template<uint32_t BITS>
bool operator==(const base_uint<BITS>& a, uint64_t b)
{
    return a == b;
}


/** Relational inequivalence binary operator (64-bit). **/
template<uint32_t BITS>
bool operator!=(const base_uint<BITS>& a, uint64_t b)
{
    return a != b;
}


/** Relational equivalence binary operator. **/
template<uint32_t BITS>
bool operator==(const base_uint<BITS>& a, const base_uint<BITS> &b)
{
    return a == b;
}


/** Relational inequivalence binary operator. **/
template<uint32_t BITS>
bool operator!=(const base_uint<BITS>& a, const base_uint<BITS> &b)
{
    return a != b;
}


/** Relational less-than binary operator. **/
template<uint32_t BITS>
bool operator<(const base_uint<BITS> &a, const base_uint<BITS> &b)
{
    return a < b;
}


/** Relational less-than-or-equal binary operator. **/
template<uint32_t BITS>
bool operator<=(const base_uint<BITS> &a, const base_uint<BITS> &b)
{
    return a <= b;
}


/** Relational greater-than binary operator. **/
template<uint32_t BITS>
bool operator>(const base_uint<BITS> &a, const base_uint<BITS> &b)
{
    return a > b;
}


/** Relational greater-than-or-equal binary operator. **/
template<uint32_t BITS>
bool operator>=(const base_uint<BITS> &a, const base_uint<BITS> &b)
{
    return a >= b;
}


/** Left shift binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator<<(const base_uint<BITS> &lhs, uint32_t shift)
{
    return base_uint<BITS>(lhs) <<= shift;
}


/** Right shift binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator>>(const base_uint<BITS> &lhs, uint32_t shift)
{
    return base_uint<BITS>(lhs) >>= shift;
}


/** Bitwise XOR binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator^(const base_uint<BITS> &lhs, const base_uint<BITS> &rhs)
{
    return base_uint<BITS>(lhs) ^= rhs;
}


/** Bitwise OR binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator|(const base_uint<BITS> &lhs, const base_uint<BITS> &rhs)
{
    return base_uint<BITS>(lhs) |= rhs;
}


/** Bitwise AND binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator&(const base_uint<BITS> &lhs, const base_uint<BITS> &rhs)
{
    return base_uint<BITS>(lhs) &= rhs;
}


/** Multiply binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator*(const base_uint<BITS> &lhs, const base_uint<BITS> &rhs)
{
    return base_uint<BITS>(lhs) *= rhs;
}


/** Multiply binary operator. (64-bit) **/
template<uint32_t BITS>
const base_uint<BITS> operator*(const base_uint<BITS> &lhs, uint64_t rhs)
{
    return base_uint<BITS>(lhs) *= rhs;
}


/** Divide binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator/(const base_uint<BITS> &lhs, const base_uint<BITS> &rhs)
{
    return base_uint<BITS>(lhs) /= rhs;
}


/** Divide binary operator. (64-bit) **/
template<uint32_t BITS>
const base_uint<BITS> operator/(const base_uint<BITS> &lhs, uint64_t rhs)
{
    return base_uint<BITS>(lhs) /= rhs;
}


/** Addition binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator+(const base_uint<BITS> &lhs, const base_uint<BITS> &rhs)
{
    return base_uint<BITS>(lhs) += rhs;
}


/** Addition binary operator. (64-bit)**/
template<uint32_t BITS>
const base_uint<BITS> operator+(const base_uint<BITS> &lhs, uint64_t rhs)
{
    return base_uint<BITS>(lhs) += rhs;
}


/** Subtraction binary operator **/
template<uint32_t BITS>
const base_uint<BITS> operator-(const base_uint<BITS> &lhs, const base_uint<BITS> &rhs)
{
    return base_uint<BITS>(lhs) -= rhs;
}


/** Subtraction binary operator. (64-bit)**/
template<uint32_t BITS>
const base_uint<BITS> operator-(const base_uint<BITS> &lhs, uint64_t rhs)
{
    return base_uint<BITS>(lhs) -= rhs;
}



#endif
