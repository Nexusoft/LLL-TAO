/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/invoices.h>
#include <TAO/API/users/types/users.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/include/timelocks.h>

#include <LLD/include/global.h>


/* Global TAO namespace. */
namespace TAO::API
{
    /* Get's the description of an item. */
    encoding::json Invoices::Get(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the Register ID. */
        const TAO::Register::Address hashRegister = ExtractAddress(jParams);

        /* Validate the payment account */
        TAO::Register::State steCheck;
        if(!LLD::Register->ReadState(hashRegister, steCheck))
            throw Exception(-13, "Object not found");

        /* Check our standard types here. */
        if(!CheckStandard(jParams, steCheck))
            throw Exception(-242, "Data at this address is not an invoice");

        /* Build the response JSON. */
        encoding::json jRet = InvoiceToJSON(jParams, steCheck, hashRegister);

        /* Filter a fieldname if supplied. */
        FilterFieldname(jParams, jRet);

        return jRet;
    }


    /* Returns a status for the invoice (outstanding/paid/cancelled) */
    std::string Invoices::get_status(const TAO::Register::State& state, const uint256_t& hashRecipient)
    {
        /* If the Invoice currently has no owner then we know it is outstanding */
        if(state.hashOwner.GetType() == TAO::Ledger::GENESIS::SYSTEM)
            return "OUTSTANDING";

        /* If register is owned by recipient, then it is paid. */
        if(state.hashOwner == hashRecipient)
            return "PAID";

        /* Otherwise treat as cancelled. */
        return "CANCELLED";
    }


    /* Looks up the transaction ID and Contract ID for the transfer transaction that needs to be paid */
    bool Invoices::get_tx(const uint256_t& hashRecipient, const TAO::Register::Address& hashInvoice,
                          uint512_t &hashTx, uint32_t &nContract)
    {
        /* Get all registers that have been transferred to the recipient but not yet paid (claimed) */
        std::vector<std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>> vUnclaimed;
        Users::get_unclaimed(hashRecipient, vUnclaimed);

        /* search the vector of unclaimed to see if this invoice is in there */
        const auto& itt = std::find_if(vUnclaimed.begin(), vUnclaimed.end(),
                            [&](const std::tuple<TAO::Operation::Contract, uint32_t, uint256_t>& tUnclaimed)
                            {
                                return std::get<2>(tUnclaimed) == hashInvoice;
                            });

        /* If found set the txid from the contract */
        if(itt != vUnclaimed.end())
        {
            hashTx = std::get<0>(*itt).Hash();
            nContract = std::get<1>(*itt);

            return true;
        }

        /* If we haven't found the transation then return false */
        return false;
    }


    /* Returns the JSON representation of this invoice */
    encoding::json Invoices::InvoiceToJSON(const encoding::json& jParams, const TAO::Register::State& state,
                                           const TAO::Register::Address& hashInvoice)
    {
        /* The JSON to return */
        encoding::json jRet;

        /* Add standard register details */
        jRet["address"]    = hashInvoice.ToString();
        jRet["created"]    = state.nCreated;
        jRet["modified"]   = state.nModified;
        jRet["owner"]      = TAO::Register::Address(state.hashOwner).ToString();

        /* Deserialize the invoice data */
        std::string strJSON;
        state >> strJSON;

        /* parse the serialized invoice JSON so that we can easily add the fields to the response */
        const encoding::json jInvoice = encoding::json::parse(strJSON);

        /* Add each of the invoice fields to the response */
        for(auto it = jInvoice.begin(); it != jInvoice.end(); ++it)
            jRet[it.key()] = it.value();

        /* Get the recipient genesis hash from the invoice data so that we can use it to calculate the status*/
        const uint256_t hashRecipient =
            uint256_t(jInvoice["recipient"].get<std::string>());

        /* Add status */
        jRet["status"] = get_status(state, hashRecipient);

        /* Add the payment txid and contract, if it is unclaimed (unpaid) */
        if(state.hashOwner.GetType() == TAO::Ledger::GENESIS::SYSTEM)
        {
            uint512_t hashTx   = 0;
            uint32_t nContract = 0;

            if(get_tx(hashRecipient, hashInvoice, hashTx, nContract))
            {
                jRet["txid"]     = hashTx.ToString();
                jRet["contract"] = nContract;
            }

        }

        return jRet;
    }
}
