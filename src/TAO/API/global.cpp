/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/types/sessionmanager.h>

#include <Util/include/debug.h>

namespace TAO::API
{
    std::map<std::string, Base*> Commands::mapTypes;

    /* The API global instance pointers. */
    Assets*     assets;
    Ledger*     ledger;
    Register*   reg;

    #ifndef NO_WALLET
    RPC*        legacy;
    #endif

    Supply*     supply;
    System*     system;
    Tokens*     tokens;
    Users*      users;
    Finance*    finance;
    Names*      names;
    Market*     dex;
    Voting*     voting;
    Invoices*   invoices;
    Crypto*     crypto;


    /*  Instantiate global instances of the API. */
    void Initialize()
    {
        debug::log(0, FUNCTION, "Initializing API");

        /* Create the API instances. */
        assets      = new Assets();
        ledger      = new Ledger();
        reg         = new Register();

        #ifndef NO_WALLET
        legacy = new RPC();
        #endif

        supply      = new Supply();
        system      = new System();
        tokens      = new Tokens();
        users       = new Users();
        finance     = new Finance();
        names       = new Names();
        dex         = new Market();
        voting      = new Voting();
        invoices    = new Invoices();
        crypto      = new Crypto();
    }


    /*  Delete global instances of the API. */
    void Shutdown()
    {
        debug::log(0, FUNCTION, "Shutting down API");

        if(assets)
            delete assets;

        if(ledger)
            delete ledger;

        if(reg)
            delete reg;

        #ifndef NO_WALLET
        if(legacy)
            delete legacy;
        #endif

        if(supply)
            delete supply;

        if(system)
            delete system;

        if(tokens)
            delete tokens;

        if(users)
            delete users;

        if(finance)
            delete finance;

        if(names)
            delete names;

        if(dex)
            delete dex;

        if(voting)
            delete voting;

        if(invoices)
            delete invoices;

        if(crypto)
            delete crypto;
    }
}
