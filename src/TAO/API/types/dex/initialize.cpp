/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/dex.h>

#include <TAO/API/include/utils.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Standard initialization function. */
        void DEX::Initialize()
        {
            mapFunctions["buy"]  = Function(std::bind(&DEX::Buy,  this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["sell"] = Function(std::bind(&DEX::Sell, this, std::placeholders::_1, std::placeholders::_2));
        }
    }
}
