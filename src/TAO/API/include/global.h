/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_API_INCLUDE_GLOBAL_H
#define NEXUS_TAO_API_INCLUDE_GLOBAL_H

#include <TAO/API/types/assets.h>
#include <TAO/API/types/dex.h>
#include <TAO/API/types/ledger.h>
#include <TAO/API/types/register.h>
#include <TAO/API/types/rpc.h>
#include <TAO/API/types/supply.h>
#include <TAO/API/types/system.h>
#include <TAO/API/types/tokens.h>
#include <TAO/API/types/users.h>
#include <TAO/API/types/finance.h>
#include <TAO/API/types/names.h>
#include <TAO/API/types/objects.h>
#include <TAO/API/types/voting.h>
#include <TAO/API/types/invoices.h>
#include <TAO/API/types/crypto.h>

namespace TAO
{
    namespace API
    {
        extern Assets*      assets;
        extern Ledger*      ledger;
        extern Register*    reg;
        extern RPC*         RPCCommands;
        extern Supply*      supply;
        extern System*      system;
        extern Tokens*      tokens;
        extern Users*       users;
        extern Finance*     finance;
        extern Names*       names;
        extern DEX*         dex;
        extern Voting*      voting;
        extern Invoices*    invoices;
        extern Crypto*      crypto;


        /** Initialize
         *
         *  Instantiate global instances of the API.
         *
         **/
        void Initialize();


        /** Shutdown
         *
         *  Delete global instances of the API.
         *
         **/
        void Shutdown();
    }
}


#endif
