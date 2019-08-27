/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/users.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get status information for the currently logged in user. */
        json::json Users::Status(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;
            
            /* Restrict Unlock to sessionless API */
            if(config::fMultiuser.load())
                throw APIException(-145, "Unlock not supported in multiuser mode");

            /* Check default session (unlock only supported in single user mode). */
            if(!mapSessions.count(0))
                throw APIException(-11, "User not logged in.");

            /* Get the sigchain from map of users. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = mapSessions[0];

            /* populate response */
            ret["username"] = user->UserName().c_str();

            /* populate unlocked status */
            json::json jsonUnlocked;

            jsonUnlocked["minting"] = !pActivePIN.IsNull() && pActivePIN->CanMint();
            jsonUnlocked["transactions"] = !pActivePIN.IsNull() && pActivePIN->CanTransact();

            ret["unlocked"] = jsonUnlocked;
            
            return ret;
        }
    }
}
