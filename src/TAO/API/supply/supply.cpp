/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/supply.h>

namespace TAO::API
{
    /* Declaration of the api */
    Supply supply;


    /* Standard initialization function. */
    void Supply::Initialize()
    {
        mapFunctions["getitem"]             = Function(std::bind(&Supply::GetItem,   this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["transfer"]            = Function(std::bind(&Supply::Transfer,  this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["submit"]              = Function(std::bind(&Supply::Submit,    this, std::placeholders::_1, std::placeholders::_2));
        mapFunctions["history"]             = Function(std::bind(&Supply::History,   this, std::placeholders::_1, std::placeholders::_2));
    }


    /* Get's the description of an item. */
    json::json Supply::GetItem(const json::json& jsonParams, bool fHelp)
    {
        json::json ret = {"getitem", jsonParams};

        //uint256_t hashAddress;
        //TAO::Register::State state;
        //LLD::regDB->ReadState(hashAddress, state);

        return ret;
    }


    /* Transfers an item. */
    json::json Supply::Transfer(const json::json& jsonParams, bool fHelp)
    {
        json::json ret = {"transfer", jsonParams};

        //TAO::Ledger tx;
        //tx << OP::TRANSFER << hashRegister << hashAddressTo;

        return ret;
    }


    /* Submits an item. */
    json::json Supply::Submit(const json::json& jsonParams, bool fHelp)
    {
        json::json ret = {"submit", jsonParams};
        //TAO::Ledger::Transaction tx;
        //set genesis
        //tx << OP::REGISTER << hashAddress << hashOwner << regData;

        return ret;
    }


    /* Gets the history of an item. */
    json::json Supply::History(const json::json& jsonParams, bool fHelp)
    {
        json::json ret = {"history", jsonParams};

        return ret;
    }
}
