/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/global.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/sessionmanager.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Lists the currently processed notifications. */
    json::json Users::Processed(const json::json& params, bool fHelp)
    {
        /* JSON return value. */
        json::json jRet = json::json::array();

        /* Get the Genesis ID. */
        const uint256_t hashGenesis = users->GetSession(params).GetAccount()->Genesis();

        /* Load the session */
        Session& session = users->GetSession(params);

        /* Check that it was loaded correctly */
        if(session.IsNull())
            throw APIException(-309, "Error loading session.");

        /* Iterate our processed vector. */
        std::vector<uint512_t> vProcessed = *session.vProcessed; //make a copy by dereferencing
        for(const auto& hashTx : vProcessed)
        {
            /* Add the txid in, we will want to support verbose settings later. */
            json::json jObj;
            jObj["txid"] = hashTx.ToString();

            /* Push to our return object. */
            jRet.push_back(jObj);
        }

        return jRet;
    }
}
