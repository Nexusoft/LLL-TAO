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
                    {
                        //XXX: check here if we have orphaned any of our own transactions mining
                        continue;
                    }

                    /* Build a json object. */
                    const encoding::json jSession =
                    {
                        { "session", hashSession.ToString() },
                    };

                    /* Check if we need to cleanup any unconfirmed transaction chains. */
                    if(!SanitizeUnconfirmed(hashGenesis, jSession))
                        continue;

                    /* Broadcast our unconfirmed transactions first. */
                    Indexing::BroadcastUnconfirmed(hashGenesis);

                    /* Build our list of contracts. */
                    std::vector<TAO::Operation::Contract> vContracts;

                    /* Track our unique events as we progress forward. */
                    std::set<std::pair<uint512_t, uint32_t>> setUnique;

                    /* Get a list of our active events. */
                    std::vector<std::pair<uint512_t, uint32_t>> vNotifications;
                    LLD::Logical->ListEvents(hashGenesis, vNotifications, 500); //maximum of 500 per iteration

                    /* Loop through our active notifications. */
                    bool fEventStop = false;
                    for(const auto& rEvent : vNotifications)
                    {
                        /* Check for unique events. */
                        if(setUnique.count(std::make_pair(rEvent.first, rEvent.second)))
                            continue;

                        /* Build our contracts now. */
                        if(build_notification(hashGenesis, jSession, rEvent, false, fEventStop, vContracts))
                        {
                            setUnique.insert(std::make_pair(rEvent.first, rEvent.second));
                            fEventStop = true;
                        }
                    }

                    /* Get a list of our active events. */
                    std::vector<std::pair<uint512_t, uint32_t>> vContractSent;
                    LLD::Logical->ListContracts(hashGenesis, vContractSent, 100); //maximum of 100 per iteration

                    /* Loop through our sent contracts. */
                    bool fContractStop = false;
                    for(const auto& rEvent : vContractSent)
                    {
                        /* Check for unique events. */
                        if(setUnique.count(std::make_pair(rEvent.first, rEvent.second)))
                            continue;

                        /* Build our contracts now. */
                        if(build_notification(hashGenesis, jSession, rEvent, true, fContractStop, vContracts))
                        {
                            setUnique.insert(std::make_pair(rEvent.first, rEvent.second));
                            fContractStop = true;
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
                                                    jBuild["proof"]       = addrAccount.ToString();
                                                    jBuild["address"]     = mapAccounts[oSource.get<uint256_t>("token")].ToString();

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

                    /* Build a list of sanitized contracts now. */
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

                    /* Build once we reach threshold. */
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


    /* Checks that the contract passes both Build() and Execute() */
    bool Notifications::SanitizeContract(const uint256_t& hashGenesis, TAO::Operation::Contract &rContract)
    {
        /* Bind our contract now to a timestamp and caller. */
        rContract.Bind(runtime::unifiedtimestamp(), hashGenesis);

        /* We are just wrapping around other overload here. */
        std::map<uint256_t, TAO::Register::State> mapStates;
        return SanitizeContract(rContract, mapStates);
    }


    /* Checks that the current unconfirmed transactions are in a valid state. */
    bool Notifications::SanitizeUnconfirmed(const uint256_t& hashGenesis, const encoding::json& jSession)
    {
        /* Build list of transaction hashes. */
        std::vector<uint512_t> vHashes;

        /* Read all transactions from our last index. */
        uint512_t hash;
        if(!LLD::Logical->ReadLast(hashGenesis, hash))
            return true; //we return true here so we don't stop notifications from processing

        /* Track our total failed contracts for debugging purposes. */
        uint32_t nFailedContracts = 0;

        /* Loop until we reach confirmed transaction. */
        while(!config::fShutdown.load())
        {
            /* Read the transaction from the ledger database. */
            TAO::API::Transaction tx;
            if(!LLD::Logical->ReadTx(hash, tx))
            {
                debug::warning(FUNCTION, "read for ", hashGenesis.SubString(), " failed at tx ", hash.SubString());
                break;
            }

            /* Check we have index to break. */
            if(LLD::Ledger->HasIndex(hash))
                break;

            /* Push transaction to list. */
            vHashes.push_back(hash); //this will warm up the LLD cache if available, or remain low footprint if not

            /* Check for first. */
            if(tx.IsFirst())
                break;

            /* Set hash to previous hash. */
            hash = tx.hashPrevTx;
        }

        /* Track the root transaction that has an invalid contract in mempool. */
        uint512_t hashRoot;

        /* We want to track our list of states to sanitize. */
        std::map<uint256_t, TAO::Register::State> mapStates;

        /* Reverse iterate our list of entries. */
        std::vector<TAO::Operation::Contract> vSanitized;
        for(auto hash = vHashes.rbegin(); hash != vHashes.rend(); ++hash)
        {
            /* Read the transaction from the ledger database. */
            TAO::API::Transaction tx;
            if(!LLD::Logical->ReadTx(*hash, tx))
            {
                debug::warning(FUNCTION, "read for ", hashGenesis.SubString(), " failed at tx ", hash->SubString());
                break;
            }

            /* Iterate through our contracts. */
            for(const auto& rContract : tx.Contracts())
            {
                /* Make a copy of contract here since we will need a fresh copy to rebuild if failed. */
                TAO::Operation::Contract tContract = rContract;

                /* Sanitize the contract. */
                if(SanitizeContract(tContract, mapStates))
                    vSanitized.emplace_back(std::move(tContract));
                else
                {
                    /* Set our root as the first occurance since the rest of the chain will then be invalid. */
                    if(hashRoot == 0)
                        hashRoot = *hash;

                    /* Increment our failed counter. */
                    ++nFailedContracts;

                    debug::warning(FUNCTION, "failed to sanitize contract at tx ", hashRoot.SubString());
                }
            }
        }

        /* Check if we need to rebuild our sigchain. */
        if(hashRoot == 0)
            return true;

        /* If we reached here, we need to rebuild our sigchain indexes and transactions. */
        debug::warning(FUNCTION, "sigchain contains ", nFailedContracts, " invalid contracts, rebuilding ", vSanitized.size(), " contracts");

        /* Now we want to disconnect our transactions up to their root. */
        for(const auto& rHash : vHashes)
        {
            /* Read the transaction from the ledger database. */
            TAO::API::Transaction tx;
            if(!LLD::Logical->ReadTx(rHash, tx))
            {
                debug::warning(FUNCTION, "read for ", hashGenesis.SubString(), " failed at tx ", rHash.SubString());
                break;
            }

            /* Reset memory states to disk indexes. */
            if(!tx.Disconnect(TAO::Ledger::FLAGS::ERASE))
                debug::warning(FUNCTION, "failed to disconnect tx ", rHash.SubString());

            /* Delete our transaction from logical database. */
            if(!tx.Delete(rHash))
                debug::warning(FUNCTION, "failed to delete tx ", rHash.SubString());

            /* Check if we are at our root now. */
            if(rHash == hashRoot)
                break;
        }

        /* Now build our official transaction. */
        const std::vector<uint512_t> vRebuilt =
            BuildAndAccept(jSession, vSanitized, TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS);

        debug::log(0, FUNCTION, "Rebuilt ", vRebuilt.size(), " transactions for ", vSanitized.size(), " contracts");

        return false;
    }


    /* Build an contract response to given notificaion. */
    bool Notifications::build_notification(const uint256_t& hashGenesis, const encoding::json& jSession, const std::pair<uint512_t, uint32_t>& rEvent,
        const bool fMine, const bool fStop, std::vector<TAO::Operation::Contract> &vContracts)
    {
        /* Grab a reference of our hash. */
        const uint512_t& hashEvent = rEvent.first;

        /* Check for Tritium transaction. */
        if(hashEvent.GetType() == TAO::Ledger::TRITIUM)
        {
            /* Get the transaction from disk. */
            TAO::API::Transaction tx;
            if(!LLD::Ledger->ReadTx(hashEvent, tx))
                return true;

            /* Check if contract has been burned. */
            if(tx.Burned(hashEvent, rEvent.second))
            {
                /* Check for our stop. */
                if(fStop)
                    return true;

                /* For a burn we increment so we don't process same event again. */
                if(fMine)
                {
                    /* Increment our contract sequence. */
                    LLD::Logical->IncrementContractSequence(hashGenesis);
                    return false;
                }

                /* Increment our notifications sequence. */
                LLD::Logical->IncrementEventSequence(hashGenesis);
                return false;
            }

            /* Check if the transaction is mature. */
            if(!tx.Mature(hashEvent))
                return true;
        }

        /* Get a referecne of our contract. */
        const TAO::Operation::Contract& rContract =
            LLD::Ledger->ReadContract(hashEvent, rEvent.second, TAO::Ledger::FLAGS::BLOCK);

        /* Check if the given contract is spent already. */
        if(rContract.Spent(rEvent.second))
        {
            /* Check for our stop. */
            if(fStop)
                return true;

            /* For a burn we increment so we don't process same event again. */
            if(fMine)
            {
                /* Increment our contract sequence. */
                LLD::Logical->IncrementContractSequence(hashGenesis);
                return false;
            }

            /* Increment our notifications sequence. */
            LLD::Logical->IncrementEventSequence(hashGenesis);
            return false;
        }

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
                        return true;

                    /* Sanitize our contract now. */
                    TAO::Operation::Contract tContract = vContracts.back();
                    if(!SanitizeContract(hashGenesis, tContract))
                    {
                        /* Check for our stop. */
                        if(fStop || fMine)
                            return true;

                        /* Log some info about this. */
                        debug::log(2, FUNCTION, "OP::CREDIT: sanitize failed for ", rEvent.first.SubString(), ", adding to work queue");

                        /* Push this to our contracts queue so we can process again later. */
                        LLD::Logical->PushContract(hashGenesis, rEvent.first, rEvent.second);

                        /* Increment our notifications sequence. */
                        LLD::Logical->IncrementEventSequence(hashGenesis);
                        return false;
                    }
                }
                catch(const Exception& e)
                {
                    debug::warning(FUNCTION, "OP::CREDIT: failed to build for ", hashEvent.SubString(), ": ", e.what());
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
                        return true;

                    /* Sanitize our contract now. */
                    TAO::Operation::Contract tContract = vContracts.back();
                    if(!SanitizeContract(hashGenesis, tContract))
                    {
                        /* Check for our stop. */
                        if(fStop || fMine)
                            return true;

                        /* Log some info about this. */
                        debug::log(2, FUNCTION, "OP::CLAIM: sanitize failed for ", rEvent.first.SubString(), ", adding to work queue");

                        /* Push this to our contracts queue so we can process again later. */
                        LLD::Logical->PushContract(hashGenesis, rEvent.first, rEvent.second);

                        /* Increment our notifications sequence. */
                        LLD::Logical->IncrementEventSequence(hashGenesis);
                        return false;
                    }
                }
                catch(const Exception& e)
                {
                    debug::warning(FUNCTION, "OP::CLAIM: failed to build for ", hashEvent.SubString(), ": ", e.what());
                }

                break;
            }
        }

        return true;
    }
}
