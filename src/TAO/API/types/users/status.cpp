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

            uint256_t hashGenesis = user->Genesis();
            /* populate response */
            ret["username"] = user->UserName().c_str();
            ret["genesis"] = hashGenesis.GetHex();

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

            /* populate fecovery flag */
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

            jsonUnlocked["mining"] = !pActivePIN.IsNull() && pActivePIN->CanMine();
            jsonUnlocked["notifications"] = !pActivePIN.IsNull() && pActivePIN->ProcessNotifications();
            jsonUnlocked["staking"] = !pActivePIN.IsNull() && pActivePIN->CanStake();
            jsonUnlocked["transactions"] = !pActivePIN.IsNull() && pActivePIN->CanTransact();

            ret["unlocked"] = jsonUnlocked;
            
            return ret;
        }
    }
}
