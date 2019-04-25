/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/Operation/include/enum.h>
#include <TAO/Operation/include/operations.h>

#include <TAO/Register/include/enum.h>
#include <TAO/Register/include/state.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Operation Layer namespace. */
    namespace Operation
    {

        /* Commits funds from a coinbase transaction. */
        bool Coinbase(const uint256_t &hashAddress, const uint64_t nAmount, const uint256_t &hashCaller, const uint8_t nFlags, TAO::Ledger::Transaction &tx)
        {
            //make the coinbase able to be credited as a debit check in credit
            //this will allow the number of confirmations to be defined.

            /* Read the to account state. */
            TAO::Register::State state;
            if(!LLD::regDB->ReadState(hashAddress, state))
            {
                //TODO: create the new account if coinbase with no account

                debug::log(0, FUNCTION, "created new account ", hashAddress.ToString(), " for coinbase transaction");
            }

            return true;
        }
    }
}
