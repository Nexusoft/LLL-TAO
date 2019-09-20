/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_UTIL_TEMPLATES_DATASTREAM_H
#define NEXUS_UTIL_TEMPLATES_DATASTREAM_H

#include <Util/templates/serialize.h>

#include <cstdint>
#include <vector>


/** DataStream
 *
 *  Class to handle the serializaing and deserializing of data to disk or over the network
 *
 **/
class DataStream
{
    /** The byte vector of data . **/
    std::vector<uint8_t> vData;


    /** The current reading position. **/
    mutable uint64_t nReadPos;


    /** The serialization type. **/
    uint32_t nSerType;


    /** The serializtion version **/
    uint32_t nSerVersion;


public:

    /** Default Constructor. **/
    DataStream(uint32_t nSerTypeIn, uint32_t nSerVersionIn);


    /** DataStream
     *
     *  Constructs the DataStream object.
     *
     *  @param[in] vchDataIn The byte vector to insert.
     *  @param[in] nSerTypeIn The serialize type.
     *  @param[in] nSerVersionIn The serialize version.
     *
     **/
    DataStream(const std::vector<uint8_t>& vchDataIn, const uint32_t nSerTypeIn, const uint32_t nSerVersionIn);


    /** DataStream
     *
     *  Constructs the DataStream object.
     *
     *  @param[in] vchDataIn The byte vector to insert.
     *  @param[in] nSerTypeIn The serialize type.
     *  @param[in] nSerVersionIn The serialize version.
     *
     **/
    DataStream(const std::vector<uint64_t>& vchDataIn, const uint32_t nSerTypeIn, const uint32_t nSerVersionIn);


    /** DataStream
     *
     *  Default constructor for initialization with serialize data, type and version
     *
     **/
    DataStream(const std::vector<uint8_t>::const_iterator pbegin,
                const std::vector<uint8_t>::const_iterator pend,
                const uint32_t nSerTypeIn, const uint32_t nSerVersionIn);


#if !defined(_MSC_VER) || _MSC_VER >= 1300

    /** DataStream
     *
     *  Default constructor for initialization with serialize data, type and version.
     *  (Microsoft compiler compatible)
     *
     **/
    DataStream(const char* pbegin, const char* pend, const uint32_t nSerTypeIn, const uint32_t nSerVersionIn);
#endif


    /** DataStream
     *
     *  Default constructor for initialization with serialize data, type and version.
     *
     **/
    DataStream(const std::vector<char>& vchDataIn, const uint32_t nSerTypeIn, const uint32_t nSerVersionIn);


    /** Destructor. */
    ~DataStream()
    {
        std::vector<uint8_t>().swap(vData);
    }


    /** SetType
     *
     *  Sets the type of stream.
     *
     *  @param[in] nSerTypeIn The serialize type to set.
     *
     **/
    void SetType(uint8_t nSerTypeIn);


    /** SetPos
     *
     *  Sets the position in the stream.
     *
     *  @param[in] nNewPos The position to set to in the stream.
     *
     **/
    void SetPos(uint64_t nNewPos) const;


    /** GetPos
     *
     *  Gets the position in the stream.
     *
     *  @return the current read position in the stream.
     *
     **/
    uint64_t GetPos() const;


    /** SetNull
     *
     *  Sets the object into null state.
     *
     **/
    void SetNull();


    /** IsNull
     *
     *  Returns if object is in null state.
     *
     **/
    bool IsNull() const;


    /** Reset
     *
     *  Resets the internal read pointer.
     *
     **/
    void Reset() const;


    /** End
     *
     *  Returns if end of stream is found.
     *
     **/
    bool End() const;


    /** read
     *
     *  Reads raw data from the stream.
     *
     *  @param[in] pch The pointer to beginning of memory to write.
     *  @param[in] nSize The total number of bytes to read.
     *
     *  @return Returns a reference to the DataStream object.
     *
     **/
    const DataStream& read(char* pch, uint64_t nSize) const;


    /** write
     *
     *  Writes data into the stream.
     *
     *  @param[in] pch The pointer to beginning of memory to write.
     *  @param[in] nSize The total number of bytes to copy.
     *
     **/
    DataStream& write(const char* pch, uint64_t nSize);


    /** Bytes
     *
     *  Get the data stream from the object as a const reference.
     *
     **/
    const std::vector<uint8_t>& Bytes() const;


    /** Bytes
     *
     *  Get the data stream from the object.
     *
     **/
    std::vector<uint8_t>& Bytes();


    /** reserve
     *
     *  Implement the same reserve functionality to vector.
     *
     **/
    void reserve(const uint64_t nSize);


    /** resize
     *
     *  Implement the same resize functionality to vector.
     *
     **/
    void resize(const uint64_t nSize);


    /** begin
     *
     *  Wrapper around the vector constant iterator.
     *
     **/
    std::vector<uint8_t>::const_iterator begin() const;


    /** end
     *
     *  Wrapper around the vector constant iterator.
     *
     **/
    std::vector<uint8_t>::const_iterator end() const;


    /** begin
     *
     *  Wrapper around the vector iterator.
     *
     **/
    std::vector<uint8_t>::iterator begin();


    /** end
     *
     *  Wrapper around the vector iterator.
     *
     **/
    std::vector<uint8_t>::iterator end();


    /** data
     *
     *  Wrapper around data to get the start of vector.
     *
     **/
    uint8_t* data(const uint64_t nOffset = 0);


    /** clear
     *
     *  Wrapper around the vector clear.
     *
     **/
    void clear();


    /** size
     *
     *  Get the size of the data stream.
     *
     **/
    uint64_t size() const;


    /** Operator Overload <<
     *
     *  Serializes data into vchOperations
     *
     *  @param[in] obj The object to serialize into ledger data.
     *
     **/
    template<typename Type>
    DataStream& operator<<(const Type& obj)
    {
        /* Serialize to the stream. */
        ::Serialize(*this, obj, nSerType, nSerVersion); //temp versinos for now

        return (*this);
    }


    /** Operator Overload >>
     *
     *  Serializes data into vchOperations.
     *
     *  @param[out] obj The object to de-serialize from ledger data.
     *
     **/
    template<typename Type>
    const DataStream& operator>>(Type& obj) const
    {
        /* Unserialize from the stream. */
        ::Unserialize(*this, obj, nSerType, nSerVersion);
        return (*this);
    }
};

#endif
