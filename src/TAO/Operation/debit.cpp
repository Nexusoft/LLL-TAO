/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/state.h>
#include <TAO/Register/objects/account.h>
#include <TAO/Register/objects/token.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Authorizes funds from an account to an account */
        bool Debit(const uint256_t &hashFrom, const uint256_t &hashTo, const uint64_t nAmount, const uint256_t &hashCaller, const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            /* Read the register from the database. */
            TAO::Register::State state;

            /* Write pre-states. */
            if((nFlags & TAO::Register::FLAGS::PRESTATE))
            {
                if(!LLD::regDB->ReadState(hashFrom, state))
                    return debug::error(FUNCTION, "register address doesn't exist ", hashFrom.ToString());

                tx.ssRegister << (uint8_t)TAO::Register::STATES::PRESTATE << state;
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
                tx.ssRegister >> state;
            }

            /* Check ownership of register. */
            if(state.hashOwner != hashCaller)
                return debug::error(FUNCTION, hashCaller.ToString(), " caller not authorized to debit from register");

            /* Check for account object register. */
            if(state.nType == TAO::Register::OBJECT::ACCOUNT)
            {
                /* Get the account object from register. */
                TAO::Register::Account account;
                state >> account;

                /* Check the balance of the from account. */
                if(nAmount > account.nBalance)
                    return debug::error(FUNCTION, hashFrom.ToString(), " account doesn't have sufficient balance");

                /* Change the state of account register. */
                account.nBalance -= nAmount;

                /* Clear the state of register. */
                state.ClearState();
                state.nTimestamp = tx.nTimestamp;
                state << account;
            }

            /* Check for token object register. */
            else if(state.nType == TAO::Register::OBJECT::TOKEN)
            {
                /* Get the account object from register. */
                TAO::Register::Token token;
                state >> token;

                /* Check the balance of the from account. */
                if(nAmount > token.nCurrentSupply)
                    return debug::error(FUNCTION, hashFrom.ToString(), " token doesn't have sufficient balance");

                /* Change the state of token register. */
                token.nCurrentSupply -= nAmount;

                /* Clear the state of register. */
                state.ClearState();
                state.nTimestamp = tx.nTimestamp;
                state << token;
            }
            else
                return debug::error(FUNCTION, hashFrom.ToString(), " is not a valid object register");

            /* Check that the register is in a valid state. */
            if(!state.IsValid())
                return debug::error(FUNCTION, "memory address ", hashFrom.ToString(), " is in invalid state");

            /* Write post-state checksum. */
            if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                tx.ssRegister << (uint8_t)TAO::Register::STATES::POSTSTATE << state.GetHash();

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
                if(nChecksum != state.GetHash())
                    return debug::error(FUNCTION, "register script has invalid post-state");

                /* Write the register to the database. */
                if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::regDB->WriteState(hashFrom, state))
                    return debug::error(FUNCTION, "failed to write new state");

                /* Write the notification foreign index. */
                if(nFlags & TAO::Register::FLAGS::WRITE) //TODO: possibly add some checks for invalid stateTo (wrong token ID)
                {
                    /* Read the register from the database. */
                    TAO::Register::State stateTo;
                    if(!LLD::regDB->ReadState(hashTo, stateTo))
                        return debug::error(FUNCTION, "register address doesn't exist ", hashTo.ToString());

                    /* Write the event to the ledger database. */
                    if(!LLD::legDB->WriteEvent(stateTo.hashOwner, tx.GetHash()))
                        return debug::error(FUNCTION, "failed to rollback event to register DB");
                }
            }

            return true;
        }
    }
}
