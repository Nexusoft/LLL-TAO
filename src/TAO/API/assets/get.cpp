/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/assets.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Get the data from a digital asset */
        json::json Assets::Get(const json::json& params, bool fHelp)
        {
            json::json ret;

            /* Get the Register ID. */
            uint256_t hashRegister = 0;

            /* Check for data parameter. */
            if(params.find("name") != params.end())
            {
                /* Get the address from the name. */
                std::string strName = GetName() + ":" + params["name"].get<std::string>();

                /* Build the address from an SK256 hash of API:NAME. */
                hashRegister = LLC::SK256(std::vector<uint8_t>(strName.begin(), strName.end()));
            }

            /* Otherwise try to find the raw hex encoded address. */
            else if(params.find("address") != params.end())
                hashRegister.SetHex(params["address"]);

            /* Fail if no required parameters supplied. */
            else
                throw APIException(-23, "Missing memory address");

            /* Get the history. */
            TAO::Register::State state;
            if(!LLD::regDB->ReadState(hashRegister, state))
                throw APIException(-24, "No state found");

            /* Build the response JSON. */
            ret["timestamp"]  = state.nTimestamp;
            ret["owner"]      = state.hashOwner.ToString();

            /* If the data type is string. */
            std::string data;
            state >> data;

            ret["metadata"] = data;

            return ret;
        }
    }
}
