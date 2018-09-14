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

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/sigchain.h>

#include <TAO/Register/objects/account.h>
#include <TAO/Register/objects/token.h>
#include <TAO/Register/include/state.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/stream.h>
#include <TAO/Operation/include/execute.h>

#include <LLC/include/random.h>

#include <LLD/include/global.h>
#include <LLD/include/version.h>

#include <LLP/include/tritium.h>
#include <LLP/templates/server.h>

int main(int argc, char** argv)
{
    /* Handle all the signals with signal handler method. */
    SetupSignals();


    /* Parse out the parameters */
    ParseParameters(argc, argv);


    /* Read the configuration file. */
    if (!boost::filesystem::is_directory(GetDataDir(false)))
    {
        fprintf(stderr, "Error: Specified directory does not exist\n");

        return 0;
    }
    ReadConfigFile(mapArgs, mapMultiArgs);


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


    while(true)
    {
        Sleep(1000);
    }


    return 0;
}
