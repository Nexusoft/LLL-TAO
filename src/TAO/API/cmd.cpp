/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2026

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/cmd.h>
#include <TAO/API/include/json.h>

#include <LLP/types/apinode.h>
#include <LLP/include/base_address.h>
#include <LLP/include/network.h>
#include <LLP/include/port.h>
#include <LLP/types/rpcnode.h>

#include <Util/include/args.h>
#include <Util/include/base64.h>
#include <Util/include/config.h>
#include <Util/include/debug.h>
#include <Util/include/json.h>
#include <Util/include/runtime.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Makes a connection, write packet, read response, and then disconnects. */
        template<typename ProtocolType>
        int WriteReadResponse(ProtocolType &rNode, const LLP::BaseAddress& rAddr, const std::vector<uint8_t> &vBuffer, const std::string& type)
        {
            if(!rNode.Connect(rAddr))
                return 0;

            /* Write the buffer to the socket. */
            rNode.Write(vBuffer, vBuffer.size());

            /* Read the response packet. */
            while(!rNode.INCOMING.Complete() && !config::fShutdown.load())
            {
                rNode.Flush();

                /* Catch if the connection was closed. */
                if(!rNode.Connected())
                {
                    debug::log(0, "Connection Terminated");

                    return 0;
                }

                /* Catch if the socket experienced errors. */
                if(rNode.Errors())
                {
                    debug::log(0, "Socket Error");

                    return 0;
                }

                /* Catch if the connection timed out. */
                if(rNode.Timeout(120000))
                {
                    debug::log(0, "Socket Timeout");
                    return 0;
                }

                /* Read the response packet. */
                rNode.ReadPacket();
                runtime::sleep(1);

            }

            /* Disconnect node. */
            rNode.Disconnect();

            return 1;
        }


        /* Executes an API call from the commandline */
        int CommandLineAPI(int argc, char** argv, int nArgBegin)
        {
            /* Make a local cache of our authorization header. */
            const static std::string strUserPass =
                (config::GetArg("-apiuser", "") + ":" + config::GetArg("-apipassword", ""));

            /* Initialize the underlying network resources such as sockets, etc */
            if(!LLP::NetworkInitialize())
                return debug::error(FUNCTION, "NetworkInitialize: Failed initializing network resources.");

            /* Check the parameters. */
            if(argc < nArgBegin + 1)
                return debug::error("Missing endpoint parameter");

            /* HTTP basic authentication for API */
            const std::string strUserPass64 =
                encoding::EncodeBase64(strUserPass);

            /* Parse out the endpoints. */
            const std::string strEndpoint = std::string(argv[nArgBegin]);

            /* Get our starting position. */
            const std::string::size_type nPos = strEndpoint.find('/');
            if(nPos == strEndpoint.npos)
                return debug::error("Endpoint argument requires a forward slash [ex. ./nexus -api <API-NAME>/<METHOD> <KEY>=<VALUE>]");

            /* Copy our arguments into a parameters object. */
            std::vector<std::string> vParams;
            for(int i = nArgBegin + 1; i < argc; ++i)
                vParams.push_back(argv[i]);

            /* Build the JSON request object. */
            const encoding::json jParameters = TAO::API::ParamsToJSON(vParams);

            /* Build the HTTP Header. */
            const std::string strContent = jParameters.dump();
            const std::string strReply = debug::safe_printstr
            (
                "POST /", strEndpoint.substr(0, nPos), "/", strEndpoint.substr(nPos + 1), " HTTP/1.1\r\n",
                "Date: ", debug::rfc1123Time(), "\r\n",
                "Connection: close\r\n",
                "Content-Length: ", strContent.size(), "\r\n",
                "Content-Type: application/json\r\n",
                "Origin: http://localhost:8080\r\n",
                "Server: Nexus-JSON-API\r\n",
                "Authorization: Basic ", strUserPass64, "\r\n",
                "\r\n",
                strContent
            );

            /* Convert the content into a byte buffer. */
            const std::vector<uint8_t> vBuffer(strReply.begin(), strReply.end());

            /* Make the connection to the API server. */
            const LLP::BaseAddress tAddr =
                LLP::BaseAddress(config::GetArg("-apiconnect", "127.0.0.1"), LLP::GetAPIPort());

            /* Make connection, write packet, read response, and disconnect. */
            LLP::APINode tNode;
            if(!WriteReadResponse<LLP::APINode>(tNode, tAddr, vBuffer, "API"))
                return debug::error("Couldn't connect to ", tAddr.ToStringIP());

            /* Parse response JSON. */
            const encoding::json jRet = encoding::json::parse(tNode.INCOMING.strContent);

            /* Check for errors. */
            std::string strPrint = "";
            if(jRet.find("error") != jRet.end())
            {
                /* Build our error string. */
                strPrint = debug::safe_printstr
                (
                    "(", jRet["error"]["code"].get<int64_t>(), ") ",
                     jRet["error"]["message"].get<std::string>()
                );
            }
            else
                strPrint = jRet["result"].dump(4);

            /* Check for status messages. */
            const std::string strStatus = jRet["info"]["status"].get<std::string>();
            if(strStatus.find("to be deprecated") != strStatus.npos)
            {
                /* Build our warning string with error message. */
                const std::string strMessage =
                            std::string(ANSI_COLOR_BRIGHT_YELLOW)
                          + std::string("WARNING")
                          + std::string(ANSI_COLOR_RESET)
                          + std::string(": ")
                          + jRet["info"]["status"].get<std::string>();

                printf("%s\n", strMessage.c_str());
            }

            /* Dump response to console. */
            printf("%s\n", strPrint.c_str());
            printf("[Completed in %s]\n", jRet["info"]["latency"].get<std::string>().c_str());

            return 0;
        }
    }
}
