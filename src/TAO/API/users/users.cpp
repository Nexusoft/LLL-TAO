/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>
#include <TAO/API/types/sessionmanager.h>
#include <TAO/API/users/types/notifications_processor.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/stake_minter.h>
#include <TAO/Ledger/types/transaction.h>

#include <Util/include/hex.h>
#include <Util/include/args.h>

#include <functional>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Default Constructor. */
        Users::Users()
        : Base()
        , fShutdown(false)
        , LOGIN_THREAD()
        , NOTIFICATIONS_PROCESSOR(nullptr)
        {
            Initialize();

            /* Auto login thread only if enabled and not in multiuser mode */
            if(!config::fMultiuser.load() && config::GetBoolArg("-autologin"))
                LOGIN_THREAD = std::thread(std::bind(&Users::LoginThread, this));

            /* Initialize the notifications processor if configured */
            if(config::fProcessNotifications)
                NOTIFICATIONS_PROCESSOR = new NotificationsProcessor(config::GetArg("-notificationsthreads", 1));
        }


        /* Destructor. */
        Users::~Users()
        {
            /* Set the shutdown flag and join events processing thread. */
            fShutdown = true;

            if(LOGIN_THREAD.joinable())
                LOGIN_THREAD.join();

            /* Destroy the notifications processor */
            if(NOTIFICATIONS_PROCESSOR)
                delete NOTIFICATIONS_PROCESSOR;

            /* Clear all sessions */
            GetSessionManager().Clear();

            /* Delete session */
            delete SESSION_MANAGER;
            SESSION_MANAGER = nullptr;
        }


        /*  Background thread to auto login user once connections are established. */
        void Users::LoginThread()
        {
            /* Mutex for this thread's condition variable */
            std::mutex LOGIN_MUTEX;

            /* The condition variable to wait on */
            std::condition_variable LOGIN_CONDITION;

            /* Loop the events processing thread until shutdown. */
            while(!fShutdown.load())
            {
                /* retry every 5s if not logged in */
                std::unique_lock<std::mutex> lock(LOGIN_MUTEX);
                LOGIN_CONDITION.wait_for(lock, std::chrono::milliseconds(5000), [this]{return fShutdown.load();});

                /* Check for a shutdown event. */
                if(fShutdown.load())
                    return;

                try
                {
                    /* Auto-login if not already logged in. */
                    if(!LoggedIn())
                        auto_login();

                }
                catch(const std::exception& e)
                {
                    /* Log the error and attempt to continue processing */
                    debug::error(FUNCTION, e.what());
                }
            }
        }


        /* Returns the genesis ID from the calling session or the the account logged in.*/
        uint256_t Users::GetCallersGenesis(const json::json & params) const
        {
            /* default to session 0 unless using multiuser mode */
            uint256_t nSession = 0;

            if(config::fMultiuser.load() && params.find("session") != params.end())
                nSession.SetHex(params["session"].get<std::string>());

            return GetGenesis(nSession);
        }


        /* Returns the genesis ID from the account logged in. */
        uint256_t Users::GetGenesis(uint256_t nSession, bool fThrow) const
        {

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint256_t nSessionToUse = config::fMultiuser.load() ? nSession : 0;

            if(!GetSessionManager().Has(nSessionToUse))
            {
                if(fThrow)
                {
                    if(config::fMultiuser.load())
                        throw APIException(-9, debug::safe_printstr("Session ", nSessionToUse.ToString(), " doesn't exist"));
                    else
                        throw APIException(-11, "User not logged in");
                }
                else
                {
                    return uint256_t(0);
                }
            }

            return GetSessionManager().Get(nSessionToUse, false).GetAccount()->Genesis(); //TODO: Assess the security of being able to generate genesis. Most likely this should be a localDB thing.
        }


        /* If the API is running in sessionless mode this method will return the currently
         * active PIN (if logged in) or the pin from the params.  If not in sessionless mode
         * then the method will return the pin from the params.  If no pin is available then
         * an APIException is thrown */
        SecureString Users::GetPin(const json::json params, uint8_t nUnlockAction) const
        {

            /* Check for pin parameter. */
            SecureString strPIN;

            /* Get the active session */
            Session& session = GetSession(params, true, false);

            /* If we have a pin already, check we are allowed to use it for the requested action */
            bool fNeedPin = session.GetActivePIN().IsNull() || session.GetActivePIN()->PIN().empty() || !(session.GetActivePIN()->UnlockedActions() & nUnlockAction);
            if(fNeedPin)
            {
                /* If we need a pin then check it is in the params */
                if(params.find("pin") == params.end())
                    throw APIException(-129, "Missing PIN");
                else
                    strPIN = params["pin"].get<std::string>().c_str();
            }
            else
                /* If we don't need the pin then use the current active one */
                strPIN = session.GetActivePIN()->PIN();

            return strPIN;
        }


        /* If the API is running in sessionless mode this method will return the default
         * session ID that is used to store the one and only session (ID 0). If the user is not
         * logged in than an APIException is thrown, if fThrow is true.
         * If not in sessionless mode then the method will return the session from the params.
         * If the session is not is available in the params then an APIException is thrown, if fThrow is true. */
        Session& Users::GetSession(const json::json params, bool fThrow, bool fLogActivity) const
        {
            /* Check for session parameter. */
            uint256_t nSession = 0; // ID 0 is used for sessionless API

            if(config::fMultiuser.load())
            {
                if(params.find("session") == params.end())
                {
                    if(fThrow)
                        throw APIException(-12, "Missing Session ID");
                }
                else
                    nSession.SetHex(params["session"].get<std::string>());

                /* Check that the session ID is valid */
                if(fThrow && !GetSessionManager().Has(nSession))
                    throw APIException(-10, "Invalid session ID");
            }

            /* Calling SessionManager.Get() with an invalid session ID will throw an exception.  Therefore if the caller has
               specified not to throw an exception we have to check whether the session exists first. */
            if(!fThrow && !GetSessionManager().Has(nSession))
                return null_session;

            return GetSessionManager().Get(nSession, fLogActivity);
        }


        /*Gets the session ID for a given genesis, if it is logged in on this node. */
        Session& Users::GetSession(const uint256_t& hashGenesis, bool fLogActivity) const
        {
            if(!config::fMultiuser.load())
            {
                if(GetSessionManager().mapSessions.count(0) > 0 && GetSessionManager().mapSessions[0].GetAccount()->Genesis() == hashGenesis)
                    return GetSessionManager().Get(0, fLogActivity);
            }
            else
            {
                auto session = GetSessionManager().mapSessions.begin();
                while(session != GetSessionManager().mapSessions.end())
                {
                    if(session->second.GetAccount()->Genesis() == hashGenesis)
                        return GetSessionManager().Get(session->first, fLogActivity);

                    /* increment iterator */
                    ++session;
                }
            }

            throw APIException(-11, "User not logged in");
        }


        /* Determine if a sessionless user is logged in. */
        bool Users::LoggedIn() const
        {
            return !config::fMultiuser.load() && GetSessionManager().Has(0);
        }
        

        /* Determine if a particular genesis is logged in on this node. */
        bool Users::LoggedIn(const uint256_t& hashGenesis) const
        {
            if(!config::fMultiuser.load())
            {
                return GetSessionManager().Has(0) > 0 && GetSessionManager().Get(0, false).GetAccount()->Genesis() == hashGenesis;
            }
            else
            {
                auto session = GetSessionManager().mapSessions.begin();
                while(session != GetSessionManager().mapSessions.end())
                {
                    if(session->second.GetAccount()->Genesis() == hashGenesis)
                        return true;
                    /* increment iterator */
                    ++session;
                }
            }

            return false;
        }


        /* Returns a key from the account logged in. */
        uint512_t Users::GetKey(uint32_t nKey, SecureString strSecret, const Session& session) const
        {
            return session.GetAccount()->Generate(nKey, strSecret);
        }


        /* Determines whether the signature chain has reached desired maturity after the last coinbase/coinstake transaction */
        uint32_t Users::BlocksToMaturity(const uint256_t hashGenesis)
        {
            /* The number of blocks to maturity to return */
            uint32_t nBlocksToMaturity = 0;

            /* Get the user configurable required maturity */
            uint32_t nMaturityRequired = config::GetArg("-maturityrequired", config::fTestNet ? 2 : 33);

            /* If set to 0 then there is no point checking the maturity so return early */
            if(nMaturityRequired == 0)
                return 0;

            /* The hash of the last transaction for this sig chain from disk */
            uint512_t hashLast = 0;
            if(LLD::Ledger->ReadLast(hashGenesis, hashLast))
            {
                /* Get the last transaction from disk for this sig chain */
                TAO::Ledger::Transaction txLast;
                if(!LLD::Ledger->ReadTx(hashLast, txLast))
                    return debug::error(FUNCTION, "last transaction not on disk");

                /* If the previous transaction is a coinbase or coinstake then check the maturity */
                if(txLast.IsCoinBase() || txLast.IsCoinStake())
                {
                    /* Get number of confirmations of previous TX */
                    uint32_t nConfirms = 0;
                    LLD::Ledger->ReadConfirmations(hashLast, nConfirms);

                    /* Check to see if it is mature */
                    if(nConfirms < nMaturityRequired)
                        nBlocksToMaturity = nMaturityRequired - nConfirms;
                }
            }

            return nBlocksToMaturity;
        }


        /* Create a new transaction object for signature chain, if allowed to do so */
        bool Users::CreateTransaction(const memory::encrypted_ptr<TAO::Ledger::SignatureChain>& user, const SecureString& pin,
                            TAO::Ledger::Transaction& tx)
        {
            /* No need to check connections or maturity in private mode as there is no PoS/Pow */
            if(!config::GetBoolArg("-private"))
            {
                /* If not on local-only testnet then we need to ensure we are connected to the network and
                synchronized before allowing any sig chain transactions to be created */
                bool fLocalTestnet = config::fTestNet.load() && !config::GetBoolArg("-dns", true);

                if(TAO::Ledger::ChainState::Synchronizing() || (LLP::TRITIUM_SERVER->GetConnectionCount() == 0 && !fLocalTestnet))
                    throw APIException(-213, "Cannot create transactions whilst synchronizing");

                /* Check that the sig chain is mature after the last coinbase/coinstake transaction in the chain. */
                CheckMature(user->Genesis());
            }

            /* Create the transaction and return */
            return TAO::Ledger::CreateTransaction(user, pin, tx);
        }

        /* Checks that the session/password/pin parameters have been provided (where necessary) and then verifies that the
        *  password and pin are correct.
        *  If authentication fails then the AuthAttempts counter in the callers session is incremented */
        bool Users::Authenticate(const json::json& params)
        {
            /* Get the PIN to be used for this API call */
            SecureString strPIN = users->GetPin(params, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = users->GetSession(params);

            /* Check the account. */
            if(!session.GetAccount())
                throw APIException(-10, "Invalid session ID");

            /* The logged in sig chain genesis hash */
            uint256_t hashGenesis = session.GetAccount()->Genesis();

            /* Get the sig chain transaction to authenticate with, using the same hash that was used at login . */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->ReadTx(session.hashAuth, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-138, "No previous transaction found");

            /* Generate a temporary transaction with the next hash based on the current password/pin */
            TAO::Ledger::Transaction tx;
            tx.NextHash(session.GetAccount()->Generate(txPrev.nSequence + 1, strPIN), txPrev.nNextType);

            /* Validate the credentials */
            if(txPrev.hashNext != tx.hashNext)
            {
                /* If the hashNext does not match then credentials are invalid, so increment the auth attempts counter */
                session.IncrementAuthAttempts();

                /* If the number of failed auth attempts exceeds the configured allowed number then log this user out */
                if(session.GetAuthAttempts() >= config::GetArg("-authattempts", 3))
                {
                    debug::log(0, FUNCTION, "Too many invalid password / pin attempts. Logging out user session:", session.ID().ToString() );

                    /* Log the user out.  NOTE this also closes down the stake minter, removes this session from the notifications
                       processor, terminates any P2P connections, and removes the session from the session manager */
                    TerminateSession(session.ID());

                    throw APIException(-290, "Invalid credentials.  User logged out due to too many password / pin attempts");

                }

                return false;
            }

            return true;
        }


        /* Gracefully closes down a users session */
        void Users::TerminateSession(const uint256_t& nSession)
        {
            /* Check that the session exists */
            if(!GetSessionManager().Has(nSession))
                throw APIException(-141, "Already logged out");

            /* The genesis of the user logging out */
            uint256_t hashGenesis = GetSessionManager().Get(nSession).GetAccount()->Genesis();

            /* If not using multi-user then we need to send a deauth message to all peers */
            if(!config::fMultiuser.load() && LLP::TRITIUM_SERVER)
            {
                /* Generate an DEAUTH message to send to all peers */
                DataStream ssMessage = LLP::TritiumNode::GetAuth(false);

                /* Check whether it is valid before relaying it to all peers */
                if(ssMessage.size() > 0)
                    LLP::TRITIUM_SERVER->_Relay(uint8_t(LLP::TritiumNode::ACTION::DEAUTH), ssMessage);
            }

            /* Remove the session from the notifications processor */
            if(NOTIFICATIONS_PROCESSOR)
                NOTIFICATIONS_PROCESSOR->Remove(nSession);

            /* If this is session 0 and stake minter is running when logout, stop it */
            TAO::Ledger::StakeMinter& stakeMinter = TAO::Ledger::StakeMinter::GetInstance();
            if(nSession == 0 && stakeMinter.IsStarted())
                stakeMinter.Stop();

            /* If this user has previously saved their session to the local DB, then delete it */
            if(LLD::Local->HasSession(hashGenesis))
                LLD::Local->EraseSession(hashGenesis);

            /* Finally remove the session from the session manager */
            GetSessionManager().Remove(nSession);
        }
    }
}
