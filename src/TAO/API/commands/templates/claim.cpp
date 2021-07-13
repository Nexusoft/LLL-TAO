/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/commands/templates.h>

#include <TAO/Operation/types/contract.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Claim an incoming transfer from recipient. */
    encoding::json Templates::Claim(const encoding::json& jParams, const bool fHelp)
    {
        /* Check for txid parameter. */
        if(jParams.find("txid") == jParams.end())
            throw Exception(-50, "Missing txid.");

        /* Get the transaction id. */
        const uint512_t hashTx =
            uint512_t(jParams["txid"].get<std::string>());

        /* Check for tritium credits. */
        std::vector<TAO::Operation::Contract> vContracts(0);
        if(!BuildContracts(jParams, hashTx, vContracts, BuildClaim))
            throw Exception(-43, "No valid contracts in debit tx.");

        /* Build response JSON boilerplate. */
        return BuildResponse(jParams, vContracts);
    }
}
