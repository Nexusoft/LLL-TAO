/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_TEMPLATES_STREAM_H
#define NEXUS_UTIL_TEMPLATES_STREAM_H

#include <vector>
#include <cstdint>

#include <Util/templates/serialize.h>


/** Stream enumeration. **/
enum STREAM
{
    /* Beginning of stream. */
    BEGIN     = 0x01,

    /* End of stream. */
    END       = 0x02,

    /* From cursor. */
    CURSOR    = 0x03
};


/** BaseStream
 *
 *  Class to handle the serializaing and deserializing of data into their types
 *
 **/
class BaseStream
{
protected:

    /** The current reading position. **/
    uint32_t nReadPos;


    /** The operation data vector. **/
    std::vector<uint8_t> vchData;


public:

    /** Default Constructor. **/
    BaseStream()
    : nReadPos(0)
    , vchData()
    {

    }


    /** Data Constructor.
     *
     *  @param[in] vchDataIn The byte vector to insert.
     *
     **/
    BaseStream(std::vector<uint8_t> vchDataIn)
    : nReadPos(0)
    , vchData(vchDataIn)
    {
    }


    /** Set null method.
     *
     *  Sets the object into null state.
     *
     **/
    void SetNull()
    {
        nReadPos = 0;
        vchData.clear();
    }


    /** Is null method
     *
     *  Returns if object is in null state.
     *
     **/
    bool IsNull()
    {
        return nReadPos == 0 && vchData.size() == 0;
    }


    /** Reset
     *
     *  Resets the internal read pointer
     *
     **/
    void reset()
    {
        nReadPos = 0;
    }


    /** Get
     *
     *  Gets a byte without chainging read pointer
     *
     **/
    uint8_t get(uint32_t nPos) const
    {
        if(nPos >= vchData.size())
            throw std::runtime_error(debug::strprintf(FUNCTION "get out of bounds %u", nPos));

        return vchData[nPos];
    }


    /** Size
     *
     *  Gets the size of the stream.
     *
     **/
    uint32_t size() const
    {
        return vchData.size();
    }


    /** Seek
     *
     *  Seeks the read pointer to position.
     *
     **/
    void seek(uint32_t nSeek, uint8_t nFlags = STREAM::CURSOR)
    {
        /* Seek from end of stream. */
        if(nFlags == STREAM::CURSOR)
        {
            if(nReadPos + nSeek > vchData.size())
                throw std::runtime_error(debug::strprintf(FUNCTION "seek out of bounds %u", nSeek));

            nReadPos += nSeek;

            return;
        }

        /* Seek from beginning of stream. */
        if(nFlags == STREAM::BEGIN)
        {
            if(nSeek > vchData.size())
                throw std::runtime_error(debug::strprintf(FUNCTION "seek out of bounds %u", nSeek));

            nReadPos = nSeek;

            return;
        }

        /* Seek from end of stream. */
        if(nFlags == STREAM::END)
        {
            if(nSeek > vchData.size())
                throw std::runtime_error(debug::strprintf(FUNCTION "seek out of bounds %u", nSeek));

            nReadPos = vchData.size() - nSeek;

            return;
        }
    }


    /** Begin
     *
     *  Returns if the opeartions stream is on first operation
     *
     **/
    bool begin() const
    {
        return nReadPos == 1;
    }


    /** End
     *
     *  Returns if end of stream is found
     *
     **/
    bool end() const
    {
        return nReadPos == vchData.size();
    }


    /** read
     *
     *  Reads raw data from the stream
     *
     *  @param[in] pch The pointer to beginning of memory to write
     *
     *  @param[in] nSize The total number of bytes to read
     *
     **/
    BaseStream& read(char* pch, int nSize)
    {
        /* Check size constraints. */
        if(nReadPos + nSize > vchData.size())
            throw std::runtime_error(debug::strprintf(FUNCTION "reached end of stream %u", nReadPos));

        /* Copy the bytes into tmp object. */
        std::copy((uint8_t*)&vchData[nReadPos], (uint8_t*)&vchData[nReadPos] + nSize, (uint8_t*)pch);

        /* Iterate the read position. */
        nReadPos += nSize;

        return *this;
    }


    /** write
     *
     *  Writes data into the stream
     *
     *  @param[in] pch The pointer to beginning of memory to write
     *
     *  @param[in] nSize The total number of bytes to copy
     *
     **/
    BaseStream& write(const char* pch, int nSize)
    {
        /* Push the obj bytes into the vector. */
        vchData.insert(vchData.end(), (uint8_t*)pch, (uint8_t*)pch + nSize);

        return *this;
    }
};

#endif
