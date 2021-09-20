/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLD/include/version.h>

#include <Util/templates/serialize.h>
#include <Util/templates/basestream.h>

#include <TAO/Operation/include/enum.h>

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO::API::Contracts
{
    /** Stream
     *
     *  Class to handle the serializaing and deserializing of operations and their types
     *
     **/
    class Params : public BaseStream
    {
        /* Keep this private for internal use. */
        using OP          = TAO::Operation::OP;

    protected:

        /* Vector to track our current parameter positions. */
        std::vector<uint64_t> vParams;

        /* Track our current write position. */
        mutable uint64_t nWritePos;

    public:

        /** Default Constructor. **/
        Params()
        : BaseStream ( )
        , vParams    ( )
        , nWritePos  (0)
        {
        }


        /** Data Constructor.
         *
         *  @param[in] vchDataIn The byte vector to insert.
         *
         **/
        Params(const std::vector<uint8_t>& vchDataIn)
        : BaseStream (vchDataIn)
        , vParams    ( )
        , nWritePos  (vchDataIn.size())
        {
        }


        /** Copy Constructor. **/
        Params(const Params& params)
        : BaseStream (params.vchData)
        , vParams    (params.vParams)
        , nWritePos  (params.nWritePos)
        {
        }


        /** Move Constructor. **/
        Params(Params&& params) noexcept
        : BaseStream (std::move(params.vchData))
        , vParams    (std::move(params.vParams))
        , nWritePos  (std::move(params.nWritePos))
        {
        }


        /** Copy Assignment. **/
        Params& operator=(const Params& params)
        {
            vchData   = params.vchData;
            nReadPos  = params.nReadPos;

            vParams   = params.vParams;
            nWritePos = params.nWritePos;

            return *this;
        }


        /** Move Assignment. **/
        Params& operator=(Params&& params)
        {
            vchData   = std::move(params.vchData);
            nReadPos  = std::move(params.nReadPos);

            vParams   = std::move(params.vParams);
            nWritePos = std::move(params.nWritePos);

            return *this;
        }


        /** Default Destructor **/
        virtual ~Params()
        {
        }


        IMPLEMENT_SERIALIZE
        (
            READWRITE(vchData);
            READWRITE(vParams);

            nWritePos = vchData.size();
        )



        template<typename Stream>
        void add(Stream &ssBuild, const uint32_t nIndex, const uint8_t nType)
        {
            /* Check for a valid parameter type. */
            switch(nType)
            {
                /* Handle for standard 8-bit unsigned integer. */
                case OP::TYPES::UINT8_T:
                {
                    ssBuild << uint8_t(OP::TYPES::UINT8_T) << get<uint8_t>(nIndex);
                    return;
                }

                /* Handle for standard 16-bit unsigned integer. */
                case OP::TYPES::UINT16_T:
                {
                    ssBuild << uint8_t(OP::TYPES::UINT16_T) << get<uint16_t>(nIndex);
                    return;
                }

                /* Handle for standard 32-bit unsigned integer. */
                case OP::TYPES::UINT32_T:
                {
                    ssBuild << uint8_t(OP::TYPES::UINT32_T) << get<uint32_t>(nIndex);
                    return;
                }

                /* Handle for standard 64-bit unsigned integer. */
                case OP::TYPES::UINT64_T:
                {
                    ssBuild << uint8_t(OP::TYPES::UINT64_T) << get<uint64_t>(nIndex);
                    return;
                }

                /* Handle for standard 256-bit unsigned integer. */
                case OP::TYPES::UINT256_T:
                {
                    ssBuild << uint8_t(OP::TYPES::UINT256_T) << get<uint256_t>(nIndex);
                    return;
                }

                /* Handle for standard 512-bit unsigned integer. */
                case OP::TYPES::UINT512_T:
                {
                    ssBuild << uint8_t(OP::TYPES::UINT512_T) << get<uint512_t>(nIndex);
                    return;
                }

                /* Handle for standard 1024-bit unsigned integer. */
                case OP::TYPES::UINT1024_T:
                {
                    ssBuild << uint8_t(OP::TYPES::UINT1024_T) << get<uint1024_t>(nIndex);
                    return;
                }

                /* Handle for standard byte vector. */
                case OP::TYPES::BYTES:
                {
                    ssBuild << uint8_t(OP::TYPES::BYTES) << get<std::vector<uint8_t>>(nIndex);
                    return;
                }

                /* Handle for standard strings. */
                case OP::TYPES::STRING:
                {
                    ssBuild << uint8_t(OP::TYPES::BYTES) << get<std::string>(nIndex);
                    return;
                }

                /* Handle for pre-state value fields. */
                case OP::CALLER::PRESTATE::VALUE: //this instruction has a string parameter
                {
                    ssBuild << uint8_t(OP::CALLER::PRESTATE::VALUE) << get<std::string>(nIndex);
                    return;
                }

                /* Handle for register codes. */
                case OP::REGISTER::CREATED:
                case OP::REGISTER::MODIFIED:
                case OP::REGISTER::OWNER:
                case OP::REGISTER::TYPE:
                case OP::REGISTER::STATE:
                {
                    ssBuild << nType << get<uint256_t>(nIndex);
                    return;
                }

                /* Default case is an exception. */
                default:
                    throw debug::exception(FUNCTION, "requesting parameter is not a valid type");
            }
        }


        /** get
         *
         *  Get's a parameter from given parameter index.
         *
         *  @param[in] nIndex The parameter index.
         *
         *  @return The given parameter deserialized.
         *
         **/
        template<typename Type>
        Type get(const uint32_t nIndex)
        {
            /* Check for access violations. */
            if(nIndex >= vParams.size())
                throw debug::exception(FUNCTION, "requesting parameter outside of scope");

            /* Set our read position to parameter position. */
            nReadPos = vParams[nIndex];

            /* Deserialize the object. */
            Type obj;
            ::Unserialize(*this, obj, (uint32_t)SER_REGISTER, LLD::DATABASE_VERSION); //TODO: version should be object version

            return obj;
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchOperations
         *
         *  @param[in] obj The object to serialize into ledger data
         *
         **/
        template<typename Type>
        Params& operator<<(const Type& obj)
        {
            /* Get our write size. */
            const uint64_t nSize =
                ::GetSerializeSize(obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION);

            /* Serialize to the stream. */
            ::Serialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

            /* Add our new parameter to binary positions. */
            vParams.push_back(nWritePos);

            /* Adjust our current write position. */
            nWritePos += nSize;

            return (*this);
        }
    };
}
