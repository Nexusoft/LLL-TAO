/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/tokens.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        Tokens tokens;

        /* Standard initialization function. */
        void Tokens::Initialize()
        {
            mapFunctions["get"]           = Function(std::bind(&Tokens::Get,            this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["debit"]         = Function(std::bind(&Tokens::Debit,          this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["create"]        = Function(std::bind(&Tokens::Create,         this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["credit"]        = Function(std::bind(&Tokens::Credit,         this, std::placeholders::_1, std::placeholders::_2));
        }
    }
}
