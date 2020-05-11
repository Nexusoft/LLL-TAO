/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/migrate.h>
#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/types/contract.h>

#include <TAO/Register/include/constants.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/reserved.h>
#include <TAO/Register/types/object.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commit the final state to disk. */
        bool Migrate::Commit(const TAO::Register::Object& trust, const uint256_t& hashAddress,
                             const uint256_t& hashCaller, const uint512_t& hashTx, const uint576_t& hashTrust,
                             const uint512_t& hashLast, const uint8_t nFlags)
        {
            /* Check if this transfer is already claimed. Trust migration is always a send from a Legacy trust key,
             * so the hashProof is always the wildcard address. Also only one output UTXO sending to trust account
             * and thus the contract number is always zero.
             */
            if(LLD::Ledger->HasProof(TAO::Register::WILDCARD_ADDRESS, hashTx, 0, nFlags))
                return debug::error(FUNCTION, "migrate credit is already claimed");

            /* Write the claimed proof. */
            if(!LLD::Ledger->WriteProof(TAO::Register::WILDCARD_ADDRESS, hashTx, 0, nFlags))
                return debug::error(FUNCTION, "failed to write migrate credit proof");

            /* Write the new register's state. */
            if(!LLD::Register->WriteState(hashAddress, trust, nFlags))
                return debug::error(FUNCTION, "failed to write post-state to disk");

            /* Migrate operation can be executed on mempool accept, so only write trust/stake items for BLOCK flag */
            if(nFlags == TAO::Ledger::FLAGS::BLOCK)
            {
                /* Update the register database to index the trust account. (migrated trust account is post-Genesis) */
                if(!LLD::Register->IndexTrust(hashCaller, hashAddress))
                    return debug::error(FUNCTION, "could not index the trust account genesis");

                /* Set hash last trust for trust account to hash last trust for Legacy trust key. */
                if(!LLD::Ledger->WriteStake(hashCaller, hashLast))
                    return debug::error(FUNCTION, "failed to write last trust to disk");

                /* Record that the legacy trust key has completed migration. */
                if(!config::fClient.load() && !LLD::Legacy->WriteTrustConversion(hashTrust))
                    return debug::error(FUNCTION, "failed to record trust key migration to disk");
            }

            return true;
        }


        /* Migrate trust key data to trust account register. */
        bool Migrate::Execute(TAO::Register::Object &trust, const uint64_t nAmount,
                              const uint32_t nScore, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!trust.Parse())
                return debug::error(FUNCTION, "failed to parse account object register");

            /* Check migrating to a trust account register. */
            if(trust.Standard() != TAO::Register::OBJECTS::TRUST)
                return debug::error(FUNCTION, "cannot migrate to a non-trust account");

            /* Check that there is no stake. */
            if(trust.get<uint64_t>("stake") != 0)
                return debug::error(FUNCTION, "cannot migrate with already existing stake");

            /* Check that there is no trust. */
            if(trust.get<uint64_t>("trust") != 0)
                return debug::error(FUNCTION, "cannot migrate with already existing trust");

            /* Write the migrated stake to trust account register. */
            if(!trust.Write("stake", nAmount))
                return debug::error(FUNCTION, "stake could not be written to object register");

            /* Write the migrated trust to trust account register. Also converts old trust score from uint32_t to uint64_t */
            if(!trust.Write("trust", static_cast<uint64_t>(nScore)))
                return debug::error(FUNCTION, "trust could not be written to object register");

            /* Update the state register's timestamp. */
            trust.nModified = nTimestamp;
            trust.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!trust.IsValid())
                return debug::error(FUNCTION, "trust account is in invalid state");

            return true;
        }


        /* Verify trust migration rules. */
        bool Migrate::Verify(const Contract& contract, const Contract& debit)
        {
            /* Reset the contract streams. */
            contract.Reset();

            /* Get operation byte. */
            uint8_t OP;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::MIGRATE)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Check transaction version. */
            if(contract.Version() > 1)
                return debug::error(FUNCTION, "OP::MIGRATE: invalid transaction version");

            /* Check for conditions. */
            if(!debit.Empty(Contract::CONDITIONS))
                return debug::error(FUNCTION, "OP::MIGRATE: conditions not allowed on migrate debit");

            contract.Seek(64);

            /* Get the trust register address. (hash to) */
            uint256_t hashAccount;
            contract >> hashAccount;

            /* Get the trust key hash. (hash from) */
            uint576_t hashTrust;
            contract >> hashTrust;

            /* Get the amount to migrate. */
            uint64_t nAmount;
            contract >> nAmount;

            /* Get the trust score to migrate. */
            uint32_t nScore;
            contract >> nScore;

            /* Get the hash last stake. */
            uint512_t hashLast;
            contract >> hashLast;

            /* Get the byte from pre-state. */
            uint8_t nState;
            contract >>= nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register contract not in pre-state");

            /* Read pre-states. */
            TAO::Register::Object trust;
            contract >>= trust;

            /* Check contract account */
            if(contract.Caller() != trust.hashOwner)
                return debug::error(FUNCTION, "no write permissions for caller ", contract.Caller().SubString());

            /* Parse the account. */
            if(!trust.Parse())
                return debug::error(FUNCTION, "failed to parse account");

            /* Check whether a trust account Genesis already indexed. */
            if(LLD::Register->HasTrust(contract.Caller()))
                return debug::error(FUNCTION, "trust account is not new");

            /* Reset debit streams */
            debit.Reset();

            /* Get operation byte. */
            OP = 0;
            debit >> OP;

            /* Check that prev is debit. */
            if(OP != OP::DEBIT)
                return debug::error(FUNCTION, "tx claim is not a debit");

            /* Get the hashFrom */
            uint256_t hashFrom;
            debit >> hashFrom;

            /* Get the hashTo. */
            uint256_t hashTo;
            debit >> hashTo;

            /* Get the debit amount. */
            uint64_t nDebit;
            debit >> nDebit;

            /* Skip placeholder */
            debit.Seek(8);

            /* Get the debit trust score */
            uint32_t nScoreDebit;
            debit >> nScoreDebit;

            /* Get the debit last stake hash */
            uint512_t hashLastDebit;
            debit >> hashLastDebit;

            /* Get the trust key hash */
            uint576_t hashTrustDebit;
            debit >> hashTrustDebit;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashTo))
                return debug::error(FUNCTION, "cannot credit register with reserved address");

            /* Migrate should always have wildcard as hashFrom because it is from UTXO. */
            if(hashFrom != TAO::Register::WILDCARD_ADDRESS)
                return debug::error(FUNCTION, "migrate debit register must be from UTXO");

            /* Check whether the legacy trust key has already completed migration. */
            if(!config::fClient.load() && LLD::Legacy->HasTrustConversion(hashTrust))
                return debug::error(FUNCTION, "trust key is already converted");

            /* Validate migrate is to address in UTXO output */
            if(hashTo != hashAccount)
                return debug::error(FUNCTION, "trust account register address must match debit");

            /* Check the debit amount. */
            if(nDebit != nAmount)
                return debug::error(FUNCTION, "debit and credit value mismatch");

            /* Verify the trust score */
            if(nScoreDebit != nScore)
                return debug::error(FUNCTION, "debit and credit trust score mismatch");

            /* Verify the hash last stake */
            if(hashLastDebit != hashLast)
                return debug::error(FUNCTION, "debit and credit hash last stake mismatch");

            /* Verify the trust key hash */
            if(hashTrustDebit != hashTrust)
                return debug::error(FUNCTION, "debit and credit trust key mismatch");

            return true;
        }
    }
}
