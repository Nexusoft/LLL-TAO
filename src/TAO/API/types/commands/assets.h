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
    /** Assets
     *
     *  Assets API Class.
     *  Manages the function pointers for all Asset commands.
     *
     **/
    class Assets : public Derived<Assets>
    {
    public:

        /** Default Constructor. **/
        Assets()
        : Derived<Assets>()
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
            return "assets";
        }


        /** Create
         *
         *  Create an asset or digital item.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Create(const encoding::json& params, const bool fHelp);


        /** Get
         *
         *  Get the data from a digital asset
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Get(const encoding::json& params, const bool fHelp);


        /** Update
         *
         *  Update the data in an asset
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Update(const encoding::json& params, const bool fHelp);


        /** Transfer
         *
         *  Transfer an asset or digital item.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Transfer(const encoding::json& params, const bool fHelp);


        /** Claim
         *
         *  Claim a transferred asset .
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Claim(const encoding::json& params, const bool fHelp);


        /** History
         *
         *  History of an asset and its ownership
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json History(const encoding::json& params, const bool fHelp);


        /** Tokenize
         *
         *  Tokenize an asset into fungible tokens that represent ownership.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Tokenize(const encoding::json& params, const bool fHelp);


        /** GetSchema
         *
         *  Get the schema for an asset
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json GetSchema(const encoding::json& params, const bool fHelp);

    };
}
