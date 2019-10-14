/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/locator.h>

#include <LLD/include/global.h>

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/types/state.h>

/* Global Legacy namespace. */
namespace TAO
{
    namespace Ledger
    {

        /* Default constructor. */
        Locator::Locator()
        : vHave ( )
        {
        }


        /* Copy Constructor. **/
        Locator::Locator(const Locator& locator)
        : vHave (locator.vHave)
        {
        }


        /* Move Constructor. */
        Locator::Locator(Locator&& locator) noexcept
        : vHave (std::move(locator.vHave))
        {
        }


        /* Copy Assignment Operator */
        Locator& Locator::operator=(const Locator& locator)
        {
            vHave = locator.vHave;

            return *this;
        }


        /* Move Assignment Operator */
        Locator& Locator::operator=(Locator&& locator) noexcept
        {
            vHave = std::move(locator.vHave);

            return *this;
        }


        /* Destructor. */
        Locator::~Locator()
        {
        }


        /* Set a locator from block state. */
        Locator::Locator(const TAO::Ledger::BlockState& state)
        : vHave()
        {
            Set(state);
        }


        /* Set a locator from block hash. */
        Locator::Locator(const uint1024_t& hashBlock)
        : vHave()
        {
            /* Push back the hash locating from. */
            vHave.push_back(hashBlock);

            /* Return on genesis. */
            if(hashBlock == TAO::Ledger::ChainState::Genesis())
                return;

            /* Attempt to read the block state. */
            TAO::Ledger::BlockState state;
            if(!LLD::Ledger->ReadBlock(hashBlock, state))
                return;

            /* On success, check back the blocks. */
            Set(state);
        }


        /*  Constructor - Set a locator from list of hashes. */
        Locator::Locator(const std::vector<uint1024_t>& vHaveIn)
        : vHave(vHaveIn)
        {
        }


        /* Set the object to null. */
        void Locator::SetNull()
        {
            vHave.clear();
        }


        /* Flag to determine if object is null. */
        bool Locator::IsNull() const
        {
            return vHave.empty();
        }


        /* Set a locator object from a block state. */
        void Locator::Set(const TAO::Ledger::BlockState& state)
        {
            /* Step iterator */
            uint32_t nStep = 1;

            /* Make a copy of the state. */
            TAO::Ledger::BlockState statePrev = state;

            /* Loop back valid blocks. */
            while(!statePrev.IsNull())
            {
                /* Break when locator size is large enough. */
                if(vHave.size() > 20)
                    break;

                /* Loop back the total blocks of step iterator. */
                for(int i = 0; !statePrev.IsNull() && i < nStep; ++i)
                    statePrev = statePrev.Prev();

                /* After 10 blocks, start taking exponential steps back. */
                if(vHave.size() > 10)
                    nStep = nStep * 2;

                /* Push back the current state hash. */
                vHave.push_back(statePrev.GetHash());
            }

            /* Push the genesis. */
            vHave.push_back(TAO::Ledger::ChainState::Genesis());
        }
    }
}
