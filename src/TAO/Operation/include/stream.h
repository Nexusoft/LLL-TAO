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
                return nReadPos == vchData.size();
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
                /* Push the size byte into vector. */
                vchData.push_back((uint8_t)sizeof(obj));

                /* Push the obj bytes into the vector. */
                vchData.insert(vchData.end(), (uint8_t*)&obj, (uint8_t*)&obj + sizeof(obj));

                return *this;
            }


            /** Operator Overload >>
             *
             *  Serializes data into vchLedgerData
             *
             *  @param[out] obj The object to de-serialize from ledger data
             *
             **/
            template<typename Type> Stream& operator>>(Type& obj) //TODO: catch the end of stream
            {
                /* Get the size from size byte. */
                uint8_t nSize = vchData[nReadPos];

                /* Create tmp object to prevent double free in std::copy. */
                Type tmp;

                /* Copy the bytes into tmp object. */
                std::copy((uint8_t*)&vchData[nReadPos + 1], (uint8_t*)&vchData[nReadPos + 1] + nSize, (uint8_t*)&tmp);

                /* Iterate the read position. */
                nReadPos += nSize + 1;

                /* Set the return value. */
                obj = tmp;

                return *this;
            }
        };
    }
}

#endif
