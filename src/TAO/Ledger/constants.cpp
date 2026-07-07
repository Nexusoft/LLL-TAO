/*__________________________________________________________________________________________

        Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

        (c) Copyright The Nexus Developers 2014 - 2025

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/include/chainstate.h>
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
        /** Hash to start a hybrid Blockchain. **/
        uint1024_t hashGenesisHybrid;


        /* Retrieve the number of blocks (confirmations) required for coinbase maturity for a given block. */
        uint32_t MaturityCoinBase(const BlockState& block)
        {
            if(config::fTestNet)
                return TESTNET_MATURITY_BLOCKS;

            /* Apply legacy interval for all versions prior to version 7.  If the caller is not able to provide a block to base
               this calculation off, then we will use the tStateBest instead */
            if((!block.IsNull() ? block.nVersion : TAO::Ledger::ChainState::tStateBest.load().nVersion) < 7 )
                return NEXUS_MATURITY_LEGACY;

            return NEXUS_MATURITY_COINBASE;
        }


        /* Retrieve the number of blocks (confirmations) required for coinstake maturity for a given block. */
        uint32_t MaturityCoinStake(const BlockState& block)
        {
            if(config::fTestNet)
                return TESTNET_MATURITY_BLOCKS;

            /* Apply legacy interval for all versions prior to version 7.  If the caller is not able to provide a block to base
               this calculation off, then we will use the tStateBest instead */
            if((!block.IsNull() ? block.nVersion : TAO::Ledger::ChainState::tStateBest.load().nVersion) < 7 )
                return NEXUS_MATURITY_LEGACY;

            return NEXUS_MATURITY_COINSTAKE;
        }


        /* [Option B] Retrieve the relaxed coinbase maturity requirement used for
         * mempool admission policy only (never for consensus or templates). */
        uint32_t MaturityCoinBaseMempool(const BlockState& block)
        {
            /* Start from the strict consensus requirement. */
            const uint32_t nMaturity = MaturityCoinBase(block);

            /* Apply the grace window, floored at 1 so a 0-confirmation coinbase
             * spend can never be admitted to the mempool. */
            if(nMaturity <= MEMPOOL_MATURITY_GRACE + 1)
                return 1;

            return nMaturity - MEMPOOL_MATURITY_GRACE;
        }
    }
}
