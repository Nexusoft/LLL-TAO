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

    /** bignum_error
     *
     *  Errors thrown by the bignum class
     *
     **/
    class bignum_error : public std::runtime_error
    {
    public:
        explicit bignum_error(const std::string& str);
    };


    /** CAutoBN_CTX
     *
     *  RAII encapsulated BN_CTX (OpenSSL bignum context)
     *
     **/
    class CAutoBN_CTX
    {
    protected:
        /* pointer to bignum context */
        BN_CTX *pctx;

        /* assignment constructor */
        BN_CTX *operator=(BN_CTX *pnew);

    public:

        /* default constructor */
        CAutoBN_CTX();

        /*default destructor */
        ~CAutoBN_CTX();

        /* operator overloads */
        operator BN_CTX*();
        BN_CTX& operator*();
        BN_CTX** operator&();
        bool operator!();
    };


    /** CAutoBN_CTX
     *
     *  C++ wrapper for BIGNUM (OpenSSL bignum)
     *
     **/
    class CBigNum
    {
    public:

        /** CBigNum
         *
         *  default constructor
         *
         **/
        CBigNum();


        /** CBigNum
         *
         *  default copy constructor
         *
         **/
        CBigNum(const CBigNum& b);


        /** operator=
         *
         *  assignment operator
         *
         **/
        CBigNum& operator=(const CBigNum& b);


        /** ~CBigNum
         *
         *  default destructor
         *
         **/
        ~CBigNum();


        /** CBigNum
         *
         *  construct class from signed integer n
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        CBigNum(int8_t n);


        /** CBigNum
         *
         *  construct class from signed integer n
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        CBigNum(int16_t n);


        /** CBigNum
         *
         *  construct class from signed integer n
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        CBigNum(int32_t n);


        /** CBigNum
         *
         *  construct class from signed integer n
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        CBigNum(int64_t n);


        /** CBigNum
         *
         *  construct class from unsigned integer n
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        CBigNum(uint8_t n);


        /** CBigNum
         *
         *  construct class from unsigned integer n
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        CBigNum(uint16_t n);


        /** CBigNum
         *
         *  construct class from unsigned integer n
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        CBigNum(uint32_t n);


        /** CBigNum
         *
         *  construct class from unsigned integer n
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        CBigNum(uint64_t n);


        /** CBigNum
         *
         *  construct class from unsigned integer n
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        explicit CBigNum(uint256_t n);


        /** CBigNum
         *
         *  construct class from unsigned integer n
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        explicit CBigNum(uint512_t n);


        /** CBigNum
         *
         *  construct class from unsigned integer n
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        explicit CBigNum(uint576_t n);


        /** CBigNum
         *
         *  construct class from unsigned integer n
         *
         *  @param[in] n number to construct bignum with
         *setuint32’ was not declared in this scope
             setuint32(n);

         **/
        explicit CBigNum(uint1024_t n);


        /** CBigNum
         *
         *  construct class from a byte array
         *
         *  @param[in] n number to construct bignum with
         *
         **/
        explicit CBigNum(const std::vector<uint8_t>& vch);


        /** getBN
         *
         *  get a pointer to the underlying BIGNUM struct
         *
         *  @return a pointer to the BIGNUM struct
         *
         **/
        BIGNUM* getBN() const;


        /** getuint32
         *
         *
         *
         *
         **/
        uint32_t getuint32() const;


        /** getuint64
         *
         *
         *
         *
         **/
        uint64_t getuint64() const;


        /** getuint256
         *
         *
         *
         *
         **/
        uint256_t getuint256() const;


        /** getuint512
         *
         *
         *
         *
         **/
        uint512_t getuint512() const;


        /** getuint576
         *
         *
         *
         *
         **/
        uint576_t getuint576() const;


        /** getuint1024
         *
         *
         *
         *
         **/
        uint1024_t getuint1024() const;


        /** getint32
         *
         *
         *
         *
         **/
        int32_t getint32() const;


        /** setint64
         *
         *
         *
         *
         **/
        void setint64(int64_t n);


        /** setuint32
         *
         *
         *
         *
         **/
        void setuint32(uint32_t n);


        /** setuint64
         *
         *
         *
         *
         **/
        void setuint64(uint64_t n);


        /** setuint256
         *
         *
         *
         *
         **/
        void setuint256(uint256_t n);


        /** setuint512
         *
         *
         *
         *
         **/
        void setuint512(uint512_t n);


        /** setuint576
         *
         *
         *
         *
         **/
        void setuint576(uint576_t n);


        /** setuint1024
         *
         *
         *
         *
         **/
        void setuint1024(uint1024_t n);


        /** setvch
         *
         *
         *
         *
         **/
        void setvch(const std::vector<uint8_t>& vch);


        /** getvch
         *
         *
         *
         *
         **/
        std::vector<uint8_t> getvch() const;


        /** SetCompact
         *
         *
         *
         *
         **/
        CBigNum& SetCompact(uint32_t nCompact);


        /** GetCompact
         *
         *
         *
         *
         **/
        uint32_t GetCompact() const;


        /** SetHex
         *
         *
         *
         *
         **/
        void SetHex(const std::string& str);


        /** ToString
         *
         *
         *
         *
         **/
        std::string ToString(uint32_t nBase=10) const;


        /** GetHex
         *
         *
         *
         *
         **/
        std::string GetHex() const;


        /** operator!
         *
         *
         *
         *
         **/
        bool operator!() const;


        /** operator+=
         *
         *
         *
         *
         **/
        CBigNum& operator+=(const CBigNum& b);


        /** operator-=
         *
         *
         *
         *
         **/
        CBigNum& operator-=(const CBigNum& b);


        /** operator*=
         *
         *
         *
         *
         **/
        CBigNum& operator*=(const CBigNum& b);


        /** operator/=
         *
         *
         *
         *
         **/
        CBigNum& operator/=(const CBigNum& b);


        /** operator%=
         *
         *
         *
         *
         **/
        CBigNum& operator%=(const CBigNum& b);


        /** operator<<=
         *
         *
         *
         *
         **/
        CBigNum& operator<<=(uint32_t shift);


        /**  operator>>=
         *
         *
         *
         *
         **/
        CBigNum& operator>>=(uint32_t shift);


        /** operator++
         *
         *
         *
         *
         **/
        CBigNum& operator++();


        /** operator--
         *
         *
         *
         *
         **/
        CBigNum& operator--();


        /** operator++
         *
         *
         *
         *
         **/
        const CBigNum operator++(int);


        /** operator--
         *
         *
         *
         *
         **/
        const CBigNum operator--(int);


        /* friend operator declarations */
        friend const CBigNum operator-(const CBigNum& a, const CBigNum& b);
        friend const CBigNum operator/(const CBigNum& a, const CBigNum& b);
        friend const CBigNum operator%(const CBigNum& a, const CBigNum& b);


        /* templated serialization function prototypes */
        uint32_t GetSerializeSize(uint32_t nSerType=0,
                                  uint32_t nVersion=LLP::PROTOCOL_VERSION) const;

        template<typename Stream>
        void Serialize(Stream& s,
                       uint32_t nSerType=0,
                       uint32_t nVersion=LLP::PROTOCOL_VERSION) const;

        template<typename Stream>
        void Unserialize(Stream& s,
                         uint32_t nSerType=0,
                         uint32_t nVersion=LLP::PROTOCOL_VERSION);

    private:

        /* internal bignum pointer */
        BIGNUM* m_BN = nullptr;


        /** allocate
         *
         *  obtain memory for the bignum class
         *
         **/
        void allocate();
    };


    /** operator+
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] b
     *
     **/
    const CBigNum operator+(const CBigNum& a, const CBigNum& b);


    /** operator-
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] b
     *
     **/
    const CBigNum operator-(const CBigNum& a, const CBigNum& b);


    /** operator-
     *
     *
     *
     *  @param[in] a
     *
     **/
    const CBigNum operator-(const CBigNum& a);


    /** operator*
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] b
     *
     **/
    const CBigNum operator*(const CBigNum& a, const CBigNum& b);


    /** operator/
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] b
     *
     **/
    const CBigNum operator/(const CBigNum& a, const CBigNum& b);


    /** operator%
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] b
     *
     **/
    const CBigNum operator%(const CBigNum& a, const CBigNum& b);


    /** operator<<
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] shift
     *
     **/
    const CBigNum operator<<(const CBigNum& a, uint32_t shift);


    /** operator>>
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] shift
     *
     **/
    const CBigNum operator>>(const CBigNum& a, uint32_t shift);


    /** operator==
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] b
     *
     **/
    bool operator==(const CBigNum& a, const CBigNum& b);


    /** operator!=
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] b
     *
     **/
    bool operator!=(const CBigNum& a, const CBigNum& b);


    /** operator<=
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] b
     *
     **/
    bool operator<=(const CBigNum& a, const CBigNum& b);


    /** operator>=
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] b
     *
     **/
    bool operator>=(const CBigNum& a, const CBigNum& b);


    /** operator<
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] b
     *
     **/
    bool operator<(const CBigNum& a, const CBigNum& b);


    /** operator>
     *
     *
     *
     *  @param[in] a
     *
     *  @param[in] b
     *
     **/
    bool operator>(const CBigNum& a, const CBigNum& b);

}

#endif
