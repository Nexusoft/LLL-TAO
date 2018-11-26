/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/include/core.h>
#include <TAO/API/include/cmd.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

#include <Util/include/json.h>

namespace TAO
{
    namespace API
    {
        /* Executes an API call from the commandline */
        int CommandLine(int argc, char** argv, int argn)
        {
            /* Check the parameters. */
            if(argc < argn + 3)
            {
                printf("Not Enough Parameters\n");

                return 0;
            }

            /* Build the JSON request object. */
            nlohmann::json parameters;
            for(int i = argn + 2; i < argc; i++)
                parameters.push_back(argv[i]);

            /* Build the HTTP Header. */
            std::string strContent = parameters.dump();
            std::string strReply = strprintf(
                    "POST /%s/%s HTTP/1.1\r\n"
                    "Date: %s\r\n"
                    "Connection: close\r\n"
                    "Content-Length: %d\r\n"
                    "Content-Type: application/json\r\n"
                    "Server: Nexus-JSON-API\r\n"
                    "\r\n"
                    "%s",
                argv[argn], argv[argn + 1],
                rfc1123Time().c_str(),
                strContent.size(),
                strContent.c_str());

            /* Convert the content into a byte buffer. */
            std::vector<uint8_t> vBuffer(strReply.begin(), strReply.end());

            /* Make the connection to the API server. */
            TAO::API::Core apiNode;
            if(!apiNode.Connect("127.0.0.1", 8080))
            {
                printf("Couldn't Connect to API\n");

                return 0;
            }

            /* Write the buffer to the socket. */
            apiNode.Write(vBuffer);

            /* Read the response packet. */
            while(!apiNode.INCOMING.Complete())
            {

                /* Catch if the connection was closed. */
                if(!apiNode.Connected())
                {
                    printf("Connection Terminated\n");

                    return 0;
                }

                /* Catch if the socket experienced errors. */
                if(apiNode.Errors())
                {
                    printf("Socket Error\n");

                    return 0;
                }

                /* Catch if the connection timed out. */
                if(apiNode.Timeout(30))
                {
                    printf("Socket Timeout\n");

                    return 0;
                }

                /* Read the response packet. */
                apiNode.ReadPacket();
                Sleep(10);
            }

            /* Dump the response to the console. */
            printf("%s\n", apiNode.INCOMING.strContent.c_str());

            return 0;
        }
    }

}
