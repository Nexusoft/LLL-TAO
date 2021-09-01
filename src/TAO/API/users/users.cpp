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

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/global.h>

#include <TAO/API/types/session-manager.h>
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
        : Derived<Users>          ( )
        , LOGIN_THREAD            ( )
        , NOTIFICATIONS_PROCESSOR (nullptr)
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
        void Users::LoginThread() //XXX: this isn't really needed, could easily be added before processing notifications, DELETE ME
        {
            /* Loop the events processing thread until shutdown. */
            while(!config::fShutdown.load())
            {
                /* We sleep in increments of 100ms so shutdown response is sub-second. */
                for(uint32_t n = 0; n < 50; ++n)
                    runtime::sleep(100);  //XXX: assess whether this should come before or after the login attempt

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
        uint256_t Users::GetCallersGenesis(const encoding::json& jParams) const
        {
            /* default to session 0 unless using multiuser mode */
            uint256_t hashSession = 0;

            if(config::fMultiuser.load() && jParams.find("session") != jParams.end())
                hashSession.SetHex(jParams["session"].get<std::string>());

            return GetGenesis(hashSession);
        }


        /* Returns the genesis ID from the account logged in. */
        uint256_t Users::GetGenesis(const uint256_t& hashSession, bool fThrow) const
        {

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint256_t hashSessionToUse = config::fMultiuser.load() ? hashSession : 0;

            if(!GetSessionManager().Has(hashSessionToUse))
            {
                if(fThrow)
                {
                    if(config::fMultiuser.load())
                        throw Exception(-9, debug::safe_printstr("Session ", hashSessionToUse.ToString(), " doesn't exist"));
                    else
                        throw Exception(-11, "User not logged in");
                }
                else
                {
                    return uint256_t(0);
                }
            }

            return GetSessionManager().Get(hashSessionToUse, false).GetAccount()->Genesis(); //TODO: Assess the security of being able to generate genesis. Most likely this should be a localDB thing.
        }


        /* If the API is running in sessionless mode this method will return the currently
         * active PIN (if logged in) or the pin from the jParams.  If not in sessionless mode
         * then the method will return the pin from the jParams.  If no pin is available then
         * an Exception is thrown */
        SecureString Users::GetPin(const encoding::json& jParams, const uint8_t nUnlockAction) const
        {
            /* Get the active session */
            Session& session = GetSession(jParams, true, false);

            /* If we have a pin already, check we are allowed to use it for the requested action */
            bool fNeedPin = session.GetActivePIN().IsNull() || session.GetActivePIN()->PIN().empty() || !(session.GetActivePIN()->UnlockedActions() & nUnlockAction);
            if(fNeedPin)
            {
                /* Grab our pin secure string. */
                const SecureString strPIN =
                    ExtractPIN(jParams);

                return strPIN;
            }

            /* If we don't need the pin then use the current active one */
            const SecureString strPIN =
                session.GetActivePIN()->PIN();

            return strPIN;
        }


        /* If the API is running in sessionless mode this method will return the default
         * session ID that is used to store the one and only session (ID 0). If the user is not
         * logged in than an Exception is thrown, if fThrow is true.
         * If not in sessionless mode then the method will return the session from the jParams.
         * If the session is not is available in the jParams then an Exception is thrown, if fThrow is true. */
        Session& Users::GetSession(const encoding::json& jParams, const bool fThrow, const bool fLogActivity) const
        {
            /* Check for session parameter. */
            uint256_t hashSession = 0; // ID 0 is used for sessionless API

            if(config::fMultiuser.load())
            {
                if(jParams.find("session") == jParams.end())
                {
                    if(fThrow)
                        throw Exception(-12, "Missing Session ID");
                }
                else
                    hashSession.SetHex(jParams["session"].get<std::string>());

                /* Check that the session ID is valid */
                if(fThrow && !GetSessionManager().Has(hashSession))
                    throw Exception(-10, "Invalid session ID");
            }

            /* Calling SessionManager.Get() with an invalid session ID will throw an exception.  Therefore if the caller has
               specified not to throw an exception we have to check whether the session exists first. */
            if(!fThrow && !GetSessionManager().Has(hashSession))
                return null_session;

            return GetSessionManager().Get(hashSession, fLogActivity);
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

            throw Exception(-11, "User not logged in");
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
        uint512_t Users::GetKey(const uint32_t nKey, const SecureString& strSecret, const Session& session) const
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
                            TAO::Ledger::Transaction& tx, const uint8_t nScheme)
        {
            /* No need to check connections or maturity in private mode as there is no PoS/Pow */
            if(!config::fHybrid.load())
            {
                /* If not on local-only testnet then we need to ensure we are connected to the network and
                synchronized before allowing any sig chain transactions to be created */
                bool fLocalTestnet = config::fTestNet.load() && !config::GetBoolArg("-dns", true);
                if(TAO::Ledger::ChainState::Synchronizing() || (LLP::TRITIUM_SERVER->GetConnectionCount() == 0 && !fLocalTestnet))
                    throw Exception(-213, "Cannot create transactions whilst synchronizing");

                /* Check that the sig chain is mature after the last coinbase/coinstake transaction in the chain. */
                CheckMature(user->Genesis());
            }

            /* Create the transaction and return */
            return TAO::Ledger::CreateTransaction(user, pin, tx, nScheme);
        }


        /* Checks that the session/password/pin parameters have been provided (where necessary) and then verifies that the
        *  password and pin are correct.
        *  If authentication fails then the AuthAttempts counter in the callers session is incremented */
        bool Users::Authenticate(const encoding::json& jParams)
        {
            /* Get the PIN to be used for this API call */
            SecureString strPIN = Commands::Get<Users>()->GetPin(jParams, TAO::Ledger::PinUnlock::TRANSACTIONS);

            /* Get the session to be used for this API call */
            Session& session = Commands::Get<Users>()->GetSession(jParams);

            /* Check the account. */
            if(!session.GetAccount())
                throw Exception(-10, "Invalid session ID");

            /* Check for password requirement field. */
            if(config::GetBoolArg("-requirepassword", false))
            {
                /* Grab our password from parameters. */
                if(!CheckParameter(jParams, "password", "string"))
                    throw Exception(-128, "-requirepassword active, must pass in password=<password> for all commands when enabled");

                /* Parse out password. */
                const SecureString strPassword =
                    SecureString(jParams["password"].get<std::string>().c_str());

                /* Check our password input compared to our internal sigchain password. */
                if(session.GetAccount()->Password() != strPassword)
                {
                    /* Increment our auth attempts if failed password. */
                    session.IncrementAuthAttempts();
                    return false;
                }
            }

            /* The logged in sig chain genesis hash */
            const uint256_t hashGenesis =
                session.GetAccount()->Genesis();

            /* Get the sig chain transaction to authenticate with, using the same hash that was used at login . */
            TAO::Ledger::Transaction txPrev;
            if(!LLD::Ledger->ReadTx(session.hashAuth, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                throw Exception(-138, "No previous transaction found");

            /* Generate our secret key from sigchain. */
            const uint512_t hashSecret =
                session.GetAccount()->Generate(txPrev.nSequence + 1, strPIN, false);

            /* Calculate our next hash for auth check. */
            const uint256_t hashNext =
                TAO::Ledger::Transaction::NextHash(hashSecret, txPrev.nNextType);

            /* Validate the credentials */
            if(txPrev.hashNext != hashNext)
            {
                /* If the hashNext does not match then credentials are invalid, so increment the auth attempts counter */
                session.IncrementAuthAttempts();

                return false;
            }

            /* Set our internal cache if credentials passed. */
            session.GetAccount()->SetCache(hashSecret, txPrev.nSequence + 1);
            session.ResetAuthAttempts();

            return true;
        }


        /* Gracefully closes down a users session */
        void Users::TerminateSession(const uint256_t& hashSession)
        {
            /* Check that the session exists */
            if(!GetSessionManager().Has(hashSession))
                throw Exception(-141, "Already logged out");

            /* The genesis of the user logging out */
            uint256_t hashGenesis = GetSessionManager().Get(hashSession).GetAccount()->Genesis();

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
                NOTIFICATIONS_PROCESSOR->Remove(hashSession);

            /* If this is session 0 and stake minter is running when logout, stop it */
            TAO::Ledger::StakeMinter& stakeMinter = TAO::Ledger::StakeMinter::GetInstance();
            if(hashSession == 0 && stakeMinter.IsStarted())
                stakeMinter.Stop();

            /* If this user has previously saved their session to the local DB, then delete it */
            if(LLD::Local->HasSession(hashGenesis))
                LLD::Local->EraseSession(hashGenesis);

            /* Finally remove the session from the session manager */
            GetSessionManager().Remove(hashSession);
        }


        /* Used for client mode, this method will download the signature chain transactions and events for a given genesis */
        bool Users::DownloadSigChain(const uint256_t& hashGenesis, bool fSyncEvents)
        {
            /* Check that server is active. */
            if(!LLP::TRITIUM_SERVER)
                return debug::error(FUNCTION, "Tritium server not initialized...");

            //XXX: GetConnection should return true/false and return connection by reference
            std::shared_ptr<LLP::TritiumNode> pNode = LLP::TRITIUM_SERVER->GetConnection();
            if(pNode != nullptr)
            {
                /* Log the start time to syncronize. */
                uint64_t nStart = runtime::unifiedtimestamp(true); //true for ms timestamp
                debug::log(1, FUNCTION, "CLIENT MODE: Synchronizing Signatiure Chain");

                /* Request the sig chain. */
                debug::log(1, FUNCTION, "CLIENT MODE: Requesting LIST::SIGCHAIN for ", hashGenesis.SubString());
                LLP::TritiumNode::SyncSigChain(pNode.get(), hashGenesis, true, fSyncEvents);

                /* Log the time it took to complete sigchain sync. */
                uint64_t nStop = runtime::unifiedtimestamp(true); //true for ms timestamp
                debug::log(1, FUNCTION, "CLIENT MODE: LIST::SIGCHAIN received for ", hashGenesis.SubString(), " in ", nStop - nStart, " milliseconds");

                return true;
            }
            else
                return debug::error(FUNCTION, "no connections available...");

        }
    }
}
