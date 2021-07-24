/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/assets.h>
#include <TAO/API/include/check.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Assets::Initialize()
    {
        mapFunctions["create/asset"]             = Function(std::bind(&Assets::Create,    this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["update/asset"]             = Function(std::bind(&Assets::Update,    this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/asset"]                = Function(std::bind(&Assets::Get,       this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["transfer/asset"]           = Function(std::bind(&Assets::Transfer,  this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["claim/asset"]              = Function(std::bind(&Assets::Claim,  this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["list/asset/history"]       = Function(std::bind(&Assets::History,   this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["tokenize/asset"]           = Function(std::bind(&Assets::Tokenize,  this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["get/schema"]                = Function(std::bind(&Assets::GetSchema,       this, std::placeholders::_1, std::placeholders::_2));
    }
}
