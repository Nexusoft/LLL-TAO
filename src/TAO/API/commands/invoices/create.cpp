/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/global.h>
#include <TAO/API/include/constants.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/get.h>

#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/invoices.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/constants.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Creates a new invoice. */
    encoding::json Invoices::Create(const encoding::json& jParams, const bool fHelp)
    {
        /* Grab our address we want to be paid to. */
        const TAO::Register::Address hashAccount = ExtractAddress(jParams);

        /* Validate the payment account */
        TAO::Register::Object rObject;
        if(!LLD::Register->ReadObject(hashAccount, rObject))
            throw Exception(-13, "Object not found");

        /* Check the object standard. */
        if(rObject.Base() != TAO::Register::OBJECTS::ACCOUNT)
            throw Exception(-65, "Object is not an account");

        /* Check for recipient by username. */
        const uint256_t hashRecipient = ExtractRecipient(jParams);

        /* Check that the recipient genesis hash exists */
        if(!LLD::Ledger->HasFirst(hashRecipient))
            throw Exception(-230, "Recipient user does not exist");

        /* Add the mandatory invoice fields to the invoice JSON */
        encoding::json jInvoice =
        {
            { "account",   hashAccount.ToString()   },
            { "recipient", hashRecipient.ToString() },
            { "items",     encoding::json::array()  }
        };

        /* Add all other non-mandatory fields that the caller has provided */
        for(auto it = jParams.begin(); it != jParams.end(); ++it)
        {
            /* Get our keyname. */
            const std::string strKey = ToLower(it.key());

            /* Skip any incoming parameters that are keywords used by this API method*/
            if(strKey == "pin"
            || strKey == "session"
            || strKey == "name"
            || strKey == "account"
            || strKey == "account_name"
            || strKey == "recipient"
            || strKey == "recipient_username"
            || strKey == "amount"
            || strKey == "token"
            || strKey == "items"
            || strKey == "request")
            {
                continue;
            }

            /* add the field to the invoice */
            jInvoice[strKey] = it.value();
        }

        /* Parse the invoice items details */
        if(jParams.find("items") == jParams.end() || !jParams["items"].is_array())
            throw Exception(-232, "Missing items");

        /* Check items is not empty */
        const encoding::json& jItems = jParams["items"];
        if(jItems.empty())
            throw Exception(-233, "Invoice must include at least one item");

        /* Grab the digits for this token type. */
        const uint8_t nDigits =
            GetDecimals(rObject);

        /* Iterate the items to validate them */
        uint64_t nTotal = 0;
        for(auto it = jItems.begin(); it != jItems.end(); ++it)
        {
            /* Get a copy of our parameters. */
            const encoding::json jItem = (*it);

            /* The item Unit Amount */
            const uint64_t nUnitAmount =
                ExtractAmount(jItem, nDigits);

            /* The item number of units */
            const uint64_t nUnits =
                ExtractInteger<uint64_t>(jItem, "units");

            /* Rebuild our JSON to use correct formatting. */
            encoding::json jNew =
            {
                { "amount",      FormatBalance(nUnitAmount, nDigits) },
                { "units",       nUnits }
            };

            /* Check for description key. */
            if(CheckParameter(jItem, "description", "string"))
                jNew["description"] = jItem["description"].get<std::string>();

            /* Now add to our new items list. */
            jInvoice["items"].push_back(jNew);

            /* Aggregate our total balance now. */
            nTotal += (nUnitAmount * nUnits);
        }

        /* The token that this invoice should be transacted in */
        const TAO::Register::Address hashToken =
            rObject.get<uint256_t>("token");

        /* Add the invoice amount and token */
        jInvoice["amount"] = FormatBalance(nTotal, nDigits);
        jInvoice["token"]  = hashToken.ToString();

        /* DataStream to help us serialize the data. */
        DataStream ssData(SER_REGISTER, 1);
        ssData << uint16_t(USER_TYPES::INVOICE) << jInvoice.dump(-1);

        /* Check the data size */
        if(ssData.size() > TAO::Register::MAX_REGISTER_SIZE)
            throw Exception(-242, "Data exceeds max register size");

        /* Generate a random hash for this objects register address */
        const TAO::Register::Address hashRegister =
            TAO::Register::Address(TAO::Register::Address::READONLY);

        /* Build our vector of contracts to submit. */
        std::vector<TAO::Operation::Contract> vContracts(2);

        /* Add the invoice creation contract. */
        vContracts[0] << uint8_t(TAO::Operation::OP::CREATE)        << hashRegister;
        vContracts[0] << uint8_t(TAO::Register::REGISTER::READONLY) << ssData.Bytes();

        /* Add the transfer contract */
        vContracts[1] << uint8_t(TAO::Operation::OP::CONDITION);
        vContracts[1] << uint8_t(TAO::Operation::OP::TRANSFER) << hashRegister << hashRecipient;
        vContracts[1] << uint8_t(TAO::Operation::TRANSFER::CLAIM);

        /* Create conditional binary stream to compare and use with the VALIDATE operation. */
        TAO::Operation::Stream ssCondition;
        ssCondition << uint8_t(TAO::Operation::OP::DEBIT) << uint256_t(0) << hashAccount << nTotal << uint64_t(0);

        /* The asset transfer condition requiring pay from specified token and value. */
        vContracts[1] <= uint8_t(TAO::Operation::OP::GROUP);
        vContracts[1] <= uint8_t(TAO::Operation::OP::CALLER::OPERATIONS) <= uint8_t(TAO::Operation::OP::CONTAINS);
        vContracts[1] <= uint8_t(TAO::Operation::OP::TYPES::BYTES) <= ssCondition.Bytes();
        vContracts[1] <= uint8_t(TAO::Operation::OP::AND);
        vContracts[1] <= uint8_t(TAO::Operation::OP::CALLER::PRESTATE::VALUE) <= std::string("token");
        vContracts[1] <= uint8_t(TAO::Operation::OP::EQUALS);
        vContracts[1] <= uint8_t(TAO::Operation::OP::TYPES::UINT256_T) <= hashToken; // ensure the token paid matches the invoice
        vContracts[1] <= uint8_t(TAO::Operation::OP::UNGROUP);

        vContracts[1] <= uint8_t(TAO::Operation::OP::OR);

        /* The clawback clause that allows sender to cancel invoice. */
        vContracts[1] <= uint8_t(TAO::Operation::OP::GROUP);
        vContracts[1] <= uint8_t(TAO::Operation::OP::CALLER::GENESIS);
        vContracts[1] <= uint8_t(TAO::Operation::OP::EQUALS);
        vContracts[1] <= uint8_t(TAO::Operation::OP::CONTRACT::GENESIS);
        vContracts[1] <= uint8_t(TAO::Operation::OP::UNGROUP);

        /* Add optional name if specified. */
        BuildName(jParams, hashRegister, vContracts);

        return BuildResponse(jParams, hashRegister, vContracts);
    }
}
