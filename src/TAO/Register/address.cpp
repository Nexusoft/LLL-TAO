/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <TAO/Register/types/address.h>

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


        /* Default constructor. */
        Address::Address(const uint8_t nType)
        : uint256_t(LLC::GetRand256())
        {
            /* Set type. */
            SetType(nType);

            /* Check for valid. */
            if(!IsValid())
                throw debug::exception(FUNCTION, "invalid type for random ", ToString());
        }


        /*Build an address from a hex encoded string.*/
        Address::Address(const std::string& strAddress)
        : uint256_t(strAddress)
        {
            /* Check for valid address types. */
            if(!IsValid())
                throw debug::exception(FUNCTION, "invalid type for string");
        }


        /* Build an address from a name or namespace.*/
        Address::Address(const std::string& strName, const uint8_t nType)
        : uint256_t(LLC::SK256(strName))
        {
            /* Check for valid types. */
            if(nType != NAME && nType!= NAMESPACE)
                throw debug::exception(FUNCTION, "invalid type for names");

            SetType(nType);
        }


        /* Build an address from a name or namespace. */
        Address::Address(const std::vector<uint8_t>& vName, const uint8_t nType)
        : uint256_t(LLC::SK256(vName))
        {
            /* Check for valid types. */
            if(nType != NAME && nType!= NAMESPACE)
                throw debug::exception(FUNCTION, "invalid type for names");

            SetType(nType);
        }


        /* Assignment operator. */
        Address& Address::operator=(const Address& addr)
        {
            /* Copy each word. */
            for(uint32_t i = 0; i < WIDTH; ++i)
                pn[i] = addr.pn[i];

            return *this;
        }


        /* Set the type byte into the address.*/
        void Address::SetType(uint8_t nType)
        {
            /* Check for testnet. */
            //if(nType != 0xff && config::fTestNet.load())
            //    nType += 0x10;

            /* Mask off most significant byte (little endian). */
            pn[WIDTH -1] = (pn[WIDTH - 1] & 0x00ffffff) + (nType << 24);
        }


        /*  Get the type byte from the address.*/
        uint8_t Address::GetType() const
        {
            /* Get type from hash. */
            uint8_t nType = (pn[WIDTH -1] >> 24);

            /* Check for testnet. */
            //if(nType != 0xff && config::fTestNet.load())
            //    nType -= 0x10;

            return nType;
        }


        /* Check if address has a valid type assoicated. */
        bool Address::IsValid() const
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
                case WILDCARD:
                    return true;
            }

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
    }
}
