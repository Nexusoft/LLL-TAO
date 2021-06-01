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
    /** Supply
     *
     *  Supply API Class.
     *  Manages the function pointers for all Supply commands.
     *
     **/
    class Supply : public Derived<Supply>
    {
    public:

        /** Default Constructor. **/
        Supply()
        : Derived<Supply>()
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
            return "supply";
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
        std::string RewriteURL(const std::string& strMethod, encoding::json& jsonParams) override;


        /** GetItem
         *
         *  Gets the description of an item.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json GetItem(const encoding::json& params, const bool fHelp);


        /** Transfer
         *
         *  Transfers an item.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Transfer(const encoding::json& params, const bool fHelp);


        /** Claims
         *
         *  Claims an item transfer
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Claim(const encoding::json& params, const bool fHelp);


        /** CreateItem
         *
         *  Creates an item.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json CreateItem(const encoding::json& params, const bool fHelp);


        /** UpdateItem
         *
         *  Updates data to an item.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json UpdateItem(const encoding::json& params, const bool fHelp);


        /** History
         *
         *  Gets the history of an item.
         *
         *  @param[in] params The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json History(const encoding::json& params, const bool fHelp);
    };
}
