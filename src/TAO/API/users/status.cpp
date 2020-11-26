/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/types/sessionmanager.h>

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

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params, true, false);

            /* The callers genesis */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* Flag indicating whether to include the username in the response. If this is in multiuser mode then we 
               will only return the username if they have provided a valid pin */
            bool fUsername = false;

            if(config::fMultiuser.load() && params.find("pin") != params.end())
            {
                /* Authenticate the users credentials */
                if(!users->Authenticate(params))
                    throw APIException(-139, "Invalid credentials");
                
                /* Pin is valid so include the username */
                fUsername = true;
            }
            else if(!config::fMultiuser.load())
            {
                /* Always return the username in single user mode */
                fUsername = true;
            }

            /* populate response */
            if(fUsername)
                ret["username"] = session.GetAccount()->UserName().c_str();
            
            /* Add the genesis */
            ret["genesis"] = hashGenesis.GetHex();

            /* Add the last active timestamp */
            ret["lastactive"] = session.GetLastActive();

            /* sig chain transaction count */
            uint32_t nTransactions = 0;

            /* flag indicating recovery has been set */
            bool fRecovery = false;
            
            /* Read the last transaction for the sig chain */
            uint512_t hashLast = 0;
            if(LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
            {
                /* Get the transaction from disk. */
                TAO::Ledger::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashLast, tx, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-108, "Failed to read transaction");

                /* Number of transactions is the last sequence number + 1 (since the sequence is 0 based) */
                nTransactions = tx.nSequence + 1;

                /* Set recovery flag if recovery hash has been set on the last transaction in the chain */
                fRecovery = tx.hashRecovery != 0;
            }

            /* populate recovery flag */
            ret["recovery"] = fRecovery;

            /* populate the transaction count */
            ret["transactions"] = nTransactions;

            /* Get the notifications so that we can return the notification count. */
            std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> vContracts;
            GetOutstanding(hashGenesis, false, vContracts);

            /* Get any expired contracts not yet voided. */
            GetExpired(hashGenesis, false, vContracts);

            /* Get any legacy transactions . */
            std::vector<std::pair<std::shared_ptr<Legacy::Transaction>, uint32_t>> vLegacyTx;
            GetOutstanding(hashGenesis, false, vLegacyTx);
            
            ret["notifications"] = vContracts.size() + vLegacyTx.size();


            /* populate unlocked status */
            json::json jsonUnlocked;

            jsonUnlocked["mining"] = !session.GetActivePIN().IsNull() && session.CanMine();
            jsonUnlocked["notifications"] = !session.GetActivePIN().IsNull() && session.CanProcessNotifications();
            jsonUnlocked["staking"] = !session.GetActivePIN().IsNull() && session.CanStake();
            jsonUnlocked["transactions"] = !session.GetActivePIN().IsNull() && session.CanTransact();

            ret["unlocked"] = jsonUnlocked;
            
            return ret;
        }
    }
}
