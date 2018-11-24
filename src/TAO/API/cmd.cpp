/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/include/core.h>

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
            if(argc < argn + 2)
            {
                printf("Not Enough Parameters\n");

                return 0;
            }

            std::string strContent = argv[1];
            std::string strReply = strprintf(
                    "POST /api/%s/%s HTTP/1.1\r\n"
                    "Date: %s\r\n"
                    "Connection: close\r\n"
                    "Content-Length: %d\r\n"
                    "Content-Type: application/json\r\n"
                    "Server: Nexus-JSON-RPC\r\n"
                    "\r\n"
                    "%s",
                argv[1], argv[2],
                rfc1123Time().c_str(),
                strContent.size(),
                strContent.c_str());

            std::vector<uint8_t> vBuffer(strReply.begin(), strReply.end());
            TAO::API::Core apiNode;
            if(!apiNode.Connect("127.0.0.1", 8080))
            {
                printf("Couldn't Connect to API\n");

                return 0;
            }


            apiNode.Write(vBuffer);

            while(!apiNode.INCOMING.Complete())
            {
                apiNode.ReadPacket();
                //Sleep(10);
            }

            printf("%s\n", apiNode.INCOMING.strContent.c_str());

            return 0;
        }
    }

}
