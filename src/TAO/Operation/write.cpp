/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>
#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/types/state.h>
#include <TAO/Register/include/system.h>
#include <TAO/Register/include/reserved.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Writes data to a register. */
        bool Write(TAO::Register::State& state, const std::vector<uint8_t>& vchData, const uint64_t nTimestamp)
        {
            /* Check for valid register types. */
            if(state.nType == TAO::Register::REGISTER::READONLY
            || state.nType == TAO::Register::REGISTER::APPEND)
                return debug::error(FUNCTION, "not allowed on readonly or append types");

            /* Write operations on the state object. */
            if(state.nType == TAO::Register::REGISTER::OBJECT)
            {
                /* Create the object register. */
                TAO::Register::Object object = TAO::Register::Object(state);

                /* Parse the object register. */
                if(!object.Parse())
                    return debug::error(FUNCTION, "object register failed to parse");

                /* Loop through the stream.
                 *
                 * Types here are stored in a special stream that
                 * are write instructions for object registers.
                 *
                 */
                Stream stream = Stream(vchData);
                while(!stream.end())
                {
                    /* Deserialize the named value. */
                    std::string strName;
                    stream >> strName;

                    /* Manually check reserved field names for now. */
                    if(TAO::Register::Reserved(strName))
                        return debug::error(FUNCTION, "cannot use reserved '", strName, "' field");

                    //TODO: maybe we should catch duplicates?
                    //no real point being that it will just overwrite a value in same transaction
                    //as long as it is mutable.
                    //but it might be best to be stricter here. to decide

                    /* Deserialize the type. */
                    uint8_t nType;
                    stream >> nType;

                    /* Switch between supported types. */
                    switch(nType)
                    {

                        /* Standard type for C++ uint8_t. */
                        case OP::TYPES::UINT8_T:
                        {
                            /* Get the byte from the stream. */
                            uint8_t nValue;
                            stream >> nValue;

                            /* Attempt to write it to register. */
                            if(!object.Write(strName, nValue))
                                return debug::error(FUNCTION, "failed to write uint8_t value");

                            break;
                        }


                        /* Standard type for C++ uint16_t. */
                        case OP::TYPES::UINT16_T:
                        {
                            /* Get the byte from the stream. */
                            uint16_t nValue;
                            stream >> nValue;

                            /* Attempt to write it to register. */
                            if(!object.Write(strName, nValue))
                                return debug::error(FUNCTION, "failed to write uint16_t value");

                            break;
                        }


                        /* Standard type for C++ uint32_t. */
                        case OP::TYPES::UINT32_T:
                        {
                            /* Get the byte from the stream. */
                            uint32_t nValue;
                            stream >> nValue;

                            /* Attempt to write it to register. */
                            if(!object.Write(strName, nValue))
                                return debug::error(FUNCTION, "failed to write uint32_t value");

                            break;
                        }


                        /* Standard type for C++ uint64_t. */
                        case OP::TYPES::UINT64_T:
                        {
                            /* Get the byte from the stream. */
                            uint64_t nValue;
                            stream >> nValue;

                            /* Attempt to write it to register. */
                            if(!object.Write(strName, nValue))
                                return debug::error(FUNCTION, "failed to write uint64_t value");

                            break;
                        }


                        /* Standard type for Custom uint256_t */
                        case OP::TYPES::UINT256_T:
                        {
                            /* Get the byte from the stream. */
                            uint256_t nValue;
                            stream >> nValue;

                            /* Attempt to write it to register. */
                            if(!object.Write(strName, nValue))
                                return debug::error(FUNCTION, "failed to write uint256_t value");

                            break;
                        }


                        /* Standard type for Custom uint512_t */
                        case OP::TYPES::UINT512_T:
                        {
                            /* Get the byte from the stream. */
                            uint512_t nValue;
                            stream >> nValue;

                            /* Attempt to write it to register. */
                            if(!object.Write(strName, nValue))
                                return debug::error(FUNCTION, "failed to write uint512_t value");

                            break;
                        }


                        /* Standard type for Custom uint1024_t */
                        case OP::TYPES::UINT1024_T:
                        {
                            /* Get the byte from the stream. */
                            uint1024_t nValue;
                            stream >> nValue;

                            /* Attempt to write it to register. */
                            if(!object.Write(strName, nValue))
                                return debug::error(FUNCTION, "failed to write uint1024_t value");

                            break;
                        }


                        /* Standard type for STL string */
                        case OP::TYPES::STRING:
                        {
                            /* Get the byte from the stream. */
                            std::string strValue;
                            stream >> strValue;

                            /* Attempt to write it to register. */
                            if(!object.Write(strName, strValue))
                                return debug::error(FUNCTION, "failed to write string");

                            break;
                        }


                        /* Standard type for STL vector with C++ type uint8_t */
                        case OP::TYPES::BYTES:
                        {
                            /* Get the byte from the stream. */
                            std::vector<uint8_t> vData;
                            stream >> vData;

                            /* Attempt to write it to register. */
                            if(!object.Write(strName, vData))
                                return debug::error(FUNCTION, "failed to write bytes");

                            break;
                        }


                        /* Fail if types are unknown. */
                        default:
                            return debug::error(FUNCTION, "invalid types ", uint32_t(nType));
                    }
                }

                /* If all succeeded, set the new state. */
                state.SetState(object.vchState);
            }

            /* Catch for all state register types except READONLY. */
            else
            {
                /* Check the new data size against register's allocated size. */
                if(vchData.size() != state.vchState.size())
                    return debug::error(FUNCTION, "size mismatch");

                /* For all non objects, write the state as raw byte sequence. */
                state.SetState(vchData);
            }

            /* Update the state register checksum. */
            state.nModified = nTimestamp;
            state.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "register is in invalid state");

            return true;
        }
    }
}
