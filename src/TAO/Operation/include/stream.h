/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_OPERATION_INCLUDE_STREAM_H
#define NEXUS_TAO_OPERATION_INCLUDE_STREAM_H

#include <vector>
#include <cstdint>

#include <LLD/include/version.h>

#include <Util/templates/serialize.h>

namespace TAO
{

    namespace Operation
    {

        /** Stream
         *
         *  Class to handle the serializaing and deserializing of operations and their types
         *
         **/
        class Stream
        {

            /** The current reading position. **/
            uint32_t nReadPos;


            /** The operation data vector. **/
            std::vector<uint8_t> vchData;


        public:

            /** Default Constructor. **/
            Stream() : nReadPos(0) { vchData.clear(); }


            /** Data Constructor.
             *
             *  @param[in] vchDataIn The byte vector to insert.
             *
             **/
            Stream(std::vector<uint8_t> vchDataIn) : nReadPos(0), vchData(vchDataIn) {  }


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
            void Reset()
            {
                nReadPos = 0;
            }


            /** End
             *
             *  Returns if end of stream is found
             *
             **/
            bool End()
            {
                return nReadPos >= vchData.size();
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
            Stream& read(char* pch, int nSize)
            {
                /* Check size constraints. */
                if(nReadPos + nSize > vchData.size())
                    throw std::runtime_error(debug::strprintf(FUNCTION "reached end of stream %u", __PRETTY_FUNCTION__, nReadPos));

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
            Stream& write(const char* pch, int nSize)
            {
                /* Push the obj bytes into the vector. */
                vchData.insert(vchData.end(), (uint8_t*)pch, (uint8_t*)pch + nSize);

                return *this;
            }


            /** Operator Overload <<
             *
             *  Serializes data into vchLedgerData
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type> Stream& operator<<(const Type& obj)
            {
                /* Serialize to the stream. */
                ::Serialize(*this, obj, SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

                return (*this);
            }


            /** Operator Overload >>
             *
             *  Serializes data into vchLedgerData
             *
             *  @param[out] obj The object to de-serialize from ledger data
             *
             **/
            template<typename Type> Stream& operator>>(Type& obj)
            {
                /* Unserialize from the stream. */
                ::Unserialize(*this, obj, SER_OPERATIONS, LLD::DATABASE_VERSION); //TODO: version should be object version
                return (*this);
            }
        };
    }
}

#endif
