/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Legacy/types/locator.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/types/state.h>

/* Global Legacy namespace. */
namespace Legacy
{

    /* Set a locator from block state. */
    Locator::Locator(const TAO::Ledger::BlockState& state)
    : vHave()
    {
        Set(state);
    }


    /* Set a locator from block hash. */
    Locator::Locator(const uint1024_t hashBlock)
    : vHave()
    {
        vHave.push_back(hashBlock);
        if(hashBlock == (config::fTestNet ? TAO::Ledger::hashGenesisTestnet : TAO::Ledger::hashGenesis))
            return;

        TAO::Ledger::BlockState state;
        if(!LLD::legDB->ReadBlock(hashBlock, state))
        {
            if(hashBlock != TAO::Ledger::ChainState::hashBestChain)
                vHave.push_back(TAO::Ledger::ChainState::hashBestChain);

            return;
        }

        Set(state);
    }


    /* Set a locator object from a block state. */
    void Locator::Set(TAO::Ledger::BlockState state)
    {
        vHave.clear();
        int32_t nStep = 1;

        while (!state.IsNull())
        {
            if (vHave.size() > 26)
                break;

            for (int i = 0; !state.IsNull() && i < nStep; i++)
                state = state.Prev();
            if(vHave.size() > 10)
                nStep = nStep * 2;

            vHave.push_back(state.GetHash());
        }
        vHave.push_back(config::fTestNet ? TAO::Ledger::hashGenesisTestnet :TAO::Ledger::hashGenesis);
    }
}
