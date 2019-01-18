/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_LEDGER_INCLUDE_CONSTANTS_H
#define NEXUS_TAO_LEDGER_INCLUDE_CONSTANTS_H

#include <LLC/types/bignum.h>
#include <Legacy/include/money.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /** Very first block hash in the blockchain. **/
        const uint1024_t hashGenesis("0x00000bb8601315185a0d001e19e63551c34f1b7abeb3445105077522a9cbabf3e1da963e3bfbb87d260218b7c6207cb9c6df90b86e6a3ce84366434763bc8bcbf6ccbd1a7d5958996aecbe8205c20b296818efb3a59c74acbc7a2d1a5a6b64aab63839b8b11a6b41c4992f835cbbc576d338404fb1217bdd7909ca7db63bbc02");


        /** Hash to start the Test Net Blockchain. **/
        const uint1024_t hashGenesisTestnet("0x00002a0ccd35f2e1e9e1c08f5a2109a82834606b121aca891d5862ba12c6987d55d1e789024fcd5b9adaf07f5445d24e78604ea136a0654497ed3db0958d63e72f146fae2794e86323126b8c3d8037b193ce531c909e5222d090099b4d574782d15c135ddd99d183ec14288437563e8a6392f70259e761e62d2ea228977dd2f7");


        /** Minimum channels difficulty. **/
        const LLC::CBigNum bnProofOfWorkLimit[] =
        {
            LLC::CBigNum(~uint1024_t(0) >> 5),
            LLC::CBigNum(20000000),
            LLC::CBigNum(~uint1024_t(0) >> 17)
        };


        /** Starting channels difficulty. **/
        const LLC::CBigNum bnProofOfWorkStart[] =
        {
            LLC::CBigNum(~uint1024_t(0) >> 7),
            LLC::CBigNum(25000000),
            LLC::CBigNum(~uint1024_t(0) >> 22)
        };


        /** Minimum prime zero bits (1016-bits). **/
        const LLC::CBigNum bnPrimeMinOrigins    =   LLC::CBigNum(~uint1024_t(0) >> 8); //minimum prime origins of 1016 bits


        /** Maximum size a block can be in transit. **/
        const uint32_t MAX_BLOCK_SIZE = 1024 * 1024 * 2;


        /** Maximum size a block can be generated as. **/
        const uint32_t MAX_BLOCK_SIZE_GEN = 1000000;


        /** Maximum signature operations per block. **/
        const uint32_t MAX_BLOCK_SIGOPS = 40000;


        /** Nexus Coinbase/Coinstake Maturity Settings **/
        const uint32_t TESTNET_MATURITY_BLOCKS = 100;


        /** Mainnet maturity for blocks. */
        const uint32_t NEXUS_MATURITY_BLOCKS   = 120;


        /** nVersion 4 and earlier trust keys expire after 24 hours. **/
        const uint32_t TRUST_KEY_EXPIRE   = 60 * 60 * 24;


        /** nVersion > 4 - timespan is 3 days, 30 minutes for testnet **/
        const uint32_t TRUST_KEY_TIMESPAN = 60 * 60 * 24 * 3;


        /** Timestamp of trust key for testnet. **/
        const uint32_t TRUST_KEY_TIMESPAN_TESTNET = 60 * 30;


        /** Minimum span between trust blocks testnet. **/
        const uint32_t TESTNET_MINIMUM_INTERVAL = 3;


        /** Minimum span between trust blocks mainnet. **/
        const uint32_t MAINNET_MINIMUM_INTERVAL = 120;


        /** Set the Maximum Output Value of Coinstake Transaction. **/
        const uint64_t MAX_STAKE_WEIGHT = 1000 * Legacy::COIN;
    }
}


#endif
