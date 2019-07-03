/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LEDGER_INCLUDE_AMBASSADOR_H
#define NEXUS_LEDGER_INCLUDE_AMBASSADOR_H

#include <inttypes.h>

#include <string>
#include <vector>


namespace TAO
{
    namespace Ledger
    {
        /** Addresses of the Exchange Channels. **/
        const std::map<uint256_t, std::tuple<uint256_t, uint256_t, uint32_t>> AMBASSADOR =
        {
            {
                "a1bc1623d785130bd7bd725d523bd2bb755518646c3831ea7cd5f86883845901", //genesis
                "e9bb4ece5f0c0f2b7db943afc815897b69ca2412567b13d2c4b0cef1aeb0bb25", //nexthash
                "9b4ae6de90bf0be7f49d00dff503a0b0a7e85a11091b06861c3255edebe79080", //prevhash
                550
            }
        };
    }
}

#endif
