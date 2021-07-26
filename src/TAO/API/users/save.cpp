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

#include <TAO/API/include/extract.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Saves the users session into the local DB so that it can be resumed later after a crash */
        encoding::json Users::Save(const encoding::json& jParams, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json ret;

            /* Pin parameter. */
            const SecureString strPin = ExtractPIN(jParams);

            /* Get the session */
            Session& session = GetSession(jParams);

            /* Get the genesis ID. */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* Get the sig chain transaction to authenticate with, using the same hash that was used at login . */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->ReadTx(session.hashAuth, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-138, "No previous transaction found");

            /* Genesis Transaction. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(session.GetAccount()->Generate(txPrev.nSequence + 1, strPin), txPrev.nNextType);

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
                throw Exception(-149, "Invalid PIN");

            /* Save the session */
            session.Save(strPin);

            /* populate reponse */;
            ret["success"] = true;
            return ret;
        }
    }
}
