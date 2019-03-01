/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEGACY_TYPES_ADDRESS_H
#define NEXUS_LEGACY_TYPES_ADDRESS_H

#include <Util/include/base58.h>

class uint256_t;

namespace Legacy
{

    /** Base58-encoded Nexus Addresses.
     *
     *  Public-key-hash-addresses have version 42 (or 111 testnet).
     *  Script-hash-addresses have version 104 (or 196 testnet).
     *  The data vector contains SK256(cscript), where cscript is the serialized redemption script.
     **/
    class NexusAddress : public encoding::CBase58Data
    {
    public:

        /** Default Constructor. **/
        NexusAddress()
        {
        }


        /** Constructor
         *
         *  Sets the hash256
         *
         *  @param[in] hash256 The input hash to copy in
         *
         **/
        NexusAddress(uint256_t hash256In);


        /** Constructor
         *
         *  Sets by the public key
         *
         *  @param[in] vchPubKey the byte vector of input data
         *
         **/
        NexusAddress(const std::vector<uint8_t>& vchPubKey);


        /** Constructor
         *
         *  Sets by a std::string
         *
         *  @param[in] strAddress The address string
         *
         **/
        NexusAddress(const std::string& strAddress);


        /** Constructor
         *
         *  Sets by a c-style string
         *
         *  @param[in] pszAddress The address c-style string
         *
         **/
        NexusAddress(const char* pszAddress);


        /** Destructor **/
        virtual ~NexusAddress()
        {
        }


        /** SetHash256
         *
         *  Sets the generated hash for address
         *
         *  @param[in] hash256 The input hash to copy in
         *
         **/
        void SetHash256(const uint256_t& hash256);


        /** SetPubKey
         *
         *  Sets the public key by hashing it to generate address hash
         *
         *  @param[in] vchPubKey the byte vector of input data
         *
         **/
        void SetPubKey(const std::vector<uint8_t>& vchPubKey);


        /** SetScriptHash256
         *
         *  Sets the address based on the hash of the script
         *
         *  @param[in] hash256 the input hash to copy in
         *
         **/
        void SetScriptHash256(const uint256_t& hash256);


        /** IsValid
         *
         *  Preliminary check to ensure the address is mapped to a valid public key or script
         *
         *  @return true if valid address
         *
         **/
        bool IsValid() const;


        /** IsScript
         *
         *  Check of version byte to see if address is P2SH
         *
         *  @return true if valid address
         *
         **/
        bool IsScript() const;


        /** GetHash256
         *
         *  Get the hash from byte vector
         *
         *  @return uint256_t of the hash copy
         *
         **/
        uint256_t GetHash256() const;
    };

}

#endif
