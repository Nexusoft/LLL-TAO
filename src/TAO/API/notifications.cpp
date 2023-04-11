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
#include <TAO/API/include/check.h>
#include <TAO/API/include/list.h>

#include <TAO/API/types/accounts.h>
#include <TAO/API/types/authentication.h>
#include <TAO/API/types/exception.h>
#include <TAO/API/types/indexing.h>
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
            /* Check if we need to loop in suspended state. */
            if(config::fSuspended.load())
            {
                runtime::sleep(100);
                continue;
            }

            /* We want to sleep while looping to not consume our cpu cycles. */
            runtime::sleep(3000);

            /* Get a current list of our active sessions. */
            const auto vSessions =
                Authentication::Sessions(nThread, nThreads);

            /* Check if we are an active thread. */
            for(const auto& rSession : vSessions)
            {
                try
                {
                    /* Check for shutdown. */
                    if(config::fShutdown.load())
                        return;

                    /* Check for suspended state. */
                    if(config::fSuspended.load())
                        break;

                    /* Cache some local variables. */
                    const uint256_t& hashSession = rSession.first;
                    const uint256_t& hashGenesis = rSession.second;

                    /* Check that account is unlocked. */
                    if(!Authentication::Unlocked(TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS, hashSession))
                        continue;

                    /* Check if sigchain is mature. */
                    if(!CheckMature(hashGenesis))
                        continue;

                    /* Broadcast our unconfirmed transactions first. */
                    Indexing::BroadcastUnconfirmed(hashGenesis);

                    /* Build a json object. */
                    const encoding::json jSession =
                    {
                        { "session", hashSession.ToString() },
                    };

                    /* Get a list of our active events. */
                    std::vector<std::pair<uint512_t, uint32_t>> vEvents;

                    /* Get our list of active events we need to respond to. */
                    LLD::Logical->ListEvents(hashGenesis, vEvents);

                    /* Get our list of active contracts we have issued. */
                    LLD::Logical->ListContracts(hashGenesis, vEvents);

                    /* Track our unique events as we progress forward. */
                    std::set<std::pair<uint512_t, uint32_t>> setUnique;

                    /* Build our list of contracts. */
                    std::vector<TAO::Operation::Contract> vContracts;
                    for(const auto& rEvent : vEvents)
                    {
                        /* Check for shutdown. */
                        if(config::fShutdown.load())
                            break;

                        /* Check for unique events. */
                        if(setUnique.count(rEvent))
                            continue;

                        /* Add our event to our unique set. */
                        setUnique.insert(rEvent);

                        /* Grab a reference of our hash. */
                        const uint512_t& hashEvent = rEvent.first;

                        /* Check for Tritium transaction. */
                        if(hashEvent.GetType() == TAO::Ledger::TRITIUM)
                        {
                            /* Get the transaction from disk. */
                            TAO::API::Transaction tx;
                            if(!LLD::Ledger->ReadTx(hashEvent, tx))
                                continue;

                            /* Check if contract has been burned. */
                            if(tx.Burned(hashEvent, rEvent.second))
                                continue;

                            /* Check if the transaction is mature. */
                            if(!tx.Mature(hashEvent))
                                continue;
                        }

                        /* Get a referecne of our contract. */
                        const TAO::Operation::Contract& rContract =
                            LLD::Ledger->ReadContract(hashEvent, rEvent.second, TAO::Ledger::FLAGS::BLOCK);

                        /* Check if the given contract is spent already. */
                        if(rContract.Spent(rEvent.second))
                            continue;

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
                                    continue;
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
                                    continue;
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
                    }

                    /* Get the list of registers owned by this sig chain */
                    std::map<uint256_t, std::pair<Accounts, uint256_t>> mapAssets;
                    if(ListPartial(hashGenesis, mapAssets))
                    {
                        /* Build our list of accounts sorted by token. */
                        std::vector<TAO::Register::Address> vAddresses;
                        if(ListAccounts(hashGenesis, vAddresses, false, false))
                        {
                            /* Build a map of our accounts (this should be done when initializing with cache updates). */
                            std::map<uint256_t, TAO::Register::Address> mapAccounts;
                            for(const auto& rAddress : vAddresses)
                            {
                                /* Check for shutdown. */
                                if(config::fShutdown.load())
                                    break;

                                /* Read the register object. */
                                TAO::Register::Object oAccount;
                                if(!LLD::Register->ReadObject(rAddress, oAccount))
                                    continue;

                                /* Check for this token-id. */
                                if(mapAccounts.count(oAccount.get<uint256_t>("token")))
                                    continue;

                                /* Insert into map now. */
                                mapAccounts.insert(std::make_pair(oAccount.get<uint256_t>("token"), rAddress));
                            }

                            /* Add the register data to the response */
                            for(const auto& pairAsset : mapAssets)
                            {
                                /* Get our current token we are working on. */
                                const uint256_t& hashToken = pairAsset.first;

                                /* Read our token now. */
                                TAO::Register::Object oToken;
                                if(!LLD::Register->ReadObject(hashToken, oToken))
                                    continue;

                                /* Cache our asset's address as reference. */
                                const uint256_t& hashAsset =
                                    pairAsset.second.second;

                                /* Check for sigchain sequence for given token. */
                                uint32_t nSequence = 0;
                                //if(!LLD::Ledger->ReadSequence(hashToken, nSequence))
                                //    continue;

                                /* Cache our account so we can run through events. */
                                Accounts& rAccounts =
                                    const_cast<Accounts&>(pairAsset.second.first);

                                /* Check for sigchain events for given token. */
                                TAO::Ledger::Transaction tx;
                                while(LLD::Ledger->ReadEvent(hashToken, ++nSequence, tx))
                                {
                                    /* Check for shutdown. */
                                    if(config::fShutdown.load())
                                        break;

                                    /* Cache our txid for transaction. */
                                    const uint512_t hashTx = tx.GetHash();

                                    /* Iterate through our contracts. */
                                    for(uint32_t nContract = 0; nContract < tx.Size(); ++nContract)
                                    {
                                        /* Check for unique events. */
                                        if(setUnique.count(std::make_pair(hashTx, nContract)))
                                            continue;

                                        /* Get a reference of our internal contract. */
                                        const TAO::Operation::Contract& rContract = tx[nContract];

                                        /* Reset the contract to the position of the primitive. */
                                        rContract.SeekToPrimitive();

                                        /* The operation */
                                        uint8_t nOP;
                                        rContract >> nOP;

                                        /* Check for DEBIT. */
                                        if(nOP != TAO::Operation::OP::DEBIT)
                                            continue;

                                        /* Skip source address. */
                                        uint256_t hashSource;
                                        rContract >> hashSource;

                                        /* Check that we are using correct token. */
                                        TAO::Register::Object oSource;
                                        if(!LLD::Register->ReadObject(hashSource, oSource))
                                            continue;

                                        /* Make sure we have a deposit account. */
                                        if(!mapAccounts.count(oSource.get<uint256_t>("token")))
                                            continue;

                                        /* Extract destination. */
                                        uint256_t hashRecipient;
                                        rContract >> hashRecipient;

                                        /* Check for correct recipient. */
                                        if(hashRecipient != hashAsset)
                                            continue;

                                        /* Reset our accounts iterator. */
                                        rAccounts.Reset();

                                        /* Loop through all accounts and address the events. */
                                        while(!rAccounts.Empty())
                                        {
                                            /* Get our current address. */
                                            const TAO::Register::Address addrAccount =
                                                rAccounts.GetAddress();

                                            /* Skip over account if active proof. */
                                            if(!LLD::Ledger->HasProof(addrAccount, hashTx, nContract, TAO::Ledger::FLAGS::LOOKUP))
                                            {
                                                /* Build our credit now. */
                                                try
                                                {
                                                    /* Build some input parameters. */
                                                    encoding::json jBuild = jSession;
                                                    jBuild["proof"]   = addrAccount.ToString();
                                                    jBuild["address"] = mapAccounts[oSource.get<uint256_t>("token")].ToString();

                                                    /* Build our credit contract now. */
                                                    if(!BuildCredit(jBuild, nContract, rContract, vContracts))
                                                    {
                                                        /* Check if we have a next account. */
                                                        if(!rAccounts.HasNext())
                                                            break;

                                                        /* Iterate to our next account now. */
                                                        rAccounts++;

                                                        continue;
                                                    }
                                                }
                                                catch(const Exception& e)
                                                {
                                                    debug::warning(FUNCTION, "failed to build partial crecit for ", hashTx.SubString(), ": ", e.what());
                                                }
                                            }

                                            /* Check if we have a next account. */
                                            if(!rAccounts.HasNext())
                                                break;

                                            /* Iterate to our next account now. */
                                            rAccounts++;
                                        }

                                        /* Push contract pair to executed set. */
                                        setUnique.insert(std::make_pair(hashTx, nContract));
                                    }
                                }
                            }
                        }
                    }

                    /* Build our transaction if there are contracts. */
                    if(vContracts.empty())
                        continue;

                    /* Build a list of contracts for transaction. */
                    std::vector<TAO::Operation::Contract> vSanitized;

                    /* Sanitize our contract here to make sure we build a valid transaction. */
                    std::map<uint256_t, TAO::Register::State> mapStates;
                    for(auto& rContract : vContracts)
                    {
                        /* Check for shutdown. */
                        if(config::fShutdown.load())
                            break;

                        /* Bind our contract now to a timestamp and caller. */
                        rContract.Bind(runtime::unifiedtimestamp(), hashGenesis);

                        /* Sanitize the contract. */
                        if(SanitizeContract(rContract, mapStates))
                            vSanitized.emplace_back(std::move(rContract));
                    }

                    /* Check for available contracts. */
                    if(vSanitized.empty())
                        continue;

                    /* Now build our official transaction. */
                    const std::vector<uint512_t> vHashes =
                        BuildAndAccept(jSession, vSanitized, TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS);

                    debug::log(0, FUNCTION, "Built ", vHashes.size(), " transactions for ", vSanitized.size(), " contracts");
                }
                catch(const Exception& e) { debug::warning("EXCEPTION: ", FUNCTION, e.what()); }
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
    bool Notifications::SanitizeContract(TAO::Operation::Contract &rContract, std::map<uint256_t, TAO::Register::State> &mapStates)
    {
        LOCK(LLP::TritiumNode::CLIENT_MUTEX);

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
}
