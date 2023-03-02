/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/pqssl.h>
#include <LLC/include/kyber.h>

#include <LLC/include/encrypt.h>

#include <LLD/include/global.h>

#include <TAO/Register/types/address.h>

namespace LLC
{

    /** PQSSL_CTX
     *
     *  Default Constructor.
     *
     **/
    PQSSL_CTX::PQSSL_CTX( )
    : vPubKey     (CRYPTO_PUBLICKEYBYTES, 0)
    , vPrivKey    (CRYPTO_SECRETKEYBYTES, 0)
    , vSeed       ( )
    , hashKey     (0)
    , oCrypto     ( )
    {
    }


    /* Create a handshake object using given credentials and secret. */
    PQSSL_CTX::PQSSL_CTX(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials, const SecureString& strSecret)
    : vPubKey     (CRYPTO_PUBLICKEYBYTES, 0)
    , vPrivKey    (CRYPTO_SECRETKEYBYTES, 0)
    , vSeed       ( )
    , hashKey     (0)
    , oCrypto     ( )
    {
        /* Just wrap around this function for constructor. */
        if(!Initialize(pCredentials, strSecret))
            throw debug::exception(FUNCTION, "failed ot initialize our context");
    }


    /* Create a handshake object using given deterministic seed. */
    PQSSL_CTX::PQSSL_CTX(const uint512_t& hashSeedIn, const uint256_t& hashGenesis)
    : vPubKey     (CRYPTO_PUBLICKEYBYTES, 0)
    , vPrivKey    (CRYPTO_SECRETKEYBYTES, 0)
    , vSeed       (hashSeedIn.GetBytes())
    , hashKey     (0)
    , oCrypto     ( )
    {
        /* Generate register address for crypto register deterministically */
        const TAO::Register::Address addrCrypto =
            TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

        /* Read the existing crypto object register. */
        if(!LLD::Register->ReadObject(addrCrypto, oCrypto, TAO::Ledger::FLAGS::LOOKUP))
            throw debug::exception(FUNCTION, "Failed to read crypto object register");
    }


    /* Setup this context with credentials if not constructed. */
    bool PQSSL_CTX::Initialize(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials, const SecureString& strSecret)
    {
        /* Set our deterministic seed. */
        vSeed = pCredentials->Generate("cert", 0, strSecret).GetBytes();

        /* Generate register address for crypto register deterministically */
        const TAO::Register::Address addrCrypto =
            TAO::Register::Address(std::string("crypto"), pCredentials->Genesis(), TAO::Register::Address::CRYPTO);

        /* Read the existing crypto object register. */
        if(!LLD::Register->ReadObject(addrCrypto, oCrypto, TAO::Ledger::FLAGS::LOOKUP))
            return debug::error(FUNCTION, "Failed to read crypto object register");

        return true;
    }


    /* Tell if the current ssl context has been initialized. */
    bool PQSSL_CTX::Initialized() const
    {
        return (!vSeed.empty() && !oCrypto.IsNull());
    }


    /* Tell if the current handshake has been completed. */
    bool PQSSL_CTX::Completed() const
    {
        return (hashKey != 0);
    }


    /* Get a hash of our internal public key for use in verification. */
    const uint256_t PQSSL_CTX::PubKeyHash() const
    {
        return LLC::SK256(vPubKey, TAO::Ledger::KEM::KYBER);
    }


    /* Generate a keypair using seed phrase and return the public key for transmission to peer. */
    const std::vector<uint8_t> PQSSL_CTX::InitiateHandshake(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials,
                                                            const SecureString& strPIN)
    {
        /* Build our response message packaging our public key and ciphertext. */
        DataStream ssResponse =
            initialize_auth(oCrypto, false);

        /* Get a hash of our message to sign. */
        const uint256_t hashMessage = LLC::SK256(ssResponse.Bytes());

        /* Use our crypto register to create a signature. */
        std::vector<uint8_t> vCryptoPub;
        std::vector<uint8_t> vCryptoSig;

        /* Now sign our message hash with crypto object register. */
        if(!oCrypto.GenerateSignature("network", pCredentials, strPIN, hashMessage.GetBytes(), vCryptoPub, vCryptoSig))
            throw debug::exception(FUNCTION, "failed to sign when initiating handshake");

        /* Add our additional handshake data now. */
        ssResponse << vCryptoPub << vCryptoSig;

        return ssResponse.Bytes();
    }


    /* Take a given peer's public key and encrypt a shared key to pass to peer. */
    const std::vector<uint8_t> PQSSL_CTX::RespondHandshake(const memory::encrypted_ptr<TAO::Ledger::Credentials>& pCredentials,
                                                           const SecureString& strPIN, const std::vector<uint8_t>& vPeerHandshake)
    {
        /* Check that the handshake is a valid response. */
        std::vector<uint8_t> vPeerPub;
        if(!validate_auth(vPeerHandshake, vPeerPub))
            throw debug::exception(FUNCTION, "handshake invalid: ", debug::GetLastError());

        /* Build our response message packaging our public key and ciphertext. */
        DataStream ssResponse =
            initialize_auth(oCrypto, true);

        /* Build our ciphertext vector to hold the handshake data. */
        std::vector<uint8_t> vCipherText(CRYPTO_CIPHERTEXTBYTES, 0);

        /* Generate our shared key and encode using peer's public key. */
        std::vector<uint8_t> vShared(CRYPTO_BYTES, 0);
        crypto_kem_enc(&vCipherText[0], &vShared[0], &vPeerPub[0]);

        /* Add our cyphertext to our response object. */
        ssResponse << vCipherText;

        /* Get a hash of our message to sign. */
        const uint256_t hashMessage = LLC::SK256(ssResponse.Bytes());

        /* Use our crypto register to create a signature. */
        std::vector<uint8_t> vCryptoPub;
        std::vector<uint8_t> vCryptoSig;

        /* Now sign our message hash with crypto object register. */
        if(!oCrypto.GenerateSignature("network", pCredentials, strPIN, hashMessage.GetBytes(), vCryptoPub, vCryptoSig))
            throw debug::exception(FUNCTION, "failed to sign when initiating handshake");

        /* Build our response message packaging our public key and ciphertext. */
        ssResponse << vCryptoPub << vCryptoSig;

        /* Finally set our internal value for the shared key hash. */
        hashKey = LLC::SK256(vShared);

        return ssResponse.Bytes();
    }


    /* Take a given encoded handshake and decrypt the shared key using our private key. */
    bool PQSSL_CTX::CompleteHandshake(const std::vector<uint8_t>& vHandshake)
    {
        /* Check that the handshake is a valid response. */
        std::vector<uint8_t> vCipherText;
        if(!validate_auth(vHandshake, vCipherText))
            return debug::error(FUNCTION, "handshake invalid: ", debug::GetLastError());

        /* Decode our shared key from the cyphertext. */
        std::vector<uint8_t> vShared(CRYPTO_BYTES, 0);
        crypto_kem_dec(&vShared[0], &vCipherText[0], &vPrivKey[0]);

        /* Hash our shared key binary data to provide additional level of security. */
        hashKey = LLC::SK256(vShared);

        return true;
    }


    /* Encrypt a given piece of data with a shared key using AES256 */
    bool PQSSL_CTX::Encrypt(const std::vector<uint8_t>& vPlainText, std::vector<uint8_t> &vCipherText)
    {
        /* Check that we have a shared key already. */
        if(hashKey == 0)
            return debug::error(FUNCTION, "cannot encrypt without first initiating handshake");

        return LLC::EncryptAES256(hashKey.GetBytes(), vPlainText, vCipherText);
    }


    /* Decrypt a given piece of data with a shared key using AES256 */
    bool PQSSL_CTX::Decrypt(const std::vector<uint8_t>& vCipherText, std::vector<uint8_t> &vPlainText)
    {
        /* Check that we have a shared key already. */
        if(hashKey == 0)
            return debug::error(FUNCTION, "cannot decrypt without first initiating handshake");

        return LLC::DecryptAES256(hashKey.GetBytes(), vCipherText, vPlainText);
    }


    /* Generate an authentication message that can be used to generate an encryption channel. */
    DataStream PQSSL_CTX::initialize_auth(const TAO::Register::Crypto& rCrypto, const bool fResponse)
    {
        /* Generate our shared key using entropy from our seed hash. */
        crypto_kem_keypair_from_secret(&vPubKey[0], &vPrivKey[0], &vSeed[0]);

        /* Build our response message packaging our public key and ciphertext. */
        DataStream ssHandshake(SER_NETWORK, LLP::PROTOCOL_VERSION);

        /* Add our type byte. */
        uint8_t nType = HANDSHAKE::INITIATE;
        if(fResponse)
            nType = HANDSHAKE::RESPONSE;

        /* Serialize our data to stream now. */
        ssHandshake << nType << rCrypto.hashOwner << rCrypto.hashChecksum << runtime::unifiedtimestamp() << vPubKey;

        return ssHandshake;
    }


    /* Take a given peer's handshake and validates it, and responds if succees */
    bool PQSSL_CTX::validate_auth(const std::vector<uint8_t>& vHandshake, std::vector<uint8_t> &vDataOut)
    {
        /* Build our response message packaging our public key and ciphertext. */
        DataStream ssHandshake(vHandshake, SER_NETWORK, LLP::PROTOCOL_VERSION);

        /* Get our type for auth message. */
        uint8_t nType = 0;
        ssHandshake >> nType;

        /* Deserialize our handshake recipient. */
        uint256_t hashGenesis;
        ssHandshake >> hashGenesis;

        /* Generate register address for crypto register deterministically */
        const TAO::Register::Address addrCrypto =
            TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

        /* Read the existing crypto object register. */
        TAO::Register::Crypto oPeerCrypto;
        if(!LLD::Register->ReadObject(addrCrypto, oPeerCrypto, TAO::Ledger::FLAGS::LOOKUP))
            return debug::error(FUNCTION, "Failed to read crypto object register");

        /* Get additional data to check. */
        uint64_t hashChecksum = 0;
        ssHandshake >> hashChecksum;

        /* Check that our checksums match. */
        if(hashChecksum != oPeerCrypto.hashChecksum)
            return debug::error(FUNCTION, "crypto object register checksum mismatch");

        /* Get the timestamp this was sent at. */
        uint64_t nTimestamp = 0;
        ssHandshake >> nTimestamp;

        /* Check that handshake wasn't stale. */
        if(nTimestamp + 30 < runtime::unifiedtimestamp())
            return debug::error(FUNCTION, "handshake is stale by ", runtime::unifiedtimestamp() - (nTimestamp + 30), " seconds");

        /* Get our peer's public key for this handshake. */
        std::vector<uint8_t> vPeerPub;
        ssHandshake >> vPeerPub;

        /* Verify our peer's public key is correct public key. */
        const uint256_t hashCertificate = oPeerCrypto.get<uint256_t>("cert");
        if(hashCertificate != LLC::SK256(vPeerPub, hashCertificate.GetType()))
            return debug::error(FUNCTION, "peer's public key does not match crypto object register");

        /* Assemble a datastream to hash. */
        DataStream ssSignature(SER_NETWORK, 1);
        ssSignature << hashGenesis << hashChecksum << nTimestamp << vPeerPub;

        /* If we are responding add ciphertext. */
        if(nType == HANDSHAKE::RESPONSE)
        {
            /* Get our ciphertext from handshake and serialize for signature. */
            ssHandshake >> vDataOut; //we copy our ciphertext into our data out
            ssSignature << vDataOut;
        }
        else
            vDataOut = vPeerPub;

        /* Get a hash of our message to sign. */
        const uint256_t hashMessage =
            LLC::SK256(ssSignature.Bytes());

        /* Get our last two items to verify. */
        std::vector<uint8_t> vCryptoPub;
        std::vector<uint8_t> vCryptoSig;

        /* Deserialize the values. */
        ssHandshake >> vCryptoPub;
        ssHandshake >> vCryptoSig;

        /* Verify our signature is a valid authentication of crypto object register. */
        if(!oPeerCrypto.VerifySignature("network", hashMessage.GetBytes(), vCryptoPub, vCryptoSig))
            return debug::error(FUNCTION, "invalid signature for handshake authentication");

        return true;
    }
}
