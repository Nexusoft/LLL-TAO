/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "act the way you'd like to be and soon you'll be the way you act" - Leonard Cohen

____________________________________________________________________________________________*/


#include <TAO/Register/include/object.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** Parse
         *
         *  Parses out the data members of an object register.
         *
         **/
        bool Object::Parse()
        {
            /* Reset the read position. */
            nReadPos   = 0;

            /* Read until end of state. */
            while(!end())
            {
                /* Deserialize the named value. */
                std::string name;
                *this >> name;

                /* Disallow duplicate value entries. */
                if(mapData.count(name))
                    return debug::error(FUNCTION, "duplicate value entries");

                /* Deserialize the type. */
                uint8_t nType;
                *this >> nType;

                /* Mutable default: false (read only). */
                bool fMutable = false;

                /* Check for mutable specifier. */
                if(nType == TYPES::MUTABLE)
                {
                    /* Set this type to be mutable. */
                    fMutable = true;

                    /* If mutable found, deserialize the type. */
                    *this >> nType;
                }

                /* Switch between supported types. */
                switch(nType)
                {

                    /* Standard type for C++ uint8_t. */
                    case TYPES::UINT8_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(name, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 2;

                        break;
                    }


                    /* Standard type for C++ uint16_t. */
                    case TYPES::UINT16_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(name, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 3;

                        break;
                    }


                    /* Standard type for C++ uint32_t. */
                    case TYPES::UINT32_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(name, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 5;

                        break;
                    }


                    /* Standard type for C++ uint64_t. */
                    case TYPES::UINT64_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(name, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 9;

                        break;
                    }


                    /* Standard type for Custom uint256_t */
                    case TYPES::UINT256_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(name, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 33;

                        break;
                    }


                    /* Standard type for Custom uint512_t */
                    case TYPES::UINT512_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(name, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 65;

                        break;
                    }


                    /* Standard type for Custom uint1024_t */
                    case TYPES::UINT1024_T:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(name, std::make_pair(--nReadPos, fMutable));

                        /* Iterate the types size plus type byte. */
                        nReadPos += 129;

                        break;
                    }


                    /* Standard type for STL string */
                    case TYPES::STRING:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(name, std::make_pair(--nReadPos, fMutable));

                        /* Iterate to start of size. */
                        ++nReadPos;

                        /* Find the serialized size of type. */
                        uint64_t nSize = ReadCompactSize(*this);

                        /* Iterate the type size */
                        nReadPos += nSize;

                        break;
                    }


                    /* Standard type for STL vector with C++ type uint8_t */
                    case TYPES::BYTES:
                    {
                        /* Track the binary position of type. */
                        mapData.emplace(name, std::make_pair(--nReadPos, fMutable));

                        /* Iterate to start of size. */
                        ++nReadPos;

                        /* Find the serialized size of type. */
                        uint64_t nSize = ReadCompactSize(*this);

                        /* Iterate the type size */
                        nReadPos += nSize;

                        break;
                    }


                    /* Fail if types are unknown. */
                    default:
                        return false;
                }
            }

            return true;
        }


        /* Write into the object register a value of type bytes. */
        bool Object::Write(const std::string& strName, const std::string& strValue)
        {
            /* Check that the name exists in the object. */
            if(!mapData.count(strName))
                return false;

            /* Check that the value is mutable (writes allowed). */
            if(!mapData[strName].second)
                return debug::error(FUNCTION, "cannot set value for READONLY data member");

            /* Find the binary position of value. */
            nReadPos = mapData[strName].first;

            /* Deserialize the type specifier. */
            uint8_t nType;
            *this >> nType;

            /* Make sure that value being written is type-safe. */
            if(nType != TYPES::STRING)
                return debug::error(FUNCTION, "type must be string");

            /* Get the expected size. */
            uint64_t nSize = ReadCompactSize(*this);
            if(nSize != strValue.size())
                return debug::error(FUNCTION, "string size mismatch");

            /* Check for memory overflows. */
            if(nReadPos + nSize >= vchState.size())
                return debug::error(FUNCTION, "performing an over-write");

            /* CnTypey the bytes into the object. */
            std::copy((uint8_t*)&strValue[0], (uint8_t*)&strValue[0] + strValue.size(), (uint8_t*)&vchState[nReadPos]);

            return true;
        }


        /* Write into the object register a value of type bytes. */
        bool Object::Write(const std::string& strName, const std::vector<uint8_t>& vData)
        {
            /* Check that the name exists in the object. */
            if(!mapData.count(strName))
                return false;

            /* Check that the value is mutable (writes allowed). */
            if(!mapData[strName].second)
                return debug::error(FUNCTION, "cannot set value for READONLY data member");

            /* Find the binary position of value. */
            nReadPos = mapData[strName].first;

            /* Deserialize the type specifier. */
            uint8_t nType;
            *this >> nType;

            /* Make sure that value being written is type-safe. */
            if(nType != TYPES::BYTES)
                return debug::error(FUNCTION, "type must be bytes");

            /* Get the expected size. */
            uint64_t nSize = ReadCompactSize(*this);
            if(nSize != vData.size())
                return debug::error(FUNCTION, "bytes size mismatch");

            /* Check for memory overflows. */
            if(nReadPos + nSize >= vchState.size())
                return debug::error(FUNCTION, "performing an over-write");

            /* CnTypey the bytes into the object. */
            std::copy((uint8_t*)&vData[0], (uint8_t*)&vData[0] + vData.size(), (uint8_t*)&vchState[nReadPos]);

            return true;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint8_t n) const
        {
            return TYPES::UINT8_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint16_t n) const
        {
            return TYPES::UINT16_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint32_t n) const
        {
            return TYPES::UINT32_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint64_t& n) const
        {
            return TYPES::UINT64_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint256_t& n) const
        {
            return TYPES::UINT256_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint512_t& n) const
        {
            return TYPES::UINT512_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const uint1024_t& n) const
        {
            return TYPES::UINT1024_T;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const std::string& n) const
        {
            return TYPES::STRING;
        }


        /* Helper function that uses template deduction to find type enum. */
        uint8_t Object::type(const std::vector<uint8_t>& n) const
        {
            return TYPES::BYTES;
        }
    }
}
