/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>

#include <Util/include/debug.h>

namespace TAO 
{
    namespace API
    {
        /* The API global instance pointers. */
        Assets* assets;
        Ledger* ledger;
        Lisp* lisp;
        Register* reg;
        RPC* RPCCommands;
        Supply* supply;
        System* system;
        Tokens* tokens;
        Users* users;

        
        /*  Instantiate global instances of the API. */
        void Initialize()
        {
            debug::log(0, "Initializing API");

            /* Create the API instances. */
            assets      = new Assets();
            ledger      = new Ledger();
            lisp        = new Lisp();
            reg         = new Register();
            RPCCommands = new RPC();
            supply      = new Supply();
            system      = new System();
            tokens      = new Tokens();
            users       = new Users();
        }


        /*  Delete global instances of the API. */
        void Shutdown()
        {
            debug::log(0, "Shutting down API");

            if(assets)
                delete assets;

            if(ledger)
                delete ledger;

            if(lisp)
                delete lisp;

            if(reg)
                delete reg;

            if(RPCCommands)
                delete RPCCommands;

            if(supply)
                delete supply;

            if(system)
                delete system;

            if(tokens)
                delete tokens;

            if(users)
                delete users;

        }    
    }
}