/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_TAO_REGISTER_INCLUDE_VERIFY_H
#define NEXUS_TAO_REGISTER_INCLUDE_VERIFY_H

#include <TAO/Ledger/types/transaction.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Register Layer namespace. */
    namespace Register
    {

        /** Verify
         *
         *  Verify the pre-states of a register to current network state.
         *
         *  @param[in] tx The transaction to verify pre-states with.
         *  @param[in] nFlags The flags to verify for     *
         *  @return true if verified correctly.
         *
         **/
        bool Verify(TAO::Ledger::Transaction tx);
        
    }

}

#endif
