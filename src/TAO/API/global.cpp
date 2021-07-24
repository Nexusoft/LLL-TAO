/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/assets/types/assets.h>
#include <TAO/API/dex/types/dex.h>
#include <TAO/API/types/commands/ledger.h>
#include <TAO/API/supply/types/supply.h>
#include <TAO/API/system/types/system.h>
#include <TAO/API/users/types/users.h>
#include <TAO/API/names/types/names.h>
#include <TAO/API/objects/types/objects.h>
#include <TAO/API/voting/types/voting.h>
#include <TAO/API/types/commands/invoices.h>
#include <TAO/API/types/commands/crypto.h>


#include <TAO/API/types/commands/finance.h>
#include <TAO/API/types/commands/register.h>
#include <TAO/API/types/commands/tokens.h>

#include <TAO/API/types/session-manager.h>

#include <Util/include/debug.h>

namespace TAO::API
{
    std::map<std::string, Base*> Commands::mapTypes;

    /*  Instantiate global instances of the API. */
    void Initialize()
    {
        debug::log(0, FUNCTION, "Initializing API");

        /* Create the API instances. */
        Commands::Register<Assets>();
        Commands::Register<Crypto>();
        Commands::Register<Market>();
        Commands::Register<Finance>();
        Commands::Register<Invoices>();
        Commands::Register<Ledger>();
        Commands::Register<Names>();
        Commands::Register<Register>();
        Commands::Register<Supply>();
        Commands::Register<System>();
        Commands::Register<Tokens>();
        Commands::Register<Users>();
        Commands::Register<Voting>();
    }


    /*  Delete global instances of the API. */
    void Shutdown()
    {
        debug::log(0, FUNCTION, "Shutting down API");

        /* Shut down our subsequent API's */
        Commands::Shutdown();
    }
}
