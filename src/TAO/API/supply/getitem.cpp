/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/supply.h>

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

            /* Check for username parameter. */
            if(params.find("address") == params.end())
                throw APIException(-23, "Missing memory address");

            /* Get the Register ID. */
            uint256_t hashRegister;
            hashRegister.SetHex(params["address"]);

            /* Get the history. */
            TAO::Register::State state;
            if(!LLD::regDB->ReadState(hashRegister, state))
                throw APIException(-24, "No state found");

            /* Build the response JSON. */
            //ret["version"]  = state.nVersion;
            //ret["type"]     = state.nType;
            ret["timestamp"]  = state.nTimestamp;
            ret["owner"]    = state.hashOwner.ToString();

            while(!state.end())
            {
                /* If the data type is string. */
                std::string data;
                state >> data;

                //ret["checksum"] = state.hashChecksum;
                ret["state"] = data;
            }

            return ret;
        }
    }
}
