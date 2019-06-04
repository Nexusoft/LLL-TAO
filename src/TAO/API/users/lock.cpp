/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/users.h>
#include <TAO/Ledger/types/tritium_minter.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Lock an account for mining (TODO: make this much more secure) */
        json::json Users::Lock(const json::json& params, bool fHelp)
        {
            /* Restrict Unlock / Lock to sessionless API */
            if(config::fMultiuser.load())
                throw APIException(-23, "Lock not supported for session-based API");

            /* Check if already unlocked. */
            if(!pActivePIN.IsNull() && pActivePIN->PIN() == "")
                throw APIException(-26, "Account already locked");


            /* Clear the pin */
            LOCK(MUTEX);

            pActivePIN.free();

            /* If stake minter is running, stop it */
            TAO::Ledger::TritiumMinter& stakeMinter = TAO::Ledger::TritiumMinter::GetInstance();

            if (stakeMinter.IsStarted())
                stakeMinter.StopStakeMinter();

            return true;
        }
    }
}
