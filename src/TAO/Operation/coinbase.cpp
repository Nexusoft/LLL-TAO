/*__________________________________________________________________________________________

        Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

        (c) Copyright The Nexus Developers 2014 - 2025

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/coinbase.h>
#include <TAO/Operation/include/enum.h>

#include <Util/include/debug.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Coinbase::Commit(const uint256_t& hashGenesis, const uint64_t /*nAmount*/, const uint512_t& hashTx,
                              const uint8_t nFlags)
        {
            /* EVENTS DISABLED for -client mode. */
            if(!config::fClient.load())
            {
                /* Check to contract caller. */
                if(nFlags == TAO::Ledger::FLAGS::BLOCK)
                {
                    /* Write the event to the database. */
                    if(!LLD::Ledger->WriteEvent(hashGenesis, hashTx))
                        return debug::error(FUNCTION, "OP::COINBASE: failed to write event for coinbase genesis ",
                                            hashGenesis.SubString());
                }
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

            /* Enforce legacy 49-byte coinbase layout: OP(1) + genesis(32) + amount(8) + extra_nonce(8). */
            if(contract.Operations().size() != 49)
            {
                return debug::error(FUNCTION, "invalid coinbase operation size: ",
                                    contract.Operations().size());
            }

            /* Extract the address from contract. */
            uint256_t hashGenesis;
            contract >> hashGenesis;

            /* Check for valid genesis. */
            if(hashGenesis.GetType() != TAO::Ledger::GENESIS::UserType())
            {
                return debug::error(FUNCTION, "invalid genesis for coinbase: type byte 0x",
                                    std::hex, static_cast<int>(hashGenesis.GetType()), std::dec,
                                    " expected 0x",
                                    std::hex, static_cast<int>(TAO::Ledger::GENESIS::UserType()), std::dec,
                                    " — coinbase recipient must be a TritiumGenesis sigchain address,",
                                    " not a Register Address");
            }

            /* Seek read position to first position after OP byte. */
            contract.Rewind(32, Contract::OPERATIONS);

            return true;
        }
    }
}
