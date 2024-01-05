/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

#include <TAO/API/types/authentication.h>

/* Global TAO namespace. */
namespace TAO::API
{

    /** Sessions
     *
     *  Sessions API Class.
     *  Manages the function pointers for all Sessions commands.
     *
     **/
    class Sessions : public Derived<Sessions>
    {
    public:

        /** Default Constructor. **/
        Sessions()
        : Derived<Sessions>()
        {
        }


        /** Initialize.
         *
         *  Sets the function pointers for this API.
         *
         **/
        void Initialize() final;


        /** Name
         *
         *  Returns the name of this API.
         *
         **/
        static std::string Name()
        {
            return "sessions";
        }


        /** Create
         *
         *  Creates a new session based on login credentials.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Create(const encoding::json& jParams, const bool fHelp);


        /** List
         *
         *  Lists the current logged in sessions for -multiusername mode
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json List(const encoding::json& jParams, const bool fHelp);


        /** Load
         *
         *  Loads and resumes the users session from the Logical DB
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Load(const encoding::json& jParams, const bool fHelp);


        /** Lock
         *
         *  Lock an account for any given action
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Lock(const encoding::json& jParams, const bool fHelp);


        /** Save
         *
         *  Saves the users session into the Logical DB so that it can be resumed later.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Save(const encoding::json& jParams, const bool fHelp);


        /** Status
         *
         *  Gets the status of current active session
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Status(const encoding::json& jParams, const bool fHelp);


        /** Terminate
         *
         *  Terminates a given session by session-id.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Terminate(const encoding::json& jParams, const bool fHelp);


        /** Unlock
         *
         *  Unlock an account for any given action
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Unlock(const encoding::json& jParams, const bool fHelp);


    private:


        /** validate_session
         *
         *  Validates a session to crypto object register to make sure credentials are correct.
         *
         *  @param[in] tSession The session object to validate before proceeding.
         *  @param[in] strPIN The PIN number to be used to valide the session
         *
         *  @return true if the session authenticated correctly.
         *
         **/
        bool validate_session(const Authentication::Session& tSession, const SecureString& strPIN);

    };
}
