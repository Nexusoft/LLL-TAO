/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/constants.h>

#include <TAO/Ledger/include/timelocks.h>

#include <Util/include/args.h>
#include <Util/include/runtime.h>


/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {
        /* Retrieve the number of blocks (confirmations) required for coinbase maturity for the current network version. */
        uint32_t MaturityCoinBase()
        {
            if(config::fTestNet)
                return TESTNET_MATURITY_BLOCKS;

            int32_t nCurrent = CurrentVersion();

            /* Apply legacy maturity for all versions prior to version 7 */
            if(nCurrent < 7)
                return NEXUS_MATURITY_LEGACY;

            /* Legacy maturity until version 7 is active */
            if(nCurrent == 7 && !VersionActive(runtime::unifiedtimestamp(), 7))
                return NEXUS_MATURITY_LEGACY;

            return NEXUS_MATURITY_COINBASE;
        }


        /* Retrieve the number of blocks (confirmations) required for coinbase maturity for a given block. */
        uint32_t MaturityCoinBase(const BlockState& block)
        {
            if(config::fTestNet)
                return TESTNET_MATURITY_BLOCKS;

            /* Apply legacy interval for all versions prior to version 7 */
            if(block.nVersion < 7)
                return NEXUS_MATURITY_LEGACY;

            return NEXUS_MATURITY_COINBASE;
        }


        /* Retrieve the number of blocks (confirmations) required for coinstake maturity for the current network version. */
        uint32_t MaturityCoinStake()
        {
            if(config::fTestNet)
                return TESTNET_MATURITY_BLOCKS;

            int32_t nCurrent = CurrentVersion();

            /* Apply legacy maturity for all versions prior to version 7 */
            if(nCurrent < 7)
                return NEXUS_MATURITY_LEGACY;

            /* Legacy maturity until version 7 is active */
            if(nCurrent == 7 && !VersionActive(runtime::unifiedtimestamp(), 7))
                return NEXUS_MATURITY_LEGACY;

            return NEXUS_MATURITY_COINSTAKE;
        }


        /* Retrieve the number of blocks (confirmations) required for coinstake maturity for a given block. */
        uint32_t MaturityCoinStake(const BlockState& block)
        {
            if(config::fTestNet)
                return TESTNET_MATURITY_BLOCKS;

            /* Apply legacy interval for all versions prior to version 7 */
            if(block.nVersion < 7)
                return NEXUS_MATURITY_LEGACY;

            return NEXUS_MATURITY_COINSTAKE;
        }

    }
}
