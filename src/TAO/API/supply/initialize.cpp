/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/supply.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        Supply supply;

        /* Standard initialization function. */
        void Supply::Initialize()
        {
            mapFunctions["getitem"]             = Function(std::bind(&Supply::GetItem,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["transfer"]            = Function(std::bind(&Supply::Transfer,   this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["claim"]               = Function(std::bind(&Supply::Claim,      this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["createitem"]          = Function(std::bind(&Supply::CreateItem, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["updateitem"]          = Function(std::bind(&Supply::UpdateItem, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["history"]             = Function(std::bind(&Supply::History,    this, std::placeholders::_1, std::placeholders::_2));
        }
    }
}
