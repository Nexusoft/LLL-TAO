/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>

#include <TAO/Register/types/object.h>
#include <TAO/Register/include/system.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Authorizes funds from an account to an account */
        bool Debit(const uint256_t& hashFrom, const uint256_t& hashTo, const uint64_t nAmount,
                   const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashFrom))
                return debug::error(FUNCTION, "cannot debit from register with reserved address");

            /* Check for debit to and from same account. */
            if(hashFrom == hashTo)
                return debug::error(FUNCTION, "cannot debit to the same address as from");

            /* Read the register from the database. */
            TAO::Register::Object account;

            /* Write pre-states. */
            if((nFlags & TAO::Register::FLAGS::PRESTATE))
            {
                if(!LLD::regDB->ReadState(hashFrom, account, nFlags))
                    return debug::error(FUNCTION, "register address doesn't exist ", hashFrom.ToString());

                tx.ssRegister << (uint8_t)TAO::Register::STATES::PRESTATE << account;
            }

            /* Get pre-states on write. */
            if(nFlags & TAO::Register::FLAGS::WRITE  || nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                /* Get the state byte. */
                uint8_t nState = 0; //RESERVED
                tx.ssRegister >> nState;

                /* Check for the pre-state. */
                if(nState != TAO::Register::STATES::PRESTATE)
                    return debug::error(FUNCTION, "register script not in pre-state");

                /* Get the pre-state. */
                tx.ssRegister >> account;
            }

            /* Check ownership of register. */
            if(account.hashOwner != tx.hashGenesis)
                return debug::error(FUNCTION, tx.hashGenesis.ToString(), " caller not authorized to debit from register");

            //TODO: sanitize the account to and ensure that it will be creditable

            /* Parse the account object register. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account object register");

            /* Check for standard types. */
            if(account.Base() != TAO::Register::OBJECTS::ACCOUNT)
                return debug::error(FUNCTION, "cannot debit from non-standard object register");

            /* Check the account balance. */
            if(nAmount > account.get<uint64_t>("balance"))
                return debug::error(FUNCTION, hashFrom.ToString(), " account doesn't have sufficient balance");

            /* Write the new balance to object register. */
            if(!account.Write("balance", account.get<uint64_t>("balance") - nAmount))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Update the state register's timestamp. */
            account.nTimestamp = tx.nTimestamp;
            account.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!account.IsValid())
                return debug::error(FUNCTION, "memory address ", hashFrom.ToString(), " is in invalid state");

            /* Write post-state checksum. */
            if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                tx.ssRegister << (uint8_t)TAO::Register::STATES::POSTSTATE << account.GetHash();

            /* Verify the post-state checksum. */
            if(nFlags & TAO::Register::FLAGS::WRITE || nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                /* Get the state byte. */
                uint8_t nState = 0; //RESERVED
                tx.ssRegister >> nState;

                /* Check for the pre-state. */
                if(nState != TAO::Register::STATES::POSTSTATE)
                    return debug::error(FUNCTION, "register script not in post-state");

                /* Get the post state checksum. */
                uint64_t nChecksum;
                tx.ssRegister >> nChecksum;

                /* Check for matching post states. */
                if(nChecksum != account.GetHash())
                    return debug::error(FUNCTION, "register script has invalid post-state");

                /* Read the register from the database. */
                TAO::Register::State stateTo;
                if(!LLD::regDB->ReadState(hashTo, stateTo))
                    return debug::error(FUNCTION, "register address doesn't exist ", hashTo.ToString());

                /* Write the register to the database. */
                if(!LLD::regDB->WriteState(hashFrom, account, nFlags))
                    return debug::error(FUNCTION, "failed to write new state");

                /* Write the event to the ledger database. */
                if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::legDB->WriteEvent(stateTo.hashOwner, tx.GetHash()))
                    return debug::error(FUNCTION, "failed to rollback event to register DB");
            }

            return true;
        }
    }
}
