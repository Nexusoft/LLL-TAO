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
#include <TAO/API/include/sessionmanager.h>
#include <TAO/API/types/notifications_thread.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/sigchain.h>
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

            /* Destroy the notifications processor */
            if(NOTIFICATIONS_PROCESSOR)
                delete NOTIFICATIONS_PROCESSOR;

            /* Clear all sessions */
            GetSessionManager().Clear();

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

            return GetGenesis(nSession, false);
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

            return GetSessionManager().Get(nSessionToUse).GetAccount()->Genesis(); //TODO: Assess the security of being able to generate genesis. Most likely this should be a localDB thing.
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
            Session& session = GetSession(params);

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
        Session& Users::GetSession(const json::json params, bool fThrow) const
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

            return GetSessionManager().Get(nSession);
        }


        /*Gets the session ID for a given genesis, if it is logged in on this node. */
        Session& Users::GetSession(const uint256_t& hashGenesis) const
        {
            if(!config::fMultiuser.load())
            {
                if(GetSessionManager().mapSessions.count(0) > 0 && GetSessionManager().mapSessions[0].GetAccount()->Genesis() == hashGenesis)
                    return GetSessionManager().Get(0);
            }
            else
            {
                auto session = GetSessionManager().mapSessions.begin();
                while(session != GetSessionManager().mapSessions.end())
                {
                    if(session->second.GetAccount()->Genesis() == hashGenesis)
                        return session->second;
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
                return GetSessionManager().Has(0) > 0 && GetSessionManager().Get(0).GetAccount()->Genesis() == hashGenesis;
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
    }
}
