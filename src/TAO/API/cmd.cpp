/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/include/cmd.h>

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
        int WriteReadResponse(ProtocolType &node, const LLP::BaseAddress& addr, std::vector<uint8_t> &vBuffer, const std::string& type)
        {
            if(!node.Connect(addr))
            {
                debug::log(0, "Couldn't Connect to ", type);

                return 0;
            }

            /* Write the buffer to the socket. */
            node.Write(vBuffer, vBuffer.size());

            /* Read the response packet. */
            while(!node.INCOMING.Complete() && !config::fShutdown.load())
            {
                node.Flush();

                /* Catch if the connection was closed. */
                if(!node.Connected())
                {
                    debug::log(0, "Connection Terminated");

                    return 0;
                }

                /* Catch if the socket experienced errors. */
                if(node.Errors())
                {
                    debug::log(0, "Socket Error");

                    return 0;
                }

                /* Catch if the connection timed out. */
                if(node.Timeout(120000))
                {
                    debug::log(0, "Socket Timeout");
                    return 0;
                }

                /* Read the response packet. */
                node.ReadPacket();
                runtime::sleep(1);

            }

            /* Disconnect node. */
            node.Disconnect();

            return 1;
        }


        /* Executes an API call from the commandline */
        int CommandLineAPI(int argc, char** argv, int argn)
        {
            /* Check the parameters. */
            if(argc < argn + 1)
            {
                debug::error("missing endpoint parameter");

                return 0;
            }


            /* HTTP basic authentication for API */
            std::string strUserPass64 = encoding::EncodeBase64(config::mapArgs["-apiuser"] + ":" + config::mapArgs["-apipassword"]);

            /* Parse out the endpoints. */
            std::string endpoint = std::string(argv[argn]);
            std::string::size_type pos = endpoint.find('/');
            if(pos == endpoint.npos)
            {
                debug::error("\nendpoint argument requires a forward slash [ex. ./nexus -api <API-NAME>/<METHOD> <KEY>=<VALUE>]");

                return 0;
            }

            /* Flag indicating that this is a list API call, in which case we need to parse the params differently */
            bool fIsList = endpoint.find("/list/") != endpoint.npos;

            std::vector<std::string> strListKeywords = {"genesis", "username", "name", "verbose", "page", "limit", "sort", "order", "where"};

            /* Build the JSON request object. */
            json::json parameters;

            /* Keep track of previous parameter. */
            std::string prev;

            /* flag indicating the parameter is a where clause  */
            bool fWhere = false;
            for(int i = argn + 1; i < argc; ++i)
            {
                /* Parse out the key / values. */
                std::string arg = std::string(argv[i]);
                std::string::size_type pos = std::string::npos;

                if(fIsList)
                    pos = arg.find_first_of("><=", 0);
                else
                    pos = arg.find('=', 0);

                /* Watch for missing delimiter. */
                if(pos == arg.npos)
                {
                    if(fWhere)
                    {
                        /* Get the last added clause */
                        json::json jsonClause = parameters["where"].back();

                        /* Append the next piece of data to it */
                        std::string strValue = jsonClause["value"];
                        strValue.append(" " + arg);
                        jsonClause["value"] = strValue;
                    }
                    else
                    {
                        /* Append this data to the previously stored parameter. */
                        std::string value = parameters[prev];
                        value.append(" " + arg);
                        parameters[prev] = value;
                    }

                    continue;
                }

                /* Set the previous argument. */
                prev = arg.substr(0, pos);

                /* If this is a list command, check to see if this is a where clause (not a keyword parameter supported by list)*/
                if(fIsList && std::find(strListKeywords.begin(), strListKeywords.end(), prev) == strListKeywords.end())
                {
                    fWhere = true;

                    /* add the where clause */
                    json::json jsonClause;
                    jsonClause["field"] = prev;

                    /* Check to see if the parameter delimter is a two-character operand */
                    if(arg.find_first_of("><=", pos+1) != arg.npos)
                    {
                        /* Extract the operand */
                        jsonClause["op"] = arg.substr(pos, 2);
                        pos++;
                    }
                    else
                    {
                        /* Extract the operand */
                        jsonClause["op"] = arg.substr(pos, 1);
                    }

                    /* Extract the value */
                    jsonClause["value"] = arg.substr(pos + 1);

                    /* Add it to the where params*/
                    parameters["where"].push_back(jsonClause);
                }
                else
                {
                    fWhere = false;

                    // if the paramter is a JSON list or array then we need to parse it
                    if(arg.compare(pos + 1,1,"{") == 0 || arg.compare(pos + 1,1,"[") == 0)
                        parameters[prev]=json::json::parse(arg.substr(pos + 1));
                    else
                        parameters[prev] = arg.substr(pos + 1);
                }
            }


            /* Build the HTTP Header. */
            std::string strContent = parameters.dump();
            std::string strReply = debug::safe_printstr(
                    "POST /", endpoint.substr(0, pos), "/", endpoint.substr(pos + 1), " HTTP/1.1\r\n",
                    "Date: ", debug::rfc1123Time(), "\r\n",
                    "Connection: close\r\n",
                    "Content-Length: ", strContent.size(), "\r\n",
                    "Content-Type: application/json\r\n",
                    "Origin: http://localhost:8080\r\n",
                    "Server: Nexus-JSON-API\r\n",
                    "Authorization: Basic ", strUserPass64, "\r\n",
                    "\r\n",
                    strContent);

            /* Convert the content into a byte buffer. */
            std::vector<uint8_t> vBuffer(strReply.begin(), strReply.end());

            /* Make the connection to the API server. */
            

            std::string strAddr = config::GetArg("-apiconnect", "127.0.0.1");
            uint16_t nPort = static_cast<uint16_t>(config::GetArg(std::string("-apiport"), config::fTestNet.load() ? TESTNET_API_PORT : MAINNET_API_PORT));
            uint16_t nSSLPort = static_cast<uint16_t>(config::GetArg(std::string("-apisslport"), config::fTestNet.load() ? TESTNET_API_SSL_PORT : MAINNET_API_SSL_PORT));

            bool fSSL = config::GetBoolArg(std::string("-apissl")) || config::GetBoolArg(std::string("-apisslrequired"));
            bool fSSLRequired = config::GetBoolArg(std::string("-apisslrequired"));

            std::string strResponse;
            bool fSuccess = false;

            /* If SSL is enabled then attempt the connection using SSL */
            if(fSSL)
            {
                LLP::APINode apiNode;
                apiNode.SetSSL(true);
                
                LLP::BaseAddress addr(strAddr, nSSLPort);

                /* Make connection, write packet, read response, and disconnect. */
                if(WriteReadResponse<LLP::APINode>(apiNode, addr, vBuffer, "API"))
                {
                    /* If successful read the response */
                    strResponse = apiNode.INCOMING.strContent;
                    fSuccess = true;
                }
            }

            /* If SSL wasn't successful or enabled and is not required, try insecure connection  */
            if(!fSuccess && !fSSLRequired)
            {
                LLP::APINode apiNode;
                apiNode.SetSSL(false);
                
                LLP::BaseAddress addr(strAddr, nPort);

                /* Make connection, write packet, read response, and disconnect. */
                if(WriteReadResponse<LLP::APINode>(apiNode, addr, vBuffer, "API"))
                {
                    /* If successful read the response */
                    strResponse = apiNode.INCOMING.strContent;
                    fSuccess = true;
                }
            }

            /* Break out if we couldn't establish a connection */
            if(!fSuccess)
                return 0;

            /* Parse response JSON. */
            json::json ret = json::json::parse(strResponse);

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

            /* Check RPC user/pass are set */
            if(config::mapArgs["-rpcuser"] == "" && config::mapArgs["-rpcpassword"] == "")
                throw std::runtime_error(debug::safe_printstr(
                    "You must set rpcpassword=<password> in the configuration file: ",
                    config::GetConfigFile(), "\n",
                    "If the file does not exist, create it with owner-readable-only file permissions.\n"));

             /* HTTP basic authentication for RPC */
            std::string strUserPass64 = encoding::EncodeBase64(config::mapArgs["-rpcuser"] + ":" + config::mapArgs["-rpcpassword"]);

            /* Build the JSON request object. */
            json::json parameters = json::json::array();
            for(int i = argn + 1; i < argc; ++i)
            {
                std::string strArg = argv[i];
                // if the paramter is a JSON list or array then we need to parse it
                if(strArg.compare(0,1,"{") == 0 || strArg.compare(0,1,"[") == 0)
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
                json::json ret = json::json::parse(strResponse);

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
