/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/invoices.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/get.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Ledger/types/transaction.h>

#include <Util/types/precision.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Transfers an item. */
    encoding::json Invoices::Pay(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the Register ID. */
        const TAO::Register::Address hashRegister = ExtractAddress(jParams);

        /* Get the invoice object register . */
        TAO::Register::State tInvoice;
        if(!LLD::Register->ReadState(hashRegister, tInvoice))
            throw Exception(-241, "Invoice not found");

        /* Now lets check our expected types match. */
        if(!CheckStandard(jParams, tInvoice))
            throw Exception(-49, "Unsupported type for name/address");

        /* Serialize the invoice into JSON. */
        const encoding::json jInvoice =
            InvoiceToJSON(tInvoice, hashRegister);

        /* The recipient genesis hash */
        const uint256_t hashRecipient =
            uint256_t(jInvoice["json"]["recipient"].get<std::string>());

        /* Get the invoice status so that we can validate that we are allowed to cancel it */
        const std::string strStatus = get_status(tInvoice, hashRecipient);

        /* Check if invoice has been paid already. */
        if(strStatus == "PAID")
            throw Exception(-245, "Cannot [pay] an invoice that has already been paid");

        /* Check if invoice has been cancelled. */
        if(strStatus == "CANCELLED")
            throw Exception(-246, "Cannot [pay] an invoice that has already been cancelled");

        /* The token type to pay. */
        const uint256_t hashToken =
            TAO::Register::Address(jInvoice["json"]["token"].get<std::string>());

        /* The recipient address. */
        const uint256_t hashTo =
            TAO::Register::Address(jInvoice["json"]["account"].get<std::string>());

        /* Get the Register ID. */
        const TAO::Register::Address hashFrom =
            ExtractAddress(jParams, "from");

        /* Get the account object. */
        TAO::Register::Object tAccount;
        if(!LLD::Register->ReadObject(hashFrom, tAccount))
            throw Exception(-13, "Object not found");

        /* Check the object standard. */
        if(tAccount.Base() != TAO::Register::OBJECTS::ACCOUNT)
            throw Exception(-65, "Object is not an account");

        /* Check that the payment is being made in the correct token */
        if(tAccount.get<uint256_t>("token") != hashToken)
            throw Exception(-252, "Account to debit is not for the required token");

        /* The amount to pay in token units */
        const uint64_t nAmount =
            jInvoice["json"]["amount"].get<double>() * GetFigures(tAccount);

        /* The transaction ID to cancel */
        uint512_t hashTx;

        /* The contract ID to cancel */
        uint32_t nContract = 0;  //XXX: THIS SECTION COULD STILL DO WITH SOME WORK

        /* Look up the transaction ID & contract ID of the transfer so that we can void it */
        if(!find_invoice(hashRecipient, hashRegister, hashTx, nContract))
            throw Exception(-247, "Could not find invoice transfer transaction");

        /* Read the debit transaction. */
        TAO::Ledger::Transaction tx;
        if(!LLD::Ledger->ReadTx(hashTx, tx))
            throw Exception(-40, "Previous transaction not found.");

        /* Build our vector of contracts to submit. */
        std::vector<TAO::Operation::Contract> vContracts(2);

        /* Add the DEBIT contract with the OP::VALIDATE */
        vContracts[0] << uint8_t(TAO::Operation::OP::VALIDATE) << hashTx   << nContract;
        vContracts[0] << uint8_t(TAO::Operation::OP::DEBIT)    << hashFrom << hashTo << nAmount << uint64_t(0);

        /* Make this contract spendable only by recipient. */
        vContracts[0] <= uint8_t(TAO::Operation::OP::CALLER::GENESIS);
        vContracts[0] <= uint8_t(TAO::Operation::OP::NOTEQUALS);
        vContracts[0] <= uint8_t(TAO::Operation::OP::CONTRACT::GENESIS);

        /* Add the CLAIM contract to claim the invoice */
        vContracts[1] << uint8_t(TAO::Operation::OP::CLAIM) << hashTx << nContract << hashRegister;

        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
