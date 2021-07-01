/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
            throw Exception(-49, "Unexpected type for name / address");

        /* Serialize the invoice into JSON. */
        const encoding::json jInvoice =
            InvoiceToJSON(jParams, steCheck, hashRegister);

        /* The recipient genesis hash */
        const uint256_t hashRecipient =
            uint256_t(jInvoice["recipient"].get<std::string>());

        /* Get the invoice status so that we can validate that we are allowed to cancel it */
        const std::string strStatus = get_status(steCheck, hashRecipient);

        /* Check if invoice has been paid already. */
        if(strStatus == "PAID")
            throw Exception(-245, "Cannot cancel an invoice that has already been paid");

        /* Check if invoice has been cancelled. */
        if(strStatus == "CANCELLED")
            throw Exception(-246, "Cannot cancel an invoice that has already been cancelled");

        /* The transaction ID to cancel */
        uint512_t hashTx;

        /* The contract ID to cancel */
        uint32_t nContract = 0;  //XXX: THIS SECTION COULD STILL DO WITH SOME WORK

        /* Look up the transaction ID & contract ID of the transfer so that we can void it */
        if(!get_tx(hashRecipient, hashRegister, hashTx, nContract))
            throw Exception(-247, "Could not find invoice transfer transaction");

        /* Read the debit transaction. */
        TAO::Ledger::Transaction tx;
        if(!LLD::Ledger->ReadTx(hashTx, tx))
            throw Exception(-40, "Previous transaction not found.");

        /* Build our vector of contracts to submit. */
        std::vector<TAO::Operation::Contract> vContracts;

        /* Process the contract and attempt to void it */
        TAO::Operation::Contract tContract;
        if(AddVoid(tx[nContract], nContract, tContract))
            vContracts.push_back(tContract);

        /* Build response JSON boilerplate. */
        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
