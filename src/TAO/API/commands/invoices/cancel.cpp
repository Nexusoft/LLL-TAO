/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/invoices.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/conditions.h>
#include <TAO/API/include/extract.h>

#include <TAO/Operation/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Transfers an item. */
    encoding::json Invoices::Cancel(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the Register ID. */
        const TAO::Register::Address hashRegister = ExtractAddress(jParams);

        /* Get the invoice object register . */
        TAO::Register::State steCheck;
        if(!LLD::Register->ReadState(hashRegister, steCheck, TAO::Ledger::FLAGS::MEMPOOL))
            throw Exception(-241, "Invoice not found");

        /* Now lets check our expected types match. */
        if(!CheckStandard(jParams, steCheck))
            throw Exception(-49, "Unsupported type for name/address");

        /* Serialize the invoice into JSON. */
        const encoding::json jInvoice =
            InvoiceToJSON(steCheck, hashRegister);

        /* The recipient genesis hash */
        const uint256_t hashRecipient =
            TAO::Register::Address(jInvoice["json"]["recipient"].get<std::string>());

        /* Get the invoice status so that we can validate that we are allowed to cancel it */
        const std::string strStatus = get_status(steCheck, hashRecipient);

        /* Check if invoice has been paid already. */
        if(strStatus == "PAID")
            throw Exception(-245, "Cannot [cancel] an invoice that has already been paid");

        /* Check if invoice has been cancelled. */
        if(strStatus == "CANCELLED")
            throw Exception(-246, "Cannot [cancel] an invoice that has already been cancelled");

        /* Look up the transaction ID & contract ID of the transfer so that we can void it */
        std::vector<uint512_t> vTransactions;
        if(!LLD::Logical->ListTransactions(hashRegister, vTransactions))
            throw Exception(-247, "Could not find invoice transfer transaction");

        /* The transaction ID to cancel */
        const uint512_t hashTx =
            vTransactions.back();

        /* Read the debit transaction. */
        TAO::Ledger::Transaction tx;
        if(!LLD::Ledger->ReadTx(hashTx, tx))
            throw Exception(-40, "Register transaction not found.");

        /* Make sure our contract is correct. */
        if(tx.Size() < 2)
            throw Exception(-247, "Invalid invoice transfer transaction: ", tx.Size());

        /* Make sure our contract is the right one to void. */
        if(tx[1].Primitive() != TAO::Operation::OP::TRANSFER)
            throw Exception(-247, "Invalid invoice transfer transaction");

        /* Build our vector of contracts to submit. */
        std::vector<TAO::Operation::Contract> vContracts;

        /* Process the contract and attempt to void it */
        if(!BuildVoid(jParams, 1, tx[1], vContracts))
            throw Exception(-43, "No valid contracts in tx.");

        /* Build response JSON boilerplate. */
        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
