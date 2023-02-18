/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_TYPES_SIGNATURE_CHAIN_H
#define NEXUS_TAO_LEDGER_TYPES_SIGNATURE_CHAIN_H


#include <LLC/types/uint1024.h>

#include <LLD/cache/template_lru.h>

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

        /** Credentials
         *
         *  Base Class for a Signature Credentials
         *
         *  Similar to HD wallet systems, but integrated into the Layer 2 of the nexus software stack.
         *  Seed phrase includes at least 128 bits of entropy (8 char username, 8 char password) and pin
         *  to attack a signature chain by dictionary attack.
         *
         */
        class Credentials : public memory::encrypted
        {

            /** Secure allocator to represent the username of this signature chain. **/
            const SecureString strUsername;


            /** Secure allocater to represent the password of this signature chain. **/
            SecureString strPassword;


            /* Internal mutex for caches. */
            mutable std::mutex MUTEX;


            /** Internal sigchain cache (to not exhaust ourselves regenerating the same key). **/
            mutable std::pair<uint32_t, SecureString> pairCache;


            /** Internal credential key hash of our generated crypto keys. **/
            mutable std::map<std::string, uint512_t> mapCrypto;


            /** Internal genesis hash. **/
            const uint256_t hashGenesis;


        public:


            /** Default constructor. **/
            Credentials() = delete;


            /** Copy Constructor **/
            Credentials(const Credentials& sigchain);


            /** Move Constructor **/
            Credentials(Credentials&& sigchain) noexcept;


            /** Copy Assignment Operator **/
            Credentials& operator=(const Credentials& sigchain)    = delete;


            /** Move Assignment Operator **/
            Credentials& operator=(Credentials&& sigchain) noexcept = delete;


            /** Destructor. **/
            ~Credentials();


            /** Equivilence operator. **/
            bool operator==(const Credentials& pCheck) const;


            /** Equivilence operator. **/
            bool operator!=(const Credentials& pCheck) const;


            /** Constructor to generate Keychain
             *
             *  @param[in] strUsernameIn The username to seed the signature chain
             *  @param[in] strPasswordIn The password to seed the signature chain
             *
             **/
            Credentials(const SecureString& strUsernameIn, const SecureString& strPasswordIn);


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
             *  @return The 256 bit hash of a genesis-id using Argon2 function.
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
             *  This function is responsible for generating the private key in the sigchain of a specific account.
             *  The sigchain is a series of keys seeded from a secret phrase and a PIN number.
             *
             *  @param[in] strType The type of signing key used.
             *  @param[in] nKeyID The key number in the keychian
             *  @param[in] strSecret The secret phrase to use
             *
             *  @return The 512 bit hash of this key in the series.
             **/
            uint512_t Generate(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret) const;


            /** Generate
             *
             *  This function is responsible for generating a private key from a seed phrase.  By comparison to the other Generate
             *  functions, this version using far stronger argon2 hashing since the only data input into the hashing function is
             *  the seed phrase itself.
             *
             *  @param[in] strRecovery The secret seed phrase to use
             *
             *  @return The 512 bit hash of the generated public key.
             **/
            uint512_t RecoveryKey(const SecureString& strRecovery) const;


            /** SigningKey
             *
             *  This function generates a public key generated from random seed phrase.
             *
             *  @param[in] strType The type of signing key used.
             *  @param[in] nKeyID The key number in the keychian
             *  @param[in] strSecret The secret phrase to use
             *  @param[in] nType The key type to use.
             *
             *  @return The vector of bytes representing this public key.
             **/
            std::vector<uint8_t> SigningKey(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret, const uint8_t nType) const;


            /** SigningKeyHash
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
            uint256_t SigningKeyHash(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret, const uint8_t nType) const;


            /** RecoveryHash
             *
             *  This function generates a hash of a public key generated from a recovery seed phrase.
             *
             *  @param[in] strRecovery The recovery seed phrase to use
             *  @param[in] nType The key type to use.
             *
             *  @return The 256 bit hash of the generated public key
             **/
            uint256_t RecoveryHash(const SecureString& strRecovery, const uint8_t nType) const;


            /** UserName
             *
             *  Returns the username for this sig chain
             *
             *  @return The SecureString containing the username for this sig chain
             **/
            const SecureString& UserName() const;


            /** Password
             *
             *  Returns the password for this sig chain
             *
             *  @return The SecureString containing the password for this sig chain
             **/
            const SecureString& Password() const;


            /** Update
             *
             *  Updates the password for this sigchain.
             *
             *  @param[in] strPasswordNew The password to update in sigchain.
             *
             **/
            void Update(const SecureString& strPasswordNew);


            /** ClearCache
             *
             *  Clears all of the active crypto keys.
             *
             **/
            void ClearCache();


            /** Encrypt
             *
             *  Special method for encrypting specific data types inside class.
             *
             **/
            void Encrypt();
        };
    }
}

#endif
