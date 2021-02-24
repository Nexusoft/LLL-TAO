/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>

#include <TAO/API/system/types/system.h>

#include <Util/include/signals.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /*  Stop Nexus server */
        json::json System::Stop(const json::json& params, bool fHelp)
        {
            if(fHelp || params.size() != 0)
                return std::string("stop - Stop Nexus server.");

            // Shutdown will take long enough that the response should get back
            Shutdown();
            return std::string("Nexus server stopping");
        }

    }

}