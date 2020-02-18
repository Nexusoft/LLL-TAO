/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

#include <TAO/Register/types/address.h>
#include <TAO/Register/types/state.h>

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
            json::json Pay(const json::json& params, bool fHelp);


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
            json::json Cancel(const json::json& params, bool fHelp);


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
            json::json History(const json::json& params, bool fHelp);


            /** InvoiceToJSON
            *
            *  Returns the JSON representation of this invoice
            *
            *  @param[in] params The parameters from the API call.
            *  @param[in] state The state register containing the invoice data
            *  @param[in] hashInvoice The register address of the invoice state register 
            *
            *  @return the invoice JSON
            *
            **/
            static json::json InvoiceToJSON(const json::json& params, const TAO::Register::State& state, 
                                       const TAO::Register::Address& hashInvoice);


        private:

            /** get_status
            *
            *  Returns a status for the invoice (draft/outstanding/paid/cancelled)
            *
            *  @param[in] state The state register containing the invoice data
            *  @param[in] hashRecipient The genesis hash of the invoice recipient
            *
            *  @return string containing the invoice status (draft/outstanding/paid/cancelled)
            *
            **/
            static std::string get_status(const TAO::Register::State& state, const uint256_t& hashRecipient);


            /** get_tx
            *
            *  Looks up the transaction ID and Contract ID for the transfer transaction that needs to be paid
            *
            *  @param[in] hashRecipient The genesis hash of the invoice recipient
            *  @param[in] hashInvoice The register address of the invoice state register
            *  @param[out] txid The transaction ID containing the conditional transfer
            *  @param[out] contract The contract ID within the transaction of the conditional transfer
            *
            *  @return boolean, True if the transaction info was found
            *
            **/
            static bool get_tx(const uint256_t& hashRecipient, const TAO::Register::Address& hashInvoice, 
                               uint512_t& txid, uint32_t& contract);


            
        
        };
    }
}
