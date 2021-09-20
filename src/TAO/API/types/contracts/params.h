/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <LLD/include/version.h>

#include <TAO/Operation/include/enum.h>

#include <Util/templates/serialize.h>
#include <Util/templates/basestream.h>

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
        std::vector<std::pair<uint8_t, uint64_t>> vParams;

    public:

        /** Default Constructor. **/
        Params()
        : BaseStream ( )
        , vParams    ( )
        {
        }


        /** Copy Constructor. **/
        Params(const Params& params)
        : BaseStream (params.vchData)
        , vParams    (params.vParams)
        {
        }


        /** Move Constructor. **/
        Params(Params&& params) noexcept
        : BaseStream (std::move(params.vchData))
        , vParams    (std::move(params.vParams))
        {
        }


        /** Copy Assignment. **/
        Params& operator=(const Params& params)
        {
            vchData   = params.vchData;
            nReadPos  = params.nReadPos;

            vParams   = params.vParams;

            return *this;
        }


        /** Move Assignment. **/
        Params& operator=(Params&& params)
        {
            vchData   = std::move(params.vchData);
            nReadPos  = std::move(params.nReadPos);

            vParams   = std::move(params.vParams);

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
        )


        bool check(const uint32_t nIndex, const uint8_t nType)
        {
            if(nIndex >= vParams.size())
                return debug::error(FUNCTION, "index out of range");

            /* Switch based on type so we can adjust our register op-code based on argument checks. */
            switch(nType)
            {
                /* Handle for register codes. */
                case OP::REGISTER::CREATED:
                case OP::REGISTER::MODIFIED:
                case OP::REGISTER::OWNER:
                case OP::REGISTER::TYPE:
                case OP::REGISTER::STATE:
                {
                    /* Check that we have correct argument type. */
                    const bool fCheck =
                        (vParams[nIndex].first == OP::TYPES::UINT256_T);

                    /* Adjust value if passed. */
                    if(fCheck)
                    {
                        /* Adjust to correct op-code if using register command. */
                        vParams[nIndex].first = nType;
                        return true;
                    }

                    return false;
                }
            }

            return nType == vParams[nIndex].first;
        }


        void bind(TAO::Operation::Contract &rContract, const uint32_t nIndex)
        {
            /* Get the type from parameter index. */
            const uint8_t nType =
                vParams[nIndex].first;

            /* Check for a valid parameter type. */
            switch(nType)
            {
                /* Handle for standard 8-bit unsigned integer. */
                case OP::TYPES::UINT8_T:
                {
                    rContract <= nType <= get<uint8_t>(nIndex);
                    return;
                }

                /* Handle for standard 16-bit unsigned integer. */
                case OP::TYPES::UINT16_T:
                {
                    rContract <= nType <= get<uint16_t>(nIndex);
                    return;
                }

                /* Handle for standard 32-bit unsigned integer. */
                case OP::TYPES::UINT32_T:
                {
                    rContract <= nType <= get<uint32_t>(nIndex);
                    return;
                }

                /* Handle for standard 64-bit unsigned integer. */
                case OP::TYPES::UINT64_T:
                {
                    rContract <= nType <= get<uint64_t>(nIndex);
                    return;
                }

                /* Handle for standard 256-bit unsigned integer. */
                case OP::TYPES::UINT256_T:
                {
                    rContract <= nType <= get<uint256_t>(nIndex);
                    return;
                }

                /* Handle for standard 512-bit unsigned integer. */
                case OP::TYPES::UINT512_T:
                {
                    rContract <= nType <= get<uint512_t>(nIndex);
                    return;
                }

                /* Handle for standard 1024-bit unsigned integer. */
                case OP::TYPES::UINT1024_T:
                {
                    rContract <= nType <= get<uint1024_t>(nIndex);
                    return;
                }

                /* Handle for standard byte vector. */
                case OP::TYPES::BYTES:
                {
                    rContract <= nType <= get<std::vector<uint8_t>>(nIndex);
                    return;
                }

                /* Handle for standard strings. */
                case OP::TYPES::STRING:
                {
                    rContract <= nType <= get<std::string>(nIndex);
                    return;
                }

                /* Handle for pre-state value fields. */
                case OP::CALLER::PRESTATE::VALUE: //this instruction has a string parameter
                {
                    rContract <= nType <= get<std::string>(nIndex);
                    return;
                }

                /* Handle for register codes. */
                case OP::REGISTER::CREATED:
                case OP::REGISTER::MODIFIED:
                case OP::REGISTER::OWNER:
                case OP::REGISTER::TYPE:
                case OP::REGISTER::STATE:
                {
                    rContract <= nType <= get<uint256_t>(nIndex);
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
            nReadPos = vParams[nIndex].second;

            /* Deserialize the object. */
            Type obj;
            ::Unserialize(*this, obj, (uint32_t)SER_REGISTER, LLD::DATABASE_VERSION); //TODO: version should be object version

            return obj;
        }

        uint64_t max() const
        {
            return vParams.size() - 1;
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchData
         *
         **/
        Params& operator<<(const uint8_t& obj)
        {
            /* Add our new parameter to binary positions. */
            vParams.push_back(std::make_pair(OP::TYPES::UINT8_T, vchData.size()));

            /* Serialize to the stream. */
            ::Serialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

            return (*this);
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchData
         *
         **/
        Params& operator<<(const uint16_t& obj)
        {
            /* Add our new parameter to binary positions. */
            vParams.push_back(std::make_pair(OP::TYPES::UINT16_T, vchData.size()));

            /* Serialize to the stream. */
            ::Serialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

            return (*this);
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchData
         *
         **/
        Params& operator<<(const uint32_t& obj)
        {
            /* Add our new parameter to binary positions. */
            vParams.push_back(std::make_pair(OP::TYPES::UINT32_T, vchData.size()));

            /* Serialize to the stream. */
            ::Serialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

            return (*this);
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchData
         *
         **/
        Params& operator<<(const uint64_t& obj)
        {
            /* Add our new parameter to binary positions. */
            vParams.push_back(std::make_pair(OP::TYPES::UINT64_T, vchData.size()));

            /* Serialize to the stream. */
            ::Serialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

            return (*this);
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchData
         *
         **/
        Params& operator<<(const uint256_t& obj)
        {
            /* Add our new parameter to binary positions. */
            vParams.push_back(std::make_pair(OP::TYPES::UINT256_T, vchData.size()));

            /* Serialize to the stream. */
            ::Serialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

            return (*this);
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchData
         *
         **/
        Params& operator<<(const uint512_t& obj)
        {
            /* Add our new parameter to binary positions. */
            vParams.push_back(std::make_pair(OP::TYPES::UINT512_T, vchData.size()));

            /* Serialize to the stream. */
            ::Serialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

            return (*this);
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchData
         *
         **/
        Params& operator<<(const uint1024_t& obj)
        {
            /* Add our new parameter to binary positions. */
            vParams.push_back(std::make_pair(OP::TYPES::UINT1024_T, vchData.size()));

            /* Serialize to the stream. */
            ::Serialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

            return (*this);
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchData
         *
         **/
        Params& operator<<(const std::vector<uint8_t>& obj)
        {
            /* Add our new parameter to binary positions. */
            vParams.push_back(std::make_pair(OP::TYPES::BYTES, vchData.size()));

            /* Serialize to the stream. */
            ::Serialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

            return (*this);
        }


        /** Operator Overload <<
         *
         *  Serializes data into vchData
         *
         **/
        Params& operator<<(const std::string& obj)
        {
            /* Add our new parameter to binary positions. */
            vParams.push_back(std::make_pair(OP::TYPES::STRING, vchData.size()));

            /* Serialize to the stream. */
            ::Serialize(*this, obj, (uint32_t)SER_OPERATIONS, LLD::DATABASE_VERSION); //temp versinos for now

            return (*this);
        }
    };
}
