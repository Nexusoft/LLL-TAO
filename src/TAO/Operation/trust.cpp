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

#include <TAO/Ledger/include/constants.h>
#include <TAO/Ledger/include/stake.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commits funds from a coinbase transaction. */
        bool Trust(TAO::Register::Object &trust, const uint64_t nReward, const uint64_t nScore, const uint64_t nTimestamp)
        {
            /* Parse the account object register. */
            if(!trust.Parse())
                return debug::error(FUNCTION, "Failed to parse account object register");

            /* Write the new trust to object register. */
            if(!trust.Write("trust", nScore))
                return debug::error(FUNCTION, "trust could not be written to object register");

            /* Write the new balance to object register. */
            if(!trust.Write("balance", trust.get<uint64_t>("balance") + nReward))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Update the state register's timestamp. */
            trust.SetChecksum();

            /* Check that the register is in a valid state. */
            trust.nModified = nTimestamp;
            if(!trust.IsValid())
                return debug::error(FUNCTION, "trust address is in invalid state");

            return true;
        }
    }
}
