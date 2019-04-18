/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/accounts.h>

#include <TAO/Ledger/types/transaction.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/include/hex.h>

#include <Util/include/args.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** List of accounts in API. **/
        Accounts accounts;


        /* Standard initialization function. */
        void Accounts::Initialize()
        {
            mapFunctions["create"]              = Function(std::bind(&Accounts::Create,          this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["login"]               = Function(std::bind(&Accounts::Login,           this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["logout"]              = Function(std::bind(&Accounts::Logout,          this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["lock"]                = Function(std::bind(&Accounts::Lock,            this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["unlock"]              = Function(std::bind(&Accounts::Unlock,          this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["transactions"]        = Function(std::bind(&Accounts::Transactions, this, std::placeholders::_1, std::placeholders::_2));
            mapFunctions["notifications"]       = Function(std::bind(&Accounts::Notifications, this, std::placeholders::_1, std::placeholders::_2));
        }


        /* Determine if a sessionless user is logged in. */
        bool Accounts::LoggedIn() const
        {
            return !config::fAPISessions && mapSessions.count(0); 
        }


        /* Determine if the accounts are locked. */
        bool Accounts::Locked() const
        {
            if(config::fAPISessions || pActivePIN.IsNull() || pActivePIN->PIN().empty())
                return true;

            return false;
        }

        /* In sessionless API mode this method checks that the active sig chain has 
         * been unlocked to allow transactions.  If the account has not been specifically
         * unlocked then we assume that they ARE allowed to transact, since the PIN would
         * need to be provided in each API call */
        bool Accounts::CanTransact() const
        {
            if(config::fAPISessions || pActivePIN.IsNull() || pActivePIN->CanTransact())
                return true;

            return false;
        }

        /* In sessionless API mode this method checks that the active sig chain has 
         *  been unlocked to allow minting */
        bool Accounts::CanMint() const
        {
            if(config::fAPISessions || (!pActivePIN.IsNull() && pActivePIN->CanMint()))
                return true;

            return false;
        }


        /* Returns a key from the account logged in. */
        uint512_t Accounts::GetKey(uint32_t nKey, SecureString strSecret, uint64_t nSession) const
        {
            LOCK(MUTEX);

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint64_t nSessionToUse = config::fAPISessions ? nSession : 0;

            if(!mapSessions.count(nSessionToUse))
            {
                if( config::fAPISessions)
                    throw APIException(-1, debug::safe_printstr("session ", nSessionToUse, " doesn't exist"));
                else
                    throw APIException(-1, "User not logged in");
            }

            return mapSessions[nSessionToUse]->Generate(nKey, strSecret);
        }


        /* Returns the genesis ID from the account logged in. */
        uint256_t Accounts::GetGenesis(uint64_t nSession) const
        {
            LOCK(MUTEX);

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint64_t nSessionToUse = config::fAPISessions ? nSession : 0;

            if(!mapSessions.count(nSessionToUse))
            {
                if( config::fAPISessions)
                    throw APIException(-1, debug::safe_printstr("session ", nSessionToUse, " doesn't exist"));
                else
                    throw APIException(-1, "User not logged in");
            }

            return mapSessions[nSessionToUse]->Genesis(); //TODO: Assess the security of being able to generate genesis. Most likely this should be a localDB thing.
        }


        /* Returns the sigchain the account logged in. */
        static memory::encrypted_ptr<TAO::Ledger::SignatureChain> null_ptr;
        memory::encrypted_ptr<TAO::Ledger::SignatureChain>& Accounts::GetAccount(uint64_t nSession) const
        {
            LOCK(MUTEX);

            /* For sessionless API use the active sig chain which is stored in session 0 */
            uint64_t nSessionToUse = config::fAPISessions ? nSession : 0;
            
            /* Check if you are logged in. */
            if(!mapSessions.count(nSessionToUse))
                return null_ptr;

            return mapSessions[nSessionToUse];
        }

        /* Returns the pin number for the currently logged in account. */
        SecureString Accounts::GetActivePin() const
        {
            return SecureString(pActivePIN->PIN().c_str());
        }

        /* If the API is running in sessionless mode this method will return the currently 
         * active PIN (if logged in) or the pin from the params.  If not in sessionless mode
         * then the method will return the pin from the params.  If no pin is available then
         * an APIException is thrown */
        SecureString Accounts::GetPin(const json::json params) const
        {
            /* Check for pin parameter. */
            SecureString strPIN;
            bool fNeedPin = accounts.Locked();

            if( fNeedPin && params.find("pin") == params.end() )
                throw APIException(-25, "Missing PIN");
            else if( fNeedPin)
                strPIN = params["pin"].get<std::string>().c_str();
            else
                strPIN = accounts.GetActivePin();
            
            return strPIN; 
        }

        /* If the API is running in sessionless mode this method will return the default
         * session ID that is used to store the one and only session (ID 0). If the user is not
         * logged in than an APIException is thrown.  
         * If not in sessionless mode then the method will return the session from the params.  
         * If the session is not is available in the params then an APIException is thrown. */
        uint64_t Accounts::GetSession(const json::json params) const
        {
            /* Check for session parameter. */
            uint64_t nSession = 0; // ID 0 is used for sessionless API

            if( !config::fAPISessions && !accounts.LoggedIn())
                throw APIException(-25, "User not logged in");
            else if(config::fAPISessions)
            {
                if(params.find("session") == params.end())
                    throw APIException(-25, "Missing Session ID");
                else
                    nSession = std::stoull(params["session"].get<std::string>());    
            }
            
            return nSession;
        }
    }
}
