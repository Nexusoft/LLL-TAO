/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/ledger.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        
        /* Standard initialization function. */
        void Ledger::Initialize()
        {
            mapFunctions["create"] = Function(std::bind(&Ledger::Create, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["blockhash"] = Function(std::bind(&Ledger::BlockHash, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["block"] = Function(std::bind(&Ledger::Block, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["blocks"] = Function(std::bind(&Ledger::Blocks, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["transaction"] = Function(std::bind(&Ledger::Transaction, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["mininginfo"] = Function(std::bind(&Ledger::MiningInfo, this, std::placeholders::_1, std::placeholders::_2));
        
        }
    }
}
