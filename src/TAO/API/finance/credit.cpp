/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>
#include <LLC/hash/SK.h>

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>

#include <TAO/API/finance/types/finance.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/execute.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/names.h>
#include <TAO/Register/include/constants.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>

#include <Util/templates/datastream.h>

#include <Legacy/include/evaluate.h>
#include <Legacy/include/trust.h>
#include <Legacy/types/transaction.h>
#include <Legacy/types/trustkey.h>

/* Global TAO namespace. */
namespace TAO::API
{
    json::json Finance::Credit(const json::json& jParams, const bool fHelp)
    {
        /* Check for txid parameter. */
        if(jParams.find("txid") == jParams.end())
            throw APIException(-50, "Missing txid.");

        /* Extract some parameters from input data. */
        const TAO::Register::Address hashCredit = ExtractAddress(jParams, "", "default");

        /* Get the transaction id. */
        const uint512_t hashTx =
            uint512_t(jParams["txid"].get<std::string>());

        /* Check for tritium credits. */
        std::vector<TAO::Operation::Contract> vContracts(0);
        if(!BuildCredits(jParams, hashTx, vContracts))
            throw APIException(-43, "No valid contracts in debit tx.");

        /* Build response JSON boilerplate. */
        return BuildResponse(jParams, hashCredit, vContracts);
    }
}
