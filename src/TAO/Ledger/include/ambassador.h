/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_LEDGER_INCLUDE_AMBASSADOR_H
#define NEXUS_TAO_LEDGER_INCLUDE_AMBASSADOR_H

#include <LLC/types/uint1024.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Ambassador Sigchains **/
        const std::map<uint256_t, std::pair<uint256_t, uint32_t>> AMBASSADOR =
        {
            /* United States. */
            {
                uint256_t("a1bc1623d785130bd7bd725d523bd2bb755518646c3831ea7cd5f86883845901"),
                {
                    uint256_t("de468f6b7d6f498544f6b1843941f7f223817a61e0b2c5a4df6b78973b29570c"), //password authentication
                    550 //55%
                }
            },

            /* United Kingdom. */
            {
                uint256_t("a1d216ee832d63a7ab18569d7d0bdc4fd383390af5b99d9de3477ad19081a490"),
                {
                    uint256_t("c52225d0131f5946536425df7ed4181064267053846294a77ceffce8feb2ec8e"), //password authentication
                    225 //22.5%
                }
            },

            /* Australia. */
            {
                uint256_t("a1206f9c4feb88239d60381f62af789febc6e71371cf6700838feb90e76987a3"),
                {
                    uint256_t("6694a7cffc46dedd67c4d9ef814a930ef16da6e0b3185394fb8f8ee4d44fa4e4"), //password authentication
                    225 //22.5%
                }
            }
        };


        /** Ambassador payout threshold. **/
        const uint16_t AMBASSADOR_PAYOUT_THRESHOLD = 100;


        /** Ambassador Sigchain for Testnet **/
        const std::map<uint256_t, std::pair<uint256_t, uint32_t>> AMBASSADOR_TESTNET =
        {
            {
                uint256_t("a2cfad9f505f8166203df2685ee7cb8582be9ae017dcafa4ca4530cd5b4f1dca"),
                {
                    uint256_t("6fcf29db16223b1aaddbbf0ad9f09758f12b2db85b5f1dbff4d0de1dac540fcc"), //password authentication
                    1000 //100%
                }
            }
        };

        /** Ambassador payout threshold. **/
        const uint16_t AMBASSADOR_PAYOUT_THRESHOLD_TESTNET = 5;

    }
}


#endif
