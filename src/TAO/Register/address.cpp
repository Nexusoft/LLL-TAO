/*__________________________________________________________________________________________

        (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

        (c) Copyright The Nexus Developers 2014 - 2021

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/address.h>

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/address.h>

#include <Util/include/encoding.h>
#include <Util/include/debug.h>

namespace TAO
{

    namespace Register
    {

        /* Default constructor. */
        Address::Address()
        : uint256_t(0)
        {
        }


        /* Build from uint256_t.. */
        Address::Address(const uint256_t& hashAddress)
        : uint256_t(hashAddress)
        {
        }


        /* Move from uint256_t hash. */
        Address::Address(uint256_t&& hashAddress) noexcept
        : uint256_t(std::move(hashAddress))
        {
        }


        /* Assignment operator. */
        Address& Address::operator=(const uint256_t& value)
        {
            for(uint8_t i = 0; i < WIDTH; ++i)
                pn[i] = value.pn[i];

            return *this;
        }


        /* Move assignment operator. */
        Address& Address::operator=(uint256_t&& value) noexcept
        {
            for(uint8_t i = 0; i < WIDTH; ++i)
                pn[i] = std::move(value.pn[i]);

            return *this;
        }


        /* Default constructor. */
        Address::Address(const uint8_t nType)
        : uint256_t(LLC::GetRand256())
        {
            /* Set type. */
            SetType(nType);

            /* Check for valid. */
            if(!IsValid())
                throw debug::exception(FUNCTION, "invalid type for random ", ToBase58());
        }


        /* Build an address from a base58 encoded string.*/
        Address::Address(const std::string& strAddress)
        : uint256_t()
        {
            /* Set the internal value from the incoming base58 encoded address */
            SetBase58(strAddress);
        }


        /* Build an address deterministically from a namespace name */
        Address::Address(const std::string& strName, const uint8_t nType)
        : uint256_t(LLC::SK256(strName))
        {
            /* Check for valid types. */
            if(nType != NAMESPACE)
                throw debug::exception(FUNCTION, "invalid type for namespace");

            SetType(nType);
        }


        /* Build an address deterministically from a key and namespace hash. */
        Address::Address(const std::string& strKey, const uint256_t& hashNamespace, const uint8_t nType)
        {
            /* The data to hash into this address */
            std::vector<uint8_t> vData;

            /* Insert the key */
            vData.insert(vData.end(), strKey.begin(), strKey.end());

            /* Insert the genessis hash */
            vData.insert(vData.end(), (uint8_t*)&hashNamespace, (uint8_t*)&hashNamespace + 32);

            /* Set the internal uint256 data based on the SK hash of the vData */
            *this = LLC::SK256(vData);

            /* Check for valid types. */
            if(nType != TRUST && nType != NAME && nType != CRYPTO)
                throw debug::exception(FUNCTION, "invalid type for deterministic address");

            SetType(nType);
        }


        /* Check if address has a valid type assoicated. */
        bool Address::IsValid() const
        {
            /* Check for invalid address ranges. */
            if(*this <= uint256_t(SYSTEM::LIMIT)) //we don't use Rserved as this would result in recursive call loop
                return false;

            /* Return on valid types. */
            switch(GetType())
            {
                case READONLY:
                case APPEND:
                case RAW:
                case OBJECT:
                case CRYPTO:
                case ACCOUNT:
                case TOKEN:
                case TRUST:
                case NAME:
                case NAMESPACE:
                case WILDCARD:
                    return true;
            }

            /* Check if is legacy. */
            if(IsLegacy())
                return true;

            return false;
        }


        /* Check if type is set to READONLY.*/
        bool Address::IsReadonly() const
        {
            return GetType() == READONLY;
        }


        /* Check if type is set to APPEND.*/
        bool Address::IsAppend() const
        {
            return GetType() == APPEND;
        }


        /*  Check if type is set to RAW.*/
        bool Address::IsRaw() const
        {
            return GetType() == RAW;
        }


        /* Check if type is set to OBJECT. */
        bool Address::IsObject() const
        {
            return GetType() == OBJECT;
        }

        /* Check if type is set to CRYPTO. */
        bool Address::IsCrypto() const
        {
            return GetType() == CRYPTO;
        }


        /* Check if type is set to ACCOUNT. */
        bool Address::IsAccount() const
        {
            return GetType() == ACCOUNT;
        }


        /* Check if type is set to TOKEN. */
        bool Address::IsToken() const
        {
            return GetType() == TOKEN;
        }


        /* Check if type is set to TRUST. */
        bool Address::IsTrust() const
        {
            return GetType() == TRUST;
        }


        /*  Check if type is set to NAME. */
        bool Address::IsName() const
        {
            return GetType() == NAME;
        }


        /* Check if type is set to NAMESPACE. */
        bool Address::IsNamespace() const
        {
            return GetType() == NAMESPACE;
        }


        /* Check if type is set to WILDCARD. */
        bool Address::IsWildcard() const
        {
            return GetType() == WILDCARD;
        }


        /* Check if base hash maps to a valid legacy address. */
        bool Address::IsLegacy() const
        {
            /* Build a legacy address for this check. */
            Legacy::NexusAddress addr =
                Legacy::NexusAddress(*this);

            return addr.IsValid() && ToBase58()[0] != '8';
            //return GetType() == LEGACY || GetType() == LEGACY_TESTNET;
        }


        /* Sets the uint256_t value of this address from a base58 encoded string. */
        void Address::SetBase58(const std::string& str)
        {
            /* The decoded bytes  */
            std::vector<uint8_t> vBytes;

            /* Decode the incoming string */
            if(encoding::DecodeBase58Check(str, vBytes))
            {
                /* Check for legaacy address char. */
                if(str[0] == '2' && (vBytes[0] == LEGACY || vBytes[0] == LEGACY_TESTNET))
                {
                    /* Build a legacy address for this check. */
                    const Legacy::NexusAddress addr =
                        Legacy::NexusAddress(str);

                    /* Special check for legacy address. */
                    if(addr.IsValid())
                    {
                        *this = addr.GetHash256();
                        return;
                    }
                }

                /* Set the internal value based on the remainder of the decoded bytes after the leading type byte */
                SetBytes(std::vector<uint8_t>(vBytes.begin() + 1, vBytes.end()));

                /* Set the type */
                SetType(vBytes[0]);
            }
        }


        /* Returns a Base58 encoded string representation of the 256-bit address hash. */
        std::string Address::ToBase58() const
        {
            /* Get the bytes for the 256-bit hash */
            std::vector<uint8_t> vch = GetBytes();

            /* Insert the type identifier byte */
            vch.insert(vch.begin(), GetType());

            /* encode the bytes and return the resultant string */
            return encoding::EncodeBase58Check(vch);
        }

        /* Returns a base58 encoded string representation of the address. */
        std::string Address::ToString() const
        {
            if(*this == 0)
                return "0";
            else if(GetType() == SYSTEM || (GetType() >= RESERVED1 && GetType() <= RESERVED2))
                return GetHex();
            else if(IsLegacy())
                return Legacy::NexusAddress(*this).ToString();
            else
                return ToBase58();
        }
    }
}
