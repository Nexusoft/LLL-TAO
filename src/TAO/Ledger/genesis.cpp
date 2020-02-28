/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/Ledger/types/genesis.h>
#include <TAO/Ledger/include/enum.h>

#include <Util/include/args.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* Default constructor. */
        Genesis::Genesis()
        : uint256_t(0)
        {
        }


        /* Copy Constructor */
        Genesis::Genesis(const uint256_t& value, bool fSet)
        : uint256_t(value)
        {
            if(fSet)
                SetType(config::fTestNet.load() ? GENESIS::TESTNET : GENESIS::MAINNET);
        }


        /* Move Constructor */
        Genesis::Genesis(uint256_t&& value) noexcept
        : uint256_t(std::move(value))
        {
        }


        /* Copy Assignment operator. */
        Genesis& Genesis::operator=(const uint256_t& value)
        {
            for(uint8_t i = 0; i < WIDTH; ++i)
                pn[i] = value.pn[i];

            return *this;
        }


        /* Move Assignment operator. */
        Genesis& Genesis::operator=(uint256_t&& value) noexcept
        {
            for(uint8_t i = 0; i < WIDTH; ++i)
                pn[i] = std::move(value.pn[i]);

            return *this;
        }


        /* Check if genesis has a valid indicator byte.*/
        bool Genesis::IsValid() const
        {
            return GetType() == TAO::Ledger::GenesisType();
        }
    }
}
