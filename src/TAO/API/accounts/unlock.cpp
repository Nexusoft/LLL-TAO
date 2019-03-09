/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/accounts.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Unlock an account for mining (TODO: make this much more secure) */
        json::json Accounts::Unlock(const json::json& params, bool fHelp)
        {
            /* Check for username parameter. */
            if(params.find("session") == params.end())
                throw APIException(-23, "Missing Session");

            /* Check for password parameter. */
            if(params.find("pin") == params.end())
                throw APIException(-24, "Missing Pin");

            /* Check if already unlocked. */
            if(pairUnlocked.first != 0)
                throw APIException(-26, "Account already unlocked");

            /* Extract the session. */
            uint64_t nSession = std::stoull(params["session"].get<std::string>());

            /* Extract the PIN. */
            pairUnlocked = std::make_pair(nSession, params["pin"].get<std::string>().c_str());

            return true;
        }
    }
}
