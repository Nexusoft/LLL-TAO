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
        bool Migrate::Commit(const TAO::Register::Object& account, const TAO::Register::Address& hashAddress,
                             const uint256_t& hashCaller, const uint512_t& hashTx, const uint576_t& hashKey,
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
            if(!LLD::Register->WriteState(hashAddress, account, nFlags))
                return debug::error(FUNCTION, "failed to write post-state to disk");

            /* Set hash last trust for trust account to hash last trust for Legacy trust key. */
            if(!LLD::Ledger->WriteStake(hashCaller, hashLast))
                return debug::error(FUNCTION, "failed to write last trust to disk");

            /* Record that the legacy trust key has completed migration. */
            if(!LLD::Legacy->WriteTrustConversion(hashKey))
                return debug::error(FUNCTION, "failed to record trust key migration to disk");

            return true;
        }


        /* Migrate trust key data to trust account register. */
        bool Migrate::Execute(TAO::Register::Object &account, const uint64_t nAmount,
                              const uint32_t nScore, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account object register");

            /* Check migrating to a trust account register. */
            if(account.Standard() != TAO::Register::OBJECTS::TRUST)
                return debug::error(FUNCTION, "cannot migrate to a non-trust account");

            /* Write the migrated stake to trust account register. */
            if(!account.Write("stake", nAmount))
                return debug::error(FUNCTION, "stake could not be written to trust account register");

            /* Write the migrated trust to trust account register. Also converts old trust score from uint32_t to uint64_t */
            if(!account.Write("trust", static_cast<uint64_t>(nScore)))
                return debug::error(FUNCTION, "trust could not be written to trust account register");

            /* Update the state register's timestamp. */
            account.nModified = nTimestamp;
            account.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!account.IsValid())
                return debug::error(FUNCTION, "memory address is in invalid state");

            return true;
        }


        /* Verify trust migration rules. */
        bool Migrate::Verify(const Contract& contract, const Contract& debit)
        {
            /* Reset the contract streams. */
            contract.Reset();

            /* Get operation byte. */
            uint8_t OP = 0;
            contract >> OP;

            /* Check operation byte. */
            if(OP != OP::MIGRATE)
                return debug::error(FUNCTION, "called with incorrect OP");

            /* Check for conditions. */
            if(!debit.Empty(Contract::CONDITIONS))
                return debug::error(FUNCTION, "OP::MIGRATE: conditions not allowed on migrate debit");

            contract.Seek(64);

            /* Get the trust register address. (hash to) */
            TAO::Register::Address hashAccount;
            contract >> hashAccount;

            /* Get the trust key hash. (hash from) */
            uint576_t hashKey;
            contract >> hashKey;

            /* Get the amount to migrate. */
            uint64_t nAmount = 0;
            contract >> nAmount;

            /* Get the trust score to migrate. */
            uint32_t nScore = 0;
            contract >> nScore;

            /* Get the hash last stake. */
            uint512_t hashLast = 0;
            contract >> hashLast;

            /* Get the byte from pre-state. */
            uint8_t nState = 0;
            contract >>= nState;

            /* Check for the pre-state. */
            if(nState != TAO::Register::STATES::PRESTATE)
                return debug::error(FUNCTION, "register contract not in pre-state");

            /* Read pre-states. */
            TAO::Register::Object account;
            contract >>= account;

            /* Check contract account */
            if(contract.Caller() != account.hashOwner)
                return debug::error(FUNCTION, "no write permissions for caller ", contract.Caller().SubString());

            /* Parse the account. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account");

            /* Reset debit streams */
            debit.Reset();

            /* Get operation byte. */
            OP = 0;
            debit >> OP;

            /* Check that prev is debit. */
            if(OP != OP::DEBIT)
                return debug::error(FUNCTION, "tx claim is not a debit");

            /* Get the hashFrom */
            TAO::Register::Address hashFrom;
            debit >> hashFrom;

            /* Get the hashTo. */
            TAO::Register::Address hashTo;
            debit >> hashTo;

            /* Get the debit amount. */
            uint64_t nDebit = 0;
            debit >> nDebit;

            /* Skip placeholder */
            debit.Seek(8);

            /* Get the debit trust score */
            uint32_t nScoreDebit = 0;
            contract >> nScoreDebit;

            /* Get the debit last stake hash */
            uint512_t hashLastDebit = 0;
            contract >> hashLastDebit;

            /* Get the trust key hash */
            uint576_t hashKeyDebit;
            contract >> hashKeyDebit;

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashTo))
                return debug::error(FUNCTION, "cannot credit register with reserved address");

            /* Migrate should always have wildcard as hashFrom because it is from UTXO. */
            if(hashFrom != TAO::Register::WILDCARD_ADDRESS)
                return debug::error(FUNCTION, "migrate debit register must be from UTXO");

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
            if(hashKeyDebit != hashKey)
                return debug::error(FUNCTION, "debit and credit trust key mismatch");

            return true;
        }
    }
}
