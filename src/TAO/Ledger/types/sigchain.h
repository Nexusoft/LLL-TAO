/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_SIGNATURE_CHAIN_H
#define NEXUS_TAO_LEDGER_TYPES_SIGNATURE_CHAIN_H


#include <LLC/types/uint1024.h>

#include <Util/include/allocators.h>
#include <Util/include/mutex.h>
#include <Util/include/memory.h>

#include <string>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** SignatureChain
         *
         *  Base Class for a Signature SignatureChain
         *
         *  Similar to HD wallet systems, but integrated into the Layer 2 of the nexus software stack.
         *  Seed phrase includes at least 128 bits of entropy (8 char username, 8 char password) and pin
         *  to attack a signature chain by dictionary attack.
         *
         */
        class SignatureChain : public memory::encrypted
        {

            /** Secure allocator to represent the username of this signature chain. **/
            const SecureString strUsername;


            /** Secure allocater to represent the password of this signature chain. **/
            const SecureString strPassword;


            /* Internal mutex for caches. */
            mutable std::mutex MUTEX;


            /** Internal sigchain cache (to not exhaust ourselves regenerating the same key). **/
            mutable std::pair<uint32_t, SecureString> pairCache;


            /** Internal genesis hash. **/
            const uint256_t hashGenesis;



        public:



            /** Default constructor. **/
            SignatureChain() = delete;


            /** Copy Constructor **/
            SignatureChain(const SignatureChain& sigchain);


            /** Move Constructor **/
            SignatureChain(SignatureChain&& sigchain) noexcept;


            /** Copy Assignment Operator **/
            SignatureChain& operator=(const SignatureChain& sigchain)    = delete;


            /** Move Assignment Operator **/
            SignatureChain& operator=(SignatureChain&& sigchain) noexcept = delete;


            /** Destructor. **/
            ~SignatureChain();


            /** Constructor to generate Keychain
             *
             *  @param[in] strUsernameIn The username to seed the signature chain
             *  @param[in] strPasswordIn The password to seed the signature chain
             *
             **/
            SignatureChain(const SecureString& strUsernameIn, const SecureString& strPasswordIn);


            /** Genesis
             *
             *  This function is responsible for generating the genesis ID for this sig chain.
             *
             *  @return The 512 bit hash of this key in the series.
             *
             **/
            uint256_t Genesis() const;


            /** Genesis
             *
             *  This function is responsible for generating the genesis ID.
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            static uint256_t Genesis(const SecureString& strUsername);


            /** Generate
             *
             *  This function is responsible for genearting the private key in the sigchain of a specific account.
             *  The sigchain is a series of keys seeded from a secret phrase and a PIN number.
             *
             *  @param[in] nKeyID The key number in the keychian
             *  @param[in] strSecret The secret phrase to use
             *  @param[in] fCache Use the cache on hand for keys.
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            uint512_t Generate(const uint32_t nKeyID, const SecureString& strSecret, bool fCache = true) const;


            /** Generate
             *
             *  This function is responsible for generating the private key in the sigchain with a specific password and pin.
             *  This version should be used when changing the password and/or pin
             *
             *  @param[in] nKeyID The key number in the keychian
             *  @param[in] strPassword The password to use
             *  @param[in] strSecret The secret phrase to use
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            uint512_t Generate(const uint32_t nKeyID, const SecureString& strPassword, const SecureString& strSecret) const;


            /** Generate
             *
             *  This function is responsible for genearting the private key in the sigchain of a specific account.
             *  The sigchain is a series of keys seeded from a secret phrase and a PIN number.
             *
             *  @param[in] strType The type of signing key used.
             *  @param[in] nKeyID The key number in the keychian
             *  @param[in] strSecret The secret phrase to use
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            uint512_t Generate(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret) const;


            /** KeyHash
             *
             *  This function generates a hash of a public key generated from random seed phrase.
             *
             *  @param[in] strType The type of signing key used.
             *  @param[in] nKeyID The key number in the keychian
             *  @param[in] strSecret The secret phrase to use
             *  @param[in] nType The key type to use.
             *
             *  @return The 256 bit hash of this key in the series.
             **/
            uint256_t KeyHash(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret, const uint8_t nType) const;


            /** UserName
             *
             *  Returns the username for this sig chain
             *
             *  @return The SecureString containing the username for this sig chain
             **/
            const SecureString& UserName() const;


            /** Encrypt
             *
             *  Special method for encrypting specific data types inside class.
             *
             **/
            void Encrypt();


            /** Sign
            *
            *  Generates a signature for the data, using the specified crypto key type
            *
            *  @param[in] strType The type of signing key to use
            *  @param[in] vchData The data to base the signature off
            *  @param[in] hashSecret The private key to use for the signature
            *  @param[out] vchPubKey The public key generated from the private key
            *  @param[out] vchSig The signature bytes
            *
            *  @return True if successful
            *
            **/
            bool Sign(const std::string& strType, const std::vector<uint8_t>& vchData, const uint512_t& hashSecret,
                                      std::vector<uint8_t>& vchPubKey, std::vector<uint8_t>& vchSig) const;
        };
    }
}

#endif
