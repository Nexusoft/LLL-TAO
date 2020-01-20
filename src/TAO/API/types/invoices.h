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
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /** Invoices
         *
         *  Invoices API Class.
         *  Manages the function pointers for all Invoices API methods.
         *
         **/
        class Invoices : public Base
        {
        public:

            /** Default Constructor. **/
            Invoices()
            : Base()
            {
                Initialize();
            }


            /** Initialize.
             *
             *  Sets the function pointers for this API.
             *
             **/
            void Initialize() final;


            /** GetName
             *
             *  Returns the name of this API.
             *
             **/
            std::string GetName() const final
            {
                return "Invoices";
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
             *  Creates a new invoice.
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
             *  Returns information about a single invoice .
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            json::json Get(const json::json& params, bool fHelp);


            /** Send
             *
             *  Sends an invoice to the recipient
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            //json::json Send(const json::json& params, bool fHelp);


            /** Pay
             *
             *  Pay an invoice and take ownership of it
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            //json::json Pay(const json::json& params, bool fHelp);


            /** Cancel
             *
             *  Cancels (voids) an invoice that has been sent 
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            //json::json Cancel(const json::json& params, bool fHelp);


            /** History
             *
             *  Gets the history of an invoice.
             *
             *  @param[in] params The parameters from the API call.
             *  @param[in] fHelp Trigger for help data.
             *
             *  @return The return object in JSON.
             *
             **/
            //json::json History(const json::json& params, bool fHelp);
        };
    }
}
