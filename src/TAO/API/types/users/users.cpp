/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/utils.h>

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
        /* Returns the sigchain the account logged in. */
        static memory::encrypted_ptr<TAO::Ledger::SignatureChain> null_ptr;


        /* Default Constructor. */
        Users::Users()
        : Base()
        , mapSessions()
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
            if(config::fMultiuser.load() == false)
                EVENTS_THREAD = std::thread(std::bind(&Users::EventsThread, this));
        }


        /* Destructor. */
        Users::~Users()
        {
            /* Iterate through the sessions map and delete any sig chains that are still active */
            for(auto& session : mapSessions)
            {
                /* Check that is hasn't already been destroyed before freeing it*/
                if(!session.second.IsNull())
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


        /* Determine if a sessionless user is logged in. */
        bool Users::LoggedIn() const
        {
            return !config::fMultiuser.load() && mapSessions.count(0);
        }


        /* Determine if the Users are locked. */
        bool Users::Locked() const
        {
            if(config::fMultiuser.load() || pActivePIN.IsNull() || pActivePIN->PIN().empty())
                return true;

            return false;
        }


        /* In sessionless API mode this method checks that the active sig chain has
         * been unlocked to allow transactions.  If the account has not been specifically
         * unlocked then we assume that they ARE allowed to transact, since the PIN would
         * need to be provided in each API call */
        bool Users::CanTransact() const
        {
            if(config::fMultiuser.load() || pActivePIN.IsNull() || pActivePIN->CanTransact())
                return true;

            return false;
        }


        /* In sessionless API mode this method checks that the active sig chain has
         *  been unlocked to allow minting */
        bool Users::CanMint() const
        {
            if(config::fMultiuser.load() || (!pActivePIN.IsNull() && pActivePIN->CanMint()))
                return true;

            return false;
        }


        /* Returns a key from the account logged in. */
        uint512_t Users::GetKey(uint32_t nKey, SecureString strSecret, uint64_t nSession) const
        {
            LOCK(MUTEX);

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint64_t nSessionToUse = config::fMultiuser.load() ? nSession : 0;

            if(!mapSessions.count(nSessionToUse))
            {
                if(config::fMultiuser.load())
                    throw APIException(-9, debug::safe_printstr("Session ", nSessionToUse, " doesn't exist"));
                else
                    throw APIException(-11, "User not logged in");
            }

            return mapSessions[nSessionToUse]->Generate(nKey, strSecret);
        }


        /* Returns the genesis ID from the account logged in. */
        uint256_t Users::GetGenesis(uint64_t nSession, bool fThrow) const
        {
            LOCK(MUTEX);

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint64_t nSessionToUse = config::fMultiuser.load() ? nSession : 0;

            if(!mapSessions.count(nSessionToUse))
            {
                if(fThrow)
                {
                    if(config::fMultiuser.load())
                        throw APIException(-9, debug::safe_printstr("Session ", nSessionToUse, " doesn't exist"));
                    else
                        throw APIException(-11, "User not logged in");
                }
                else
                {
                    return uint256_t(0);
                }
            }

            return mapSessions[nSessionToUse]->Genesis(); //TODO: Assess the security of being able to generate genesis. Most likely this should be a localDB thing.
        }


        /* Returns the genesis ID from the calling session or the the account logged in.*/
        uint256_t Users::GetCallersGenesis(const json::json & params) const
        {
            /* default to session 0 unless using multiuser mode */
            uint64_t nSession = 0;

            if(config::fMultiuser.load() && params.find("session") != params.end())
                nSession = std::stoull(params["session"].get<std::string>());

            return GetGenesis(nSession, false);
        }


        /*  Returns the sigchain the account logged in. */
        memory::encrypted_ptr<TAO::Ledger::SignatureChain>& Users::GetAccount(uint64_t nSession) const
        {
            LOCK(MUTEX);

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint64_t nUse = config::fMultiuser.load() ? nSession : 0;

            /* Check if you are logged in. */
            if(!mapSessions.count(nUse))
                return null_ptr;

            return mapSessions[nUse];
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
                throw APIException(-129, "Missing PIN");
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

            if(!config::fMultiuser.load() && !users->LoggedIn())
            {
                if(fThrow)
                    throw APIException(-11, "User not logged in");
                else
                    return -1;
            }
            else if(config::fMultiuser.load())
            {
                if(params.find("session") == params.end())
                {
                    if(fThrow)
                        throw APIException(-12, "Missing Session ID");
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
