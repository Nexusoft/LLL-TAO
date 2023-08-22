/*__________________________________________________________________________________________

            Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

            (c) Copyright The Nexus Developers 2014 - 2023

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/include/build.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/types/commands.h>
#include <TAO/API/types/commands/finance.h>

#include <TAO/Operation/types/contract.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Reverses a debit or transfer transaction that the caller has made */
    encoding::json Finance::Void(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the transaction id. */
        const uint512_t hashTx =
            ExtractHash(jParams);

        /* Check for tritium credits. */
        std::vector<TAO::Operation::Contract> vContracts(0);
        if(!BuildContracts(jParams, hashTx, vContracts, BuildVoid))
            throw Exception(-43, "No valid contracts in tx.");

        /* Build response JSON boilerplate. */
        return BuildResponse(jParams, vContracts);
    }
}
