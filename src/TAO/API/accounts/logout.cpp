/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <TAO/API/include/accounts.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/hex.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Login to a user account. */
        json::json Accounts::Logout(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Check for username parameter. */
            if(params.find("session") == params.end())
                throw APIException(-23, "Missing Session ID");

            /* Generate the signature chain. */
            uint64_t nSession = std::stoull(params["session"].get<std::string>());
            if(!mapSessions.count(nSession))
                throw APIException(-1, "Already logged out");

            /* Set the return value. */
            ret["genesis"] = GetGenesis(nSession).ToString();

            /* Delete the sigchan. */
            memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = mapSessions[nSession];
            user.free();

            /* Erase the session. */
            mapSessions.erase(nSession);

            return ret;
        }
    }
}
