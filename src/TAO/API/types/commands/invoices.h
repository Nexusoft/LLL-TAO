/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once

#include <TAO/API/types/base.h>

namespace TAO::Register { class Address; class State; }

/* Global TAO namespace. */
namespace TAO::API
{
    /** Invoices
     *
     *  Invoices API Class.
     *  Manages the function pointers for all Invoices API methods.
     *
     **/
    class Invoices : public Derived<Invoices>
    {
    public:

        /** Default Constructor. **/
        Invoices()
        : Derived<Invoices>()
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
            return "invoices";
        }


        /** Create
         *
         *  Creates a new invoice.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Create(const encoding::json& jParams, const bool fHelp);


        /** Pay
         *
         *  Pay an invoice and take ownership of it
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Pay(const encoding::json& jParams, const bool fHelp);


        /** Cancel
         *
         *  Cancels (voids) an invoice that has been sent
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json Cancel(const encoding::json& jParams, const bool fHelp);


        /** History
         *
         *  Gets the history of an invoice.
         *
         *  @param[in] jParams The parameters from the API call.
         *  @param[in] fHelp Trigger for help data.
         *
         *  @return The return object in JSON.
         *
         **/
        encoding::json History(const encoding::json& jParams, const bool fHelp);


        /** InvoiceToJSON
         *
         *  Returns the JSON representation of this invoice
         *
         *  @param[in] rObject The state register containing the invoice data
         *  @param[in] hashRegister The register address of the invoice state register
         *
         *  @return the invoice JSON
         *
         **/
        static encoding::json InvoiceToJSON(const TAO::Register::Object& rObject, const uint256_t& hashRegister);


    private:

        /** get_status
        *
        *  Returns a status for the invoice (outstanding/paid/cancelled)
        *
        *  @param[in] state The state register containing the invoice data
        *  @param[in] hashRecipient The genesis hash of the invoice recipient
        *
        *  @return string containing the invoice status (outstanding/paid/cancelled)
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
