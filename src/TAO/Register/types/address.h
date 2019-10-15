/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_ADDRESS_H
#define NEXUS_TAO_REGISTER_INCLUDE_ADDRESS_H

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

                /* To identify legacy addresses */
                LEGACY = 0x2a,
                LEGACY_TESTNET = 0x6f,

                /* Standard register type bytes.
                   NOTE: These MUST be between 0xd1 and 0xed for the base58 encoded string to start with 8
                */
                READONLY  = 0xd1,
                APPEND    = 0xd2,
                RAW       = 0xd3,
                OBJECT    = 0xd4,
                CRYPTO    = 0xd5,
                ACCOUNT   = 0xd6,
                TOKEN     = 0xd7,
                TRUST     = 0xd8,
                NAME      = 0xd9,
                NAMESPACE = 0xda,

                WILDCARD  = 0xff
            };


            /** Default constructor. **/
            Address();


            /** Address Constructor
             *
             *  Build from uint256_t hash.
             *
             *  @param[in] hashAddress The hash.
             *
             **/
            Address(const uint256_t& hashAddress);


            /** Address Move Constructor
             *
             *  Build from uint256_t hash.
             *
             *  @param[in] hashAddress The hash.
             *
             **/
            Address(uint256_t&& hashAddress) noexcept;


            /** Assignment operator.
             *
             *  @param[in] value The value to assign this to.
             *
             **/
            Address& operator=(const uint256_t& value);


            /** Assignment operator.
             *
             *  @param[in] value The value to assign this to.
             *
             **/
            Address& operator=(uint256_t&& value) noexcept;


            /** Address Constructor
             *
             *  Build from type and random address.
             *
             *  @param[in] nType The type enum to create .
             *
             **/
            Address(const uint8_t nType);


            /** Address Constructor
             *
             *  Build an address from a base58 encoded string.
             *
             *  @param[in] strName The name to assign to this address.
             *
             **/
            Address(const std::string& strAddress);


            /** Address Constructor
             *
             *  Build an address deterministically from a namespace name.
             *
             *  @param[in] strName The name to assign to this address.
             *  @param[in] nType The type of the address (NAMESPACE)
             *
             **/
            Address(const std::string& strName, const uint8_t nType);


            /** Address Constructor
             *
             *  Build an address deterministically from a key and namespace hash.
             *
             *  @param[in] strKey The key to combine with the hash to generate this address.
             *  @param[in] hashNamespace The hash to combine with the key to generate this address.
             *  @param[in] nType The type of the address (NAME or TRUST)
             *
             **/
            Address(const std::string& strKey, const uint256_t& hashNamespace, const uint8_t nType);


            /** IsValid
             *
             *  Check if address has a valid type associated.
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


            /** IsCrypto
             *
             *  Check if type is set to CRYPTO.
             *
             *  @return True if using CRYPTO type.
             *
             **/
            bool IsCrypto() const;


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


            /** IsWildcard
             *
             *  Check if type is set to WILDCARD.
             *
             *  @return True if using WILDCARD type.
             *
             **/
            bool IsWildcard() const;


            /** IsLegacy
             *
             *  Check if type is set to LEGACY or LEGACY_TESTNET.
             *
             *  @return True if using LEGACY or LEGACY_TESTNET type.
             *
             **/
            bool IsLegacy() const;


            /** SetBase58
             *
             *  Sets the uint256_t value of this address from a base58 encoded string.
             *
             *  @param[in] str The base58 encoded string representation of the address.
             *
             **/
            void SetBase58(const std::string& str);

            /** ToBase58
             *
             *  Returns a Base58 encoded string representation of the 256-bit address hash.
             *
             **/
            std::string ToBase58() const;

            /** ToString
             *
             *  Returns a base58 encoded string representation of the address .
             *
             **/
            std::string ToString() const;
        };
    }
}

#endif
