/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Register/include/state.h>

#include <TAO/Register/include/enum.h>

#include <TAO/Register/include/stream.h>

namespace TAO
{
    namespace Register
    {
        class Object : public State
        {
        public:
            std::vector<uint8_t> vchMethods; //methods for object register

            std::vector<uint8_t> vchSystem; //system level memory in object register

            mutable std::map< std::string, std::pair<uint16_t, bool> > mapData; //internal map for data members

            bool fParsed = false;

            Object()
            : State()
            , vchMethods()
            , vchSystem()
            , mapData()
            {
            }

            uint8_t type(const uint8_t n) const
            {
                return TYPES::UINT8_T;
            }

            uint8_t type(const uint16_t n) const
            {
                return TYPES::UINT16_T;
            }

            uint8_t type(const uint32_t n) const
            {
                return TYPES::UINT32_T;
            }

            uint8_t type(const uint64_t& n) const
            {
                return TYPES::UINT64_T;
            }

            uint8_t type(const uint256_t& n) const
            {
                return TYPES::UINT256_T;
            }

            uint8_t type(const uint512_t& n) const
            {
                return TYPES::UINT512_T;
            }

            uint8_t type(const uint1024_t& n) const
            {
                return TYPES::UINT1024_T;
            }

            uint8_t type(const std::string& n) const
            {
                return TYPES::STRING;
            }

            uint8_t type(const std::vector<uint8_t>& n) const
            {
                return TYPES::BYTES;
            }

            template<typename Type>
            uint8_t type(const Type& n) const
            {
                return TYPES::UNSUPPORTED;
            }


            bool Parse()
            {
                nReadPos   = 0;

                while(!end())
                {
                    std::string name;
                    *this >> name;

                    if(mapData.count(name))
                        return debug::error(FUNCTION, "duplicate value entries");

                    uint8_t op;
                    *this >> op;

                    bool fMutable = false;
                    if(op == TYPES::MUTABLE)
                    {
                        fMutable = true;

                        *this >> op;
                    }

                    switch(op)
                    {

                        case TYPES::UINT8_T:
                        {
                            mapData[name] = std::make_pair(--nReadPos, fMutable);

                            nReadPos += 2;

                            break;
                        }


                        case TYPES::UINT16_T:
                        {
                            mapData[name] = std::make_pair(--nReadPos, fMutable);

                            nReadPos += 3;

                            break;
                        }


                        case TYPES::UINT32_T:
                        {
                            mapData[name] = std::make_pair(--nReadPos, fMutable);

                            nReadPos += 5;

                            break;
                        }


                        case TYPES::UINT64_T:
                        {
                            mapData[name] = std::make_pair(--nReadPos, fMutable);

                            nReadPos += 9;

                            break;
                        }


                        case TYPES::UINT256_T:
                        {
                            mapData[name] = std::make_pair(--nReadPos, fMutable);

                            nReadPos += 33;

                            break;
                        }


                        case TYPES::UINT512_T:
                        {
                            mapData[name] = std::make_pair(--nReadPos, fMutable);

                            nReadPos += 65;

                            break;
                        }


                        case TYPES::UINT1024_T:
                        {
                            mapData[name] = std::make_pair(--nReadPos, fMutable);

                            nReadPos += 129;

                            break;
                        }


                        case TYPES::STRING:
                        {
                            mapData[name] = std::make_pair(--nReadPos, fMutable);

                            ++nReadPos;
                            uint64_t nSize = ReadCompactSize(*this);

                            nReadPos += nSize;

                            break;
                        }



                        //fail on object registers with unknown types
                        default:
                            return false;
                    }
                }

                //(std::string) (OP::TYPE) (DATA)

                fParsed = true;

                return true;
            }

            template<typename Type>
            bool GetValue(const std::string& str, Type& value)
            {
                if(!fParsed)
                    return false;

                if(!mapData.count(str))
                    return false;

                nReadPos = mapData[str].first;

                uint8_t nType;
                *this >> nType;

                if(nType == TYPES::UNSUPPORTED)
                    return debug::error(FUNCTION, "unsupported type");

                if(type(value) != nType)
                    return debug::error(FUNCTION, "type mismatch");

                *this >> value;

                return true;
            }


            bool GetValue(const std::string& str, std::string& value)
            {
                if(!fParsed)
                    return false;

                if(!mapData.count(str))
                    return false;

                nReadPos = mapData[str].first;

                uint8_t nType;
                *this >> nType;

                if(nType != TYPES::STRING)
                    return debug::error("INVALID STRING TYPE");

                *this >> value;

                return true;
            }


            template<typename Type>
            bool SetValue(const std::string& str, const Type& value)
            {
                if(!fParsed)
                    return false;

                if(!mapData.count(str))
                    return false;

                //if(type(value) == TYPES::UNSUPPORTED)
                //    return debug::error(FUNCTION, "unsupported type");
                if(!mapData[str].second)
                    return debug::error(FUNCTION, "cannot set value for READONLY data member");

                nReadPos = mapData[str].first;

                uint8_t nType;
                *this >> nType;

                if(nType == TYPES::UNSUPPORTED)
                    return debug::error(FUNCTION, "unsupported type");

                if(type(value) != nType)
                    return debug::error(FUNCTION, "type mismatch");

                //TODO: check expected sizes
                if(nReadPos + sizeof(value) >= vchState.size())
                    return debug::error(FUNCTION, "performing an over-write");

                /* Copy the bytes into tmp object. */
                std::copy((uint8_t*)&value, (uint8_t*)&value + sizeof(value), (uint8_t*)&vchState[nReadPos]);

                return true;
            }


            bool SetValue(const std::string& str, const std::string& value)
            {
                if(!fParsed)
                    return false;

                if(!mapData.count(str))
                    return false;

                if(!mapData[str].second)
                    return debug::error(FUNCTION, "cannot set value for READONLY data member");

                nReadPos = mapData[str].first;

                uint8_t nType;
                *this >> nType;

                if(nType != TYPES::STRING)
                    return debug::error("INVALID STRING TYPE");

                //TODO: check expected sizes
                uint64_t nSize = ReadCompactSize(*this);
                if(nSize != value.size())
                    return debug::error("INVALID STRING SIZES ", nSize, "::", value.size());

                if(nReadPos + nSize >= vchState.size())
                    return debug::error(FUNCTION, "performing an over-write");

                /* Copy the bytes into tmp object. */
                std::copy((uint8_t*)&value[0], (uint8_t*)&value[0] + value.size(), (uint8_t*)&vchState[nReadPos]);

                return true;
            }
        };
    }
}



//This main function is for prototyping new code
//It is accessed by compiling with LIVE_TESTS=1
int main(int argc, char** argv)
{
    using namespace TAO::Register;

    Object object;
    object << std::string("byte") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::UINT8_T) << uint8_t(55)
           << std::string("test") << uint8_t(TYPES::MUTABLE) << uint8_t(TYPES::STRING) << std::string("this string")
           << std::string("balance") << uint8_t(TYPES::UINT64_T) << uint64_t(55)
           << std::string("identifier") << uint8_t(TYPES::STRING) << std::string("NXS");

    runtime::timer timer;
    timer.Start();

    for(int i = 0; i < 1000000; i++)
        object.Parse();

    uint64_t nTime = timer.ElapsedMicroseconds();

    debug::log(0, "Parsed ", 1000000.0 / nTime, " million registers / s");

    uint8_t nTest;
    object.GetValue("byte", nTest);

    debug::log(0, "TEST ", uint32_t(nTest));

    object.SetValue("byte", uint8_t(98));

    uint8_t nTest2;
    object.GetValue("byte", nTest2);

    debug::log(0, "TEST2 ", uint32_t(nTest2));

    std::string strTest;
    object.GetValue("test", strTest);

    debug::log(0, "STRING ", strTest);

    object.SetValue("test", std::string("fail"));
    object.SetValue("test", "fail");

    object.SetValue("test", std::string("THIS string"));

    std::string strGet;
    object.GetValue("test", strGet);

    debug::log(0, strGet);

    std::string identifier;
    object.GetValue("identifier", identifier);

    debug::log(0, "Token Type ", identifier);

    return 0;
}
