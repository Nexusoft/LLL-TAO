/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/Register/types/crypto.h>

#include <TAO/Ledger/types/credentials.h>

#include <Util/templates/datastream.h>

namespace LLC
{

    /** @class PQSSL_CTX
     *
     *  Post Quantum Socket Layer Context for managing encryption keys for PQ encryption channels.
     *
     **/
    class PQSSL_CTX
    {

        /** The internal binary data of the public key used for encryption. **/
        std::vector<uint8_t> vPubKey;


        /** The internal binary data of the private key used for decryption. **/
        std::vector<uint8_t> vPrivKey;


        /** The internal seed data that's used to deterministically generate our public keys. **/
        std::vector<uint8_t> vSeed;


        /** The shared key stored as a 256-bit unsigned integer. **/
        uint256_t hashKey;


        /** The current user that is initiating this handshake. **/
        TAO::Register::Crypto oCrypto;


        /** Internal ENUM to track if ciphertext is included. */
        enum HANDSHAKE : uint8_t
        {
            INITIATE = 0x01,
            RESPONSE = 0x02,
        };

    public:


        /** PQSSL_CTX
         *
         *  Create a handshake object using given deterministic seed.
         *
         *  @param[in] hashSeedIn The seed data used to generate public/private keypairs.
         *
         **/
        PQSSL_CTX(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials, const SecureString& strSecret);


        /** PQSSL_CTX
         *
         *  Create a handshake object using given deterministic seed.
         *
         *  @param[in] hashSeedIn The seed data used to generate public/private keypairs.
         *
         **/
        PQSSL_CTX(const uint512_t& hashSeedIn, const uint256_t& hashGenesis);


        /** Completed
         *
         *  Tell if the current handshake has been completed.
         *
         **/
        bool Completed() const;


        /** PubKeyHash
         *
         *  Get a hash of our internal public key for use in verification.
         *
         *  @return An SK-256 hash of the given public key.
         *
         **/
        const uint256_t PubKeyHash() const;


        /** InitiateHandshake
         *
         *  Generate a keypair using seed phrase and return the public key for transmission to peer.
         *
         *  @return The binary data of encdoded public key.
         *
         **/
        const std::vector<uint8_t> InitiateHandshake(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials,
                                                     const SecureString& strPIN);


        /** RespondHandshake
         *
         *  Take a given peer's public key and encrypt a shared key to pass to peer.
         *
         *  @param[in] vPeerPub The binary data of peer's public key to encode
         *
         *  @return The binary data of encdoded handshake.
         *
         **/
        const std::vector<uint8_t> RespondHandshake(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials,
                                                    const SecureString& strPIN, const std::vector<uint8_t>& vPeerHandshake);


        /** CompleteHandshake
         *
         *  Take a given encoded handshake and decrypt the shared key using our private key.
         *
         *  @param[in] vHandshake The binary data of handshake passed from socket buffers.
         *
         **/
        void CompleteHandshake(const std::vector<uint8_t>& vHandshake);


        /** Encrypt
         *
         *  Encrypt a given piece of data with a shared key using AES256
         *
         *  @param[in] vPlainText The plaintext data to encrypt
         *  @param[out] vCipherText The encrypted data returned.
         *
         *  @return True if encryption succeeded, false if not.
         *
         **/
        bool Encrypt(const std::vector<uint8_t>& vPlainText, std::vector<uint8_t> &vCipherText);


        /** Decrypt
         *
         *  Decrypt a given piece of data with a shared key using AES256
         *
         *  @param[in] vCipherText The encrypted ciphertext data to decrypt.
         *  @param[out] vPlainText The decrypted data returned.
         *
         *  @return True if decryption succeeded, false if not.
         *
         **/
        bool Decrypt(const std::vector<uint8_t>& vCipherText, std::vector<uint8_t> &vPlainText);


    private:

        /** initialize_auth
         *
         *  Generate an authentication message that can be used to generate an encryption channel.
         *
         *  @param[in] rCrypto The crypto object register we are generating handshake for.
         *
         *  @return The serialized message payload.
         *
         **/
        DataStream initialize_auth(const TAO::Register::Crypto& rCrypto, const bool fResponse);


        /** validate_auth
         *
         *  Take a given peer's handshake and validates it, and responds if succees
         *
         *  @param[in] vPeerPub The binary data of peer's public key to encode
         *
         *  @return True if the handshake is valid.
         *
         **/
        bool validate_auth(const std::vector<uint8_t>& vHandshake, std::vector<uint8_t> &vDataOut);
    };
}
