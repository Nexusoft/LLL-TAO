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

        /* Extract our verbose parameter. */
        const uint32_t  nVerbose =
            ExtractVerbose(jParams);

        /* Number of results to return. */
        uint32_t nLimit = 100, nOffset = 0;

        /* Get the parameters to apply to the response. */
        std::string strOrder = "desc";
        ExtractList(jParams, strOrder, nLimit, nOffset);

        /* JSON return value. */
        encoding::json jRet =
            encoding::json::array();

        /* List off our events available. */
        std::vector<std::pair<uint512_t, uint32_t>> vEvents;

        /* Grab out list of contracts and events. */
        LLD::Logical->ListEvents(hashGenesis, vEvents);
        LLD::Logical->ListContracts(hashGenesis, vEvents);

        /* Only render if we have events that have been returned. */
        if(!vEvents.empty())
        {
            /* Flip our list if ascending order. */
            if(strOrder == "asc")
                std::reverse(vEvents.begin(), vEvents.end());

            /* List out our active events to process. */
            uint32_t nTotal = 0;
            for(const auto& rEvent : vEvents)
            {
                /* Grab a reference of our hash. */
                const uint512_t& hashEvent = rEvent.first;

                /* Get the transaction from disk. */
                TAO::API::Transaction tx;
                if(!LLD::Ledger->ReadTx(hashEvent, tx))
                    throw Exception(-108, "Failed to read transaction");

                /* Check if contract has been spent. */
                if(tx.Spent(hashEvent, rEvent.second))
                    continue;

                /* Get the transaction JSON. */
                encoding::json jContract =
                    TAO::API::ContractToJSON(tx[rEvent.second], rEvent.second, nVerbose);

                /* Add some items from our transction. */
                jContract["timestamp"] = tx.nTimestamp;
                jContract["txid"]      = hashEvent.ToString();

                /* Check to see whether the transaction has had all children filtered out */
                if(jContract.empty())
                    continue;

                /* Apply our where filters now. */
                if(!FilterResults(jParams, jContract))
                    continue;

                /* Filter out our expected fieldnames if specified. */
                if(!FilterFieldname(jParams, jContract))
                    continue;

                /* Check the offset. */
                if(++nTotal <= nOffset)
                    continue;

                /* Check the limit */
                if(nTotal - nOffset > nLimit)
                    break;

                jRet.push_back(jContract);
            }
        }

        return jRet;
    }
}
