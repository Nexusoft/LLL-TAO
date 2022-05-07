/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/exception.h>
#include <TAO/API/types/notifications.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Operation/include/enum.h>

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
        const int64_t nThreads =
            config::GetArg("-notificationsthreads", 1); //we use int64_t to be consistent with Authentication::Sessions

        /* Build our list of threads. */
        for(int64_t n = 0; n < nThreads; ++n)
            vThreads.push_back(std::thread(&Notifications::Manager, n));
    }


    /* Handle notification of all events for API. */
    void Notifications::Manager(const int64_t nThread)
    {
        /* Loop until shutdown. */
        while(!config::fShutdown.load())
        {
            /* We want to sleep while looping to not consume our cpu cycles. */
            runtime::sleep(100);

            /* Get a current list of our active sessions. */
            const auto vSessions =
                Authentication::Sessions(nThread, vThreads.size());

            /* Check if we are an active thread. */
            for(const auto& rSession : vSessions)
            {
                /* Cache some local variables. */
                const uint256_t& hashSession = rSession.first;
                const uint256_t& hashGenesis = rSession.second;

                /* Check that account is unlocked. */
                if(!Authentication::Unlocked(hashSession, TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS))
                    continue;

                /* Get a list of our active events. */
                std::vector<std::pair<uint512_t, uint32_t>> vEvents;
                if(LLD::Logical->ListEvents(hashGenesis, vEvents))
                {
                    /* Build our list of contracts. */
                    std::vector<TAO::Operation::Contract> vContracts;

                    /* List out our active events to process. */
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
                                /* Build a json object. */
                                encoding::json jParams =
                                {
                                    { "session", hashSession.ToString() },
                                };

                                /* If in mainnet mode, we want to check for events account. */
                                if(!config::fHybrid.load())
                                    jParams["address"] = config::GetArg("-eventsaccount", "default");

                                try
                                {
                                    /* Build our credit contract now. */
                                    if(!BuildCredit(jParams, rEvent.second, rContract, vContracts))
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
                                break;
                            }

                            /* Unknown ops we want to continue looping. */
                            default:
                            {
                                debug::warning(FUNCTION, "unknown OP: ", std::hex, uint32_t(nOP));
                                continue;
                            }
                        }
                    }

                    /* Build our transaction if there are contracts. */
                    if(!vContracts.empty())
                    {
                        /* Log that we have completed this step. */
                        debug::log(0, FUNCTION, "Building transaction for ", hashGenesis.SubString(), " with ", vContracts.size());

                        /* Build a json object. */
                        const encoding::json jParams =
                        {
                            { "session", hashSession.ToString() },
                        };

                        try
                        {
                            /* Now build our official transaction. */
                            const uint512_t hashTx =
                                BuildAndAccept(jParams, vContracts);

                            debug::log(0, FUNCTION, "Built transaction ", hashTx.ToString());
                        }
                        catch(const Exception& e)
                        {
                            debug::warning(FUNCTION, "Failed to build tx for ", hashGenesis.SubString(), ": ", e.what());
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
}
