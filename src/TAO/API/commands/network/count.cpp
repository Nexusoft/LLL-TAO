/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLP/include/global.h>

#include <TAO/API/types/commands/network.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/include/extract.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Lists the current logged in sessions for -multiusername mode. */
    encoding::json Network::Count(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the number of days to check back. */
        const uint64_t nDays =
            ExtractInteger<uint64_t>(jParams, "days", 7, 365);

        /* Get the number of hours to check back. */
        const uint64_t nHours =
            ExtractInteger<uint64_t>(jParams, "hours", 0, 24);

        /* Get addresses from manager. */
        std::vector<LLP::TrustAddress> vAddr;
        if(LLP::TRITIUM_SERVER->GetAddressManager())
            LLP::TRITIUM_SERVER->GetAddressManager()->GetAddresses(vAddr, CONNECT_FLAGS_ALL);

        /* Build our return JSON data. */
        uint64_t nCount = 0;
        for(const auto& rAddr : vAddr)
        {
            /* Check that it has been seen within one week. */
            if(rAddr.nLastSeen + (86400 * nDays) + (3600 * nHours) > runtime::unifiedtimestamp())
                ++nCount;
        }

        /* Build our return JSON. */
        const encoding::json jRet =
            {{ "count", nCount }};

        return jRet;
    }
}
