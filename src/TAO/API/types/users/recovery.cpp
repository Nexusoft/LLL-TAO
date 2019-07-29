/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLP/include/base_address.h>
#include <LLP/types/apinode.h>

#include <TAO/API/types/exception.h>
#include <TAO/API/types/users.h>
#include <TAO/API/include/utils.h>

#include <Util/include/string.h>
#include <map>


std::map<std::string, std::vector<std::string> > mapDictionaryCache;


bool CheckCache(const std::string &strLink, std::vector<std::string> &vDictionary)
{
    if(mapDictionaryCache.find(strLink) == mapDictionaryCache.end())
        return false;

    vDictionary = mapDictionaryCache[strLink];

    return true;
}


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

        if(apiNode.Timeout(10))
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


bool GenerateRecovery(std::string &strWords, const std::vector<std::string> &vDictionary, const uint32_t nWords = 20)
{
    uint64_t nDictionarySize = vDictionary.size();

    if(nDictionarySize < nWords)
        return false;

    strWords.clear();
    for(uint32_t i = 0; i < nWords; ++i)
    {
        uint64_t nRand = LLC::GetRand();
        strWords += vDictionary[nRand % nDictionarySize];

        if(i < nWords - 1)
            strWords += " ";
    }

    return true;
}


/* Global TAO namespace. */
namespace TAO
{
    /* API Layer namespace. */
    namespace API
    {

        /* Create's a user account. */
        json::json Users::Recovery(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* The recovery phrase to generate. */
            std::string strRecovery;

            /* The link to the website to request dictionary from. */
            std::string strLink = "http://data.nexus.io/wordlists/wordlist.txt";

            /* The dictionary used to generate passphrase recovery from. */
            std::vector<std::string> vDictionary;

            /* The number of words in the recovery phrase. */
            uint8_t nWordCount = 20;

            /* Check for user supplied dictionary link. */
            if(params.find("link") != params.end())
                strLink = params["link"].get<std::string>();

            /* Check for user supplied word count. */
            if(params.find("count") != params.end())
                nWordCount = params["count"].get<uint8_t>();

            /* Check the dictionary cache first. */
            if(!CheckCache(strLink, vDictionary))
            {
                /* Fetch the dictionary. */
                if(!DownloadDictionary(vDictionary, strLink, 100))
                    throw APIException(-129, debug::safe_printstr("Failed to fetch dictionary from ", strLink));

                mapDictionaryCache[strLink] = vDictionary;
            }

            /* Generate the recovery phrase. */
            if(!GenerateRecovery(strRecovery, vDictionary, nWordCount))
                throw APIException(-130, "Failed to generate recovery phrase. ");

            /* Build a JSON response object. */
            ret["recovery"] = strRecovery;
            ret["count"] = nWordCount;

            return ret;
        }

    }
}
