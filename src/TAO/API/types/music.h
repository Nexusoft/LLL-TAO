/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2018] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_API_TYPES_MUSIC_H
#define NEXUS_TAO_API_TYPES_MUSIC_H

#include <functional>

#include <Util/include/json.h>

namespace TAO
{
    namespace API
    {
        namespace Music
        {
            extern std::map<std::string, std::function<nlohmann::json(bool, nlohmann::json)> > mapFunctions;

            void Initialize();

            nlohmann::json TestFunc(bool fHelp, nlohmann::json parameters);
        }
    }
}

#endif
