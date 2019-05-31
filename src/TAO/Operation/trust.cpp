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

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Coinstake operation for a trust transaction. */
        bool Trust(const uint512_t& hashLastTrust, const uint64_t nTrustScore, const uint64_t nCoinstakeReward, const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            /* Read the register from the database. */
            TAO::Register::Object trustAccount;

            /* Write pre-states. */
            if((nFlags & TAO::Register::FLAGS::PRESTATE))
            {
                /* Set the register pre-states. */
                if(!LLD::regDB->ReadTrust(tx.hashGenesis, trustAccount)) //TODO: memory states for this index
                        return debug::error(FUNCTION, "Trust address doesn't exist ", tx.hashGenesis.ToString());

                tx.ssRegister << uint8_t(TAO::Register::STATES::PRESTATE) << trustAccount;
            }

            /* Get pre-states on write. */
            if(nFlags & TAO::Register::FLAGS::WRITE
            || nFlags & TAO::Register::FLAGS::MEMPOOL)
            {
                /* Get the state byte. */
                uint8_t nState = 0; //RESERVED
                tx.ssRegister >> nState;

                /* Check for the pre-state. */
                if(nState != TAO::Register::STATES::PRESTATE)
                        return debug::error(FUNCTION, "Register script not in pre-state");

                /* Get the pre-state. */
                tx.ssRegister >> trustAccount;
            }

            /* Check ownership of register. */
            if(trustAccount.hashOwner != tx.hashGenesis)
                return debug::error(FUNCTION, tx.hashGenesis.ToString(), "Caller not authorized to stake trust for trust account");

            /* Parse the account object register. */
            if(!trustAccount.Parse())
                return debug::error(FUNCTION, "Failed to parse account object register");

            /* Get account starting values */
            uint64_t nBalancePrev = trustAccount.get<uint64_t>("balance");

            /* Update account balance with coinstake reward */
            uint64_t nBalance = nBalancePrev + nCoinstakeReward;

            /* Write the new trust to object register. */
            if(!trustAccount.Write("trust", nTrustScore))
                return debug::error(FUNCTION, "trust could not be written to object register");

            /* Write the new balance to object register. */
            if(!trustAccount.Write("balance", nBalance))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Update the state register's timestamp. */
            trustAccount.nTimestamp = tx.nTimestamp;
            trustAccount.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!trustAccount.IsValid())
                return debug::error(FUNCTION, "trust address ", tx.hashGenesis.ToString(), " is in invalid state");

            /* Write post-state checksum. */
            if((nFlags & TAO::Register::FLAGS::POSTSTATE))
                tx.ssRegister << uint8_t(TAO::Register::STATES::POSTSTATE) << trustAccount.GetHash();

            /* Verify the post-state checksum. */
            if(nFlags & TAO::Register::FLAGS::WRITE
            || nFlags & TAO::Register::FLAGS::MEMPOOL)
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
                if(nChecksum != trustAccount.GetHash())
                    return debug::error(FUNCTION, "register script has invalid post-state");

                /* Update the register database with the index. */
                if((nFlags & TAO::Register::FLAGS::WRITE) && !LLD::regDB->WriteTrust(tx.hashGenesis, trustAccount))
                    return debug::error(FUNCTION, "failed to write new state");

            }

            return true;
        }
    }
}
