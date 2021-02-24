/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/system/types/system.h>
#include <TAO/API/include/utils.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Standard initialization function. */
        void System::Initialize()
        {
            mapFunctions["get/info"]         = Function(std::bind(&System::GetInfo,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["get/metrics"]    = Function(std::bind(&System::Metrics,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["stop"]             = Function(std::bind(&System::Stop,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/peers"]       = Function(std::bind(&System::ListPeers,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/lisp-eids"]   = Function(std::bind(&System::LispEIDs, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["validate/address"] = Function(std::bind(&System::Validate,    this, std::placeholders::_1, std::placeholders::_2));
        }

    }

}