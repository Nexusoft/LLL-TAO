/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <Util/include/json.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /** @struct Templates
     *
     *  Structure to hold the template commands we will use for other command-sets.
     *
     **/
    struct Templates
    {
        /** Claim
         *
         *  Claims an incoming transfer from recipient.
         *
         *  @param[in] jParams The input parameters to the command.
         *  @param[in] fHelp Flag to determine if help was requested for command.
         *
         *  @return the json representation of given object.
         *
         **/
        static encoding::json Claim(const encoding::json& jParams, const bool fHelp);


        /** Create
         *
         *  Create an object based on pre-determined encoding and names.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *  @param[in] strAllowed The allowed formats for creating object.
         *  @param[in] nUserType The object type enumeration if applicable.
         *  @param[in] strDefault The default format to use
         *
         *  @return The return object in JSON.
         *
         **/
        static encoding::json Create(const encoding::json& jParams, const bool fHelp,
                                     const std::string& strAllowed, const uint16_t nUserType, const std::string& strDefault);


        /** Get
         *
         *  Get's a register by address or name.
         *
         *  @param[in] jParams The input parameters to the command.
         *  @param[in] fHelp Flag to determine if help was requested for command.
         *
         *  @return the json representation of given object.
         *
         **/
        static encoding::json Get(const encoding::json& jParams, const bool fHelp);


        /** List
         *
         *  Gets a list of registers that match given standard noun.
         *
         *  @param[in] jParams The input parameters to the command.
         *  @param[in] fHelp Flag to determine if help was requested for command.
         *  @param[in] fTransferred Flag to show values that have been transferred or not.
         *
         *  @return the json list of given object(s).
         *
         **/
        static encoding::json List(const encoding::json& jParams, const bool fTransferred = false);


        /** History
         *
         *  Gets a history of register states over the course of its lifetime.
         *
         *  @param[in] jParams The input parameters to the command.
         *  @param[in] fHelp Flag to determine if help was requested for command.
         *
         *  @return the json list of given object(s).
         *
         **/
        static encoding::json History(const encoding::json& jParams, const bool fHelp);


        /** Transactions
         *
         *  Lists all transactions of supported type in any scope.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        static encoding::json Transactions(const encoding::json& jParams, const bool fHelp);


        /** Transfer
         *
         *  Transfers an object to another user's signature chain
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        static encoding::json Transfer(const encoding::json& jParams, const bool fHelp);


        /** Update
         *
         *  Updates an object based on pre-determined allowed formats. Checks against standards.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *  @param[in] strAllowed The allowed formats for creating object.
         *  @param[in] nUserType The object type enumeration if applicable.
         *  @param[in] strDefault The default format to use
         *
         *  @return The return object in JSON.
         *
         **/
        static encoding::json Update(const encoding::json& jParams, const bool fHelp,
                                     const std::string& strAllowed, const uint16_t nUserType, const std::string& strDefault);



        /** Deprecated
         *
         *  Dummy method for assigning deprecated endpoints to.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        static encoding::json Deprecated(const encoding::json& jParams, const bool fHelp)
        {
            return encoding::json::array();
        }
    };
}
