/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/session-manager.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/include/extract.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>


/* Global TAO namespace. */
namespace TAO::API
{
    /* Saves the users session into the local DB so that it can be resumed later after a crash */
    encoding::json Users::Save(const encoding::json& jParams, const bool fHelp)
    {
        /* Pin parameter. */
        const SecureString strPIN =
            ExtractPIN(jParams);

        /* Get the session */
        Session& session =
            GetSession(jParams);

        /* Authenticate the users credentials */
        if(!Commands::Get<Users>()->Authenticate(jParams))
            throw Exception(-139, "Invalid credentials");

        /* Save the session */
        session.Save(strPIN);

        /* populate reponse */;
        const encoding::json jRet =
        {
            { "success", true}
        };

        return jRet;
    }
}
