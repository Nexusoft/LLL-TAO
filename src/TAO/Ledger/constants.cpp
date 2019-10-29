/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

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
        /* Retrieve the number of blocks (confirmations) required for coinbase maturity for a given block. */
        uint32_t MaturityCoinBase(const BlockState& block)
        {
            if(config::fTestNet)
                return TESTNET_MATURITY_BLOCKS;

            /* Apply legacy interval for all versions prior to version 7.  If the caller is not able to provide a block to base
               this calculation off, then we will use the stateBest instead */
            if((!block.IsNull() ? block.nVersion : TAO::Ledger::ChainState::stateBest.load().nVersion) < 7 )
                return NEXUS_MATURITY_LEGACY;

            return NEXUS_MATURITY_COINBASE;
        }


        /* Retrieve the number of blocks (confirmations) required for coinstake maturity for a given block. */
        uint32_t MaturityCoinStake(const BlockState& block)
        {
            if(config::fTestNet)
                return TESTNET_MATURITY_BLOCKS;

            /* Apply legacy interval for all versions prior to version 7.  If the caller is not able to provide a block to base
               this calculation off, then we will use the stateBest instead */
            if((!block.IsNull() ? block.nVersion : TAO::Ledger::ChainState::stateBest.load().nVersion) < 7 )
                return NEXUS_MATURITY_LEGACY;

            return NEXUS_MATURITY_COINSTAKE;
        }

    }
}
