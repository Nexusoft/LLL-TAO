/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLP/types/corenode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/include/base_address.h>
#include <TAO/API/include/cmd.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <Util/include/json.h>
#include <Util/include/config.h>
#include <Util/include/base64.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Executes an API call from the commandline */
        int CommandLineAPI(int argc, char** argv, int argn)
        {
            /* Check the parameters. */
            if(argc < argn + 1)
            {
                debug::error("missing endpoint parameter");

                return 0;
            }

            /* Parse out the endpoints. */
            std::string endpoint = std::string(argv[argn]);
            std::string::size_type pos = endpoint.find('/');
            if(pos == endpoint.npos)
            {
                debug::error("\nendpoint argument requires a forward slash [ex. ./nexus -api <API-NAME>/<METHOD> <KEY>=<VALUE>]");

                return 0;
            }

            /* Build the JSON request object. */
            json::json parameters;

            /* Keep track of previous parameter. */
            std::string prev;
            for(int i = argn + 1; i < argc; ++i)
            {
                /* Parse out the key / values. */
                std::string arg = std::string(argv[i]);
                std::string::size_type pos = arg.find('=', 0);

                /* Watch for missing delimiter. */
                if(pos == arg.npos)
                {
                    /* Append this data with URL encoding. */
                    std::string value = parameters[prev];
                    value.append(" " + arg);
                    parameters[prev] = value;

                    continue;
                }

                /* Set the previous argument. */
                prev = arg.substr(0, pos);

                /* Add to parameters object. */
                parameters[prev] = arg.substr(pos + 1);
            }


            /* Build the HTTP Header. */
            std::string strContent = parameters.dump();
            std::string strReply = debug::safe_printstr(
                    "POST /", endpoint.substr(0, pos), "/", endpoint.substr(pos + 1), " HTTP/1.1\r\n",
                    "Date: ", debug::rfc1123Time(), "\r\n",
                    "Connection: close\r\n",
                    "Content-Length: ", strContent.size(), "\r\n",
                    "Content-Type: application/json\r\n",
                    "Server: Nexus-JSON-API\r\n",
                    "\r\n",
                    strContent);

            /* Convert the content into a byte buffer. */
            std::vector<uint8_t> vBuffer(strReply.begin(), strReply.end());

            /* Make the connection to the API server. */
            LLP::CoreNode apiNode;

            LLP::BaseAddress addr("127.0.0.1", 8080);

            if(!apiNode.Connect(addr))
            {
                debug::log(0, "Couldn't Connect to API");

                return 0;
            }

            /* Write the buffer to the socket. */
            apiNode.Write(vBuffer, vBuffer.size());
            apiNode.Flush();

            /* Read the response packet. */
            while(!apiNode.INCOMING.Complete() && !config::fShutdown.load())
            {

                /* Catch if the connection was closed. */
                if(!apiNode.Connected())
                {
                    debug::log(0, "Connection Terminated");
                    return 0;
                }

                /* Catch if the socket experienced errors. */
                if(apiNode.Errors())
                {
                    debug::log(0, "Socket Error");
                    return 0;
                }

                /* Catch if the connection timed out. */
                if(apiNode.Timeout(120))
                {
                    debug::log(0, "Socket Timeout");
                    return 0;
                }

                /* Read the response packet. */
                apiNode.ReadPacket();
                runtime::sleep(10);
            }

            /* Parse response JSON. */
            json::json ret = json::json::parse(apiNode.INCOMING.strContent);

            /* Check for errors. */
            std::string strPrint = "";
            if(ret.find("error") != ret.end())
                strPrint = ret["error"]["message"];
            else
                strPrint = ret["result"].dump(4);

            /* Dump response to console. */
            printf("%s\n", strPrint.c_str());

            return 0;
        }


        /* Executes an API call from the commandline */
        int CommandLineRPC(int argc, char** argv, int argn)
        {
            /* Check the parameters. */
            if(argc < argn + 1)
            {
                debug::log(0, "Not Enough Parameters");

                return 0;
            }

            /** Check RPC user/pass are set */
            if (config::mapArgs["-rpcuser"] == "" && config::mapArgs["-rpcpassword"] == "")
                throw std::runtime_error(debug::safe_printstr(
                    "You must set rpcpassword=<password> in the configuration file: ",
                    config::GetConfigFile(), "\n",
                    "If the file does not exist, create it with owner-readable-only file permissions.\n"));

             // HTTP basic authentication
            std::string strUserPass64 = encoding::EncodeBase64(config::mapArgs["-rpcuser"] + ":" + config::mapArgs["-rpcpassword"]);

            /* Build the JSON request object. */
            json::json parameters = json::json::array();
            for(int i = argn + 1; i < argc; ++i)
            {
                std::string strArg = argv[i];
                // if the paramter is a JSON list or array then we need to parse it
                if( strArg.compare(0,1,"{") == 0 || strArg.compare(0,1,"[") == 0)
                    parameters.push_back(json::json::parse(argv[i]));
                else
                    parameters.push_back(argv[i]);
            }
            /* Build the HTTP Header. */
            json::json body =
            {
                {"method", argv[argn]},
                {"params", parameters},
                {"id", 1}
            };

            std::string strContent = body.dump();
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

            /* Make the connection to the API server. */
            LLP::RPCNode rpcNode;

            std::string strAddr = config::GetArg("-rpcconnect", "127.0.0.1");
            uint16_t port = config::GetArg("-rpcport",config::fTestNet? 8336 : 9336);

            LLP::BaseAddress addr(strAddr, port);

            if(!rpcNode.Connect(addr))
            {
                debug::log(0, "Couldn't Connect to RPC");

                return 0;
            }

            /* Write the buffer to the socket. */
            rpcNode.Write(vBuffer, vBuffer.size());

            /* Read the response packet. */
            while(!rpcNode.INCOMING.Complete() && !config::fShutdown.load())
            {
                rpcNode.Flush();

                /* Catch if the connection was closed. */
                if(!rpcNode.Connected())
                {
                    debug::log(0, "Connection Terminated");

                    return 0;
                }

                /* Catch if the socket experienced errors. */
                if(rpcNode.Errors())
                {
                    debug::log(0, "Socket Error");

                    return 0;
                }

                /* Read the response packet. */
                rpcNode.ReadPacket();
                runtime::sleep(10);
            }

            /* Dump the response to the console. */
            int nRet = 0;
            std::string strPrint = "";
            if(rpcNode.INCOMING.strContent.length() > 0)
            {
                json::json ret = json::json::parse(rpcNode.INCOMING.strContent);

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
                strPrint = rpcNode.INCOMING.strRequest;
            }

            // output to console
            debug::log(0, strPrint);

            return nRet;
        }
    }
}
