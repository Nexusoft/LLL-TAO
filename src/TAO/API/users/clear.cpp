/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/sessionmanager.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Lists the currently processed notifications. */
    encoding::json Users::Clear(const encoding::json& params, const bool fHelp)
    {
        /* JSON return value. */
        encoding::json jRet;

        /* Get the Genesis ID. */
        const uint256_t hashGenesis = Commands::Get<Users>()->GetSession(params).GetAccount()->Genesis();

        /* Load the session */
        Session& session = Commands::Get<Users>()->GetSession(params);

        /* Check that it was loaded correctly */
        if(session.IsNull())
            throw Exception(-309, "Error loading session.");

        /* Return the total processed in results. */
        jRet["total"] = session.vProcessed->size();

        /* Wipe clean our procssed queue. */
        session.vProcessed->clear(); //XXX: we want to ensure this doesn't grow without bounds

        return jRet;
    }
}
