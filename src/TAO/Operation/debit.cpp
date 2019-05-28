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
        bool Debit(TAO::Register::Object &account, const uint64_t nAmount)
        {
            /* Parse the account object register. */
            if(!account.Parse())
                return debug::error(FUNCTION, "failed to parse account object register");

            /* Check for standard types. */
            if(account.Base() != TAO::Register::OBJECTS::ACCOUNT)
                return debug::error(FUNCTION, "cannot debit from non-standard object register");

            /* Check the account balance. */
            if(nAmount > account.get<uint64_t>("balance"))
                return debug::error(FUNCTION, hashFrom.SubString(), " account doesn't have sufficient balance");

            /* Write the new balance to object register. */
            if(!account.Write("balance", account.get<uint64_t>("balance") - nAmount))
                return debug::error(FUNCTION, "balance could not be written to object register");

            /* Update the register's checksum. */
            account.SetChecksum();

            /* Check that the register is in a valid state. */
            if(!account.IsValid())
                return debug::error(FUNCTION, "memory address is in invalid state");

            return true;
        }
    }
}
