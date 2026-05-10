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
#include <TAO/Register/types/address.h>

#include <Util/include/debug.h>
#include <Util/include/runtime.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {
        namespace
        {
            constexpr size_t COINBASE_LEGACY_OPERATION_SIZE = 1 + 32 + 8 + 8;
            constexpr size_t COINBASE_AUTOCREDIT_OPERATION_SIZE = 1 + 32 + 32 + 8 + 8;
        }


        /* Check for extended auto-credit account payload. */
        bool Coinbase::HasAutoCreditAccount(const Contract& contract)
        {
            return contract.Operations().size() == COINBASE_AUTOCREDIT_OPERATION_SIZE;
        }


        /* Commit the final state to disk. */
        bool Coinbase::Commit(const uint256_t& hashGenesis, const uint64_t nAmount, const uint512_t& hashTx,
                              const uint8_t nFlags, const uint256_t& hashAccount)
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

                    /* Legacy coinbase payloads carry only the sigchain genesis.
                     * Those rewards are intentionally event-only and are claimed
                     * by the recipient sigchain's default account when events are
                     * processed. */
                    if(hashAccount == 0 || !LLP::GenesisConstants::IsAutoCreditEnabled())
                        return true;

                    TAO::Register::Address hashRewardAccount = hashAccount;
                    if(!hashRewardAccount.IsAccount())
                    {
                        debug::warning(FUNCTION, "AUTO-CREDIT skipped: recipient account ",
                                       hashRewardAccount.SubString(), " is not an account register");
                        return true;
                    }

                    /* Read the account state. */
                    TAO::Register::Object account;
                    if(!LLD::Register->ReadState(hashRewardAccount, account, nFlags))
                    {
                        debug::warning(FUNCTION, "AUTO-CREDIT skipped: cannot read reward account state for ",
                                      hashRewardAccount.SubString(),
                                      ". Falling back to event-only mode.");
                        return true;
                    }

                    /* Parse the account object. */
                    if(!account.Parse() || account.Standard() != TAO::Register::OBJECTS::ACCOUNT)
                    {
                        debug::warning(FUNCTION, "AUTO-CREDIT skipped: reward account object is not a valid account ",
                                       hashRewardAccount.SubString(), ". Falling back to event-only mode.");
                        return true;
                    }

                    /* Direct credit to reward account. */
                    uint64_t nBalance = account.get<uint64_t>("balance");
                    account.Write("balance", nBalance + nAmount);
                    account.nModified = runtime::unifiedtimestamp();
                    account.SetChecksum();

                    /* Write updated account state to register database. */
                    if(!LLD::Register->WriteState(hashRewardAccount, account, nFlags))
                    {
                        debug::warning(FUNCTION, "AUTO-CREDIT skipped: failed to write account state for ",
                                       hashRewardAccount.SubString(), ". Falling back to event-only mode.");
                        return true;
                    }

                    /* Write proof so rollback can distinguish direct-credit from event-only mode. */
                    if(!LLD::Ledger->WriteProof(hashGenesis, hashTx, 0, nFlags))
                    {
                        debug::warning(FUNCTION, "AUTO-CREDIT credited account but failed to write rollback proof");
                        return true;
                    }

                    debug::log(0, FUNCTION, "AUTO-CREDIT ", nAmount, " NXS to ", hashRewardAccount.SubString(),
                              " (owner: ", account.hashOwner.SubString(), ")");
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
            {
                return debug::error(FUNCTION, "invalid genesis for coinbase: type byte 0x",
                                    std::hex, static_cast<int>(hashGenesis.GetType()), std::dec,
                                    " expected 0x",
                                    std::hex, static_cast<int>(TAO::Ledger::GENESIS::UserType()), std::dec,
                                    " — coinbase recipient must be a TritiumGenesis sigchain address,",
                                    " not a Register Address");
            }

            if(HasAutoCreditAccount(contract))
            {
                TAO::Register::Address hashAccount;
                contract >> hashAccount;

                if(!hashAccount.IsAccount())
                    return debug::error(FUNCTION, "invalid coinbase auto-credit account: type byte 0x",
                                        std::hex, static_cast<int>(hashAccount.GetType()), std::dec,
                                        " expected account register type 0x",
                                        std::hex, static_cast<int>(TAO::Register::Address::ACCOUNT), std::dec);
            }
            else if(contract.Operations().size() != COINBASE_LEGACY_OPERATION_SIZE)
            {
                return debug::error(FUNCTION, "invalid coinbase operation size: ",
                                    contract.Operations().size());
            }

            /* Seek read position to first position after OP byte. */
            contract.Rewind(HasAutoCreditAccount(contract) ? 64 : 32, Contract::OPERATIONS);

            return true;
        }
    }
}
