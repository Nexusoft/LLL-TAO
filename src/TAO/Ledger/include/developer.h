/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_DEVELOPER_H
#define NEXUS_TAO_LEDGER_INCLUDE_DEVELOPER_H

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Developer Sigchains **/
        const std::map<uint256_t, std::pair<uint256_t, uint32_t>> DEVELOPER =
        {
            {
                uint256_t("a10963c98e8996c70f470cdeb660ee98e233fda957439c81b19d9cb9ec5a6611"),
                {
                    uint256_t("fe71f627fef7b25605873fa7dc7c3fe8d33c0e90dbf01dfb10fd7c1809296588"), //password authentication
                    1000 //55%
                }
            }
        };


        /** Developer payout threshold. **/
        const uint16_t DEVELOPER_PAYOUT_THRESHOLD = 1000;


        /** Ambassador Sigchain for Testnet **/
        const std::map<uint256_t, std::pair<uint256_t, uint32_t>> DEVELOPER_TESTNET =
        {
            {
                uint256_t("a20963c98e8996c70f470cdeb660ee98e233fda957439c81b19d9cb9ec5a6611"),
                {
                    uint256_t("4f816e60e8161564d02f5e1ccc252db857a0121a248bd263559672cf843547d6"), //password authentication
                    1000 //100%
                }
            }
        };


        /** Developer payout threshold. **/
        const uint16_t DEVELOPER_PAYOUT_THRESHOLD_TESTNET = 10;


        /** Developer
         *
         *  Grab the developer object based on version and testnet.
         *
         *  @param[in] nVersion The block version to check by.
         *
         *  @return a reference of the correct sigchain map.
         *
         **/
        inline const std::map<uint256_t, std::pair<uint256_t, uint32_t>>& Developer(const uint32_t nVersion)
        {
            /* Grab a reference of the ambassador sigchain. */
            const std::map<uint256_t, std::pair<uint256_t, uint32_t>>& mapRet =
                (config::fTestNet.load() ? DEVELOPER_TESTNET : DEVELOPER);

            return mapRet;
        }
    }
}


#endif
