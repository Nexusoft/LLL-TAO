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

#include <LLC/types/uint1024.h>

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
                /* These are here to prevent you from making the mistake of using these two 'genesis' enum values. */
                RESERVED  = 0xa1,
                RESERVED2 = 0xa2,

                /* Standard register type bytes. */
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
            Address();


            /** Build from type and random address. */
            Address(const uint8_t nType);


            /** Address Constructor
             *
             *  Build an address from a hex encoded string.
             *
             *  @param[in] strName The name to assign to this address.
             *  @param[in] nType The type of the address (Name or Namespace)
             *
             **/
            Address(const std::string& strAddress);


            /** Address Constructor
             *
             *  Build an address from a name or namespace.
             *
             *  @param[in] strName The name to assign to this address.
             *  @param[in] nType The type of the address (Name or Namespace)
             *
             **/
            Address(const std::string& strName, const uint8_t nType);


            /** Address Constructor
             *
             *  Build an address from a name or namespace.
             *
             *  @param[in] vName The name to assign to this address.
             *  @param[in] nType The type of the address (Name or Namespace)
             *
             **/
            Address(const std::vector<uint8_t>& vName, const uint8_t nType);


            /** Assignment operator.
             *
             *  @param[in] addr Address to assign this to.
             *
             **/
            Address& operator=(const Address& addr);


            /** SetType
             *
             *  Set the type byte into the address.
             *
             *  @param[in] nType The type byte for address.
             *
             **/
            void SetType(uint8_t nType);


            /** GetType
             *
             *  Get the type byte from the address.
             *
             *  @param[in] nType The type byte for address.
             *
             **/
            uint8_t GetType() const;


            /** IsValid
             *
             *  Check if address has a valid type assoicated.
             *
             *  @return True if type has valid header byte.
             *
             **/
            bool IsValid() const;


            /** IsReadonly
             *
             *  Check if type is set to READONLY.
             *
             *  @return True if using READONLY type.
             *
             **/
            bool IsReadonly() const;


            /** IsAppend
             *
             *  Check if type is set to APPEND.
             *
             *  @return True if using APPEND type.
             *
             **/
            bool IsAppend() const;


            /** IsRaw
             *
             *  Check if type is set to RAW.
             *
             *  @return True if using RAW type.
             *
             **/
            bool IsRaw() const;


            /** IsObject
             *
             *  Check if type is set to OBJECT.
             *
             *  @return True if using OBJECT type.
             *
             **/
            bool IsObject() const;


            /** IsAccount
             *
             *  Check if type is set to ACCOUNT.
             *
             *  @return True if using ACCOUNT type.
             *
             **/
            bool IsAccount() const;


            /** IsToken
             *
             *  Check if type is set to TOKEN.
             *
             *  @return True if using TOKEN type.
             *
             **/
            bool IsToken() const;


            /** IsTrust
             *
             *  Check if type is set to TRUST.
             *
             *  @return True if using TRUST type.
             *
             **/
            bool IsTrust() const;


            /** IsName
             *
             *  Check if type is set to NAME.
             *
             *  @return True if using NAME type.
             *
             **/
            bool IsName() const;


            /** IsNamespace
             *
             *  Check if type is set to NAMESPACE.
             *
             *  @return True if using NAMESPACE type.
             *
             **/
            bool IsNamespace() const;
        };
    }
}

#endif
