/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#define FLATDATA(obj)   REF(util::encoding::flat_data((char*)&(obj), (char*)&(obj) + sizeof(obj)))

namespace util::encoding
{
    /** flat_data
     *
     *  Wrapper for serializing arrays and POD.
     *  There's a clever template way to make arrays serialize normally, but MSVC6 doesn't support it.
     *
     **/
    class flat_data
    {
    protected:
        char* pbegin;
        char* pend;
    public:


        /** flat_data
         *
         *  Default constructor, initializes begin and end pointers.
         *
         *  @param[in] pbeginIn begin pointer.
         *  @param[in] pendIn end pointer.
         *
         **/
        flat_data(void* pbeginIn, void* pendIn)
        : pbegin((char*)pbeginIn)
        , pend((char*)pendIn) { }


        /** begin
         *
         *  Get the begin pointer.
         *
         * return The begin pointer.
         *
         **/
        char* begin() { return pbegin; }


        /** begin
         *
         *  Get the begin pointer.
         *
         * return The constant begin pointer.
         *
         **/
        const char* begin() const { return pbegin; }


        /** end
         *
         *  Get the end pointer.
         *
         * return The end pointer.
         *
         **/
        char* end() { return pend; }


        /** end
         *
         *  Get the end pointer.
         *
         * return The constant end pointer.
         *
         **/
        const char* end() const { return pend; }


        /** GetSerializeSize
         *
         *  The the size, in bytes, of the serialize object.
         *
         *  @return The number of bytes.
         *
         **/
        uint64_t GetSerializeSize(uint32_t, uint32_t=0) const
        {
            return pend - pbegin;
        }


        /** Serialize
         *
         *  Use the template stream to write the serialize object
         *
         *  @param[in] s The templated output stream object
         *
         **/
        template<typename Stream>
        void Serialize(Stream& s, uint32_t, uint32_t=0) const
        {
            s.write(pbegin, pend - pbegin);
        }


        /** Unserialize
         *
         *  Use the template stream to read the serialize object
         *
         *  @param[in] The templated input stream object
         *
         **/
        template<typename Stream>
        void Unserialize(Stream& s, uint32_t, uint32_t=0)
        {
            s.read(pbegin, pend - pbegin);
        }
    };
}
