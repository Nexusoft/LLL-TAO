/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/types/users.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/include/unpack.h>
#include <TAO/Register/include/build.h>
#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/enum.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/process.h>
#include <TAO/Ledger/include/timelocks.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/stake_minter.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/transaction.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/trust.h>
#include <Legacy/types/transaction.h>
#include <Legacy/types/trustkey.h>

#include <Util/include/debug.h>

#include <vector>


namespace TAO
{
    namespace API
    {
        /*  Background thread to initiate user events . */
        void Users::EventsThread()
        {
            /* Loop the events processing thread until shutdown. */
            while(!fShutdown.load())
            {
                /* Reset the events flag. */
                fEvent = false;

                /* If mining is enabled, notify miner LLP that events processor is finished processing transactions so mined blocks
                   can include these transactions and not orphan a mined block. */
                if(LLP::MINING_SERVER)
                    LLP::MINING_SERVER.load()->NotifyEvent();

                /* Wait for the events processing thread to be woken up (such as a login) */
                std::unique_lock<std::mutex> lock(EVENTS_MUTEX);
                CONDITION.wait_for(lock, std::chrono::milliseconds(5000), [this]{ return fEvent.load() || fShutdown.load();});

                /* Check for a shutdown event. */
                if(fShutdown.load())
                    return;

                try
                {
                    /* Auto-login feature if configured and not already logged in. */
                    if(!config::fMultiuser.load() && !LoggedIn() && config::GetBoolArg("-autologin"))
                        auto_login();

                    /* Ensure that the user is logged, in, wallet unlocked, and unlocked for notifications. */
                    if(LoggedIn() && !Locked() && CanProcessNotifications() && !TAO::Ledger::ChainState::Synchronizing())
                        auto_process_notifications();

                }
                catch(const std::exception& e)
                {
                    /* Log the error and attempt to continue processing */
                    debug::error(FUNCTION, e.what());
                }

            }
        }



        /* Automatically logs in the sig chain using the credentials configured in the config file.  Will also create the sig
        *  chain if it doesn't exist and configured with autocreate=1.
        *  When autocreate=1 this will log in the user while sig chain create is still in the mempool */
        void Users::auto_login()
        {
            /* Flag indicating that the auto login has successfully run.  Once it has run successfully once it will not run again
               for the lifespan of the application, to avoid auto-logging you back in if you intentionally log out. */
            static bool fAutoLoggedIn = false;
            try
            {
                /* If we haven't already logged in at least once, are configured for auto login, and are not currently logged in */
                if(!fAutoLoggedIn && config::GetBoolArg("-autologin") && !config::fMultiuser.load() && !LoggedIn())
                {
                    /* Keep a the credentials in secure allocated strings. */
                    SecureString strUsername = config::GetArg("-username", "").c_str();
                    SecureString strPassword = config::GetArg("-password", "").c_str();
                    SecureString strPin = config::GetArg("-pin", "").c_str();

                    /* Check we have user/pass/pin */
                    if(strUsername.empty() || strPassword.empty() || strPin.empty())
                        throw APIException(-203, "Autologin missing username/password/pin");

                    /* Create the sigchain. */
                    memory::encrypted_ptr<TAO::Ledger::SignatureChain> user =
                        new TAO::Ledger::SignatureChain(strUsername.c_str(), strPassword.c_str());

                    /* Get the genesis ID. */
                    uint256_t hashGenesis = user->Genesis();

                    /* See if the sig chain exists */
                    if(!LLD::Ledger->HasGenesis(hashGenesis) && !TAO::Ledger::mempool.Has(hashGenesis))
                    {
                        /* If it doesn't exist then create it if configured to do so */
                        if(config::GetBoolArg("-autocreate"))
                        {
                            /* Testnet is considered local if no dns is being used or if using a private network */
                            bool fLocalTestnet = config::fTestNet.load()
                                && (!config::GetBoolArg("-dns", true) || config::GetBoolArg("-private"));

                            /* Can only create user if synced and (if not local) have connections.
                             * Return without create/login if cannot create, yet. It will have to try again.
                             */
                            if(TAO::Ledger::ChainState::Synchronizing()
                            || (LLP::TRITIUM_SERVER->GetConnectionCount() == 0 && !fLocalTestnet))
                            {
                                user.free();
                                return;
                            }

                            /* The genesis transaction  */
                            TAO::Ledger::Transaction tx;

                            /* Create the sig chain genesis transaction */
                            create_sig_chain(strUsername, strPassword, strPin, tx);

                            /* Display that login was successful. */
                            debug::log(0, "Auto-Create Successful");
                        }
                        else
                            throw APIException(-203, "Autologin user not found");
                    }

                    /* Check for duplicates in ledger db. */
                    TAO::Ledger::Transaction txPrev;

                    /* Get the last transaction. */
                    uint512_t hashLast;
                    if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                    {
                        user.free();
                        throw APIException(-138, "No previous transaction found");
                    }

                    /* Get previous transaction */
                    if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                    {
                        user.free();
                        throw APIException(-138, "No previous transaction found");
                    }

                    /* Genesis Transaction. */
                    TAO::Ledger::Transaction tx;
                    tx.NextHash(user->Generate(txPrev.nSequence + 1, config::GetArg("-pin", "").c_str(), false), txPrev.nNextType);

                    /* Check for consistency. */
                    if(txPrev.hashNext != tx.hashNext)
                    {
                        user.free();
                        throw APIException(-139, "Invalid credentials");
                    }

                    /* Setup the account. */
                    {
                        LOCK(MUTEX);
                        mapSessions.emplace(0, std::move(user));
                    }

                    /* Extract the PIN. */
                    if(!pActivePIN.IsNull())
                        pActivePIN.free();

                    /* The unlock actions to apply for autologin.  NOTE we do NOT unlock for transactions */
                    uint8_t nUnlockActions = TAO::Ledger::PinUnlock::UnlockActions::MINING
                                           | TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS
                                           | TAO::Ledger::PinUnlock::UnlockActions::STAKING;

                    /* Set account to unlocked. */
                    pActivePIN = new TAO::Ledger::PinUnlock(config::GetArg("-pin", "").c_str(), nUnlockActions);

                    /* Display that login was successful. */
                    debug::log(0, "Auto-Login Successful");

                    /* Set the flag so that we don't attempt to log in again */
                    fAutoLoggedIn = true;

                    /* Start the stake minter if successful login. */
                    TAO::Ledger::StakeMinter::GetInstance().Start();
                }
            }
            catch(const APIException& e)
            {
                debug::error(FUNCTION, e.what());
            }

        }


        /* Process notifications for the currently logged in user(s) */
        void Users::auto_process_notifications()
        {
            /* Dummy params to pass into ProcessNotifications call */
            json::json params;

            try
            {
                /* Invoke the process notifications method to process all oustanding */
                ProcessNotifications(params, false);
            }
            catch(const APIException& ex)
            {
                /* Absorb certain errors and write them to the log instead */
                switch (ex.id)
                {
                case 202: // Signature chain not mature after your previous mined/stake block. X more confirmation(s) required
                case 255: // Cannot process notifications until peers are connected
                case 256: // Cannot process notifications whilst synchronizing
                    debug::log(2, FUNCTION, ex.what());
                    break;
                
                default:
                    break;
                }
            }
            
            

            

        }


        /*  Notifies the events processor that an event has occurred so it
         *  can check and update it's state. */
        void Users::NotifyEvent()
        {
            fEvent = true;
            CONDITION.notify_one();
        }

        /* Checks that the contract passes both Build() and Execute() */
        bool Users::sanitize_contract(TAO::Operation::Contract& contract, std::map<uint256_t, TAO::Register::State> &mapStates)
        {
            /* Return flag */
            bool fSanitized = false;

            /* Lock the mempool at this point so that we can build and execute inside a mempool transaction */
            RLOCK(TAO::Ledger::mempool.MUTEX);

            try
            {
                /* Start a ACID transaction (to be disposed). */
                LLD::TxnBegin(TAO::Ledger::FLAGS::MEMPOOL);

                fSanitized = TAO::Register::Build(contract, mapStates, TAO::Ledger::FLAGS::MEMPOOL)
                             && TAO::Operation::Execute(contract, TAO::Ledger::FLAGS::MEMPOOL);

                /* Abort the mempool ACID transaction once the contract is sanitized */
                LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL);

            }
            catch(const std::exception& e)
            {
                /* Abort the mempool ACID transaction */
                LLD::TxnAbort(TAO::Ledger::FLAGS::MEMPOOL);

                /* Log the error and attempt to continue processing */
                debug::error(FUNCTION, e.what());
            }

            return fSanitized;
        }
    }
}
