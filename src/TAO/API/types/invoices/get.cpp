/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/invoices.h>
#include <TAO/API/types/names.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/json.h>
#include <TAO/API/include/user_types.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/timelocks.h>

#include <LLD/include/global.h>


/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {
        /* Get's the description of an item. */
        json::json Invoices::Get(const json::json& params, bool fHelp)
        {
            /* First ensure that transaction version 2 active, as the conditions required for invoices were not enabled until v2 */
            const uint32_t nCurrent = TAO::Ledger::CurrentTransactionVersion();
            if(nCurrent < 2 || (nCurrent == 2 && !TAO::Ledger::TransactionVersionActive(runtime::unifiedtimestamp(), 2)))
                throw APIException(-254, "Invoices API not yet active.");

            json::json ret;

            /* Get the Register ID. */
            TAO::Register::Address hashRegister ;

            /* Check whether the caller has provided the asset name parameter. */
            if(params.find("name") != params.end())
            {
                /* If name is provided then use this to deduce the register address */
                hashRegister = Names::ResolveAddress(params, params["name"].get<std::string>());
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetBase58(params["address"].get<std::string>());

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-33, "Missing name / address");

            /* Get the invoice object register . */
            TAO::Register::State state;
            if(!LLD::Register->ReadState(hashRegister, state, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-241, "Invoice not found");

            /* Ensure that it is an invoice register */
            if(state.nType != TAO::Register::REGISTER::READONLY)
                throw APIException(-242, "Data at this address is not an invoice");

            /* Deserialize the leading byte of the state data to check the data type */
            uint16_t type;
            state >> type;

            /* Check that the state is an invoice */
            if(type != USER_TYPES::INVOICE)
                throw APIException(-242, "Data at this address is not an invoice");

            /* Build the response JSON. */
            ret = InvoiceToJSON(params, state, hashRegister);

            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            FilterResponse(params, ret);

            return ret;
        }


        /* Returns a status for the invoice (outstanding/paid/cancelled) */
        std::string Invoices::get_status(const TAO::Register::State& state, const uint256_t& hashRecipient)
        {
            /* The return value string */
            std::string strStatus;

            /* If the Invoice currently has no owner then we know it is outstanding */
            if(state.hashOwner.GetType() == TAO::Ledger::GENESIS::SYSTEM)
                strStatus = "OUTSTANDING";
            else if(state.hashOwner == hashRecipient)
                strStatus = "PAID";
            else
                strStatus = "CANCELLED";

            return strStatus;
        }

        /* Looks up the transaction ID and Contract ID for the transfer transaction that needs to be paid */
        bool Invoices::get_tx(const uint256_t& hashRecipient, const TAO::Register::Address& hashInvoice,
                              uint512_t& txid, uint32_t& contractID)
        {
            /* Get all registers that have been transferred to the recipient but not yet paid (claimed) */
            std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> vUnclaimed;
            Users::get_unclaimed(hashRecipient, vUnclaimed);

            /* search the vector of unclaimed to see if this invoice is in there */
            const auto& itt = std::find_if(vUnclaimed.begin(), vUnclaimed.end(),
                                            [&](const std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>& unclaimed) { return std::get<2>(unclaimed) == hashInvoice; });

            /* If found set the txid from the contract */
            if( itt != vUnclaimed.end())
            {
                txid = std::get<0>(*itt).Hash();
                contractID = std::get<1>(*itt);

                return true;
            }

            /* If we haven't found the transation then return false */
            return false;
        }

        /* Returns the JSON representation of this invoice */
        json::json Invoices::InvoiceToJSON(const json::json& params, const TAO::Register::State& state,
                                             const TAO::Register::Address& hashInvoice)
        {
            /* The JSON to return */
            json::json ret;

            /* Build the response JSON. */
            /* Look up the object name based on the Name records in the caller's sig chain */
            std::string strName = Names::ResolveName(users->GetCallersGenesis(params), hashInvoice);

            /* Add the name to the response if one is found. */
            if(!strName.empty())
                ret["name"] = strName;

            /* Add standard register details */
            ret["address"]    = hashInvoice.ToString();
            ret["created"]    = state.nCreated;
            ret["modified"]   = state.nModified;
            ret["owner"]      = TAO::Register::Address(state.hashOwner).ToString();

            /* Deserialize the invoice data */
            std::string strJSON;
            state >> strJSON;

            /* parse the serialized invoice JSON so that we can easily add the fields to the response */
            json::json invoice = json::json::parse(strJSON);

            /* Add each of the invoice fields to the response */
            for(auto it = invoice.begin(); it != invoice.end(); ++it)
                ret[it.key()] = it.value();

            /* Get the recipient genesis hash from the invoice data so that we can use it to calculate the status*/
            uint256_t hashRecipient;
            hashRecipient.SetHex(invoice["recipient"].get<std::string>());

            /* Add status */
            ret["status"] = get_status(state, hashRecipient);

            /* Add the payment txid and contract, if it is unclaimed (unpaid) */
            if(state.hashOwner.GetType() == TAO::Ledger::GENESIS::SYSTEM)
            {
                uint512_t txid = 0;
                uint32_t contract = 0;

                if(get_tx(hashRecipient, hashInvoice, txid, contract))
                {
                    ret["txid"] = txid.ToString();
                    ret["contract"] = contract;
                }

            }
            /* return the JSON */
            return ret;

        }
    }
}
