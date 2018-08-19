/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2017] ++

			(c) Copyright The Nexus Developers 2014 - 2017

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"fides in stellis, virtus in numeris" - Faith in the Stars, Power in Numbers

____________________________________________________________________________________________*/

#ifndef NEXUS_LLC_TYPES_BIGNUM_H
#define NEXUS_LLC_TYPES_BIGNUM_H

#include <stdexcept>
#include <vector>
#include <openssl/bn.h>


namespace LLC
{

    /** Errors thrown by the bignum class */
    class bignum_error : public std::runtime_error
    {
    public:
        explicit bignum_error(const std::string& str) : std::runtime_error(str) {}
    };


    /** RAII encapsulated BN_CTX (OpenSSL bignum context) */
    class CAutoBN_CTX
    {
    protected:
        BN_CTX* pctx;
        BN_CTX* operator=(BN_CTX* pnew) { return pctx = pnew; }

    public:
        CAutoBN_CTX()
        {
            pctx = BN_CTX_new();
            if (pctx == NULL)
                throw bignum_error("CAutoBN_CTX : BN_CTX_new() returned NULL");
        }

        ~CAutoBN_CTX()
        {
            if (pctx != NULL)
                BN_CTX_free(pctx);
        }

        operator BN_CTX*() { return pctx; }
        BN_CTX& operator*() { return *pctx; }
        BN_CTX** operator&() { return &pctx; }
        bool operator!() { return (pctx == NULL); }
    };


    /** C++ wrapper for BIGNUM (OpenSSL bignum) */
    class CBigNum : public BIGNUM
    {
    public:
        CBigNum()
        {
            BN_init(this);
        }

        CBigNum(const CBigNum& b)
        {
            BN_init(this);
            if (!BN_copy(this, &b))
            {
                BN_clear_free(this);
                throw bignum_error("CBigNum::CBigNum(const CBigNum&) : BN_copy failed");
            }
        }

        CBigNum& operator=(const CBigNum& b)
        {
            if (!BN_copy(this, &b))
                throw bignum_error("CBigNum::operator= : BN_copy failed");
            return (*this);
        }

        ~CBigNum()
        {
            BN_clear_free(this);
        }

        //CBigNum(char n) is not portable.  Use 'signed char' or 'uint8_t'.
        CBigNum(signed char n)      { BN_init(this); if (n >= 0) setulong(n); else setint64(n); }
        CBigNum(signed short n)            { BN_init(this); if (n >= 0) setulong(n); else setint64(n); }
        CBigNum(signed int n)              { BN_init(this); if (n >= 0) setulong(n); else setint64(n); }
        //CBigNum(long n)             { BN_init(this); if (n >= 0) setulong(n); else setint64(n); }
        CBigNum(int64_t n)            { BN_init(this); setint64(n); }
        CBigNum(uint8_t n)    { BN_init(this); setulong(n); }
        CBigNum(uint16_t n)   { BN_init(this); setulong(n); }
        CBigNum(uint32_t n)     { BN_init(this); setulong(n); }
        //CBigNum(unsigned long n)    { BN_init(this); setulong(n); }
        CBigNum(uint64_t n)           { BN_init(this); setuint64(n); }
        explicit CBigNum(uint256 n) { BN_init(this); setuint256(n); }
    	explicit CBigNum(uint512 n) { BN_init(this); setuint512(n); }
    	explicit CBigNum(uint576 n) { BN_init(this); setuint576(n); }
    	explicit CBigNum(uint1024 n) { BN_init(this); setuint1024(n); }

        explicit CBigNum(const std::vector<uint8_t>& vch)
        {
            BN_init(this);
            setvch(vch);
        }

        void setulong(unsigned long n)
        {
            if (!BN_set_word(this, n))
                throw bignum_error("CBigNum conversion from unsigned long : BN_set_word failed");
        }

        unsigned long getulong() const
        {
            return BN_get_word(this);
        }

        uint32_t getuint() const
        {
            return BN_get_word(this);
        }

        int getint() const
        {
            unsigned long n = BN_get_word(this);
            if (!BN_is_negative(this))
                return (n > (unsigned long)std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : n);
            else
                return (n > (unsigned long)std::numeric_limits<int>::max() ? std::numeric_limits<int>::min() : -(int)n);
        }

        void setint64(int64_t n)
        {
            uint8_t pch[sizeof(n) + 6];
            uint8_t* p = pch + 4;
            bool fNegative = false;
            if (n < (int64_t)0)
            {
                n = -n;
                fNegative = true;
            }
            bool fLeadingZeroes = true;
            for (int i = 0; i < 8; i++)
            {
                uint8_t c = (n >> 56) & 0xff;
                n <<= 8;
                if (fLeadingZeroes)
                {
                    if (c == 0)
                        continue;
                    if (c & 0x80)
                        *p++ = (fNegative ? 0x80 : 0);
                    else if (fNegative)
                        c |= 0x80;
                    fLeadingZeroes = false;
                }
                *p++ = c;
            }
            uint32_t nSize = p - (pch + 4);
            pch[0] = (nSize >> 24) & 0xff;
            pch[1] = (nSize >> 16) & 0xff;
            pch[2] = (nSize >> 8) & 0xff;
            pch[3] = (nSize) & 0xff;
            BN_mpi2bn(pch, p - pch, this);
        }

        void setuint64(uint64_t n)
        {
            uint8_t pch[sizeof(n) + 6];
            uint8_t* p = pch + 4;
            bool fLeadingZeroes = true;
            for (int i = 0; i < 8; i++)
            {
                uint8_t c = (n >> 56) & 0xff;
                n <<= 8;
                if (fLeadingZeroes)
                {
                    if (c == 0)
                        continue;
                    if (c & 0x80)
                        *p++ = 0;
                    fLeadingZeroes = false;
                }
                *p++ = c;
            }
            uint32_t nSize = p - (pch + 4);
            pch[0] = (nSize >> 24) & 0xff;
            pch[1] = (nSize >> 16) & 0xff;
            pch[2] = (nSize >> 8) & 0xff;
            pch[3] = (nSize) & 0xff;
            BN_mpi2bn(pch, p - pch, this);
        }

        uint64_t getuint64()
        {
            uint32_t nSize = BN_bn2mpi(this, NULL);
            if (nSize < 4)
                return 0;
            std::vector<uint8_t> vch(nSize);
            BN_bn2mpi(this, &vch[0]);
            if (vch.size() > 4)
                vch[4] &= 0x7f;
            uint64_t n = 0;
            for (int i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
                ((uint8_t*)&n)[i] = vch[j];
            return n;
        }

        void setuint256(uint256 n)
        {
            uint8_t pch[sizeof(n) + 6];
            uint8_t* p = pch + 4;
            bool fLeadingZeroes = true;
            uint8_t* pbegin = (uint8_t*)&n;
            uint8_t* psrc = pbegin + sizeof(n);
            while (psrc != pbegin)
            {
                uint8_t c = *(--psrc);
                if (fLeadingZeroes)
                {
                    if (c == 0)
                        continue;
                    if (c & 0x80)
                        *p++ = 0;
                    fLeadingZeroes = false;
                }
                *p++ = c;
            }
            uint32_t nSize = p - (pch + 4);
            pch[0] = (nSize >> 24) & 0xff;
            pch[1] = (nSize >> 16) & 0xff;
            pch[2] = (nSize >> 8) & 0xff;
            pch[3] = (nSize >> 0) & 0xff;
            BN_mpi2bn(pch, p - pch, this);
        }

        uint256 getuint256()
        {
            uint32_t nSize = BN_bn2mpi(this, NULL);
            if (nSize < 4)
                return 0;
            std::vector<uint8_t> vch(nSize);
            BN_bn2mpi(this, &vch[0]);
            if (vch.size() > 4)
                vch[4] &= 0x7f;
            uint256 n = 0;
            for (uint32_t i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
                ((uint8_t*)&n)[i] = vch[j];
            return n;
        }

    	void setuint512(uint512 n)
        {
            uint8_t pch[sizeof(n) + 6];
            uint8_t* p = pch + 4;
            bool fLeadingZeroes = true;
            uint8_t* pbegin = (uint8_t*)&n;
            uint8_t* psrc = pbegin + sizeof(n);
            while (psrc != pbegin)
            {
                uint8_t c = *(--psrc);
                if (fLeadingZeroes)
                {
                    if (c == 0)
                        continue;
                    if (c & 0x80)
                        *p++ = 0;
                    fLeadingZeroes = false;
                }
                *p++ = c;
            }
            uint32_t nSize = p - (pch + 4);
            pch[0] = (nSize >> 24) & 0xff;
            pch[1] = (nSize >> 16) & 0xff;
            pch[2] = (nSize >> 8) & 0xff;
            pch[3] = (nSize >> 0) & 0xff;
            BN_mpi2bn(pch, p - pch, this);
        }

        uint512 getuint512()
        {
            uint32_t nSize = BN_bn2mpi(this, NULL);
            if (nSize < 4)
                return 0;
            std::vector<uint8_t> vch(nSize);
            BN_bn2mpi(this, &vch[0]);
            if (vch.size() > 4)
                vch[4] &= 0x7f;
            uint512 n = 0;
            for (uint32_t i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
                ((uint8_t*)&n)[i] = vch[j];
            return n;
        }

    	void setuint576(uint576 n)
        {
            uint8_t pch[sizeof(n) + 6];
            uint8_t* p = pch + 4;
            bool fLeadingZeroes = true;
            uint8_t* pbegin = (uint8_t*)&n;
            uint8_t* psrc = pbegin + sizeof(n);
            while (psrc != pbegin)
            {
                uint8_t c = *(--psrc);
                if (fLeadingZeroes)
                {
                    if (c == 0)
                        continue;
                    if (c & 0x80)
                        *p++ = 0;
                    fLeadingZeroes = false;
                }
                *p++ = c;
            }
            uint32_t nSize = p - (pch + 4);
            pch[0] = (nSize >> 24) & 0xff;
            pch[1] = (nSize >> 16) & 0xff;
            pch[2] = (nSize >> 8) & 0xff;
            pch[3] = (nSize >> 0) & 0xff;
            BN_mpi2bn(pch, p - pch, this);
        }

        uint576 getuint576()
        {
            uint32_t nSize = BN_bn2mpi(this, NULL);
            if (nSize < 4)
                return 0;
            std::vector<uint8_t> vch(nSize);
            BN_bn2mpi(this, &vch[0]);
            if (vch.size() > 4)
                vch[4] &= 0x7f;
            uint576 n = 0;
            for (uint32_t i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
                ((uint8_t*)&n)[i] = vch[j];
            return n;
        }

    	void setuint1024(uint1024 n)
        {
            uint8_t pch[sizeof(n) + 6];
            uint8_t* p = pch + 4;
            bool fLeadingZeroes = true;
            uint8_t* pbegin = (uint8_t*)&n;
            uint8_t* psrc = pbegin + sizeof(n);
            while (psrc != pbegin)
            {
                uint8_t c = *(--psrc);
                if (fLeadingZeroes)
                {
                    if (c == 0)
                        continue;
                    if (c & 0x80)
                        *p++ = 0;
                    fLeadingZeroes = false;
                }
                *p++ = c;
            }
            uint32_t nSize = p - (pch + 4);
            pch[0] = (nSize >> 24) & 0xff;
            pch[1] = (nSize >> 16) & 0xff;
            pch[2] = (nSize >> 8) & 0xff;
            pch[3] = (nSize >> 0) & 0xff;
            BN_mpi2bn(pch, p - pch, this);
        }

        uint1024 getuint1024()
        {
            uint32_t nSize = BN_bn2mpi(this, NULL);
            if (nSize < 4)
                return 0;
            std::vector<uint8_t> vch(nSize);
            BN_bn2mpi(this, &vch[0]);
            if (vch.size() > 4)
                vch[4] &= 0x7f;
            uint1024 n = 0;
            for (uint32_t i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
                ((uint8_t*)&n)[i] = vch[j];
            return n;
        }

        void setvch(const std::vector<uint8_t>& vch)
        {
            std::vector<uint8_t> vch2(vch.size() + 4);
            uint32_t nSize = vch.size();
            // BIGNUM's byte stream format expects 4 bytes of
            // big endian size data info at the front
            vch2[0] = (nSize >> 24) & 0xff;
            vch2[1] = (nSize >> 16) & 0xff;
            vch2[2] = (nSize >> 8) & 0xff;
            vch2[3] = (nSize >> 0) & 0xff;
            // swap data to big endian
            reverse_copy(vch.begin(), vch.end(), vch2.begin() + 4);
            BN_mpi2bn(&vch2[0], vch2.size(), this);
        }

        std::vector<uint8_t> getvch() const
        {
            uint32_t nSize = BN_bn2mpi(this, NULL);
            if (nSize <= 4)
                return std::vector<uint8_t>();
            std::vector<uint8_t> vch(nSize);
            BN_bn2mpi(this, &vch[0]);
            vch.erase(vch.begin(), vch.begin() + 4);
            reverse(vch.begin(), vch.end());
            return vch;
        }

        CBigNum& SetCompact(uint32_t nCompact)
        {
            uint32_t nSize = nCompact >> 24;
            std::vector<uint8_t> vch(4 + nSize);
            vch[3] = nSize;
            if (nSize >= 1) vch[4] = (nCompact >> 16) & 0xff;
            if (nSize >= 2) vch[5] = (nCompact >> 8) & 0xff;
            if (nSize >= 3) vch[6] = (nCompact >> 0) & 0xff;
            BN_mpi2bn(&vch[0], vch.size(), this);
            return *this;
        }

        uint32_t GetCompact() const
        {
            uint32_t nSize = BN_bn2mpi(this, NULL);
            std::vector<uint8_t> vch(nSize);
            nSize -= 4;
            BN_bn2mpi(this, &vch[0]);
            uint32_t nCompact = nSize << 24;
            if (nSize >= 1) nCompact |= (vch[4] << 16);
            if (nSize >= 2) nCompact |= (vch[5] << 8);
            if (nSize >= 3) nCompact |= (vch[6] << 0);
            return nCompact;
        }

        void SetHex(const std::string& str)
        {
            // skip 0x
            const char* psz = str.c_str();
            while (isspace(*psz))
                psz++;
            bool fNegative = false;
            if (*psz == '-')
            {
                fNegative = true;
                psz++;
            }
            if (psz[0] == '0' && tolower(psz[1]) == 'x')
                psz += 2;
            while (isspace(*psz))
                psz++;

            // hex string to bignum
            static signed char phexdigit[256] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0, 0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0xa,0xb,0xc,0xd,0xe,0xf,0,0,0,0,0,0,0,0,0 };
            *this = 0;
            while (isxdigit(*psz))
            {
                *this <<= 4;
                int n = phexdigit[(uint8_t)*psz++];
                *this += n;
            }
            if (fNegative)
                *this = 0 - *this;
        }

        std::string ToString(int nBase=10) const
        {
            CAutoBN_CTX pctx;
            CBigNum bnBase = nBase;
            CBigNum bn0 = 0;
            std::string str;
            CBigNum bn = *this;
            BN_set_negative(&bn, false);
            CBigNum dv;
            CBigNum rem;
            if (BN_cmp(&bn, &bn0) == 0)
                return "0";
            while (BN_cmp(&bn, &bn0) > 0)
            {
                if (!BN_div(&dv, &rem, &bn, &bnBase, pctx))
                    throw bignum_error("CBigNum::ToString() : BN_div failed");
                bn = dv;
                uint32_t c = rem.getulong();
                str += "0123456789abcdef"[c];
            }
            if (BN_is_negative(this))
                str += "-";
            reverse(str.begin(), str.end());
            return str;
        }

        std::string GetHex() const
        {
            return ToString(16);
        }

        uint32_t GetSerializeSize(int nType=0, int nVersion=SER_NETWORK) const
        {
            return ::GetSerializeSize(getvch(), nType, nVersion);
        }

        template<typename Stream>
        void Serialize(Stream& s, int nType=0, int nVersion=SER_NETWORK) const
        {
            ::Serialize(s, getvch(), nType, nVersion);
        }

        template<typename Stream>
        void Unserialize(Stream& s, int nType=0, int nVersion=SER_NETWORK)
        {
            std::vector<uint8_t> vch;
            ::Unserialize(s, vch, nType, nVersion);
            setvch(vch);
        }


        bool operator!() const
        {
            return BN_is_zero(this);
        }

        CBigNum& operator+=(const CBigNum& b)
        {
            if (!BN_add(this, this, &b))
                throw bignum_error("CBigNum::operator+= : BN_add failed");
            return *this;
        }

        CBigNum& operator-=(const CBigNum& b)
        {
            *this = *this - b;
            return *this;
        }

        CBigNum& operator*=(const CBigNum& b)
        {
            CAutoBN_CTX pctx;
            if (!BN_mul(this, this, &b, pctx))
                throw bignum_error("CBigNum::operator*= : BN_mul failed");
            return *this;
        }

        CBigNum& operator/=(const CBigNum& b)
        {
            *this = *this / b;
            return *this;
        }

        CBigNum& operator%=(const CBigNum& b)
        {
            *this = *this % b;
            return *this;
        }

        CBigNum& operator<<=(uint32_t shift)
        {
            if (!BN_lshift(this, this, shift))
                throw bignum_error("CBigNum:operator<<= : BN_lshift failed");
            return *this;
        }

        CBigNum& operator>>=(uint32_t shift)
        {
            // Note: BN_rshift segfaults on 64-bit if 2^shift is greater than the number
            //   if built on ubuntu 9.04 or 9.10, probably depends on version of openssl
            CBigNum a = 1;
            a <<= shift;
            if (BN_cmp(&a, this) > 0)
            {
                *this = 0;
                return *this;
            }

            if (!BN_rshift(this, this, shift))
                throw bignum_error("CBigNum:operator>>= : BN_rshift failed");
            return *this;
        }


        CBigNum& operator++()
        {
            // prefix operator
            if (!BN_add(this, this, BN_value_one()))
                throw bignum_error("CBigNum::operator++ : BN_add failed");
            return *this;
        }

        const CBigNum operator++(int)
        {
            // postfix operator
            const CBigNum ret = *this;
            ++(*this);
            return ret;
        }

        CBigNum& operator--()
        {
            // prefix operator
            CBigNum r;
            if (!BN_sub(&r, this, BN_value_one()))
                throw bignum_error("CBigNum::operator-- : BN_sub failed");
            *this = r;
            return *this;
        }

        const CBigNum operator--(int)
        {
            // postfix operator
            const CBigNum ret = *this;
            --(*this);
            return ret;
        }


        friend inline const CBigNum operator-(const CBigNum& a, const CBigNum& b);
        friend inline const CBigNum operator/(const CBigNum& a, const CBigNum& b);
        friend inline const CBigNum operator%(const CBigNum& a, const CBigNum& b);
    };



    inline const CBigNum operator+(const CBigNum& a, const CBigNum& b)
    {
        CBigNum r;
        if (!BN_add(&r, &a, &b))
            throw bignum_error("CBigNum::operator+ : BN_add failed");
        return r;
    }

    inline const CBigNum operator-(const CBigNum& a, const CBigNum& b)
    {
        CBigNum r;
        if (!BN_sub(&r, &a, &b))
            throw bignum_error("CBigNum::operator- : BN_sub failed");
        return r;
    }

    inline const CBigNum operator-(const CBigNum& a)
    {
        CBigNum r(a);
        BN_set_negative(&r, !BN_is_negative(&r));
        return r;
    }

    inline const CBigNum operator*(const CBigNum& a, const CBigNum& b)
    {
        CAutoBN_CTX pctx;
        CBigNum r;
        if (!BN_mul(&r, &a, &b, pctx))
            throw bignum_error("CBigNum::operator* : BN_mul failed");
        return r;
    }

    inline const CBigNum operator/(const CBigNum& a, const CBigNum& b)
    {
        CAutoBN_CTX pctx;
        CBigNum r;
        if (!BN_div(&r, NULL, &a, &b, pctx))
            throw bignum_error("CBigNum::operator/ : BN_div failed");
        return r;
    }

    inline const CBigNum operator%(const CBigNum& a, const CBigNum& b)
    {
        CAutoBN_CTX pctx;
        CBigNum r;
        if (!BN_mod(&r, &a, &b, pctx))
            throw bignum_error("CBigNum::operator% : BN_div failed");
        return r;
    }

    inline const CBigNum operator<<(const CBigNum& a, uint32_t shift)
    {
        CBigNum r;
        if (!BN_lshift(&r, &a, shift))
            throw bignum_error("CBigNum:operator<< : BN_lshift failed");
        return r;
    }

    inline const CBigNum operator>>(const CBigNum& a, uint32_t shift)
    {
        CBigNum r = a;
        r >>= shift;
        return r;
    }

    inline bool operator==(const CBigNum& a, const CBigNum& b) { return (BN_cmp(&a, &b) == 0); }
    inline bool operator!=(const CBigNum& a, const CBigNum& b) { return (BN_cmp(&a, &b) != 0); }
    inline bool operator<=(const CBigNum& a, const CBigNum& b) { return (BN_cmp(&a, &b) <= 0); }
    inline bool operator>=(const CBigNum& a, const CBigNum& b) { return (BN_cmp(&a, &b) >= 0); }
    inline bool operator<(const CBigNum& a, const CBigNum& b)  { return (BN_cmp(&a, &b) < 0); }
    inline bool operator>(const CBigNum& a, const CBigNum& b)  { return (BN_cmp(&a, &b) > 0); }

}

#endif
