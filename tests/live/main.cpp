/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/types/uint1024.h>
#include <LLC/hash/SK.h>
#include <LLC/include/random.h>
#include <LLC/hash/macro.h>

#include <LLP/include/base_address.h>
#include <LLP/templates/socket.h>
#include <LLP/types/apinode.h>

#include <Util/include/debug.h>
#include <Util/include/base64.h>

#include <LLC/hash/argon2.h>
#include <LLC/include/flkey.h>

#include <iostream>


bool DownloadDictionary(std::vector<std::string> &vDictionary, const std::string &strLink, const uint32_t nMinimumWords = 100)
{
    debug::log(0, FUNCTION, strLink);

    std::string strHost = strLink;
    std::string strEndpoint;

    uint16_t nPort = 80;

    /* Make sure the link contains a web extension. */
    if(strLink.find('.') == std::string::npos)
        return debug::error(FUNCTION, "Improper link: ", strLink);

    std::string::size_type pos = strHost.find("http://");
    if(pos != std::string::npos)
        strHost.erase(strHost.begin(), strHost.begin() + pos + 7);
    else
    {
        pos = strHost.find("https://");

        if(pos != std::string::npos)
            strHost.erase(strHost.begin(), strHost.begin() + pos + 8);

        //nPort = 443;
    }

    pos = strHost.find('/');
    if(pos != std::string::npos)
    {
        strEndpoint = std::string(strHost.begin() + pos + 1, strHost.end());
        strHost.erase(strHost.begin() + pos, strHost.end());
    }

    debug::log(3, FUNCTION, "Host: ", strHost);
    debug::log(3, FUNCTION, "Endpoint: ", strEndpoint);


    LLP::APINode apiNode;
    LLP::BaseAddress addr(strHost, nPort, true);

    if(!apiNode.Connect(addr))
        return debug::error(FUNCTION, "Could not connect to ", strHost, " (", addr.ToString(), ")");

    debug::log(0, FUNCTION "Connected to ", strHost, " (", addr.ToString(), ")");


    /* Build the HTTP Header. */
    std::string strReply = debug::safe_printstr(
            "GET /", strEndpoint, " HTTP/1.1\r\n",
            "Host: data.nexus.io\r\n",
            "Date: ", debug::rfc1123Time(), "\r\n",
            "Connection: close\r\n",
            "\r\n");

    /* Convert the content into a byte buffer. */
    std::vector<uint8_t> vBuffer(strReply.begin(), strReply.end());

    apiNode.Write(vBuffer, vBuffer.size());
    apiNode.Flush();


    while(!apiNode.INCOMING.Complete())
    {
        if(!apiNode.Connected())
            return debug::error(FUNCTION, "Connection terminated");

        if(apiNode.Errors())
            return debug::error(FUNCTION, "Socket error");

        if(apiNode.Timeout(120))
            return debug::error(FUNCTION, "Socket timeout");

        apiNode.ReadPacket();

        runtime::sleep(1);
    }

    apiNode.Disconnect();


    vDictionary = Split(apiNode.INCOMING.strContent, '\n');

    debug::log(0, FUNCTION, "Received ", vDictionary.size(), " Words");


    if(vDictionary.size() < nMinimumWords)
        return debug::error(FUNCTION, "Not enough dictionary words. Make sure there are at least ", nMinimumWords, " words.");


    return true;
}


bool ReadDictionary(std::vector<std::string> &vDictionary, const std::string &strDictionaryPath, const uint32_t nMinimumWords = 100)
{
    std::string strLine;
    std::ifstream fin(strDictionaryPath, std::ios_base::in);
    uint32_t nWords = 0;

    if(!fin.is_open())
        return debug::error(FUNCTION, "Failed to open ", strDictionaryPath);


    while(!std::getline(fin, strLine).eof())
    {
        strLine.erase(std::remove(strLine.begin(), strLine.end(), '\r'), strLine.end());
        vDictionary.push_back(strLine);
        ++nWords;
    }

    if(vDictionary.size() < nMinimumWords)
        return debug::error(FUNCTION, "Not enough dictionary words. Make sure there are at least ", nMinimumWords, " words.");

    fin.close();


    debug::log(0, FUNCTION, "Read ", nWords, " words");

    return true;
}


bool GetRandomWordString(std::string &strWords, const std::vector<std::string> &vDictionary, const uint32_t nWords = 20)
{
    uint64_t nDictionarySize = vDictionary.size();

    strWords.clear();
    for(uint32_t i = 0; i < nWords; ++i)
    {
        uint64_t nRand = LLC::GetRand();
        strWords += vDictionary[nRand % nDictionarySize];

        if(i < nWords - 1)
            strWords += " ";
    }

    debug::log(0, FUNCTION, strWords);

    return true;
}


uint64_t GenerateBirthdaySecret(uint8_t DD, uint8_t MM, uint16_t YYYY)
{
    uint32_t nEncoded = static_cast<uint32_t>(YYYY);

    nEncoded |= static_cast<uint32_t>(MM) << 16;
    nEncoded |= static_cast<uint32_t>(DD) << 24;

    uint64_t nSecret = LLC::SK64(BEGIN(nEncoded), END(nEncoded));

    debug::log(0, FUNCTION, nSecret);

    return nSecret;
}


bool GenerateRecoveryHash(uint512_t &txRecoveryOut, const std::string &strUsername, const std::string &strRecovery, uint64_t nBirthdaySecret)
{
    /* Generate the Secret Phrase */
    std::vector<uint8_t> vUsername(strUsername.begin(), strUsername.end());
    if(vUsername.size() < 8)
        vUsername.resize(8);


    std::vector<uint8_t> vRecovery(strRecovery.begin(), strRecovery.end());
    std::vector<uint8_t> vHash(64);

    const uint32_t nComputationalCost = 108;
    const uint32_t nMemoryCostKB = 1 << 16;
    const uint32_t nThreads = 1;
    const uint32_t nLanes = 1;

    /* Create the hash context. */
    argon2_context context =
    {
        /* Hash Return Value. */
        &vHash[0],
        static_cast<uint32_t>(vHash.size()),

        /* Recovery input data. */
        &vRecovery[0],
        static_cast<uint32_t>(vRecovery.size()),

        /* Username input data. */
        &vUsername[0],
        static_cast<uint32_t>(vUsername.size()),

        /* The salt */
        reinterpret_cast<uint8_t *>(&nBirthdaySecret),
        static_cast<uint32_t>(sizeof(uint64_t)),

        /* Optional associated data */
        NULL, 0,

        nComputationalCost,
        nMemoryCostKB,
        nThreads, nLanes,

        /* Algorithm Version */
        ARGON2_VERSION_13,

        /* Custom memory allocation / deallocation functions. */
        NULL, NULL,

        /* By default only internal memory is cleared (pwd is not wiped) */
        ARGON2_DEFAULT_FLAGS
    };

    /* Run the argon2 computation. */
    int32_t nRet = argon2id_ctx(&context);

    if(nRet != ARGON2_OK)
        return debug::error(FUNCTION, "Argon2 failed with code ", nRet);

    /* Set the bytes for the key. */
    txRecoveryOut.SetBytes(vHash);

    /* Output recovery private key. */
    debug::log(0, FUNCTION, txRecoveryOut.ToString());

    return true;
}

/* This is for prototyping new code. This main is accessed by building with LIVE_TESTS=1. */
int main(int argc, char** argv)
{
    std::string strRecovery;
    std::vector<std::string> vDictionary;

    config::ParseParameters(argc, argv);


    /** Initialize network resources. (Need before RPC/API for WSAStartup call in Windows) **/
    if (!LLP::NetworkStartup())
    {
        printf("ERROR: Failed initializing network resources");

        return 0;
    }


    std::string strLink = config::GetArg(std::string("-link"), "http://data.nexus.io/wordlists/wordlist.txt");


    if(config::GetBoolArg("-http"))
    {

        if(!DownloadDictionary(vDictionary, strLink, 100))
            return 0;
    }
    else
    {
        if(!ReadDictionary(vDictionary, config::GetDataDir() + "wordlist.txt", 1 << 15))
            return 0;
    }


    if(!GetRandomWordString(strRecovery, vDictionary, 20))
        return 0;

    std::string strUsername = "jack";
    uint64_t nSecret = GenerateBirthdaySecret(11, 17, 1991);
    uint512_t hashRecovery;


    if(!GenerateRecoveryHash(hashRecovery, strUsername, strRecovery, nSecret))
        return 0;


    /** After all servers shut down, clean up underlying networking resources **/
    LLP::NetworkShutdown();


    return 0;
}
