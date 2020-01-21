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
            uint8_t type;
            state >> type;

            /* Check that the state is an invoice */
            if(type != USER_TYPES::INVOICE)
                throw APIException(-242, "Data at this address is not an invoice");

            /* Build the response JSON. */
            /* Look up the object name based on the Name records in the caller's sig chain */
            std::string strName = Names::ResolveName(users->GetCallersGenesis(params), hashRegister);

            /* Add the name to the response if one is found. */
            if(!strName.empty())
                ret["name"] = strName;

            /* Add standard register details */
            ret["address"]    = hashRegister.ToString();
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

            /* Add status */
            ret["status"] = "TODO draft/outstanding/paid/cancelled";

            /* If the caller has requested to filter on a fieldname then filter out the json response to only include that field */
            FilterResponse(params, ret);

            return ret;
        }
    }
}
