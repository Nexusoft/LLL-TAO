/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2018

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_LLC_TYPES_BIGNUM_H
#define NEXUS_LLC_TYPES_BIGNUM_H

#include <LLC/types/uint1024.h>
#include <LLP/include/version.h> /* for serialization version */
#include <stdexcept>
#include <vector>

/* forward declarations */
struct bignum_st;
struct bignum_ctx;
typedef struct bignum_st BIGNUM;
typedef struct bignum_ctx BN_CTX;

namespace LLC
{

    /** Errors thrown by the bignum class */
    class bignum_error : public std::runtime_error
    {
    public:
        explicit bignum_error(const std::string& str);
    };

    /** RAII encapsulated BN_CTX (OpenSSL bignum context) */
    class CAutoBN_CTX
    {
    protected:
        BN_CTX* pctx;
        BN_CTX* operator=(BN_CTX* pnew);

    public:
        CAutoBN_CTX();

        ~CAutoBN_CTX();

        operator BN_CTX*();

        BN_CTX& operator*();

        BN_CTX** operator&();

        bool operator!();
    };


    /** C++ wrapper for BIGNUM (OpenSSL bignum) */
    class CBigNum
    {
    public:
        CBigNum();

        CBigNum(const CBigNum& b);

        CBigNum& operator=(const CBigNum& b);

        ~CBigNum();

        //CBigNum(char n) is not portable.  Use 'signed char' or 'uint8_t'.
        CBigNum(int8_t n);
        CBigNum(int16_t n);
        CBigNum(int32_t n);
        //CBigNum(long n);
        CBigNum(int64_t n);
        CBigNum(uint8_t n);
        CBigNum(uint16_t n);
        CBigNum(uint32_t n);
        //CBigNum(unsigned long n);
        CBigNum(uint64_t n);
        explicit CBigNum(uint256_t n);
        explicit CBigNum(uint512_t n);
        explicit CBigNum(uint576_t n);
        explicit CBigNum(uint1024_t n);

        explicit CBigNum(const std::vector<uint8_t>& vch);

        BIGNUM* const getBN() const;

        void setulong(unsigned long n);

        unsigned long getulong() const;

        uint32_t getuint() const;

        int getint() const;

        void setint64(int64_t n);

        void setuint64(uint64_t n);

        uint64_t getuint64() const;

        void setuint256(uint256_t n);

        uint256_t getuint256() const;

        void setuint512(uint512_t n);

        uint512_t getuint512() const;

        void setuint576(uint576_t n);

        uint576_t getuint576() const;

        void setuint1024(uint1024_t n);

        uint1024_t getuint1024() const;

        void setvch(const std::vector<uint8_t>& vch);

        std::vector<uint8_t> getvch() const;

        CBigNum& SetCompact(uint32_t nCompact);

        uint32_t GetCompact() const;

        void SetHex(const std::string& str);

        std::string ToString(int nBase=10) const;

        std::string GetHex() const;

        uint32_t GetSerializeSize(int nSerType=0, int nVersion=LLP::PROTOCOL_VERSION) const;

        template<typename Stream>
        void Serialize(Stream& s, int nSerType=0, int nVersion=LLP::PROTOCOL_VERSION) const;

        template<typename Stream>
        void Unserialize(Stream& s, int nSerType=0, int nVersion=LLP::PROTOCOL_VERSION);


        bool operator!() const;
        CBigNum& operator+=(const CBigNum& b);
        CBigNum& operator-=(const CBigNum& b);
        CBigNum& operator*=(const CBigNum& b);
        CBigNum& operator/=(const CBigNum& b);
        CBigNum& operator%=(const CBigNum& b);
        CBigNum& operator<<=(uint32_t shift);
        CBigNum& operator>>=(uint32_t shift);
        CBigNum& operator++();
        const CBigNum operator++(int);
        CBigNum& operator--();
        const CBigNum operator--(int);


        friend const CBigNum operator-(const CBigNum& a, const CBigNum& b);
        friend const CBigNum operator/(const CBigNum& a, const CBigNum& b);
        friend const CBigNum operator%(const CBigNum& a, const CBigNum& b);

    private:
        BIGNUM* m_BN = nullptr;

        void allocate();
    };


    const CBigNum operator+(const CBigNum& a, const CBigNum& b);
    const CBigNum operator-(const CBigNum& a, const CBigNum& b);
    const CBigNum operator-(const CBigNum& a);
    const CBigNum operator*(const CBigNum& a, const CBigNum& b);
    const CBigNum operator/(const CBigNum& a, const CBigNum& b);
    const CBigNum operator%(const CBigNum& a, const CBigNum& b);
    const CBigNum operator<<(const CBigNum& a, uint32_t shift);
    const CBigNum operator>>(const CBigNum& a, uint32_t shift);
    bool operator==(const CBigNum& a, const CBigNum& b);
    bool operator!=(const CBigNum& a, const CBigNum& b);
    bool operator<=(const CBigNum& a, const CBigNum& b);
    bool operator>=(const CBigNum& a, const CBigNum& b);
    bool operator<(const CBigNum& a, const CBigNum& b);
    bool operator>(const CBigNum& a, const CBigNum& b);

}

#endif
