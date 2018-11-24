/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/signals.h>
#include <Util/include/convert.h>
#include <Util/include/runtime.h>
#include <Util/include/filesystem.h>

#include <TAO/API/include/core.h>
#include <LLP/templates/server.h>


int main(int argc, char** argv)
{
    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Parse out the parameters */
    ParseParameters(argc, argv);


    /* Create directories if they don't exist yet. */
    if(filesystem::create_directory(GetDataDir(false)))
        printf(FUNCTION "Generated Path %s\n", __PRETTY_FUNCTION__, GetDataDir(false).c_str());


    /* Read the configuration file. */
    ReadConfigFile(mapArgs, mapMultiArgs);


    /* Create the database instances. */
    LLD::regDB = new LLD::RegisterDB("r+");
    LLD::legDB = new LLD::LedgerDB("r+");
    LLD::locDB = new LLD::LocalDB("r+");


    /* Handle Commandline switch */
    for (int i = 1; i < argc; i++)
    {
        if (!IsSwitchChar(argv[i][0]) && !(strlen(argv[i]) >= 7 && strncasecmp(argv[i], "Nexus:", 7) == 0))
        {
            //int ret = Net::CommandLineRPC(argc, argv);
            //exit(ret);

            if(argc < i + 2)
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
            while(!fShutdown)
            {
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

                apiNode.INCOMING.SetNull();
            }

            //printf("%s\n", apiNode.INCOMING.strContent.c_str());

            return 0;
        }
    }

    LLP::Server<TAO::API::Core>* CORE_SERVER = new LLP::Server<TAO::API::Core>(8080, 10, 30, false, 0, 0, 60, true, true);
    while(!fShutdown)
    {
        Sleep(1000);
    }


    TAO::Ledger::SignatureChain sigChain(GetArg("-username", "user"), GetArg("-password", "default"));
    uint512_t hashGenesis = sigChain.Generate(0, GetArg("-pin", "1235"));

    printf("Genesis %s\n", hashGenesis.ToString().c_str());

    /* Extract username and password from config. */
    printf("Username: %s\n", GetArg("-username", "user").c_str());
    printf("Password: %s\n", GetArg("-password", "default").c_str());

    return 0;
}
