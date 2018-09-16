/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/args.h>
#include <Util/include/config.h>
#include <Util/include/signals.h>
#include <Util/include/convert.h>


#include <LLD/include/global.h>
#include <LLP/include/tritium.h>
#include <LLP/templates/server.h>



int main(int argc, char** argv)
{
    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Parse out the parameters */
    ParseParameters(argc, argv);


    /* Create directories if they don't exist yet. */
    if(boost::filesystem::create_directories(GetDataDir(false)))
        printf(FUNCTION "Generated Path %s\n", __PRETTY_FUNCTION__, GetDataDir(false).c_str());


    /* Read the configuration file. */
    ReadConfigFile(mapArgs, mapMultiArgs);


    /* Create the database instances. */
    LLD::regDB = new LLD::RegisterDB("r+");
    LLD::legDB = new LLD::LedgerDB("r+");


    /* Handle Commandline switch */
    for (int i = 1; i < argc; i++)
    {
        if (!IsSwitchChar(argv[i][0]) && !(strlen(argv[i]) >= 7 && strncasecmp(argv[i], "Nexus:", 7) == 0))
        {
            //int ret = Net::CommandLineRPC(argc, argv);
            //exit(ret);
        }
    }


    /* Create an LLP Server. */
    LLP::Server<LLP::TritiumNode>* SERVER = new LLP::Server<LLP::TritiumNode>(1111, 10, 30, false, 0, 0, 60, GetBoolArg("-listen", true), true);

    if(mapMultiArgs["-addnode"].size() > 0)
        for(auto node : mapMultiArgs["-addnode"])
            SERVER->AddConnection(node, 1111);

    while(!fShutdown)
    {
        Sleep(1000);
    }


    return 0;
}
