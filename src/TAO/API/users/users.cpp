/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/hex.h>
#include <Util/include/args.h>

#include <functional>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** Default Constructor. **/
        Users::Users()
        : mapSessions()
        , pActivePIN()
        , MUTEX()
        , EVENTS_MUTEX()
        , EVENTS_THREAD()
        , CONDITION()
        , fEvent(false)
        , fShutdown(false)
        {
            Initialize();

            /* Events processor only enabled if multi-user session is disabled. */
            //if(config::fAPISessions.load() == false)
                //EVENTS_THREAD = std::thread(std::bind(&Users::EventsThread, this));
        }


        /** Destructor. **/
        Users::~Users()
        {
            /* Iterate through the sessions map and delete any sig chains that are still active */
            for(auto& session : mapSessions)
            {
                /* Check that is hasn't already been destroyed before freeing it*/
                if( !session.second.IsNull())
                    session.second.free();
            }

            /* Clear the sessions map of all entries */
            mapSessions.clear();

            if(!pActivePIN.IsNull())
                pActivePIN.free();

            /* Set the shutdown flag and join events processing thread. */
            fShutdown = true;

            /* Events processor only enabled if multi-user session is disabled. */
            if(EVENTS_THREAD.joinable())
            {
                NotifyEvent();
                EVENTS_THREAD.join();
            }
        }


        /* Allows derived API's to handle custom/dynamic URL's where the strMethod does not
        *  map directly to a function in the target API.  Insted this method can be overriden to
        *  parse the incoming URL and route to a different/generic method handler, adding parameter
        *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
        *  added to the jsonParams
        *  The return json contains the modifed method URL to be called.
        */
        std::string Users::RewriteURL(const std::string& strMethod, json::json& jsonParams)
        {
            std::string strMethodRewritten = strMethod;

            /* route create/myusername to create/user?username=myusername */
            /* check to see if this method is a create/myusername format. i.e. it starts with get/ */
            if(strMethod.find("create/") == 0)
            {
                /* get the user name from after the create/ */
                std::string strUserName = strMethod.substr(7);

                strMethodRewritten = "create/user";
                jsonParams["username"] = strUserName;
            }
            /* route login/myusername to login/user?username=myusername */
            /* check to see if this method is a login/myusername format. i.e. it starts with get/ */
            else if(strMethod.find("login/") == 0)
            {
                /* get the user name from after the login/ */
                std::string strUserName = strMethod.substr(6);

                strMethodRewritten = "login/user";
                jsonParams["username"] = strUserName;
            }
            /* route logout/myusername to logout/user?username=myusername */
            /* check to see if this method is a logout/myusername format. i.e. it starts with get/ */
            else if(strMethod.find("logout/") == 0)
            {
                /* get the user name from after the logout/ */
                std::string strUserName = strMethod.substr(7);

                strMethodRewritten = "logout/user";
                jsonParams["username"] = strUserName;
            }
            /* route lock/myusername to lock/user?username=myusername */
            /* check to see if this method is a lock/myusername format. i.e. it starts with get/ */
            else if(strMethod.find("lock/") == 0)
            {
                /* get the user name from after the lock/ */
                std::string strUserName = strMethod.substr(5);

                strMethodRewritten = "lock/user";
                jsonParams["username"] = strUserName;
            }
            /* route unlock/myusername to unlock/user?username=myusername */
            /* check to see if this method is a unlock/myusername format. i.e. it starts with get/ */
            else if(strMethod.find("unlock/") == 0)
            {
                /* get the user name from after the unlock/ */
                std::string strUserName = strMethod.substr(7);

                strMethodRewritten = "unlock/user";
                jsonParams["username"] = strUserName;
            }

            return strMethodRewritten;
        }


        /* Standard initialization function. */
        void Users::Initialize()
        {
            mapFunctions["create/user"]              = Function(std::bind(&Users::Create,          this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["login/user"]               = Function(std::bind(&Users::Login,           this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["logout/user"]              = Function(std::bind(&Users::Logout,          this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["lock/user"]                = Function(std::bind(&Users::Lock,            this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["unlock/user"]              = Function(std::bind(&Users::Unlock,          this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/transactions"]        = Function(std::bind(&Users::Transactions,    this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["list/notifications"]       = Function(std::bind(&Users::Notifications,   this, std::placeholders::_1, std::placeholders::_2));
        }


        /* Determine if a sessionless user is logged in. */
        bool Users::LoggedIn() const
        {
            return !config::fAPISessions.load() && mapSessions.count(0);
        }


        /* Determine if the Users are locked. */
        bool Users::Locked() const
        {
            if(config::fAPISessions.load() || pActivePIN.IsNull() || pActivePIN->PIN().empty())
                return true;

            return false;
        }


        /* In sessionless API mode this method checks that the active sig chain has
         * been unlocked to allow transactions.  If the account has not been specifically
         * unlocked then we assume that they ARE allowed to transact, since the PIN would
         * need to be provided in each API call */
        bool Users::CanTransact() const
        {
            if(config::fAPISessions.load() || pActivePIN.IsNull() || pActivePIN->CanTransact())
                return true;

            return false;
        }


        /* In sessionless API mode this method checks that the active sig chain has
         *  been unlocked to allow minting */
        bool Users::CanMint() const
        {
            if(config::fAPISessions.load() || (!pActivePIN.IsNull() && pActivePIN->CanMint()))
                return true;

            return false;
        }


        /* Returns a key from the account logged in. */
        uint512_t Users::GetKey(uint32_t nKey, SecureString strSecret, uint64_t nSession) const
        {
            LOCK(MUTEX);

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint64_t nSessionToUse = config::fAPISessions.load() ? nSession : 0;

            if(!mapSessions.count(nSessionToUse))
            {
                if( config::fAPISessions.load())
                    throw APIException(-1, debug::safe_printstr("session ", nSessionToUse, " doesn't exist"));
                else
                    throw APIException(-1, "User not logged in");
            }

            return mapSessions[nSessionToUse]->Generate(nKey, strSecret);
        }


        /* Returns the genesis ID from the account logged in. */
        uint256_t Users::GetGenesis(uint64_t nSession) const
        {
            LOCK(MUTEX);

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint64_t nSessionToUse = config::fAPISessions.load() ? nSession : 0;

            if(!mapSessions.count(nSessionToUse))
            {
                if(config::fAPISessions.load())
                    throw APIException(-1, debug::safe_printstr("session ", nSessionToUse, " doesn't exist"));
                else
                    throw APIException(-1, "User not logged in");
            }

            return mapSessions[nSessionToUse]->Genesis(); //TODO: Assess the security of being able to generate genesis. Most likely this should be a localDB thing.
        }


        /* Returns the sigchain the account logged in. */
        static memory::encrypted_ptr<TAO::Ledger::SignatureChain> null_ptr;
        memory::encrypted_ptr<TAO::Ledger::SignatureChain>& Users::GetAccount(uint64_t nSession) const
        {
            LOCK(MUTEX);

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint64_t nSessionToUse = config::fAPISessions.load() ? nSession : 0;

            /* Check if you are logged in. */
            if(!mapSessions.count(nSessionToUse))
                return null_ptr;

            return mapSessions[nSessionToUse];
        }


        /* Returns the pin number for the currently logged in account. */
        SecureString Users::GetActivePin() const
        {
            return SecureString(pActivePIN->PIN().c_str());
        }


        /* If the API is running in sessionless mode this method will return the currently
         * active PIN (if logged in) or the pin from the params.  If not in sessionless mode
         * then the method will return the pin from the params.  If no pin is available then
         * an APIException is thrown */
        SecureString Users::GetPin(const json::json params) const
        {
            /* Check for pin parameter. */
            SecureString strPIN;
            bool fNeedPin = users->Locked();

            if(fNeedPin && params.find("pin") == params.end())
                throw APIException(-25, "Missing PIN");
            else if(fNeedPin)
                strPIN = params["pin"].get<std::string>().c_str();
            else
                strPIN = users->GetActivePin();

            return strPIN;
        }


        /* If the API is running in sessionless mode this method will return the default
         * session ID that is used to store the one and only session (ID 0). If the user is not
         * logged in than an APIException is thrown, if fThrow is true.
         * If not in sessionless mode then the method will return the session from the params.
         * If the session is not is available in the params then an APIException is thrown, if fThrow is true. */
        uint64_t Users::GetSession(const json::json params, bool fThrow) const
        {
            /* Check for session parameter. */
            uint64_t nSession = 0; // ID 0 is used for sessionless API

            if(!config::fAPISessions.load() && !users->LoggedIn())
            {
                if(fThrow)
                    throw APIException(-25, "User not logged in");
                else
                    return -1;
            }
            else if(config::fAPISessions.load())
            {
                if(params.find("session") == params.end())
                {
                    if(fThrow)
                        throw APIException(-25, "Missing Session ID");
                    else
                        return -1;
                }
                else
                    nSession = std::stoull(params["session"].get<std::string>());
            }

            return nSession;
        }
    }
}
