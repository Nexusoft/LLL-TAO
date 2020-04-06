/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/supply.h>
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
        json::json Supply::GetItem(const json::json& params, bool fHelp)
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

            /* Get the history. */
            TAO::Register::State state;
            if(!LLD::Register->ReadState(hashRegister, state, TAO::Ledger::FLAGS::MEMPOOL))
                throw APIException(-117, "Item not found");

            if(config::fClient.load() && state.hashOwner != users->GetCallersGenesis(params))
                throw APIException(-300, "API can only be used to lookup data for the currently logged in signature chain when running in client mode");

            /* Ensure that it is an append register */
            if(state.nType != TAO::Register::REGISTER::APPEND)
                throw APIException(-117, "Item not found");

            /* Deserialize the leading byte of the state data to check the data type */
            uint16_t type;
            state >> type;

            /* Check that the state is an invoice */
            if(type != USER_TYPES::SUPPLY)
                throw APIException(-243, "Data at this address is not a supply item");

            /* Build the response JSON. */
            ret = ObjectToJSON(params, state, hashRegister);

            return ret;
        }
    }
}
