/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/finance/types/finance.h>
#include <TAO/API/objects/types/objects.h>
#include <TAO/API/include/global.h>

#include <TAO/API/include/get.h>
#include <TAO/API/include/list.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/transaction.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/types/object.h>

#include <Util/include/debug.h>
#include <Util/include/math.h>


/* Global TAO namespace. */
namespace TAO::API
{
    /* Lists all transactions for a given account. */
    json::json Tokens::ListTransactions(const json::json& params, bool fHelp)
    {
        /* The account to list transactions for. */
        TAO::Register::Address hashAccount;

        /* If name is provided then use this to deduce the register address,
         * otherwise try to find the raw hex encoded address. */
        if(params.find("name") != params.end() && !params["name"].get<std::string>().empty())
            hashAccount = Names::ResolveAddress(params, params["name"].get<std::string>());
        else if(params.find("address") != params.end())
            hashAccount.SetBase58(params["address"].get<std::string>());
        else
            throw APIException(-33, "Missing name or address");

        /* Get the account object. */
        TAO::Register::Object object;
        if(!LLD::Register->ReadState(hashAccount, object, TAO::Ledger::FLAGS::MEMPOOL))
        {
            throw APIException(-125, "Token not found");
        }

        /* Parse the object register. */
        if(!object.Parse())
            throw APIException(-14, "Object failed to parse");

        /* Get the object standard. */
        uint8_t nStandard = object.Standard();

        /* Check the object standard. */
        if(nStandard != TAO::Register::OBJECTS::TOKEN)
        {
            throw APIException(-124, "Unknown token / account.");
        }

        return Objects::ListTransactions(params, fHelp);
    }
}
