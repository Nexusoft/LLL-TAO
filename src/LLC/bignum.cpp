/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/bignum.h>
#include <Util/templates/serialize.h>
#include <openssl/bn.h>
#include <algorithm>

namespace LLC
{

    bignum_error::bignum_error(const std::string& str)
    : std::runtime_error(str) { }


    BN_CTX* CAutoBN_CTX::operator=(BN_CTX* pnew)
    {
        return pctx = pnew;
    }

    CAutoBN_CTX::CAutoBN_CTX()
    {
        pctx = BN_CTX_new();
        if (pctx == nullptr)
            throw bignum_error("CAutoBN_CTX : BN_CTX_new() returned nullptr");
    }

    CAutoBN_CTX::~CAutoBN_CTX()
    {
        if (pctx != nullptr)
            BN_CTX_free(pctx);
    }

    CAutoBN_CTX::operator BN_CTX*()
    {
        return pctx;
    }

    BN_CTX& CAutoBN_CTX::operator*()
    {
        return *pctx;
    }

    BN_CTX** CAutoBN_CTX::operator&()
    {
        return &pctx;
    }

    bool CAutoBN_CTX::operator!()
    {
        return (pctx == nullptr);
    }


    CBigNum::CBigNum()
    {
        allocate();
    }

    CBigNum::CBigNum(const CBigNum& b)
    : m_BN(BN_dup(b.getBN()))
    {
        if (nullptr == m_BN)
        {
            throw bignum_error("CBigNum::CBigNum(const CBigNum&): BN_dup failed");
        }
    }

    CBigNum& CBigNum::operator=(const CBigNum& b)
    {
        if (nullptr == BN_copy(m_BN, b.getBN()))
        {
            throw bignum_error("CBigNum::operator=(const CBigNum&): BN_copy failed");
        }
        return (*this);
    }

    CBigNum::~CBigNum()
    {
        BN_clear_free(m_BN);
    }


    CBigNum::CBigNum(int8_t n)
    {
        allocate();
        if (n >= 0)
            setuint32(n);
        else
            setint64(n);
    }


    CBigNum::CBigNum(int16_t n)
    {
        allocate();
        if (n >= 0)
            setuint32(n);
        else
            setint64(n);
    }

    void CBigNum::setuint32(uint32_t n)
    {
        if (!BN_set_word(m_BN, n))
            throw bignum_error("CBigNum conversion from uint32_t : BN_set_word failed");
    }

    CBigNum::CBigNum(int32_t n)
    {
        allocate();
        if (n >= 0)
            setuint32(n);
        else
            setint64(n);
    }


    CBigNum::CBigNum(int64_t n)
    {
        allocate();
        setint64(n);
    }

    CBigNum::CBigNum(uint8_t n)
    {
        allocate();
        setuint32(n);
    }

    CBigNum::CBigNum(uint16_t n)
    {
        allocate();
        setuint32(n);
    }

    CBigNum::CBigNum(uint32_t n)
    {
        allocate();
        setuint32(n);
    }


    CBigNum::CBigNum(uint64_t n)
    {
        allocate();
        setuint64(n);
    }

    CBigNum::CBigNum(uint256_t n)
    {
        allocate();
        setuint256(n);
    }

    CBigNum::CBigNum(uint512_t n)
    {
        allocate();
        setuint512(n);
    }

    CBigNum::CBigNum(uint576_t n)
    {
        allocate();
        setuint576(n);
    }

    CBigNum::CBigNum(uint1024_t n)
    {
        allocate();
        setuint1024(n);
    }

    CBigNum::CBigNum(const std::vector<uint8_t>& vch)
    {
        allocate();
        setvch(vch);
    }

    BIGNUM* CBigNum::getBN() const
    {
        return m_BN;
    }


    uint32_t CBigNum::getuint32() const
    {
        return BN_get_word(m_BN);
    }


    int32_t CBigNum::getint32() const
    {
        unsigned long n = BN_get_word(m_BN);
        if (!BN_is_negative(m_BN))
            return (n > (unsigned long)std::numeric_limits<int32_t>::max() ? std::numeric_limits<int32_t>::max() : n);
        else
            return (n > (unsigned long)std::numeric_limits<int32_t>::max() ? std::numeric_limits<int32_t>::min() : -(int32_t)n);
    }

    void CBigNum::setint64(int64_t n)
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
        for (uint32_t i = 0; i < 8; i++)
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
        BN_mpi2bn(pch, p - pch, m_BN);
    }

    void CBigNum::setuint64(uint64_t n)
    {
        uint8_t pch[sizeof(n) + 6];
        uint8_t* p = pch + 4;
        bool fLeadingZeroes = true;
        for (uint32_t i = 0; i < 8; i++)
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
        BN_mpi2bn(pch, p - pch, m_BN);
    }

    uint64_t CBigNum::getuint64() const
    {
        uint32_t nSize = BN_bn2mpi(m_BN, nullptr);
        if (nSize < 4)
            return 0;
        std::vector<uint8_t> vch(nSize);
        BN_bn2mpi(m_BN, &vch[0]);
        if (vch.size() > 4)
            vch[4] &= 0x7f;
        uint64_t n = 0;
        for (uint32_t i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
            ((uint8_t*)&n)[i] = vch[j];
        return n;
    }

    void CBigNum::setuint256(uint256_t n)
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
        BN_mpi2bn(pch, p - pch, m_BN);
    }


    uint256_t CBigNum::getuint256() const
    {
        uint32_t nSize = BN_bn2mpi(m_BN, nullptr);
        if (nSize < 4)
            return 0;
        std::vector<uint8_t> vch(nSize);
        BN_bn2mpi(m_BN, &vch[0]);
        if (vch.size() > 4)
            vch[4] &= 0x7f;
        uint256_t n = 0;
        for (uint32_t i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
            ((uint8_t*)&n)[i] = vch[j];
        return n;
    }


    void CBigNum::setuint512(uint512_t n)
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
        BN_mpi2bn(pch, p - pch, m_BN);
    }


    uint512_t CBigNum::getuint512() const
    {
        uint32_t nSize = BN_bn2mpi(m_BN, nullptr);
        if (nSize < 4)
            return 0;
        std::vector<uint8_t> vch(nSize);
        BN_bn2mpi(m_BN, &vch[0]);
        if (vch.size() > 4)
            vch[4] &= 0x7f;
        uint512_t n = 0;
        for (uint32_t i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
            ((uint8_t*)&n)[i] = vch[j];
        return n;
    }


    void CBigNum::setuint576(uint576_t n)
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
        BN_mpi2bn(pch, p - pch, m_BN);
    }


    uint576_t CBigNum::getuint576() const
    {
        uint32_t nSize = BN_bn2mpi(m_BN, nullptr);
        if (nSize < 4)
            return 0;
        std::vector<uint8_t> vch(nSize);
        BN_bn2mpi(m_BN, &vch[0]);
        if (vch.size() > 4)
            vch[4] &= 0x7f;
        uint576_t n = 0;
        for (uint32_t i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
            ((uint8_t*)&n)[i] = vch[j];
        return n;
    }


    void CBigNum::setuint1024(uint1024_t n)
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
        BN_mpi2bn(pch, p - pch, m_BN);
    }


    uint1024_t CBigNum::getuint1024() const
    {
        uint32_t nSize = BN_bn2mpi(m_BN, nullptr);
        if (nSize < 4)
            return 0;
        std::vector<uint8_t> vch(nSize);
        BN_bn2mpi(m_BN, &vch[0]);
        if (vch.size() > 4)
            vch[4] &= 0x7f;
        uint1024_t n = 0;
        for (uint32_t i = 0, j = vch.size()-1; i < sizeof(n) && j >= 4; i++, j--)
            ((uint8_t*)&n)[i] = vch[j];
        return n;
    }


    void CBigNum::setvch(const std::vector<uint8_t>& vch)
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
        std::reverse_copy(vch.begin(), vch.end(), vch2.begin() + 4);
        BN_mpi2bn(&vch2[0], vch2.size(), m_BN);
    }


    std::vector<uint8_t> CBigNum::getvch() const
    {
        uint32_t nSize = BN_bn2mpi(m_BN, nullptr);
        if (nSize <= 4)
            return std::vector<uint8_t>();
        std::vector<uint8_t> vch(nSize);
        BN_bn2mpi(m_BN, &vch[0]);
        vch.erase(vch.begin(), vch.begin() + 4);
        std::reverse(vch.begin(), vch.end());
        return vch;
    }


    CBigNum& CBigNum::SetCompact(uint32_t nCompact)
    {
        uint32_t nSize = nCompact >> 24;
        std::vector<uint8_t> vch(4 + nSize);
        vch[3] = nSize;
        if (nSize >= 1) vch[4] = (nCompact >> 16) & 0xff;
        if (nSize >= 2) vch[5] = (nCompact >> 8) & 0xff;
        if (nSize >= 3) vch[6] = (nCompact >> 0) & 0xff;
        BN_mpi2bn(&vch[0], vch.size(), m_BN);
        return *this;
    }


    uint32_t CBigNum::GetCompact() const
    {
        uint32_t nSize = BN_bn2mpi(m_BN, nullptr);
        std::vector<uint8_t> vch(nSize);
        nSize -= 4;
        BN_bn2mpi(m_BN, &vch[0]);
        uint32_t nCompact = nSize << 24;
        if (nSize >= 1) nCompact |= (vch[4] << 16);
        if (nSize >= 2) nCompact |= (vch[5] << 8);
        if (nSize >= 3) nCompact |= (vch[6] << 0);
        return nCompact;
    }


    void CBigNum::SetHex(const std::string& str)
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
            uint32_t n = phexdigit[(uint8_t)*psz++];
            *this += n;
        }
        if (fNegative)
            *this = 0 - *this;
    }


    std::string CBigNum::ToString(uint32_t nBase) const
    {
        CAutoBN_CTX pctx;
        CBigNum bnBase = nBase;
        CBigNum bn0 = 0;
        std::string str;
        CBigNum bn = *this;
        BN_set_negative(bn.getBN(), false);
        CBigNum dv;
        CBigNum rem;
        if (BN_cmp(bn.getBN(), bn0.getBN()) == 0)
            return "0";
        while (BN_cmp(bn.getBN(), bn0.getBN()) > 0)
        {
            if (!BN_div(dv.getBN(), rem.getBN(), bn.getBN(), bnBase.getBN(), pctx))
                throw bignum_error("CBigNum::ToString() : BN_div failed");
            bn = dv;
            uint32_t c = rem.getuint32();
            str += "0123456789abcdef"[c];
        }
        if (BN_is_negative(m_BN))
            str += "-";
        std::reverse(str.begin(), str.end());
        return str;
    }


    std::string CBigNum::GetHex() const
    {
        return ToString(16);
    }


    uint32_t CBigNum::GetSerializeSize(uint32_t nSerType, uint32_t nVersion) const
    {
        return ::GetSerializeSize(getvch(), nSerType, nVersion);
    }

    template<typename Stream>
    void CBigNum::Serialize(Stream& s, uint32_t nSerType, uint32_t nVersion) const
    {
        ::Serialize(s, getvch(), nSerType, nVersion);
    }

    template<typename Stream>
    void CBigNum::Unserialize(Stream& s, uint32_t nSerType, uint32_t nVersion)
    {
        std::vector<uint8_t> vch;
        ::Unserialize(s, vch, nSerType, nVersion);
        setvch(vch);
    }

    bool CBigNum::operator!() const
    {
        return BN_is_zero(m_BN);
    }


    CBigNum& CBigNum::operator+=(const CBigNum& b)
    {
        if (!BN_add(m_BN, m_BN, b.getBN()))
            throw bignum_error("CBigNum::operator+= : BN_add failed");
        return *this;
    }


    CBigNum& CBigNum::operator-=(const CBigNum& b)
    {
        *this = *this - b;
        return *this;
    }


    CBigNum& CBigNum::operator*=(const CBigNum& b)
    {
        CAutoBN_CTX pctx;
        if (!BN_mul(m_BN, m_BN, b.getBN(), pctx))
            throw bignum_error("CBigNum::operator*= : BN_mul failed");
        return *this;
    }


    CBigNum& CBigNum::operator/=(const CBigNum& b)
    {
        *this = *this / b;
        return *this;
    }


    CBigNum& CBigNum::operator%=(const CBigNum& b)
    {
        *this = *this % b;
        return *this;
    }


    CBigNum& CBigNum::operator<<=(uint32_t shift)
    {
        if (!BN_lshift(m_BN, m_BN, shift))
            throw bignum_error("CBigNum:operator<<= : BN_lshift failed");
        return *this;
    }


    CBigNum& CBigNum::operator>>=(uint32_t shift)
    {
        // Note: BN_rshift segfaults on 64-bit if 2^shift is greater than the number
        //   if built on ubuntu 9.04 or 9.10, probably depends on version of openssl
        CBigNum a = 1;
        a <<= shift;
        if (BN_cmp(a.getBN(), m_BN) > 0)
        {
            *this = 0;
            return *this;
        }

        if (!BN_rshift(m_BN, m_BN, shift))
            throw bignum_error("CBigNum:operator>>= : BN_rshift failed");
        return *this;
    }


    CBigNum& CBigNum::operator++()
    {
        // prefix operator
        if (!BN_add(m_BN, m_BN, BN_value_one()))
            throw bignum_error("CBigNum::operator++ : BN_add failed");
        return *this;
    }


    const CBigNum CBigNum::operator++(int)
    {
        // postfix operator
        const CBigNum ret = *this;
        ++(*this);
        return ret;
    }


    CBigNum& CBigNum::operator--()
    {
        // prefix operator
        CBigNum r;
        if (!BN_sub(r.getBN(), m_BN, BN_value_one()))
            throw bignum_error("CBigNum::operator-- : BN_sub failed");
        *this = r;
        return *this;
    }


    const CBigNum CBigNum::operator--(int)
    {
        // postfix operator
        const CBigNum ret = *this;
        --(*this);
        return ret;
    }

    void CBigNum::allocate()
    {
        m_BN = BN_new();
        if(nullptr == m_BN)
        {
            throw bignum_error("CBigNum::allocate(): failed to allocate BIGNUM");
        }
    }


    const CBigNum operator+(const CBigNum& a, const CBigNum& b)
    {
        CBigNum r;
        if (!BN_add(r.getBN(), a.getBN(), b.getBN()))
            throw bignum_error("CBigNum::operator+ : BN_add failed");
        return r;
    }

    const CBigNum operator-(const CBigNum& a, const CBigNum& b)
    {
        CBigNum r;
        if (!BN_sub(r.getBN(), a.getBN(), b.getBN()))
            throw bignum_error("CBigNum::operator- : BN_sub failed");
        return r;
    }

    const CBigNum operator-(const CBigNum& a)
    {
        CBigNum r(a);
        BN_set_negative(r.getBN(), !BN_is_negative(r.getBN()));
        return r;
    }

    const CBigNum operator*(const CBigNum& a, const CBigNum& b)
    {
        CAutoBN_CTX pctx;
        CBigNum r;
        if (!BN_mul(r.getBN(), a.getBN(), b.getBN(), pctx))
            throw bignum_error("CBigNum::operator* : BN_mul failed");
        return r;
    }

    const CBigNum operator/(const CBigNum& a, const CBigNum& b)
    {
        CAutoBN_CTX pctx;
        CBigNum r;
        if (!BN_div(r.getBN(), nullptr, a.getBN(), b.getBN(), pctx))
            throw bignum_error("CBigNum::operator/ : BN_div failed");
        return r;
    }

    const CBigNum operator%(const CBigNum& a, const CBigNum& b)
    {
        CAutoBN_CTX pctx;
        CBigNum r;
        if (!BN_mod(r.getBN(), a.getBN(), b.getBN(), pctx))
            throw bignum_error("CBigNum::operator% : BN_mod failed");
        return r;
    }

    const CBigNum operator<<(const CBigNum& a, uint32_t shift)
    {
        CBigNum r;
        if (!BN_lshift(r.getBN(), a.getBN(), shift))
            throw bignum_error("CBigNum:operator<< : BN_lshift failed");
        return r;
    }

    const CBigNum operator>>(const CBigNum& a, uint32_t shift)
    {
        CBigNum r = a;
        r >>= shift;
        return r;
    }

    bool operator==(const CBigNum& a, const CBigNum& b)
    {
        return (BN_cmp(a.getBN(), b.getBN()) == 0);
    }

    bool operator!=(const CBigNum& a, const CBigNum& b)
    {
        return (BN_cmp(a.getBN(), b.getBN()) != 0);
    }

    bool operator<=(const CBigNum& a, const CBigNum& b)
    {
        return (BN_cmp(a.getBN(), b.getBN()) <= 0);
    }

    bool operator>=(const CBigNum& a, const CBigNum& b)
    {
        return (BN_cmp(a.getBN(), b.getBN()) >= 0);
    }

    bool operator<(const CBigNum& a, const CBigNum& b)
    {
        return (BN_cmp(a.getBN(), b.getBN()) < 0);
    }

    bool operator>(const CBigNum& a, const CBigNum& b)
    {
        return (BN_cmp(a.getBN(), b.getBN()) > 0);
    }

}
