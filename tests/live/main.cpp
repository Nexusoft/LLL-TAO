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
#include <TAO/Ledger/types/sigchain.h>
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

#include <TAO/API/include/contracts/verify.h>

#include <TAO/API/types/contracts/expiring.h>
#include <TAO/API/types/contracts/params.h>

#include <TAO/Operation/include/enum.h>

/** Build
 *
 *  Build that a given contract with variadic parameters based on placeholder binary templates.
 *
 *  @param[in] vByteCode The binary template to build off of.
 *  @param[in] rContract The contract we are checking against.
 *  @param[in] args The variadic template pack
 *
 *  @return true if contract matches, false otherwise.
 *
 **/
template<class... Args>
bool Build(const std::vector<uint8_t> vByteCode, TAO::Operation::Contract &rContract, Args&&... args)
{
    /* Reset our conditional stream each time this is called. */
    rContract.Clear(TAO::Operation::Contract::CONDITIONS);

    /* Unpack our parameter pack and place expected parameters over the PLACEHOLDERS. */
    TAO::API::Contracts::Params ssParams;
    ((ssParams << args), ...);

    /* Loop through our byte-code to generate our contract. */
    uint32_t nMax = 0;
    for(uint64_t nPos = 0; nPos < vByteCode.size(); ++nPos)
    {
        /* Get our current operation code. */
        const uint8_t nCode = vByteCode[nPos];

        /* Check for valid placeholder. */
        if(TAO::Operation::PLACEHOLDER::Valid(nCode))
        {
            /* Get placeholder index. */
            const uint32_t nIndex = (nCode - 1);

            /* Check our placehoder. */
            if(!ssParams.check(nIndex, vByteCode[++nPos]))
                return debug::error(FUNCTION, "parameter type mismatch");

            /* Bind parameter to placeholder. */
            ssParams.bind(rContract, nIndex);
            nMax = std::max(nIndex, nMax);

            continue;
        }

        /* Add the instructions. */
        rContract <= nCode;
    }

    /* Check that we don't have too many parameters passed into variadic template. */
    if(nMax < ssParams.max())
        return debug::error(FUNCTION, "too many parameters for binary template");

    return true;
}

#include <TAO/API/include/extract.h>

#include <Legacy/types/address.h>

/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    encoding::json jParams;
    jParams["test1"] = "12.58";
    jParams["test2"] = 1.589;

    uint64_t nAmount1 = TAO::API::ExtractAmount(jParams, 100, "test1");
    uint64_t nAmount2 = TAO::API::ExtractAmount(jParams, 1000, "test2");

    debug::log(0, VARIABLE(nAmount1), " | ", VARIABLE(nAmount2));

    return 0;

    TAO::Operation::Contract tContract;

    //debug::log(0, "First param is ", ssParams.find(0, uint8_t(TAO::Operation::OP::TYPES::UINT256_T)).ToString());
    Build(TAO::API::Contracts::Expiring::Receiver[1], tContract, uint64_t(3333));

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
