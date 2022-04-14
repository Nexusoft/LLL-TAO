/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/extract.h>
#include <TAO/API/include/global.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>
#include <TAO/API/types/commands/profiles.h>
#include <TAO/API/types/authentication.h>
#include <TAO/API/types/transaction.h>

#include <TAO/Ledger/types/state.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* List the notifications from a particular sigchain. */
    encoding::json Profiles::Notifications(const encoding::json& jParams, const bool fHelp)
    {
        /* Extract input parameters. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the parameters to apply to the response. */
        std::string strOrder = "desc";
        ExtractList(jParams, strOrder, nLimit, nOffset);

        /* JSON return value. */
        encoding::json jRet =
            encoding::json::array();

        return jRet;
    }
}
