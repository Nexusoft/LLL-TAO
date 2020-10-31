/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_TEMPLATES_BITARRAY_H
#define NEXUS_UTIL_TEMPLATES_BITARRAY_H

#include <cstdint>
#include <vector>


/** Bit Array Class
 *
 *  Class to act as a container for keys.
 *  This class operateds in O(1) for insert and search
 *
 *  It has an internal file handler that commits to disk when called.
 *
 **/
class BitArray
{
protected:

    /** is_set
     *
     *  Check if a particular bit is set in the bloom filter.
     *
     *  @param[in] nIndex The bucket to check bit for.
     *
     *  @return true if the bit is set.
     *
     **/
    bool is_set(const uint64_t nIndex) const
    {
        return (vRegisters[nIndex / 64] & (uint64_t(1) << (nIndex % 64)));
    }


    /** set_bit
     *
     *  Set a bit in the bloom filter at given bucket
     *
     *  @param[in] nIndex The bucket to set bit for.
     *
     **/
    void set_bit(const uint64_t nIndex)
    {
        vRegisters[nIndex / 64] |= (uint64_t(1) << (nIndex % 64));
    }


    /** The bitarray using 64 bit registers. **/
    std::vector<uint64_t> vRegisters;

public:


    /** Default Constructor. **/
    BitArray()                                    = delete;


    /** Copy Constructor. **/
    BitArray(const BitArray& filter)
    : vRegisters (filter.vRegisters)
    {
    }


    /** Move Constructor. **/
    BitArray(BitArray&& filter)
    : vRegisters (std::move(filter.vRegisters))
    {
    }


    /** Copy assignment. **/
    BitArray& operator=(const BitArray& filter)
    {
        vRegisters = filter.vRegisters;

        return *this;
    }


    /** Move assignment. **/
    BitArray& operator=(BitArray&& filter)
    {
        vRegisters = std::move(filter.vRegisters);

        return *this;
    }


    /** Default Destructor. **/
    ~BitArray()
    {
    }


    /** Create bit array with given number of elements. **/
    BitArray  (const uint64_t nElements)
    : vRegisters ((nElements / 64) + 1, 0)
    {
    }



    /** Bytes
     *
     *  Get the beginning memory location of the bit array.
     *
     **/
    uint8_t* Bytes() const
    {
        return (uint8_t*)&vRegisters[0];
    }


    /** Size
     *
     *  Get the size (in bytes) of the bit array.
     *
     **/
    uint64_t Size() const
    {
        return vRegisters.size() * 8;
    }
};

#endif
