/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_API_INCLUDE_GLOBAL_H
#define NEXUS_TAO_API_INCLUDE_GLOBAL_H

#include <TAO/API/assets/types/assets.h>
#include <TAO/API/dex/types/dex.h>
#include <TAO/API/ledger/types/ledger.h>
#include <TAO/API/register/types/register.h>
#include <TAO/API/rpc/types/rpc.h>
#include <TAO/API/supply/types/supply.h>
#include <TAO/API/system/types/system.h>
#include <TAO/API/tokens/types/tokens.h>
#include <TAO/API/users/types/users.h>
#include <TAO/API/finance/types/finance.h>
#include <TAO/API/names/types/names.h>
#include <TAO/API/objects/types/objects.h>
#include <TAO/API/voting/types/voting.h>
#include <TAO/API/invoices/types/invoices.h>
#include <TAO/API/crypto/types/crypto.h>
#include <TAO/API/p2p/types/p2p.h>

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
        extern P2P*      p2p;

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
