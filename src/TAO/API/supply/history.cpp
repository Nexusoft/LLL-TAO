/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/users.h>
#include <TAO/API/include/supply.h>

#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/verify.h>
#include <TAO/Register/include/enum.h>

#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>

#include <LLC/include/random.h>
#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Gets the history of an item. */
        json::json Supply::History(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Check for username parameter. */
            if(params.find("address") == params.end())
                throw APIException(-23, "Missing memory address");

            /* Get the Register ID. */
            uint256_t hashRegister;
            hashRegister.SetHex(params["address"].get<std::string>());

            /* Get the history. */
            std::vector<TAO::Register::State> states;
            if(!LLD::regDB->GetStates(hashRegister, states))
                throw APIException(-24, "No states found");

            /* Build the response JSON. */
            for(const auto& state : states)
            {
                json::json obj;
                obj["version"]  = state.nVersion;
                obj["owner"]    = state.hashOwner.ToString();
                obj["timestamp"]  = state.nTimestamp;

                while(!state.end())
                {
                    /* If the data type is string. */
                    std::string data;
                    state >> data;

                    obj["checksum"] = state.hashChecksum;
                    obj["state"] = data;
                }

                ret.push_back(obj);
            }

            return ret;
        }
    }
}
