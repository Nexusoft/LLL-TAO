/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/coinbase.h>
#include <TAO/Operation/include/enum.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Coinbase::Commit(const uint256_t& hashAddress, const uint512_t& hashTx, const uint8_t nFlags)
        {
            /* Check to contract caller. */
            if(nFlags == TAO::Ledger::FLAGS::BLOCK)
            {
                /* Write the event to the database. */
                if(!LLD::Ledger->WriteEvent(hashAddress, hashTx))
                    return debug::error(FUNCTION, "OP::COINBASE: failed to write event for coinbase");
            }

            return true;
        }


        /* Verify append validation rules and caller. */
        bool Coinbase::Verify(const Contract& contract)
        {
            /* Rewind back on byte. */
            contract.Rewind(1, Contract::OPERATIONS);

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::COINBASE)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Extract the address from contract. */
            uint256_t hashGenesis;
            contract >> hashGenesis;

            /* Check for valid genesis. */
            if(hashGenesis.GetType() != (config::fTestNet.load() ? 0xa2 : 0xa1))
                return debug::error(FUNCTION, "invalid genesis for coinbase");

            /* Seek read position to first position. */
            contract.Rewind(32, Contract::OPERATIONS);


            return true;
        }
    }
}
