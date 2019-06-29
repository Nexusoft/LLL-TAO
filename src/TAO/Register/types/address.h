/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_VALUE_H
#define NEXUS_TAO_REGISTER_INCLUDE_VALUE_H

#include <LLC/types/base_uint.h>

namespace TAO
{

    namespace Register
    {

        /** Address
         *
         *  An object that keeps track of a register address type.
         *
         **/
        class Address : public uint256_t
        {
        public:

            /** Different bytes that are prepended to addresses. */
            enum
            {
                READONLY  = 0xd0,
                APPEND    = 0xd1,
                RAW       = 0xd2,
                OBJECT    = 0xd3,
                ACCOUNT   = 0xd4,
                TOKEN     = 0xd5,
                TRUST     = 0xd6,
                NAME      = 0xd7,
                NAMESPACE = 0xd8
            };


            /** Default constructor. **/
            Address()
            : uint256_t(0)
            {
            }


            /** Copy Constructor */
            Address(const uint256_t& hashAddress)
            : uint256_t(hashAddress)
            {
            }


            /** Address Constructor
             *
             *  Build an address from a hex encoded string.
             *
             *  @param[in] strName The name to assign to this address.
             *  @param[in] nType The type of the address (Name or Namespace)
             *
             **/
            Address(const std::string& strAddress)
            : uint256_t(strAddress)
            {
                /* Check for valid address types. */
                if(!IsValid())
                    throw debug::exception(FUNCTION, "invalid type");
            }


            /** Address Constructor
             *
             *  Build an address from a name or namespace.
             *
             *  @param[in] strName The name to assign to this address.
             *  @param[in] nType The type of the address (Name or Namespace)
             *
             **/
            Address(const std::string& strName, const uint8_t nType)
            : uint256_t(LLC::SK256(strName))
            {
                /* Check for valid types. */
                if(nType != NAME && nType!= NAMESPACE)
                    throw debug::exception(FUNCTION, "invalid type for names");

                SetType(nType);
            }


            /** Assignment operator.
             *
             *  @param[in] addr Address to assign this to.
             *
             **/
            Address& operator=(const Address& addr)
            {
                /* Copy each word. */
                for(uint32_t i = 0; i < WIDTH; ++i)
                    pn[i] = addr.pn[i];

                return *this;
            }


            /** SetType
             *
             *  Set the type byte into the address.
             *
             *  @param[in] nType The type byte for address.
             *
             **/
            void SetType(const uint8_t nType)
            {
                /* Mask off most significant byte (little endian). */
                pn[WIDTH -1] = (pn[WIDTH - 1] & 0x00ffffff) + (nType << 24);
            }


            /** GetType
             *
             *  Get the type byte from the address.
             *
             *  @param[in] nType The type byte for address.
             *
             **/
            uint8_t GetType() const
            {
                return (pn[WIDTH -1] >> 24);
            }


            /** IsValid
             *
             *  Check if address has a valid type assoicated.
             *
             *  @return True if type has valid header byte.
             *
             **/
            bool IsValid() const
            {
                /* Get the type. */
                uint8_t nType = GetType();

                /* Return on valid types. */
                switch(nType)
                {
                    case READONLY:
                    case APPEND:
                    case RAW:
                    case OBJECT:
                    case ACCOUNT:
                    case TOKEN:
                    case TRUST:
                    case NAME:
                    case NAMESPACE:
                        return true;
                }

                return false;
            }


            /** IsReadonly
             *
             *  Check if type is set to READONLY.
             *
             *  @return True if using READONLY type.
             *
             **/
            bool IsReadonly() const
            {
                return GetType() == READONLY;
            }


            /** IsAppend
             *
             *  Check if type is set to APPEND.
             *
             *  @return True if using APPEND type.
             *
             **/
            bool IsAppend() const
            {
                return GetType() == APPEND;
            }


            /** IsRaw
             *
             *  Check if type is set to RAW.
             *
             *  @return True if using RAW type.
             *
             **/
            bool IsRaw() const
            {
                return GetType() == RAW;
            }


            /** IsObject
             *
             *  Check if type is set to OBJECT.
             *
             *  @return True if using OBJECT type.
             *
             **/
            bool IsObject() const
            {
                return GetType() == OBJECT;
            }


            /** IsAccount
             *
             *  Check if type is set to ACCOUNT.
             *
             *  @return True if using ACCOUNT type.
             *
             **/
            bool IsAccount() const
            {
                return GetType() == ACCOUNT;
            }


            /** IsToken
             *
             *  Check if type is set to TOKEN.
             *
             *  @return True if using TOKEN type.
             *
             **/
            bool IsToken() const
            {
                return GetType() == TOKEN;
            }


            /** IsTrust
             *
             *  Check if type is set to TRUST.
             *
             *  @return True if using TRUST type.
             *
             **/
            bool IsTrust() const
            {
                return GetType() == TRUST;
            }


            /** IsName
             *
             *  Check if type is set to NAME.
             *
             *  @return True if using NAME type.
             *
             **/
            bool IsName() const
            {
                return GetType() == NAME;
            }


            /** IsNamespace
             *
             *  Check if type is set to NAMESPACE.
             *
             *  @return True if using NAMESPACE type.
             *
             **/
            bool IsNamespace() const
            {
                return GetType() == NAMESPACE;
            }
        };
    }
}

#endif
