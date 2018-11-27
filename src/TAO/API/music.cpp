/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/music.h>
#include <TAO/API/include/core.h>

namespace TAO
{
    namespace API
    {
        namespace Music
        {
            nlohmann::json TestFunc(bool fHelp, nlohmann::json parameters)
            {
                debug::log("Test Function!\n");

                nlohmann::json ret;
                ret.push_back("Test!");

                return ret;
            }

            void Initialize()
            {
                mapFunctions["music"]["testfunc"] = TestFunc;
            }
        }
    }
}
