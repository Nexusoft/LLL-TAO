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

        /* Commits funds from a coinbase transaction. */
        bool Genesis(const uint256_t& hashAddress, const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            /* Check for reserved values. */
            if(TAO::Register::Reserved(hashAddress))
                return debug::error(FUNCTION, "cannot create genesis from register with reserved address");

            /* Check that a trust key exists. */
            if(LLD::regDB->HasTrust(tx.hashGenesis))
                return debug::error(FUNCTION, "cannot create genesis when already exists");

            /* Read the register from the database. */
            TAO::Register::Object account;

            /* Write pre-states. */
            if((nFlags & TAO::Register::FLAGS::PRESTATE))
            {
                /* Set the register pre-states. */
                if(!LLD::regDB->ReadState(hashAddress, account, nFlags))
                    return debug::error(FUNCTION, "register address doesn't exist ", hashAddress.ToString());

                tx.ssRegister << uint8_t(TAO::Register::STATES::PRESTATE) << account;
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

            /* Check that there is no stake. */
            if(account.get<uint64_t>("stake") != 0)
                return debug::error(FUNCTION, "cannot create genesis with already existing stake");

            /* Check that there is no trust. */
            if(account.get<uint64_t>("trust") != 0)
                return debug::error(FUNCTION, "cannot create genesis with already existing trust");

            /* Check that there is no trust. */
            if(account.get<uint64_t>("balance") == 0)
                return debug::error(FUNCTION, "cannot create genesis with no available balance");

            /* Write the new balance to object register. */
            if(!account.Write("stake", account.get<uint64_t>("balance")))
                return debug::error(FUNCTION, "stake could not be written to object register");

            /* Write the new balance to object register. */
            if(!account.Write("balance", uint64_t(0)))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Update the state register's timestamp. */
            account.nTimestamp = tx.nTimestamp;
            account.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!account.IsValid())
                return debug::error(FUNCTION, "memory address ", hashAddress.ToString(), " is in invalid state");

            /* Write post-state checksum. */
            if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                tx.ssRegister << uint8_t(TAO::Register::STATES::POSTSTATE) << account.GetHash();

            /* Verify the post-state checksum. */
            if(nFlags & TAO::Register::FLAGS::WRITE || nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                /* Check register post-state checksum. */
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

                /* Write the register to the database. */
                if(!LLD::regDB->WriteState(hashAddress, account, nFlags))
                    return debug::error(FUNCTION, "failed to write new state");

                /* Update the register database with the index. */
                if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::regDB->IndexTrust(tx.hashGenesis, hashAddress))
                    return debug::error(FUNCTION, "could not index the address to the genesis");
            }

            return true;
        }
    }
}
