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

#include <TAO/Ledger/include/stake.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commits funds from a coinbase transaction. */
        bool Genesis(TAO::Register::Object &trust, const uint64_t nReward, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!trust.Parse())
                return debug::error(FUNCTION, "failed to parse trust account object register");

            /* Check that there is no stake. */
            if(trust.get<uint64_t>("stake") != 0)
                return debug::error(FUNCTION, "cannot create genesis with already existing stake");

            /* Check that there is no trust. */
            if(trust.get<uint64_t>("trust") != 0)
                return debug::error(FUNCTION, "cannot create genesis with already existing trust");

            /* Check available balance to stake. */
            if(trust.get<uint64_t>("balance") == 0)
                return debug::error(FUNCTION, "cannot create genesis with no available balance");

            /* Move existing balance to stake. */
            if(!trust.Write("stake", trust.get<uint64_t>("balance")))
                return debug::error(FUNCTION, "stake could not be written to object register");

            /* Write the stake reward to balance in object register. */
            if(!trust.Write("balance", nReward))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Update the state register's timestamp. */
            trust.nModified = nTimestamp;
            trust.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!trust.IsValid())
                return debug::error(FUNCTION, "memory address is in invalid state");

            return true;
        }
    }
}
