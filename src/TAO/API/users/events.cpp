/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/users.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/args.h>
#include <Util/include/debug.h>


namespace TAO
{
    namespace API
    {

        /*  Background thread to handle/suppress sigchain notifications. */
        void Users::EventsThread()
        {
            while(!config::fShutdown.load())
            {
                //uint32_t nSequence = 0;

                //TODO: keep track of


                /* Wait for the events processing thread to be woken up (such as a login) */
                std::unique_lock<std::mutex> lk(EVENTS_MUTEX);
                CONDITION.wait_for(lk, std::chrono::milliseconds(1000), [this]{ return fEvent.load() || config::fShutdown.load();});

                if(config::fShutdown.load())
                    return;

                if(!LoggedIn() || Locked() || !CanTransact())
                    continue;

                /* Get the session to be used for this API call */
                json::json params;
                uint64_t nSession = users.GetSession(params);

                /* Get the account. */
                memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users.GetAccount(nSession);
                if(!user)
                    throw APIException(-25, "Invalid session ID");


                params["genesis"] = user->Genesis().ToString();

                /* Get the PIN to be used for this API call */
                SecureString strPIN = users.GetPin(params);

                try
                {
                    json::json ret = Notifications(params, false);

                    for(const auto& notification : ret)
                    {
                        if(notification["operation"]["OP"] == "DEBIT")
                        {
                            /* Create the transaction. */
                            TAO::Ledger::Transaction tx;
                            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                                throw APIException(-25, "Failed to create transaction");

                            /* Check for data parameter. */
                            uint256_t hashProof;
                            hashProof.SetHex(notification["operation"]["address"]);

                            /* Get the credit. */
                            uint64_t nAmount = notification["operation"]["amount"];

                            uint512_t hashTx;
                            hashTx.SetHex(notification["hash"]);

                            uint256_t hashTo;
                            hashTo.SetHex(notification["operation"]["transfer"]);

                            /* Submit the payload object. */
                            tx << uint8_t(TAO::Operation::OP::CREDIT) << hashTx << hashProof << hashTo << nAmount;

                            /* Execute the operations layer. */
                            if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
                                throw APIException(-26, "Operations failed to execute");

                            /* Sign the transaction. */
                            if(!tx.Sign(users.GetKey(tx.nSequence, strPIN, users.GetSession(params))))
                                throw APIException(-26, "Ledger failed to sign transaction");

                            /* Execute the operations layer. */
                            if(!TAO::Ledger::mempool.Accept(tx))
                                throw APIException(-26, "Failed to accept");
                        }
                        else if(notification["operation"]["OP"] == "TRANSFER")
                        {
                            /* Create the transaction. */
                            TAO::Ledger::Transaction tx;
                            if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                                throw APIException(-25, "Failed to create transaction");

                            uint512_t hashTx;
                            hashTx.SetHex(notification["hash"]);

                            /* Submit the payload object. */
                            tx << uint8_t(TAO::Operation::OP::CLAIM) << hashTx;

                            /* Execute the operations layer. */
                            if(!TAO::Operation::Execute(tx, TAO::Register::FLAGS::PRESTATE | TAO::Register::FLAGS::POSTSTATE))
                                throw APIException(-26, "Operations failed to execute");

                            /* Sign the transaction. */
                            if(!tx.Sign(users.GetKey(tx.nSequence, strPIN, users.GetSession(params))))
                                throw APIException(-26, "Ledger failed to sign transaction");

                            /* Execute the operations layer. */
                            if(!TAO::Ledger::mempool.Accept(tx))
                                throw APIException(-26, "Failed to accept");
                        }
                    }

                    debug::log(0, ret.dump(4));


                }
                catch(const APIException& e)
                {
                }



                /* Scan sigchain by hashLast to hashPrev == 0 */


                /* Unpack all registers */


                /* Write in localDB with a genesis index (sequence, genesis) */


                /* Use the local DB cache for scanning events to any owned register address */


                /* Respond to all TRANSFER notifications with CLAIM. */
                //debug::log(0, FUNCTION, "Respond to TRANSFER notifications with CLAIM");
                //debug::log(0, FUNCTION, "Respond to DEBIT notifications with CREDIT");


                /* Respond to all DEBIT notifications with CREDIT. */


                /* Reset the events flag. */
                fEvent = false;
            }
        }


        /*  Notifies the events processor that an event has occurred so it
         *  can check and update it's state. */
        void Users::NotifyEvent()
        {
            fEvent = true;
            CONDITION.notify_one();
        }
    }
}
