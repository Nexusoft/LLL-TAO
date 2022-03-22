/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2022

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <LLD/include/global.h>


#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/get.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/constants.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/session-manager.h>
#include <TAO/API/types/commands.h>
#include <TAO/API/types/session.h>

#include <TAO/API/types/commands/system.h>


/* Global TAO namespace. */
namespace TAO::API
{
    /* Verify if passed pin is correct */
    encoding::json System::VerifyPin(const encoding::json& jParams, const bool fHelp)
    {
         /* Get the session */
        Session& session =
            Commands::Get<Users>()->GetSession(jParams);

        /*Build JSON Obj */
        encoding::json jRet;

        /* Get the PIN to be used for this API call */
        SecureString strPin = ExtractPIN(jParams);

        /* Check for pin size. */
        if(strPin.size() == 0)
            throw Exception(-135, "Zero-length PIN");     

        /* Get active pin from session */
        SecureString activePin = session.GetActivePIN()->PIN();

        /* Set our response values. */
        jRet["valid"] = strPin == activePin ? true : false ;

        /*return JSON */
        return jRet;
    }
}
