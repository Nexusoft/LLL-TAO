/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_STREAM_H
#define NEXUS_TAO_REGISTER_INCLUDE_STREAM_H

#include <vector>
#include <cstdint>

#include <LLD/include/version.h>

#include <Util/templates/basestream.h>
#include <Util/templates/serialize.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** Stream
         *
         *  Class to handle the serializaing and deserializing of operations and their types
         *
         **/
        class Stream : public BaseStream
        {
        public:

            /** Default Constructor. **/
            Stream()
            : BaseStream()
            {

            }


            /** Data Constructor.
             *
             *  @param[in] vchDataIn The byte vector to insert.
             *
             **/
            Stream(const std::vector<uint8_t>& vchDataIn)
            : BaseStream(vchDataIn)
            {
            }


            /** Serialization **/
            IMPLEMENT_SERIALIZE
            (
                READWRITE(vchData);
            )


            /** Operator Overload <<
             *
             *  Serializes data into vchOperations
             *
             *  @param[in] obj The object to serialize into ledger data
             *
             **/
            template<typename Type> Stream& operator<<(const Type& obj)
            {
                /* Serialize to the stream. */
                ::Serialize(*this, obj, (uint32_t)SER_REGISTER, LLD::DATABASE_VERSION); //temp versinos for now

                return (*this);
            }


            /** Operator Overload >>
             *
             *  Serializes data into vchOperations
             *
             *  @param[out] obj The object to de-serialize from ledger data
             *
             **/
            template<typename Type> const Stream& operator>>(Type& obj) const
            {
                /* Unserialize from the stream. */
                ::Unserialize(*this, obj, (uint32_t)SER_REGISTER, LLD::DATABASE_VERSION); //TODO: version should be object version
                return (*this);
            }
        };
    }
}

#endif
