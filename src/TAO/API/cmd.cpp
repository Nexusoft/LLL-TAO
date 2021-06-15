/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/include/cmd.h>
#include <TAO/API/include/json.h>

#include <LLP/types/apinode.h>
#include <LLP/include/base_address.h>
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
            /* Check the parameters. */
            if(argc < nArgBegin + 1)
                return debug::error("Missing endpoint parameter");

            /* HTTP basic authentication for API */
            const std::string strUserPass64 =
                encoding::EncodeBase64(config::mapArgs["-apiuser"] + ":" + config::mapArgs["-apipassword"]);

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
                strPrint = jRet["error"]["message"];
            else
                strPrint = jRet["result"].dump(4);

            /* Dump response to console. */
            printf("%s\n", strPrint.c_str());
            printf("[%s]\n", jRet["info"]["latency"].get<std::string>().c_str());

            return 0;
        }


        /* Executes an API call from the commandline */
        int CommandLineRPC(int argc, char** argv, int nArgBegin)
        {
            /* Check the parameters. */
            if(argc < nArgBegin + 1)
            {
                debug::log(0, "Not Enough Parameters");

                return 0;
            }

            /* Check RPC user/pass are set */
            if(config::mapArgs["-rpcuser"] == "" && config::mapArgs["-rpcpassword"] == "")
                throw std::runtime_error(debug::safe_printstr(
                    "You must set rpcpassword=<password> in the configuration file: ",
                    config::GetConfigFile(), "\n",
                    "If the file does not exist, create it with owner-readable-only file permissions.\n"));

             /* HTTP basic authentication for RPC */
            std::string strUserPass64 = encoding::EncodeBase64(config::mapArgs["-rpcuser"] + ":" + config::mapArgs["-rpcpassword"]);

            /* Build the JSON request object. */
            encoding::json jParameters = encoding::json::array();
            for(int i = nArgBegin + 1; i < argc; ++i)
            {
                std::string strArg = argv[i];
                // if the paramter is a JSON list or array then we need to parse it
                if(strArg.compare(0,1,"{") == 0 || strArg.compare(0,1,"[") == 0)
                    jParameters.push_back(encoding::json::parse(argv[i]));
                else
                    jParameters.push_back(argv[i]);
            }

            /* Build the HTTP Header. */
            encoding::json jBody =
            {
                {"method", argv[nArgBegin]},
                {"params", jParameters},
                {"id", 1}
            };

            std::string strContent = jBody.dump();
            std::string strReply = debug::safe_printstr(
                    "POST / HTTP/1.1\r\n",
                    "Date: ", debug::rfc1123Time(), "\r\n",
                    "Connection: close\r\n",
                    "Content-Length: ", strContent.size(), "\r\n",
                    "Content-Type: application/json\r\n",
                    "Server: Nexus-JSON-RPC\r\n",
                    "Authorization: Basic ", strUserPass64, "\r\n",
                    "\r\n",
                    strContent);

            /* Convert the content into a byte buffer. */
            std::vector<uint8_t> vBuffer(strReply.begin(), strReply.end());

            std::string strAddr = config::GetArg("-rpcconnect", "127.0.0.1");
            uint16_t nPort = static_cast<uint16_t>(config::GetArg(std::string("-rpcport"), config::fTestNet.load() ? TESTNET_RPC_PORT : MAINNET_RPC_PORT));
            uint16_t nSSLPort = static_cast<uint16_t>(config::GetArg(std::string("-rpcsslport"), config::fTestNet.load() ? TESTNET_RPC_SSL_PORT : MAINNET_RPC_SSL_PORT));

            bool fSSL = config::GetBoolArg(std::string("-rpcssl")) || config::GetBoolArg(std::string("-rpcsslrequired"));
            bool fSSLRequired = config::GetBoolArg(std::string("-rpcsslrequired"));

            std::string strRequest;
            std::string strResponse;
            bool fSuccess = false;

            /* If SSL is enabled then attempt the connection using SSL */
            if(fSSL)
            {
                LLP::RPCNode rpcNode;
                rpcNode.SetSSL(true);

                LLP::BaseAddress addr(strAddr, nSSLPort);

                /* Make connection, write packet, read response, and disconnect. */
                if(WriteReadResponse<LLP::RPCNode>(rpcNode, addr, vBuffer, "RPC"))
                {
                    /* If successful read the response */
                    strResponse = rpcNode.INCOMING.strContent;
                    strRequest = rpcNode.INCOMING.strRequest;
                    fSuccess = true;
                }
            }

            /* If SSL wasn't successful or enabled and is not required, try insecure connection  */
            if(!fSuccess && !fSSLRequired)
            {
                LLP::RPCNode rpcNode;
                rpcNode.SetSSL(false);

                LLP::BaseAddress addr(strAddr, nPort);

                /* Make connection, write packet, read response, and disconnect. */
                if(WriteReadResponse<LLP::RPCNode>(rpcNode, addr, vBuffer, "RPC"))
                {
                    /* If successful read the response */
                    strResponse = rpcNode.INCOMING.strContent;
                    strRequest = rpcNode.INCOMING.strRequest;
                    fSuccess = true;
                }
            }

            /* Break out if we couldn't establish a connection */
            if(!fSuccess)
                return 0;


            /* Dump the response to the console. */
            int nRet = 0;
            std::string strPrint = "";
            if(strResponse.length() > 0)
            {
                encoding::json ret = encoding::json::parse(strResponse);

                if(!ret["error"].is_null())
                {
                    strPrint = ret["error"]["message"];
                    nRet     = ret["error"]["code"];
                }
                else
                {
                    if(ret["result"].is_string())
                        strPrint = ret["result"].get<std::string>();
                    else
                        strPrint = ret["result"].dump(4);
                }
            }
            else
            {
                // If the server returned no content then just output the packet header type, which will include any HTTP error code
                strPrint = strRequest;
            }

            // output to console
            printf("%s\n", strPrint.c_str());

            return nRet;
        }
    }
}
