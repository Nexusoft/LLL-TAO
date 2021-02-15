/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/sessionmanager.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Saves the users session into the local DB so that it can be resumed later after a crash */
        json::json Users::Save(const json::json& params, bool fHelp)
        {
            /* JSON return value. */
            json::json ret;

            /* Pin parameter. */
            SecureString strPin;

            /* Get the session */
            Session& session = GetSession(params);

            /* Check for pin parameter. Parse the pin parameter. */
            if(params.find("pin") != params.end())
                strPin = SecureString(params["pin"].get<std::string>().c_str());
            else if(params.find("PIN") != params.end())
                strPin = SecureString(params["PIN"].get<std::string>().c_str());
            else
                throw APIException(-129, "Missing PIN");

            if(strPin.size() == 0)
                throw APIException(-135, "Zero-length PIN");

            /* Get the genesis ID. */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* Get the sig chain transaction to authenticate with, using the same hash that was used at login . */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->ReadTx(session.hashAuth, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-138, "No previous transaction found");

            /* Genesis Transaction. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(session.GetAccount()->Generate(txPrev.nSequence + 1, strPin), txPrev.nNextType);

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
                throw APIException(-149, "Invalid PIN");
            
            /* Save the session */
            session.Save(strPin);

            /* populate reponse */;
            ret["success"] = true;
            return ret;
        }
    }
}
