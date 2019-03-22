/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_TEMPLATES_X86_STREAM_H
#define NEXUS_UTIL_TEMPLATES_X86_STREAM_H

#include <Util/include/debug.h>

#include <vector>
#include <cstdint>
#include <stdexcept>


/** BaseStream
 *
 *  Class to handle the serializaing and deserializing of data into their types
 *
 **/
class x86_BaseStream
{
protected:

    /** The current reading position. **/
    mutable uint64_t nReadPos;


    /** The operation data vector. **/
    std::vector<uint32_t> vchData;


public:

    /** Default Constructor. **/
    x86_BaseStream()
    : nReadPos(0)
    , vchData()
    {

    }


    /** Data Constructor.
     *
     *  @param[in] vchDataIn The byte vector to insert.
     *
     **/
    x86_BaseStream(const std::vector<uint32_t>& vchDataIn)
    : nReadPos(0)
    , vchData(vchDataIn)
    {
    }

    /** Default Destructor **/
    virtual ~x86_BaseStream()
    {
    }


    /** SetNull
     *
     *  Sets the object into null state.
     *
     **/
    void SetNull()
    {
        nReadPos = 0;
        vchData.clear();
    }


    /** IsNull
     *
     *  Returns if object is in null state.
     *
     **/
    bool IsNull() const
    {
        return nReadPos == 0 && vchData.size() == 0;
    }


    /** reset
     *
     *  Resets the internal read pointer.
     *
     **/
    void reset() const
    {
        nReadPos = 0;
    }


    void Push(const uint32_t nData)
    {
        vchData.push_back(nData);
    }


    void Set(const std::vector<uint32_t> vBytes)
    {
        SetNull();

        vchData = vBytes;
    }


    /** get
     *
     *  Gets a byte without chainging read pointer.
     *
     **/
    uint8_t get(uint64_t nPos) const
    {
        if(nPos >= vchData.size())
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "get out of bounds ", nPos));

        return vchData[nPos];
    }


    std::vector<uint32_t>& Bytes()
    {
        return vchData;
    }


    /** size
     *
     *  Gets the size of the stream.
     *
     **/
    uint64_t size() const
    {
        return vchData.size();
    }


    /** seek
     *
     *  Seeks the read pointer to position.
     *
     **/
    void seek(int64_t nSeek, uint8_t nFlags = STREAM::CURSOR) const
    {
        /* Seek from end of stream. */
        if(nFlags == STREAM::CURSOR)
        {
            if(nReadPos + nSeek > vchData.size())
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "out of bounds ", nSeek));

            nReadPos += nSeek;

            return;
        }

        /* Seek from beginning of stream. */
        if(nFlags == STREAM::BEGIN)
        {
            if(nSeek > vchData.size())
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "out of bounds ", nSeek));

            nReadPos = nSeek;

            return;
        }

        /* Seek from end of stream. */
        if(nFlags == STREAM::END)
        {
            if(nSeek > vchData.size())
                throw std::runtime_error(debug::safe_printstr(FUNCTION, "out of bounds ", nSeek));

            nReadPos = vchData.size() - nSeek;

            return;
        }
    }


    /** begin
     *
     *  Returns if the opeartions stream is on first operation.
     *
     **/
    bool begin() const
    {
        return nReadPos == 1;
    }


    /** end
     *
     *  Returns if end of stream is found.
     *
     **/
    bool end() const
    {
        return nReadPos == vchData.size();
    }


    /** read
     *
     *  Reads raw data from the stream.
     *
     *  @param[in] pch The pointer to beginning of memory to write.
     *  @param[in] nSize The total number of bytes to read.
     *
     *  @return Returns a reference to the BaseStream.
     *
     **/
    const x86_BaseStream& read(char* pch, uint64_t nSize) const
    {
        /* Check size constraints. */
        if(nReadPos + nSize > vchData.size())
            throw std::runtime_error(debug::safe_printstr(FUNCTION, "reached end of stream ", nReadPos));

        /* Copy the bytes into tmp object. */
        std::copy((uint8_t*)&vchData[nReadPos], (uint8_t*)&vchData[nReadPos] + nSize, (uint8_t*)pch);

        /* Iterate the read position. */
        nReadPos += nSize;

        return *this;
    }


    /** write
     *
     *  Writes data into the stream.
     *
     *  @param[in] pch The pointer to beginning of memory to write.
     *  @param[in] nSize The total number of bytes to copy.
     *
     **/
    x86_BaseStream& write(const char* pch, uint64_t nSize)
    {
        /* Push the obj bytes into the vector. */
        vchData.insert(vchData.end(), (uint8_t*)pch, (uint8_t*)pch + nSize);

        return *this;
    }


    /** Operator Overload <<
     *
     *  Serializes data into vchOperations
     *
     *  @param[in] obj The object to serialize into ledger data
     *
     **/
    template<typename Type> x86_BaseStream& operator<<(const Type& obj)
    {
        /* Serialize to the stream. */
        ::Serialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

        return (*this);
    }


    /** Operator Overload >>
     *
     *  Serializes data into vchOperations
     *
     *  @param[out] obj The object to de-serialize from ledger data
     *
     **/
    template<typename Type> const x86_BaseStream& operator>>(Type& obj) const
    {
        /* Unserialize from the stream. */
        ::Unserialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //TODO: version should be object version
        return (*this);
    }
};

#endif
