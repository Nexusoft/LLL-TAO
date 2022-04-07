/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

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


        /** Save
         *
         *  Saves the users session into the Logical DB so that it can be resumed later after restart.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Save(const encoding::json& jParams, const bool fHelp);


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


        /** Has
         *
         *  Checks to see if a saves session exists in the Logical DB for the given user
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Has(const encoding::json& jParams, const bool fHelp);


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

    };
}
