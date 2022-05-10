/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>
#include <LLP/include/global.h>

#include <TAO/API/include/build.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/exception.h>
#include <TAO/API/types/notifications.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/build.h>

#include <Util/include/args.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Track the list of threads for processing. */
    std::vector<std::thread> Notifications::vThreads;


    /* Initializes the current notification systems. */
    void Notifications::Initialize()
    {
        /* Get the total manager threads. */
        const uint64_t nThreads =
            config::GetArg("-notificationsthreads", 1);

        /* Build our list of threads. */
        for(int64_t nThread = 0; nThread < nThreads; ++nThread)
            vThreads.push_back(std::thread(&Notifications::Manager, nThread, nThreads));
    }


    /* Handle notification of all events for API. */
    void Notifications::Manager(const int64_t nThread, const uint64_t nThreads)
    {
        /* Loop until shutdown. */
        while(!config::fShutdown.load())
        {
            /* We want to sleep while looping to not consume our cpu cycles. */
            runtime::sleep(500);

            /* Get a current list of our active sessions. */
            const auto vSessions =
                Authentication::Sessions(nThread, nThreads);

            /* Check if we are an active thread. */
            for(const auto& rSession : vSessions)
            {
                /* Cache some local variables. */
                const uint256_t& hashSession = rSession.first;
                const uint256_t& hashGenesis = rSession.second;

                /* Check that account is unlocked. */
                if(!Authentication::Unlocked(hashSession, TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS))
                    continue;

                /* Build a json object. */
                const encoding::json jSession =
                {
                    { "session", hashSession.ToString() },
                };

                /* Get a list of our active events. */
                std::vector<std::pair<uint512_t, uint32_t>> vEvents;

                /* Get our list of active contracts we have issued. */
                LLD::Logical->ListContracts(hashGenesis, vEvents);

                /* Get our list of active events we need to respond to. */
                LLD::Logical->ListEvents(hashGenesis, vEvents);

                //we need to list our active legacy transaction events

                /* Process contracts if we found from disk. */
                if(vEvents.empty())
                    continue;

                /* Build our list of contracts. */
                std::vector<TAO::Operation::Contract> vContracts;
                for(const auto& rEvent : vEvents)
                {
                    /* Grab a reference of our hash. */
                    const uint512_t& hashEvent = rEvent.first;

                    /* Get the transaction from disk. */
                    TAO::API::Transaction tx;
                    if(!LLD::Ledger->ReadTx(hashEvent, tx))
                        throw Exception(-108, "Failed to read transaction");

                    /* Check if contract has been spent. */
                    if(tx.Spent(hashEvent, rEvent.second))
                        continue;

                    /* Get a referecne of our contract. */
                    const TAO::Operation::Contract& rContract = tx[rEvent.second];

                    /* Seek our contract to primitive OP. */
                    rContract.SeekToPrimitive();

                    /* Get a copy of our primitive. */
                    uint8_t nOP = 0;
                    rContract >> nOP;

                    /* Switch for valid primitives. */
                    switch(nOP)
                    {
                        /* Handle for if we need to credit. */
                        case TAO::Operation::OP::LEGACY:
                        case TAO::Operation::OP::DEBIT:
                        case TAO::Operation::OP::COINBASE:
                        {
                            try
                            {
                                /* Build our credit contract now. */
                                if(!BuildCredit(jSession, rEvent.second, rContract, vContracts))
                                    continue;
                            }
                            catch(const Exception& e)
                            {
                                debug::warning(FUNCTION, "failed to build crecit for ", hashEvent.SubString(), ": ", e.what());
                            }

                            break;
                        }

                        /* Handle for if we need to claim. */
                        case TAO::Operation::OP::TRANSFER:
                        {
                            try
                            {
                                /* Build our credit contract now. */
                                if(!BuildClaim(jSession, rEvent.second, rContract, vContracts))
                                    continue;
                            }
                            catch(const Exception& e)
                            {
                                debug::warning(FUNCTION, "failed to build claim for ", hashEvent.SubString(), ": ", e.what());
                            }

                            break;
                        }

                        /* Unknown ops we want to continue looping. */
                        default:
                        {
                            debug::warning(FUNCTION, "unknown OP: ", std::hex, uint32_t(nOP));
                            continue;
                        }
                    }

                    /* Build our transaction if there are contracts. */
                    while(!vContracts.empty())
                    {
                        /* Track our current index. */
                        uint64_t nIndex = 0;

                        /* Build a list of contracts for transaction. */
                        std::vector<TAO::Operation::Contract> vSanitized;

                        /* Sanitize our contract here to make sure we build a valid transaction. */
                        std::map<uint256_t, TAO::Register::State> mapStates;
                        for( ; vSanitized.size() < 100 && nIndex < vContracts.size(); ++nIndex)
                        {
                            /* Grab a reference of contract. */
                            TAO::Operation::Contract& rContract = vContracts[nIndex];

                            /* Bind our contract now to a timestamp and caller. */
                            rContract.Bind(runtime::unifiedtimestamp(), hashGenesis);

                            /* Sanitize the contract. */
                            if(sanitize_contract(rContract, mapStates))
                                vSanitized.emplace_back(std::move(rContract));
                        }

                        /* Erase all the contracts that were iterated. */
                        vContracts.erase(vContracts.begin(), vContracts.begin() + nIndex);

                        /* Check for available contracts. */
                        if(vSanitized.empty())
                            continue;

                        try
                        {
                            /* Now build our official transaction. */
                            const uint512_t hashTx =
                                BuildAndAccept(jSession, vSanitized, TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS);

                            debug::log(0, FUNCTION, "Built ", hashTx.SubString(), " with ", vSanitized.size(), " contracts");
                        }
                        catch(const Exception& e)
                        {
                            debug::warning(FUNCTION, "Failed to build ", hashGenesis.SubString(), ": ", e.what());
                        }
                    }
                }
            }
        }
    }


    /* Shuts down the current notification systems. */
    void Notifications::Shutdown()
    {
        /* Loop and detach all threads. */
        for(auto& tThread : vThreads)
            tThread.join();
    }


    /* Checks that the contract passes both Build() and Execute() */
    bool Notifications::sanitize_contract(TAO::Operation::Contract &rContract, std::map<uint256_t, TAO::Register::State> &mapStates)
    {
        /* Return flag */
        bool fSanitized = false;

        /* Start a ACID transaction (to be disposed). */
        LLD::TxnBegin(TAO::Ledger::FLAGS::MINER);

        /* Temporarily disable error logging so that we don't log errors for contracts that fail to execute. */
        debug::fLogError = false;

        try
        {
            /* Sanitize contract by building and executing it. */
            fSanitized =
                TAO::Register::Build(rContract, mapStates, TAO::Ledger::FLAGS::MINER) && TAO::Operation::Execute(rContract, TAO::Ledger::FLAGS::MINER);

            /* Reenable error logging. */
            debug::fLogError = true;
        }
        catch(const std::exception& e)
        {
            /* Just in case we encountered an exception whilst error logging was off, reenable error logging. */
            debug::fLogError = true;

            /* Log the error and attempt to continue processing */
            debug::error(FUNCTION, e.what());
        }

        /* Abort the mempool ACID transaction once the contract is sanitized */
        LLD::TxnAbort(TAO::Ledger::FLAGS::MINER);

        return fSanitized;
    }


    /* Validate a transaction by sending it off to a peer, For use in -client mode. */
    bool Notifications::validate_transaction(const TAO::Ledger::Transaction& tx, uint32_t& nContract)
    {
        bool fValid = false;

        /* Check tritium server enabled. */
        if(LLP::TRITIUM_SERVER)
        {
            std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
            if(pNode != nullptr)
            {
                debug::log(1, FUNCTION, "CLIENT MODE: Validating transaction");

                /* Create our trigger nonce. */
                uint64_t nNonce = LLC::GetRand();
                pNode->PushMessage(LLP::TritiumNode::TYPES::TRIGGER, nNonce);

                /* Request the transaction validation */
                pNode->PushMessage(LLP::TritiumNode::ACTION::VALIDATE, uint8_t(LLP::TritiumNode::TYPES::TRANSACTION), tx);

                /* Create the condition variable trigger. */
                LLP::Trigger REQUEST_TRIGGER;
                pNode->AddTrigger(LLP::TritiumNode::RESPONSE::VALIDATED, &REQUEST_TRIGGER);

                /* Process the event. */
                REQUEST_TRIGGER.wait_for_nonce(nNonce, 10000);

                /* Cleanup our event trigger. */
                pNode->Release(LLP::TritiumNode::RESPONSE::VALIDATED);

                debug::log(1, FUNCTION, "CLIENT MODE: RESPONSE::VALIDATED received");

                /* Check the response args to see if it was valid */
                if(REQUEST_TRIGGER.HasArgs())
                {
                    REQUEST_TRIGGER >> fValid;

                    /* If it was not valid then deserialize the failing contract ID from the response */
                    if(!fValid)
                    {
                        /* Deserialize the failing hash (which should be the one we sent) */
                        uint512_t hashTx;
                        REQUEST_TRIGGER >> hashTx;

                        /* Deserialize the failing contract ID */
                        REQUEST_TRIGGER >> nContract;

                        /* Check the hash is valid */
                        if(hashTx != tx.GetHash())
                            throw Exception(0, "Invalid transaction ID received from RESPONSE::VALIDATED");

                        /* Check the contract ID is valid */
                        if(nContract > tx.Size() -1)
                            throw Exception(0, "Invalid contract ID received from RESPONSE::VALIDATED");
                    }
                }
                else
                {
                    throw Exception(0, "CLIENT MODE: timeout waiting for RESPONSE::VALIDATED");
                }

            }
            else
                debug::error(FUNCTION, "no connections available...");
        }

        /* return the valid flag */
        return fValid;
    }
}
