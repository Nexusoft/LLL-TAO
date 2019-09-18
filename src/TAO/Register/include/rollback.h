/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_TAO_REGISTER_INCLUDE_ROLLBACK_H
#define NEXUS_TAO_REGISTER_INCLUDE_ROLLBACK_H

/* Global TAO namespace. */
namespace TAO
{

    /* Register layer. */
    namespace Operation
    {
        /* Forward declarations. */
        class Contract;
    }

    /* Register Layer namespace. */
    namespace Register
    {

        /** Rollback
         *
         *  Rollback the current network state to register pre-states.
         *
         *  @param[in] contract The contract to rollback to pre-states
         *  @param[in] nFlags Ledger level flags to determine if memory or disk
         *
         *  @return True if verified correctly, false otherwise.
         *
         **/
        bool Rollback(const TAO::Operation::Contract& contract, const uint8_t nFlags = TAO::Ledger::FLAGS::BLOCK);
    }

}

#endif
