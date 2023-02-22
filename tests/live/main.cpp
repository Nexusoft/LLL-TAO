/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>

#include <LLD/include/global.h>
#include <LLD/cache/binary_lru.h>
#include <LLD/keychain/hashmap.h>
#include <LLD/templates/sector.h>

#include <Util/include/debug.h>
#include <Util/include/base64.h>

#include <openssl/rand.h>

#include <LLC/hash/argon2.h>
#include <LLC/hash/SK.h>
#include <LLC/include/flkey.h>
#include <LLC/types/bignum.h>

#include <Util/include/hex.h>

#include <iostream>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <TAO/Register/include/create.h>

#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/types/credentials.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Ledger/include/ambassador.h>

#include <Legacy/types/address.h>
#include <Legacy/types/transaction.h>

#include <LLP/templates/ddos.h>
#include <Util/include/runtime.h>

#include <list>
#include <variant>

#include <Util/include/softfloat.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

#include <LLP/types/tritium.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/locator.h>

class TestDB : public LLD::SectorDatabase<LLD::BinaryHashMap, LLD::BinaryLRU>
{
public:
    TestDB()
    : SectorDatabase("testdb"
    , LLD::FLAGS::CREATE | LLD::FLAGS::FORCE
    , 256 * 256 * 64
    , 1024 * 1024 * 4)
    {
    }

    ~TestDB()
    {

    }

    bool WriteKey(const uint1024_t& key, const uint1024_t& value)
    {
        return Write(std::make_pair(std::string("key"), key), std::make_pair(key, value));
    }


    bool ReadKey(const uint1024_t& key, uint1024_t &value)
    {
        std::pair<uint1024_t, uint1024_t&> pairResults = std::make_pair(key, std::ref(value));

        return Read(std::make_pair(std::string("key"), key), pairResults);
    }


    bool WriteLast(const uint1024_t& last)
    {
        return Write(std::string("last"), last);
    }

    bool ReadLast(uint1024_t &last)
    {
        return Read(std::string("last"), last);
    }

};



#include <TAO/Ledger/include/genesis_block.h>

const uint256_t hashSeed = 55;

#include <Util/include/math.h>

#include <Util/include/json.h>

#include <TAO/API/types/function.h>
#include <TAO/API/types/exception.h>

#include <TAO/Operation/include/enum.h>

#include <Util/encoding/include/utf-8.h>

#include <TAO/API/include/contracts/build.h>
#include <TAO/API/include/contracts/verify.h>

#include <TAO/API/types/contracts/expiring.h>
#include <TAO/API/types/contracts/params.h>

#include <TAO/Operation/include/enum.h>



#include <TAO/API/include/extract.h>

#include <Legacy/types/address.h>

#include <Util/types/precision.h>

extern "C"
{
    #include <LLC/kyber/kem.h>
    #include <LLC/kyber/symmetric.h>
}

#include <LLC/include/encrypt.h>


class KyberHandshake
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
    const uint256_t hashGenesis;

public:

    /** KyberHandshake
     *
     *  Create a handshake object using given deterministic seed.
     *
     *  @param[in] hashSeedIn The seed data used to generate public/private keypairs.
     *
     **/
    KyberHandshake(const uint256_t& hashSeedIn, const uint256_t& hashGenesisIn)
    : vPubKey     (CRYPTO_PUBLICKEYBYTES, 0)
    , vPrivKey    (CRYPTO_SECRETKEYBYTES, 0)
    , vSeed       (hashSeedIn.GetBytes())
    , hashKey     (0)
    , hashGenesis (hashGenesisIn)
    {
    }


    /** PubKeyHash
     *
     *  Get a hash of our internal public key for use in verification.
     *
     *  @return An SK-256 hash of the given public key.
     *
     **/
    const uint256_t PubKeyHash() const
    {
        return LLC::SK256(vPubKey);
    }


    /** ValidateHandshake
     *
     *  Validate a peer's certificate hash in crypto object register to the sent public key.
     *
     *  @param[in] hashPeerGenesis The genesis of the peer we are verifying.
     *  @param[in] vPeerPub The binary data of peer's public key to check
     *
     *  @return True if the handshake was validated against the crypto object register.
     *
     **/
    bool ValidateHandshake(const uint256_t& hashPeerGenesis, const std::vector<uint8_t>& vPeerPub) const
    {
        /* Generate register address for crypto register deterministically */
        const TAO::Register::Address addrCrypto =
            TAO::Register::Address(std::string("crypto"), hashPeerGenesis, TAO::Register::Address::CRYPTO);

        /* Read the existing crypto object register. */
        TAO::Register::Crypto oCrypto;
        if(!LLD::Register->ReadObject(addrCrypto, oCrypto, TAO::Ledger::FLAGS::LOOKUP))
            return debug::error(FUNCTION, "Failed to read crypto object register");

        /* Check the crypto object register mathces our peer's public key hash ot make sure they aren't an imposter. */
        if(oCrypto.get<uint256_t>("cert") != LLC::SK256(vPeerPub))
            return debug::error(FUNCTION, "peer certificate key mismatch to public key");

        return true;
    }


    /** InitiateHandshake
     *
     *  Generate a keypair using seed phrase and return the public key for transmission to peer.
     *
     *  @return The binary data of encdoded public key.
     *
     **/
    const std::vector<uint8_t> InitiateHandshake()
    {
        /* Generate our shared key using entropy from our seed hash. */
        crypto_kem_keypair_seed(&vPubKey[0], &vPrivKey[0], &vSeed[0]);

        return vPubKey;
    }


    /** RespondHandshake
     *
     *  Take a given peer's public key and encrypt a shared key to pass to peer.
     *
     *  @param[in] vPeerPub The binary data of peer's public key to encode
     *
     *  @return The binary data of encdoded handshake.
     *
     **/
    const std::vector<uint8_t> RespondHandshake(const std::vector<uint8_t>& vPeerPub)
    {
        /* Build our ciphertext vector to hold the handshake data. */
        std::vector<uint8_t> vCiphertext(CRYPTO_CIPHERTEXTBYTES, 0);

        /* Generate a keypair as part of our response. */
        crypto_kem_keypair_seed(&vPubKey[0], &vPrivKey[0], &vSeed[0]);

        /* Generate our shared key and encode using peer's public key. */
        std::vector<uint8_t> vShared(CRYPTO_BYTES, 0);
        crypto_kem_enc(&vCiphertext[0], &vShared[0], &vPeerPub[0]);

        /* Build our response message packaging our public key and ciphertext. */
        DataStream ssHandshake(SER_NETWORK, 1);
        ssHandshake << vPubKey << vCiphertext;

        /* Finally set our internal value for the shared key hash. */
        hashKey = LLC::SK256(vShared);

        return ssHandshake.Bytes();
    }


    /** CompleteHandshake
     *
     *  Take a given encoded handshake and decrypt the shared key using our private key.
     *
     *  @param[in] vHandshake The binary data of handshake passed from socket buffers.
     *
     **/
    void CompleteHandshake(const std::vector<uint8_t>& vHandshake)
    {
        /* Get our handshake in a datastream to deserialize. */
        DataStream ssHandshake(vHandshake, SER_NETWORK, 1);

        /* Get our peer's public key. */
        std::vector<uint8_t> vPeerPub(CRYPTO_PUBLICKEYBYTES, 0);
        ssHandshake >> vPeerPub;

        /* Get our cyphertext to decode our shared key from. */
        std::vector<uint8_t> vCiphertext(CRYPTO_CIPHERTEXTBYTES, 0);
        ssHandshake >> vCiphertext;

        /* Decode our shared key from the cyphertext. */
        std::vector<uint8_t> vShared(CRYPTO_BYTES, 0);
        crypto_kem_dec(&vShared[0], &vCiphertext[0], &vPrivKey[0]);

        /* Hash our shared key binary data to provide additional level of security. */
        hashKey = LLC::SK256(vShared);
    }


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
    bool Encrypt(const std::vector<uint8_t>& vPlainText, std::vector<uint8_t> &vCipherText)
    {
        /* Check that we have a shared key already. */
        if(hashKey == 0)
            return debug::error(FUNCTION, "cannot encrypt without first initiating handshake");

        return LLC::EncryptAES256(hashKey.GetBytes(), vPlainText, vCipherText);
    }


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
    bool Decrypt(const std::vector<uint8_t>& vCipherText, std::vector<uint8_t> &vPlainText)
    {
        /* Check that we have a shared key already. */
        if(hashKey == 0)
            return debug::error(FUNCTION, "cannot decrypt without first initiating handshake");

        return LLC::DecryptAES256(hashKey.GetBytes(), vCipherText, vPlainText);
    }
};


int main(void)
{
    uint256_t hash = LLC::SK256("testing");

    uint256_t hash2 = LLC::SK256("testing2");

    KyberHandshake shake(hash, TAO::Ledger::Credentials::Genesis("testing"));
    KyberHandshake shake2(hash2, TAO::Ledger::Credentials::Genesis("testing2"));

    const std::vector<uint8_t> vPayload = shake.InitiateHandshake();

    const std::vector<uint8_t> vResponse = shake2.RespondHandshake(vPayload);

    shake.CompleteHandshake(vResponse);

    debug::log(0, "PubKey 1: ", shake.PubKeyHash().ToString());
    debug::log(0, "PubKey 2: ", shake2.PubKeyHash().ToString());

    std::string strPayload = "This is our message that we have now decrypted! We want to test that this is coming through even with a lot of data!";

    std::vector<uint8_t> vCipherText;
    shake.Encrypt(std::vector<uint8_t>(strPayload.begin(), strPayload.end()), vCipherText);
    debug::log(0, "Encrypted: ", std::string(vCipherText.begin(), vCipherText.end()));

    std::vector<uint8_t> vPlainText;
    shake2.Decrypt(vCipherText, vPlainText);
    debug::log(0, "Decrypted: ", std::string(vPlainText.begin(), vPlainText.end()));

  return 0;
}

/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int oldmain(int argc, char** argv)
{
    precision_t nDigits1 = precision_t(5.98198, 5);
    precision_t nDigits2 = precision_t(3.321, 3);

    precision_t nDigits3 = std::move(precision_t("333.141592654", 9, true));

    precision_t nSum = nDigits1 + nDigits2;
    //nSum += nDigits2;//(nDigits1);// = nDigits1;// + nDigits2;

    precision_t nProduct = nDigits1;// * nDigits2;
    nProduct *= nDigits2;

    precision_t nDiv = nDigits1 / nDigits2;

    debug::log(0, VARIABLE(nDigits1.double_t()), " | ", VARIABLE(nDigits2.double_t()), " | ",
                  VARIABLE(nSum.double_t()), " | ", VARIABLE(nProduct.double_t()), " | ", VARIABLE(nDiv.double_t()));


    debug::log(0, VARIABLE(nDigits3.dump()));

    precision_t nDigits4 = precision_t("3.142238743879234");

    encoding::json jValue;
    jValue["test"] = nDigits4.double_t();

    debug::log(0, VARIABLE(jValue["test"].dump()), " | ", jValue.dump());

    precision_t nDigits5 =
        precision_t(jValue["test"].dump());

    nDigits5 = nDigits5 / 2;

    debug::log(0, VARIABLE(nDigits5.dump()));

    nDigits5 = nDigits5 * 2;

    debug::log(0, VARIABLE(nDigits5.dump()));

    debug::log(0, jValue.dump());

    debug::log(0, VARIABLE(nDigits4.dump()));

    if(nDigits2 < nDigits1)
        debug::log(0, "We have lessthan");


    if(nDigits1 > nDigits2)
        debug::log(0, "We have greaterthan");

    if(nDigits1 == precision_t(5.98198, 6))
        debug::log(0, "We have equal precision");

    return 0;

    encoding::json jParams;
    jParams["test1"] = "12.58";
    jParams["test2"] = 1.589;

    uint64_t nAmount1 = TAO::API::ExtractAmount(jParams, 100, "test1");
    uint64_t nAmount2 = TAO::API::ExtractAmount(jParams, 1000, "test2");

    debug::log(0, VARIABLE(nAmount1), " | ", VARIABLE(nAmount2));

    return 0;

    TAO::Operation::Contract tContract;

    //debug::log(0, "First param is ", ssParams.find(0, uint8_t(TAO::Operation::OP::TYPES::UINT256_T)).ToString());
    TAO::API::Contracts::Build(TAO::API::Contracts::Expiring::Receiver[1], tContract, uint64_t(3333));

    if(!TAO::API::Contracts::Verify(TAO::API::Contracts::Expiring::Receiver[1], tContract))
        return debug::error("Contract binary template mismatch");

    return 0;

    /* Read the configuration file. Pass argc and argv for possible -datadir setting */
    config::ReadConfigFile(config::mapArgs, config::mapMultiArgs, argc, argv);


    /* Parse out the parameters */
    config::ParseParameters(argc, argv);


    /* Once we have read in the CLI paramters and config file, cache the args into global variables*/
    config::CacheArgs();


    /* Initalize the debug logger. */
    debug::Initialize();

    //config::mapArgs["-datadir"] = "/database/SYNC1";

    TestDB* DB = new TestDB();

    uint1024_t hashKey = LLC::GetRand1024();
    uint1024_t hashValue = LLC::GetRand1024();

    debug::log(0, VARIABLE(hashKey.SubString()), " | ", VARIABLE(hashValue.SubString()));

    DB->WriteKey(hashKey, hashValue);

    {
        uint1024_t hashValue2;
        DB->ReadKey(hashKey, hashValue2);

        debug::log(0, VARIABLE(hashKey.SubString()), " | ", VARIABLE(hashValue2.SubString()));
    }

    hashKey = LLC::GetRand1024();
    hashValue = LLC::GetRand1024();

    debug::log(0, VARIABLE(hashKey.SubString()), " | ", VARIABLE(hashValue.SubString()));

    DB->WriteKey(hashKey, hashValue);

    {
        uint1024_t hashValue2;
        DB->ReadKey(hashKey, hashValue2);

        debug::log(0, VARIABLE(hashKey.SubString()), " | ", VARIABLE(hashValue2.SubString()));
    }

    return 0;

    std::string strTest = "This is a test unicode string";

    std::vector<uint8_t> vHash = LLC::GetRand256().GetBytes();

    debug::log(0, strTest);
    debug::log(0, "VALID: ", encoding::utf8::is_valid(strTest.begin(), strTest.end()) ? "TRUE" : "FALSE");
    debug::log(0, HexStr(vHash.begin(), vHash.end()));
    debug::log(0, "VALID: ", encoding::utf8::is_valid(vHash.begin(), vHash.end()) ? "TRUE" : "FALSE");

    return 0;

    /* Initialize LLD. */
    LLD::Initialize();

    uint32_t nScannedCount = 0;

    uint512_t hashLast = 0;

    /* If started from a Legacy block, read the first Tritium tx to set hashLast */
    std::vector<TAO::Ledger::Transaction> vtx;
    if(LLD::Ledger->BatchRead("tx", vtx, 1))
        hashLast = vtx[0].GetHash();

    bool fFirst = true;

    runtime::timer timer;
    timer.Start();

    uint32_t nTransactionCount = 0;

    debug::log(0, FUNCTION, "Scanning Tritium from tx ", hashLast.SubString());
    while(!config::fShutdown.load())
    {
        /* Read the next batch of inventory. */
        std::vector<TAO::Ledger::Transaction> vtx;
        if(!LLD::Ledger->BatchRead(hashLast, "tx", vtx, 1000, !fFirst))
            break;

        /* Loop through found transactions. */
        TAO::Ledger::BlockState state;
        for(const auto& tx : vtx)
        {
            //_unused(tx);

            for(uint32_t n = 0; n < tx.Size(); ++n)
            {
                uint8_t nOP = 0;
                tx[n] >> nOP;

                if(nOP == TAO::Operation::OP::CONDITION || nOP == TAO::Operation::OP::VALIDATE)
                    ++nTransactionCount;
            }

            /* Update the scanned count for meters. */
            ++nScannedCount;

            /* Meter for output. */
            if(nScannedCount % 100000 == 0)
            {
                /* Get the time it took to rescan. */
                uint32_t nElapsedSeconds = timer.Elapsed();
                debug::log(0, FUNCTION, "Processed ", nTransactionCount, " of ", nScannedCount, " in ", nElapsedSeconds, " seconds (",
                    std::fixed, (double)(nScannedCount / (nElapsedSeconds > 0 ? nElapsedSeconds : 1 )), " tx/s)");
            }
        }

        /* Set hash Last. */
        hashLast = vtx.back().GetHash();

        /* Check for end. */
        if(vtx.size() != 1000)
            break;

        /* Set first flag. */
        fFirst = false;
    }

    return 0;


    {
        uint1024_t hashBlock = uint1024_t("0x9e804d2d1e1d3f64629939c6f405f15bdcf8cd18688e140a43beb2ac049333a230d409a1c4172465b6642710ba31852111abbd81e554b4ecb122bdfeac9f73d4f1570b6b976aa517da3c1ff753218e1ba940a5225b7366b0623e4200b8ea97ba09cb93be7d473b47b5aa75b593ff4b8ec83ed7f3d1b642b9bba9e6eda653ead9");


        TAO::Ledger::BlockState state = TAO::Ledger::TritiumGenesis();
        debug::log(0, state.hashMerkleRoot.ToString());

        return 0;

        if(!LLD::Ledger->ReadBlock(hashBlock, state))
            return debug::error("failed to read block");

        printf("block.hashPrevBlock = uint1024_t(\"0x%s\");\n", state.hashPrevBlock.ToString().c_str());
        printf("block.nVersion = %u;\n", state.nVersion);
        printf("block.nHeight = %u;\n", state.nHeight);
        printf("block.nChannel = %u;\n", state.nChannel);
        printf("block.nTime = %lu;\n",    state.nTime);
        printf("block.nBits = %u;\n", state.nBits);
        printf("block.nNonce = %lu;\n", state.nNonce);

        for(int i = 0; i < state.vtx.size(); ++i)
        {
            printf("/* Hardcoded VALUE for INDEX %i. */\n", i);
            printf("vHashes.push_back(uint512_t(\"0x%s\"));\n\n", state.vtx[i].second.ToString().c_str());
            //printf("block.vtx.push_back(std::make_pair(%u, uint512_t(\"0x%s\")));\n\n", state.vtx[i].first, state.vtx[i].second.ToString().c_str());
        }

        for(int i = 0; i < state.vOffsets.size(); ++i)
            printf("block.vOffsets.push_back(%u);\n", state.vOffsets[i]);


        return 0;

        //config::mapArgs["-datadir"] = "/public/tests";

        //TestDB* db = new TestDB();

        uint1024_t hashLast = 0;
        //db->ReadLast(hashLast);

        std::fstream stream1(config::GetDataDir() + "/test1.txt", std::ios::in | std::ios::out | std::ios::binary);
        std::fstream stream2(config::GetDataDir() + "/test2.txt", std::ios::in | std::ios::out | std::ios::binary);
        std::fstream stream3(config::GetDataDir() + "/test3.txt", std::ios::in | std::ios::out | std::ios::binary);
        std::fstream stream4(config::GetDataDir() + "/test4.txt", std::ios::in | std::ios::out | std::ios::binary);
        std::fstream stream5(config::GetDataDir() + "/test5.txt", std::ios::in | std::ios::out | std::ios::binary);

        std::vector<uint8_t> vBlank(1024, 0); //1 kb

        stream1.write((char*)&vBlank[0], vBlank.size());
        stream2.write((char*)&vBlank[0], vBlank.size());
        stream3.write((char*)&vBlank[0], vBlank.size());
        stream4.write((char*)&vBlank[0], vBlank.size());
        stream5.write((char*)&vBlank[0], vBlank.size());

        runtime::timer timer;
        timer.Start();
        for(uint64_t n = 0; n < 100000; ++n)
        {
            stream1.seekp(0, std::ios::beg);
            stream1.write((char*)&vBlank[0], vBlank.size());

            stream1.seekp(8, std::ios::beg);
            stream1.write((char*)&vBlank[0], vBlank.size());

            stream1.seekp(16, std::ios::beg);
            stream1.write((char*)&vBlank[0], vBlank.size());

            stream1.seekp(32, std::ios::beg);
            stream1.write((char*)&vBlank[0], vBlank.size());

            stream1.seekp(64, std::ios::beg);
            stream1.write((char*)&vBlank[0], vBlank.size());
            stream1.flush();

            //db->WriteKey(n, n);
        }

        debug::log(0, "Wrote 100k records in ", timer.ElapsedMicroseconds(), " micro-seconds");


        timer.Reset();
        for(uint64_t n = 0; n < 100000; ++n)
        {
            stream1.seekp(0, std::ios::beg);
            stream1.write((char*)&vBlank[0], vBlank.size());
            stream1.flush();

            stream2.seekp(8, std::ios::beg);
            stream2.write((char*)&vBlank[0], vBlank.size());
            stream2.flush();

            stream3.seekp(16, std::ios::beg);
            stream3.write((char*)&vBlank[0], vBlank.size());
            stream3.flush();

            stream4.seekp(32, std::ios::beg);
            stream4.write((char*)&vBlank[0], vBlank.size());
            stream4.flush();

            stream5.seekp(64, std::ios::beg);
            stream5.write((char*)&vBlank[0], vBlank.size());
            stream5.flush();
            //db->WriteKey(n, n);
        }
        timer.Stop();

        debug::log(0, "Wrote 100k records in ", timer.ElapsedMicroseconds(), " micro-seconds");

        //db->WriteLast(hashLast + 100000);
    }




    return 0;
}
