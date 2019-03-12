/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/assets.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        Assets assets;

        /* Standard initialization function. */
        void Assets::Initialize()
        {
            mapFunctions["create"]             = Function(std::bind(&Assets::Create,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get"]                = Function(std::bind(&Assets::Get,       this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["transfer"]           = Function(std::bind(&Assets::Transfer,  this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["history"]            = Function(std::bind(&Assets::History,   this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["tokenize"]           = Function(std::bind(&Assets::Tokenize,  this, std::placeholders::_1, std::placeholders::_2));
        }
    }
}
