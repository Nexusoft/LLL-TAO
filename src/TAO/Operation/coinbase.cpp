/*__________________________________________________________________________________________

        Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014]++

        (c) Copyright The Nexus Developers 2014 - 2025

        Distributed under the MIT software license, see the accompanying
        file COPYING or http://www.opensource.org/licenses/mit-license.php.

        "Doubt is the precursor to fear" - Alex Hannold

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <LLP/include/genesis_constants.h>
#include <LLP/include/stateless_manager.h>

#include <TAO/Operation/include/coinbase.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Register/include/enum.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {
        /* Commit the final state to disk. */
        bool Coinbase::Commit(const uint256_t& hashGenesis, const uint64_t nAmount, const uint512_t& hashTx, const uint8_t nFlags)
        {
            /* EVENTS DISABLED for -client mode. */
            if(!config::fClient.load())
            {
                /* Check to contract caller. */
                if(nFlags == TAO::Ledger::FLAGS::BLOCK)
                {
                    /* Write the event to the database. */
                    if(!LLD::Ledger->WriteEvent(hashGenesis, hashTx))
                        return debug::error(FUNCTION, "OP::COINBASE: failed to write event for coinbase");

                    /* AUTO-CREDIT LOGIC - Direct reward routing
                     * With the new Direct Reward Address system, hashGenesis is the reward address
                     * provided by the miner via MINER_SET_REWARD (encrypted with ChaCha20).
                     * The reward address is already validated as a valid NXS account. */
                    if(LLP::GenesisConstants::IsAutoCreditEnabled())
                    {
                        /* Use hashGenesis directly as the reward account address.
                         * In the new system, miners provide account addresses, not genesis hashes. */
                        TAO::Register::Address hashRewardAccount = hashGenesis;

                        /* Read the account state */
                        TAO::Register::Object account;
                        if(!LLD::Register->ReadState(hashRewardAccount, account, nFlags))
                        {
                            debug::log(1, FUNCTION, "Failed to read reward account ", hashRewardAccount.SubString(),
                                      ", using event-only mode");
                            return true;  // Fallback to event-only
                        }

                        /* Parse the account object */
                        if(!account.Parse())
                        {
                            debug::log(1, FUNCTION, "Failed to parse reward account object, using event-only mode");
                            return true;  // Fallback to event-only
                        }

                        /* Direct credit to reward account */
                        uint64_t nBalance = account.get<uint64_t>("balance");
                        account.Write("balance", nBalance + nAmount);
                        account.nModified = runtime::unifiedtimestamp();
                        account.SetChecksum();

                        /* Write updated account state to register database */
                        if(!LLD::Register->WriteState(hashRewardAccount, account, nFlags))
                        {
                            debug::log(0, FUNCTION, "Failed to write account state, using event-only mode");
                            return true;  // Fallback to event-only
                        }

                        /* Write proof to prevent double-claim */
                        if(!LLD::Ledger->WriteProof(hashGenesis, hashTx, 0, nFlags))
                        {
                            debug::log(0, FUNCTION, "Failed to write proof, but account credited");
                            return true;  // Account already credited, continue
                        }

                        debug::log(0, FUNCTION, "AUTO-CREDIT ", nAmount, " NXS to ", hashRewardAccount.SubString(),
                                  " (owner: ", account.hashOwner.SubString(), ")");
                    }
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

            /* Extract the address from contract. */
            uint256_t hashGenesis;
            contract >> hashGenesis;

            /* Check for valid genesis. */
            if(hashGenesis.GetType() != TAO::Ledger::GENESIS::UserType())
                return debug::error(FUNCTION, "invalid genesis for coinbase");

            /* Seek read position to first position. */
            contract.Rewind(32, Contract::OPERATIONS);

            return true;
        }
    }
}
