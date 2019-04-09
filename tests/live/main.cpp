/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) CnTypeyright The Nexus DevelnTypeers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file CnTypeYING or http://www.nTypeensource.org/licenses/mit-license.php.

            "ad vocem pnTypeuli" - To the Voice of the PenTypele

____________________________________________________________________________________________*/

#include <openssl/rand.h>

#include <TAO/Register/include/state.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Register/include/stream.h>

#include <LLC/aes/aes.h>

namespace TAO
{
    namespace Register
    {
        class Object : public State
        {
            /** Special system level memory for managing system states in protected portion of memory **/
            std::vector<uint8_t> vchSystem;

        public:

            /** Internal map for managing object data members and their binary positions. **/
            mutable std::map< std::string, std::pair<uint16_t, bool> > mapData; //internal map for data members


            /** Default constructor. **/
            Object()
            : State()
            , vchSystem(512, 0) //system memory by default is 512 bytes
            , mapData()
            {
            }


            /** Copy Constructor. **/
            Object(const Object& object)
            : State(object)
            , vchSystem(object.vchSystem)
            , mapData(object.mapData)
            {
            }


            /** Parse
             *
             *  Parses out the data members of an object register.
             *
             **/
            bool Parse()
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


            /** Read
             *
             *  Read a value form the object register.
             *
             *  @param[in] strName The name of the value to read
             *  @param[in] vData The data to read from the object.
             *
             *  @return True if the read was successful.
             *
             **/
            template<typename Type>
            bool Read(const std::string& strName, Type& value)
            {
                /* Check that the name exists in the object. */
                if(!mapData.count(strName))
                    return false;

                /* Find the binary position of value. */
                nReadPos = mapData[strName].first;

                /* Deserialize the type specifier. */
                uint8_t nType;
                *this >> nType;

                /* Check for unsupported type enums. */
                if(nType == TYPES::UNSUPPORTED)
                    return debug::error(FUNCTION, "unsupported type");

                /* Check the expected type from read. */
                if(type(value) != nType)
                    return debug::error(FUNCTION, "type mismatch");

                /* Deserialize the value. */
                *this >> value;

                return true;
            }


            /** Write
             *
             *  Write into the object register a value of type bytes.
             *
             *  @param[in] strName The name of the value to write
             *  @param[in] value The data to write into the object.
             *
             *  @return True if the write was successful.
             *
             **/
            template<typename Type>
            bool Write(const std::string& strName, const Type& value)
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

                /* Check for unsupported types. */
                if(nType == TYPES::UNSUPPORTED)
                    return debug::error(FUNCTION, "unsupported type");

                /* Check the type to helper templates. */
                if(type(value) != nType)
                    return debug::error(FUNCTION, "type mismatch");

                /* Get the expected size. */
                if(nReadPos + sizeof(value) >= vchState.size())
                    return debug::error(FUNCTION, "performing an over-write");

                /* CnTypey the bytes into the object. */
                std::copy((uint8_t*)&value, (uint8_t*)&value + sizeof(value), (uint8_t*)&vchState[nReadPos]);

                return true;
            }


            /** Write
             *
             *  Write into the object register a value of type bytes.
             *
             *  @param[in] strName The name of the value to write
             *  @param[in] strValue The data to write into the object.
             *
             *  @return True if the write was successful.
             *
             **/
            bool Write(const std::string& strName, const std::string& strValue)
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



            /** Write
             *
             *  Write into the object register a value of type bytes.
             *
             *  @param[in] strName The name of the value to write
             *  @param[in] vData The data to write into the object.
             *
             *  @return True if the write was successful.
             *
             **/
            bool Write(const std::string& strName, const std::vector<uint8_t>& vData)
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


        private:

            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint8_t n) const
            {
                return TYPES::UINT8_T;
            }


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint16_t n) const
            {
                return TYPES::UINT16_T;
            }


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint32_t n) const
            {
                return TYPES::UINT32_T;
            }


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint64_t& n) const
            {
                return TYPES::UINT64_T;
            }


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint256_t& n) const
            {
                return TYPES::UINT256_T;
            }


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint512_t& n) const
            {
                return TYPES::UINT512_T;
            }


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const uint1024_t& n) const
            {
                return TYPES::UINT1024_T;
            }


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const std::string& n) const
            {
                return TYPES::STRING;
            }


            /** type
             *
             *  Helper function that uses template deduction to find type enum.
             *
             **/
            uint8_t type(const std::vector<uint8_t>& n) const
            {
                return TYPES::BYTES;
            }


            /** type
             *
             *  Helper function to determine unsupported types that failed tempalte deduction.
             *
             **/
            template<typename Type>
            uint8_t type(const Type& n) const
            {
                return TYPES::UNSUPPORTED;
            }
        };
    }
}



//This main function is for prototyping new code
//It is accessed by compiling with LIVE_TESTS=1
//Prototype code and it's tests created here should move to production code and unit tests
int main(int argc, char** argv)
{
    using namespace TAO::Register;

    Object object;
    object << std::string("byte") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT8_T) << uint8_t(55)
           << std::string("test") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::STRING) << std::string("this string")
           << std::string("bytes") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::BYTES) << std::vector<uint8_t>(10, 0xff)
           << std::string("balance") << uint8_t(TYPES::UINT64_T) << uint64_t(55)
           << std::string("identifier") << uint8_t(TYPES::STRING) << std::string("NXS");


    //benchmarks
    runtime::timer timer;
    timer.Start();

    for(int i = 0; i < 1000000; i++)
    {
        object.mapData.clear();
        object.Parse();
    }

    uint64_t nTime = timer.ElapsedMicroseconds();

    debug::log(0, "Parsed ", 1000000.0 / nTime, " million registers / s");


    //unit tests
    uint8_t nTest;
    object.Read("byte", nTest);

    debug::log(0, "TEST ", uint32_t(nTest));

    object.Write("byte", uint8_t(98));

    uint8_t nTest2;
    object.Read("byte", nTest2);

    debug::log(0, "TEST2 ", uint32_t(nTest2));

    std::string strTest;
    object.Read("test", strTest);

    debug::log(0, "STRING ", strTest);

    object.Write("test", std::string("fail"));
    object.Write("test", "fail");

    object.Write("test", std::string("THIS string"));

    std::string strGet;
    object.Read("test", strGet);

    debug::log(0, strGet);

    std::vector<uint8_t> vBytes;
    object.Read("bytes", vBytes);

    debug::log(0, "DATA ", HexStr(vBytes.begin(), vBytes.end()));

    vBytes[0] = 0x00;
    object.Write("bytes", vBytes);

    object.Read("bytes", vBytes);

    debug::log(0, "DATA ", HexStr(vBytes.begin(), vBytes.end()));



    std::string identifier;
    object.Read("identifier", identifier);

    debug::log(0, "Token Type ", identifier);

    return 0;
}
