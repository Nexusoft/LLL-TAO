/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLC_TYPES_UINT1024_H
#define NEXUS_LLC_TYPES_UINT1024_H

#include <LLC/types/base_uint.h>



/** 192-bit integer */
class uint192_t : public base_uint<192>
{
public:

    uint192_t()
    : base_uint<192>()
    {
    }

    uint192_t(const  base_uint<192>& b)
    {
        for (int i = 0; i < WIDTH; ++i)
            pn[i] = b.pn[i];
    }

    uint192_t& operator=(const  base_uint<192>& b)
    {
        for (int i = 0; i < WIDTH; ++i)
            pn[i] = b.pn[i];
        return *this;
    }

    uint192_t(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; ++i)
            pn[i] = 0;
    }

    uint192_t& operator=(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; ++i)
            pn[i] = 0;
        return *this;
    }

    explicit uint192_t(const std::string& str)
    {
        SetHex(str);
    }

    explicit uint192_t(const std::vector<uint8_t>& vch)
    {
        if (vch.size() == sizeof(pn))
            //memcpy(pn, &vch[0], sizeof(pn));
            std::copy(&vch[0], &vch[0] + sizeof(pn), (uint8_t *)pn);
        else
            *this = 0;
    }
};



/** 256-bit integer */
class uint256_t : public base_uint<256>
{
public:

    uint256_t()
    : base_uint<256>()
    {
    }


    uint256_t(const base_uint<256>& b)
    {
        for (int i = 0; i < WIDTH; ++i)
            pn[i] = b.pn[i];
    }

    uint256_t(const uint192_t& b)
    {
        for (int i = 0; i < b.WIDTH; ++i)
            pn[i] = b.pn[i];
    }


    uint256_t& operator=(const base_uint<256>& b)
    {
        for (int i = 0; i < WIDTH; ++i)
            pn[i] = b.pn[i];
        return *this;
    }


    uint256_t(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; ++i)
            pn[i] = 0;
    }

    uint192_t getuint128() const
    {
        uint192_t b;
        for (int i = 0; i < b.WIDTH; ++i)
            b.pn[i] = pn[i];

        return b;
    }

    uint256_t& operator=(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; ++i)
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
            std::copy(&vch[0], &vch[0] + sizeof(pn), (uint8_t *)pn);
        else
            *this = 0;
    }
};



/** 512-bit integer */
class uint512_t : public base_uint<512>
{
public:

    uint512_t()
    : base_uint<512>()
    {
    }


    uint512_t(const base_uint<512>& b)
    {
        for (int i = 0; i < WIDTH; ++i)
            pn[i] = b.pn[i];
    }


    uint512_t& operator=(const base_uint<512>& b)
    {
        for (int i = 0; i < WIDTH; ++i)
            pn[i] = b.pn[i];
        return *this;
    }


    uint512_t(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; ++i)
            pn[i] = 0;
    }


    uint512_t& operator=(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; ++i)
            pn[i] = 0;
        return *this;
    }


    explicit uint512_t(const std::vector<uint8_t>& vch)
    {
        if (vch.size() == sizeof(pn))
            std::copy(&vch[0], &vch[0] + sizeof(pn), (uint8_t *)pn);
        else
            *this = 0;
    }


    explicit uint512_t(const std::string& str)
    {
        SetHex(str);
    }

};


/** 576-bit integer */
class uint576_t : public base_uint<576>
{
public:

    uint576_t()
    : base_uint<576>()
    {
    }


    uint576_t(const base_uint<576>& b)
    {
        for (int i = 0; i < WIDTH; ++i)
            pn[i] = b.pn[i];
    }


    uint576_t& operator=(const base_uint<576>& b)
    {
        for (int i = 0; i < WIDTH; ++i)
            pn[i] = b.pn[i];
        return *this;
    }


    uint576_t(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; ++i)
            pn[i] = 0;
    }


    uint576_t& operator=(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; ++i)
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
            std::copy(&vch[0], &vch[0] + sizeof(pn), (uint8_t *)pn);
        else
            *this = 0;
    }
};


/** 1024-bit integer */
class uint1024_t : public base_uint<1024>
{
public:

    uint1024_t()
    : base_uint<1024>()
    {
    }


    uint1024_t(const base_uint<1024>& b)
    {
        for (int i = 0; i < WIDTH; ++i)
            pn[i] = b.pn[i];
    }


    uint1024_t& operator=(const base_uint<1024>& b)
    {
        for (int i = 0; i < WIDTH; ++i)
            pn[i] = b.pn[i];
        return *this;
    }


    uint1024_t(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; ++i)
            pn[i] = 0;
    }


    uint1024_t& operator=(uint64_t b)
    {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; ++i)
            pn[i] = 0;
        return *this;
    }


    uint1024_t(uint256_t b)
    {
        for (int i = 0; i < WIDTH; ++i)
            if(i < b.WIDTH)
                pn[i] = b.pn[i];
            else
                pn[i] = 0;
    }


    uint1024_t& operator=(uint256_t b)
    {
        for (int i = 0; i < WIDTH; ++i)
            if(i < b.WIDTH)
                pn[i] = b.pn[i];
            else
                pn[i] = 0;

        return *this;
    }


    uint1024_t(uint512_t b)
    {
        for (int i = 0; i < WIDTH; ++i)
            if(i < b.WIDTH)
                pn[i] = b.pn[i];
            else
                pn[i] = 0;
    }


    uint1024_t& operator=(uint512_t b)
    {
        for (int i = 0; i < WIDTH; ++i)
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
        for (int i = 0; i < b.WIDTH; ++i)
            b.pn[i] = pn[i];

        return b;
    }


    /** This method should only be used to retrieve an uint512_t when stored inside an uint1024_t.
        This is necessary for the inventory system to function with both a 1024 bit block
        and 512 bit transaction. */
    uint512_t getuint512() const
    {
        uint512_t b;
        for (int i = 0; i < b.WIDTH; ++i)
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
            std::copy(&vch[0], &vch[0] + sizeof(pn), (uint8_t *)pn);
        else
            *this = 0;
    }
};



#endif
