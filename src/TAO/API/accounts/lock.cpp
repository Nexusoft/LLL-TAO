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

        /* Lock an account for mining (TODO: make this much more secure) */
        json::json Accounts::Lock(const json::json& params, bool fHelp)
        {
            /* Restrict Unlock / Lock to sessionless API */
            if(config::fAPISessions)
                throw APIException(-23, "Lock not supported for session-based API");
            
            /* Check if already unlocked. */
            if(strActivePIN == "")
                throw APIException(-26, "Account already locked");


            /* Clear the pin */
            LOCK(MUTEX);

            strActivePIN = "";

            return true;
        }
    }
}
