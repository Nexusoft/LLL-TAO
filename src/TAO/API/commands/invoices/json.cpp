/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/invoices.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Returns the JSON representation of this invoice */
    encoding::json Invoices::InvoiceToJSON(const TAO::Register::Object& rObject, const uint256_t& hashRegister)
    {
        /* The JSON to return */
        encoding::json jRet =
            RegisterToJSON(rObject, hashRegister);

        /* Check for recipient to find status. */
        if(jRet.find("json") != jRet.end())
        {
            /* Get the recipient genesis hash from the invoice data so that we can use it to calculate the status*/
            const uint256_t hashRecipient =
                uint256_t(jRet["json"]["recipient"].get<std::string>());

            /* Add status */
            jRet["json"]["status"] = get_status(rObject, hashRecipient);
        }

        return jRet;
    }
}
