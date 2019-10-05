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


//forward declarations
namespace TAO { namespace Register { class Address; } }


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


    /** Copy Constructor. **/
    base_uint(const base_uint& n);


    /** Move Constructor. **/
    base_uint(base_uint&& n) noexcept;


    /** Copy Assignment. **/
    base_uint& operator=(const base_uint& n);


    /** Move Assignment. **/
    base_uint& operator=(base_uint&& n) noexcept;


    /** Assignment operator. (64-bit) **/
    base_uint& operator=(const uint64_t nn);


    /** Copy Assignment Variable Width Template. **/
    template<uint32_t BITS2>
    base_uint& operator=(const base_uint<BITS2>& n)
    {
        for(uint8_t i = 0; i < n.WIDTH; ++i)
        {
            if(i < WIDTH)
                pn[i] = n.pn[i];
        }

        for(uint8_t i = n.WIDTH; i < WIDTH; ++i)
            pn[i] = 0;

        return *this;
    }


    /** Move Assignment Variable Width Template. **/
    template<uint32_t BITS2>
    base_uint& operator=(base_uint<BITS2>&& n) noexcept
    {
        for(uint8_t i = 0; i < n.WIDTH; ++i)
        {
            if(i < WIDTH)
                pn[i] = std::move(n.pn[i]);
        }

        for(uint8_t i = n.WIDTH; i < WIDTH; ++i)
            pn[i] = 0;

        return *this;
    }


    /** Destructor. **/
    ~base_uint();


    /** Copy Constructor. **/
    template<uint32_t BITS2>
    base_uint(const base_uint<BITS2>& n)
    {
        for(uint8_t i = 0; i < n.WIDTH; ++i)
        {
            if(i < WIDTH)
                pn[i] = n.pn[i];
        }

        for(uint8_t i = n.WIDTH; i < WIDTH; ++i)
            pn[i] = 0;
    }


    /** Constructor. (64-bit) **/
    base_uint(const uint64_t n);


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
    base_uint& operator^=(const base_uint& n);


    /** XOR assignment operator. (64-bit) **/
    base_uint& operator^=(uint64_t n);


    /** OR assignment operator. **/
    base_uint& operator|=(const base_uint& n);


    /** OR assignment operator. (64-bit) **/
    base_uint& operator|=(uint64_t n);


    /** AND assignment operator. **/
    base_uint& operator&=(const base_uint& n);


    /** Left shift assignment operator. **/
    base_uint& operator<<=(uint32_t shift);


    /** Right shift assignment operator. **/
    base_uint& operator>>=(uint32_t shift);


    /** Addition assignment operator. **/
    base_uint& operator+=(const base_uint& n);


    /** Addition assignment operator. (64-bit) **/
    base_uint& operator+=(uint64_t n);


    /** Subtraction assignment operator. **/
    base_uint& operator-=(const base_uint& n);


    /** Subtraction assignment operator. (64-bit) **/
    base_uint& operator-=(uint64_t n);


    /** Multiply assignment operator. **/
    base_uint& operator*=(const base_uint& n);


    /** Multiply assignment operator. (64-bit) **/
    base_uint& operator*=(uint64_t n);


    /** Divide assignment operator. **/
    base_uint& operator/=(const base_uint& n);


    /** Divide assignment operator. (64-bit) **/
    base_uint& operator/=(uint64_t n);


    /** Prefix increment operator. **/
    base_uint& operator++();


    /** Postfix increment operator. **/
    const base_uint operator++(int);


    /** Prefix decrement operator. **/
    base_uint& operator--();


    /** Postfix decrement operator. **/
    const base_uint operator--(int);


    /** Relational less-than operator. **/
    bool operator<(const base_uint& n) const;


    /** Relational less-than-or-equal operator. **/
    bool operator<=(const base_uint& n) const;


    /** Relational greater-than operator. **/
    bool operator>(const base_uint& n) const;


    /** Relational greater-than-or-equal operator. **/
    bool operator>=(const base_uint& n) const;


    /** Relational equivalence operator. **/
    bool operator==(const base_uint& n) const;


    /** Relational equivalence operator. (64-bit) **/
    bool operator==(uint64_t n) const;


    /** Relational inequivalence operator. **/
    bool operator!=(const base_uint& n) const;


    /** Relational inequivalence operator. (64-bit) **/
    bool operator!=(uint64_t n) const;


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


    /** SetType
     *
     *  Set a type byte into base_uint.
     *
     *  @param[in] nType The type byte for address.
     *
     **/
    void SetType(const uint8_t nType);


    /** GetType
     *
     *  Get the type byte from base_uint.
     *
     *  @param[in] nType The type byte for address.
     *
     **/
    uint8_t GetType() const;

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


    /** SubString
     *
     *  Returns a sub-string representation of the base_uint object.
     *
     **/
    std::string SubString(const uint32_t nSize = 20) const;


    /** begin
     *
     *  Returns a byte pointer to the begin of the base_uint object.
     *
     **/
    uint8_t* begin() const;


    /** end
     *
     *  Returns a byte pointer to the end of the base_uint object.
     *
     **/
    uint8_t* end() const;


    /** get
     *
     *  Gets a 32-bit word from the base_uint array at the given index.
     *
     *  @param[in] n The index to retrieve a word from.
     *
     *  @return Returns the 32-bit word at the given index.
     *
     **/
    uint32_t get(uint32_t n) const;


    /** getuint32
     *
     *  Gets a 32-bit word from the base_uint array. If the base_uint cannot be
     *  represented as 32-bit bit word. Returns all-bits set (i.e 0xFFFFFFFF)
     *
     *  @return Returns the 32-bit integer representation, or all-bits-set.
     *
     **/
    uint32_t getuint32() const;


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
     *  TODO: Not currently operatational for base_uint due to OpenSSL port
     *        issues. needs a proper port
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
    friend class TAO::Register::Address;

    friend class base_uint<1088>;
    friend class base_uint<1056>;
    friend class base_uint<1024>;
    friend class base_uint<576>;
    friend class base_uint<512>;
    friend class base_uint<256>;
    friend class base_uint<192>;
    friend class base_uint<128>;

};


/** Relational equivalence binary operator (64-bit). **/
template<uint32_t BITS>
bool operator==(const base_uint<BITS>& a, uint64_t b)
{
    return a == b;
}


/** Relational equivalence binary operator (64-bit). **/
template<uint32_t BITS>
bool operator==(uint64_t b, const base_uint<BITS>& a)
{
    return a == b;
}


/** Relational inequivalence binary operator (64-bit). **/
template<uint32_t BITS>
bool operator!=(const base_uint<BITS>& a, uint64_t b)
{
    return a != b;
}

/** Relational inequivalence binary operator (64-bit). **/
template<uint32_t BITS>
bool operator!=(uint64_t b, const base_uint<BITS>& a)
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


/** Relational less-than binary operator. (64-bit) **/
template<uint32_t BITS>
bool operator<(const base_uint<BITS> &a, uint64_t b)
{
    return a < base_uint<BITS>(b);
}


/** Relational less-than binary operator. (64-bit) **/
template<uint32_t BITS>
bool operator<(uint64_t a, const base_uint<BITS> &b)
{
    return base_uint<BITS>(a) < b;
}


/** Relational less-than-or-equal binary operator. **/
template<uint32_t BITS>
bool operator<=(const base_uint<BITS> &a, const base_uint<BITS> &b)
{
    return a <= b;
}


/** Relational less-than-or-equal binary operator. (64-bit) **/
template<uint32_t BITS>
bool operator<=(const base_uint<BITS> &a, uint64_t b)
{
    return a <= base_uint<BITS>(b);
}


/** Relational less-than-or-equal binary operator. (64-bit) **/
template<uint32_t BITS>
bool operator<=(uint64_t a, const base_uint<BITS> &b)
{
    return base_uint<BITS>(a) <= b;
}


/** Relational greater-than binary operator. **/
template<uint32_t BITS>
bool operator>(const base_uint<BITS> &a, const base_uint<BITS> &b)
{
    return a > b;
}


/** Relational greater-than binary operator. (64-bit) **/
template<uint32_t BITS>
bool operator>(const base_uint<BITS> &a, uint64_t b)
{
    return a > base_uint<BITS>(b);
}


/** Relational greater-than binary operator. (64-bit) **/
template<uint32_t BITS>
bool operator>(uint64_t a, const base_uint<BITS> &b)
{
    return base_uint<BITS>(a) > b;
}


/** Relational greater-than-or-equal binary operator. **/
template<uint32_t BITS>
bool operator>=(const base_uint<BITS> &a, const base_uint<BITS> &b)
{
    return a >= b;
}


/** Relational greater-than-or-equal binary operator  (64-bit). **/
template<uint32_t BITS>
bool operator>=(const base_uint<BITS> &a, uint64_t b)
{
    return a >= base_uint<BITS>(b);
}


/** Relational greater-than-or-equal binary operator  (64-bit). **/
template<uint32_t BITS>
bool operator>=(uint64_t a, const base_uint<BITS> &b)
{
    return base_uint<BITS>(a) >= b;
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
const base_uint<BITS> operator^(const base_uint<BITS> &lhs, const base_uint<BITS> &n)
{
    return base_uint<BITS>(lhs) ^= n;
}


/** Bitwise OR binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator|(const base_uint<BITS> &lhs, const base_uint<BITS> &n)
{
    return base_uint<BITS>(lhs) |= n;
}


/** Bitwise AND binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator&(const base_uint<BITS> &lhs, const base_uint<BITS> &n)
{
    return base_uint<BITS>(lhs) &= n;
}


/** Multiply binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator*(const base_uint<BITS> &lhs, const base_uint<BITS> &n)
{
    return base_uint<BITS>(lhs) *= n;
}


/** Multiply binary operator. (64-bit) **/
template<uint32_t BITS>
const base_uint<BITS> operator*(const base_uint<BITS> &lhs, uint64_t n)
{
    return base_uint<BITS>(lhs) *= n;
}


/** Multiply binary operator. (64-bit) **/
template<uint32_t BITS>
const base_uint<BITS> operator*(uint64_t n, const base_uint<BITS> &lhs)
{
    return base_uint<BITS>(lhs) *= n;
}


/** Divide binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator/(const base_uint<BITS> &lhs, const base_uint<BITS> &n)
{
    return base_uint<BITS>(lhs) /= n;
}


/** Divide binary operator. (64-bit) **/
template<uint32_t BITS>
const base_uint<BITS> operator/(const base_uint<BITS> &lhs, uint64_t n)
{
    return base_uint<BITS>(lhs) /= n;
}

/** Modulo binary operator. (16-bit) **/
template<uint32_t BITS>
uint32_t operator%(const base_uint<BITS> &lhs, uint16_t n)
{
	uint32_t x=0;
    uint32_t y=0;
    uint32_t z=0;

	for(int8_t i = (BITS>>5)-1; i>=0; --i)
	{
		x = lhs.get(i);
		y = (y << 16) | (x >> 16);
		z = y / n;
		y -= z * n;
		x <<= 16;
		y = (y << 16) | (x >> 16);
		z = y / n;
		y -= z * n;
	}

	return y;
}


/** Addition binary operator. **/
template<uint32_t BITS>
const base_uint<BITS> operator+(const base_uint<BITS> &lhs, const base_uint<BITS> &n)
{
    return base_uint<BITS>(lhs) += n;
}


/** Addition binary operator. (64-bit)**/
template<uint32_t BITS>
const base_uint<BITS> operator+(const base_uint<BITS> &lhs, uint64_t n)
{
    return base_uint<BITS>(lhs) += n;
}


/** Addition binary operator. (64-bit)**/
template<uint32_t BITS>
const base_uint<BITS> operator+(uint64_t n, const base_uint<BITS> &lhs)
{
    return base_uint<BITS>(lhs) += n;
}


/** Subtraction binary operator **/
template<uint32_t BITS>
const base_uint<BITS> operator-(const base_uint<BITS> &lhs, const base_uint<BITS> &n)
{
    return base_uint<BITS>(lhs) -= n;
}


/** Subtraction binary operator. (64-bit)**/
template<uint32_t BITS>
const base_uint<BITS> operator-(const base_uint<BITS> &lhs, uint64_t n)
{
    return base_uint<BITS>(lhs) -= n;
}


/** Subtraction binary operator. (64-bit)**/
template<uint32_t BITS>
const base_uint<BITS> operator-(uint64_t lhs, const base_uint<BITS> &n)
{
    return base_uint<BITS>(lhs) -= n;
}


/* Custom hash function to allow all base_uint specializations to be used wherever a std::hash function
   is required, such as in an unordered_set */
namespace std
{
    template <uint32_t BITS>
    struct hash<base_uint<BITS> >
    {
        size_t operator()(const base_uint<BITS>& val) const
        {
            /* Base the hash function on the string representation, which is the number converted to hex */
            return hash<std::string>()(val.ToString());
        }
    };
}

#endif
