/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>
#include <LLD/include/ledger.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/jsonutils.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/names.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <Util/include/debug.h>


namespace TAO
{
    namespace API
    {

        // TODO:
        // get identifer from notification (you need to add identifier to notification JSON)
        // check event processor config to see if we have an account or register address configured for identifier X
        // if so then use that to process this debit...
        //2 = jack:savings OR 2=asdfasdfsdfsdf
        // if they have configured a register address then set hashTo from that address
        // if they have configured an account name then we need to generate the register address from that name
        //  - register address is namespacehash:token:name
        //  - namespacehash is argon2 hash of username
        //  - Look at AddressFromName() method as you can probably use that

        /*  Background thread to handle/suppress sigchain notifications. */
        void Users::EventsThread()
        {
            uint512_t hashTx;
            uint256_t hashFrom;
            uint256_t hashTo;
            uint256_t hashGenesis;
            uint64_t nSession = 0;
            uint64_t nAmount = 0;
            uint32_t nContract = 0;

            /* Loop the events processing thread until shutdown. */
            while(!fShutdown.load())
            {

                /* Wait for the events processing thread to be woken up (such as a login) */
                std::unique_lock<std::mutex> lk(EVENTS_MUTEX);
                CONDITION.wait_for(lk, std::chrono::milliseconds(1000), [this]{ return fEvent.load() || fShutdown.load();});

                /* Check for a shutdown event. */
                if(fShutdown.load())
                    return;

                /* Ensure that the user is logged, in, wallet unlocked, and able to transact. */
                if(!LoggedIn() || Locked() || !CanTransact())
                    continue;

                /* Get the session to be used for this API call */
                json::json params;
                nSession = users->GetSession(params);

                /* Get the account. */
                memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user = users->GetAccount(nSession);
                if(!user)
                    throw APIException(-25, "Invalid session ID");

                /* Set the hash genesis for this user. */
                hashGenesis = user->Genesis();

                /* Set the genesis id for the user. */
                params["genesis"] = hashGenesis.ToString();

                /* Get the PIN to be used for this API call */
                SecureString strPIN = users->GetPin(params);

                try
                {
                    json::json ret = Notifications(params, false);

                    /* Loop through each transaction in the notification queue. */
                    for(const auto& transaction : ret)
                    {
                        /* Look for the txid string. */
                        if(transaction.find("txid") == transaction.end())
                            throw APIException(-25, "Notification missing txid");

                        /* Set the transaction hash for this transaction. */
                        hashTx.SetHex(transaction["txid"]);

                        /* Get the number of confirmations for this transaction. */
                        uint32_t nConfirms = 0;
                        TAO::Ledger::BlockState state;
                        if(LLD::Ledger->ReadBlock(hashTx, state))
                            nConfirms = TAO::Ledger::ChainState::stateBest.load().nHeight - state.nHeight;

                        /* Create the transaction. */
                        TAO::Ledger::Transaction tx;
                        if(!TAO::Ledger::CreateTransaction(user, strPIN, tx))
                            throw APIException(-25, "Failed to create transaction");

                        /* Track the number of contracts built for this transaction object. */
                        uint32_t nID = 0;

                        /* Loop through each contract in the transaction. */
                        for(const auto& contract : transaction["contracts"])
                        {
                            /* Look for the OP string. */
                            if(contract.find("OP") == contract.end())
                                throw APIException(-25, "Contract missing opcode.");

                            /* Get the opcode. */
                            std::string strOP = contract["OP"];

                            /* Check for output field. */
                            if(contract.find("output") == contract.end())
                                throw APIException(-25, "Contract missing output (contract id)");

                            /* Set the contract ID. */
                            nContract = contract["output"];

                            /* Check for Debits. */
                            if(strOP == "DEBIT")
                            {
                                /* Check for from field. */
                                if(contract.find("from") == contract.end())
                                    throw APIException(-25, "Debit contract missing from");

                                /* Check for to field. */
                                if(contract.find("to") == contract.end())
                                    throw APIException(-25, "Debit contract missing to");

                                /* Check for amount field. */
                                if(contract.find("amount") == contract.end())
                                    throw APIException(-25, "Debit contract missing amount");

                                /* Set to and from hashes and amount. */
                                hashFrom.SetHex(contract["from"]);
                                hashTo.SetHex(contract["to"]);
                                nAmount = contract["amount"];

                                /* Submit the payload object. */
                                tx[nID] << uint8_t(TAO::Operation::OP::CREDIT);
                                tx[nID] << hashTx << nContract;
                                tx[nID] << hashTo << hashFrom;
                                tx[nID] << nAmount;

                                TAO::Register::Object object;
                                if(!LLD::Register->ReadState(hashTo, object))
                                    throw APIException(-25, "Couldn't read the state object");

                                /* Increment the contract ID. */
                                ++nID;
                            }

                            /* Check for Coinbases. */
                            else if(strOP == "COINBASE")
                            {
                                /* Set the genesis hash and the amount. */
                                hashFrom.SetHex(contract["genesis"]);

                                /* Check that the coinbase was mined by the current active user. */
                                if(hashFrom != hashGenesis)
                                    throw APIException(-25, "Coinbase transaction mined by different user.");


                                /* Check that the coinbase transaction is ready to be credited. */
                                if(nConfirms < (config::fTestNet ? TAO::Ledger::TESTNET_MATURITY_BLOCKS
                                                                 : TAO::Ledger::NEXUS_MATURITY_BLOCKS))
                                {
                                    //debug::log(0, "Coinbase is immature ", nConfirms, " confirmations.");
                                    continue;
                                }

                                /* Get the amount from the coinbase transaction. */
                                nAmount = contract["amount"];

                                TAO::Register::Object account;
                                if(!TAO::Register::GetNameRegister(hashGenesis, std::string("default"), account))
                                    throw APIException(-25, "Could not retrieve default NXS account.");

                                /* Get the address that this name register is pointing to. */
                                hashTo = account.get<uint256_t>("address");

                                /* Submit the payload object. */
                                tx[nID] << uint8_t(TAO::Operation::OP::CREDIT);
                                tx[nID] << hashTx << nContract;
                                tx[nID] << hashTo << hashFrom;
                                tx[nID] << nAmount;

                                /* Increment the contract ID. */
                                ++nID;
                            }

                            /* Check for Transfers. */
                            else if(strOP == "TRANSFER")
                            {
                                /* Submit the payload object. */
                                tx[nID] << uint8_t(TAO::Operation::OP::CLAIM);
                                tx[nID] << hashTx;

                                /* Increment the contract ID. */
                                ++nID;
                            }
                        }

                        /* If any of the notifications have been matched, execute the operations layer and sign the transaction. */
                        if(nID)
                        {
                            /* Execute the operations layer. */
                            if(!tx.Build())
                                throw APIException(-26, "Operations failed to execute");

                            /* Sign the transaction. */
                            if(!tx.Sign(users->GetKey(tx.nSequence, strPIN, users->GetSession(params))))
                                throw APIException(-26, "Ledger failed to sign transaction");

                            /* Execute the operations layer. */
                            if(!TAO::Ledger::mempool.Accept(tx))
                                throw APIException(-26, "Failed to accept");

                            /* Print the JSON value for the notifications processed. */
                            debug::log(0, FUNCTION, "\n", transaction.dump(4));
                        }

                    }
                }
                catch(const APIException& e)
                {
                    debug::error(FUNCTION, e.what());
                }

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
