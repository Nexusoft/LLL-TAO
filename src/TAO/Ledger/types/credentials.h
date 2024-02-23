/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

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
             *  @return The 512 bit hash of this key in the series.
             **/
            static uint256_t Genesis(const SecureString& strUsername);


            /** SetCache
             *
             *  Set's the current cached key manually in case it was generated externally from this object.
             *
             *  @param[in] hashSecret The secret key to cache internally.
             *  @param[in] nKeyID The current sequence this key was generated from.
             *
             **/
            void SetCache(const uint512_t& hashSecret, const uint32_t nKeyID);


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
             *  @param[in] strSecret The secret seed phrase to use
             *
             *  @return The 512 bit hash of the generated public key.
             **/
            uint512_t Generate(const SecureString& strSecret) const;


            /** Generate
             *
             *  This function is to remain backwards compatible if recovery was created after 5.0.6 and before 5.1.1.
             *
             *  @param[in] strSecret The secret seed phrase to use
             *
             *  @return The 512 bit hash of the generated public key.
             **/
            uint512_t GenerateDeprecated(const SecureString& strSecret) const;


            /** RecoveryDeprecated
             *
             *  This function is to remain backwards compatible if recovery was created after 5.0.6 and before 5.1.1
             *
             *  @param[in] strRecovery The recovery seed phrase to use
             *  @param[in] nType The key type to use.
             *
             *  @return The 256 bit hash of the generated public key
             **/
            uint256_t RecoveryDeprecated(const SecureString& strRecovery, const uint8_t nType) const;


            /** Key
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
            std::vector<uint8_t> Key(const std::string& strType, const uint32_t nKeyID, const SecureString& strSecret, const uint8_t nType) const;


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


            /** Encrypt
             *
             *  Special method for encrypting specific data types inside class.
             *
             **/
            void Encrypt();


            /** Sign
            *
            *  Generates a signature for the data, using the specified crypto key from the crypto object register
            *
            *  @param[in] strKey The name of the signing key from the crypto object register
            *  @param[in] vchData The data to base the signature off
            *  @param[in] hashSecret The private key to use for the signature
            *  @param[out] vchPubKey The public key generated from the private key
            *  @param[out] vchSig The signature bytes
            *
            *  @return True if successful
            *
            **/
            bool Sign(const std::string& strKey, const std::vector<uint8_t>& vchData, const uint512_t& hashSecret,
                                      std::vector<uint8_t>& vchPubKey, std::vector<uint8_t>& vchSig) const;


            /** Sign
            *
            *  Generates a signature for the data, using the specified crypto key type
            *
            *  @param[in] nKeyType The type of signing key to use
            *  @param[in] vchData The data to base the signature off
            *  @param[in] hashSecret The private key to use for the signature
            *  @param[out] vchPubKey The public key generated from the private key
            *  @param[out] vchSig The signature bytes
            *
            *  @return True if successful
            *
            **/
            bool Sign(const uint8_t& nKeyType, const std::vector<uint8_t>& vchData, const uint512_t& hashSecret,
                                      std::vector<uint8_t>& vchPubKey, std::vector<uint8_t>& vchSig) const;


            /** Verify
            *
            *  Verifies a signature for the data, as well as verifying that the hashed public key matches the
            *  specified key from the crypto object register
            *
            *  @param[in] hashGenesis The genesis hash of the sig chain to read the crypto object register for
            *  @param[in] strKey The name of the signing key from the crypto object register
            *  @param[in] vchData The data to base the verification from
            *  @param[in] vchPubKey The public key of the private key used to sign the data
            *  @param[in] vchSig The signature bytes
            *
            *  @return True if the signature is successfully verified
            *
            **/
            static bool Verify(const uint256_t hashGenesis, const std::string& strKey, const std::vector<uint8_t>& vchData,
                        const std::vector<uint8_t>& vchPubKey, const std::vector<uint8_t>& vchSig);


        };
    }
}

#endif
