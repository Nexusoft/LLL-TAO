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


        /** RewriteURL
         *
         *  Allows derived API's to handle custom/dynamic URL's where the strMethod does not
         *  map directly to a function in the target API.  Insted this method can be overriden to
         *  parse the incoming URL and route to a different/generic method handler, adding parameter
         *  values if necessary.  E.g. get/myasset could be rerouted to get/asset with name=myasset
         *  added to the jsonParams
         *  The return json contains the modifed method URL to be called.
         *
         *  @param[in] strMethod The name of the method being invoked.
         *  @param[in] jsonParams The json array of parameters being passed to this method.
         *
         *  @return the API method URL
         *
         **/
        std::string RewriteURL(const std::string& strMethod, json::json& jsonParams) override;



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
        json::json Create(const json::json& params, bool fHelp);


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
        json::json Get(const json::json& params, bool fHelp);


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
        json::json Update(const json::json& params, bool fHelp);


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
        json::json Transfer(const json::json& params, bool fHelp);


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
        json::json Claim(const json::json& params, bool fHelp);


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
        json::json History(const json::json& params, bool fHelp);


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
        json::json Tokenize(const json::json& params, bool fHelp);


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
        json::json GetSchema(const json::json& params, bool fHelp);

    };
}
