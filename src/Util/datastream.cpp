/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/templates/datastream.h>


/** Default Constructor. **/
DataStream::DataStream(uint32_t nSerTypeIn, uint32_t nSerVersionIn)
: vData()
, nReadPos(0)
, nSerType(nSerTypeIn)
, nSerVersion(nSerVersionIn)
{
}


/*  Constructs the DataStream object. */
DataStream::DataStream(const std::vector<uint8_t>& vchDataIn, const uint32_t nSerTypeIn, const uint32_t nSerVersionIn)
: vData(vchDataIn)
, nReadPos(0)
, nSerType(nSerTypeIn)
, nSerVersion(nSerVersionIn)
{
}

/*  Default constructor for initialization with serialize data, type and version. */
DataStream::DataStream(const std::vector<uint64_t>& vchDataIn, const uint32_t nSerTypeIn, const uint32_t nSerVersionIn)
: vData((uint8_t*)&vchDataIn.begin()[0], (uint8_t*)&vchDataIn.end()[0])
, nReadPos(0)
, nSerType(nSerTypeIn)
, nSerVersion(nSerVersionIn)
{
}


/*  Default constructor for initialization with serialize data, type and version */
DataStream::DataStream(const std::vector<uint8_t>::const_iterator pbegin,
            const std::vector<uint8_t>::const_iterator pend,
            const uint32_t nSerTypeIn, const uint32_t nSerVersionIn)
: vData(pbegin, pend)
, nReadPos(0)
, nSerType(nSerTypeIn)
, nSerVersion(nSerVersionIn)
{
}



#if !defined(_MSC_VER) || _MSC_VER >= 1300

    /*  Default constructor for initialization with serialize data, type and version.
     *  (Microsoft compiler compatible) */
    DataStream::DataStream(const char* pbegin, const char* pend, const uint32_t nSerTypeIn, const uint32_t nSerVersionIn)
    : vData((uint8_t*)pbegin, (uint8_t*)pend)
    , nReadPos(0)
    , nSerType(nSerTypeIn)
    , nSerVersion(nSerVersionIn)
    {
    }
#endif


/*  Default constructor for initialization with serialize data, type and version. */
DataStream::DataStream(const std::vector<char>& vchDataIn, const uint32_t nSerTypeIn, const uint32_t nSerVersionIn)
: vData((uint8_t*)&vchDataIn.begin()[0], (uint8_t*)&vchDataIn.end()[0])
, nReadPos(0)
, nSerType(nSerTypeIn)
, nSerVersion(nSerVersionIn)
{
}


/*  Sets the type of stream. */
void DataStream::SetType(uint8_t nSerTypeIn)
{
    nSerType = nSerTypeIn;
}


/*  Sets the position in the stream. */
void DataStream::SetPos(uint64_t nNewPos) const
{
    /* Check size constraints. */
    if(nNewPos > size())
        throw std::runtime_error(debug::safe_printstr(FUNCTION, "cannot set at end of stream ", nNewPos));

    /* Set the new read pos. */
    nReadPos = nNewPos;
}


/*  Sets the object into null state. */
void DataStream::SetNull()
{
    nReadPos = 0;
    vData.clear();
}


/*  Returns if object is in null state. */
bool DataStream::IsNull() const
{
    return nReadPos == 0 && size() == 0;
}


/*  Resets the internal read pointer. */
void DataStream::Reset() const
{
    nReadPos = 0;
}


/*  Returns if end of stream is found. */
bool DataStream::End() const
{
    return nReadPos >= size();
}


/*  Reads raw data from the stream. */
const DataStream& DataStream::read(char* pch, uint64_t nSize) const
{
    /* Check size constraints. */
    if(nReadPos + nSize > size())
        throw std::runtime_error(debug::safe_printstr(FUNCTION, "reached end of stream ", nReadPos));

    /* Copy the bytes into tmp object. */
    std::copy((uint8_t*)&vData.at(nReadPos), (uint8_t*)&vData.at(nReadPos) + nSize, (uint8_t*)pch);

    /* Iterate the read position. */
    nReadPos += nSize;

    return *this;
}


/*  Writes data into the stream. */
DataStream& DataStream::write(const char* pch, uint64_t nSize)
{
    /* Push the obj bytes into the vector. */
    vData.insert(vData.end(), (uint8_t*)pch, (uint8_t*)pch + nSize);

    return *this;
}


/*  Get the data stream from the object. */
const std::vector<uint8_t>& DataStream::Bytes()
{
    return vData;
}


/*  Implement the same reserve functionality to vector. */
void DataStream::reserve(const uint64_t nSize)
{
    vData.reserve(nSize);
}


/*  Implement the same reserve functionality to vector. */
void DataStream::resize(const uint64_t nSize)
{
    vData.resize(nSize);
}


/*  Wrapper around the vector constant iterator. */
std::vector<uint8_t>::const_iterator DataStream::begin() const
{
    return vData.begin();
}


/*  Wrapper around the vector constant iterator. */
std::vector<uint8_t>::const_iterator DataStream::end() const
{
    return vData.end();
}


/*  Wrapper around the vector iterator. */
std::vector<uint8_t>::iterator DataStream::begin()
{
    return vData.begin();
}


/*  Wrapper around the vector iterator. */
std::vector<uint8_t>::iterator DataStream::end()
{
    return vData.end();
}


/*  Wrapper around data to get the start of vector. */
uint8_t* DataStream::data(const uint64_t nOffset)
{
    return &vData[nOffset];
}


/*  Wrapper around the vector clear. */
void DataStream::clear()
{
    return vData.clear();
}


/*  Get the size of the data stream. */
uint64_t DataStream::size() const
{
    return vData.size();
}
