/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/system.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commits funds from a coinbase transaction. */
        bool Genesis(const uint256_t& hashAddress, const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {

            /*
                System Register will have its own LLD records to keep track of all the genesis-id’s that have updated the system value by:

                When network updates, check if genesis exists. If it does not exist, trust += nTrust and stake += stake.
                If the key does exist trust += (nCurrentTrust - LLD::Trust), stake += (nCurrentstake - LLD::Stake)
                When OP::DEBIT from trust account, stake -= nAmount. Update the LLD::Stake value.

                The LLD instance is the TrustDB. This existing key will be like the “Genesis” of the
                Trust account that happens on the very first TRUST block. We could make this a GENESIS like it is now, with an OP::GENESIS op code.

            */

            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAddress))
                return debug::error(FUNCTION, "cannot create genesis from register with reserved address");

            /* Read the register from the database. */
            TAO::Register::Object account;

            /* Write pre-states. */
            if((nFlags & TAO::Register::FLAGS::PRESTATE))
            {
                if(!LLD::regDB->ReadState(hashAddress, account))
                    return debug::error(FUNCTION, "register address doesn't exist ", hashAddress.ToString());

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

            /* Parse the account object register. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account object register");


            return true;
        }
    }
}
